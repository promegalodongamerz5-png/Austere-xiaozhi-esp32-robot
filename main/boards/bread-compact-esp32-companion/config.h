#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// ============================================================================
//  Bread Compact ESP32 Companion
//  ESP32 DevKit + I2S mic + I2S amp + SSD1306 OLED + 3 servos
//  Audio / OLED / button pins are identical to the original bread-compact-esp32.
// ============================================================================

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_METHOD_SIMPLEX

#ifdef AUDIO_I2S_METHOD_SIMPLEX

#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_25
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_26
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_32

#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_33
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_14
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_27

#else

#define AUDIO_I2S_GPIO_WS GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_7

#endif

#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define TOUCH_BUTTON_GPIO       GPIO_NUM_5
#define ASR_BUTTON_GPIO         GPIO_NUM_19
#define BUILTIN_LED_GPIO        GPIO_NUM_2

// ---------------------------------------------------------------- OLED
#define DISPLAY_SDA_PIN GPIO_NUM_4
#define DISPLAY_SCL_PIN GPIO_NUM_15
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  64          // 64 for 128x64 OLEDs, 32 for 128x32 OLEDs
#define DISPLAY_I2C_ADDR 0x3C
#define DISPLAY_I2C_HZ  400000

#define DISPLAY_MIRROR_X true       // flip these if the face is mirrored / upside down
#define DISPLAY_MIRROR_Y true
#define DISPLAY_INVERT_COLOR false  // set true if the face is dark on a lit background
#define FACE_FPS 20

// ---------------------------------------------------------------- Servos
// Power the servos from their own 5 V supply (NOT from the ESP32 3.3 V pin)
// and connect the supply ground to the ESP32 ground.
#define SERVO_HEAD_GPIO        GPIO_NUM_23   // turns the head left/right
#define SERVO_LEFT_HAND_GPIO   GPIO_NUM_13
#define SERVO_RIGHT_HAND_GPIO  GPIO_NUM_12   // GPIO12 is a boot-strapping pin: keep nothing pulling it high at reset

// Pulse range of your servo (SG90 / MG90S: about 500..2400 us).
#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2400

// Head: angle at the centre and how far it may turn to each side.
#define SERVO_HEAD_CENTER_DEG  90
#define SERVO_HEAD_RANGE_DEG   45
#define SERVO_HEAD_REVERSED    false  // true if "right" turns the head to the left

// Hands: servo angle when the hand hangs down / is raised. The two hands are
// usually mounted mirrored, so their numbers run in opposite directions.
// Calibrate: lower the numbers' distance if a hand hits the body.
#define SERVO_LEFT_HAND_DOWN_DEG   10
#define SERVO_LEFT_HAND_UP_DEG     100
#define SERVO_RIGHT_HAND_DOWN_DEG  170
#define SERVO_RIGHT_HAND_UP_DEG    80

// Stop driving the servos this long after a movement ends (quiet, cooler,
// saves current). 0 = keep holding position forever.
#define SERVO_RELAX_MS 1500

// Optional MCP test lamp (an LED the AI can switch on and off).
#define LAMP_GPIO GPIO_NUM_18

#endif  // _BOARD_CONFIG_H_
