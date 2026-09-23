#pragma once
#include "config.hpp"
#include "driver/uart.h"
#include "hal/uart_types.h"
#include "soc/gpio_num.h"
#include <array>
// Changed this from uart_num_2
constexpr uart_port_t UART_PORT = UART_NUM_1;

constexpr gpio_num_t RS485_TXD_PIN = GPIO_NUM_21;
constexpr gpio_num_t RS485_RXD_PIN = GPIO_NUM_20;

constexpr int RS485_RTS_PIN = UART_PIN_NO_CHANGE;
constexpr int RS485_CTS_PIN = UART_PIN_NO_CHANGE;

constexpr gpio_num_t STATUS_LED = GPIO_NUM_10;

constexpr gpio_num_t PAIR_INIT_GPIO_BTN = GPIO_NUM_9;

constexpr gpio_num_t SOLENOID_PIN = GPIO_NUM_5;
constexpr gpio_num_t DOWNSTREAM_PWR_SWITCH = GPIO_NUM_7;

constexpr int BAUD_RATE = 115200;
constexpr int UART_BUF_SIZE = 1024;
