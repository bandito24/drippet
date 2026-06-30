file(GLOB SELF_ENV_ESP "core/env/esp32/*.cpp")
file(GLOB CORE_SRC "core/src/*.cpp")
file(GLOB TASK_SRC "tasks/*.cpp")
file(GLOB BLE "ble/*.cpp")

#############
idf_component_register(SRCS "drippet.cpp"
"core/src/clock.cpp"
${ENV_ESP}
${SHARE_SRC}
${SELF_ENV_ESP}

${NODE_ENV_SRC}
${NODE_TASK}
${CORE_SRC}
${TASK_SRC}
${BLE}
REQUIRES bt nvs_flash esp_driver_uart esp_driver_gpio
#
INCLUDE_DIRS
"core/include"  "core/env/esp32"  "tasks"
)
#
#
