/// \cond
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>
#include <exception>
/// \endcond

#include "Core/Command.hpp"
#include "Core/ConnectionManager.hpp"
#include "Utilities/Binary.hpp"
#include "Utilities/Common.hpp"
#include "Utilities/Logging.hpp"

#define REORD_DEBUG
	#define QATCOMP_DEBUG

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace daq::core;
using namespace std::chrono_literals;

bool ConnectionManager::setupCommandConnection(uint8_t ioT, std::string connStr) {
  if (m_is_cmd_setup) {
    INFO(__METHOD_NAME__ << " Command is already online... Won't do anything.");
    return false;
  }
  m_cmd_context = std::make_unique<zmq::context_t>(ioT);
  m_cmd_socket = std::make_unique<zmq::socket_t>(*(m_cmd_context.get()), ZMQ_REP);
  m_cmd_socket->bind(connStr);
  INFO(__METHOD_NAME__ << " Command is connected on: " << connStr);
  m_cmd_handler = std::thread([&]() {
    Command& cmd = Command::instance();
    zmq::message_t cmdMsg;
    while (!m_stop_cmd_handler) {
      // INFO(m_className << " CMD_THREAD: Going for RECV poll...");
      if ((m_cmd_socket->recv(&cmdMsg, ZMQ_DONTWAIT)) == true) {
        std::string cmdmsgStr(static_cast<char*>(cmdMsg.data()), cmdMsg.size());
        // INFO(m_className << " CMD_THREAD: Got CMD: " << cmdmsgStr);
        cmd.setMessage(cmdmsgStr);
        cmd.handleCommand();
        s_send(*(m_cmd_socket.get()), cmd.getResponse());
      }
      // INFO(m_className << " Sleeping a second...");
      std::this_thread::sleep_for(100ms);
    }
  });
  utilities::setThreadName(m_cmd_handler, "cmd", 0);
  m_is_cmd_setup = true;
  return true;
}

bool ConnectionManager::addChannel(uint64_t chn, EDirection dir, uint16_t tag, std::string host,
                                   uint16_t port, size_t queueSize, bool zerocopy) {
  std::ostringstream connStr;
  connStr << "tcp://" << host << ":" << port;
  return addChannel(chn, dir, connStr.str(), queueSize);
}

bool ConnectionManager::addChannel(uint64_t chn, EDirection dir, const std::string& connStr,
                                   size_t queueSize) {
  if (m_sockets.find(chn) != m_sockets.end()) {
    INFO(__METHOD_NAME__ << " Socket for channel already exists... Won't add this channel again.");
    return false;
  }
  uint8_t ioT = 1;
  m_contexts[chn] = std::make_unique<zmq::context_t>(ioT);
  m_sockets[chn] = std::make_unique<zmq::socket_t>(*(m_contexts[chn].get()), ZMQ_PAIR);
  m_pcqs[chn] = std::make_unique<MessageQueue>(queueSize);
  m_directions[chn] = dir;
  try {
    if ( dir == EDirection::SERVER ) {
      m_sockets[chn]->bind(connStr.c_str());
      INFO(__METHOD_NAME__ << " Adding channel for: [" << chn << "] bind: " << connStr);
    } else if ( dir == EDirection::CLIENT ) {
      m_sockets[chn]->connect(connStr.c_str());
      INFO(__METHOD_NAME__ << " Adding channel for: [" << chn << "] connect: " << connStr);
    }
  }
  catch (std::exception& e) {
    ERROR(__METHOD_NAME__ << " Failed to add channel! ZMQ returned: " << e.what());
    return false;
  }
  return true;
}

bool ConnectionManager::addReceiveHandler(uint64_t chn) {
  INFO(__METHOD_NAME__ << " [SERVER] ReceiveHandler for channel [" << chn << "] starting...");
  m_handlers[chn] = std::thread([&, chn]() {
    while (!m_stop_handlers) {
      zmq::message_t msg;
      if ((m_sockets[chn]->recv(&msg, ZMQ_DONTWAIT)) == true) {
        INFO("WOOOOF WOOOF  RECEIVED SOMETHING!");
        m_pcqs[chn]->write(std::move(msg));
        INFO("    -> wrote to queue");
      }
      // INFO(m_className << " No messages for some time... sleeping a second...");
      INFO("SERVER -> queue population: " << m_pcqs[chn]->sizeGuess());
      std::this_thread::sleep_for(100ms);
    }
    INFO(__METHOD_NAME__ << " joining channel [" << chn << "] handler.");
  });
  return true;
}

bool ConnectionManager::addSendHandler(uint64_t chn) {
  INFO(__METHOD_NAME__ << " [CLIENT] SendHandler for channel [" << chn << "] starting...");
  m_handlers[chn] = std::thread([&, chn]() {
    while (!m_stop_handlers) {
      zmq::message_t msg;
      if (m_pcqs[chn]->sizeGuess() != 0) {
        INFO("CLIENT -> queue population: " << m_pcqs[chn]->sizeGuess());
      }
      if (m_pcqs[chn]->read(msg)) {
        // s_send( *(m_sockets[chn].get()), msg );
        m_sockets[chn]->send(msg);
      }
    }
    INFO(__METHOD_NAME__ << " joining channel [" << chn << "] handler.");
  });
  return true;
}

bool ConnectionManager::get(uint64_t chn, daq::utilities::Binary& bin) {
  if (m_pcqs[chn]->sizeGuess() != 0) {
    utilities::Binary msgBin(m_pcqs[chn]->frontPtr()->data(), m_pcqs[chn]->frontPtr()->size());
    m_pcqs[chn]->popFront();
    bin = msgBin;
    return true;
  }
  return false;
}

void ConnectionManager::put(uint64_t chn, utilities::Binary& msgBin) {
  zmq::message_t message(msgBin.size());
  memcpy(message.data(), msgBin.data(), msgBin.size());
  m_pcqs[chn]->write(std::move(message));
}

void ConnectionManager::putStr(uint64_t chn, const std::string& string) {
  zmq::message_t message(string.size());
  memcpy(message.data(), string.data(), string.size());
  m_pcqs[chn]->write(std::move(message));
}

std::string ConnectionManager::getStr(uint64_t chn) {
  std::string s("");
  if (m_pcqs[chn]->sizeGuess() != 0) {
    s = std::string(static_cast<char*>(m_pcqs[chn]->frontPtr()->data()),
                    m_pcqs[chn]->frontPtr()->size());
    m_pcqs[chn]->popFront();
  }
  return s;
}

bool ConnectionManager::start() {
  m_stop_handlers.store(false);
  for (auto const& dirIt : m_directions)  //([first: chn, second:dir])
  {
    switch (dirIt.second) {
      case CLIENT:
        addSendHandler(dirIt.first);
        break;
      case SERVER:
        addReceiveHandler(dirIt.first);
        break;
      case PUBLISHER:
        ERROR(__METHOD_NAME__ << " PUBLISHING requested but it's not implemented!");
        break;
      case SUBSCRIBER:
        ERROR(__METHOD_NAME__ << " SUBSCRIBE requested but it's not implemented!");
        break;
    }
  }
  return true;  // TODO put some meaning or return void
}

bool ConnectionManager::stop() {
  m_stop_handlers.store(true);
  std::this_thread::sleep_for(1s);
  for (auto& tIt : m_handlers) {
    tIt.second.join();
  }
  m_handlers.clear();
  return true;  // TODO put some meaning or return void
}
