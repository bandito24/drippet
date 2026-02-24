
#include "driver/uart.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/uart_types.h"
#include "soc/gpio_num.h"
#include <stdio.h>
#include <string.h>
#define RS485_UART_PORT (UART_NUM_1) // Or UART_NUM_2, etc.
#define RS485_TXD_PIN (GPIO_NUM_17)
#define RS485_RXD_PIN (GPIO_NUM_16)
#define RS485_RTS_PIN                                                          \
  (UART_PIN_NO_CHANGE) // This will control DE/RE on the transceiver
#define RS485_CTS_PIN (UART_PIN_NO_CHANGE) // Not used in half-duplex RS485

// Setup UART buffered IO with event queue
#define BAUD_RATE (115200)
#define UART_BUF_SIZE (1024)

void app_main(void) {

  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));

  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2,
                               GPIO_NUM_4, // TX
                               GPIO_NUM_5, // RX
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, 1024, 0, 0, NULL, 0));

  while (1) {
    const char *msg = "Hello RS485\n";
    uart_write_bytes(UART_NUM_2, msg, strlen(msg));
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
