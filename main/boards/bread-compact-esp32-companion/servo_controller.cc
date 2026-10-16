#include "servo_controller.h"

#include <esp_log.h>
#include <esp_timer.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include "config.h"

#define TAG "Servos"

namespace {

constexpr ledc_mode_t kMode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t kTimer = LEDC_TIMER_2;
constexpr ledc_timer_bit_t kResolution = LEDC_TIMER_14_BIT;
constexpr uint32_t kMaxDuty = (1u << 14) - 1;
constexpr ledc_channel_t kChannels[3] = {LEDC_CHANNEL_2, LEDC_CHANNEL_3, LEDC_CHANNEL_4};

using KF = ServoController::KeyFrame;

// {ms, head, left hand, right hand}  head: -1 left .. +1 right, hands: 0 down .. 1 up
const KF kHappy[] = {{250, 0, .9f, .9f}, {200, .3f, .5f, .9f}, {200, -.3f, .9f, .5f}, {200, .3f, .5f, .9f}, {200, -.3f, .9f, .5f}, {300, 0, 0, 0}};
const KF kLaugh[] = {{150, .25f, .8f, .8f}, {150, -.25f, .4f, .4f}, {150, .25f, .8f, .8f}, {150, -.25f, .4f, .4f}, {150, .25f, .8f, .8f}, {150, -.25f, .4f, .4f}, {300, 0, 0, 0}};
const KF kSad[] = {{700, -.35f, .12f, .12f}, {900, .35f, .12f, .12f}, {700, 0, 0, 0}};
const KF kAngry[] = {{120, 0, .55f, .55f}, {100, .5f, .6f, .6f}, {100, -.5f, .5f, .5f}, {100, .5f, .6f, .6f}, {100, -.5f, .5f, .5f}, {300, 0, 0, 0}};
const KF kSurprised[] = {{150, 0, 1, 1}, {900, 0, 1, 1}, {400, 0, 0, 0}};
const KF kThink[] = {{500, -.5f, 0, .7f}, {1300, -.5f, 0, .7f}, {500, .2f, 0, .7f}, {500, 0, 0, 0}};
const KF kLove[] = {{300, .3f, .4f, .4f}, {400, -.3f, .6f, .6f}, {400, .3f, .4f, .4f}, {400, -.3f, .6f, .6f}, {300, 0, 0, 0}};
const KF kShy[] = {{300, .7f, .5f, 0}, {1000, .7f, .5f, 0}, {400, 0, 0, 0}};
const KF kSleepy[] = {{900, .3f, 0, 0}, {900, -.3f, 0, 0}, {800, 0, 0, 0}};
const KF kConfused[] = {{250, -.5f, 0, .4f}, {250, .5f, 0, .4f}, {250, -.5f, .4f, 0}, {250, .5f, .4f, 0}, {300, 0, 0, 0}};
const KF kCool[] = {{300, .2f, 0, .8f}, {900, .2f, 0, .8f}, {300, 0, 0, 0}};
const KF kWink[] = {{250, .3f, 0, .7f}, {500, .3f, 0, .7f}, {250, 0, 0, 0}};
const KF kSilly[] = {{150, .6f, 1, 0}, {150, -.6f, 0, 1}, {150, .6f, 1, 0}, {150, -.6f, 0, 1}, {150, .6f, 1, 0}, {150, -.6f, 0, 1}, {300, 0, 0, 0}};
const KF kDelicious[] = {{300, 0, .3f, .3f}, {250, .3f, .5f, .3f}, {250, -.3f, .3f, .5f}, {250, .3f, .5f, .3f}, {250, -.3f, .3f, .5f}, {300, 0, 0, 0}};
const KF kWaveRight[] = {{250, .2f, 0, .8f}, {180, .2f, 0, 1}, {180, .2f, 0, .55f}, {180, .2f, 0, 1}, {180, .2f, 0, .55f}, {180, .2f, 0, 1}, {300, 0, 0, 0}};
const KF kWaveLeft[] = {{250, -.2f, .8f, 0}, {180, -.2f, 1, 0}, {180, -.2f, .55f, 0}, {180, -.2f, 1, 0}, {180, -.2f, .55f, 0}, {180, -.2f, 1, 0}, {300, 0, 0, 0}};
const KF kWaveBoth[] = {{250, 0, .8f, .8f}, {180, 0, 1, .55f}, {180, 0, .55f, 1}, {180, 0, 1, .55f}, {180, 0, .55f, 1}, {300, 0, 0, 0}};
const KF kShakeHead[] = {{150, -.8f, 0, 0}, {300, .8f, 0, 0}, {300, -.8f, 0, 0}, {300, .8f, 0, 0}, {200, 0, 0, 0}};
const KF kLookAround[] = {{400, -.9f, 0, 0}, {900, -.9f, 0, 0}, {700, .9f, 0, 0}, {900, .9f, 0, 0}, {400, 0, 0, 0}};
const KF kCheer[] = {{250, 0, 1, 1}, {150, .2f, .6f, 1}, {150, -.2f, 1, .6f}, {150, .2f, .6f, 1}, {150, -.2f, 1, .6f}, {300, 0, 0, 0}};
const KF kRest[] = {{350, 0, 0, 0}};

struct GestureEntry {
    const char* name;
    const KF* frames;
    size_t count;
};
#define G(arr) arr, sizeof(arr) / sizeof(arr[0])
const GestureEntry kTable[] = {
    {"rest", G(kRest)},          {"happy", G(kHappy)},         {"laugh", G(kLaugh)},
    {"sad", G(kSad)},            {"angry", G(kAngry)},         {"surprised", G(kSurprised)},
    {"think", G(kThink)},        {"love", G(kLove)},           {"shy", G(kShy)},
    {"sleepy", G(kSleepy)},      {"confused", G(kConfused)},   {"cool", G(kCool)},
    {"wink", G(kWink)},          {"silly", G(kSilly)},         {"delicious", G(kDelicious)},
    {"wave_right", G(kWaveRight)}, {"wave_left", G(kWaveLeft)}, {"wave_both", G(kWaveBoth)},
    {"shake_head", G(kShakeHead)}, {"look_around", G(kLookAround)}, {"cheer", G(kCheer)},
};
static_assert(sizeof(kTable) / sizeof(kTable[0]) == static_cast<size_t>(Gesture::kCount),
              "gesture table must match the Gesture enum");

float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float SmoothStep(float x) { return x * x * (3 - 2 * x); }

}  // namespace

ServoController::ServoController(gpio_num_t head_pin, gpio_num_t left_hand_pin, gpio_num_t right_hand_pin)
    : pins_{head_pin, left_hand_pin, right_hand_pin} {}

float ServoController::Rand() {
    rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
    return static_cast<float>(rng_ & 0xFFFF) / 65535.0f;
}

void ServoController::Start() {
    ledc_timer_config_t timer = {.speed_mode = kMode,
                                 .duty_resolution = kResolution,
                                 .timer_num = kTimer,
                                 .freq_hz = 50,
                                 .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&timer));
    for (int i = 0; i < 3; i++) {
        if (pins_[i] == GPIO_NUM_NC) continue;
        ledc_channel_config_t ch = {.gpio_num = pins_[i],
                                    .speed_mode = kMode,
                                    .channel = kChannels[i],
                                    .intr_type = LEDC_INTR_DISABLE,
                                    .timer_sel = kTimer,
                                    .duty = 0,
                                    .hpoint = 0};
        ESP_ERROR_CHECK(ledc_channel_config(&ch));
    }
    queue_ = xQueueCreate(1, sizeof(Cmd));
    xTaskCreate(TaskEntry, "servo", 3072, this, 3, nullptr);
    ESP_LOGI(TAG, "Servos started (head=%d, left=%d, right=%d)", pins_[0], pins_[1], pins_[2]);
}

bool ServoController::GestureFromName(const std::string& name, Gesture& out) {
    for (size_t i = 0; i < sizeof(kTable) / sizeof(kTable[0]); i++) {
        if (name == kTable[i].name) {
            out = static_cast<Gesture>(i);
            return true;
        }
    }
    return false;
}

void ServoController::SetPose(float head, float left_hand, float right_hand) {
    if (!queue_) return;
    Cmd cmd{true, Gesture::kRest, Clamp(head, -1, 1), Clamp(left_hand, 0, 1), Clamp(right_hand, 0, 1)};
    xQueueOverwrite(queue_, &cmd);  // latest command wins
}

void ServoController::PlayGesture(Gesture gesture) {
    if (!queue_) return;
    Cmd cmd{false, gesture, 0, 0, 0};
    xQueueOverwrite(queue_, &cmd);
}

void ServoController::PlayEmotion(const std::string& e) {
    Gesture g;
    if (e == "happy") g = Gesture::kHappy;
    else if (e == "laughing") g = Gesture::kLaugh;
    else if (e == "funny" || e == "silly") g = Gesture::kSilly;
    else if (e == "sad" || e == "crying") g = Gesture::kSad;
    else if (e == "angry") g = Gesture::kAngry;
    else if (e == "loving" || e == "kissy") g = Gesture::kLove;
    else if (e == "embarrassed") g = Gesture::kShy;
    else if (e == "surprised" || e == "shocked") g = Gesture::kSurprised;
    else if (e == "thinking") g = Gesture::kThink;
    else if (e == "winking") g = Gesture::kWink;
    else if (e == "cool" || e == "confident") g = Gesture::kCool;
    else if (e == "delicious") g = Gesture::kDelicious;
    else if (e == "sleepy") g = Gesture::kSleepy;
    else if (e == "confused") g = Gesture::kConfused;
    else return;  // neutral / relaxed / unknown: keep whatever the body is doing
    PlayGesture(g);
}

void ServoController::TaskEntry(void* arg) { static_cast<ServoController*>(arg)->Loop(); }

bool ServoController::AtRest() const {
    return std::fabs(cur_head_) < 0.05f && cur_left_ < 0.05f && cur_right_ < 0.05f;
}

void ServoController::Loop() {
    Write(0, 0, 0, true);
    last_motion_us_ = esp_timer_get_time();
    int64_t next_talk_us = 0;
    for (;;) {
        Cmd cmd;
        if (xQueueReceive(queue_, &cmd, pdMS_TO_TICKS(40)) == pdTRUE) {
            Execute(cmd);
            continue;
        }
        const int64_t now = esp_timer_get_time();
        if (talking_ && now >= next_talk_us) {
            // little head sways and hand lifts while speaking
            float h = (Rand() - 0.5f) * 0.7f;
            bool left = Rand() < 0.5f;
            float lift = 0.15f + Rand() * 0.3f;
            KF kf[] = {{260, h, left ? lift : 0.0f, left ? 0.0f : lift}, {260, h * 0.3f, 0, 0}};
            Play(kf, 2);
            next_talk_us = esp_timer_get_time() + static_cast<int64_t>((900 + Rand() * 2200) * 1000);
        } else if (!talking_ && SERVO_RELAX_MS > 0 && !relaxed_ && AtRest() &&
                   now - last_motion_us_ > static_cast<int64_t>(SERVO_RELAX_MS) * 1000) {
            Relax();
        }
    }
}

void ServoController::Execute(const Cmd& cmd) {
    if (cmd.is_pose) {
        KF kf = {300, cmd.head, cmd.left, cmd.right};
        Play(&kf, 1);
        return;
    }
    const GestureEntry& g = kTable[static_cast<size_t>(cmd.gesture)];
    Play(g.frames, g.count);
}

bool ServoController::Play(const KF* frames, size_t count) {
    constexpr int kStepMs = 20;
    for (size_t i = 0; i < count; i++) {
        const float h0 = cur_head_, l0 = cur_left_, r0 = cur_right_;
        const int steps = std::max<int>(1, frames[i].ms / kStepMs);
        for (int s = 1; s <= steps; s++) {
            const float k = SmoothStep(static_cast<float>(s) / steps);
            cur_head_ = h0 + (frames[i].head - h0) * k;
            cur_left_ = l0 + (frames[i].left - l0) * k;
            cur_right_ = r0 + (frames[i].right - r0) * k;
            Write(cur_head_, cur_left_, cur_right_);
            vTaskDelay(pdMS_TO_TICKS(kStepMs));
            if (uxQueueMessagesWaiting(queue_) > 0) return true;
        }
    }
    last_motion_us_ = esp_timer_get_time();
    return false;
}

void ServoController::Write(float head, float left, float right, bool force) {
    float head_deg = SERVO_HEAD_CENTER_DEG + (SERVO_HEAD_REVERSED ? -1.0f : 1.0f) * head * SERVO_HEAD_RANGE_DEG;
    float left_deg = SERVO_LEFT_HAND_DOWN_DEG + (SERVO_LEFT_HAND_UP_DEG - SERVO_LEFT_HAND_DOWN_DEG) * left;
    float right_deg = SERVO_RIGHT_HAND_DOWN_DEG + (SERVO_RIGHT_HAND_UP_DEG - SERVO_RIGHT_HAND_DOWN_DEG) * right;
    const float deg[3] = {head_deg, left_deg, right_deg};
    for (int i = 0; i < 3; i++) {
        float d = Clamp(deg[i], 0.0f, 180.0f);
        if (force || relaxed_ || std::fabs(d - written_[i]) >= 0.25f) {
            WriteChannel(i, d);
            written_[i] = d;
        }
    }
    relaxed_ = false;
}

void ServoController::WriteChannel(int index, float angle_deg) {
    if (pins_[index] == GPIO_NUM_NC) return;
    const float pulse_us = SERVO_MIN_PULSE_US + (angle_deg / 180.0f) * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);
    const uint32_t duty = static_cast<uint32_t>(pulse_us * kMaxDuty / 20000.0f);
    ledc_set_duty(kMode, kChannels[index], duty);
    ledc_update_duty(kMode, kChannels[index]);
}

void ServoController::Relax() {
    // Stop sending pulses so the servos go quiet and do not buzz or heat up.
    for (int i = 0; i < 3; i++) {
        if (pins_[i] != GPIO_NUM_NC) ledc_stop(kMode, kChannels[i], 0);
    }
    relaxed_ = true;
}
