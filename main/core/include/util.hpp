
#include <cassert>
#include <cstdint>
#include <span>
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
inline void le16_to_le8(std::span<uint8_t> out, std::span<uint16_t> in) {
  assert(out.size() == (in.size() * 2));
  size_t insert_idx = 0;
  for (size_t i = 0; i < in.size(); i++) {
    Util::put_le16(&out.data()[insert_idx], in[i]);
    insert_idx += 2;
  }
}

inline void le8_to_le16(std::span<uint16_t> out, std::span<uint8_t> in) {
  assert(out.size() == (in.size() / 2));
  size_t insert_idx = 0;
  for (size_t i = 0; i < in.size(); i += 2) {
    uint16_t val = Util::get_le16(&in.data()[i]);
    out[insert_idx] = val;
    insert_idx += 1;
  }
}
} // namespace Util
