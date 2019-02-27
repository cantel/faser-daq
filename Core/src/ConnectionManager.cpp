#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"

#include "core/ConnectionManager.hpp"

#include <ctime>
#include <iomanip>
#include <pthread.h>

#define REORD_DEBUG
#define QATCOMP_DEBUG

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace daq::core;


bool ConnectionManager::setupCmdConnection(uint8_t cid, std::string connStr) {
  if (m_is_cmd_setup){
    INFO(__METHOD_NAME__ << " Command socket is already open... Won't do anything.");
    return false;
  }
  m_cmd_context = std::make_unique<zmq::context_t>(cid);
  m_cmd_socket = std::make_unique<zmq::socket_t>(*(m_cmd_context.get()), ZMQ_REP);
  m_cmd_socket->bind(connStr);
  INFO(__METHOD_NAME__ << " Connected to connStr" << connStr);
  m_cmd_handler = std::thread([&](){
    while(!m_stop_handlers){
      zmq::message_t cmd;
      INFO(m_className << " CMD_THREAD: Going for RECV poll...");
      m_cmd_socket->recv(&cmd);
      std::string cmdmsg(static_cast<char*>(cmd.data()), cmd.size());
      INFO(m_className << " CMD_THREAD: Got CMD: " << cmdmsg);

      // RS -> TODO: Handle CMDs! 
      //    Deserialization by a library, or queue in commands to be handled.
      //    The DAQProcess should have a thread that checks for unhandled commands in queue.
      //    Or we have the command handler right here... 
      //       -> e.g.: m_cmd_handler->process(cmd);

      s_send( *(m_cmd_socket.get()), "OK" ); 
    }
  });
  utilities::setThreadName(m_cmd_handler, "cmd", 0);
  m_is_cmd_setup = true;
  return true; 
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

 
