#ifndef DAQ_UTILITIES_SINGLETON_HPP_
#define DAQ_UTILITIES_SINGLETON_HPP_

/*
 * Types
 * Author: Roland.Sipos@cern.ch
 * Description: 
 *   Singleton template from CodeReviews
 *   https://codereview.stackexchange.com/questions/173929/modern-c-singleton-template
 * Date: March 2019
*/


namespace daq{
namespace utilities{

template<typename T>
class Singleton {
public:
    static T& instance();

    // Prevent copying and moving.
    Singleton(Singleton const&) = delete;             // Copy construct
    Singleton(Singleton&&) = delete;                  // Move construct
    Singleton& operator=(Singleton const&) = delete;  // Copy assign
    Singleton& operator=(Singleton &&) = delete;      // Move assign


protected:
    Singleton() {}
};

#include <memory>
template<typename T>
T& Singleton<T>::instance()
{
    static const std::unique_ptr<T> instance{ new T{} };
    return *instance;
}

}
}

#endif

