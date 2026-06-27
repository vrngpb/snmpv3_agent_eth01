#ifndef ETHERNET_APP_H
#define ETHERNET_APP_H

#include "esp_err.h"

// Экспортируем переменные для использования в main.c
extern char ip_addr_str[16];
extern char gw_addr_str[16];
extern char netmask_str[16];

void load_network_settings(void);
esp_err_t ethernet_init_static(void);

#endif