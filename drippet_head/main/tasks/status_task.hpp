#pragma once
#include "conn_context.hpp"
#include "gatt_char.hpp"
#include "task.hpp"
class StatusTask : public Task {

public:
  StatusTask(NodeDescChar &descChar, ExtReqChar &rspChar,
             const ConnContext &ctxt)
      : Task("HEAD", 4096, 5), desc_chr(descChar), rsp_chr{rspChar},
        conn_ctxt(ctxt) {}

protected:
  void run() override;

private:
  const NodeDescChar &desc_chr;
  const ExtReqChar &rsp_chr;
  const ConnContext &conn_ctxt;
};
