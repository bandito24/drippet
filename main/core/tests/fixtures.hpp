#pragma once

#include "head.hpp"
#include <fakeit.hpp>

struct HeadFixture {
  fakeit::Mock<iValve> valveMock;
  fakeit::Mock<iClock> clockMock;
  Head head;

  HeadFixture() : head{valveMock.get(), clockMock.get()} {};
};
