/// \cond
#include <chrono>
/// \endcond
using namespace std::chrono_literals;
using namespace std::chrono;

#include <map>
#include <boost/format.hpp>
#include <boost/histogram/ostream.hpp> // write histogram straight to ostream
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream

using namespace boost::histogram;

#include "Modules/Monitor.hpp"

#include "Modules/EventFormat.hpp"

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

extern "C" Monitor *create_object() { return new Monitor; }

extern "C" void destroy_object(Monitor *object) { delete object; }

Monitor::Monitor() { 

   INFO("Monitor::Monitor");

   //initialize_hists( m_histMap );
   initialize_hists( );

   m_timeDelayTolerance = 100. ; // make configurable ?

 }

Monitor::~Monitor() { 
  INFO(__METHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
 }

void Monitor::start() {
  DAQProcess::start();
  INFO(__METHOD_NAME__ << " getState: " << this->getState());

}

void Monitor::stop() {
  DAQProcess::stop();
  INFO(__METHOD_NAME__ << " getState: " << this->getState());
}

void Monitor::runner() {
  INFO(__METHOD_NAME__ << " Running...");

  //histogram< std::tuple<axis::regular<>> , unlimited_storage<> > testhist;

  //INFO("printing test hist ...");
  //std::ostringstream os;
  //os << testhist;
  //std::cout << os.str() << std::endl;


  bool isData(true);

 //std::map<uint32_t, EventFragment> fragments;
  const uint32_t EVENTHEADERSIZE = 24;
  const uint32_t FRAGMENTHEADERSIZE = 24;
  const uint64_t COMBINEDHEADERSIZE = EVENTHEADERSIZE + FRAGMENTHEADERSIZE;
 
  while (m_run) {
    daq::utilities::Binary eventBuilderBinary;

    isData = m_connections.get(1, eventBuilderBinary);
    if ( !isData ) std::this_thread::sleep_for(10ms);
    else {
        INFO(__METHOD_NAME__ <<  " isData "<< isData );
        auto dataTotalSize = eventBuilderBinary.size();

        const EventHeader * eventHeader((EventHeader *)malloc(EVENTHEADERSIZE));
        eventHeader = static_cast<const EventHeader *>(eventBuilderBinary.data());
        uint16_t numChannels = eventHeader->num_fragments;
        uint32_t totalPayloadSize = eventBuilderBinary.size();

        std::map<uint32_t, EventFragmentHeader*> fragmentHeaders;
        std::map<uint32_t, uint16_t> payloadSizes;
        std::map<uint32_t, uint64_t> timeStamps;
        uint32_t accumulatedPayloadSize = 0;

        INFO(__METHOD_NAME__ <<  " about to loop throught "<< numChannels << " channels." ); 
        for ( int chno=0; chno<numChannels; ++chno){

            int ch = chno + 1;

            INFO(__METHOD_NAME__ <<  " channel number "<< ch ); 

            EventFragmentHeader * fragmentHeader((EventFragmentHeader *)malloc(FRAGMENTHEADERSIZE));
            memcpy( fragmentHeader, static_cast<const char*>(eventBuilderBinary.data())+EVENTHEADERSIZE+FRAGMENTHEADERSIZE*(ch-1)+accumulatedPayloadSize, FRAGMENTHEADERSIZE);

            fragmentHeaders[ch] = fragmentHeader;	

            uint16_t payloadSize = fragmentHeader->payload_size; 
            payloadSizes[ch] = payloadSize;
	    uint32_t fragmentStatus = fragmentHeader->status;
            timeStamps[ch] = fragmentHeader->timestamp;
            
            accumulatedPayloadSize += payloadSize;
            INFO(__METHOD_NAME__ <<  " accumulated payload size "<<accumulatedPayloadSize ); 
       
            EventFragment * payload((EventFragment *)malloc(payloadSize));
            memcpy( payload, static_cast<const char*>(eventBuilderBinary.data())+COMBINEDHEADERSIZE, payloadSize);

	    // fill per channel hists.
	    if ( fragmentStatus & CorruptedFragment ) h_fragmenterrors.hist( "Corrupted");
	    if ( fragmentStatus & EmptyFragment ) h_fragmenterrors.hist( "Empty");
	    if ( fragmentStatus & MissingFragment ) h_fragmenterrors.hist( "Missing");
	    if ( fragmentStatus & BCIDMismatch ) h_fragmenterrors.hist( "BCIDMismatch");

        }

        auto totalCalculatedSize = EVENTHEADERSIZE+FRAGMENTHEADERSIZE*2+accumulatedPayloadSize ;
	
	// sanity check
	if ( totalCalculatedSize != totalPayloadSize ) 
	ERROR(__METHOD_NAME__ <<  "total byte size unpacked, "
		              << totalCalculatedSize 
                              <<", not equal to the total payload size, "
                              <<totalPayloadSize
                              <<" given in header. FIX ME." );

        if ( (timeStamps.find(1) == timeStamps.end()) || (timeStamps.find(2) == timeStamps.end()) ) {
        	ERROR(__METHOD_NAME__ <<  "not all channels found. ");
        }
        else{
        	int64_t timedelay = timeStamps[1] - timeStamps[2];
        	h_timedelay_rcv1_rcv2.hist( timedelay );
		INFO(" time delay "<< timedelay );
        }
        if ( !(payloadSizes.find(1) == payloadSizes.end()) ) {
		INFO(" filling payload 1 size "<< payloadSizes[1] );
		h_payloadsize_rcv1.hist( payloadSizes[1]);
        }
	else ERROR(__METHOD_NAME__ <<  "did not find channel 1.");
        if ( !(payloadSizes.find(2) == payloadSizes.end()) ) {
		INFO(" filling payload 1 size "<< payloadSizes[2] );
		h_payloadsize_rcv2.hist( payloadSizes[2]);
        }
	else ERROR(__METHOD_NAME__ <<  "did not find channel 2.");

    }
  }
  
  INFO(__METHOD_NAME__ << " flushing h_payloadsize_rcv1");
  flush_hist( h_payloadsize_rcv1 );
  INFO(__METHOD_NAME__ << " flushing h_payloadsize_rcv2");
  flush_hist( h_payloadsize_rcv2 );
  INFO(__METHOD_NAME__ << " flushing h_timedelay_rcv1_rcv2");
  flush_hist( h_timedelay_rcv1_rcv2 );
  INFO(__METHOD_NAME__ << " flushing h_fragmenterrors");
  flush_hist( h_fragmenterrors );

   // write out
  INFO(__METHOD_NAME__ << " writing out h_payloadsize_rcv2 ");
  write_hist_to_file( h_payloadsize_rcv2, "/home/faser/cantel/");
  write_hist_to_file( h_payloadsize_rcv1, "/home/faser/cantel/");
  write_hist_to_file( h_timedelay_rcv1_rcv2, "/home/faser/cantel/"); 
  write_hist_to_file( h_fragmenterrors, "/home/faser/cantel/"); 


  INFO(__METHOD_NAME__ << " Runner stopped");
}

//void Monitor::initialize_hists( HistMaps &histMap ) {
void Monitor::initialize_hists( ) {

  // histograms
  
  h_timedelay_rcv1_rcv2.hist = make_histogram(axis::regular<>(100, -500., 500., "time [mus]"));
    INFO("making h_payloadsize_rcv1"); 
  h_payloadsize_rcv1.hist = make_histogram(axis::regular<>(100, -500., 500., "time [mus]"));
    INFO("making h_payloadsize_rcv2");
  h_payloadsize_rcv2.hist = make_histogram(axis::regular<>(100, -500., 500., "time [mus]"));
    INFO("making h_fragmenterrors");
  auto axis_fragmenterrors = axis::category<std::string>({"Corrupted","Empty", "Missing", "BCIDMismatch"}, "error type");
  h_fragmenterrors.hist = make_histogram(axis_fragmenterrors);
  
  //
  // using HistMap storage instead.
    /*
    INFO("making h_timedelay_rcv1_rcv2");
    histMap.reghists["h_timedelay_rcv1_rcv2"] = make_histogram(axis::regular<>(100, -500., 500., "time [mus]"));
    INFO("making h_payloadsize_rcv1");
    histMap.reghists["h_payloadsize_rcv1"] = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));
    INFO("making h_payloadsize_rc2");
    histMap.reghists["h_payloadsize_rcv2"] = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));

    INFO("making h_fragmenterrors");
    auto axis_fragmenterrors = axis::category<std::string>({"Corrupted","Empty", "Missing", "BCIDMismatch"}, "error type");
    histMap.cathists["h_fragmenterrors"] = make_histogram(axis_fragmenterrors);
    */

    return ;

}

template <typename T>
void Monitor::flush_hist( T histStruct, bool coverage_all ) {

  INFO(__METHOD_NAME__ << " flushing hist info for "<< histStruct.histname);

  std::ostringstream os;
  auto hist = histStruct.hist;
  auto this_axis = hist.axis();
  auto thiscoverage = coverage::all;
  if ( !coverage_all ) thiscoverage = coverage::inner;

   for (auto x : indexed(hist, thiscoverage) ) { 
      os << boost::format("bin %2i bin value %s : %i\n") % x.index() % this_axis.value(x.index()) % *x;
   }

  std::cout << os.str() << std::flush;

  return ;

}

void Monitor::flush_hist( CategoryHist histStruct, bool coverage_all ) {

  INFO(__METHOD_NAME__ << " flushing hist info for "<<histStruct.histname);
  INFO(__METHOD_NAME__ << " hist axis is of type category; hist has no under/overflow bins. ");

  std::ostringstream os;
  auto hist = histStruct.hist;
  auto this_axis = hist.axis();

   for (auto x : indexed(hist, coverage::inner) ) { // no under/overflow bins for category axis. 
      os << boost::format("bin %2i bin value %s : %i\n") % x.index() % this_axis.value(x.index()) % *x;
   }

  std::cout << os.str() << std::flush;

  return ;

}

template <typename T>
void Monitor::write_hist_to_file( T histStruct, std::string dir, bool coverage_all ) {

  auto hist = histStruct.hist; 
  std::string file_name = dir + "histoutput_"+histStruct.histname+".txt";

  INFO(__METHOD_NAME__ << " writing hist info  to file ... "<< file_name);

  //std::ostringstream os;
  std::ofstream ofs( file_name );

  ofs << "bin \t bin width \t bin center \t x \n";

  auto this_axis = hist.axis();
  auto thiscoverage = coverage::all;
  if ( !coverage_all ) thiscoverage = coverage::inner;
  for (auto x : indexed(hist, thiscoverage) ) { // no under/overflow bins for category axis. 
      ofs << boost::format("%2i \t %2f \t %2f \t %i\n") % x.index() %(x.bin().upper()-x.bin().lower()) % this_axis.value(x.index()) % *x;
   }

  ofs.close();

  return ;

}

void Monitor::write_hist_to_file( CategoryHist histStruct, std::string dir, bool coverage_all ) {

  auto hist = histStruct.hist; 
  std::string file_name = dir + "histoutput_"+histStruct.histname+".txt";

  INFO(__METHOD_NAME__ << " writing hist info  to file ... "<< file_name);

  std::ofstream ofs( file_name );

  ofs << "bin \t bin center \t x \n";

  auto this_axis = hist.axis();
  auto thiscoverage = coverage::inner; // no under/overflow bins for category axis.
  for (auto x : indexed(hist, thiscoverage) ) {
      ofs << boost::format("%2i  \t %2f \t %i\n") % x.index() % this_axis.value(x.index()) % *x;
   }

  ofs.close();

  return ;

}
