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
using categoryaxis_t = axis::category<std::string>;
using categoryhist_t = decltype(make_histogram(std::declval<categoryaxis_t>())); 

class HistBase {
  public:
  HistBase(){}
  HistBase(std::string name, std::string xlabel, std::string ylabel, float start_range, float end_range, unsigned int number_bins, float delta_t) : name(name), title(name), xlabel(xlabel), ylabel(ylabel), start_range(start_range), end_range(end_range), number_bins(number_bins), delta_t(delta_t) {timestamp = std::time(nullptr); json_object = json::object(); }
  HistBase(std::string name, std::string xlabel, std::string ylabel, unsigned int number_bins, float delta_t) : name(name), title(name), xlabel(xlabel), ylabel(ylabel), number_bins(number_bins), delta_t(delta_t) { timestamp = std::time(nullptr); }
  ~HistBase(){}
  //define hist
  std::string name;
  std::string title;
  std::string xlabel;
  std::string ylabel;
  float start_range = -1.;
  float end_range = -1.;
  unsigned int number_bins;
  //define published msg
  std::string type = "numerical_fixedwidth";
  std::ostringstream msg_head;
  //for publishing
  json json_object;
  std::time_t timestamp; 
  float delta_t;
  //public functions
   virtual void fill(unsigned int ){ std::cout<<"Value not filled. Is this the right data type for this histogram?"<<std::endl;}
   virtual void fill(int ){ std::cout<<"Value not filled. Is this the right data type for this histogram?"<<std::endl;}
   virtual void fill(float ){ std::cout<<"Value not filled. Is this the right data type for this histogram?"<<std::endl;}
   virtual void fill(double ){ std::cout<<"Value not filled. Is this the right data type for this histogram?"<<std::endl;}
   virtual void fill(std::string ){ std::cout<<"Value not filled. Is this the right data type for this histogram?"<<std::endl;}
   virtual void fill(const char * ){ std::cout<<"Value not filled. Is this the right data type for this histogram?"<<std::endl;}
  virtual std::string publish() { return "nothing";}
};


template <typename T>
class Hist : public HistBase {
  public:
  Hist() : HistBase(){}
  Hist(std::string name, std::string xlabel, std::string ylabel, float start_range, float end_range, unsigned int number_bins, float delta_t) : HistBase(name, xlabel, ylabel, start_range, end_range, number_bins, delta_t) { 
     configure();
  }
  Hist(std::string name, std::string xlabel, float start_range, float end_range, unsigned int number_bins, float delta_t) : HistBase(name, xlabel, "counts", start_range, end_range, number_bins, delta_t) {
     configure();
  }
  ~Hist(){}
  void fill(unsigned int x) override { hist_object(x); }
  void fill(int x) override { hist_object(x);}
  void fill(float x) override { hist_object(x);}
  void fill(double x) override { hist_object(x);}
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
      hist_object = make_histogram(axis_t(number_bins, start_range, end_range, name));
      write_to_json(); 
  }
  void write_to_json(){
    json_object["name"]=name; 
    json_object["type"]=type; 
    json_object["xlabel"]=xlabel;
    json_object["ylabel"]=ylabel;
    json_object["number_bins"]=number_bins;
    json_object["start_range"]=start_range;
    json_object["end_range"]=end_range;
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
  void fill(std::string x) override { hist_object(x);}
  void fill(const char * x) override { hist_object(x);}
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
      write_to_json();
  }
  std::string get_category_string( std::vector<std::string> categories ) { 
      std::string category_string;
      category_string+=",'categories':[";
      for ( auto c: categories ) { category_string+="'"; category_string+=c; category_string+="'"; category_string+=",";}
      category_string+="]";
      return category_string;
  }
  void write_to_json(){
    json_object["name"]=name; 
    json_object["type"]=type; 
    json_object["xlabel"]=xlabel;
    json_object["ylabel"]=ylabel;
    json_object["number_bins"]=number_bins;
    json_object["categories"]=categories;
  }
};
