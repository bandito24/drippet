
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
using Hose_Dur = NodeTypes::HoseDuration;

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
    auto address = Mocks::create_node_pending(*fix.head, TEST_KEY);
    REQUIRE(fix.head->get_node_status(0) == NodeStatus::INITIALIZING);
  }
  SECTION("head does not allow more than max addresses") {

    Fixture fix;
    bool add_correct = true;
    for (size_t i = 0; i < config::max_nodes; i++) {
      auto msg = Mocks::create_node_pending(*fix.head, i);
      if (!msg) {
        add_correct = false;
        break;
      }
    }
    REQUIRE(add_correct);
    auto last_msg = Mocks::create_node_pending(*fix.head, TEST_KEY);
    REQUIRE(last_msg == std::nullopt);
  }
  SECTION("will not create another instance if key has been seen before") {
    Fixture fix;
    // Tests to make sure if a key is duplicate then it just sends back first
    // address
    auto address = Mocks::create_node_pending(*fix.head, TEST_KEY);
    auto addressAgain = Mocks::create_node_pending(*fix.head, TEST_KEY);
    REQUIRE(addressAgain == address);
    REQUIRE(fix.head->get_node_status(1) == NodeStatus::NODE_NONEXISTANT);

    SECTION("will only confirm node pending if the key matches the stored "
            "address") {

      auto address2 = Mocks::create_node_pending(*fix.head, TEST_KEY + 1);
      auto address3 = Mocks::create_node_pending(*fix.head, TEST_KEY + 2);
      // Addr 0 should map to TEST_KEY so this is mismatch
      Mocks::confirm_node_pending(*fix.head, 0, TEST_KEY + 1);

      REQUIRE(fix.head->get_node_status(1) == NodeStatus::INITIALIZING);

      Mocks::confirm_node_pending(*fix.head, 1, TEST_KEY + 1);
      REQUIRE(fix.head->get_node_status(1) == NodeStatus::READY);
      REQUIRE(fix.head->get_node_status(0) == NodeStatus::INITIALIZING);
      REQUIRE(fix.head->get_node_status(2) == NodeStatus::INITIALIZING);
    }
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
        REQUIRE(res2->address ==
                fix.head->get_node_addr_by_key(incoming.data[0]));
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
        REQUIRE(fix.head->get_node_status(*fix.head->get_node_addr_by_key(
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
      // First making a few node conditions that should not init on watering
      config::Address add_check = config::max_nodes - 1;
      config::Address add_check_2 = config::max_nodes - 2;
      config::Address add_check_3 = config::max_nodes - 3;
      Mocks::set_node_status(*fix.head, add_check, NodeStatus::INITIALIZING);
      Mocks::set_node_duration(*fix.head, add_check_2, 0);
      NodeTypes::WateringCycle falseCycle = {false, false, false, false,
                                             false, false, false};
      Mocks::set_watering_cycle(*fix.head, add_check_3, falseCycle);

      fix.head->process_watering_schedule();
      REQUIRE(fix.head->get_head_status() == HeadStatus::WATERING_CMDS);
      Verify(Method(fix.clockMock, progress_watering_due)).Exactly(1);

      SECTION("Nodes with no duration and not NodeStatus::ready and false "
              "phase do not go in "
              "queue") {

        for (size_t addr = 0; addr < config::max_nodes; addr++) {
          auto status = fix.head->get_node_status(addr);
          if (addr != add_check && addr != add_check_2 && addr != add_check_3) {
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
          REQUIRE(msg->data ==
                  Protocol::FrameDataArray{
                      fix.head->get_node_hose_duration(0).value()});
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

                REQUIRE(fix.head->get_active_watering_index() == std::nullopt);
              }
              SECTION("Err status from node will shut down system") {
                auto msg = Mocks::incoming_status_frame(0, NodeStatus::ERR);
                fix.head->handle_incoming_frame(msg);
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

      for (size_t i = 0; i < config::max_nodes; i++) {
        auto msg1 = Mocks::incoming_ack_frame(i);
        fix.head->handle_incoming_frame(msg1);
        auto msg2 = Mocks::incoming_status_frame(i, NodeStatus::READY);
        fix.head->handle_incoming_frame(msg2);
      }

      REQUIRE(fix.head->get_head_status() == HeadStatus::STANDBY);
      REQUIRE(fix.head->get_active_watering_index() == std::nullopt);
    }
    SECTION(
        "Initialization of pairing mode clears out all nodes and resets head") {

      fix.head->ext_req_pairing_mode();
      // NOTE: This will begin pairing procedure
      fix.head->process_external_requests();
      Verify(Method(fix.mockReset, do_it)).AtLeastOnce();
      REQUIRE(fix.head->get_head_status() == HeadStatus::PAIRING);
      REQUIRE(fix.head->get_active_watering_index() == std::nullopt);
      bool clear = true;
      for (size_t i = 0; i < config::max_nodes; i++) {
        if (fix.head->node_exists(i)) {
          clear = false;
        }
      }
      REQUIRE(clear == true);

      SECTION("Process pairing will continue until a no response threshold") {

        for (int i = 0; i < STOP_PAIRING_COUNT - 1; i++) {
          // This is the way to stop pairing
          fix.head->process_pairing(std::nullopt, 1);
          REQUIRE(fix.head->get_no_response_pairing_count() == i + 1);
        }
        fix.head->process_pairing(UartMessage{ADDR_UNSET, CMD::DISCOVERY, {0}},
                                  0);

        REQUIRE(fix.head->get_head_status() == HeadStatus::PAIRING);
        REQUIRE(fix.head->get_no_response_pairing_count() == 0);

        for (int i = 0; i <= STOP_PAIRING_COUNT + 1; i++) {
          fix.head->process_pairing(std::nullopt, 1);
        }

        REQUIRE(fix.head->get_head_status() == HeadStatus::STANDBY);
        REQUIRE(fix.head->get_no_response_pairing_count() == 0);
      }
    }
  } //
}

TEST_CASE("testing calculation and handling of time pool", "[time_pool]") {

  Time::Long PHASE_LEN = 200;
  Fixture fix{PHASE_LEN};
  SECTION("adding nodes will subtract their sum from time_pool") {

    uint8_t val1 = 1;
    uint8_t second_hose_duration = 33;
    NodeTypes::HoseDuration duration1{val1};
    Mocks::populate_single_node(*fix.head, 0, duration1);
    auto pool_check = PHASE_LEN - val1;
    REQUIRE(fix.head->get_remaining_time_pool() == pool_check);

    NodeTypes::HoseDuration duration2{second_hose_duration};
    Mocks::populate_single_node(*fix.head, 1, duration2);
    pool_check -= second_hose_duration;
    REQUIRE(fix.head->get_remaining_time_pool() == pool_check);
    SECTION(
        "modifying existing nodes will accurately recalculate the time pool") {
      auto first_node_replacement = 20;
      Mocks::set_node_duration(*fix.head, 0, first_node_replacement);
      auto new_check =
          PHASE_LEN - first_node_replacement - second_hose_duration;
      REQUIRE(fix.head->get_remaining_time_pool() == new_check);
      SECTION("can reach maximum time pool but not more") {

        auto remaining =
            static_cast<uint16_t>(fix.head->get_remaining_time_pool());
        NodeTypes::HoseDuration max_out = remaining;

        Mocks::populate_single_node(*fix.head, 2, max_out);
        // Make sure using the rest of it worked
        REQUIRE(fix.head->get_node_hose_duration(2) == max_out);

        NodeTypes::HoseDuration failing_durs = 1;
        Mocks::populate_single_node(*fix.head, 3, failing_durs);

        // Can't add a single second more than time pool
        REQUIRE(fix.head->get_node_hose_duration(3) == 0);
        SECTION("calling init pairing mode resets time pool to phase len") {
          fix.head->ext_req_pairing_mode();
          fix.head->process_external_requests();

          REQUIRE(fix.head->get_remaining_time_pool() == PHASE_LEN);
        }
      }
    }
  }
}
// NOTE: we subtract one because node change req type is called by head task.
// also dont call init so - 2
size_t target_req_count = Util::to_i(BLE::Cmds::REQUEST_COUNT) - 2;
TEST_CASE("Processes external requests correctly", "[head_req]") {

  Fixture fix;
  Mocks::populate_head_nodes(*fix.head, 5);

  size_t ready_count = fix.head->get_node_status_count(NodeStatus::READY);
  REQUIRE(ready_count == 5);
  ExtRqManager &mgr = fix.head->extRequestsManager;
  SECTION("Will push and pop from the stack correctly") {
    NodeTypes::DurationSchedule sch{32, {1, 0, 0, 1, 0, 0}};

    fix.head->ext_req_set_clock(12, 12);
    REQUIRE(mgr.peek_request_count() == 1);
    REQUIRE(mgr.peek_request(BLE::Cmds::WRITE_CONF_TIME) != std::nullopt);
    fix.head->ext_req_set_phase(12, 12);
    REQUIRE(mgr.peek_request(BLE::Cmds::WRITE_CONF_PHASE) != std::nullopt);
    fix.head->ext_req_set_node_cycle(Util::water_cycle_to_bytes(sch.cycle), 0);
    REQUIRE(mgr.peek_request(BLE::Cmds::WRITE_CYCLE) != std::nullopt);
    fix.head->ext_req_set_node_duration(sch.duration, 0);
    REQUIRE(mgr.peek_request(BLE::Cmds::WRITE_CELL) != std::nullopt);

    size_t op_count = fix.head->process_external_requests();
    REQUIRE(op_count == target_req_count);
    REQUIRE(mgr.peek_request_count() == 0);
    REQUIRE(mgr.peek_event_count() == target_req_count);

    Verify(Method(fix.clockMock, set_time)).Exactly(1);
    Verify(Method(fix.clockMock, set_next_phase_start_time)).Exactly(1);
    auto check = fix.head->get_node_duration_schedule(0);
    REQUIRE(*check == sch);

    fix.head->ext_req_pairing_mode();
    REQUIRE(mgr.peek_request(BLE::Cmds::INIT_PAIRING) != std::nullopt);
    fix.head->process_external_requests();
    size_t ready_count = fix.head->get_node_status_count(NodeStatus::READY);
    REQUIRE(ready_count == 0);

    REQUIRE(mgr.peek_event_count() == target_req_count + 1);
    SECTION("popping events from events stack works") {
      for (size_t i = target_req_count + 1; i > 0; i--) {
        OptionalRequest optReq = mgr.popEvent();
        REQUIRE(optReq != std::nullopt);
      }

      OptionalRequest optReq = mgr.popEvent();
      REQUIRE(optReq == std::nullopt);
    }

    // REQUIRE(mgr.peek_request_count() == 0);
    // REQUIRE(mgr.peek_request(BLE::Cmds::INIT_PAIRING) == std::nullopt);
  }

  // REQUIRE(mgr.peek_event(BLE::Cmds::WRITE_CELL) == std::nullopt);
}
