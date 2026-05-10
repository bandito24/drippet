set(ABS_PROJ_ROOT "/Users/char/esp/drippet")



#Specific to the node task for head emulating the first node
set(NODE_CORE_INCLUDE "${ABS_PROJ_ROOT}/drippet_node/main/core/include")
set(NODE_ENV "${ABS_PROJ_ROOT}/drippet_node/main/core/env/esp32")
file(GLOB NODE_CORE_SRC "${ABS_PROJ_ROOT}/drippet_node/main/core/src/*.cpp")
file(GLOB NODE_ENV_SRC "${NODE_ENV}/*.cpp")

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

${NODE_ENV_SRC}
${NODE_TASK}
${CORE_SRC}
${TASK_SRC}
${BLE}
REQUIRES bt nvs_flash esp_driver_uart esp_driver_gpio
#
INCLUDE_DIRS
"core/include" "${NODE_CORE_INCLUDE}" "${ABS_PROJ_ROOT}/shared/include" "${ABS_PROJ_ROOT}/shared/env/esp32" "core/env/esp32" ${NODE_ENV}
)
#
#
