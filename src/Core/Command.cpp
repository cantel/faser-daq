#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"

#include "core/Command.hpp"

#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace daq::core;
using namespace std::chrono_literals;


bool daq::core::Command::startCommandHandler()
{
  //m_commandHandler = std::make_unique<daq::utilities::ReusableThread>(10);
  unsigned tid = 1;
  m_commandFunctors.push_back( 
    [&, tid]{
      INFO("CommandThread  ->>> Should handle message: " << m_message);
      setResponse("Booooo");
      setHandled(true);
    }
  );
  
}

bool daq::core::Command::handleCommand()
{
  m_commandHandler->set_work(m_commandFunctors[0]);
  while (busy()){
    std::this_thread::sleep_for(500ms);
  }
  return true;
}

bool daq::core::Command::busy()
{
  bool busy = (m_commandHandler->get_readiness()==false) ? true : false;
  return busy;
}

//template <typename TValue, typename TPred>
//BinarySearchTree<TValue, TPred>::BinarySearchTree() 



/*
template <class ST>
ConnectionManager<ST>::ConnectionManager(m_token)
{
  
}
*/

/*
template <class ST>
ConnectionManager<ST>::~ConnectionManager() {

}
*/

 
