#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include "led_app.h"

#ifndef STATUS_LED_POWER_GPIO
#define STATUS_LED_POWER_GPIO -1
#endif

#ifndef STATUS_LED_NETWORK_GPIO
#define STATUS_LED_NETWORK_GPIO 2
#endif

#ifndef STATUS_LED_WARNING_GPIO
#define STATUS_LED_WARNING_GPIO 4
#endif

#ifndef STATUS_LED_POWER_ACTIVE_LEVEL
#define STATUS_LED_POWER_ACTIVE_LEVEL 1
#endif

#ifndef STATUS_LED_NETWORK_ACTIVE_LEVEL
#define STATUS_LED_NETWORK_ACTIVE_LEVEL 1
#endif

#ifndef STATUS_LED_WARNING_ACTIVE_LEVEL
#define STATUS_LED_WARNING_ACTIVE_LEVEL 1
#endif

#define STATUS_LED_TASK_PERIOD_MS 500

static const char *TAG = "STATUS_LED";

static volatile bool s_network_connected = false;
static volatile bool s_warning_active = false;
static TaskHandle_t s_status_led_task_handle = NULL;

static bool led_gpio_is_enabled(gpio_num_t gpio_num)
{
    return GPIO_IS_VALID_OUTPUT_GPIO(gpio_num);
}

static void led_write(gpio_num_t gpio_num, int active_level, bool on)
{
    if (!led_gpio_is_enabled(gpio_num))
    {
        return;
    }

    gpio_set_level(gpio_num, on ? active_level : !active_level);
}

static esp_err_t led_init(gpio_num_t gpio_num, int active_level)
{
    if (!led_gpio_is_enabled(gpio_num))
    {
        return ESP_OK;
    }

    gpio_reset_pin(gpio_num);
    esp_err_t err = gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    if (err != ESP_OK)
    {
        return err;
    }

    led_write(gpio_num, active_level, false);
    return ESP_OK;
}

static void status_leds_task(void *pvParameters)
{
    bool blink_on = false;

    esp_task_wdt_add(NULL);

    while (1)
    {
        blink_on = !blink_on;

        led_write(STATUS_LED_POWER_GPIO, STATUS_LED_POWER_ACTIVE_LEVEL, true);
        led_write(STATUS_LED_NETWORK_GPIO, STATUS_LED_NETWORK_ACTIVE_LEVEL,
                  s_network_connected || blink_on);
        led_write(STATUS_LED_WARNING_GPIO, STATUS_LED_WARNING_ACTIVE_LEVEL,
                  s_warning_active && blink_on);

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(STATUS_LED_TASK_PERIOD_MS));
    }
}

esp_err_t status_leds_init(void)
{
    esp_err_t err = led_init(STATUS_LED_POWER_GPIO, STATUS_LED_POWER_ACTIVE_LEVEL);
    if (err != ESP_OK)
    {
        return err;
    }

    err = led_init(STATUS_LED_NETWORK_GPIO, STATUS_LED_NETWORK_ACTIVE_LEVEL);
    if (err != ESP_OK)
    {
        return err;
    }

    err = led_init(STATUS_LED_WARNING_GPIO, STATUS_LED_WARNING_ACTIVE_LEVEL);
    if (err != ESP_OK)
    {
        return err;
    }

    if (s_status_led_task_handle == NULL)
    {
        BaseType_t created = xTaskCreate(status_leds_task, "status_leds", 2048,
                                         NULL, 4, &s_status_led_task_handle);
        if (created != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to create status LED task");
            return ESP_ERR_NO_MEM;
        }
    }

    ESP_LOGI(TAG, "Status LEDs: power GPIO %d, network GPIO %d, warning GPIO %d",
             STATUS_LED_POWER_GPIO, STATUS_LED_NETWORK_GPIO, STATUS_LED_WARNING_GPIO);

    return ESP_OK;
}

void status_leds_set_network(bool connected)
{
    s_network_connected = connected;
}

void status_leds_set_warning(bool active)
{
    s_warning_active = active;
}