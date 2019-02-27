#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include "utilities/Logging.hpp"
#include "utilities/Singleton.hpp"

namespace daq{
namespace core{

class Command : public daq::utilities::Singleton<Command>
{
  public:
    Command() : m_handled(false), m_message{""}, m_response{""} {}
    ~Command(){}

//    std::string getResponse() { return m_response; }
//    bool getHandled() { return m_handled; }

    bool getHandled() { return m_handled; }
    void setHandled(bool handled) { m_handled = handled; }
    std::string getMessage() { return m_message; }
    void setMessage(std::string message) { m_message = message; }
    std::string getResponse() { return m_response; }
    void setResponse(std::string response) { m_response = response; }
    

  private:  
    bool m_handled;
    std::string m_message;
    std::string m_response;

};

}
}

#endif

