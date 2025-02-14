#pragma once
#include <type_traits>
#include <set>

namespace mqas::tools{

template<typename T>
requires std::is_unsigned_v<T>
struct unique_id_generator
{
    T next();
    bool remove(T id);
    std::pair<T,T> get_border() const;
private:
    std::set<T> ordered_set;
};

}


#include "unique_id_generator.impl.hpp"