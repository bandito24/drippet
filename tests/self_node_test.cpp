
#include "config.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "fixtures.hpp"
#include "mocks.hpp"
#include "node.hpp"
#include "protocol.hpp"
#include "util.hpp"
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <optional>
using CMD = Protocol::Command;
constexpr uint16_t test_key = 20;
auto ser = Util::serialize_key(test_key);
Protocol::FrameDataArray key_data{
    ser[0], ser[1]}; // Represents the max uint16_t data size
config::Address test_addr = 5;
NodeTypes::HoseDurations test_durs = Mocks::get_new_hose_durations(10);

config::Address wrong_addr = test_addr + 1;

TEST_CASE("self_hode all", "[self_node]") {
  SelfNodeFixture fix{};

  UartMessage msg1{
      .command = CMD::DISCOVERY, .data = key_data, .data_length = 2};

  SelfNode &node = fix.self_node;

  SECTION("handle_incoming_frame main") {

    SECTION("Will serialize and store new keys to use") {

      auto res = node.handle_incoming_frame(msg1);
      REQUIRE(res->command == CMD::DISCOVERY);
      REQUIRE(res->data == key_data);
      REQUIRE(fix.self_node.get_key() == test_key);

      msg1.data[1] += 1;

      auto res2 = node.handle_incoming_frame(msg1);
      REQUIRE(res->data == key_data); // Does not update key after getting a new
                                      // descovery but will respond
      REQUIRE(fix.self_node.get_key() == test_key);

      SECTION("Will respond correctly to addressing") {
        // Will not resond to addressing on different key
        UartMessage msg2{.address = test_addr,
                         .command = CMD::ADDRESSING,
                         .data = key_data,
                         .data_length = 2};

        msg2.data[1] += 1;

        auto res2 = node.handle_incoming_frame(msg2);
        REQUIRE(res2 == std::nullopt);
        REQUIRE(node.get_addr() == ADDR_UNSET);

        // Now will respond and set the address if the key matches
        msg2.data[1] -= 1;

        auto res3 = node.handle_incoming_frame(msg2);

        REQUIRE(res3 != std::nullopt);
        REQUIRE(res3->command == CMD::ADDRESSING);
        REQUIRE(res3->data == key_data);
        REQUIRE(res3->address == test_addr);
        REQUIRE(node.get_addr() == test_addr);
        REQUIRE(node.get_status() == NodeStatus::INITIALIZING);
        SECTION("ack will complete initialization now") {

          config::Address wrong = test_addr + 1;
          UartMessage msg3{.address = wrong, .command = CMD::ACK};

          auto res4 = node.handle_incoming_frame(msg3);
          REQUIRE(res4 == std::nullopt);

          UartMessage msg4{.address = test_addr, .command = CMD::ACK};

          auto res5 = node.handle_incoming_frame(msg4);
          REQUIRE(msg4.address == node.get_addr());
          REQUIRE(node.get_status() == NodeStatus::READY);

          SECTION("Initializing and reporting water durations") {

            UartMessage msg{.address = wrong_addr,
                            .command = CMD::INIT_WATER_DURATIONS,
                            .data = test_durs};

            auto res = fix.self_node.handle_incoming_frame(msg);
            REQUIRE(res == std::nullopt);
            REQUIRE(node.get_status() == NodeStatus::READY);
            msg.address = test_addr;
            SECTION("Confirming initialization of durations is correct") {

              res = fix.self_node.handle_incoming_frame(msg);
              REQUIRE(res ==
                      UartMessage{.address = test_addr, .command = CMD::ACK});
              REQUIRE(node.get_status() == NodeStatus::WATERING);
              // Making sure the durations is clock now plus each duration
              auto now = fix.steady_clock.get().now();
              SelfHoseDurations durs{};
              for (size_t i = 0; i < durs.size(); i++) {
                now += test_durs[i];
                durs[i] = now;
              }
              REQUIRE(durs == node.get_hose_durations());
              REQUIRE(node.get_active_hose_index() == 0);
            }
          }
        }
      }
    }
  }
}
