#include "protocol.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include <assert.h>

#include "logger.hpp"
#include <cstdio>
#include <optional>

SizedFrameBuffer UartProtocol::prepare_bytes(const UartMessage &data) const {

  assert(data.data_length <= config::node_hose_count);
  uint8_t data_len = data.data_length * 2; // uint16_t broken up into 2

  SizedFrameBuffer frame_buffer{};
  Protocol::Frame &frame = frame_buffer.frame;
  frame = {Protocol::start_bit, data.address,
           static_cast<uint8_t>(data.command), data_len};
  size_t frame_index = to_index(Protocol::HeaderOrder::HEADER_LENGTH);
  for (size_t i = 0; i < data.data_length; i++) {
    uint16_t byte = data.data[i];
    frame[frame_index++] = byte & 0xFF;
    frame[frame_index++] = (byte >> 8) & 0xFF;
  }
  uint16_t crc = UartFunctions::crc16_modbus(&frame[1], frame_index - 1);
  frame[frame_index++] = crc & 0xFF;
  frame[frame_index++] = (crc >> 8) & 0xFF;
  assert(frame_index < Protocol::FrameLength);
  frame_buffer.content_length = frame_index;
  return frame_buffer;
}
byte_count UartProtocol::write_bytes(const SizedFrameBuffer &buffer) const {
  return this->driver.send(buffer.frame, buffer.content_length);
}

std::optional<IndexedFrame>
UartFunctions::create_indexed_frame(const SizedReadBuffer &buffer,
                                    size_t start_index) {

  if (buffer.content[start_index] != Protocol::start_bit)
    return std::nullopt;
  IndexedFrame packet{};
  auto &segment = packet.frame;
  auto &i = packet.i;

  size_t data_length_index =
      start_index + to_index(Protocol::HeaderOrder::DATA_LENGTH);
  size_t data_length = buffer.content[data_length_index];
  int header_end = static_cast<int>(Protocol::HeaderOrder::HEADER_LENGTH);
  if (data_length) {
    i.data_start = header_end;
  } else {
    i.data_start = -1;
  }
  i.cdc_start = header_end + data_length;
  i.length = i.cdc_start + 2;
  if ((start_index + i.length) > buffer.length) {
    return std::nullopt;
  }
  if (i.length > segment.size()) {
    return std::nullopt;
  }
  auto begin = buffer.content.begin() + start_index;
  auto end = begin + i.length;
  std::copy(begin, end, segment.begin());

  return packet;
}

// Sequence Goes:
// 1) uart_read (from driver),
// 1) parse_uart_read (iterate over the buffer returned from previous in a task)
// 2) build_uart_message with each extracted Frame (which uart_task adds to the
// queue)

std::optional<IndexedFrame>
UartProtocol::parse_uart_read(const SizedReadBuffer &buffer,
                              size_t start_index) {

  if (buffer.length == 0 || start_index >= buffer.length)
    return std::nullopt;

  auto &content = buffer.content;
  size_t i = start_index;
  while (i < buffer.length && (content[i] != Protocol::start_bit)) {
    i++;
  }
  if (i >= buffer.length) {
    return std::nullopt;
  }

  return UartFunctions::create_indexed_frame(buffer, start_index);
}

// 3)
std::optional<UartMessage>
UartProtocol::build_uart_message(const Protocol::Frame &frame_seg) {

  if (UartFunctions::validate_frame(frame_seg) != ParseResult::Ok) {
    return std::nullopt;
  }
  return UartFunctions::reconstruct_uart_message(frame_seg);
}

// 3a)
ParseResult UartFunctions::validate_frame(Protocol::Frame buffer) {

  if (buffer[to_index(MsgIndex::START_BIT)] != Protocol::start_bit) {
    Logger::log_error("Invalid Start Bit"); // Drop it, start process over
                                            // of discovery or water init
                                            // return
    return ParseResult::InvalidStartBit;
  }

  size_t crc_start = to_index(Protocol::HeaderOrder::HEADER_LENGTH) +
                     buffer[to_index(Protocol::HeaderOrder::DATA_LENGTH)];

  for (size_t i = 0; i < 10; i++) {
    printf("index: %d = %u\n", i, buffer[i]);
  }

  uint16_t crc_computed =
      UartFunctions::crc16_modbus(&buffer[1], crc_start - 1);
  uint16_t crc_received = UartFunctions::merge_uint8(crc_start, crc_start + 1);
  printf("received: %d, calculated: %d", crc_received, crc_computed);
  if (crc_computed != crc_received) {
    Logger::log_error("CRC16 MODBUS values do not match"); // Drop it, restart
    return ParseResult::CrcMismatch;
  }
  return ParseResult::Ok;
}

// 3b)
using Header = Protocol::HeaderOrder;
UartMessage UartFunctions::reconstruct_uart_message(Protocol::Frame buffer) {

  Protocol::FrameDataArray result{};
  size_t data_length = buffer[to_index(Protocol::HeaderOrder::DATA_LENGTH)];
  if (data_length != 0) {
    assert(data_length % 2 == 0);
    for (size_t i = Protocol::DATA_START_INDEX; i < data_length; i = i + 2) {
      result[i - Protocol::DATA_START_INDEX] =
          UartFunctions::merge_uint8(buffer[i], buffer[i + 1]);
    }
  }

  return {.address = buffer[to_index(Header::ADDRESS)],
          .command =
              static_cast<Protocol::Command>(buffer[to_index(Header::COMMAND)]),
          .data = result,
          .data_length = data_length / 2};
}

UartProtocol::UartProtocol(Driver &transportDriver)
    : driver{transportDriver} {};

uint16_t UartFunctions::crc16_modbus(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];

    for (int j = 0; j < 8; ++j) {
      if (crc & 0x0001)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }

  return crc;
}

uint16_t
UartFunctions::merge_uint8(uint8_t first_bit,
                           uint8_t second_bit) { // Little Endian Reconstruction
  return (static_cast<uint16_t>(first_bit)) |
         static_cast<uint16_t>(second_bit << 8);
}
