
#include "config.hpp"
#include "driver.hpp"
#include "fixtures.hpp"
#include "head.hpp"
#include "mocks.hpp"
#include "node.hpp"
#include "protocol.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <optional>

using Fixture = HeadFixture;
constexpr NodeKey_t TEST_KEY = 500;

TEST_CASE("create node pending behaves correctly", "[head]") {

  SECTION("Node Links all start as nullptr") {

    Fixture fix;
    Head head = std::move(fix.head);

    bool all_null = true;
    for (std::size_t i = 0; i < config::max_nodes; i++) {
      if (head.node_exists(i)) {
        all_null = false;
        break;
      }
    }
    REQUIRE(all_null == true);
  }
  SECTION("successfully creates a node in initializing state") {

    Fixture fix;
    Head head = std::move(fix.head);
    auto address = head.create_node_pending(TEST_KEY);
    REQUIRE(*address == 0); // First address location;
    REQUIRE(head.get_node_status(*address) == NodeStatus::INITIALIZING);
  }
  SECTION("head does not allow more than max addresses") {

    Fixture fix;
    Head head = std::move(fix.head);
    bool add_correct = true;
    for (size_t i = 0; i < config::max_nodes; i++) {
      auto address = head.create_node_pending(i);
      if (!address) {
        add_correct = false;
        break;
      }
    }
    REQUIRE(add_correct);
    auto last_address = head.create_node_pending(TEST_KEY);

    auto last_address2 = head.create_node_pending(TEST_KEY + 5);
    REQUIRE(last_address == config::max_nodes);
    REQUIRE(last_address2 ==
            config::max_nodes); // Means it full--this return means full
  }
  SECTION("confirm node pending sets a node as ready if key and address match "
          "for valid index") {
    Fixture fix;
    Head head = std::move(fix.head);
    auto address = head.create_node_pending(TEST_KEY);
    auto addressAgain = head.create_node_pending(TEST_KEY);
    REQUIRE(addressAgain == std::nullopt);

    auto address2 = head.create_node_pending(TEST_KEY + 1);

    auto address3 = head.create_node_pending(TEST_KEY + 2);
    auto status1 = head.confirm_node_pending(TEST_KEY + 1, 0);
    REQUIRE(status1 == NodeLinkStatus::LINK_KEY_MISMATCH);
    REQUIRE(head.get_node_status(1) == NodeStatus::INITIALIZING);

    auto status2 = head.confirm_node_pending(TEST_KEY + 1, 1);
    REQUIRE(status2 == NodeLinkStatus::LINK_OK);
    REQUIRE(head.get_node_status(1) == NodeStatus::READY);
    REQUIRE(head.get_node_status(0) == NodeStatus::INITIALIZING);
    REQUIRE(head.get_node_status(2) == NodeStatus::INITIALIZING);
  }
  SECTION("retrieve_all_nodes accurately gives all arrays") {
    Fixture fix;
    all_durations_t durs = fix.head.retrieve_all_durations();
    all_durations_t check{};
    REQUIRE(durs == check);
    NodeTypes::HoseDurations dur_check = {1, 1, 1, 1, 1};
    Mocks::populate_head_nodes(fix.head, 1);
    fix.head.set_node_durations(0, dur_check);

    all_durations_t durs2 = fix.head.retrieve_all_durations();
    check[0] = dur_check;
    REQUIRE(durs2 == check);
  }
}

TEST_CASE("Head task behaves as expected in the task loop", "[head_task]") {

  SECTION("handle_incoming_frame updates state and emits the correct response "
          "from input") {
    Fixture fix;
    SECTION("reacts correctly to discovery incoming") {
      auto incoming = Mocks::incoming_discovery_frame(Mocks::sample_key);
      auto res = fix.head.handle_incoming_frame(incoming);
      REQUIRE(res != std::nullopt);
      auto r = *res;

      REQUIRE(fix.head.get_node_status(0) == NodeStatus::INITIALIZING);
      REQUIRE(r.address == 0);
      REQUIRE(r.data[0] == Mocks::sample_key);

      // Disregard duplicates
      auto res2 = fix.head.handle_incoming_frame(incoming);
      REQUIRE(res2 == std::nullopt);
      REQUIRE(!fix.head.node_exists(1));
      SECTION("will not set empty watering durations to in queue") {

        auto incoming2 = Mocks::incoming_adressing_frame(0, Mocks::sample_key);
        auto res3 = fix.head.handle_incoming_frame(incoming2);
        // node = 0
        REQUIRE(fix.head.get_node_status(0) == NodeStatus::READY);
        REQUIRE(res3 == std::nullopt);
      }
    }
  }
  SECTION("correctly processes watering schedule") {
    Fixture fix;
    When(Method(fix.clockMock, is_watering_due)).AlwaysReturn(true);
    When(Method(fix.clockMock, progress_watering_due)).AlwaysReturn();
    Mocks::populate_head_nodes(fix.head);
    config::Address add_check = config::max_nodes - 1;
    config::Address add_check_2 = config::max_nodes - 2;

    fix.head.set_node_status(add_check, NodeStatus::INITIALIZING);
    NodeTypes::HoseDurations empty_durs{};
    fix.head.set_node_durations(add_check_2, empty_durs);

    fix.head.process_watering_schedule();

    SECTION("initialize_watering_states test") {
      // Fpr Making sure it doesnt queue empty or non-ready nodes

      for (size_t addr = 0; addr < config::max_nodes; addr++) {
        auto status = fix.head.get_node_status(addr);
        if (addr != add_check && addr != add_check_2) {
          REQUIRE(status == NodeStatus::IN_QUEUE);
        } else if (addr == add_check) {
          REQUIRE(status == NodeStatus::INITIALIZING);
        } else {
          REQUIRE(status == NodeStatus::READY);
        }
      }
      REQUIRE(fix.head.get_head_status() == HeadStatus::WATERING_CMDS);

      Verify(Method(fix.clockMock, progress_watering_due)).Exactly(1);
    }
    SECTION("handling next_watering_frame with watering_cmds status") {

      SECTION("starts with the first node and sets it to command sent status") {
        auto msg = fix.head.next_watering_frame();

        REQUIRE(msg->address == 0);
        REQUIRE(msg->data == Mocks::hose_durations);
        REQUIRE(fix.head.get_node_status(0) == NodeStatus::COMMAND_SENT);

        SECTION("max retries without response will reset node and head state") {

          When(Method(fix.valveMock, close_valve)).AlwaysReturn();
          std::optional<UartMessage> msg2;
          for (size_t i = 0; i < RETRY_NODE_MAX - 1; i++) {
            msg2 = fix.head.next_watering_frame();
          }
          REQUIRE(msg == msg2);

          auto msg3 = fix.head.next_watering_frame();
          REQUIRE(msg3 == std::nullopt);
          REQUIRE(fix.head.get_node_status(0) == NodeStatus::ERR);
          REQUIRE(fix.head.get_head_status() == HeadStatus::FAULTY_NODE);
          Verify(Method(fix.valveMock, close_valve)).AtLeastOnce();
        }
        SECTION("unsuccessful sends will increase retry count and successfull "
                "connection attempt will reset attempts") {

          for (size_t i = 0; i < RETRY_NODE_MAX - 1; i++) {
            fix.head.next_watering_frame();
          }

          REQUIRE(fix.head.get_node_retry_count(0) == RETRY_NODE_MAX - 1);

          auto ackMsg =
              fix.head.handle_incoming_frame(Mocks::incoming_ack_frame(0));

          REQUIRE(fix.head.get_node_retry_count(0) == 0);
        }
        SECTION("receiving an ack from node on watering will allow head to "
                "move to next node") {
          REQUIRE(fix.head.get_node_status(0) == NodeStatus::COMMAND_SENT);
          auto ackMsg =
              fix.head.handle_incoming_frame(Mocks::incoming_ack_frame(0));
          REQUIRE(fix.head.get_node_status(0) == NodeStatus::READY);
          REQUIRE(ackMsg->address == 0);
          REQUIRE(ackMsg->command == Protocol::Command::ACK);
          auto next_msg = fix.head.next_watering_frame();
          REQUIRE(fix.head.get_node_status(1) == NodeStatus::COMMAND_SENT);
        }
        SECTION("once all nodes are reset back to ready state the head returns "
                "to standby state") {
          for (config::Address i = 0; i < config::max_nodes; i++) {
            fix.head.set_node_status(i, NodeStatus::READY);
          }
          auto finalRes = fix.head.next_watering_frame();
          REQUIRE(finalRes == std::nullopt);
          REQUIRE(fix.head.get_head_status() == HeadStatus::STANDBY);
        }
      }
    }
  }
}
