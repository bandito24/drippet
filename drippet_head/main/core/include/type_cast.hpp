#include <cstddef>
namespace T {

template <typename T> inline size_t to_i(T input) {
  return static_cast<size_t>(input);
}
} // namespace T
