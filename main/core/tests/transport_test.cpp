
#include "driver.hpp"
#include "protocol.hpp"
#include <algorithm>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <optional>

constexpr uint16_t sample_key = 500;
constexpr size_t calculate_uint8_size(UartMessage &msg) {
  return to_index(Protocol::HeaderOrder::HEADER_LENGTH) +
         (msg.data_length * 2) +
         2; // Uint8_t headers, 2 * uint16_t data, 2 crc for crcmod_16
}

fakeit::Mock<Driver> driverMock;
using ORDER = Protocol::HeaderOrder;
UartMessage addressingOutgoing{.address = 2,
                               .command = Protocol::Command::ADDRESSING,
                               .data = Protocol::FrameDataArray{sample_key},
                               .data_length = 1};
UartMessage wateringOutgoing{
    .address = 1,
    .command = Protocol::Command::INIT_WATER_DURATIONS,
    .data = Protocol::FrameDataArray{100, 200, 300, 400, 500},
    .data_length = 5};

TEST_CASE("Messages can correctly receive and send", "[uart]") {
  UartProtocol protocol{driverMock.get()};
  SECTION("Message is correctly prepared for write") {
    SizedFrameBuffer addressing = protocol.prepare_bytes(addressingOutgoing);
    size_t expected_length = calculate_uint8_size(addressingOutgoing);

    REQUIRE(addressing.content_length == expected_length);
    size_t data_start = to_index(ORDER::HEADER_LENGTH);
    uint16_t data = UartFunctions::merge_uint8(
        addressing.frame[data_start], addressing.frame[data_start + 1]);
    REQUIRE(data == sample_key);
    SECTION("Input is parsed and coorectly read") {
      SizedReadBuffer sizedBuffer{};
      auto &buffer = sizedBuffer.content;
      std::copy(addressing.frame.begin(),
                addressing.frame.begin() + addressing.content_length,
                buffer.begin());

      size_t i = addressing.content_length;
      buffer[i++] =
          0x12; // 2 Garbage Values to Make sure it skips garbage values
      buffer[i++] = 0x12;
      SizedFrameBuffer watering = protocol.prepare_bytes(wateringOutgoing);

      std::copy(watering.frame.begin(),
                watering.frame.begin() + watering.content_length,
                buffer.begin() + i);
      std::array<std::optional<IndexedFrame>, 3> results{};
      sizedBuffer.length =
          watering.content_length + 2 + addressing.content_length;

      // for (size_t i = 0; i < sizedBuffer.length + 1; i++) {
      //   printf("%d\n", buffer[i]);
      // }

      std::optional<IndexedFrame> first_read =
          protocol.parse_uart_read(sizedBuffer, 0);

      std::optional<IndexedFrame> second_read =
          protocol.parse_uart_read(sizedBuffer, first_read->i.length);
      std::optional<IndexedFrame> third_read =
          protocol.parse_uart_read(sizedBuffer, second_read->i.length);

      REQUIRE(first_read != std::nullopt);
      REQUIRE(second_read != std::nullopt);
      REQUIRE(third_read == std::nullopt);
    }
  }

  // REQUIRE( Factorial(1) == 1 );
}
