
#include <cstdint>
namespace Util {
inline void put_le16(void *buf, uint16_t x) {

  auto u8ptr = static_cast<uint8_t *>(buf);

  u8ptr[0] = (uint8_t)x;
  u8ptr[1] = (uint8_t)(x >> 8);
}
inline uint16_t get_le16(const void *buf) {

  uint16_t x;
  auto u8ptr = static_cast<const uint8_t *>(buf);

  x = u8ptr[0];
  x |= (uint16_t)u8ptr[1] << 8;

  return x;
}
} // namespace Util
