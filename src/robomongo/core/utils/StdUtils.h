#pragma once
#include <algorithm>

namespace Robomongo
{
    namespace StdUtils
    {
        template<typename T>
        struct RemoveIfFound: public std::unary_function<const T, bool>
        {
            RemoveIfFound(const T whatSearch) : _whatSearch(whatSearch) {}

            bool operator()(const T item) const {
                if (item == _whatSearch){
                    delete item;
                    return true;
                }
                return false;
            }

            const T _whatSearch;
        };
    }
}
