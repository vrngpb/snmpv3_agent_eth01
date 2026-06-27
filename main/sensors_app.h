#ifndef SENSORS_APP_H
#define SENSORS_APP_H

#include "esp_err.h"

void sensors_init(void);
void sensors_trigger_temperature_conversion(void);
float sensors_read_temperature(void);
int get_water_leak_status(void);
int get_door_open_status_1(void);
int get_door_open_status_2(void);

#endif