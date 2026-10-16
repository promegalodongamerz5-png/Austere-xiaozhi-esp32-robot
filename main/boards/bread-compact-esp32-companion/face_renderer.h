// Procedural full-screen OLED face renderer (monochrome, SSD1306 page format).
// Pure C++ with no ESP-IDF dependencies so it can be unit-tested on a PC.
#ifndef FACE_RENDERER_H_
#define FACE_RENDERER_H_

#include <cstddef>
#include <cstdint>
#include <string>

class FaceRenderer {
public:
    static constexpr int kWidth = 128;
    static constexpr int kVirtualHeight = 64;  // faces are always designed for 128x64

    // out_height: 64 or 32. For 32 the face is down-sampled, text is drawn natively.
    explicit FaceRenderer(int out_height);
    ~FaceRenderer();

    // Emotion names are the ones sent by the Xiaozhi server (neutral, happy, laughing, ...).
    void SetEmotion(const std::string& name);
    void SetSpeaking(bool speaking);
    void SetListening(bool listening);

    // Text screen (used for Wi-Fi setup / errors). Empty message returns to the face.
    void SetStatusText(const std::string& status);
    void SetMessage(const std::string& message);
    // Short banner at the bottom of the face, disappears after `ms`.
    void ShowBanner(const std::string& text, uint32_t ms);

    void Update(uint32_t dt_ms);
    // Returns a page-format buffer: byte index = page * 128 + x, bit = y % 8, 1 = lit.
    const uint8_t* Render();
    size_t BufferSize() const { return static_cast<size_t>(kWidth) * out_height_ / 8; }
    int out_height() const { return out_height_; }

private:
    struct Impl;
    Impl* impl_;
    int out_height_;
};

#endif  // FACE_RENDERER_H_
