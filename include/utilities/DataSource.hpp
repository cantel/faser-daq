#ifndef DAQ_UTILITIES_DATASOURCE_h
#define DAQ_UTILITIES_DATASOURCE_h

/*
 * DataSource
 * Author: Roland.Sipos@cern.ch
 * Description: 
 *   Generic data source with casters
 * Date: March 2019
*/

namespace daq {

  namespace utilities {

    class DataSourceBase {
    public:
      virtual ~DataSourceBase() {}
      template<class T> std::shared_ptr<T>& getAs();
      template<class T, class U> void setAs(const std::shared_ptr<U>& rhs);
    };

    template <typename T>
    class DataSource : public DataSourceBase {
    public:
      DataSource(const std::shared_ptr<T>& ds) : m_ds(ds) { /*std::cout << " woof: DataSource<T>(const T& ds): new instance of DataSource!" << std::endl; */ }
      std::shared_ptr<T>& getAs() { return m_ds; }
      void setAs(const std::shared_ptr<T>& rhs) { /*std::cout << " woof: setAs()!" << std::endl;*/ m_ds=rhs; }
    private:
      std::shared_ptr<T> m_ds;
    };

    //Here's the trick: dynamic_cast rather than virtual
    template<class T> std::shared_ptr<T>& DataSourceBase::getAs() { 
      return dynamic_cast<DataSource<T>&>(*this).getAs();
    }

    template<class T, class U> void DataSourceBase::setAs(const std::shared_ptr<U>& rhs) { 
      return dynamic_cast<DataSource<T>&>(*this).setAs(rhs); 
    }

  }

}

#endif

