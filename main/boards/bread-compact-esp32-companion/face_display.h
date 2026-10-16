#ifndef FACE_DISPLAY_H_
#define FACE_DISPLAY_H_

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <functional>
#include <memory>
#include <string>

#include "display/display.h"
#include "face_renderer.h"

// Full-screen animated face on a monochrome SSD1306 OLED. Does not use LVGL.
class FaceDisplay : public Display {
public:
    enum class Activity { kIdle, kListening, kSpeaking };

    FaceDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width,
                int height, bool mirror_x, bool mirror_y);
    ~FaceDisplay() override;

    void SetStatus(const char* status) override;
    using Display::ShowNotification;
    void ShowNotification(const char* notification, int duration_ms = 3000) override;
    void SetEmotion(const char* emotion) override;
    void SetChatMessage(const char* role, const char* content) override;
    void ClearChatMessages() override;
    void SetPowerSaveMode(bool on) override;
    bool IsMonochrome() const override { return true; }

    // Hooks so the board can move the servos in sync with the face.
    void OnEmotion(std::function<void(const std::string&)> cb) { emotion_cb_ = std::move(cb); }
    void OnActivity(std::function<void(Activity)> cb) { activity_cb_ = std::move(cb); }

private:
    bool Lock(int timeout_ms = 0) override;
    void Unlock() override;
    static void TaskEntry(void* arg);
    void RenderLoop();

    esp_lcd_panel_io_handle_t panel_io_;
    esp_lcd_panel_handle_t panel_;
    std::unique_ptr<FaceRenderer> renderer_;
    SemaphoreHandle_t mutex_ = nullptr;
    TaskHandle_t task_ = nullptr;
    volatile bool running_ = true;
    std::function<void(const std::string&)> emotion_cb_;
    std::function<void(Activity)> activity_cb_;
};

#endif  // FACE_DISPLAY_H_
