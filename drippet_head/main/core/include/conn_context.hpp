#include <cstdint>
struct ConnContext {
  uint8_t indicate_status{};
  uint8_t notify_status{};
  uint16_t conn_handle{};
};
