
#include "esp_button.hpp"
#include "freertos/idf_additions.h"
Esp_Err_t EspButton::init() {
  esp_err_t rc = ESP_OK;

  rc |= gpio_reset_pin(this->gpio);
  gpio_config_t conf = {.pin_bit_mask = 1ULL << PAIR_INIT_GPIO_BTN,
                        .mode = GPIO_MODE_INPUT,
                        .pull_up_en = GPIO_PULLUP_ENABLE,
                        .pull_down_en = GPIO_PULLDOWN_DISABLE,
                        .intr_type = GPIO_INTR_NEGEDGE};

  rc |= gpio_config(&conf);
  esp_err_t install_err = gpio_install_isr_service(0);
  if (install_err != ESP_OK) {
    Logger::log_simple("Failed to install GPIO ISR service: %s",
                       esp_err_to_name(install_err));
    return install_err;
  }
  esp_err_t add_err = gpio_isr_handler_add(
      this->gpio, EspButton::gpio_isr_handler, this->pairing_handle);
  if (add_err != ESP_OK) {
    Logger::log_simple("Failed to add ISR handler for GPIO %d: %s", this->gpio,
                       esp_err_to_name(add_err));
    gpio_uninstall_isr_service();
    return add_err;
  }

  return rc;
}

void IRAM_ATTR EspButton::gpio_isr_handler(void *arg) {
  TaskHandle_t handle = static_cast<TaskHandle_t>(arg);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(handle, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}
