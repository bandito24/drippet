#include "gatt_service.hpp"
#pragma once
#include "task.hpp"
class StatusTask : public Task {

public:
  StatusTask(NodeDescAttr &descAttr, ExtReqResponseAttr &rsp_attr,
             const ConnContext &ctxt)
      : Task("HEAD", 4096, 5), desc_attr(descAttr), rsp_attr{rsp_attr},
        conn_ctxt(ctxt) {}

protected:
  void run() override;

private:
  const NodeDescAttr &desc_attr;
  const ExtReqResponseAttr &rsp_attr;
  const ConnContext &conn_ctxt;
};
