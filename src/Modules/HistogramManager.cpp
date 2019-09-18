#include "Modules/HistogramManager.hpp"
#include <type_traits>
#include <typeinfo>

//using namespace daqling::core;

//HistogramManager::HistogramManager(std::unique_ptr<zmq::socket_t>& histSock, unsigned interval) : m_hist_socket{histSock}, m_interval{interval} {
HistogramManager::HistogramManager( unsigned interval) : m_interval{interval} {
  m_stop_thread = false;
  m_zmq_publisher = false;  
}


HistogramManager::~HistogramManager(){
  m_histogram_map.clear();
  m_stop_thread = true;
  if(m_histogram_thread.joinable()) m_histogram_thread.join();
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

  std::cout<<"publishing histogram"<<std::endl;
  std::cout<<"time is "<<std::time(nullptr)<<std::endl;

  //if(m_zmq_publisher){
    std::ostringstream msg;
    msg<<h->publish();
    zmq::message_t message(msg.str().size());
    memcpy (message.data(), msg.str().data(), msg.str().size());
    std::cout<<" MSG " << msg.str()<<std::endl;
    //bool rc = m_hist_socket->send(message);
    //if(!rc)
    //  WARNING("Failed to publish hist: " << hist->name);
  //}
    h->timestamp = std::time(nullptr);

  return;
}
