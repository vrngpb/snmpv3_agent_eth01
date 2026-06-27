set(LWIP_PATH "$ENV{IDF_PATH}/components/lwip/lwip")
file(GLOB SNMP_SOURCES
    "${LWIP_PATH}/src/apps/snmp/*.c"
    "${LWIP_PATH}/src/apps/snmp/snmp_mib2/*.c"
)
# Convert Windows backslashes to CMake-compatible forward slashes
list(TRANSFORM SNMP_SOURCES REPLACE "\\\\" "/")

idf_component_register(
    SRCS ${SNMP_SOURCES}
    INCLUDE_DIRS "."
    REQUIRES lwip mbedtls
)

if(NOT CMAKE_SCRIPT_MODE_FILE)
    set_source_files_properties(${SNMP_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wno-dangling-pointer")
endif()

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    -DLWIP_SNMP=1 -DSNMP_LWIP=1 -DSNMP_USE_RAW=1
    -DLWIP_SNMP_V3=1 -DLWIP_SNMP_V3_MBEDTLS=1
    -DMIB2_STATS=1 -DLWIP_STATS=1
)
