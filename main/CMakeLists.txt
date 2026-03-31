
file(GLOB CORE_SRC "core/src/*.cpp")
file(GLOB TASK_SRC "tasks/*.cpp")
file(GLOB ENV_ESP "core/env/esp32/*.cpp")
file(GLOB BLE "ble/*.cpp")
##
idf_component_register(SRCS "drippet.cpp"
"core/src/clock.cpp"
${CORE_SRC}
${ENV_ESP}
${TASK_SRC}
${BLE}
REQUIRES bt nvs_flash esp_driver_uart
#
INCLUDE_DIRS
"core/include"
)
#
#
