// Three-servo body for the companion: head (left/right) and two hands.
#ifndef SERVO_CONTROLLER_H_
#define SERVO_CONTROLLER_H_

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <string>

enum class Gesture : uint8_t {
    kRest = 0,
    kHappy,
    kLaugh,
    kSad,
    kAngry,
    kSurprised,
    kThink,
    kLove,
    kShy,
    kSleepy,
    kConfused,
    kCool,
    kWink,
    kSilly,
    kDelicious,
    kWaveRight,
    kWaveLeft,
    kWaveBoth,
    kShakeHead,
    kLookAround,
    kCheer,
    kCount
};

class ServoController {
public:
    ServoController(gpio_num_t head_pin, gpio_num_t left_hand_pin, gpio_num_t right_hand_pin);
    ~ServoController() = default;

    // Configure PWM and start the motion task. Call once.
    void Start();

    // Normalised poses. head: -1 (left) .. +1 (right), hands: 0 (down) .. 1 (raised).
    void SetPose(float head, float left_hand, float right_hand);
    void PlayGesture(Gesture gesture);
    // Picks a gesture matching a Xiaozhi emotion name ("happy", "sad", ...).
    void PlayEmotion(const std::string& emotion);
    // Small random movements while the assistant is talking.
    void SetTalking(bool talking) { talking_ = talking; }

    static bool GestureFromName(const std::string& name, Gesture& out);

public:
    struct KeyFrame {
        uint16_t ms;  // time to reach this pose
        float head, left, right;
    };

private:
    struct Cmd {
        bool is_pose;
        Gesture gesture;
        float head, left, right;
    };

    static void TaskEntry(void* arg);
    void Loop();
    bool Play(const KeyFrame* frames, size_t count);  // returns true if interrupted by a new command
    void Execute(const Cmd& cmd);
    void Write(float head, float left, float right, bool force = false);
    void WriteChannel(int index, float angle_deg);
    void Relax();
    bool AtRest() const;
    float Rand();

    gpio_num_t pins_[3];
    QueueHandle_t queue_ = nullptr;
    volatile bool talking_ = false;
    float cur_head_ = 0, cur_left_ = 0, cur_right_ = 0;
    float written_[3] = {-999, -999, -999};
    bool relaxed_ = true;
    int64_t last_motion_us_ = 0;
    uint32_t rng_ = 0xC0FFEE;
};

#endif  // SERVO_CONTROLLER_H_
