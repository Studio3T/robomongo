#pragma once

#include <algorithm>
#include <functional>

namespace Robomongo
{
    namespace stdutils
    {
        template<typename T>
        inline void destroy(T *&v)
        {
            delete v;
            v = NULL;
        }

        template<typename T>
        struct RemoveIfFound
        {
            RemoveIfFound(T whatSearch) : _whatSearch(whatSearch) {}

            bool operator()(T item) const {
                if (item == _whatSearch) {
                    destroy(item);
                    return true;
                }
                return false;
            }

            T _whatSearch;
        };

        template <typename T>
        struct default_delete
        {
            inline void operator ()(T *ptr) const
            {
                  destroy(ptr);
            }
        };

        template <typename T>
        struct default_delete<T*>
        {
            inline void operator ()(T *ptr) const
            {
                  destroy(ptr);
            }
        };

        template<typename T, unsigned int N>
        struct default_delete<T[N]>
        {
            inline void operator ()(const T ptr) const
            {
                delete [] ptr;
            }
        };
    }
}
