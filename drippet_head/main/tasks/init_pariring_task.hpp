
#pragma once
#include "freertos/idf_additions.h"
#include "task.hpp"
#include <functional>

class InitParingTask : public Task {

public:
  InitParingTask(std::function<void()> _head_status_cb)
      : Task("INIT_PAIRING", 4096, 1), head_status_cb(_head_status_cb){};

protected:
  void run() override;

private:
  std::function<void()> head_status_cb;
};
