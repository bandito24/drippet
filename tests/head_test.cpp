
#include "config.hpp"
#include "constants.hpp"
#include "driver.hpp"
#include "fixtures.hpp"
#include "head.hpp"
#include "logger.hpp"
#include "mocks.hpp"
#include "node.hpp"
#include "protocol.hpp"
#include "protocol_types.hpp"
#include "util.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <optional>

using Fixture = HeadFixture;
constexpr NodeKey_t TEST_KEY = 500;

TEST_CASE("create node pending behaves correctly", "[head]") {

  SECTION("Node Links all start as nullptr") {

    Fixture fix;

    bool all_null = true;
    for (std::size_t i = 0; i < config::max_nodes; i++) {
      if (fix.head->node_exists(i)) {
        all_null = false;
        break;
      }
    }
    REQUIRE(all_null == true);
  }
  SECTION("successfully creates a node in initializing state") {

    Fixture fix;
    auto address = fix.head->create_node_pending(TEST_KEY);
    REQUIRE(*address == 0); // First address location;
    REQUIRE(fix.head->get_node_status(*address) == NodeStatus::INITIALIZING);
  }
  SECTION("head does not allow more than max addresses") {

    Fixture fix;
    bool add_correct = true;
    for (size_t i = 0; i < config::max_nodes; i++) {
      auto address = fix.head->create_node_pending(i);
      if (!address) {
        add_correct = false;
        break;
      }
    }
    REQUIRE(add_correct);
    auto last_address = fix.head->create_node_pending(TEST_KEY);

    auto last_address2 = fix.head->create_node_pending(TEST_KEY + 5);
    REQUIRE(last_address == std::nullopt);
    REQUIRE(last_address2 ==
            std::nullopt); // Means it full--this return means full
  }
  SECTION("will not create another instance if key has been seen before") {
    Fixture fix;
    // Tests to make sure if a key is duplicate then it just sends back first
    // address
    auto address = fix.head->create_node_pending(TEST_KEY);
    auto addressAgain = fix.head->create_node_pending(TEST_KEY);
    REQUIRE(addressAgain == address);

    SECTION("will only confirm node pending if the key matches the stored "
            "address") {

      auto address2 = fix.head->create_node_pending(TEST_KEY + 1);
      auto address3 = fix.head->create_node_pending(TEST_KEY + 2);
      auto status1 = fix.head->confirm_node_pending(TEST_KEY + 1, 0);
      REQUIRE(status1 == NodeLinkStatus::LINK_KEY_MISMATCH);
      REQUIRE(fix.head->get_node_status(1) == NodeStatus::INITIALIZING);

      auto status2 = fix.head->confirm_node_pending(TEST_KEY + 1, 1);
      REQUIRE(status2 == NodeLinkStatus::LINK_OK);
      REQUIRE(fix.head->get_node_status(1) == NodeStatus::READY);
      REQUIRE(fix.head->get_node_status(0) == NodeStatus::INITIALIZING);
      REQUIRE(fix.head->get_node_status(2) == NodeStatus::INITIALIZING);
    }
  }
  SECTION("retrieve_all_nodes accurately gives all arrays") {
    Fixture fix;
    all_durations_t durs = fix.head->retrieve_all_durations();
    all_durations_t check{};
    REQUIRE(durs == check);
    NodeTypes::HoseDurations dur_check = {1, 1, 1, 1, 1};
    Mocks::populate_head_nodes(*fix.head, 1);
    fix.head->set_node_durations(0, dur_check);

    all_durations_t durs2 = fix.head->retrieve_all_durations();
    check[0] = dur_check;
    REQUIRE(durs2 == check);
  }
}

using CMD = Protocol::Command;
auto ser_key = Util::serialize_key(Mocks::sample_key);
TEST_CASE("Head task behaves as expected in the task loop", "[head_task]") {

  SECTION("handle_incoming_frame updates state and emits the correct response "
          "from input") {

    Fixture fix;

    SECTION("sends addressing frame with discovery incoming") {
      auto incoming = Mocks::incoming_discovery_frame(Mocks::sample_key);
      auto res = fix.head->handle_incoming_frame(incoming);
      REQUIRE(res != std::nullopt);
      auto r = *res;

      REQUIRE(fix.head->get_node_status(0) == NodeStatus::INITIALIZING);
      REQUIRE(r.address == 0);
      REQUIRE(r.command == Protocol::Command::ADDRESSING);
      REQUIRE(r.data[0] == Mocks::sample_key);

      // Replies to duplicates without
      SECTION("Replies to duplicates without creating new node") {
        auto res2 = fix.head->handle_incoming_frame(incoming);
        REQUIRE(res2->address == fix.head->get_node_by_key(incoming.data[0]));
        REQUIRE(!fix.head->node_exists(1));
      }
      SECTION("Will correct an incorrect addressing response") {
        auto wrongAddress =
            Mocks::incoming_adressing_frame(1, Mocks::sample_key);
        auto res = fix.head->handle_incoming_frame(incoming);

        REQUIRE(*res == UartMessage{.address = 0,
                                    .command = CMD::ADDRESSING,
                                    .data{ser_key[0], ser_key[1]},
                                    .data_length = 2});
      }
      SECTION("Will confirm node of a correct addressing response") {
        auto right = Mocks::incoming_adressing_frame(0, Mocks::sample_key);
        auto res2 = fix.head->handle_incoming_frame(right);
        REQUIRE(res2->command == CMD::ACK);
        REQUIRE(fix.head->get_node_status(*fix.head->get_node_by_key(
                    Mocks::sample_key)) == NodeStatus::READY);
      }
    }
  }
  SECTION("correctly processes watering schedule") {
    Fixture fix;
    When(Method(fix.clockMock, is_watering_due)).AlwaysReturn(true);
    When(Method(fix.clockMock, progress_watering_due)).AlwaysReturn();

    Mocks::populate_head_nodes(*fix.head);

    SECTION("Logical control path") {

      // Building dynamic node structure
      config::Address add_check = config::max_nodes - 1;
      config::Address add_check_2 = config::max_nodes - 2;
      fix.head->set_node_status(add_check, NodeStatus::INITIALIZING);
      NodeTypes::HoseDurations empty_durs{};
      fix.head->set_node_durations(add_check_2, empty_durs);

      fix.head->process_watering_schedule();
      REQUIRE(fix.head->get_head_status() == HeadStatus::WATERING_CMDS);
      Verify(Method(fix.clockMock, progress_watering_due)).Exactly(1);
      Verify(Method(fix.valveMock, enable)).AtLeastOnce();

      SECTION("Nodes with no duration and not NodeStatus::ready do not go in "
              "queue") {

        for (size_t addr = 0; addr < config::max_nodes; addr++) {
          auto status = fix.head->get_node_status(addr);
          if (addr != add_check && addr != add_check_2) {
            REQUIRE(status == NodeStatus::IN_QUEUE);
          } else if (addr == add_check) {
            REQUIRE(status == NodeStatus::INITIALIZING);
          } else {
            REQUIRE(status == NodeStatus::READY);
          }
        }
      }
      SECTION("handling next_watering_frame with watering_cmds status") {

        SECTION(
            "starts with the first node and sets it to command sent status") {
          auto msg = fix.head->next_watering_frame();

          REQUIRE(msg->address == 0);
          REQUIRE(msg->data == Mocks::hose_durations);
          REQUIRE(fix.head->get_node_status(0) == NodeStatus::COMMAND_SENT);

          SECTION(
              "max retries without response will reset node and head state") {

            std::optional<UartMessage> msg2;
            for (size_t i = 0; i < RETRY_NODE_MAX - 1; i++) {
              msg2 = fix.head->next_watering_frame();
            }
            REQUIRE(msg == msg2);

            auto msg3 = fix.head->next_watering_frame();
            REQUIRE(msg3 == std::nullopt);
            REQUIRE(fix.head->get_node_status(0) == NodeStatus::ERR);
            REQUIRE(fix.head->get_head_status() == HeadStatus::FAULTY_NODE);
            Verify(Method(fix.valveMock, disable)).AtLeastOnce();
            REQUIRE(fix.head->get_active_watering_index() == std::nullopt);
          }
          SECTION(
              "unsuccessful sends will increase retry count and successfull "
              "connection attempt will reset attempts") {

            for (size_t i = 0; i < RETRY_NODE_MAX - 1; i++) {
              fix.head->next_watering_frame();
            }

            REQUIRE(fix.head->get_node_retry_count(0) == RETRY_NODE_MAX - 1);

            SECTION("Has to be from the correct node") {
              auto ackMsg =
                  fix.head->handle_incoming_frame(Mocks::incoming_ack_frame(1));
              REQUIRE(fix.head->get_node_retry_count(0) != 0);
              REQUIRE(fix.head->get_node_status(0) == NodeStatus::COMMAND_SENT);
            }
            auto ackMsg =
                fix.head->handle_incoming_frame(Mocks::incoming_ack_frame(0));
            REQUIRE(fix.head->get_node_retry_count(0) == 0);
          }
          SECTION(
              "receiving an ack from node from watering commnand will change "
              "state") {

            REQUIRE(fix.head->get_node_status(0) == NodeStatus::COMMAND_SENT);
            auto ackMsg =
                fix.head->handle_incoming_frame(Mocks::incoming_ack_frame(0));
            REQUIRE(fix.head->get_node_status(0) == NodeStatus::WATERING);
            SECTION("subsequent water frames will be status related") {

              auto next_msg = fix.head->next_watering_frame();
              REQUIRE(next_msg == UartMessage{0, CMD::STATUS});
              REQUIRE(fix.head->get_active_watering_index() == 0);
              REQUIRE(fix.head->get_node_retry_count(0) == 1);
              SECTION("enough status requests without response will err out "
                      "system") {

                for (size_t i = 0; i <= RETRY_NODE_MAX; i++) {
                  fix.head->next_watering_frame();
                }

                REQUIRE(fix.head->get_head_status() == HeadStatus::FAULTY_NODE);
                Verify(Method(fix.valveMock, disable)).AtLeastOnce();

                REQUIRE(fix.head->get_active_watering_index() == std::nullopt);
              }
              SECTION("Err status from node will shut down system") {
                auto msg = Mocks::incoming_status_frame(0, NodeStatus::ERR);
                fix.head->handle_incoming_frame(msg);
                Verify(Method(fix.valveMock, disable)).AtLeastOnce();
                REQUIRE(fix.head->get_active_watering_index() == std::nullopt);
                REQUIRE(fix.head->get_head_status() == HeadStatus::FAULTY_NODE);
              }
              SECTION("incoming status frame will reset node retry count") {
                fix.head->next_watering_frame();
                REQUIRE(fix.head->get_node_retry_count(0) != 1);
                auto msg =
                    Mocks::incoming_status_frame(0, NodeStatus::WATERING);
                fix.head->handle_incoming_frame(msg);
                REQUIRE(fix.head->get_node_retry_count(0) == 0);
              }
              SECTION(
                  "incoming ready (meaning all done) status from a watering "
                  "status will advance watering index") {

                SECTION("will not advance is ready state is not from watering "
                        "index") {
                  REQUIRE(fix.head->get_active_watering_index() == 0);
                  auto msg = Mocks::incoming_status_frame(1, NodeStatus::READY);
                  fix.head->handle_incoming_frame(msg);
                  REQUIRE(fix.head->get_active_watering_index() == 0);
                }
                SECTION("will advance if the current hose replies ready") {

                  auto msg = Mocks::incoming_status_frame(0, NodeStatus::READY);
                  fix.head->handle_incoming_frame(msg);
                  REQUIRE(fix.head->get_active_watering_index() == 1);
                }
              }
            }
          }
        }
      }
    }

    SECTION("After progressing through all nodes watering is concluded") {

      fix.head->process_watering_schedule();

      Logger::log_simple(
          "status is %d",
          static_cast<int>(*fix.head->get_active_watering_index()));

      for (size_t i = 0; i < config::max_nodes; i++) {
        auto msg1 = Mocks::incoming_ack_frame(i);
        fix.head->handle_incoming_frame(msg1);
        auto msg2 = Mocks::incoming_status_frame(i, NodeStatus::READY);
        fix.head->handle_incoming_frame(msg2);
      }

      Verify(Method(fix.valveMock, disable)).AtLeastOnce();
      REQUIRE(fix.head->get_head_status() == HeadStatus::STANDBY);
      REQUIRE(fix.head->get_active_watering_index() == std::nullopt);
    }
    SECTION(
        "Initialization of pairing mode clears out all nodes and resets head") {

      fix.head->init_pairing_mode();
      Verify(Method(fix.valveMock, disable)).AtLeastOnce();
      REQUIRE(fix.head->get_head_status() == HeadStatus::PAIRING);
      REQUIRE(fix.head->get_active_watering_index() == std::nullopt);
      bool clear = true;
      for (size_t i = 0; i < config::max_nodes; i++) {
        if (fix.head->node_exists(i)) {
          clear = false;
        }
      }
      REQUIRE(clear == true);
    }
  } //
}
