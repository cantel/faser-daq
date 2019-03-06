#ifndef DAQ_CORE_CONNECTION_MANAGER_HH_
#define DAQ_CORE_CONNECTION_MANAGER_HH_

#include "utilities/Singleton.hpp"
#include "utilities/ProducerConsumerQueue.hpp"
#include "utilities/ReusableThread.hpp"

#include "core/Command.hpp"

//#include "zmq.hpp"
//#include "netio/netio.hpp"

#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <algorithm>

#include "zmq.hpp"
#include "utilities/zhelpers.hpp"

//#define MSGQ
//#define QACHECK
//#define REORD_DEBUG

namespace daq{
namespace core{

/*
 * ConnectionManager
 * Author: Roland.Sipos@cern.ch
 * Description: Wrapper class for sockets and SPSC circular buffers.
 *   Makes the communication between DAQ processes easier and scalable.
 * Date: November 2017
*/

//template <class CT, class ST>
class ConnectionManager : public daq::utilities::Singleton<ConnectionManager>
{
public:

  // 
  ConnectionManager() 
    : m_is_cmd_setup{false}, m_stop_handlers{false} 
  { }
  ~ConnectionManager() { m_stop_handlers = true; m_cmd_handler.join(); } 

  // Functionalities
  bool setupCommandConnection(uint8_t cid, std::string connStr);
  

  bool addChannel(uint64_t chn, uint16_t tag, std::string host, uint16_t port, size_t queueSize, bool zerocopy) { return false; }
  bool addChannel(const std::string& connectionStr, size_t queueSize) { return false; }
  bool connect(uint64_t chn, uint16_t tag) { return false; } // Connect/subscriber to given channel.
  bool disconnect(uint64_t chn, uint16_t tag) { return false; } // Disconnect/unsubscriber from a given channel.
  void start() {} // Starts the subscri threads.
  void stopSubscribers() {}  // Stops the subscriber threads.
  bool busy() { return false; } // are processor threads busy
  void startProcessors() {}  // Start data processor threads when available.
  void stopProcessors() {}  // Stops data processors threads when available.
  size_t getNumOfChannels() { return m_activeChannels; } // Get the number of active channels.

private:
  const std::string m_className = "ConnectionManager";
  size_t m_activeChannels;

  //Configuration:
  std::vector<uint64_t> m_channels;

  // Queues 
//#ifdef MSGQ
//  std::map<uint64_t, UniqueMessageQueue> m_pcqs; // Queues for elink RX.
//#else 
//  std::map<uint64_t, UniqueFrameQueue> m_pcqs;
//#endif

  // Network library handling
  std::thread m_cmd_handler;
  std::unique_ptr<zmq::context_t> m_cmd_context;
  std::unique_ptr<zmq::socket_t> m_cmd_socket;
  std::atomic<bool> m_is_cmd_setup;

  std::map<uint64_t, std::unique_ptr<zmq::context_t> > m_contexts; // context descriptors
  std::map<uint64_t, std::unique_ptr<zmq::socket_t> > m_sockets;  // sockets.

  // Threads
  std::vector<std::thread> m_socketHandlers;
  std::vector<std::unique_ptr<daq::utilities::ReusableThread>> m_processors;
  std::vector<std::function<void()>> m_functors;

  // Thread control
  std::atomic<bool> m_stop_handlers;
  std::atomic<bool> m_stop_processors;
  std::atomic<bool> m_cpu_lock; 

  std::mutex m_mutex;
  std::mutex m_mtx_cleaning;
  
};

}
}

#endif
