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

#pragma once

#include <string>

#include "Core/DAQProcess.hpp"
#include "Common/DataType.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

struct header_t1 {
  uint16_t payload_size;
  uint16_t source_id;
  uint32_t seq_number;
  uint64_t timestamp;
} __attribute__((__packed__));



struct data_t1 {
  header_t1 header{};
  char payload[24000]{};
  inline size_t size() { return sizeof(header) + header.payload_size; }
  inline void *data() { return this; }
  data_t1() = default;
  data_t1(const void *data, const size_t size) { memcpy(this, data, size); }
  // const void* data() const{return this;};
} __attribute__((__packed__));

namespace daqling {

ERS_DECLARE_ISSUE(module, SequenceLimitReached,
                  "Reached maximum sequence number! That's enough for an example...", ERS_EMPTY)
}

class ReadoutInterfaceTopModule : public daqling::core::DAQProcess {
  void pause();
  void resume();

public:
  ReadoutInterfaceTopModule(const std::string & /*n*/);
  void configure() override;
  void start(unsigned run_num) override;
  void stop() override;

  void runner() noexcept override;

private:
  unsigned m_board_id;
  std::chrono::microseconds m_delay_us{};
  size_t m_min_payload, m_max_payload;
  bool m_pause;
};
