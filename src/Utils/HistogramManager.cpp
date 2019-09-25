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
  m_histogram_map.clear();
}

bool HistogramManager::configure(unsigned interval) {
  m_interval = interval;
  return true; 
}

void HistogramManager::start(){
  std::cout<<"Start"<<std::endl;
  m_histogram_thread = std::thread(&HistogramManager::CheckHistograms,this);
}

void HistogramManager::CheckHistograms(){

  std::cout<<"HistogramManager thread about to spawn..."<<std::endl;
  
  while(!m_stop_thread){

    for (auto &pair : m_histogram_map ) { 
      HistBase * x = pair.second;
      if(std::difftime(std::time(nullptr), x->timestamp) >= x->delta_t){
            publish(x);
      }
    }
  }

}

void HistogramManager::publish( HistBase * h){

  std::ostringstream msg;
  msg<<h->name<<": "<<h->publish();
  std::cout<<"START_OF_MSG:" <<std::endl << msg.str()<<std::endl;
  if(m_zmq_publisher){
     zmq::message_t message(msg.str().size());
     memcpy (message.data(), msg.str().data(), msg.str().size());
     bool rc = m_stat_socket->send(message);
     if(!rc)
        std::cout<<"WARNING: Failed to publish histogram with name "<<h->name<<std::endl;
   }
  h->timestamp = std::time(nullptr);

  return;
}
