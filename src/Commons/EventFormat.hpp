#pragma once

#include <stdint.h>
#include <stdexcept>
#include <chrono>

#include <Utils/Binary.hpp>

using namespace std::chrono_literals;
using namespace std::chrono;

using namespace daqling::utilities;

enum EventTags {
  PhysicsTag = 0x00,
  CalibrationTag = 0x01,
  MonitoringTag = 0x02,
  TLBMonitoringTag = 0x03
};

enum SourceIDs {
  TriggerSourceID = 0x020000,
  TrackerSourceID = 0x030000,
  PMTSourceID = 0x040000
};
  
const uint16_t EventHeaderVersion = 0x0001;

enum EventStatus { // to be reviewed as not all are relevant for FASER
    UnclassifiedError = 1,
    BCIDMismatch = 1<<1,
    TagMismatch = 1<<2,
    Timeout = 1<<3,
    Overflow = 1<<4,
    CorruptedFragment = 1<<5,
    DummyFragment = 1<<6,
    MissingFragment = 1<<7,  
    EmptyFragment = 1<<8,
    DuplicateFragment = 1<<9
  };
  
class EventFragment {
public:

  EventFragment() = delete;

  /// Sets up new fragment for a binary payload
  EventFragment(uint8_t fragment_tag, uint32_t source_id,
		uint64_t event_id, uint16_t bc_id, const Binary & payload) {
    microseconds timestamp;
    timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch());

    struct EventFragmentHeader new_header;
    new_header.marker         = FragmentMarker;
    new_header.fragment_tag   = fragment_tag;
    new_header.trigger_bits   = 0;
    new_header.version_number = FragmentVersionLatest;
    new_header.header_size    = sizeof(struct EventFragmentHeader);
    new_header.payload_size   = payload.size();
    new_header.source_id      = source_id;
    new_header.event_id       = event_id;
    new_header.bc_id          = bc_id;
    new_header.status         = 0;
    new_header.timestamp      = timestamp.count();
    fragment=Binary(&new_header,new_header.header_size);
    fragment+=payload;
    header=fragment.data<struct EventFragmentHeader*>();
  }
  
  /// Sets up fragment from array of bytes
  EventFragment(const uint8_t *data,size_t size, bool allowExcessData=false) {
    if (size<8) throw std::runtime_error("Too little data for fragment header");
    const struct EventFragmentHeader* newHeader=reinterpret_cast<const struct EventFragmentHeader*>(data);
    if (newHeader->marker!=FragmentMarker) throw std::runtime_error("No fragment header");
    if (newHeader->version_number!=FragmentVersionLatest) {
      //should do conversion here
      throw std::runtime_error("Unsupported fragment version");
    }
    if (size<newHeader->header_size) throw std::runtime_error("Too little data for fragment header");
    if (size<newHeader->header_size+newHeader->payload_size) throw std::runtime_error("Too little data for fragment");
    if ((size!=newHeader->header_size+newHeader->payload_size)&&!allowExcessData) throw std::runtime_error("fragment size does not match header information");
    size=newHeader->header_size+newHeader->payload_size;
    fragment=Binary(data,size);
    header=fragment.data<struct EventFragmentHeader *>();
  }

  /// Sets up fragment from binary blob
  EventFragment(const Binary & frag) : EventFragment(frag.data<uint8_t*>(),frag.size()) {}

  /// Returns the payload
  template <typename T = void *> T payload() const {
    static_assert(std::is_pointer<T>(), "Type parameter must be a pointer type");
    return reinterpret_cast<T>(fragment.data<uint8_t *>()+header->header_size);
  }
  
  /// Return raw fragment 
  const Binary & raw() const { return fragment; }

  /// Set status bits
  void set_status(uint16_t status) {
    header->status = status;
  } 

  /// Set trigger bits
  void set_trigger_bits(uint16_t trigger_bits) {
    header->trigger_bits = trigger_bits;
  } 

  //getters here
  uint64_t event_id() const { return header->event_id; }
  uint8_t  fragment_tag() const { return header->fragment_tag; }
  uint32_t source_id() const { return header->source_id; }
  uint16_t bc_id() const { return header->bc_id; }
  uint16_t status() const { return header->status; }
  uint16_t trigger_bits() const { return header->trigger_bits; }
  uint32_t size() const { return header->header_size+header->payload_size; }
  uint32_t payload_size() const { return header->payload_size; }

private:
  const uint16_t FragmentVersionLatest = 0x0001;
  const uint8_t FragmentMarker = 0xAA;
  struct EventFragmentHeader {
    uint8_t marker;
    uint8_t fragment_tag;
    uint16_t trigger_bits;
    uint16_t version_number;
    uint16_t header_size;
    uint32_t payload_size;
    uint32_t source_id;
    uint64_t event_id;
    uint16_t bc_id;
    uint16_t status;
    uint64_t timestamp;
  }  __attribute__((__packed__));
  struct EventFragmentHeader *header;
  Binary fragment;
};

class EventFull {
public:
  EventFull(uint8_t event_tag, unsigned int run_number,uint64_t event_number) {
    microseconds timestamp;
    timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch());

    header.marker         = EventMarker;
    header.event_tag      = event_tag;
    header.trigger_bits   = 0;
    header.version_number = EventVersionLatest;
    header.header_size    = sizeof(struct EventHeader);
    header.payload_size   = 0;
    header.fragment_count = 0;
    header.run_number     = run_number;
    header.event_id       = 0;
    header.event_counter  = event_number;
    header.bc_id          = 0xFFFF;
    header.status         = 0;
    header.timestamp      = timestamp.count();
  }

  /// Sets up fragment from binary blob
  EventFull(const Binary & event) {
    if (event.size()<sizeof(struct EventHeader)) throw std::runtime_error("Too small to be event");
    header=*event.data<struct EventHeader *>();
    if (header.marker!=EventMarker) throw std::runtime_error("Wrong event header");
    if (header.version_number!=EventVersionLatest) {
      //should do conversion here
      throw std::runtime_error("Unsupported event format version");
    }
    if (size()!=event.size()) throw std::runtime_error("Event size does not match header information");

    uint8_t *data=event.data<uint8_t *>();
    data+=header.header_size;
    uint32_t dataLeft=header.payload_size;

    for(int fragNum=0;fragNum<header.fragment_count;fragNum++) {
      EventFragment *fragment=new EventFragment(data,dataLeft,true);
      data+=fragment->size();
      dataLeft-=fragment->size();
      fragments[fragment->source_id()]=fragment;
    }
  }


  void updateStatus(uint16_t status) {
    header.status|=status;
  }

  Binary* raw() {
    Binary* full=new Binary(&header,sizeof(header));
    for(const auto& frag: fragments) {
      *full+=frag.second->raw();
    }
    return full;
  }

  ~EventFull() {
    for(const auto& frag: fragments) {
      delete frag.second;
    }
  }

  int16_t addFragment(const EventFragment* fragment) { //takes ownership of fragment memory, see above, BP: should use unique ptrs or something
    int16_t status=0;
    if (fragments.find(fragment->source_id())!=fragments.end()) 
      throw std::runtime_error("Duplicate fragment addition!");
    fragments[fragment->source_id()]=fragment;
    if (!header.fragment_count) {
      header.bc_id=fragment->bc_id();
      header.event_id = fragment->event_id();
    }
    header.fragment_count++;
    header.trigger_bits |= fragment->trigger_bits(); //should only come from trigger fragment - BP: add check?
    header.payload_size+=fragment->size();
    if (header.bc_id!=fragment->bc_id()) {
      status |= EventStatus::BCIDMismatch;
    }
    //BP: could check for event ID mismatch, but should not happen...

    updateStatus(fragment->status()|status);
    return status;
  }
  uint8_t event_tag() const { return header.event_tag; }
  uint8_t status() const { return header.status; }
  uint64_t event_id() const { return header.event_id; }
  uint64_t bc_id() const { return header.bc_id; }
  uint32_t size() const { return header.header_size+header.payload_size; }
  uint32_t payload_size() const { return header.payload_size; }

  const EventFragment* find_fragment(uint32_t source_id) const {
    if (fragments.find(source_id)==fragments.end()) return nullptr;
    return fragments.find(source_id)->second;
  }

private:
  const uint16_t EventVersionLatest = 0x0001;
  const uint8_t EventMarker = 0xBB;
  struct EventHeader {
    uint8_t marker;
    uint8_t event_tag;
    uint16_t trigger_bits;
    uint16_t version_number;
    uint16_t header_size;
    uint32_t payload_size;
    uint8_t  fragment_count;
    unsigned int run_number : 24;
    uint64_t event_id;  
    uint64_t event_counter;
    uint16_t bc_id;
    uint16_t status;
    uint64_t timestamp;
  }  __attribute__((__packed__)) header;
  std::map<uint32_t,const EventFragment*> fragments;
};
