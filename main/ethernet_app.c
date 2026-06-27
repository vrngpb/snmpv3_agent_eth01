#include "esp_eth_netif_glue.h"
#include "esp_eth_driver.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "ethernet_app.h"
#include "led_app.h"

#include "nvs.h"

// Значения по умолчанию
char ip_addr_str[16] = "10.149.130.75";
char gw_addr_str[16] = "10.149.130.65";
char netmask_str[16] = "255.255.255.224";

static const char *TAG = "eth";

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
  esp_netif_t *eth_netif = (esp_netif_t *)arg;
  if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_CONNECTED) {
    status_leds_set_network(true);
    esp_netif_dhcpc_stop(eth_netif);
    esp_netif_ip_info_t ip_info;
    ip_info.ip.addr = esp_ip4addr_aton(ip_addr_str);
    ip_info.gw.addr = esp_ip4addr_aton(gw_addr_str);
    ip_info.netmask.addr = esp_ip4addr_aton(netmask_str);
    esp_netif_set_ip_info(eth_netif, &ip_info);
  } else if (event_base == ETH_EVENT &&
             (event_id == ETHERNET_EVENT_DISCONNECTED ||
              event_id == ETHERNET_EVENT_STOP)) {
    status_leds_set_network(false);
  }
}

static void load_setting(nvs_handle_t handle, const char *key, char *buf, size_t buf_size, bool *dirty)
{
  size_t sz = buf_size;
  if (nvs_get_str(handle, key, buf, &sz) != ESP_OK) {
    nvs_set_str(handle, key, buf);
    *dirty = true;
  }
}

void load_network_settings(void) {
  nvs_handle_t my_handle;
  if (nvs_open("storage", NVS_READWRITE, &my_handle) != ESP_OK) return;

  bool dirty = false;
  load_setting(my_handle, "ip_addr", ip_addr_str, sizeof(ip_addr_str), &dirty);
  load_setting(my_handle, "gw_addr", gw_addr_str, sizeof(gw_addr_str), &dirty);
  load_setting(my_handle, "netmask", netmask_str,  sizeof(netmask_str),  &dirty);

  if (dirty) nvs_commit(my_handle);
  nvs_close(my_handle);
}

esp_err_t ethernet_init_static(void) {
  ESP_ERROR_CHECK(esp_netif_init());

  esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
  esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);

  // Сначала загружаем настройки из памяти
  load_network_settings();

  // Регистрируем обработчик для установки статического IP при подключении кабеля
  esp_event_handler_instance_t instance_connected;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(ETH_EVENT,
                                                      ETHERNET_EVENT_CONNECTED,
                                                      &eth_event_handler,
                                                      eth_netif,
                                                      &instance_connected));

  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();

  esp32_emac_config.smi_gpio.mdc_num = 23;
  esp32_emac_config.smi_gpio.mdio_num = 18;

  // LAN8720A crystal osc → CLKOUT → GPIO 0 (EXT_IN).
  // GPIO 16 is LAN8720A nRST — must NOT be used as clock output.
  esp32_emac_config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
  esp32_emac_config.clock_config.rmii.clock_gpio = 0;

  esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);

  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
  phy_config.reset_gpio_num = 16;

  esp_eth_handle_t eth_handle = NULL;
  esp_err_t err = ESP_FAIL;

  for (int addr = 0; addr <= 7; addr++) {
    phy_config.phy_addr = addr;
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    eth_handle = NULL;
    err = esp_eth_driver_install(&eth_config, &eth_handle);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "PHY found at MDIO address %d", addr);
      break;
    }
    phy->del(phy);
    ESP_LOGW(TAG, "No PHY at MDIO addr %d", addr);
  }

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "No PHY found at addresses 0-7; verify MDC=GPIO23 MDIO=GPIO18");
    return err;
  }

  ESP_ERROR_CHECK(
      esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
  return esp_eth_start(eth_handle);
}
