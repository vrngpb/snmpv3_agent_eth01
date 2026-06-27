cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Note: component_validation.cmake will warn about snmp_lwip component building
# files from lwip/ directory. This is intentional — IDF v6 removed LWIP_SNMP Kconfig,
# so SNMP sources must be built manually. The warnings are cosmetic and harmless.

project(snmp_agent_eth01)
