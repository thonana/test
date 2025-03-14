#pragma once

#include "ds/array.h"
#include "ds/objectpool.h"
#include <H5Cpp.h>
#include <string>

namespace ds {
namespace detail {

template<typename T>
class ArrayStoreOp
{
public:
    Array<T> Read(const H5::DataSet&   dset,
                  const H5::DataType&  type,
                  const H5::DataSpace& mspace,
                  const H5::DataSpace& fspace,
                  hsize_t              size)
    {
        Array<T> array(size);
        dset.read(array.data(), type, mspace, fspace);
        return array;
    }

    void Write(const H5::DataSet&   dset,
               const H5::DataType&  type,
               const H5::DataSpace& mspace,
               const H5::DataSpace& fspace,
               const Array<T>&      array)
    {
        dset.write(array.data(), type, mspace, fspace);
    }
};

// array variable for length
template<typename T>
class ArrayStoreOp<Array<T>>
{
    using value_type = Array<T>;
    using self_type  = ArrayStoreOp<value_type>;
    Array<hvl_t>       m_vlen;
    Array<value_type>* m_array;

public:
    Array<value_type> Read(const H5::DataSet&   dset,
                           const H5::DataType&  type,
                           const H5::DataSpace& mspace,
                           const H5::DataSpace& fspace,
                           hsize_t              size)
    {
        Array<value_type> array;
        array.reserve(size);
        m_vlen.reserve(size);
        m_array = &array;

        auto xfer = H5::DSetMemXferPropList();
        xfer.setVlenMemManager(
          [](size_t size, void* info) -> void* {
              auto  self  = static_cast<self_type*>(info);
              auto& value = self->m_array->emplace_back(size / sizeof(T));
              return value.data();
          },
          this,
          [](void*, void*) -> void {},
          nullptr);

        dset.read(m_vlen.data(), type, mspace, fspace, xfer);
        return array;
    }

    void Write(const H5::DataSet&       dset,
               const H5::DataType&      type,
               const H5::DataSpace&     mspace,
               const H5::DataSpace&     fspace,
               const Array<value_type>& array)
    {
        m_vlen.reserve(array.size());
        m_vlen.clear();
        for (auto& value : array) {
            m_vlen.emplace_back(value.size(), const_cast<T*>(value.data()));
        }

        dset.write(m_vlen.data(), type, mspace, fspace);
    }
};

// arrayptr for variable length
template<typename T>
class ArrayStoreOp<ArrayPtr<T>>
{
    using value_type = ArrayPtr<T>;
    using self_type  = ArrayStoreOp<value_type>;
    Array<hvl_t>         m_vlen;
    Array<value_type>*   m_array;
    ObjectPool<Array<T>> m_pool;

public:
    Array<value_type> Read(const H5::DataSet&   dset,
                           const H5::DataType&  type,
                           const H5::DataSpace& mspace,
                           const H5::DataSpace& fspace,
                           hsize_t              size)
    {
        Array<value_type> array;
        array.reserve(size);
        m_vlen.reserve(size);
        m_array = &array;

        auto xfer = H5::DSetMemXferPropList();
        xfer.setVlenMemManager(
          [](size_t size, void* info) -> void* {
              auto  self  = static_cast<self_type*>(info);
              auto& value = self->m_array->emplace_back(self->m_pool.Acquire());
              value->resize(size / sizeof(T));
              return value->data();
          },
          this,
          [](void*, void*) -> void {},
          nullptr);

        dset.read(m_vlen.data(), type, mspace, fspace, xfer);
        return array;
    }

    void Write(const H5::DataSet&       dset,
               const H5::DataType&      type,
               const H5::DataSpace&     mspace,
               const H5::DataSpace&     fspace,
               const Array<value_type>& array)
    {
        m_vlen.reserve(array.size());
        m_vlen.clear();
        for (auto& value : array) {
            m_vlen.emplace_back(value->size(), const_cast<T*>(value->data()));
        }

        dset.write(m_vlen.data(), type, mspace, fspace);
    }
};

// string
template<>
class ArrayStoreOp<std::string>
{
    Array<const char*> m_vlen;

public:
    Array<std::string> Read(const H5::DataSet&   dset,
                            const H5::DataType&  type,
                            const H5::DataSpace& mspace,
                            const H5::DataSpace& fspace,
                            hsize_t              size)
    {
        Array<std::string> array;
        array.reserve(size);
        m_vlen.reserve(size);

        auto xfer = H5::DSetMemXferPropList();
        xfer.setVlenMemManager(
          [](size_t size, void* info) -> void* {
              auto  array = static_cast<Array<std::string>*>(info);
              auto& s     = array->emplace_back();
              s.reserve(size);
              s.resize(size - 1);
              return s.data();
          },
          &array,
          [](void*, void*) -> void {},
          nullptr);

        dset.read(m_vlen.data(), type, mspace, fspace, xfer);
        return array;
    }

    void Write(const H5::DataSet&        dset,
               const H5::DataType&       type,
               const H5::DataSpace&      mspace,
               const H5::DataSpace&      fspace,
               const Array<std::string>& array)
    {
        m_vlen.reserve(array.size());
        m_vlen.clear();
        for (auto& s : array) {
            m_vlen.emplace_back(s.data());
        }

        dset.write(m_vlen.data(), type, mspace, fspace);
    }
};

} // namespace detail
} // namespace ds
