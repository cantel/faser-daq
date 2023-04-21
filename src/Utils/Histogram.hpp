/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include <boost/histogram.hpp>
#include <boost/format.hpp>
#include <boost/histogram/ostream.hpp> // write histogram straight to ostream
#include <ctime>
#include <iostream>
#include <nlohmann/json.hpp> // dump Hist as json structure
#include <algorithm> // std::fill
#include <mutex>

using namespace boost::histogram;
using json = nlohmann::json;

//currently supporting:
//    - real number histograms with fixed widths.
//    - category histograms with labelled bins (labels are "std::string"s)
//may consider more types in future: https://www.boost.org/doc/libs/1_71_0/libs/histogram/doc/html/histogram/overview.html#histogram.overview.rationale.structure.axis

/*
struct Axis{
  float vmin;
  float vmax;
  float vbins;
  bool extendable = false;
}*/

using axis_t = axis::regular<>; 
using hist_t = decltype(make_histogram(std::declval<axis_t>()));
using stretchy_axis_t = axis::regular<double, use_default, use_default, axis::option::growth_t>; 
using stretchy_hist_t = decltype(make_histogram(std::declval<stretchy_axis_t>()));
using hist2d_t = decltype(make_histogram(std::declval<axis_t>(), std::declval<axis_t>()));
using categoryaxis_t = axis::category<std::string>;
using categoryhist_t = decltype(make_histogram(std::declval<categoryaxis_t>())); 

class HistBase {
  public:
  HistBase(){}
  HistBase(std::string name, std::string xlabel, std::string ylabel, float xmin, float xmax, unsigned int xbins, bool extendable, float delta_t) : name(name), title(name), xlabel(xlabel), ylabel(ylabel), xmin(xmin), xmax(xmax), xbins(xbins), extendable(extendable), delta_t(delta_t) {timestamp = std::time(nullptr); json_object = json::object(); }
  HistBase(std::string name, std::string xlabel, std::string ylabel, unsigned int xbins, bool extendable, float delta_t) : name(name), title(name), xlabel(xlabel), ylabel(ylabel), xbins(xbins), extendable(extendable), delta_t(delta_t) { timestamp = std::time(nullptr); }
  virtual ~HistBase(){}
  std::mutex m_hist_mutex; // prevent histogram access clashes
  //define hist
  std::string name, title, xlabel, ylabel;
  float xmin = -1.;
  float xmax = -1.;
  unsigned int xbins;
  bool extendable;
  bool b_reset = false; // true: reset histogram entries to 0 after publish
  bool b_norm = false;  // true: normalise histogram by total entries at publish
  std::atomic<int>* norm_ptr = nullptr;
  //define published msg
  std::string type = "num_fixedwidth";
  std::ostringstream msg_head;
  //for publishing
  json json_object;
  std::time_t timestamp; 
  unsigned int delta_t;
  //public functions
  virtual std::string publish() { return "nothing";}
  virtual void reset() {}
  void reset_on_publish(bool reset=true){ b_reset=reset;}
  void normalise_on_publish(bool norm=true){ b_norm=norm;}
  void set_normalisation_metric(std::atomic<int>* ptr){ norm_ptr=ptr;}
};


template <typename T>
class Hist : public HistBase {
  public:
  Hist() : HistBase(){}
  Hist(std::string name, std::string xlabel, std::string ylabel, float xmin, float xmax, unsigned int xbins, bool extendable, float delta_t) : HistBase(name, xlabel, ylabel, xmin, xmax, xbins, extendable, delta_t) { 
     configure();
  }
  ~Hist(){}
  template <typename X, typename W>
  void fill(X x, W w = 1)  { 
      std::lock_guard<std::mutex> lock_guard(m_hist_mutex);
      hist_object(x, weight(w));
      }
  std::string publish() {
      std::lock_guard<std::mutex> lock_guard(m_hist_mutex);
      auto this_axis = hist_object.axis();
      std::vector<int> yvalues;
      float weight(1);
      if (b_norm) {
          if (norm_ptr != nullptr) {
              *norm_ptr > 0 ? weight = 1./(*norm_ptr) : 1;
          }
          else {
              unsigned total_entries = std::accumulate(hist_object.begin(), hist_object.end(), 0.0);
              total_entries > 0 ? weight = 1./total_entries : 1;
          }
      }
      for (auto y : indexed(hist_object, coverage::all ) ) {
            yvalues.push_back((*y)*weight);
      }
      json jsonupdate;
      if (extendable) {
        unsigned int xbins_new = hist_object.axis().size();
        jsonupdate["xbins"] = xbins_new;
        jsonupdate["xmin"] = hist_object.axis().bin(0).lower(); 
        jsonupdate["xmax"] = hist_object.axis().bin(xbins_new-1).upper(); 
      }
      jsonupdate["yvalues"] = yvalues;
      json_object.update(jsonupdate);
      if (b_reset) {
       //auto ind = indexed(hist_object, coverage::all);
       std::fill(hist_object.begin(), hist_object.end(), 0); // FIXME: This might break with boost version > 1.70
      }
      return json_object.dump();
  }
  void reset(){
       std::lock_guard<std::mutex> lock_guard(m_hist_mutex);
       std::fill(hist_object.begin(), hist_object.end(), 0); // FIXME: This might break with boost version > 1.70
  }
  private:
  T hist_object;
  void configure() {
    if (extendable){
      hist_object = make_histogram(stretchy_axis_t(xbins, xmin, xmax, xlabel));
      type.append("_ext");
      set_base_info(); 
    }
    else {
      hist_object = make_histogram(axis_t(xbins, xmin, xmax, xlabel));
      set_base_info(); 
    }
  }
  void set_base_info(){
    json_object["name"]=name; 
    json_object["type"]=type; 
    json_object["xlabel"]=xlabel;
    json_object["ylabel"]=ylabel;
    json_object["xbins"]=xbins;
    json_object["xmin"]=xmin;
    json_object["xmax"]=xmax;
  }
};

class CategoryHist : public HistBase { // this hist object is of special type: filled by string values. Hence needs special handling upon creation and publishing.
  public:
  CategoryHist() : HistBase(){}
  CategoryHist(std::string name, std::string xlabel, std::string ylabel, std::vector<std::string> categories, float delta_t) : HistBase(name, xlabel, ylabel, categories.size() , false, delta_t), categories(categories) { 
     configure();
  }
  ~CategoryHist(){}
  template <typename W>
  void fill(std::string x, W w = 1)  {
    std::lock_guard<std::mutex> lock_guard(m_hist_mutex);
    hist_object(x, weight(w));
  }
  template <typename W>
  void fill(const char * x, W w = 1)  { 
    std::lock_guard<std::mutex> lock_guard(m_hist_mutex);
    hist_object(x, weight(w));
  }
  std::string publish() override {
      std::lock_guard<std::mutex> lock_guard(m_hist_mutex);
      auto this_axis = hist_object.axis();
      std::vector<float> yvalues;
      float weight(1);
      if (b_norm) {
          if (norm_ptr != nullptr) {
              *norm_ptr > 0 ? weight = 1./(*norm_ptr) : 1;
          }
          else {
              unsigned total_entries = std::accumulate(hist_object.begin(), hist_object.end(), 0.0);
              total_entries > 0 ? weight = 1./total_entries : 1;
          }
      }
      for (auto y : indexed(hist_object, coverage::inner ) ) { // no under/overflows for boost hist objects of axis type category.
            yvalues.push_back((*y)*weight);
      }
      json jsonupdate;
      jsonupdate["yvalues"] = yvalues;
      json_object.update(jsonupdate);
      if (b_reset) {
       //auto ind = indexed(hist_object, coverage::all);
       std::fill(hist_object.begin(), hist_object.end(), 0); // FIXME: This might break with boost version > 1.70
      }
      return json_object.dump();
  }
  void reset(){
       std::lock_guard<std::mutex> lock_guard(m_hist_mutex);
       std::fill(hist_object.begin(), hist_object.end(), 0); // FIXME: This might break with boost version > 1.70
  }
  private:
  categoryhist_t hist_object;
  std::vector<std::string> categories;
   void configure() {
      categoryaxis_t category_axis = categoryaxis_t(categories); 
      hist_object = make_histogram(category_axis);
      type = "categories";
      set_base_info();
  }
  std::string get_category_string( std::vector<std::string> categories ) { 
      std::string category_string;
      category_string+=",'categories':[";
      for ( auto c: categories ) { category_string+="'"; category_string+=c; category_string+="'"; category_string+=",";}
      category_string+="]";
      return category_string;
  }
  void set_base_info(){
    json_object["name"]=name; 
    json_object["type"]=type; 
    json_object["xlabel"]=xlabel;
    json_object["ylabel"]=ylabel;
    json_object["xbins"]=xbins;
    json_object["categories"]=categories;
  }
};

class Hist2D : public HistBase {
  public:
  Hist2D() : HistBase(){}
  Hist2D(std::string name, std::string xlabel, float xmin, float xmax, unsigned int xbins, std::string ylabel, float ymin, float ymax, unsigned int ybins, float delta_t) : HistBase(name, xlabel, ylabel, xmin, xmax, xbins, false, delta_t), ymin(ymin), ymax(ymax), ybins(ybins) { 
     configure();
  }
  ~Hist2D(){}
  float ymin;
  float ymax;
  float ybins;
  template <typename X, typename Y, typename W>
  void fill( X x, Y y, W w=1) { 
      std::lock_guard<std::mutex> lock_guard(m_hist_mutex);
      hist_object(x,y, weight(w));
      }
  std::string publish() {
      std::lock_guard<std::mutex> lock_guard(m_hist_mutex);
      auto this_axis = hist_object.axis();
      std::vector<float> zvalues;
      float weight(1);
      if (b_norm) {
          if (norm_ptr != nullptr) {
              *norm_ptr > 0 ? weight = 1./(*norm_ptr) : 1;
          }
          else {
              unsigned total_entries = std::accumulate(hist_object.begin(), hist_object.end(), 0.0);
              total_entries > 0 ? weight = 1./total_entries : 1;
          }
      }
      for (auto z : indexed(hist_object, coverage::all ) ) {
            zvalues.push_back((*z)*weight);
      }
      json jsonupdate;
      jsonupdate["zvalues"] = zvalues;
      json_object.update(jsonupdate);
      if (b_reset) {
       //auto ind = indexed(hist_object, coverage::all);
       std::fill(hist_object.begin(), hist_object.end(), 0); // FIXME: This might break with boost version > 1.70
      }
      return json_object.dump();
  }
  void reset(){
       std::lock_guard<std::mutex> lock_guard(m_hist_mutex);
       std::fill(hist_object.begin(), hist_object.end(), 0); // FIXME: This might break with boost version > 1.70
  }
  private:
  hist2d_t hist_object;
  void configure() {
      hist_object = make_histogram(axis_t(xbins, xmin, xmax, xlabel), axis_t(ybins, ymin, ymax, ylabel));
      type="2d_num_fixedwidth";
      set_base_info(); 
  }
  void set_base_info(){
    json_object["name"]=name; 
    json_object["type"]=type;
    json_object["xlabel"]=xlabel;
    json_object["ylabel"]=ylabel;
    json_object["xbins"]=xbins;
    json_object["xmin"]=xmin;
    json_object["xmax"]=xmax;
    json_object["ybins"]=ybins;
    json_object["ymin"]=ymin;
    json_object["ymax"]=ymax;
  }
};

