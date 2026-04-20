#pragma once
#include "constants.hpp"
#include "protocol_types.hpp"

struct Driver {

  virtual Esp_Err_t init() = 0;
  virtual byte_count send(const Protocol::Frame &frame,
                          size_t frame_length) const = 0;
  virtual SizedReadBuffer receive() = 0;
};
class UartDriver : public Driver {
public:
  Esp_Err_t init() override;
  SizedReadBuffer receive() override;
  size_t get_buffered_rx_length();
  byte_count send(const Protocol::Frame &frame,
                  size_t frame_length) const override;
  UartDriver(Pin tx, Pin rx);
  UartDriver();

private:
  Pin tx_pin;
  Pin rx_pin;
};
