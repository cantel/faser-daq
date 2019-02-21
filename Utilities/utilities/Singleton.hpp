#ifndef DAQ_UTILITIES_SINGLETON_HPP_
#define DAQ_UTILITIES_SINGLETON_HPP_

namespace daq{
namespace utilities{

template<typename T>
class Singleton {
public:
    static T& instance();

    Singleton(const Singleton&) = delete;
    Singleton& operator= (const Singleton) = delete;

protected:
    struct m_token {};
    Singleton() {}
};

#include <memory>
template<typename T>
T& Singleton<T>::instance()
{
    static const std::unique_ptr<T> instance{new T{m_token{}}};
    return *instance;
}

}
}

#endif

