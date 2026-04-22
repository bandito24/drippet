set(ABS_PROJ_ROOT "/Users/char/esp/drippet")
file(GLOB ENV_ESP "${ABS_PROJ_ROOT}/shared/env/esp32/*.cpp")
file(GLOB CORE_SRC "core/src/*.cpp")
file(GLOB SHARE_SRC "${ABS_PROJ_ROOT}/shared/src/*.cpp")
file(GLOB SELF_ENV_ESP "core/env/esp32/*.cpp")

idf_component_register(SRCS "main.cpp"
${ENV_ESP}
${SELF_ENV_ESP}
${CORE_SRC}
${SHARE_SRC}
REQUIRES esp_driver_uart
INCLUDE_DIRS
"core/include" "${ABS_PROJ_ROOT}/shared/include" "${ABS_PROJ_ROOT}/shared/env/esp32"
)
