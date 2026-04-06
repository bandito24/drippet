#include "config.hpp"
#include "gap_manager.hpp"
#include "head.hpp"
class NodeDescAttr {
public:
  NodeDescAttr(Head &head, ConnContext &ctxt)
      : head_node{head}, conn_ctxt(ctxt){};

  std::array<uint8_t, config::max_nodes> get_node_statuses();

private:
  Head &head_node;
  ConnContext &conn_ctxt;
};
