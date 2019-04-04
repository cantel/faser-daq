#ifndef DAQ_UTILITIES_DATASTORE_h
#define DAQ_UTILITIES_DATASTORE_h

/*
 * DataStore
 * Author: Roland.Sipos@cern.ch
 * Description: 
 *   Generic data source with casters
 * Date: March 2019
*/

namespace daq {

  namespace utilities {

    class DataStoreBase {
    public:
      virtual ~DataStoreBase() {}
      template<class T> std::shared_ptr<T>& getAs();
      template<class T, class U> void setAs(const std::shared_ptr<U>& rhs);
    };

    template <typename T>
    class DataStore : public DataStoreBase {
    public:
      DataStore(const std::shared_ptr<T>& ds) : m_ds(ds) { /*std::cout << " woof: DataStore<T>(const T& ds): new instance of DataStore!" << std::endl; */ }
      std::shared_ptr<T>& getAs() { return m_ds; }
      void setAs(const std::shared_ptr<T>& rhs) { /*std::cout << " woof: setAs()!" << std::endl;*/ m_ds=rhs; }
    private:
      std::shared_ptr<T> m_ds;
    };

    //Here's the trick: dynamic_cast rather than virtual
    template<class T> std::shared_ptr<T>& DataStoreBase::getAs() { 
      return dynamic_cast<DataStore<T>&>(*this).getAs();
    }

    template<class T, class U> void DataStoreBase::setAs(const std::shared_ptr<U>& rhs) { 
      return dynamic_cast<DataStore<T>&>(*this).setAs(rhs); 
    }

  }

}

#endif

