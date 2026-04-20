#include "gatt_service.hpp"
#pragma once
#include "task.hpp"
class StatusTask : public Task {

public:
  StatusTask(NodeDescAttr &descAttr, const ConnContext &ctxt)
      : Task("HEAD", 4096, 5), desc_attr(descAttr), conn_ctxt(ctxt) {}

protected:
  void run() override;

private:
  const NodeDescAttr &desc_attr;
  const ConnContext &conn_ctxt;
};
