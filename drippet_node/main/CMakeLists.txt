#Node Specific
set(ABS_PROJ_ROOT "/Users/char/esp/drippet")


set(NODE_CORE_INCLUDE "${ABS_PROJ_ROOT}/drippet_node/main/core/include")
set(NODE_ENV "${ABS_PROJ_ROOT}/drippet_node/main/core/env/esp32")
file(GLOB NODE_CORE_SRC "${ABS_PROJ_ROOT}/drippet_node/main/core/src/*.cpp")
file(GLOB NODE_ENV_SRC "${NODE_ENV}/*.cpp")

file(GLOB ENV_ESP "${ABS_PROJ_ROOT}/shared/env/esp32/*.cpp")
file(GLOB SHARE_SRC "${ABS_PROJ_ROOT}/shared/src/*.cpp")



idf_component_register(SRCS "main.cpp"
${ENV_ESP}
${NODE_CORE_SRC}

${NODE_ENV_SRC}
${SHARE_SRC}
REQUIRES esp_driver_uart
INCLUDE_DIRS
"${NODE_CORE_INCLUDE}" "${ABS_PROJ_ROOT}/shared/include" "${ABS_PROJ_ROOT}/shared/env/esp32" "${NODE_ENV}"
)
