set(ABS_PROJ_ROOT "/Users/char/esp/drippet")

file(GLOB ENV_ESP "${ABS_PROJ_ROOT}/shared/env/esp32/*.cpp")
file(GLOB SHARE_SRC "${ABS_PROJ_ROOT}/shared/src/*.cpp")

file(GLOB SELF_ENV_ESP "core/env/esp32/*.cpp")
file(GLOB CORE_SRC "core/src/*.cpp")
file(GLOB TASK_SRC "tasks/*.cpp")
file(GLOB BLE "ble/*.cpp")

########
idf_component_register(SRCS "drippet.cpp"
"core/src/clock.cpp"
${ENV_ESP}
${SHARE_SRC}
${SELF_ENV_ESP}
${CORE_SRC}
${TASK_SRC}
${BLE}
REQUIRES bt nvs_flash esp_driver_uart
#
INCLUDE_DIRS
"core/include" "${ABS_PROJ_ROOT}/shared/include" "${ABS_PROJ_ROOT}/shared/env/esp32" "core/env/esp32"
)
#
#
