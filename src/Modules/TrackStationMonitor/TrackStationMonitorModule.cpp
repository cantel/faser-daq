/*
  Copyright (C) 2019-2021 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
#include <iostream>
#include <sstream>
#include <fstream>
#include <math.h>
#include <numeric>
#include "Utils/Ers.hpp"
/// \endcond

#include "TrackStationMonitorModule.hpp"

#define PI 3.14159265

using namespace std::chrono_literals;
using namespace std::chrono;
using Eigen::MatrixXd;


bool TrackStationMonitorModule::adjacent(int strip1, int strip2) {
  return std::abs(strip1 - strip2) <= 2;
}


// calculate line line intersection given two points on each line (top and bottom of each strip)
// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
double TrackStationMonitorModule::intersection(double yf, double yb) {
  double y1 = yf, y2 = yf;
  double y3 = yb + tan(kSTRIP_ANGLE) * kXMIN;
  double y4 = yb + tan(kSTRIP_ANGLE) * kXMAX;
  double x1 = kXMIN, x3 = kXMIN;
  double x2 = kXMAX, x4 = kXMAX;

  double d = (x1-x2) * (y3-y4) - (y1-y2) * (x3-x4);
  double px = (x1*y2 - y1*x2) * (x3-x4) - (x1-x2) * (x3*y4 - y3*x4);
  return d != 0 ? px/d : 0;
}


// linear regression in 3 dimensions
// solve \theta = (X^T X)^{-1} X^T y where \theta is giving the coefficients that
// best fit the data and X is the design matrix
// https://gist.github.com/ialhashim/0a2554076a6cf32831ca
std::pair<Vector3, Vector3> TrackStationMonitorModule::linear_fit(const std::vector<Vector3>& spacepoints) {
  size_t n_spacepoints = spacepoints.size();
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> centers(n_spacepoints, 3);
  for (size_t i = 0; i < n_spacepoints; ++i) centers.row(i) = spacepoints[i];

  Vector3 origin = centers.colwise().mean();
  Eigen::MatrixXd centered = centers.rowwise() - origin.transpose();
  Eigen::MatrixXd cov = centered.adjoint() * centered;
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig(cov);
  Vector3 axis = eig.eigenvectors().col(2).normalized();

  return std::make_pair(origin, axis);
}


// return the mean squared error of the fit
double TrackStationMonitorModule::mse_fit(std::vector<Vector3> track, std::pair<Vector3, Vector3> fit) {
  Vector3 origin = fit.first;
  Vector3 dir = fit.second;

  double rms = 0;
  for (int i  = 0; i < 3; ++i) {
    double lambda = (track[i].z() - origin.z()) / dir.z();
    Vector3 delta = origin + lambda * dir - track[i];
    rms += delta.transpose()*delta;
  }
  return rms;
}

double TrackStationMonitorModule::mean(double* x, int n) {
	double sum = 0;
	for (int i = 0; i < n; i++)
		sum += x[i];
	return sum / n;
}

double TrackStationMonitorModule::rms(double* x, int n) {
	double sum = 0;
	for (int i = 0; i < n; i++)
		sum += pow(x[i], 2);
	return sqrt(sum / n);
}


TrackStationMonitorModule::TrackStationMonitorModule(const std::string& n) : MonitorBaseModule(n) { 
  INFO("");
  auto cfg = getModuleSettings();
  auto cfg_stationID = cfg["stationID"];
  if (cfg_stationID != "" && cfg_stationID != nullptr) {
    DEBUG("read station " << cfg_stationID << " from config");
    m_stationID = cfg_stationID;
  }
  else m_stationID = 0;

  try {
    m_trb_ids = m_map_trb_ids.at(m_stationID);
  }
  catch (const std::out_of_range &e) {
    ERROR("Configured station ID "<<static_cast<int>(m_stationID)<<" does not exist. Check your configuration file.");
    throw MonitorBase::ConfigurationIssue(ERS_HERE, "Exiting now as I don't know which station to monitor.");
  }
  std::stringstream trbid_out;
  std::copy(m_trb_ids.cbegin(), m_trb_ids.cend(), std::ostream_iterator<int>(trbid_out, " "));
  INFO("Reading data from Station ID "<<static_cast<int>(m_stationID)<<" and TRBs with IDs "<<trbid_out.str());

  auto cfg_hitMode = cfg["hitMode"];
  if (cfg_hitMode == "HIT")
    m_hitMode = HitMode::HIT;
  else if (cfg_hitMode == "EDGE")
    m_hitMode = HitMode::EDGE;
  else if (cfg_hitMode == "LEVEL")
    m_hitMode = HitMode::LEVEL;
  else {
      ERROR("Unknown hit mode, allowed values are HIT, EDGE and LEVEL.\nSetting hit mode to HIT");
      m_hitMode = HitMode::HIT;
  }
}

TrackStationMonitorModule::~TrackStationMonitorModule() { 
  INFO("With config: " << m_config.dump());
}

void TrackStationMonitorModule::monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if (m_event->event_tag() != m_eventTag) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  if (m_event->payload_size() > kMAXFRAGSIZE) {
    WARNING("Large event encountered. Skipping event.");
    return;
  }

  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary, SourceIDs::TriggerSourceID);
  if (fragmentUnpackStatus) {
    WARNING("ERROR unpacking trigger fragment");
    return;
  }

  for ( auto TRBBoardId : m_trb_ids ) {

    uint8_t LayerIdx = m_stationID == 0? (TRBBoardId-2)%kTRB_BOARDS : TRBBoardId%kTRB_BOARDS; // adjust for IFT

    try {
      TrackerDataFragment trackerDataFragment = get_tracker_data_fragment(eventBuilderBinary, SourceIDs::TrackerSourceID + TRBBoardId);
      m_eventId = trackerDataFragment.event_id();
      if (LayerIdx == 0)
        DEBUG("event " << m_eventId);

      for (auto sctEvent : trackerDataFragment) {
        if (sctEvent == nullptr) {
          m_status = STATUS_WARN;
          continue;
        }

        if (sctEvent->HasError()) {
          auto sctErrorList= sctEvent->GetErrors();
          for (unsigned idx = 0; idx < 12; idx++) {
            auto sctErrors = sctErrorList.at(idx);
            for (uint8_t sctError : sctErrors) {
              WARNING("SCT Error for module "<<sctEvent->GetModuleID()<<", chip idx "<<idx<<". Error code = "<<static_cast<int>(sctError));
            }
          }
        };

        int diff_bcid = (m_bcid-sctEvent->GetBCID())&0xFF;
        int diff_l1id = (m_l1id-sctEvent->GetL1ID())&0xF;
        if (diff_bcid != kBCIDOFFSET) {
          if (m_print_WARNINGS) WARNING("BCID mismatch for module "<<sctEvent->GetModuleID()<<". TRB BCID = "<<m_bcid<<", SCT BCID = "<<sctEvent->GetBCID());
          m_total_WARNINGS++;
        }
        if (diff_l1id != 0) {
          if (m_print_WARNINGS) WARNING("L1ID mismatch for module "<<sctEvent->GetModuleID()<<". TRB BCID = "<<m_l1id<<", SCT L1ID = "<<sctEvent->GetL1ID());
          m_total_WARNINGS++;
        }



        int module = sctEvent->GetModuleID();

        // combine adjacent strips to a cluster
        std::vector<Cluster> clustersPerChip = {};
        Cluster currentCluster;
        int previousHitPos = 0;
        auto allHits = sctEvent->GetHits();
        std::map<unsigned, std::vector<Cluster>> allClusters;
        for (unsigned chipIdx = 0; chipIdx < kCHIPS_PER_MODULE; chipIdx++) {
          auto hitsPerChip = allHits[chipIdx];
          previousHitPos = 0;
          for (auto hit : hitsPerChip) {
            auto hitPattern = hit.second;
            if (hitPattern == 7) continue;
            if (m_hitMode == HitMode::EDGE && (((hitPattern & 0x2) == 0 ) || ((hitPattern & 0x4) != 0) ) ) continue; // 01X
            if (m_hitMode == HitMode::LEVEL && ((hitPattern & 0x2) == 0)) continue; // X1X
            if ((not adjacent(previousHitPos, hit.first)) and (not currentCluster.empty())) {
              clustersPerChip.push_back(currentCluster);
              currentCluster.clear();
            }
            currentCluster.addHit(hit);
            previousHitPos = hit.first;
          }
          if (not currentCluster.empty()) clustersPerChip.push_back(currentCluster);
          if (not clustersPerChip.empty()) allClusters[chipIdx] = clustersPerChip;
          currentCluster.clear();
          clustersPerChip.clear();
        }


        // create space points
        for (int chipIdx1 = 0; chipIdx1 < (int)kCHIPS_PER_MODULE*0.5; chipIdx1++) {
          int chipIdx2 = kCHIPS_PER_MODULE - 1 - chipIdx1;
          if ((allClusters[chipIdx1].empty()) or (allClusters[chipIdx2].empty())) continue;
          for (auto cluster1 : allClusters[chipIdx1]) {
            for (auto cluster2 : allClusters[chipIdx2]) {

              // every second module is flipped
              // invert for the downstream side the cluster and chip number
              int cluster1Pos = cluster1.position();
              int cluster2Pos = cluster2.position();
              double chip = module % 2 == 0 ? chipIdx1 : 5 - chipIdx1;
              if (module % 2 == 0)
                cluster2Pos = kSTRIPS_PER_CHIP - 1 - cluster2Pos; 
              else
                cluster1Pos = kSTRIPS_PER_CHIP - 1 - cluster1Pos; 

              // check for intersections
              if (std::abs(cluster1Pos-cluster2Pos) > kSTRIPDIFFTOLERANCE) continue;

              // calculate intersection
              double yf = kMODULEPOS[module % 4] + (chip * kSTRIPS_PER_CHIP + cluster1Pos) * kSTRIP_PITCH;
              double yb = kMODULEPOS[module % 4] + (chip * kSTRIPS_PER_CHIP + cluster2Pos) * kSTRIP_PITCH;
              double px = intersection(yf, yb);
              double py = 0.5 * (yf + yb) + kLAYER_OFFSET[LayerIdx];

              // invert every second module and add x-offset
              if (module % 2 == 1) px *= -1;
              px = module / 4 == 0 ? kXMIN - px : kXMAX + px;

              m_x = px;
              m_y = py;
              m_x_vec[m_vec_idx] = px;
              m_y_vec[m_vec_idx] = py;
              m_vec_idx = (m_vec_idx+1) % kAVGSIZE;

              m_histogrammanager->fill2D(m_hit_maps[LayerIdx], px, py, 1);
              m_spacepoints[LayerIdx].emplace_back(SpacePoint({px, py, kLAYERPOS[LayerIdx]}, cluster1.hitPatterns(), cluster2.hitPatterns()));
              if (m_spacepoints[LayerIdx].size() > 10) {
                break;
              }
            }
          }
        }
      }
    } catch (MonitorBase::UnpackDataIssue &e) {
      ERROR("Error checking data packet: "<<e.what()<<" Skipping event!");
    }
  }

  // calculate direction only for events with a space point in each layer
  if (m_spacepoints.size() == 3) {

    // create all combinations of three space points
    std::vector<Tracklet> tracklets;
    for (const auto& p0 : m_spacepoints[0]) {
      for (const auto& p1 : m_spacepoints[1]) {
        for (const auto& p2 : m_spacepoints[2]) {
          tracklets.push_back(Tracklet(p0.position(), p1.position(), p2.position(), p0.hitPatterns(), p1.hitPatterns(), p2.hitPatterns()));
        }
      }
    }

    // fit all track candidates and get candidate with best mean-squared-error
    Vector3 origin;
    Vector3 direction;
    std::map<int, std::vector<int>> layerHitPatternsMap;
    double mse;
    double mse_min = 999;
    for (auto tracklet : tracklets) {
      std::pair<Vector3, Vector3> fit = linear_fit(tracklet.spacePoints());
      mse = mse_fit(tracklet.spacePoints(), fit);
      if (mse < mse_min) {
        mse_min = mse;
        origin = fit.first;
        direction = fit.second;
        layerHitPatternsMap = tracklet.hitPatterns();
      }
    }

    if (mse_min < 999) {
      for (const std::pair<int, std::vector<int>>& layerHitPatterns : layerHitPatternsMap) {
        std::string hname_hitp = m_prefix_hname_hitp+std::to_string(layerHitPatterns.first);
        for (const auto& hitPatterns : layerHitPatterns.second) {
          std::bitset<3> bitset_hitp(hitPatterns);
          m_histogrammanager->fill(hname_hitp, bitset_hitp.to_string());
        }
      }
      double tan_phi_xz = direction.x() / direction.z();
      double tan_phi_yz = direction.y() / direction.z();
      double phi_xz = atan(tan_phi_xz) * 180 / PI;
      double phi_yz = atan(tan_phi_yz) * 180 / PI;
      m_histogrammanager->fill("x_track", origin.x());
      m_histogrammanager->fill("y_track", origin.y());
      m_histogrammanager->fill("phi_xz", phi_xz);
      m_histogrammanager->fill("phi_yz", phi_yz);
      m_histogrammanager->fill("tan_phi_xz", tan_phi_xz);
      m_histogrammanager->fill("tan_phi_yz", tan_phi_yz);
      m_histogrammanager->fill2D("hitmap_track", origin.x(), origin.y(), 1);
      m_number_good_events++;
    }
  }

  m_spacepoints.clear();

  mean_x = mean(m_x_vec, kAVGSIZE);
  mean_y = mean(m_y_vec, kAVGSIZE);
  rms_x = rms(m_x_vec, kAVGSIZE);
  rms_y = rms(m_y_vec, kAVGSIZE);
}

void TrackStationMonitorModule::register_hists() {
  INFO(" ... registering histograms in TrackerMonitor ... " );
  const unsigned kPUBINT = 5; // publishing interval in seconds
  for ( const auto& hit_map : m_hit_maps)
    m_histogrammanager->register2DHistogram(hit_map, "x", -kSTRIP_LENGTH, kSTRIP_LENGTH, 50, "y",  -kSTRIP_LENGTH, kSTRIP_LENGTH, 50, kPUBINT);
  for (uint8_t i = 0; i < kLAYERS; ++i) {
    std::string hname_hitp = m_prefix_hname_hitp+std::to_string(i);
    m_histogrammanager->registerHistogram(hname_hitp, "hit pattern", m_hitp_categories, m_PUBINT);
  }
  m_histogrammanager->register2DHistogram("hitmap_track", "x", -kSTRIP_LENGTH, kSTRIP_LENGTH, 50, "y",  -kSTRIP_LENGTH, kSTRIP_LENGTH, 50, kPUBINT);
  m_histogrammanager->registerHistogram("x_track", "x_track", -128, 128, 25, kPUBINT);
  m_histogrammanager->registerHistogram("y_track", "y_track", -128, 128, 25, kPUBINT);
  m_histogrammanager->registerHistogram("phi_xz", "phi_xz", -5, 5, 100, kPUBINT);
  m_histogrammanager->registerHistogram("tan_phi_xz", "tan(phi_xz)", -0.2, 0.2, 40, kPUBINT);
  m_histogrammanager->registerHistogram("phi_yz", "phi_yz", -2, 2, 100, kPUBINT);
  m_histogrammanager->registerHistogram("tan_phi_yz", "tan(phi_yz)", -0.01, 0.01, 40, kPUBINT);

  INFO(" ... done registering histograms ... " );
}

void TrackStationMonitorModule::register_metrics() {
  INFO( "... registering metrics in TrackerMonitorModule ... " );

  registerVariable(m_x, "X", daqling::core::metrics::AVERAGE);
  registerVariable(m_y, "Y", daqling::core::metrics::AVERAGE);
  registerVariable(mean_x, "mean_x");
  registerVariable(mean_y, "mean_y");
  registerVariable(rms_x, "rms_x");
  registerVariable(rms_y, "rms_y");
  registerVariable(m_number_good_events, "number_good_events");

}
