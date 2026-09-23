#include <cstddef>
namespace T {

template <typename T> inline size_t to_size(T input) {
  return static_cast<size_t>(input);
}
template <typename T> inline int to_int(T input) {
  return static_cast<int>(input);
}
} // namespace T
