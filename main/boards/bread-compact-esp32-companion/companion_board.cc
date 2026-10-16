#include <driver/i2c_master.h>
#include <esp_log.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>

#include "application.h"
#include "button.h"
#include "codecs/no_audio_codec.h"
#include "config.h"
#include "face_display.h"
#include "lamp_controller.h"
#include "led/single_led.h"
#include "mcp_server.h"
#include "servo_controller.h"
#include "system_reset.h"
#include "wifi_board.h"

#define TAG "CompanionBoard"

class CompanionBoard : public WifiBoard {
private:
    Button boot_button_;
    Button touch_button_;
    Button asr_button_;

    i2c_master_bus_handle_t display_i2c_bus_ = nullptr;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    Display* display_ = nullptr;
    FaceDisplay* face_display_ = nullptr;  // same object as display_, or nullptr if the OLED failed
    ServoController* servos_ = nullptr;

    void InitializeDisplayI2c() {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = (i2c_port_t)0,
            .sda_io_num = DISPLAY_SDA_PIN,
            .scl_io_num = DISPLAY_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &display_i2c_bus_));
    }

    void InitializeSsd1306Display() {
        esp_lcd_panel_io_i2c_config_t io_config = {
            .dev_addr = DISPLAY_I2C_ADDR,
            .scl_speed_hz = DISPLAY_I2C_HZ,
            .control_phase_bytes = 1,
            .dc_bit_offset = 6,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .flags = {
                .dc_low_on_data = 0,
                .disable_control_phase = 0,
            },
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(display_i2c_bus_, &io_config, &panel_io_));

        ESP_LOGI(TAG, "Install SSD1306 driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = GPIO_NUM_NC;
        panel_config.bits_per_pixel = 1;

        esp_lcd_panel_ssd1306_config_t ssd1306_config = {
            .height = static_cast<uint8_t>(DISPLAY_HEIGHT),
        };
        panel_config.vendor_config = &ssd1306_config;

        ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(panel_io_, &panel_config, &panel_));

        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
        if (esp_lcd_panel_init(panel_) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize display");
            display_ = new NoDisplay();
            return;
        }
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

        face_display_ = new FaceDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                        DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        display_ = face_display_;
    }

    void InitializeServos() {
        servos_ = new ServoController(SERVO_HEAD_GPIO, SERVO_LEFT_HAND_GPIO, SERVO_RIGHT_HAND_GPIO);
        servos_->Start();

        if (face_display_ != nullptr) {
            // Body language follows the face: every emotion shown on the OLED plays a gesture,
            // and the body makes small movements while the assistant is speaking.
            face_display_->OnEmotion([this](const std::string& emotion) { servos_->PlayEmotion(emotion); });
            face_display_->OnActivity([this](FaceDisplay::Activity a) {
                servos_->SetTalking(a == FaceDisplay::Activity::kSpeaking);
            });
        }
    }

    void InitializeButtons() {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << BUILTIN_LED_GPIO,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);

        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            gpio_set_level(BUILTIN_LED_GPIO, 1);
            app.ToggleChatState();
        });

        asr_button_.OnClick([this]() {
            std::string wake_word = "你好小智";
            Application::GetInstance().WakeWordInvoke(wake_word);
        });

        touch_button_.OnPressDown([this]() {
            gpio_set_level(BUILTIN_LED_GPIO, 1);
            Application::GetInstance().StartListening();
        });
        touch_button_.OnPressUp([this]() {
            gpio_set_level(BUILTIN_LED_GPIO, 0);
            Application::GetInstance().StopListening();
        });
    }

    // Tools the AI can call (MCP). They let the assistant move on request,
    // e.g. "turn your head to the left" or "wave at me".
    void InitializeTools() {
        static LampController lamp(LAMP_GPIO);

        auto& mcp = McpServer::GetInstance();

        mcp.AddTool("self.companion.look",
                    "Turn the head left or right. position: -100 = fully left, 0 = straight ahead, 100 = fully right.",
                    PropertyList({Property("position", kPropertyTypeInteger, -100, 100)}),
                    [this](const PropertyList& properties) -> ReturnValue {
                        head_ = properties["position"].value<int>() / 100.0f;
                        servos_->SetPose(head_, left_, right_);
                        return true;
                    });

        mcp.AddTool("self.companion.arms",
                    "Move the left and right hand. 0 = hanging down, 100 = fully raised. "
                    "'Left' and 'right' are from the robot's own point of view.",
                    PropertyList({Property("left", kPropertyTypeInteger, 0, 100),
                                  Property("right", kPropertyTypeInteger, 0, 100)}),
                    [this](const PropertyList& properties) -> ReturnValue {
                        left_ = properties["left"].value<int>() / 100.0f;
                        right_ = properties["right"].value<int>() / 100.0f;
                        servos_->SetPose(head_, left_, right_);
                        return true;
                    });

        mcp.AddTool("self.companion.gesture",
                    "Play a body gesture. name is one of: wave_right, wave_left, wave_both, shake_head "
                    "(means 'no'), look_around, cheer, happy, sad, angry, surprised, think, love, shy, "
                    "sleepy, confused, cool, wink, silly, delicious, rest (return to the neutral pose).",
                    PropertyList({Property("name", kPropertyTypeString)}),
                    [this](const PropertyList& properties) -> ReturnValue {
                        Gesture g;
                        if (!ServoController::GestureFromName(properties["name"].value<std::string>(), g)) {
                            return false;
                        }
                        head_ = left_ = right_ = 0;
                        servos_->PlayGesture(g);
                        return true;
                    });

        mcp.AddTool("self.companion.emotion",
                    "Show an emotion on your face and play the matching body gesture. emotion is one of: "
                    "neutral, happy, laughing, funny, sad, angry, crying, loving, embarrassed, surprised, "
                    "shocked, thinking, winking, cool, relaxed, delicious, kissy, confident, sleepy, silly, confused.",
                    PropertyList({Property("emotion", kPropertyTypeString)}),
                    [this](const PropertyList& properties) -> ReturnValue {
                        if (display_ == nullptr) return false;
                        display_->SetEmotion(properties["emotion"].value<std::string>().c_str());
                        return true;
                    });
    }

    float head_ = 0, left_ = 0, right_ = 0;

public:
    CompanionBoard()
        : WifiBoard(),
          boot_button_(BOOT_BUTTON_GPIO),
          touch_button_(TOUCH_BUTTON_GPIO),
          asr_button_(ASR_BUTTON_GPIO) {
        InitializeDisplayI2c();
        InitializeSsd1306Display();
        InitializeServos();
        InitializeButtons();
        InitializeTools();
    }

    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
#else
        static NoAudioCodecDuplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
#endif
        return &audio_codec;
    }

    virtual Display* GetDisplay() override { return display_; }
};

DECLARE_BOARD(CompanionBoard);
