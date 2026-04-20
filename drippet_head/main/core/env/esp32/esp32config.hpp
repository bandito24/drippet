#pragma once
#include "driver/uart.h"
#include "hal/uart_types.h"
#include "soc/gpio_num.h"
constexpr uart_port_t UART_PORT = UART_NUM_2;

constexpr gpio_num_t RS485_TXD_PIN = GPIO_NUM_4;
constexpr gpio_num_t RS485_RXD_PIN = GPIO_NUM_5;

constexpr int RS485_RTS_PIN = UART_PIN_NO_CHANGE;
constexpr int RS485_CTS_PIN = UART_PIN_NO_CHANGE;

constexpr int BAUD_RATE = 115200;
constexpr int UART_BUF_SIZE = 1024;
