#pragma once

#include <H5Cpp.h>
#include <string>

namespace ds::h5 {

struct Node
{
    bool Exists(const std::string& loc);

    template<typename T>
    void SetAttribute(const std::string& key, T&& value)
    {
        auto h5 = GetH5Object();
        if (h5->attrExists(key)) {
            hsize_t size = 0;
            auto    attr = h5.openAttribute(metadata_name);
            attr.getSpace().getSimpleExtentDims(&size, nullptr);
            mpack.resize(size);
            attr.read(H5::PredType::NATIVE_B8, mpack.data());
        }
    }

    template<typename T>
    T GetAttribute(const std::string& key)
    {
        auto h5 = GetH5Object();
        if (h5->attrExists(key)) {
            hsize_t size = 0;
            auto    attr = h5.openAttribute(metadata_name);
            attr.getSpace().getSimpleExtentDims(&size, nullptr);
            mpack.resize(size);
            attr.read(H5::PredType::NATIVE_B8, mpack.data());
        }
    }


protected:
    virtual H5::H5Object* GetH5Object() = 0;
};

} // namespace ds::h5