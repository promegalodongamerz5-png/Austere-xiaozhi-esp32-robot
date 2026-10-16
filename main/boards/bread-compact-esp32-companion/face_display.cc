#include "face_display.h"

#include <esp_log.h>
#include <esp_timer.h>

#include <cstring>

#include "assets/lang_config.h"
#include "config.h"

#define TAG "FaceDisplay"

FaceDisplay::FaceDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width,
                         int height, bool mirror_x, bool mirror_y)
    : panel_io_(panel_io), panel_(panel) {
    width_ = width;
    height_ = height;
    mutex_ = xSemaphoreCreateMutex();
    renderer_ = std::make_unique<FaceRenderer>(height);

    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_, mirror_x, mirror_y));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_, DISPLAY_INVERT_COLOR));

    xTaskCreate(TaskEntry, "face", 5120, this, 2, &task_);
}

FaceDisplay::~FaceDisplay() {
    running_ = false;
    vTaskDelay(pdMS_TO_TICKS(100));
    if (mutex_) vSemaphoreDelete(mutex_);
}

bool FaceDisplay::Lock(int timeout_ms) {
    return xSemaphoreTake(mutex_, timeout_ms <= 0 ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void FaceDisplay::Unlock() { xSemaphoreGive(mutex_); }

void FaceDisplay::TaskEntry(void* arg) {
    static_cast<FaceDisplay*>(arg)->RenderLoop();
    vTaskDelete(nullptr);
}

void FaceDisplay::RenderLoop() {
    const TickType_t period = pdMS_TO_TICKS(1000 / FACE_FPS);
    TickType_t last_wake = xTaskGetTickCount();
    int64_t last_us = esp_timer_get_time();
    while (running_) {
        vTaskDelayUntil(&last_wake, period);
        const int64_t now = esp_timer_get_time();
        const uint32_t dt_ms = static_cast<uint32_t>((now - last_us) / 1000);
        last_us = now;

        const uint8_t* frame = nullptr;
        {
            DisplayLockGuard lock(this);
            renderer_->Update(dt_ms);
            frame = renderer_->Render();  // buffer is only written by this task
        }
        esp_err_t err = esp_lcd_panel_draw_bitmap(panel_, 0, 0, DISPLAY_WIDTH, height_, frame);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "draw_bitmap failed: %s", esp_err_to_name(err));
        }
    }
}

void FaceDisplay::SetStatus(const char* status) {
    if (status == nullptr) return;
    ESP_LOGI(TAG, "Status: %s", status);
    Activity activity = Activity::kIdle;
    if (strcmp(status, Lang::Strings::SPEAKING) == 0) activity = Activity::kSpeaking;
    else if (strcmp(status, Lang::Strings::LISTENING) == 0) activity = Activity::kListening;
    {
        DisplayLockGuard lock(this);
        renderer_->SetStatusText(status);
        renderer_->SetSpeaking(activity == Activity::kSpeaking);
        renderer_->SetListening(activity == Activity::kListening);
    }
    if (activity_cb_) activity_cb_(activity);
}

void FaceDisplay::ShowNotification(const char* notification, int duration_ms) {
    if (notification == nullptr) return;
    DisplayLockGuard lock(this);
    renderer_->ShowBanner(notification, duration_ms > 0 ? duration_ms : 3000);
}

void FaceDisplay::SetEmotion(const char* emotion) {
    if (emotion == nullptr) return;
    {
        DisplayLockGuard lock(this);
        renderer_->SetEmotion(emotion);
    }
    if (emotion_cb_) emotion_cb_(emotion);
}

void FaceDisplay::SetChatMessage(const char* role, const char* content) {
    if (role == nullptr || content == nullptr) return;
    // Only system messages (Wi-Fi setup, errors, activation codes) take over the screen.
    // Normal conversation text is not shown so the face stays visible.
    if (strcmp(role, "system") != 0) return;
    DisplayLockGuard lock(this);
    renderer_->SetMessage(content);
}

void FaceDisplay::ClearChatMessages() {
    DisplayLockGuard lock(this);
    renderer_->SetMessage("");
}

void FaceDisplay::SetPowerSaveMode(bool on) {
    // The face keeps animating; nothing to do (the panel is never switched off here).
    (void)on;
}
