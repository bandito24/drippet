#include "constants.hpp"
#include "driver.hpp"
class Node {
public:
  Node(Driver &driver) : uart(driver){};
  Esp_Err_t init();

private:
  Driver &uart;
};
