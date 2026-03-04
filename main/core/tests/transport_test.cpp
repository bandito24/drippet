
#include "config.hpp"
#include "driver.hpp"
#include "protocol.hpp"
#include <algorithm>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <functional>
#include <optional>

constexpr uint16_t sample_key = 500;
constexpr size_t calculate_uint8_size(UartMessage &msg) {
  return to_index(Protocol::HeaderOrder::HEADER_LENGTH) +
         (msg.data_length * 2) +
         2; // Uint8_t headers, 2 * uint16_t data, 2 crc for crcmod_16
}
constexpr int to_int(size_t val) { return static_cast<size_t>(val); }

fakeit::Mock<Driver> driverMock;
using ORDER = Protocol::HeaderOrder;
UartMessage addressingOutgoing{.address = 2,
                               .command = Protocol::Command::ADDRESSING,
                               .data = Protocol::FrameDataArray{sample_key},
                               .data_length = 1};
Protocol::FrameDataArray exampleWatering{100, 200, 300, 400, 500};
UartMessage wateringOutgoing{.address = 2,
                             .command = Protocol::Command::INIT_WATER_DURATIONS,
                             .data = exampleWatering,
                             .data_length = 5};
UartMessage emptyDataOutgoing{.address = 0,
                              .command = Protocol::Command::BROADCAST,
                              .data = Protocol::FrameDataArray{},
                              .data_length = 0};

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
      SizedFrameBuffer watering = protocol.prepare_bytes(wateringOutgoing);
      SizedFrameBuffer empty = protocol.prepare_bytes(emptyDataOutgoing);

      std::copy(addressing.frame.begin(),
                addressing.frame.begin() + addressing.content_length,
                buffer.begin());
      // POPULATING THE BUFFER MOCK DATA
      size_t i = addressing.content_length;
      buffer[i++] =
          0x12; // 2 Garbage Values to Make sure it skips garbage values
      buffer[i++] = 0x12;
      std::copy(watering.frame.begin(),
                watering.frame.begin() + watering.content_length,
                buffer.begin() + i);
      i += watering.content_length;
      std::copy(empty.frame.begin(), empty.frame.begin() + empty.content_length,
                buffer.begin() + i);

      sizedBuffer.length = watering.content_length + 2 +
                           addressing.content_length + empty.content_length;

      ////////////
      size_t start_index = 0;
      std::optional<IndexedFrame> first_read =
          protocol.parse_uart_read(sizedBuffer, start_index);
      start_index += first_read->i.length;
      std::optional<IndexedFrame> second_read =
          protocol.parse_uart_read(sizedBuffer, start_index);
      start_index += second_read->i.length;
      std::optional<IndexedFrame> third_read =
          protocol.parse_uart_read(sizedBuffer, start_index);

      printf("\nFIRST\n\n");
      for (size_t i = 0; i < first_read->i.length; i++) {
        printf("%d\n", first_read->frame[i]);
      }
      printf("\n\nSECOND\n");

      for (size_t i = 0; i < second_read->i.length; i++) {
        printf("%d\n", second_read->frame[i]);
      }

      printf("\n\nTHIRD\n");

      for (size_t i = 0; i < third_read->i.length; i++) {
        printf("%d\n", third_read->frame[i]);
      }

      printf("\n\nEND\n");

      start_index += third_read->i.length;
      std::optional<IndexedFrame> fourth_read =
          protocol.parse_uart_read(sizedBuffer, start_index);

      SECTION("Checking out of bounds is nullopt") {

        REQUIRE(first_read != std::nullopt);
        REQUIRE(second_read != std::nullopt);
        REQUIRE(third_read != std::nullopt);
        REQUIRE(fourth_read == std::nullopt);
      }

      SECTION("Calulating proper indexing") {

        REQUIRE(first_read->i.length ==
                calculate_uint8_size(addressingOutgoing));
        REQUIRE(second_read->i.length ==
                calculate_uint8_size(wateringOutgoing));
        REQUIRE(second_read->i.cdc_start ==
                to_index(ORDER::HEADER_LENGTH) +
                    (wateringOutgoing.data_length * 2));
        REQUIRE(second_read->frame.size() ==
                second_read->i.length); // Represents the max size

        REQUIRE(third_read->i.length ==
                (to_index(ORDER::HEADER_LENGTH) + 2)); // No Data Values
      }

      Protocol::FrameDataArray conf_array{};
      SECTION("Calculating the information is rearranged correctly") {

        size_t j = 0;
        for (size_t i = Protocol::DATA_START_INDEX;
             i < second_read->i.cdc_start - 1; i += 2) {
          uint16_t merged = UartFunctions::merge_uint8(
              second_read->frame[i], second_read->frame[i + 1]);
          conf_array[j++] = merged;
        }

        // for (size_t i = 0; i < config::max_nodes; i++) {
        //  printf("%d\n", conf_array[i]);
        //}

        REQUIRE(
            std::equal(exampleWatering.begin(),
                       exampleWatering.begin() + wateringOutgoing.data_length,
                       conf_array.begin()));
      }
      SECTION("Frame is properly validated") {

        IndexedFrame first_cpy = *first_read;
        IndexedFrame second_cpy = *first_read;
        first_cpy.frame[to_index(ORDER::START_BIT)] = 0x11;
        second_cpy.frame[second_cpy.i.cdc_start] = 0x01;
        ParseResult res1 = UartFunctions::validate_frame(first_cpy.frame);
        ParseResult res2 = UartFunctions::validate_frame(second_cpy.frame);
        ParseResult res3 = UartFunctions::validate_frame(first_read->frame);
        ParseResult res4 = UartFunctions::validate_frame(third_read->frame);

        REQUIRE(res1 == ParseResult::InvalidStartBit);
        REQUIRE(res2 == ParseResult::CrcMismatch);
        REQUIRE(res3 == ParseResult::Ok);
        REQUIRE(res4 == ParseResult::Ok);
      }
      SECTION(
          "Frame is properly reconstructed into a UartMessage for send off") {
        UartMessage msg1 =
            UartFunctions::reconstruct_uart_message(first_read->frame);
        UartMessage msg2 =
            UartFunctions::reconstruct_uart_message(second_read->frame);
        UartMessage msg3 =
            UartFunctions::reconstruct_uart_message(third_read->frame);
        REQUIRE(msg1 == addressingOutgoing);
        REQUIRE(msg2 == wateringOutgoing);
        REQUIRE(msg3 == emptyDataOutgoing);
      }
    }
  }
}
