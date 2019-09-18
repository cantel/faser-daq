#ifndef HISTOGRAM_HPP
#define HISTOGRAM_HPP

#include <boost/histogram.hpp>
#include <boost/format.hpp>
#include <boost/histogram/ostream.hpp> // write histogram straight to ostream
#include <ctime>
#include <iostream>

  using namespace boost::histogram;

  using axis_t = axis::regular<>;
  using hist_t = decltype(make_histogram(std::declval<axis_t>())); // most hists are of this type
  using categoryaxis_t = axis::category<std::string>;
  using categoryhist_t = decltype(make_histogram(std::declval<categoryaxis_t>())); 
  using graph_t = std::vector<std::pair<unsigned int, unsigned int>>; // assume for now int vs int is the most common type
  
  class HistBase {
    public:
    HistBase(){}
    HistBase(std::string name, std::string xlabel, std::string ylabel, float start_range, float end_range, unsigned int number_bins, float delta_t) : name(name), title(name), xlabel(xlabel), ylabel(ylabel), start_range(start_range), end_range(end_range), number_bins(number_bins), delta_t(delta_t) {timestamp = std::time(nullptr); }
    HistBase(std::string name, std::string xlabel, std::string ylabel, unsigned int number_bins, float delta_t) : name(name), title(name), xlabel(xlabel), ylabel(ylabel), number_bins(number_bins), delta_t(delta_t) { timestamp = std::time(nullptr); }
    ~HistBase(){}
    std::string name;
    std::string title;
    std::string xlabel;
    std::string ylabel;
    float start_range = -1.;
    float end_range = -1.;
    unsigned int number_bins;
    std::time_t timestamp; 
    float delta_t;
    virtual std::string publish() { return "nothing";}
    virtual void fill(void* x) { }
  };

  
  template <typename T, typename U>
  class Hist : public HistBase {
    public:
    Hist() : HistBase(){}
    Hist(std::string name, std::string xlabel, std::string ylabel, float start_range, float end_range, unsigned int number_bins, float delta_t) : HistBase(name, xlabel, ylabel, start_range, end_range, number_bins, delta_t) { 
        hist_object = make_histogram(axis_t(number_bins, start_range, end_range, name));
	get_msg_head(msg_head);
	msg_head<<std::endl;
    }
    Hist(std::string name, std::string xlabel, float start_range, float end_range, unsigned int number_bins, float delta_t) : HistBase(name, xlabel, "counts", start_range, end_range, number_bins, delta_t) {
        hist_object = make_histogram(axis_t(number_bins, start_range, end_range, name));
	get_msg_head(msg_head);
	msg_head<<std::endl;
    }
    Hist(std::string name, std::string xlabel, std::string ylabel, std::vector<std::string> categories, float delta_t) : HistBase(name, xlabel, ylabel, categories.size() , delta_t) { 
        categoryaxis_t category_axis = categoryaxis_t(categories);
        hist_object = make_histogram(category_axis);
	get_msg_head(msg_head);
	msg_head<<get_category_string(categories)<<std::endl;
    }
    Hist(std::string name, std::string xlabel, std::vector<std::string> categories, float delta_t) : HistBase(name, xlabel, "counts", categories.size() , delta_t) { 
        categoryaxis_t category_axis = categoryaxis_t(categories); 
        hist_object = make_histogram(category_axis);
        type = "category_string"; 
        hist_coverage_all = false; 
	get_msg_head(msg_head);
	msg_head<<get_category_string(categories)<<std::endl;
    }
    ~Hist(){}
    T hist_object;
    std::string type = "regular_uint";
    bool hist_coverage_all = true;
    std::ostringstream msg_head;
    std::ostringstream msg_tail;
    std::string publish() {
        std::ostringstream os;
        os<<msg_head.str();
        auto this_axis = hist_object.axis();
        auto hist_coverage = coverage::all;
        if ( hist_coverage_all == false ) hist_coverage = coverage::inner;
        for (auto x : indexed(hist_object, hist_coverage ) ) {
              os << boost::format("bin %2i: %i\n") % x.index() % *x;
        }
        os<<std::endl<<"incl under/overflow:"<<hist_coverage_all<<std::endl;
        return os.str();
    }
    void fill(void* x) override { 
        U * val = static_cast<U*>(x);
        hist_object(*val); } 
    std::string get_category_string( std::vector<std::string> categories ) { 
        std::string category_string;
	category_string+=",categories:";
        for ( auto c: categories ) { category_string+=c; category_string+=","; std::cout<<category_string<<std::endl;}
        return category_string;
    }
    void get_msg_head( std::ostringstream& msg_head){
        msg_head<<"type:"<<type<<std::endl<<"name:"<<name<<",xlabel:"<<xlabel<<",ylabel:"<<ylabel<<",number_bins:"<<number_bins<<",start:"<<start_range<<",end:"<<end_range;
    }
  };
  
#endif // HISTOGRAM_HPP
