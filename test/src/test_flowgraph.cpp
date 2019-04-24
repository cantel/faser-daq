#include "Utilities/Logging.hpp"
#include "tbb/flow_graph.h"

using namespace daq;
using namespace tbb::flow;

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

  INFO("WOOF WOOF -> Cleaning house...");
  return 0;
}

