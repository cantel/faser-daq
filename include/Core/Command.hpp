#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include "Utilities/Logging.hpp"
#include "Utilities/ReusableThread.hpp"
#include "Utilities/Singleton.hpp"

namespace daq {
namespace core {

class Command : public daq::utilities::Singleton<Command> {
 public:
  Command() : m_should_stop{false}, m_handled(false), m_message{""}, m_response{""} {
    m_commandHandler = std::make_unique<daq::utilities::ReusableThread>(10);
  }
  ~Command() {}

  //    std::string getResponse() { return m_response; }
  //    bool getHandled() { return m_handled; }

  bool startCommandHandler();
  bool executeCommand(std::string& response);
  bool handleCommand();

  bool getHandled() { return m_handled; }
  void setHandled(bool handled) { m_handled = handled; }
  std::string getMessage() { return m_message; }
  void setMessage(std::string message) { m_message = message; }
  std::string getResponse() { return m_response; }
  void setResponse(std::string response) { m_response = response; }
  bool getShouldStop() { return m_should_stop; }
  std::mutex* getMutex() { return &m_mtx; }
  std::condition_variable* getCondVar() { return &m_cv; }

 private:
  bool busy();
  bool m_should_stop;
  bool m_handled;
  std::string m_message;
  std::string m_response;
  std::mutex m_mtx;
  std::condition_variable m_cv;

  // Command handler
  std::unique_ptr<daq::utilities::ReusableThread> m_commandHandler;
  std::vector<std::function<void()>> m_commandFunctors;
};

}  // namespace core
}  // namespace daq

#endif
