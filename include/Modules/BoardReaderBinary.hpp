// enrico.gamberini@cern.ch

#ifndef BOARDREADER_HPP_
#define BOARDREADER_HPP_

/// \cond
#include <string>
/// \endcond

#include "Core/DAQProcess.hpp"

class BoardReader : public DAQProcess {
 public:
  BoardReader(std::string name, int num);
  ~BoardReader();
  void start();
  void stop();

  void runner();

 private:
  unsigned m_board_id;
};

#endif /* BOARDREADER_HPP_ */
