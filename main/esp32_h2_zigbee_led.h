#ifndef ESP32_H2_ZIGBEE_LED_H
#define ESP32_H2_ZIGBEE_LED_H

#include <stdint.h>
#include <stdbool.h>
#include "ha/esp_zigbee_ha_standard.h"

/* GPIO pins for RGB LED Strip */
#define LED_GPIO_RED    25  // Adjust to your GPIO pin for the RED wire
#define LED_GPIO_GREEN  26  // Adjust to your GPIO pin for the GREEN wire
#define LED_GPIO_BLUE   27  // Adjust to your GPIO pin for the BLUE wire

/* Zigbee endpoint */
#define LIGHT_ENDPOINT  10

/* LEDC (PWM) configurations */
#define LEDC_TIMER           LEDC_TIMER_0
#define LEDC_MODE            LEDC_LOW_SPEED_MODE
#define LEDC_FREQ_HZ         5000   // 5 kHz PWM frequency
#define LEDC_RESOLUTION      LEDC_TIMER_8_BIT  // 8-bit resolution (0-255 brightness levels)

/* Color XY to RGB conversion precision */
#define MAX_BRIGHTNESS 255  // Max brightness for PWM

/* Structure to represent the RGB light state */
typedef struct {
    bool is_on;              // Light on/off state
    uint8_t brightness;      // Brightness (0-255)
    uint16_t color_x;        // XY color coordinate X
    uint16_t color_y;        // XY color coordinate Y
} rgb_light_t;

/* Function prototypes */
void configure_pwm(void);  // Configure the LEDC (PWM) driver
void zigbee_light_init(void);  // Initialize Zigbee light device
void set_light_state(bool on);  // Turn light on/off
void set_light_brightness(uint8_t brightness);  // Set brightness
void set_light_color(uint16_t x, uint16_t y);  // Set color via XY color coordinates

#endif // ESP32_H2_ZIGBEE_LED_H