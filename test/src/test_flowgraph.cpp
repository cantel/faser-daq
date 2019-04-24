#include "Utilities/Logging.hpp"
#include "tbb/flow_graph.h"

using namespace daq;
using namespace tbb::flow;

struct body {
    std::string my_name;
    body(const char *name) : my_name(name) {}
    void operator()(continue_msg) const {
        printf("%s\n", my_name.c_str());
    }
};

int
main(int argc, char** argv)
{
  INFO("WOOF WOOF -> Test basic TBB FlowGraph");

  graph g;
  continue_node< continue_msg> hello( g,
    []( const continue_msg &) {
        INFO("Hello");
    }
  );
  continue_node< continue_msg> world( g,
    []( const continue_msg &) {
        INFO("World");
    }
  );
  make_edge(hello, world);
  hello.try_put(continue_msg());
  g.wait_for_all();


  graph g2;

  broadcast_node< continue_msg > start(g2);
  continue_node<continue_msg> a(g2, body("A"));
  continue_node<continue_msg> b(g2, body("B"));
  continue_node<continue_msg> c(g2, body("C"));
  continue_node<continue_msg> d(g2, body("D"));
  continue_node<continue_msg> e(g2, body("E"));

  make_edge(start, a);
  make_edge(start, b);
  make_edge(a, c);
  make_edge(b, c);
  make_edge(c, d);
  make_edge(a, e);

  INFO("WOOF WOOF -> Launching dependency graph...");
  for (int i = 0; i < 3; ++i) {
      start.try_put(continue_msg());
      g2.wait_for_all();
      INFO("   -> Next round...");
     
  }

  INFO("WOOF WOOF -> Cleaning house...");
  return 0;
}

