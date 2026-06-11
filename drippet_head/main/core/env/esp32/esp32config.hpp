#pragma once
#include "config.hpp"
#include "driver/uart.h"
#include "hal/uart_types.h"
#include "soc/gpio_num.h"
#include <array>
constexpr uart_port_t UART_PORT = UART_NUM_2;

constexpr gpio_num_t RS485_TXD_PIN = GPIO_NUM_4;
constexpr gpio_num_t RS485_RXD_PIN = GPIO_NUM_5;

constexpr int RS485_RTS_PIN = UART_PIN_NO_CHANGE;
constexpr int RS485_CTS_PIN = UART_PIN_NO_CHANGE;

constexpr gpio_num_t STATUS_LED = GPIO_NUM_13;

constexpr gpio_num_t PAIR_INIT_GPIO_BTN = GPIO_NUM_26;

constexpr gpio_num_t SOLENOID_PIN = GPIO_NUM_21;
constexpr gpio_num_t DOWNSTREAM_PWR_SWITCH = GPIO_NUM_19;
constexpr gpio_num_t valve_3_pin = GPIO_NUM_21;
constexpr gpio_num_t valve_4_pin = GPIO_NUM_22;
constexpr gpio_num_t valve_5_pin = GPIO_NUM_23;
constexpr gpio_num_t valve_6_pin = GPIO_NUM_25;

constexpr int BAUD_RATE = 115200;
constexpr int UART_BUF_SIZE = 1024;
