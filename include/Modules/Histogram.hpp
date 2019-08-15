#ifndef HISTOGRAM_HPP
#define HISTOGRAM_HPP

#include <boost/histogram.hpp>

 using axis_t = boost::histogram::axis::regular<>;
 using hist_t = decltype(boost::histogram::make_histogram(std::declval<axis_t>())); // most hists are of this type
 using categoryaxis_t = boost::histogram::axis::category<std::string>;
 using categoryhist_t = decltype(boost::histogram::make_histogram(std::declval<categoryaxis_t>())); 
 using graph_t = std::vector<std::pair<unsigned int, unsigned int>>; // assume for now int vs int is the most common type
 
  class HistBase {
    friend class HistogramManger;
    public:
    HistBase() : ylabel("counts") {}
    HistBase(std::string name, std::string xlabel) : name(name), title(name), xlabel(xlabel), ylabel("counts") {}
    HistBase(std::string name, std::string xlabel, std::string ylabel) : name(name), title(name), xlabel(xlabel), ylabel(ylabel) {}
    ~HistBase(){}
    hist_t object;
    hist_t getObject() { return object; };
    //protected:
    std::string name;
    std::string title;
    std::string xlabel;
    std::string ylabel;
  };

  /*
  template <typename T>
  class Hist : public HistBase {
    friend class HistogramManger;
    public:
    Hist() : HistBase(){}
    Hist(std::string name, std::string xlabel) : HistBase(name, xlabel){}
    Hist(std::string name, std::string xlabel, std::string ylabel) : HistBase(name, xlabel, ylabel){}
    ~Hist(){}
    protected:
    T hist_object;
    void fill(float x) { hist_object(x) };
    void fill(double x) { hist_object(x) };
    void fill(int x) { hist_object(x) };
    void fill(std::string x) { hist_object(x) };
  };*/
  
  class RegularHist : public HistBase {
    public:
    RegularHist() : HistBase() {};
    RegularHist(std::string name, std::string xlabel) : HistBase(name, xlabel){}
    ~RegularHist(){}
    hist_t object;
    hist_t getObject() { return object; };
  };
 
  class CategoryHist : public HistBase {
    public:
    CategoryHist() : HistBase() {};
    CategoryHist(std::string name, std::string xlabel) : HistBase(name, xlabel){}
    ~CategoryHist(){}
    categoryhist_t object;
    categoryhist_t getObject() { return object; };
  };
 
  class Graph : public HistBase {
    public:
    Graph(){};
    Graph(std::string name, std::string xlabel, std::string ylabel) : HistBase(name, xlabel, ylabel){}
    ~Graph(){}
    graph_t object;
    graph_t getObject() { return object; };
  };


#endif // HISTOGRAM_HPP
