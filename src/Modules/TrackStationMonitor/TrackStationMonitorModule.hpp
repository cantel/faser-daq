/*
  Copyright (C) 2019-2021 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"
#include <Eigen/Dense>
#include <vector>
#include <map>

typedef Eigen::Matrix<double, 3, 1> Vector3;


class TrackStationMonitorModule : public MonitorBaseModule {
 public:
  TrackStationMonitorModule(const std::string&);
  ~TrackStationMonitorModule();

 protected:

  void monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void register_hists( );
  void register_metrics();
  std::atomic<float> m_x;
  std::atomic<float> m_y;
  std::atomic<float> mean_x;
  std::atomic<float> mean_y;
  std::atomic<float> rms_x;
  std::atomic<float> rms_y;
  std::atomic<int> m_number_good_events = 0;

 private:

  const std::map<uint8_t,std::array<uint8_t,3>> m_map_trb_ids = {{0,{11,12,13}},{1,{0,1,2}},{2,{3,4,5}},{3,{6,7,8}}}; // Station 0: IFT , Station 1-3: Spectrometer trackers
  std::array<uint8_t,3> m_trb_ids;  // filled at initialize

  struct Cluster {
   public:
    Cluster() = default;
    void addHit(std::pair<int, int> hit) {
      m_hitPositions.push_back(hit.first);
      m_hitPatterns.push_back(hit.second);
    }
    int position() const {
      size_t n = m_hitPositions.size();
      return n != 0 ? (int)std::accumulate(m_hitPositions.begin(), m_hitPositions.end(), 0.0) / n : 0;
    }
    std::vector<int> hitPatterns() const { return m_hitPatterns; }
    bool empty() const { return m_hitPositions.size() == 0; }
    void clear() {
      m_hitPositions.clear();
      m_hitPatterns.clear();
    }
   private:
    std::vector<int> m_hitPositions {};
    std::vector<int> m_hitPatterns {};
  };
  struct SpacePoint {
   public:
    SpacePoint(const Vector3& sp, const std::vector<int>& hitPatterns0, const std::vector<int>& hitPatterns1) : m_sp(sp) {
      m_hitPatterns.reserve(hitPatterns0.size() + hitPatterns1.size());
      m_hitPatterns.insert(m_hitPatterns.end(), hitPatterns0.begin(), hitPatterns0.end());
      m_hitPatterns.insert(m_hitPatterns.end(), hitPatterns1.begin(), hitPatterns1.end());
    }
    Vector3 position() const { return m_sp; }
    std::vector<int> hitPatterns() const { return m_hitPatterns; }
   private:
    Vector3 m_sp;
    std::vector<int> m_hitPatterns;
  };
  struct Tracklet {
   public:
    Tracklet(Vector3 sp0, Vector3 sp1, Vector3 sp2,
        std::vector<int> hitPatterns0, std::vector<int> hitPatterns1, std::vector<int> hitPatterns2) :
        m_sp0(sp0), m_sp1(sp1), m_sp2(sp2) {
      m_layerHitPatternMap[0] = hitPatterns0;
      m_layerHitPatternMap[1] = hitPatterns1;
      m_layerHitPatternMap[2] = hitPatterns2;
    }
    std::vector<Vector3> spacePoints() const { return {m_sp0, m_sp1, m_sp2}; }
    std::map<int, std::vector<int>> hitPatterns() const { return m_layerHitPatternMap; }
   private:
    Vector3 m_sp0, m_sp1, m_sp2;
    std::map<int, std::vector<int>> m_layerHitPatternMap;
  };

  enum class HitMode { HIT = 0, LEVEL, EDGE };
  HitMode m_hitMode {HitMode::HIT};

  bool adjacent(int strip1, int strip2);
  double intersection(double y1, double y2);
  std::pair<Vector3, Vector3> linear_fit(const std::vector<Vector3>& spacepoints);
  double mse_fit(std::vector<Vector3> track, std::pair<Vector3, Vector3> fit);
  double mean(double* x, int n);
  double rms(double* x, int n);

  std::map<int, std::vector<SpacePoint>> m_spacepoints = {};
  uint8_t m_stationID = 0;
  const uint32_t kMAXFRAGSIZE=380; // max size in bytes for biggest TRB event fragment for track station to be analysed
  const std::string m_hit_maps[3] = {"hitmap_l0", "hitmap_l1", "hitmap_l2"};
  const std::string m_prefix_hname_hitp = "hitpattern_layer";
  const std::vector<std::string> m_hitp_categories = { "000", "001", "010", "011", "100", "110"};

  double kLAYERPOS[3] = {16.2075, 47.7075, 79.2075}; // values in mm
  double kMODULEPOS[4] = {64.92386246, 1.20386696, -62.55613708, -126.25613403}; // values in mm
  int m_vec_idx = 0;
  static const int kAVGSIZE = 10000;
  double m_x_vec[kAVGSIZE] = {0};
  double m_y_vec[kAVGSIZE] = {0};
  const double kLAYER_OFFSET[3] = {0, -5, 5}; // in mm
  const double kSTRIP_PITCH = 0.08; // in mm
  const double kXMIN = -63.96; // in mm
  const double kXMAX = 63.96; // in mm
  const double kSTRIP_LENGTH = 126.08; // in mm
  const double kSTRIP_ANGLE = 0.04; // in radian
  int m_eventId = 0;
  uint16_t m_bcid;
  uint32_t m_l1id;
  uint16_t number;
  uint16_t module;
  unsigned m_total_WARNINGS;
  bool m_print_WARNINGS;
  const uint8_t kLAYERS = 3;
  const uint8_t kTRB_BOARDS = 3;
  const uint8_t kSTRIPDIFFTOLERANCE = 32; // arctan(0.04) * 63.96mm / 0.08mm = 32
  const uint8_t kSTRIPS_PER_CHIP = 128;
  const uint8_t kCHIPS_PER_MODULE = 12;
  const uint8_t kTOTAL_MODULES = 8;
  const uint8_t kBCIDOFFSET = 4;
  const uint8_t kMAXWARNINGS = 50;
};
