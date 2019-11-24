#include "HistogramManager.hpp"
#include <type_traits>
#include <typeinfo>

HistogramManager::HistogramManager(std::unique_ptr<zmq::socket_t>& statSock, unsigned interval) : m_stat_socket{statSock}, m_interval{interval} {
  m_stop_thread = false;
  m_zmq_publisher = false;  
}


HistogramManager::~HistogramManager(){
  m_stop_thread = true;
  if(m_histogram_thread.joinable()) m_histogram_thread.join();

  for (std::map<std::string, HistBase *>::iterator hist_itr = m_histogram_map.begin(); hist_itr!=m_histogram_map.end(); hist_itr++){
    delete (hist_itr->second);
  }

  m_histogram_map.clear();
}

bool HistogramManager::configure(unsigned interval) {
  m_interval = interval;
  return true; 
}

void HistogramManager::start(){

  INFO("HistogramManager thread about to spawn...");
  
  m_histogram_thread = std::thread(&HistogramManager::CheckHistograms,this);
}

void HistogramManager::CheckHistograms(){

  while(!m_stop_thread){

    for (auto &pair : m_histogram_map ) { 
      HistBase * x = pair.second;
      if(std::difftime(std::time(nullptr), x->timestamp) >= x->delta_t){
            publish(x);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(m_interval));
  }

}

void HistogramManager::publish( HistBase * h){

  std::ostringstream msg;
  msg<<h->name<<": "<<h->publish();
  DEBUG("START_OF_MSG:" <<std::endl << msg.str());
  if(m_zmq_publisher){
     zmq::message_t message(msg.str().size());
     memcpy (message.data(), msg.str().data(), msg.str().size());
     bool rc = m_stat_socket->send(message);
     if(!rc)
        WARNING("Failed to publish histogram with name "<<h->name);
   }
  h->timestamp = std::time(nullptr);
 
  return;
}
