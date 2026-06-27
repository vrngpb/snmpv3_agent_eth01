set(SNMPV3_USERNAME         "snmpv3admin"         CACHE STRING "SNMPv3 username")
set(SNMPV3_AUTH_PASSPHRASE  "authpassphrase01"    CACHE STRING "SNMPv3 auth passphrase (min 8 chars, RFC 3414)")
set(SNMPV3_PRIV_PASSPHRASE  "privpassphrase01"    CACHE STRING "SNMPv3 priv passphrase (min 8 chars, RFC 3414)")

idf_component_register(
    SRCS "main.c" "ethernet_app.c" "sensors_app.c" "led_app.c" "snmpv3_usm.c"
    INCLUDE_DIRS "."
    REQUIRES snmp_lwip esp_netif esp_eth esp_event nvs_flash esp_timer esp_driver_gpio spi_flash
             heap esp_system espressif__ds18b20 espressif__onewire_bus
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    -DSNMPV3_USERNAME="${SNMPV3_USERNAME}"
    -DSNMPV3_AUTH_PASSPHRASE="${SNMPV3_AUTH_PASSPHRASE}"
    -DSNMPV3_PRIV_PASSPHRASE="${SNMPV3_PRIV_PASSPHRASE}"
)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-variable -Wno-unused-function
)
