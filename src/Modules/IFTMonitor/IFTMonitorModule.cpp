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
/// \endcond

#include "IFTMonitorModule.hpp"

#define PI 3.14159265

using namespace std::chrono_literals;
using namespace std::chrono;
using Eigen::MatrixXd;


bool IFTMonitorModule::adjacent(unsigned int strip1, unsigned int strip2) {
  return ((strip2 - strip1 == 1) or (strip1 - strip2 == 1));
}


int IFTMonitorModule::average(std::vector<int> strips) {
  size_t n = strips.size();
  return n != 0 ? (int)std::accumulate(strips.begin(), strips.end(), 0.0) / n : 0;
}


double IFTMonitorModule::intersection(double y1, double y2) {
  // calculate line line intersection given two points on each line (top and bottom of each strip)
  // https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
  double det = -kSTRIP_LENGTH * kSTRIP_LENGTH * tan(kSTRIP_ANGLE);
  double ix = kSTRIP_LENGTH * kSTRIP_LENGTH * y1 + kSTRIP_LENGTH * kXMIN * (2*y2 + kSTRIP_LENGTH * tan(kSTRIP_ANGLE));
  return det != 0 ? ix/det : 0;
}


// linear regression in 3 dimensions
// solve \theta = (X^T X)^{-1} X^T y where \theta is giving the coefficients that
// best fit the data and X is the design matrix
// https://gist.github.com/ialhashim/0a2554076a6cf32831ca
std::pair<Vector3, Vector3> linear_fit(const std::vector<Vector3>& spacepoints) {
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
double mse_fit(std::vector<Vector3> track, std::pair<Vector3, Vector3> fit) {
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


IFTMonitorModule::IFTMonitorModule() {}

IFTMonitorModule::~IFTMonitorModule() { 
  INFO("With config: " << m_config.dump());
}

void IFTMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

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
    fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary, SourceIDs::TrackerSourceID + TRBBoardId);
    if (fragmentUnpackStatus) {
      fill_error_status_to_metric(fragmentUnpackStatus);
      return;
    }

    fill_error_status_to_metric(m_fragment->status());
  
    const TrackerDataFragment* trackerdataFragment = m_trackerdataFragment;
    m_eventId = m_trackerdataFragment->event_id();

    if (TRBBoardId == 0)
      DEBUG("event " << m_eventId);
    
    size_t payload_size = m_fragment->payload_size();
    if (payload_size > MAXFRAGSIZE) {
       WARNING(" VERY large payload size received. Payload size of " << payload_size<<" bytes exceeds maximum allowable for histogram filling. Resetting to "<<MAXFRAGSIZE);
       payload_size = MAXFRAGSIZE;
    } 

    if (trackerdataFragment->valid()){
      m_bcid = trackerdataFragment->bc_id();
      m_l1id = trackerdataFragment->event_id();
    }
    else {
      WARNING("Ignoring corrupted data fragment.");
      return;
    }

    m_print_WARNINGS = m_total_WARNINGS < kMAXWARNINGS;

    for (auto it = trackerdataFragment->cbegin(); it != trackerdataFragment->cend(); ++it) {
      auto sctEvent = *it;
      if (sctEvent == nullptr) {
        WARNING("Invalid SCT Event for event " << trackerdataFragment->event_id());
        WARNING("tracker data fragment: " << *trackerdataFragment);
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
      for (unsigned chipIdx1 = 0; chipIdx1 < (unsigned)kCHIPS_PER_MODULE*0.5; chipIdx1++) {
        unsigned int chipIdx2 = kCHIPS_PER_MODULE - 1 - chipIdx1;
        if ((allClusters[chipIdx1].empty()) or (allClusters[chipIdx2].empty())) continue;
        for (auto cluster1 : allClusters[chipIdx1]) {
          for (auto cluster2 : allClusters[chipIdx2]) {
            // invert
            cluster2 = kSTRIPS_PER_CHIP - 1 - cluster2; 

            // check for intersections
            if (std::abs(cluster1-cluster2) > kSTRIPDIFFTOLERANCE) continue;

            // every second moudle is flipped
            int c1 = module % 2 == 0 ? chipIdx1 : 5 - chipIdx1;
            int c2 = module % 2 == 0 ? 5 - (chipIdx2 % 6) : chipIdx2 % 6;

            // calculate intersection
            double y1 = kMODULEPOS[module % 4] + (c1 * kSTRIPS_PER_CHIP + cluster1) * kSTRIP_PITCH;
            double y2 = kMODULEPOS[module % 4] + (c2 * kSTRIPS_PER_CHIP + cluster2) * kSTRIP_PITCH;
            double py = 0.5 * (y1 + y2);
            double px = intersection(y1, y2);

            // add x-offset
            px = module / 4 == 0 ? 2 * kXMIN + px : 2 * kXMAX - px;

            m_histogrammanager->fill2D(m_hit_maps[TRBBoardId], px, py, 1);
            m_spacepoints[TRBBoardId].emplace_back(px, py, kLAYERPOS[TRBBoardId]);
            m_spacepointsList.push_back({m_eventId, TRBBoardId, px, py});
          }
        }
      }
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
      double phi_xz = atan(direction.x() / direction.z()) * 180 / PI;
      double phi_yz = atan(direction.y() / direction.z()) * 180 / PI;
      m_histogrammanager->fill("phi_xz", phi_xz);
      m_histogrammanager->fill("phi_yz", phi_yz);
      m_eventInfo.push_back({m_eventId, origin.x(), origin.y(), origin.z(), phi_xz, phi_yz});
    }
  }

  m_spacepoints.clear();


  // write out debug information every 1000 events
  if (m_eventId % 1000 == 0) {
    for (auto info : m_eventInfo)
      DEBUG("?? " << info.event << ": x " << info.x << ", " << info.y << ", " << info.z << ", phi1 " << info.phi1 << ", phi2 " << info.phi2);
    for (auto sp : m_spacepointsList)
      DEBUG(sp.event << ", " << sp.layer << ", " << sp.x << ", " << sp.y);
  }
}

void IFTMonitorModule::register_hists() {
  INFO(" ... registering histograms in TrackerMonitor ... " );
  const unsigned kPUBINT = 30; // publishing interval in seconds
  for ( const auto& hit_map : m_hit_maps)
    m_histogrammanager->register2DHistogram(hit_map, "x", -kSTRIP_LENGTH, kSTRIP_LENGTH, 40, "y",  -kSTRIP_LENGTH, kSTRIP_LENGTH, 40, kPUBINT);
  m_histogrammanager->registerHistogram("phi_xz", "phi(xz)", -90, 90, 36, kPUBINT);
  m_histogrammanager->registerHistogram("phi_yz", "phi(yz)", -90, 90, 36, kPUBINT);
  INFO(" ... done registering histograms ... " );
}

void IFTMonitorModule::register_metrics() {
  INFO( "... registering metrics in TrackerMonitorModule ... " );
  register_error_metrics();
}
