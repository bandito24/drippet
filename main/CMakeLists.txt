
file(GLOB CORE_SRC "core/src/*.cpp")
file(GLOB TASK_SRC "tasks/*.cpp")
file(GLOB ENV_ESP "core/env/esp32/*.cpp")
#
idf_component_register(SRCS "drippet.cpp"
"core/env/esp32/clock.cpp"
${CORE_SRC}
${ENV_ESP}
${TASK_SRC}
#
          INCLUDE_DIRS
          "core/include"
)
#
#
