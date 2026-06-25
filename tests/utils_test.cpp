#include "config.hpp"
#include "node.hpp"
#include "protocol_types.hpp"
#include "util.hpp"

#include <array>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <fakeit.hpp>

TEST_CASE("utils test", "[utils]") {

  SECTION("works for converting uint16_t to uint8_t little endian") {
    constexpr size_t test_len = Protocol::MAX_DATA_LEN;
    std::array<uint8_t, test_len * 2> arr8{};
    std::array<uint16_t, test_len> arr16{1000, 2000, 3000, 4000, 5000};
    std::array<uint16_t, test_len> arr16_2{};
    Util::le16_to_le8(arr8, arr16);

    Util::le8_to_le16(arr16_2, arr8);
    REQUIRE(arr16_2 == arr16);
  }

  SECTION("serialize key and deserialize key works") {
    NodeKey_t key = 20;
    auto val = Util::serialize_key(key);
    REQUIRE(val[1] == 0); // Asserts little endian
    NodeKey_t key2 = Util::deserialize_key(val);
    REQUIRE(key == key2);
  }
  SECTION("bytes are serialized with water cycle") {
    NodeTypes::WateringCycle cycle{true, false, false, false, true, true, true};
    uint8_t bytes = Util::water_cycle_to_bytes(cycle);
    auto res = Util::byte_to_water_cycle(bytes);
    REQUIRE(res == cycle);
  }
}
