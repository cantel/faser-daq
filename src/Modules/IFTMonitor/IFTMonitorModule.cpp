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

#include "IFTMonitorModule.hpp"

#define PI 3.14159265

using namespace std::chrono_literals;
using namespace std::chrono;
using Eigen::MatrixXd;


bool IFTMonitorModule::adjacent(int strip1, int strip2) {
  return std::abs(strip1 - strip2) <= 2;
}


int IFTMonitorModule::average(std::vector<int> strips) {
  size_t n = strips.size();
  return n != 0 ? (int)std::accumulate(strips.begin(), strips.end(), 0.0) / n : 0;
}


// calculate line line intersection given two points on each line (top and bottom of each strip)
// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
double IFTMonitorModule::intersection(double yf, double yb) {
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
std::pair<Vector3, Vector3> IFTMonitorModule::linear_fit(const std::vector<Vector3>& spacepoints) {
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
double IFTMonitorModule::mse_fit(std::vector<Vector3> track, std::pair<Vector3, Vector3> fit) {
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

double IFTMonitorModule::mean(double* x, int n) {
	double sum = 0;
	for (int i = 0; i < n; i++)
		sum += x[i];
	return sum / n;
}

double IFTMonitorModule::rms(double* x, int n) {
	double sum = 0;
	for (int i = 0; i < n; i++)
		sum += pow(x[i], 2);
	return sqrt(sum / n);
}


IFTMonitorModule::IFTMonitorModule(const std::string& n) : MonitorBaseModule(n) { 
  INFO("");
}

IFTMonitorModule::~IFTMonitorModule() { 
  INFO("With config: " << m_config.dump());
}

void IFTMonitorModule::monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if (m_event->event_tag() != m_eventTag) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary, SourceIDs::TriggerSourceID);
  if (fragmentUnpackStatus) {
    fill_error_status_to_metric(fragmentUnpackStatus);
    return;
  }

  if (m_tlbdataFragment->tbp() & 0x10) {
    WARNING("Skipping random trigger data");
    return;
  }


  for (int TRBBoardId=0; TRBBoardId < kTRB_BOARDS; TRBBoardId++) {

    try {
      TrackerDataFragment trackerDataFragment = get_tracker_data_fragment(eventBuilderBinary, SourceIDs::TrackerSourceID + TRBBoardId);
      m_eventId = trackerDataFragment.event_id();
      if (TRBBoardId == 0)
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
        std::vector<int> clustersPerChip = {};
        std::vector<int> currentCluster = {};
        int previousStrip = 0;
        int currentStrip;
        auto allHits = sctEvent->GetHits();
        std::map<unsigned, std::vector<int>> allClusters;
        for (unsigned chipIdx = 0; chipIdx < kCHIPS_PER_MODULE; chipIdx++) {
          auto hitsPerChip = allHits[chipIdx];
          previousStrip = 0;
          for (auto hit : hitsPerChip) {
            if (hit.second == 7) continue;
            currentStrip = hit.first;
            if ((not adjacent(previousStrip, currentStrip)) and (not currentCluster.empty())) {
              clustersPerChip.push_back(average(currentCluster));
              currentCluster.clear();
            }
            currentCluster.push_back(currentStrip);
            previousStrip = currentStrip;
          }
          if (not currentCluster.empty()) clustersPerChip.push_back(average(currentCluster));
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
              double chip = module % 2 == 0 ? chipIdx1 : 5 - chipIdx1;
              if (module % 2 == 0)
                cluster2 = kSTRIPS_PER_CHIP - 1 - cluster2; 
              else
                cluster1 = kSTRIPS_PER_CHIP - 1 - cluster1; 

              // check for intersections
              if (std::abs(cluster1-cluster2) > kSTRIPDIFFTOLERANCE) continue;

              // calculate intersection
              double yf = kMODULEPOS[module % 4] + (chip * kSTRIPS_PER_CHIP + cluster1) * kSTRIP_PITCH;
              double yb = kMODULEPOS[module % 4] + (chip * kSTRIPS_PER_CHIP + cluster2) * kSTRIP_PITCH;
              double px = intersection(yf, yb);

              double py = 0.5 * (yf + yb) + kLAYER_OFFSET[TRBBoardId];

              // invert every second module and add x-offset
              if (module % 2 == 1) px *= -1;
              px = module / 4 == 0 ? kXMIN - px : kXMAX + px;

              m_x = px;
              m_y = py;
              m_x_vec[m_vec_idx] = px;
              m_y_vec[m_vec_idx] = py;
              m_vec_idx = (m_vec_idx+1) % kAVGSIZE;

              m_histogrammanager->fill2D(m_hit_maps[TRBBoardId], px, py, 1);
              if (TRBBoardId == 0) {
                m_histogrammanager->fill("x_l0", px);
                m_histogrammanager->fill("y_l0", py);
              }
              m_spacepoints[TRBBoardId].emplace_back(px, py, kLAYERPOS[TRBBoardId]);
              m_spacepointsList.push_back({m_eventId, TRBBoardId, px, py});
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
    std::vector<std::vector<Vector3>> tracks;
    for (const auto& p0 : m_spacepoints[0]) {
      for (const auto& p1 : m_spacepoints[1]) {
        for (const auto& p2 : m_spacepoints[2]) {
          tracks.push_back({p0, p1, p2});
        }
      }
    }

    // fit all track candidates and get candidate with best mean-squared-error
    Vector3 origin;
    Vector3 direction;
    double mse;
    double mse_min = 999;
    for (auto track : tracks) {
      std::pair<Vector3, Vector3> fit = linear_fit(track);
      mse = mse_fit(track, fit);
      if (mse < mse_min) {
        mse_min = mse;
        origin = fit.first;
        direction = fit.second;
      }
    }

    if (mse_min < 999) {
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
      m_eventInfo.push_back({m_eventId, origin.x(), origin.y(), origin.z(), tan_phi_xz, tan_phi_yz, mse_min});
      m_number_good_events++;
    }
  }

  m_spacepoints.clear();

  // write out debug information every 1000 events
  if (m_eventId % 1000 == 0) {
    for (auto info : m_eventInfo)
      DEBUG(info.event << ", " << info.x << ", " << info.y << ", " << info.z << ", " << info.phi1 << ", " << info.phi2 << ", " << info.mse);
    for (auto sp : m_spacepointsList)
      DEBUG(sp.event << ", " << sp.layer << ", " << sp.x << ", " << sp.y);
  }

  mean_x = mean(m_x_vec, kAVGSIZE);
  mean_y = mean(m_y_vec, kAVGSIZE);
  rms_x = rms(m_x_vec, kAVGSIZE);
  rms_y = rms(m_y_vec, kAVGSIZE);
}

void IFTMonitorModule::register_hists() {
  INFO(" ... registering histograms in TrackerMonitor ... " );
  const unsigned kPUBINT = 5; // publishing interval in seconds
  for ( const auto& hit_map : m_hit_maps)
    m_histogrammanager->register2DHistogram(hit_map, "x", -kSTRIP_LENGTH, kSTRIP_LENGTH, 50, "y",  -kSTRIP_LENGTH, kSTRIP_LENGTH, 50, kPUBINT);
  m_histogrammanager->register2DHistogram("hitmap_track", "x", -kSTRIP_LENGTH, kSTRIP_LENGTH, 50, "y",  -kSTRIP_LENGTH, kSTRIP_LENGTH, 50, kPUBINT);
  m_histogrammanager->registerHistogram("x_l0", "x_l0", -128, 128, 50, kPUBINT);
  m_histogrammanager->registerHistogram("y_l0", "y_l0", -128, 128, 50, kPUBINT);
  m_histogrammanager->registerHistogram("x_track", "x_track", -128, 128, 50, kPUBINT);
  m_histogrammanager->registerHistogram("y_track", "y_track", -128, 128, 50, kPUBINT);
  m_histogrammanager->registerHistogram("phi_xz", "phi_xz", -5, 5, 100, kPUBINT);
  m_histogrammanager->registerHistogram("tan_phi_xz", "tan(phi_xz)", -0.2, 0.2, 40, kPUBINT);
  m_histogrammanager->registerHistogram("phi_yz", "phi_yz", -2, 2, 100, kPUBINT);
  m_histogrammanager->registerHistogram("tan_phi_yz", "tan(phi_yz)", -0.01, 0.01, 40, kPUBINT);
  INFO(" ... done registering histograms ... " );
}

void IFTMonitorModule::register_metrics() {
  INFO( "... registering metrics in TrackerMonitorModule ... " );

  registerVariable(m_x, "X", daqling::core::metrics::AVERAGE);
  registerVariable(m_y, "Y", daqling::core::metrics::AVERAGE);
  registerVariable(mean_x, "mean_x");
  registerVariable(mean_y, "mean_y");
  registerVariable(rms_x, "rms_x");
  registerVariable(rms_y, "rms_y");
  registerVariable(m_number_good_events, "number_good_events");

  register_error_metrics();
}
