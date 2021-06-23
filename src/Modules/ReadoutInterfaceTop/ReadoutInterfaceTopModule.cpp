/**
 * Copyright (C) 2019-2021 CERN
 *
 * DAQling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DAQling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DAQling. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ReadoutInterfaceTopModule.hpp"
#include "Common/DataFormat.hpp"
#include "Utils/Ers.hpp"
#include <chrono>
#include <random>
#include <utility>
using namespace std::chrono_literals;
using namespace std::chrono;
using namespace daqling::module;

ReadoutInterfaceTopModule::ReadoutInterfaceTopModule(const std::string &n) : DAQProcess(n) {
  ERS_DEBUG(0, "With config: " << getModuleSettings());
  m_board_id = getModuleSettings()["board_id"];
  m_delay_us = std::chrono::microseconds(getModuleSettings()["delay_us"]);
  m_min_payload = getModuleSettings()["payload"]["min"];
  m_max_payload = getModuleSettings()["payload"]["max"];
  m_pause = false;
  registerCommand("pause", "pausing", "paused", &ReadoutInterfaceTopModule::pause, this);
  registerCommand("resume", "resuming", "running", &ReadoutInterfaceTopModule::resume, this);
}

void ReadoutInterfaceTopModule::configure() {
  DAQProcess::configure();
  std::this_thread::sleep_for(2s); // some sleep to demonstrate transition states
}

void ReadoutInterfaceTopModule::pause() { m_pause = true; }

void ReadoutInterfaceTopModule::resume() { m_pause = false; }

void ReadoutInterfaceTopModule::start(unsigned run_num) { DAQProcess::start(run_num); }

void ReadoutInterfaceTopModule::stop() { DAQProcess::stop(); }

void ReadoutInterfaceTopModule::runner() noexcept {
  unsigned sequence_number = 0;
  microseconds timestamp{};

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(static_cast<int>(m_min_payload),
                                      static_cast<int>(m_max_payload));
  ERS_DEBUG(0, "Running...");
  while (m_run) {
    if (m_pause) {
      ERS_INFO("Paused at sequence number " << sequence_number);
      while (m_pause && m_run) {
        std::this_thread::sleep_for(10ms);
      }
    }
    timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    const auto payload_size = static_cast<unsigned>(dis(gen));

    ERS_DEBUG(0, "sequence number " << sequence_number << " | timestamp " << std::hex << "0x"
                                    << timestamp.count() << std::dec << " | payload size "
                                    << payload_size);

    DataFragment<data_t1> dataFrag(new data_t1);
    dataFrag->header.payload_size = payload_size;
    dataFrag->header.seq_number = sequence_number;
    dataFrag->header.source_id = m_board_id;
    dataFrag->header.timestamp = static_cast<uint64_t>(timestamp.count());
    memset(dataFrag->payload, 0xFE, payload_size);
    while ((!m_connections.sleep_send(0, dataFrag)) && m_run) {
      ERS_WARNING("put() failed. Trying again");
    };
    ERS_INFO("Succesfully sent data_t1 struct.");

    sequence_number++;
    if (sequence_number == UINT32_MAX) {
      ers::fatal(SequenceLimitReached(ERS_HERE));
    }
    std::this_thread::sleep_for(m_delay_us);
  }
  ERS_DEBUG(0, "Runner stopped");
}
