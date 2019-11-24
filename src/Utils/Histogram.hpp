#pragma once

#include <boost/histogram.hpp>
#include <boost/format.hpp>
#include <boost/histogram/ostream.hpp> // write histogram straight to ostream
#include <ctime>
#include <iostream>
#include <nlohmann/json.hpp> // dump Hist as json structure

using namespace boost::histogram;
using json = nlohmann::json;

//currently supporting:
//    - real number histograms with fixed widths.
//    - category histograms with labelled bins (labels are "std::string"s)
//may consider more types in future: https://www.boost.org/doc/libs/1_71_0/libs/histogram/doc/html/histogram/overview.html#histogram.overview.rationale.structure.axis

using axis_t = axis::regular<>; 
using hist_t = decltype(make_histogram(std::declval<axis_t>()));
using hist2d_t = decltype(make_histogram(std::declval<axis_t>(), std::declval<axis_t>()));
using categoryaxis_t = axis::category<std::string>;
using categoryhist_t = decltype(make_histogram(std::declval<categoryaxis_t>())); 

class HistBase {
  public:
  HistBase(){}
  HistBase(std::string name, std::string xlabel, std::string ylabel, float xmin, float xmax, unsigned int xbins, float delta_t) : name(name), title(name), xlabel(xlabel), ylabel(ylabel), xmin(xmin), xmax(xmax), xbins(xbins), delta_t(delta_t) {timestamp = std::time(nullptr); json_object = json::object(); }
  HistBase(std::string name, std::string xlabel, std::string ylabel, unsigned int xbins, float delta_t) : name(name), title(name), xlabel(xlabel), ylabel(ylabel), xbins(xbins), delta_t(delta_t) { timestamp = std::time(nullptr); }
  virtual ~HistBase(){}
  //define hist
  std::string name;
  std::string title;
  std::string xlabel;
  std::string ylabel;
  float xmin = -1.;
  float xmax = -1.;
  unsigned int xbins;
  //define published msg
  std::string type = "num_fixedwidth";
  std::ostringstream msg_head;
  //for publishing
  json json_object;
  std::time_t timestamp; 
  float delta_t;
  //public functions
  virtual std::string publish() { return "nothing";}
};


template <typename T>
class Hist : public HistBase {
  public:
  Hist() : HistBase(){}
  Hist(std::string name, std::string xlabel, std::string ylabel, float xmin, float xmax, unsigned int xbins, float delta_t) : HistBase(name, xlabel, ylabel, xmin, xmax, xbins, delta_t) { 
     configure();
  }
  Hist(std::string name, std::string xlabel, float xmin, float xmax, unsigned int xbins, float delta_t) : HistBase(name, xlabel, "counts", xmin, xmax, xbins, delta_t) {
     configure();
  }
  ~Hist(){}
  template <typename X>
  void fill(X x)   { hist_object(x); }
  std::string publish() {
      std::ostringstream os;
      auto this_axis = hist_object.axis();
      std::vector<int> yvalues;
      for (auto y : indexed(hist_object, coverage::all ) ) {
            yvalues.push_back(*y);
      }
      json jsonupdate;
      jsonupdate["yvalues"] = yvalues;
      json_object.update(jsonupdate);
      std::string json_str = json_object.dump();
      os << json_str;
      return os.str();
  }
  private:
  T hist_object;
  void configure() {
      hist_object = make_histogram(axis_t(xbins, xmin, xmax, xlabel));
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
  }
};

template <>
class Hist<categoryhist_t> : public HistBase { // this hist object is of special type: filled by string values. Hence needs special handling upon creation and publishing.
  public:
  Hist() : HistBase(){}
  Hist(std::string name, std::string xlabel, std::string ylabel, std::vector<std::string> categories, float delta_t) : HistBase(name, xlabel, ylabel, categories.size() , delta_t), categories(categories) { 
     configure();
  }
  Hist(std::string name, std::string xlabel, std::vector<std::string> categories, float delta_t) : HistBase(name, xlabel, "counts", categories.size() , delta_t), categories(categories) { 
     configure();
  }
  ~Hist(){}
  void fill(std::string x)  { hist_object(x);}
  void fill(const char * x)  { hist_object(x);}
  std::string publish() override {
      std::ostringstream os;
      auto this_axis = hist_object.axis();
      std::vector<int> yvalues;
      for (auto y : indexed(hist_object, coverage::inner ) ) { // no under/overflows for boost hist objects of axis type category.
            yvalues.push_back(*y);
      }
      json jsonupdate;
      jsonupdate["yvalues"] = yvalues;
      json_object.update(jsonupdate);
      std::string json_str = json_object.dump();
      os << json_str;
      return os.str();
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
  Hist2D(std::string name, std::string xlabel, float xmin, float xmax, unsigned int xbins, std::string ylabel, float ymin, float ymax, unsigned int ybins, float delta_t) : HistBase(name, xlabel, ylabel, xmin, xmax, xbins, delta_t), ymin(ymin), ymax(ymax), ybins(ybins) { 
     configure();
  }
  ~Hist2D(){}
  float ymin;
  float ymax;
  float ybins;
  template <typename X, typename Y>
  void fill( X x, Y y) { hist_object(x,y);}
  std::string publish() {
      std::ostringstream os;
      auto this_axis = hist_object.axis();
      std::vector<int> zvalues;
      for (auto z : indexed(hist_object, coverage::all ) ) {
            zvalues.push_back(*z);
      }
      json jsonupdate;
      jsonupdate["zvalues"] = zvalues;
      json_object.update(jsonupdate);
      std::string json_str = json_object.dump();
      os << json_str;
      return os.str();
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

