#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_zigbee_core.h"
#include "ha/esp_zigbee_ha_standard.h"

/* Zigbee configuration */
#define MAX_CHILDREN                      10                                    /* the max number of connected devices */
#define INSTALLCODE_POLICY_ENABLE         false                                 /* enable the install code policy for security */
#define HA_COLOR_DIMMABLE_LIGHT_ENDPOINT  10                                    /* Zigbee endpoint */
#define ESP_ZB_PRIMARY_CHANNEL_MASK       ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK  /* Zigbee primary channel mask */

/* Basic manufacturer information */
#define ESP_MANUFACTURER_NAME "ESPRESSIF"
#define ESP_MODEL_IDENTIFIER "ESP32-H2"

/* Zigbee role and radio configuration */
#define ESP_ZB_ZR_CONFIG()                                                              \
    {                                                                                   \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,                                       \
        .install_code_policy = INSTALLCODE_POLICY_ENABLE,                               \
        .nwk_cfg.zczr_cfg = {                                                           \
            .max_children = MAX_CHILDREN,                                               \
        },                                                                              \
    }

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                           \
    {                                                           \
        .radio_mode = ZB_RADIO_MODE_NATIVE,                     \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                            \
    {                                                           \
        .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,   \
    }

/* LED Pins */
#define RED_PIN    25
#define GREEN_PIN  26
#define BLUE_PIN   27

static const char *TAG = "RGB_LED_CONTROLLER";

// Configure LEDC PWM timers and channels for RGB pins
static void configure_ledc(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 5000,  // 5 kHz PWM frequency
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel[] = {
        {
            .channel    = LEDC_CHANNEL_0,
            .duty       = 0,
            .gpio_num   = RED_PIN,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0,
        },
        {
            .channel    = LEDC_CHANNEL_1,
            .duty       = 0,
            .gpio_num   = GREEN_PIN,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0,
        },
        {
            .channel    = LEDC_CHANNEL_2,
            .duty       = 0,
            .gpio_num   = BLUE_PIN,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0,
        }
    };

    // Configure LEDC channels for RED, GREEN, BLUE
    for (int i = 0; i < 3; i++) {
        ledc_channel_config(&ledc_channel[i]);
    }
}

// Function to update RGB LED colors
void set_rgb_color(uint8_t r, uint8_t g, uint8_t b) {
    // Scale values for 8-bit to 10-bit (0–255 → 0–1023)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, r * 4);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, g * 4);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, b * 4);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);

    ESP_LOGI(TAG, "RGB Color Set to: R=%d, G=%d, B=%d", r, g, b);
}

// Zigbee attribute handler to manage incoming Zigbee commands
static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message) {
    esp_err_t ret = ESP_OK;
    uint8_t r = 0, g = 0, b = 0;

    ESP_LOGI(TAG, "Received Zigbee message: endpoint(%d), cluster(0x%X), attribute(0x%X), data size(%d)",
             message->info.dst_endpoint, message->info.cluster, message->attribute.id, message->attribute.data.size);

    if (message->info.dst_endpoint == HA_COLOR_DIMMABLE_LIGHT_ENDPOINT) {
        switch (message->info.cluster) {
            case ESP_ZB_ZCL_CLUSTER_ID_COLOR_CONTROL:
                if (message->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U16) {
                    r = *(uint16_t *)message->attribute.data.value >> 8; // Extract Red value
                } else if (message->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U16) {
                    g = *(uint16_t *)message->attribute.data.value >> 8; // Extract Green value
                }
                b = ((r + g) >> 1) & 0xFF; // Compute Blue as midpoint
                set_rgb_color(r, g, b);
                break;
            default:
                ESP_LOGW(TAG, "Unhandled cluster: 0x%X", message->info.cluster);
        }
    }

    return ret;
}

// Zigbee action handler for callbacks
static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message) {
    esp_err_t ret = ESP_OK;
    if (callback_id == ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID) {
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
    } else {
        ESP_LOGW(TAG, "Unhandled Zigbee action callback: 0x%X", callback_id);
    }
    return ret;
}

// Zigbee task to initialize and run the stack
static void esp_zb_task(void *pvParameters) {
    // Zigbee Router Configuration
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZR_CONFIG();
    esp_zb_init(&zb_nwk_cfg);

    // Configure RGB Endpoint
    esp_zb_color_dimmable_light_cfg_t light_cfg = ESP_ZB_DEFAULT_COLOR_DIMMABLE_LIGHT_CONFIG();
    esp_zb_ep_list_t *light_endpoint = esp_zb_color_dimmable_light_ep_create(HA_COLOR_DIMMABLE_LIGHT_ENDPOINT, &light_cfg);

    // Register endpoint
    esp_zb_device_register(light_endpoint);

    // Register Zigbee Handlers
    esp_zb_core_action_handler_register(zb_action_handler);

    // Start Zigbee Stack
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

// Application main entry point
void app_main(void) {
    // Initialize non-volatile storage
    ESP_ERROR_CHECK(nvs_flash_init());

    // Configure RGB PWM
    configure_ledc();

    // Configure Zigbee settings
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    // Create and start Zigbee task
    xTaskCreate(esp_zb_task, "Zigbee_Main_Task", 8192, NULL, 5, NULL);
}