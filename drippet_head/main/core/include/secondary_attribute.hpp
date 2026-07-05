#pragma once
#include "conn_context.hpp"
#include "gatt_attribute.hpp"
#include "head.hpp"

class SecondaryAttr {
public:
  SecondaryAttr(Head &head, const ConnContext &ctxt)
      : head_node(head), conn_ctxt(ctxt){};
  Head &head_node;
  uint16_t chr_handle;
  const ConnContext &conn_ctxt;
};
class NodeDescAttr : public SecondaryAttr {
public:
  using SecondaryAttr::SecondaryAttr;
};

class ExtReqResponseAttr : public SecondaryAttr {
public:
  using SecondaryAttr::SecondaryAttr;
};
class SysConfigAttr : public SecondaryAttr {
public:
  using SecondaryAttr::SecondaryAttr;

  void handle_ext_write_conf(const std::span<uint8_t> &raw_data);
};
