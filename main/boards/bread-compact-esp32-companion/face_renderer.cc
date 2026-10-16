#include "face_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979f;
constexpr int W = FaceRenderer::kWidth;
constexpr int VH = FaceRenderer::kVirtualHeight;

// ---------------------------------------------------------------- 5x7 font
// Classic column-major 5x7 ASCII font, bit0 = top row. Covers 0x20..0x7E.
const uint8_t kFont5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x56, 0x20, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x08, 0x2A, 0x1C, 0x2A, 0x08}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00}, {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06}, {0x32, 0x49, 0x79, 0x41, 0x3E},
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x7F, 0x41, 0x41, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x7F, 0x00}, {0x04, 0x02, 0x01, 0x02, 0x04},
    {0x40, 0x40, 0x40, 0x40, 0x40}, {0x00, 0x01, 0x02, 0x04, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78},
    {0x7F, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20}, {0x38, 0x44, 0x44, 0x48, 0x7F},
    {0x38, 0x54, 0x54, 0x54, 0x18}, {0x08, 0x7E, 0x09, 0x01, 0x02}, {0x0C, 0x52, 0x52, 0x52, 0x3E},
    {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, {0x20, 0x40, 0x44, 0x3D, 0x00},
    {0x7F, 0x10, 0x28, 0x44, 0x00}, {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x18, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, {0x7C, 0x14, 0x14, 0x14, 0x08},
    {0x08, 0x14, 0x14, 0x18, 0x7C}, {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3F, 0x44, 0x40, 0x20}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, {0x1C, 0x20, 0x40, 0x20, 0x1C},
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0C, 0x50, 0x50, 0x50, 0x3C},
    {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, {0x00, 0x00, 0x7F, 0x00, 0x00},
    {0x00, 0x41, 0x36, 0x08, 0x00}, {0x10, 0x08, 0x08, 0x10, 0x08},
};

// ---------------------------------------------------------------- canvas
struct Canvas {
    uint8_t* buf;
    int w;
    int h;

    inline void Px(int x, int y, bool on) const {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        uint8_t& b = buf[(y >> 3) * w + x];
        const uint8_t m = static_cast<uint8_t>(1u << (y & 7));
        if (on) b |= m; else b &= static_cast<uint8_t>(~m);
    }
    inline bool Get(int x, int y) const {
        if (x < 0 || y < 0 || x >= w || y >= h) return false;
        return (buf[(y >> 3) * w + x] >> (y & 7)) & 1;
    }
    void Clear(bool on = false) const { memset(buf, on ? 0xFF : 0x00, static_cast<size_t>(w) * h / 8); }
    void VLine(int x, int y0, int y1, bool on = true) const {
        for (int y = y0; y <= y1; y++) Px(x, y, on);
    }
    void FillRect(int x, int y, int rw, int rh, bool on = true) const {
        for (int j = 0; j < rh; j++)
            for (int i = 0; i < rw; i++) Px(x + i, y + j, on);
    }
    void Disc(float cx, float cy, float r, bool on = true) const {
        int x0 = static_cast<int>(std::floor(cx - r)), x1 = static_cast<int>(std::ceil(cx + r));
        int y0 = static_cast<int>(std::floor(cy - r)), y1 = static_cast<int>(std::ceil(cy + r));
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) {
                float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
                if (dx * dx + dy * dy <= r * r) Px(x, y, on);
            }
    }
    // Thick line, drawn with a round brush.
    void Line(float x0, float y0, float x1, float y1, float thick = 2.0f, bool on = true) const {
        float dx = x1 - x0, dy = y1 - y0;
        int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy)))));
        for (int i = 0; i <= steps; i++) {
            float t = static_cast<float>(i) / steps;
            Disc(x0 + dx * t, y0 + dy * t, thick * 0.5f, on);
        }
    }
    void FillEllipse(float cx, float cy, float rx, float ry, bool on = true) const {
        if (rx <= 0 || ry <= 0) return;
        for (int y = static_cast<int>(std::floor(cy - ry)); y <= static_cast<int>(std::ceil(cy + ry)); y++)
            for (int x = static_cast<int>(std::floor(cx - rx)); x <= static_cast<int>(std::ceil(cx + rx)); x++) {
                float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry;
                if (nx * nx + ny * ny <= 1.0f) Px(x, y, on);
            }
    }
    void RingEllipse(float cx, float cy, float rx, float ry, float thick, bool on = true) const {
        if (rx <= 0 || ry <= 0) return;
        float irx = std::max(0.1f, rx - thick), iry = std::max(0.1f, ry - thick);
        for (int y = static_cast<int>(std::floor(cy - ry)); y <= static_cast<int>(std::ceil(cy + ry)); y++)
            for (int x = static_cast<int>(std::floor(cx - rx)); x <= static_cast<int>(std::ceil(cx + rx)); x++) {
                float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry;
                float ix = (x + 0.5f - cx) / irx, iy = (y + 0.5f - cy) / iry;
                if (nx * nx + ny * ny <= 1.0f && ix * ix + iy * iy > 1.0f) Px(x, y, on);
            }
    }
    // Heart of total width `s` centred on (cx, cy).
    void Heart(float cx, float cy, float s, bool on = true) const {
        if (s < 2) return;
        float unit = s / 2.4f;
        for (int y = static_cast<int>(cy - unit * 1.4f); y <= static_cast<int>(cy + unit * 1.2f); y++)
            for (int x = static_cast<int>(cx - unit * 1.3f); x <= static_cast<int>(cx + unit * 1.3f); x++) {
                float u = (x + 0.5f - cx) / unit;
                float v = -(y + 0.5f - cy) / unit + 0.15f;
                float a = u * u + v * v - 1.0f;
                if (a * a * a - u * u * v * v * v <= 0) Px(x, y, on);
            }
    }
    void Glyph(int x, int y, char c, int scale, bool on = true) const {
        if (c < 0x20 || c > 0x7E) c = '?';
        const uint8_t* g = kFont5x7[c - 0x20];
        for (int col = 0; col < 5; col++)
            for (int row = 0; row < 7; row++)
                if ((g[col] >> row) & 1) FillRect(x + col * scale, y + row * scale, scale, scale, on);
    }
    void Text(int x, int y, const std::string& s, int scale = 1, bool on = true) const {
        for (char c : s) {
            Glyph(x, y, c, scale, on);
            x += 6 * scale;
        }
    }
};

// Reduce UTF-8 to printable ASCII, one '?' per non-ASCII code point.
std::string ToAscii(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.size();) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (c < 0x80) {
            out.push_back(c == '\n' || c == '\r' || c == '\t' ? ' ' : static_cast<char>(c));
            i++;
        } else {
            out.push_back('?');
            i++;
            while (i < in.size() && (static_cast<unsigned char>(in[i]) & 0xC0) == 0x80) i++;
        }
    }
    return out;
}

// Word wrap to `cols` columns.
std::vector<std::string> Wrap(const std::string& text, int cols) {
    std::vector<std::string> lines;
    std::string line, word;
    auto flush_word = [&]() {
        while (static_cast<int>(word.size()) > cols) {  // hard-break very long words (URLs)
            if (!line.empty()) { lines.push_back(line); line.clear(); }
            lines.push_back(word.substr(0, cols));
            word.erase(0, cols);
        }
        if (word.empty()) return;
        if (line.empty()) line = word;
        else if (static_cast<int>(line.size() + 1 + word.size()) <= cols) line += " " + word;
        else { lines.push_back(line); line = word; }
        word.clear();
    };
    for (char c : text) {
        if (c == ' ') flush_word(); else word.push_back(c);
    }
    flush_word();
    if (!line.empty()) lines.push_back(line);
    return lines;
}

// ---------------------------------------------------------------- face model
enum EyeShape { EYE_RECT, EYE_ARC_UP, EYE_ARC_DOWN, EYE_LINE, EYE_HEART, EYE_SHADES };
enum MouthShape { MOUTH_CURVE, MOUTH_RING, MOUTH_WAVY };
enum Extra : uint32_t {
    X_TEARS = 1, X_BLUSH = 2, X_ZZZ = 4, X_HEARTS = 8, X_DOTS = 16, X_QUESTION = 32, X_SWEAT = 64, X_SPARKLE = 128,
};

struct EyeP {
    float w, h, r;       // size and corner radius
    float dx, dy;        // offset from the base position
    float lid_top;       // 0..1 of height covered from the top
    float lid_slant;     // >0 angry (inner lower), <0 sad (inner higher)
    float lid_bottom;    // 0..1 of height covered from the bottom
    float pupil;         // 0 = none, otherwise black pupil radius as fraction of height
};
struct MouthP {
    float curve;         // -1 frown .. +1 smile
    float w;
    float open;          // 0..1 mouth opening
    float dx, dy;
    float skew;          // smirk, -1..1
    float tongue;        // 0..1
    float ring_w, ring_h;
};
struct Cont {            // everything here is smoothly animated
    EyeP eye[2];
    MouthP mouth;
    float bounce;        // vertical bounce amplitude (px)
    float shake;         // horizontal shake amplitude (px)
};
static_assert(sizeof(Cont) % sizeof(float) == 0, "Cont must be floats only");

struct Disc_ {           // values applied on the swap point of a transition
    EyeShape eye_shape[2];
    MouthShape mouth_shape;
    uint32_t extras;
    bool blink;
    bool wander;
};

struct Preset {
    Cont c;
    Disc_ d;
};

EyeP E(float w = 30, float h = 32, float r = 9) {
    return EyeP{w, h, r, 0, 0, 0, 0, 0, 0};
}
MouthP M(float curve = 0.25f, float w = 18, float open = 0) {
    return MouthP{curve, w, open, 0, 0, 0, 0, 8, 8};
}

Preset Base() {
    Preset p;
    p.c.eye[0] = E();
    p.c.eye[1] = E();
    p.c.mouth = M();
    p.c.bounce = 0;
    p.c.shake = 0;
    p.d.eye_shape[0] = p.d.eye_shape[1] = EYE_RECT;
    p.d.mouth_shape = MOUTH_CURVE;
    p.d.extras = 0;
    p.d.blink = true;
    p.d.wander = true;
    return p;
}

Preset MakePreset(const std::string& n) {
    Preset p = Base();
    auto both = [&](auto fn) { fn(p.c.eye[0], 0); fn(p.c.eye[1], 1); };
    auto shape = [&](EyeShape s) { p.d.eye_shape[0] = p.d.eye_shape[1] = s; };

    if (n == "happy") {
        shape(EYE_ARC_UP);
        p.c.mouth = M(0.9f, 30, 0.35f);
        p.d.extras = X_SPARKLE; p.d.blink = false; p.d.wander = false; p.c.bounce = 1.5f;
    } else if (n == "laughing") {
        shape(EYE_ARC_UP);
        p.c.mouth = M(0.25f, 36, 1.0f);
        p.d.blink = false; p.d.wander = false; p.c.bounce = 3.0f;
    } else if (n == "funny") {
        both([](EyeP& e, int) { e.h = 30; e.lid_top = 0.3f; e.lid_bottom = 0.25f; });
        p.c.mouth = M(0.7f, 30, 0.5f); p.c.mouth.tongue = 1;
        p.d.blink = false; p.d.wander = false; p.c.bounce = 1.5f;
    } else if (n == "sad") {
        both([](EyeP& e, int) { e.w = 28; e.lid_slant = -1.0f; e.lid_top = 0.15f; e.dy = 3; });
        p.c.mouth = M(-0.8f, 16); p.c.mouth.dy = 3;
        p.d.wander = false;
    } else if (n == "angry") {
        both([](EyeP& e, int) { e.lid_slant = 1.0f; e.lid_top = 0.25f; });
        p.c.mouth = M(-0.55f, 24); p.c.shake = 1.2f;
        p.d.wander = false; p.d.blink = false;
    } else if (n == "crying") {
        both([](EyeP& e, int) { e.w = 28; e.lid_slant = -1.0f; e.lid_top = 0.2f; e.dy = 2; });
        p.c.mouth = M(-0.7f, 24, 0.55f); p.c.mouth.dy = 2; p.c.shake = 0.5f;
        p.d.extras = X_TEARS; p.d.wander = false; p.d.blink = false;
    } else if (n == "loving") {
        both([](EyeP& e, int) { e.w = 30; e.h = 28; });
        shape(EYE_HEART);
        p.c.mouth = M(0.8f, 22, 0.25f);
        p.d.extras = X_HEARTS; p.d.blink = false; p.d.wander = false; p.c.bounce = 1.0f;
    } else if (n == "embarrassed") {
        both([](EyeP& e, int) { e.w = 26; e.h = 26; e.dy = 3; e.dx = -3; e.lid_top = 0.2f; });
        p.d.mouth_shape = MOUTH_WAVY; p.c.mouth = M(0, 18);
        p.d.extras = X_BLUSH | X_SWEAT; p.d.wander = false;
    } else if (n == "surprised") {
        both([](EyeP& e, int) { e.w = 32; e.h = 36; e.r = 16; e.pupil = 0.33f; });
        p.d.mouth_shape = MOUTH_RING; p.c.mouth = M(0, 10); p.c.mouth.ring_w = 11; p.c.mouth.ring_h = 13;
        p.d.wander = false; p.d.blink = false;
    } else if (n == "shocked") {
        both([](EyeP& e, int) { e.w = 34; e.h = 38; e.r = 17; e.pupil = 0.2f; });
        p.d.mouth_shape = MOUTH_RING; p.c.mouth = M(0, 10); p.c.mouth.ring_w = 17; p.c.mouth.ring_h = 18;
        p.d.extras = X_SWEAT; p.c.shake = 1.0f; p.d.wander = false; p.d.blink = false;
    } else if (n == "thinking") {
        both([](EyeP& e, int) { e.w = 28; e.h = 30; e.dx = 4; e.dy = -4; });
        p.c.eye[1].lid_top = 0.35f; p.c.eye[1].w = 26;
        p.c.mouth = M(0.0f, 14); p.c.mouth.dx = 9;
        p.d.extras = X_DOTS; p.d.wander = false;
    } else if (n == "winking") {
        p.d.eye_shape[1] = EYE_ARC_UP;
        p.c.mouth = M(0.8f, 28, 0.15f);
        p.d.extras = X_SPARKLE; p.d.blink = false; p.d.wander = false;
    } else if (n == "cool") {
        shape(EYE_SHADES);
        p.c.mouth = M(0.5f, 24); p.c.mouth.skew = 0.6f;
        p.d.blink = false; p.d.wander = false;
    } else if (n == "relaxed") {
        shape(EYE_ARC_DOWN);
        p.c.mouth = M(0.5f, 22);
        p.d.blink = false; p.d.wander = false; p.c.bounce = 0.8f;
    } else if (n == "delicious") {
        shape(EYE_ARC_UP);
        p.c.mouth = M(0.8f, 30, 0.55f); p.c.mouth.tongue = 1;
        p.d.extras = X_SPARKLE; p.d.blink = false; p.d.wander = false; p.c.bounce = 1.0f;
    } else if (n == "kissy") {
        both([](EyeP& e, int) { e.w = 26; e.h = 28; e.lid_top = 0.15f; });
        p.d.mouth_shape = MOUTH_RING; p.c.mouth = M(0, 8); p.c.mouth.ring_w = 6; p.c.mouth.ring_h = 6;
        p.d.extras = X_HEARTS; p.d.wander = false;
    } else if (n == "confident") {
        both([](EyeP& e, int) { e.h = 26; e.lid_top = 0.3f; e.lid_slant = 0.2f; e.dy = -1; });
        p.c.mouth = M(0.6f, 24); p.c.mouth.skew = 0.7f; p.c.mouth.dy = -1;
        p.d.wander = false;
    } else if (n == "sleepy") {
        both([](EyeP& e, int) { e.lid_top = 0.65f; e.dy = 3; });
        p.d.mouth_shape = MOUTH_RING; p.c.mouth = M(0, 8); p.c.mouth.ring_w = 5; p.c.mouth.ring_h = 4;
        p.d.extras = X_ZZZ; p.d.wander = false; p.c.bounce = 0.8f;
    } else if (n == "silly") {
        p.c.eye[0] = E(34, 36, 12); p.c.eye[0].dx = 3;
        p.c.eye[1] = E(22, 22, 8);  p.c.eye[1].dx = -3; p.c.eye[1].dy = 4;
        p.c.mouth = M(0.5f, 26, 0.2f); p.c.mouth.tongue = 1;
        p.d.wander = false; p.d.blink = false; p.c.bounce = 1.0f;
    } else if (n == "confused") {
        p.c.eye[1].lid_top = 0.3f; p.c.eye[1].lid_slant = 0.5f;
        p.d.mouth_shape = MOUTH_WAVY; p.c.mouth = M(0, 20);
        p.d.extras = X_QUESTION;
    }
    // "neutral" and every unknown name use the base preset.
    return p;
}

float Approach(float cur, float tgt, float k) { return cur + (tgt - cur) * k; }
float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

}  // namespace

// ---------------------------------------------------------------- implementation
struct FaceRenderer::Impl {
    int out_h;
    std::vector<uint8_t> face_buf;  // 128x64 virtual canvas
    std::vector<uint8_t> out_buf;   // final buffer (128 x out_h)

    Preset cur, tgt;
    std::string emotion = "neutral";

    // transition (squash -> swap -> reopen)
    int trans_phase = 0;
    float trans_t = 0;
    bool have_pending = false;

    // blink
    float blink_t = -1;
    float next_blink = 2500;

    // gaze wander
    float gaze_x = 0, gaze_y = 0, gaze_tx = 0, gaze_ty = 0;
    float next_wander = 1800;

    // activity
    bool speaking = false, listening = false;
    float talk_open = 0, talk_target = 0, talk_timer = 0;
    float listen_amt = 0;

    // text screen / banner
    std::string status, message, banner;
    float banner_left = 0;
    float scroll_ms = 0;

    float t = 0;  // animation clock (ms)
    uint32_t rng = 0x1234567;

    explicit Impl(int oh)
        : out_h(oh),
          face_buf(static_cast<size_t>(W) * VH / 8, 0),
          out_buf(static_cast<size_t>(W) * oh / 8, 0) {
        cur = tgt = MakePreset("neutral");
    }

    float Rand() {  // 0..1
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return static_cast<float>(rng & 0xFFFF) / 65535.0f;
    }

    // ------------------------------------------------------------ update
    void Update(float dt) {
        t += dt;
        const float k = 1.0f - std::exp(-dt / 85.0f);
        {
            float* a = reinterpret_cast<float*>(&cur.c);
            const float* b = reinterpret_cast<const float*>(&tgt.c);
            for (size_t i = 0; i < sizeof(Cont) / sizeof(float); i++) a[i] = Approach(a[i], b[i], k);
        }
        // squash transition between emotions
        if (trans_phase == 1) {
            trans_t += dt;
            if (trans_t >= 70) {
                cur.d = tgt.d;
                trans_phase = 2; trans_t = 0;
            }
        } else if (trans_phase == 2) {
            trans_t += dt;
            if (trans_t >= 90) trans_phase = 0;
        }
        // blink
        if (blink_t >= 0) {
            blink_t += dt;
            if (blink_t > 160) { blink_t = -1; next_blink = 2200 + Rand() * 3500; }
        } else if (cur.d.blink && trans_phase == 0) {
            next_blink -= dt;
            if (next_blink <= 0) blink_t = 0;
        }
        // gaze wander
        if (cur.d.wander) {
            next_wander -= dt;
            if (next_wander <= 0) {
                bool center = Rand() < 0.35f;
                gaze_tx = center ? 0 : (Rand() - 0.5f) * 12;
                gaze_ty = center ? 0 : (Rand() - 0.5f) * 6;
                next_wander = 1200 + Rand() * 2800;
            }
        } else {
            gaze_tx = gaze_ty = 0;
        }
        const float gk = 1.0f - std::exp(-dt / 110.0f);
        gaze_x = Approach(gaze_x, gaze_tx, gk);
        gaze_y = Approach(gaze_y, gaze_ty, gk);
        // talking mouth
        talk_timer -= dt;
        if (speaking && talk_timer <= 0) {
            talk_target = Rand() < 0.2f ? 0.05f : 0.25f + Rand() * 0.75f;
            talk_timer = 70 + Rand() * 90;
        } else if (!speaking) {
            talk_target = 0;
        }
        talk_open = Approach(talk_open, talk_target, 1.0f - std::exp(-dt / 45.0f));
        listen_amt = Approach(listen_amt, listening ? 1.0f : 0.0f, k);
        // banner / scroll
        if (banner_left > 0) banner_left = std::max(0.0f, banner_left - dt);
        scroll_ms += dt;
    }

    // ------------------------------------------------------------ face drawing
    float EyeSquash() const {
        float s = 1.0f;
        if (trans_phase == 1) s = 1.0f - 0.9f * Clamp(trans_t / 70.0f, 0, 1);
        else if (trans_phase == 2) s = 0.1f + 0.9f * Clamp(trans_t / 90.0f, 0, 1);
        if (blink_t >= 0) s *= 1.0f - 0.92f * std::sin(kPi * Clamp(blink_t / 160.0f, 0, 1));
        return s;
    }

    void DrawEye(const Canvas& c, int idx, float ox, float oy, float squash) {
        const EyeP& e = cur.c.eye[idx];
        const EyeShape shape = cur.d.eye_shape[idx];
        const float base_x = idx == 0 ? 38.0f : 90.0f;
        const float base_y = 24.0f;
        const float lscale = 1.0f + 0.08f * listen_amt;
        const float cx = base_x + e.dx + gaze_x + ox;
        const float cy = base_y + e.dy + gaze_y + oy;
        const float w = e.w * lscale;

        switch (shape) {
            case EYE_ARC_UP:
            case EYE_ARC_DOWN: {
                const float h = e.h * 0.5f;
                float prev_x = 0, prev_y = 0;
                for (int i = 0; i <= 16; i++) {
                    float tt = i / 16.0f;
                    float x = cx - w * 0.5f + w * tt;
                    float s = std::sin(kPi * tt);
                    float y = shape == EYE_ARC_UP ? (cy + h * 0.45f - h * 0.9f * s)
                                                  : (cy - h * 0.35f + h * 0.75f * s);
                    if (i > 0) c.Line(prev_x, prev_y, x, y, 5.0f);
                    prev_x = x; prev_y = y;
                }
                break;
            }
            case EYE_LINE:
                c.Line(cx - w * 0.5f, cy, cx + w * 0.5f, cy, 4.0f);
                break;
            case EYE_HEART: {
                float pulse = 1.0f + 0.12f * std::sin(t / 140.0f);
                c.Heart(cx, cy, e.w * 1.05f * pulse);
                break;
            }
            case EYE_SHADES: {
                const float lw = w + 6, lh = 20.0f;
                const float r = 6.0f;
                for (int x = static_cast<int>(cx - lw / 2); x <= static_cast<int>(cx + lw / 2); x++) {
                    float d = std::fabs(x + 0.5f - cx) - (lw / 2 - r);
                    float inset = d > 0 ? r - std::sqrt(std::max(0.0f, r * r - d * d)) : 0;
                    c.VLine(x, static_cast<int>(cy - lh / 2 + inset + 2), static_cast<int>(cy + lh / 2 - inset + 2));
                }
                // glints
                float gx = cx - lw / 2 + 8;
                c.Line(gx, cy - 2, gx + 6, cy - 8, 1.5f, false);
                c.Line(gx + 5, cy + 2, gx + 11, cy - 4, 1.5f, false);
                break;
            }
            case EYE_RECT:
            default: {
                const float h = std::max(2.0f, e.h * lscale * squash);
                const float r = std::min(e.r, std::min(w, h) * 0.5f);
                const float top0 = cy - h * 0.5f, bot0 = cy + h * 0.5f;
                for (int x = static_cast<int>(std::floor(cx - w * 0.5f)); x < static_cast<int>(std::ceil(cx + w * 0.5f)); x++) {
                    float px = x + 0.5f;
                    float d = std::fabs(px - cx) - (w * 0.5f - r);
                    float inset = d > 0 ? r - std::sqrt(std::max(0.0f, r * r - d * d)) : 0;
                    float top = top0 + inset, bot = bot0 - inset;
                    float tt = Clamp((px - (cx - w * 0.5f)) / w, 0, 1);
                    float inner = idx == 0 ? tt : 1.0f - tt;
                    float cut_top = top0 + h * e.lid_top + e.lid_slant * (inner - 0.3f) * h * 0.6f;
                    float cut_bot = bot0 - h * e.lid_bottom;
                    top = std::max(top, cut_top);
                    bot = std::min(bot, cut_bot);
                    if (bot > top) c.VLine(x, static_cast<int>(std::lround(top)), static_cast<int>(std::lround(bot)) - 1);
                }
                if (e.pupil > 0.01f && squash > 0.5f) {
                    c.Disc(cx + gaze_x * 0.3f, cy + gaze_y * 0.3f, e.pupil * h * 0.5f, false);
                }
                break;
            }
        }
    }

    void DrawMouth(const Canvas& c, float ox, float oy) {
        const MouthP& m = cur.c.mouth;
        const float cx = 64.0f + m.dx + ox + gaze_x * 0.15f;
        const float y0 = 50.0f + m.dy + oy;

        if (cur.d.mouth_shape == MOUTH_RING) {
            float rh = m.ring_h + (speaking ? talk_open * 6.0f : 0.0f);
            if (cur.d.extras & X_ZZZ) rh += 2.0f * (1.0f + std::sin(t / 700.0f));  // slow yawn
            c.RingEllipse(cx, y0 + 2, m.ring_w * 0.5f + 1, rh * 0.5f + 1, 2.5f);
            return;
        }
        if (cur.d.mouth_shape == MOUTH_WAVY) {
            float px = 0, py = 0;
            for (int i = 0; i <= 14; i++) {
                float tt = i / 14.0f;
                float x = cx - m.w * 0.5f + m.w * tt;
                float y = y0 + 2.0f * std::sin(tt * 3.0f * kPi + t / 250.0f);
                if (i > 0) c.Line(px, py, x, y, 2.5f);
                px = x; py = y;
            }
            return;
        }
        // curve / open mouth
        const float open = Clamp(m.open + (speaking ? talk_open * (1.0f - m.open * 0.5f) : 0.0f), 0, 1);
        const float depth = 7.0f;
        auto top_y = [&](float tt) {
            float u = 2 * tt - 1;
            return y0 + m.curve * depth * (1 - u * u) + m.skew * (tt - 0.5f) * -8.0f;
        };
        auto bot_y = [&](float tt) {
            float u = 2 * tt - 1;
            return top_y(tt) + open * 15.0f * std::sqrt(std::max(0.0f, 1 - u * u));
        };
        const float w = m.w;
        if (open > 0.05f) {
            for (int x = static_cast<int>(cx - w * 0.5f); x <= static_cast<int>(cx + w * 0.5f); x++) {
                float tt = Clamp((x + 0.5f - (cx - w * 0.5f)) / w, 0, 1);
                c.VLine(x, static_cast<int>(top_y(tt)) - 1, static_cast<int>(bot_y(tt)));
            }
            if (m.tongue > 0.05f && open > 0.25f) {
                c.FillEllipse(cx + 2.0f * std::sin(t / 300.0f), bot_y(0.5f) - 3.0f, 4.5f, 2.5f * m.tongue, false);
            }
        } else {
            float px = 0, py = 0;
            for (int i = 0; i <= 12; i++) {
                float tt = i / 12.0f;
                float x = cx - w * 0.5f + w * tt, y = top_y(tt);
                if (i > 0) c.Line(px, py, x, y, 3.0f);
                px = x; py = y;
            }
            if (m.tongue > 0.05f) {
                c.FillEllipse(cx + 3.0f * std::sin(t / 300.0f), top_y(0.5f) + 3.0f, 5.0f, 4.0f * m.tongue);
            }
        }
    }

    void DrawExtras(const Canvas& c, float ox, float oy) {
        const uint32_t x = cur.d.extras;
        if (x & X_TEARS) {
            for (int side = 0; side < 2; side++) {
                float ex = (side == 0 ? 38.0f - 9 : 90.0f + 9) + ox;
                for (int j = 0; j < 2; j++) {
                    float ph = std::fmod(t / 1100.0f + side * 0.37f + j * 0.5f, 1.0f);
                    float yy = 38.0f + ph * 22.0f + oy;
                    if (yy < 60) {
                        c.Line(ex, 36 + oy, ex, yy, 1.5f);
                        c.Disc(ex, yy, 2.2f);
                    }
                }
            }
        }
        if (x & X_BLUSH) {
            for (int side = 0; side < 2; side++) {
                float bx = side == 0 ? 14.0f : 100.0f;
                for (int i = 0; i < 3; i++) c.Line(bx + i * 5, 46, bx + i * 5 + 4, 40, 1.5f);
            }
        }
        if (x & X_SWEAT) {
            float dy = 2.0f * std::sin(t / 400.0f);
            c.Disc(114, 12 + dy, 3.0f);
            c.Line(114, 5 + dy, 112, 10 + dy, 1.5f);
            c.Line(114, 5 + dy, 116, 10 + dy, 1.5f);
        }
        if (x & X_ZZZ) {
            for (int i = 0; i < 3; i++) {
                float ph = std::fmod(t / 2400.0f + i * 0.33f, 1.0f);
                float zx = 100 + ph * 14 - i * 2;
                float zy = 30 - ph * 26;
                int sc = 1 + (ph > 0.4f ? 1 : 0);
                c.Glyph(static_cast<int>(zx), static_cast<int>(zy), 'z', sc);
            }
        }
        if (x & X_HEARTS) {
            for (int i = 0; i < 2; i++) {
                float ph = std::fmod(t / 1800.0f + i * 0.5f, 1.0f);
                float hx = (i == 0 ? 104.0f : 20.0f) + 4.0f * std::sin(ph * 2 * kPi);
                float hy = 52.0f - ph * 44.0f;
                c.Heart(hx, hy, 9.0f * (1.0f - ph * 0.4f));
            }
        }
        if (x & X_DOTS) {
            int n = 1 + static_cast<int>(t / 450.0f) % 3;
            for (int i = 0; i < n; i++) c.Disc(106 + i * 7, 5 - i * 1.0f, 1.6f + i * 0.5f);
        }
        if (x & X_QUESTION) {
            float bob = 2.0f * std::sin(t / 300.0f);
            c.Glyph(110, static_cast<int>(2 + bob), '?', 3);
        }
        if (x & X_SPARKLE) {
            for (int i = 0; i < 2; i++) {
                float cx = i == 0 ? 116.0f : 11.0f, cy = i == 0 ? 10.0f : 12.0f;
                float s = 3.0f + 3.0f * (0.5f + 0.5f * std::sin(t / 220.0f + i * 2.0f));
                c.Line(cx - s, cy, cx + s, cy, 1.5f);
                c.Line(cx, cy - s, cx, cy + s, 1.5f);
            }
        }
    }

    void DrawFace(const Canvas& c) {
        c.Clear();
        float oy = cur.c.bounce * std::sin(t / 110.0f);
        float ox = cur.c.shake * std::sin(t / 38.0f);
        const float squash = EyeSquash();
        // Sunglasses bridge first so lenses draw over it.
        if (cur.d.eye_shape[0] == EYE_SHADES) {
            c.Line(54, 22 + oy, 74, 22 + oy, 2.5f);
            c.Line(22, 22 + oy, 10, 18 + oy, 2.0f);
            c.Line(106, 22 + oy, 118, 18 + oy, 2.0f);
        }
        DrawEye(c, 0, ox, oy, squash);
        DrawEye(c, 1, ox, oy, squash);
        DrawMouth(c, ox, oy);
        DrawExtras(c, ox, oy);
    }

    // ------------------------------------------------------------ text screen
    void DrawTextScreen(const Canvas& c) {
        c.Clear();
        const int cols = W / 6;
        const int header_h = 9;
        c.FillRect(0, 0, W, header_h, true);
        std::string head = ToAscii(status);
        if (static_cast<int>(head.size()) > cols) head.resize(cols);
        c.Text(1, 1, head, 1, false);
        auto lines = Wrap(ToAscii(message), cols);
        const int rows = (c.h - header_h - 1) / 9;
        int first = 0;
        if (static_cast<int>(lines.size()) > rows) {
            int pages = static_cast<int>(lines.size()) - rows + 1;
            first = static_cast<int>(scroll_ms / 1800.0f) % (pages + 1);
            if (first >= pages) first = 0;
        }
        for (int i = 0; i < rows && first + i < static_cast<int>(lines.size()); i++)
            c.Text(0, header_h + 2 + i * 9, lines[first + i], 1, true);
    }

    void DrawBanner(const Canvas& c) {
        if (banner_left <= 0 || banner.empty()) return;
        const int cols = W / 6;
        std::string s = ToAscii(banner);
        if (static_cast<int>(s.size()) > cols) s.resize(cols);
        c.FillRect(0, c.h - 9, W, 9, true);
        c.Text(1, c.h - 8, s, 1, false);
    }

    const uint8_t* Render() {
        Canvas face{face_buf.data(), W, VH};
        Canvas out{out_buf.data(), W, out_h};
        if (!message.empty()) {
            DrawTextScreen(out);
            return out_buf.data();
        }
        DrawFace(face);
        if (out_h == VH) {
            memcpy(out_buf.data(), face_buf.data(), face_buf.size());
        } else {
            out.Clear();
            for (int y = 0; y < out_h; y++)
                for (int x = 0; x < W; x++)
                    if (face.Get(x, y * 2) || face.Get(x, y * 2 + 1)) out.Px(x, y, true);
        }
        DrawBanner(out);
        return out_buf.data();
    }
};

// ---------------------------------------------------------------- public API
FaceRenderer::FaceRenderer(int out_height)
    : impl_(new Impl(out_height == 32 ? 32 : 64)), out_height_(out_height == 32 ? 32 : 64) {}

FaceRenderer::~FaceRenderer() { delete impl_; }

void FaceRenderer::SetEmotion(const std::string& name) {
    static const struct { const char* from; const char* to; } kAlias[] = {
        {"love", "loving"}, {"heart", "loving"}, {"wink", "winking"}, {"smile", "happy"},
        {"laugh", "laughing"}, {"cry", "crying"}, {"surprise", "surprised"}, {"shock", "shocked"},
        {"think", "thinking"}, {"sleep", "sleepy"}, {"mad", "angry"}, {"kiss", "kissy"},
    };
    std::string n = name;
    for (auto& a : kAlias) if (n == a.from) n = a.to;
    if (n == impl_->emotion) return;
    impl_->emotion = n;
    impl_->tgt = MakePreset(n);
    impl_->trans_phase = 1;
    impl_->trans_t = 0;
    impl_->blink_t = -1;
    impl_->next_blink = 2500;
}
void FaceRenderer::SetSpeaking(bool s) { impl_->speaking = s; }
void FaceRenderer::SetListening(bool l) { impl_->listening = l; }
void FaceRenderer::SetStatusText(const std::string& s) { impl_->status = s; }
void FaceRenderer::SetMessage(const std::string& m) {
    if (m != impl_->message) impl_->scroll_ms = 0;
    impl_->message = m;
}
void FaceRenderer::ShowBanner(const std::string& text, uint32_t ms) {
    impl_->banner = text;
    impl_->banner_left = static_cast<float>(ms);
}
void FaceRenderer::Update(uint32_t dt_ms) { impl_->Update(static_cast<float>(std::min<uint32_t>(dt_ms, 200))); }
const uint8_t* FaceRenderer::Render() { return impl_->Render(); }
