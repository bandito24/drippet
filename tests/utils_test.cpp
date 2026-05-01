#include "config.hpp"
#include "util.hpp"

#include <array>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <fakeit.hpp>

TEST_CASE("utils test", "[utils]") {

  SECTION("works for converting uint16_t to uint8_t little endian") {
    constexpr size_t test_len = config::node_hose_count;
    std::array<uint8_t, test_len * 2> arr8{};
    std::array<uint16_t, test_len> arr16{1000, 2000, 3000, 4000, 5000};
    std::array<uint16_t, test_len> arr16_2{};
    Util::le16_to_le8(arr8, arr16);

    Util::le8_to_le16(arr16_2, arr8);
    REQUIRE(arr16_2 == arr16);
  }
}
