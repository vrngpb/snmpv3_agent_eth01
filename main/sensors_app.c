#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "onewire_bus.h"
#include "ds18b20.h"
#include "sensors_app.h"
#include "driver/gpio.h"

static const char *TAG = "SENSORS";
#define ONEWIRE_BUS_GPIO    32 
#define WATER_LEAK_GPIO     33
#define DOOR_OPEN_1_GPIO    14
#define DOOR_OPEN_2_GPIO    15

static ds18b20_device_handle_t ds18b20_dev = NULL;

void sensors_init(void) {
    // Инициализация датчика протечки Gidrolock WSP
    gpio_reset_pin(WATER_LEAK_GPIO);
    gpio_set_direction(WATER_LEAK_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(WATER_LEAK_GPIO, GPIO_PULLUP_ONLY);
    ESP_LOGI(TAG, "Water leak sensor initialized on GPIO %d", WATER_LEAK_GPIO);


     // Инициализация датчиков открывания дверей
    gpio_reset_pin(DOOR_OPEN_1_GPIO);
    gpio_set_direction(DOOR_OPEN_1_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DOOR_OPEN_1_GPIO, GPIO_PULLUP_ONLY);
    ESP_LOGI(TAG, "Door sensor 1 initialized on GPIO %d", DOOR_OPEN_1_GPIO);

    gpio_reset_pin(DOOR_OPEN_2_GPIO);
    gpio_set_direction(DOOR_OPEN_2_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DOOR_OPEN_2_GPIO, GPIO_PULLUP_ONLY);
    ESP_LOGI(TAG, "Door sensor 2 initialized on GPIO %d", DOOR_OPEN_2_GPIO);

    onewire_bus_handle_t bus = NULL;
    onewire_bus_config_t bus_config = { .bus_gpio_num = ONEWIRE_BUS_GPIO };
    onewire_bus_rmt_config_t rmt_config = { .max_rx_bytes = 10 };

    // 1. Создаем шину RMT
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus));

    // 2. Инициализируем итератор поиска
    onewire_device_iter_handle_t iter = NULL;
    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));
    onewire_device_t next_dev;

    // 3. Ищем первое устройство и создаем дескриптор DS18B20
    if (onewire_device_iter_get_next(iter, &next_dev) == ESP_OK) {
        ds18b20_config_t ds_cfg = {}; 
        
        // Используем точное имя из вашего grep
        if (ds18b20_new_device_from_enumeration(&next_dev, &ds_cfg, &ds18b20_dev) == ESP_OK) {
            ESP_LOGI(TAG, "DS18B20 initialized on GPIO %d", ONEWIRE_BUS_GPIO);
        } else {
            ESP_LOGE(TAG, "Failed to create DS18B20 handle");
        }
    } else {
        ESP_LOGW(TAG, "No DS18B20 found!");
    }
    onewire_del_device_iter(iter);
}

void sensors_trigger_temperature_conversion(void) {
    if (ds18b20_dev) {
        ds18b20_trigger_temperature_conversion(ds18b20_dev);
    }
}

float sensors_read_temperature(void) {
    float temp = -127.0f;
    if (ds18b20_dev) {
        ds18b20_get_temperature(ds18b20_dev, &temp);
    }
    return temp;
}

int get_water_leak_status(void) {
    // Если датчик замкнут (протечка), пин притянется к земле = 0.
    // Если сухо = 1 (благодаря PULLUP).
    // Возвращаем 1 в случае утечки (логический 1 = тревога).
    return (gpio_get_level(WATER_LEAK_GPIO) == 0) ? 1 : 0;
}

// Функции для получения состояния датчиков открывания дверей
int get_door_open_status_1(void) {
    // Согласно hardware_analysis.md:
    // Дверь закрыта -> Геркон замкнут на GND -> PIN 0.
    // Дверь открыта -> Геркон разомкнут -> PIN 1 (PULLUP).
    // Возвращаем 1 в случае открытия (логический 1 = тревога).
    return (gpio_get_level(DOOR_OPEN_1_GPIO) == 1) ? 1 : 0;
}

int get_door_open_status_2(void) {
    // Аналогично первой двери: возвращаем 1, если контакт разомкнут (дверь открыта).
    return (gpio_get_level(DOOR_OPEN_2_GPIO) == 1) ? 1 : 0;
}