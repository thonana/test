#pragma once

#include "ds/h5/node.h"
#include <string>
#include <vector>

namespace ds::h5 {

template<typename T>
class Field : public Node
{
public:
    Field();
    ~Field();

    T operator[](size_t pos);

    std::vector<T> FetchAll();
    std::vector<T> Fetch(size_t first, size_t last);

    Field<T>& operator<<(const T& value)
    {
        Append(value);
        return *this;
    }

    void Append(const T& value);
};

} // namespace ds::h5