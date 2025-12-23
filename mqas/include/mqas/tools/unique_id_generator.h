#pragma once
#include <type_traits>
#include <set>

namespace mqas::tools{

template<typename T,bool THREAD_SAFE = false>
requires std::is_unsigned_v<T>
struct unique_id_generator
{
    T next();
    bool remove(T id);
    std::pair<T,T> get_border() const;
private:
    std::set<T> ordered_set;
};

template<typename T>
requires std::is_unsigned_v<T>
struct unique_id_generator<T,true>
{
    T next();
    bool remove(T id);

    std::pair<T, T> get_border();
private:
    std::set<T> ordered_set;
    std::mutex _mutex;
};

}


#include "unique_id_generator.impl.hpp"