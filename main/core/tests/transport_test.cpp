
#include "driver.hpp"
#include "protocol.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

fakeit::Mock<Driver> driverMock;
UartMessage addressingOutgoing{.address = 2,
                               .command = Protocol::Command::ADDRESSING,
                               .data = Protocol::FrameDataArray{500},
                               .data_length = 1};

TEST_CASE("Messages can correctly receive and send", "[uart]") {
  UartProtocol protocol{driverMock.get()};
  SECTION("")

  // REQUIRE( Factorial(1) == 1 );
}
