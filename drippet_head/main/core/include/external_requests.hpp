
#include <array>
#include <cstddef>
#include <optional>
#include <stdint.h>
enum class Req_t {

  MODIFY_CLOCK_TIME,
  MODIFY_NODE_DURATIONS,
  MODIFY_NODE_CYCLE,
  // THis is just the daily time that it starts watering procedure
  MODIFY_PHASE_START_TIME,
  // NOTE: Init_pairing should be the last thing since node durations will be
  // reset with init pairing
  MODIFY_NODE_STATE,
  INIT_PAIRING,
  REQUEST_COUNT
};
constexpr size_t ExtRqLen = static_cast<size_t>(Req_t::REQUEST_COUNT);

struct ExtRequest {
  ExtRequest(Req_t _type, std::array<uint16_t, 4> _data)
      : type{_type}, data{_data} {};

  ExtRequest(Req_t _type) : type{_type} {};
  Req_t type;
  std::array<uint16_t, 4> data{};
};
using OptionalRequest = std::optional<ExtRequest>;
using ExternalQueue = std::array<OptionalRequest, ExtRqLen>;
class ExtRqManager {
private:
  ExternalQueue extRequests{};
  ExternalQueue extEvents{};
  static std::size_t to_i(Req_t t) { return static_cast<size_t>(t); }
  void put(const ExtRequest &req, ExternalQueue &queue) {
    queue[to_i(req.type)] = req;
  }
  OptionalRequest pop(ExternalQueue &queue) {
    for (size_t i = 0; i < queue.size(); i++) {
      if (queue[i]) {
        auto res = queue[i];
        queue[i] = std::nullopt;
        return res;
      }
    }
    return std::nullopt;
  }
  void putRequest(const ExtRequest &req) { this->put(req, this->extRequests); }
  void putEvents(const ExtRequest &req) { this->put(req, this->extEvents); }
  OptionalRequest popRequest() { return this->pop(this->extRequests); }

public:
  OptionalRequest popEvent() { return this->pop(this->extEvents); }
  uint8_t peek_event_count() {
    uint8_t count = 0;
    for (size_t i = 0; i < this->extEvents.size(); i++) {
      if (this->extEvents[i]) {
        count++;
      }
    }
    return count;
  }
  friend class Head;
};
