#include "time.hpp"
#include <head.hpp>

#include <iostream>
#include <memory>

int main() {

  MainValve mainValve;
  std::unique_ptr<iClock> clock = initialize_clock();
  Head headNode(mainValve, *clock);
  headNode.init_node_discovery();

  std::array<Time::Time_Seconds, config::node_hose_count> times1;
  for (std::size_t i = 0; i < config::node_hose_count; i++) {
    times1.at(i) = Time::Time_Seconds{i * 3};
  }
  iNode *node = headNode.get_node(0);
  if (node) {
    // node->configure_watering_schedule(Time::Time_Seconds{1000}, times1);
    auto val = node->get_node_status();

  } else {
    std::cout << "what ?" << std::endl;
  }
  return 0;
}
