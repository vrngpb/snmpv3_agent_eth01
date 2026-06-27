#ifndef LED_APP_H
#define LED_APP_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t status_leds_init(void);
void status_leds_set_network(bool connected);
void status_leds_set_warning(bool active);

#endif
