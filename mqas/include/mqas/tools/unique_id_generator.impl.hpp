#include <numeric>

namespace mqas::tools{

template<typename T>
requires std::is_unsigned_v<T>
T unique_id_generator<T>::next()
{
    auto [min,max] = get_border();
    if(min > 1)
    {
        ordered_set.insert(min - 1);
        return min - 1;
    }else if(max < std::numeric_limits<T>::max()){
        ordered_set.insert(max + 1);
        return max + 1;
    }
    assert(false);
    return 0;
}

template<typename T>
requires std::is_unsigned_v<T>
bool unique_id_generator<T>::remove(T id)
{
    if(ordered_set.contains(id))
    {
        ordered_set.erase(id);
        return true;
    }
    return false;
}

template<typename T>
requires std::is_unsigned_v<T>
std::pair<T,T> unique_id_generator<T>::get_border() const
{
    std::pair<T,T> res;
    res.first = ordered_set.empty() ? 0 : *(ordered_set.begin());
    res.second = ordered_set.empty() ? 0 : *(--ordered_set.end());
    return res;
}

}