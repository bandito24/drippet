
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
#include <memory>
#include <optional>
using CMD = Protocol::Command;
constexpr uint16_t test_key = 20;
auto ser = Util::serialize_key(test_key);
Protocol::FrameDataArray key_data{
    ser[0], ser[1]}; // Represents the max uint16_t data size
config::Address test_addr = 5;
NodeTypes::HoseDuration test_dur = Mocks::get_new_hose_durations(10);

UartMessage status_request = {.address = test_addr, .command = CMD::STATUS};
using SelfPtr = std::unique_ptr<SelfNode>;
config::Address wrong_addr = test_addr + 1;

void mock_node_ready(SelfPtr &self_node) {

  UartMessage msg1{
      .command = CMD::DISCOVERY, .data = key_data, .data_length = 2};
  UartMessage msg2{.address = test_addr,
                   .command = CMD::ADDRESSING,
                   .data = key_data,
                   .data_length = 2};
  UartMessage msg4{.address = test_addr, .command = CMD::ACK};
  UartMessage msgs[] = {msg1, msg2, msg4};
  for (auto msg : msgs) {
    self_node->handle_incoming_frame(msg);
  }
}

TEST_CASE("self_hode all", "[self_node]") {
  SelfNodeFixture fix{};

  UartMessage msg1{
      .command = CMD::DISCOVERY, .data = key_data, .data_length = 2};

  SelfPtr &node = fix.self_node;

  SECTION("handle_incoming_frame main") {

    SECTION("Will serialize and store new keys to use") {

      auto res = node->handle_incoming_frame(msg1);
      REQUIRE(res->command == CMD::DISCOVERY);
      REQUIRE(res->data == key_data);
      REQUIRE(fix.self_node->get_key() == test_key);

      msg1.data[1] += 1;

      auto res2 = node->handle_incoming_frame(msg1);
      REQUIRE(res->data == key_data); // Does not update key after getting a new
                                      // descovery but will respond
      REQUIRE(fix.self_node->get_key() == test_key);

      SECTION("Will respond correctly to addressing") {
        // Will not resond to addressing on different key
        UartMessage msg2{.address = test_addr,
                         .command = CMD::ADDRESSING,
                         .data = key_data,
                         .data_length = 2};

        msg2.data[1] += 1;

        auto res2 = node->handle_incoming_frame(msg2);
        REQUIRE(res2 == std::nullopt);
        REQUIRE(node->get_addr() == ADDR_UNSET);

        // Now will respond and set the address if the key matches
        msg2.data[1] -= 1;

        auto res3 = node->handle_incoming_frame(msg2);

        REQUIRE(res3 != std::nullopt);
        REQUIRE(res3->command == CMD::ADDRESSING);
        REQUIRE(res3->data == key_data);
        REQUIRE(res3->address == test_addr);
        REQUIRE(node->get_addr() == test_addr);
        REQUIRE(node->get_status() == NodeStatus::INITIALIZING);

        SECTION("can change address if not yet acknowledged by head") {

          msg2.address = 99;
          auto res22 = node->handle_incoming_frame(msg2);
          REQUIRE(node->get_addr() == 99);
        }
        SECTION("ack will e-complete initialization now") {

          config::Address wrong = test_addr + 1;
          UartMessage msg3{.address = wrong, .command = CMD::ACK};

          auto res4 = node->handle_incoming_frame(msg3);
          REQUIRE(res4 == std::nullopt);

          UartMessage msg4{.address = test_addr, .command = CMD::ACK};

          REQUIRE(node->is_downstream_enabled() == false);
          auto res5 = node->handle_incoming_frame(msg4);
          REQUIRE(node->is_downstream_enabled() == true);
          REQUIRE(msg4.address == node->get_addr());
          REQUIRE(node->get_status() == NodeStatus::READY);

          SECTION("responds to incoming status request while not watering") {

            OptMsg rm2 = fix.self_node->handle_incoming_frame(status_request);
            REQUIRE(rm2->data[0] == static_cast<uint16_t>(node->get_status()));
          }

          SECTION("Initializing and reporting water durations") {

            UartMessage msg{.address = wrong_addr,
                            .command = CMD::INIT_WATER_DURATIONS,
                            .data = {test_dur}};
            SECTION("will not init on wrong address") {

              auto res = fix.self_node->handle_incoming_frame(msg);

              REQUIRE(node->is_solenoid_enabled() == false);
              REQUIRE(res == std::nullopt);
              REQUIRE(node->get_status() == NodeStatus::READY);
            }
            SECTION("Confirming initialization of duration is correct and "
                    "starts watering") {

              msg.address = test_addr;

              res = fix.self_node->handle_incoming_frame(msg);
              REQUIRE(res ==
                      UartMessage{.address = test_addr, .command = CMD::ACK});
              REQUIRE(node->get_status() == NodeStatus::WATERING);
              // Making sure the durations is clock now plus each duration
              auto now = fix.steady_clock.get().now();
              int ending_time = now + test_dur;

              REQUIRE(ending_time == node->get_hose_duration());
              REQUIRE(node->is_solenoid_enabled() == true);
              SECTION("sending an additional watering command will reset") {
                SECTION("with set times") {
                  msg.data = {200};
                  fix.self_node->handle_incoming_frame(msg);
                  REQUIRE(node->is_solenoid_enabled() == true);
                  REQUIRE(node->get_hose_duration() == now + msg.data[0]);
                  REQUIRE(node->get_status() == NodeStatus::WATERING);
                }
                SECTION("with empty duration will stop node") {

                  msg.data = {0};
                  fix.self_node->handle_incoming_frame(msg);
                  REQUIRE(node->is_solenoid_enabled() == false);
                  REQUIRE(node->get_status() == NodeStatus::READY);
                }
              }

              SECTION("responds to incoming status request while watering") {

                OptMsg rm2 =
                    fix.self_node->handle_incoming_frame(status_request);
                REQUIRE(rm2->data[0] ==
                        static_cast<uint16_t>(NodeStatus::WATERING));
              }

              SECTION("concludes after duration ends") {

                REQUIRE(node->is_solenoid_enabled() == true);
                REQUIRE(node->get_status() == NodeStatus::WATERING);

                fix.set_mock_now(ending_time);
                node->process_watering_schedule();

                REQUIRE(node->is_solenoid_enabled() == false);
                REQUIRE(node->get_status() == NodeStatus::READY);
              }
            }
          }

          SECTION("Will progress though an empty list immediately") {

            REQUIRE(node->is_solenoid_enabled() == false);
            REQUIRE(node->get_status() == NodeStatus::READY);

            Time::Long time = 300;
            fix.set_mock_now(time);
            UartMessage msg{.address = test_addr,
                            .command = CMD::INIT_WATER_DURATIONS,
                            .data = {0}};
            OptMsg rm = fix.self_node->handle_incoming_frame(msg);
            REQUIRE(rm->command == CMD::ACK);

            node->process_watering_schedule();

            REQUIRE(node->is_solenoid_enabled() == false);
            REQUIRE(node->get_status() == NodeStatus::READY);

            // Responds that it is done if requested
            OptMsg rm2 = fix.self_node->handle_incoming_frame(status_request);
            REQUIRE(rm2->data[0] == static_cast<uint16_t>(NodeStatus::READY));
          }
        }
      }
    }
  }
}
