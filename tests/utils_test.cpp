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

  SECTION("le64 round-trips and is little endian") {
    std::array<uint8_t, 8> buf{};
    Util::put_le64(buf.data(), 1790627400ULL);
    REQUIRE(buf == std::array<uint8_t, 8>{0x48, 0xCE, 0xBA, 0x6A, 0, 0, 0, 0});
    REQUIRE(Util::get_le64(buf.data()) == 1790627400ULL);

    uint64_t big = 0x0123456789ABCDEFULL;
    Util::put_le64(buf.data(), big);
    REQUIRE(buf[0] == 0xEF);
    REQUIRE(buf[7] == 0x01);
    REQUIRE(Util::get_le64(buf.data()) == big);

    int64_t negative = -1790627400LL;
    Util::put_le64(buf.data(), static_cast<uint64_t>(negative));
    REQUIRE(static_cast<int64_t>(Util::get_le64(buf.data())) == negative);
  }
  SECTION("a negative int16 offset survives le16") {
    std::array<uint8_t, 2> buf{};
    int16_t offset = -420;
    Util::put_le16(buf.data(), static_cast<uint16_t>(offset));
    REQUIRE(buf == std::array<uint8_t, 2>{0x5C, 0xFE});
    REQUIRE(static_cast<int16_t>(Util::get_le16(buf.data())) == offset);
  }
}
