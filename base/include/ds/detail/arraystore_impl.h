#pragma once

#include "ds/detail/arraystore_op.h"
#include "ds/detail/arraystore_type.h"
#include "ds/errc.h"

#include <H5Cpp.h>
#include <atomic>
#include <filesystem>
#include <map>
#include <memory>

namespace ds {
namespace detail {

enum class ArrayStoreMode
{
    exists,
    append,
    overwrite
};

struct ArrayStoreFile
{
    static std::map<std::filesystem::path, std::weak_ptr<ArrayStoreFile>>
                                ms_opened;
    const std::filesystem::path path;
    const H5::H5File            h5;

    ArrayStoreFile(const std::filesystem::path& path, H5::H5File&& h5)
      : path(path)
      , h5(std::move(h5))
    {
    }

    ~ArrayStoreFile()
    {
        auto it = ms_opened.find(path);
        if (it != ms_opened.end()) {
            ms_opened.erase(it);
        }
    }
};

template<typename T>
class ArrayStoreImpl
{
    static constexpr const char* metadata_name = "metadata";

    const std::shared_ptr<ArrayStoreFile> m_file;
    const std::string                     m_loc;
    const H5::DataSet                     m_dset;
    std::atomic<hsize_t>                  m_size;

    ArrayStoreType<T> m_type;
    ArrayStoreOp<T>   m_op;

    static std::shared_ptr<ArrayStoreFile> OpenFile(
      const std::filesystem::path& path)
    {
        auto& opened = ArrayStoreFile::ms_opened;
        auto  it     = opened.find(path);
        if (it != opened.end()) { // already ms_opened
            if (auto file = it->second.lock()) {
                return file;
            } else {
                opened.erase(it); // remove dangling pointer
            }
        }

        auto file = std::shared_ptr<ArrayStoreFile>(nullptr);
        if (not std::filesystem::exists(path)) {
            file = std::make_shared<ArrayStoreFile>( // Fail if file already exists
              path,
              H5::H5File(path.string(), H5F_ACC_EXCL)); 

        } else {
            file = std::make_shared<ArrayStoreFile>( // Open for read and write
              path,
              H5::H5File(path.string(), H5F_ACC_RDWR));
        }

        opened.insert({ path, file });
        return file;
    }

public:
    static std::shared_ptr<ArrayStoreImpl<T>> Open(
      const std::filesystem::path& path,
      const std::string&           loc,
      ArrayStoreMode               mode,
      size_t                       chunk_size)
    {
        auto file = OpenFile(path);

        auto store = std::shared_ptr<ArrayStoreImpl<T>>();
        if (DoExists(file->h5, loc)) {
            if (mode == ArrayStoreMode::overwrite) {
                file->h5.unlink(loc);

                hsize_t size       = 0;
                hsize_t maxdims[1] = { H5S_UNLIMITED }; //Value for 'unlimited' dimensions
                auto    type       = ArrayStoreType<T>();
                auto    space      = H5::DataSpace(1, &size, maxdims);
                auto    dcpl       = H5::DSetCreatPropList();
                auto    lcpl       = H5::LinkCreatPropList();

                dcpl.setChunk(1, &chunk_size);
                lcpl.setCharEncoding(H5T_CSET_UTF8);
                lcpl.setCreateIntermediateGroup(true);

                auto dset = file->h5.createDataSet(loc,
                                                   type.obj,
                                                   space,
                                                   dcpl,
                                                   H5::DSetAccPropList::DEFAULT,
                                                   lcpl);
                store     = std::make_shared<ArrayStoreImpl<T>>(
                  file, loc, std::move(dset), size);

            } else {
                hsize_t size  = 0;
                auto    dset  = file->h5.openDataSet(loc);
                auto    space = dset.getSpace();
                space.getSimpleExtentDims(&size, nullptr);

                store = std::make_shared<ArrayStoreImpl<T>>(
                  file, loc, std::move(dset), size);
            }
        } else if (mode == ArrayStoreMode::append ||
                   mode == ArrayStoreMode::overwrite) {
            hsize_t size       = 0;
            hsize_t maxdims[1] = { H5S_UNLIMITED };
            auto    type       = ArrayStoreType<T>();
            auto    space      = H5::DataSpace(1, &size, maxdims);
            auto    dcpl       = H5::DSetCreatPropList();
            auto    lcpl       = H5::LinkCreatPropList();

            dcpl.setChunk(1, &chunk_size);
            lcpl.setCharEncoding(H5T_CSET_UTF8);
            lcpl.setCreateIntermediateGroup(true);

            auto dset = file->h5.createDataSet(
              loc, type.obj, space, dcpl, H5::DSetAccPropList::DEFAULT, lcpl);
            store = std::make_shared<ArrayStoreImpl<T>>(
              file, loc, std::move(dset), size);

        } else {
            throw std::system_error(Errc::arraystore_open_fail);
        }

        return store;
    }

    ArrayStoreImpl(const std::shared_ptr<ArrayStoreFile>& file,
                   const std::string&                     loc,
                   H5::DataSet&&                          dset,
                   size_t                                 size)
      : m_file(file)
      , m_loc(loc)
      , m_dset(std::move(dset))
      , m_size(size)
      , m_type()
      , m_op()
    {
    }

    ~ArrayStoreImpl() = default;

    const std::filesystem::path& GetPath() const { return m_file->path; }

    bool Exists(const std::string& loc) const { return DoExists(m_dset, loc); }

    std::vector<uint8_t> ReadMetadata() const { return DoReadMetadata(m_dset); }

    std::vector<uint8_t> ReadMetadata(const std::string& loc) const
    {
        if (Exists(loc)) {
            switch (m_file->h5.childObjType(loc)) {
                case H5O_TYPE_GROUP: {
                    auto obj = m_file->h5.openGroup(loc);
                    return DoReadMetadata(obj);
                }
                case H5O_TYPE_DATASET: {
                    auto obj = m_file->h5.openDataSet(loc);
                    return DoReadMetadata(obj);
                }
            }
        }
        return {};
    }

    void DumpMetadata(const std::vector<uint8_t>& mpack) const
    {
        DoDumpMetadata(m_dset, mpack);
    }

    void DumpMetadata(const std::string&          loc,
                      const std::vector<uint8_t>& mpack) const
    {
        if (Exists(loc)) {
            switch (m_file->h5.childObjType(loc)) {
                case H5O_TYPE_GROUP: {
                    auto obj = m_file->h5.openGroup(loc);
                    DoDumpMetadata(obj, mpack);
                    break;
                }
                case H5O_TYPE_DATASET: {
                    auto obj = m_file->h5.openDataSet(loc);
                    DoDumpMetadata(obj, mpack);
                    break;
                }
            }
        }
    }

    Array<T> Read()
    {
        return m_op.Read(
          m_dset, m_type.obj, H5::DataSpace::ALL, H5::DataSpace::ALL, m_size);
    }

    Array<T> Read(size_t first, size_t last)
    {
        hsize_t size   = last - first;
        auto    mspace = H5::DataSpace(1, &size, nullptr);
        auto    fspace = m_dset.getSpace();
        fspace.selectHyperslab(H5S_SELECT_SET, &size, &first, nullptr, nullptr);

        return m_op.Read(m_dset, m_type.obj, mspace, fspace, size);
    }

    void Write(size_t first, const Array<T>& array)
    {
        hsize_t size   = array.size();
        auto    mspace = H5::DataSpace(1, &size, nullptr);
        auto    fspace = m_dset.getSpace();
        fspace.selectHyperslab(H5S_SELECT_SET, &size, &first, nullptr, nullptr);

        m_op.Write(m_dset, m_type.obj, mspace, fspace, array);
    }

    void Append(const Array<T>& array)
    {
        hsize_t first = 0;
        auto    space = m_dset.getSpace();
        space.getSimpleExtentDims(&first, nullptr);

        hsize_t size = array.size();
        hsize_t last = first + size;
        m_dset.extend(&last);

        Write(first, array);
        m_size = last;
    }

    void Dump(const Array<T>& array)
    {
        hsize_t last = array.size();
        m_dset.extend(&last);

        m_op.Write(
          m_dset, m_type.obj, H5::DataSpace::ALL, H5::DataSpace::ALL, array);
        m_size = last;
    }

    size_t GetSize() const { return m_size; }

private:
    static bool DoExists(const auto& h5, const std::string& loc)
    {
        if (loc.size() > 0) {
            size_t pos = 1;

            while ((pos = loc.find('/', pos)) != loc.npos) {
                auto sub = loc.substr(0, pos++);
                if (not h5.exists(sub)) {
                    return false;
                }
            }
            return h5.exists(loc);
        }

        return true;
    }

    static std::vector<uint8_t> DoReadMetadata(const auto& h5)
    {
        std::vector<uint8_t> mpack;

        if (h5.attrExists(metadata_name)) {
            hsize_t size = 0;
            auto    attr = h5.openAttribute(metadata_name);
            attr.getSpace().getSimpleExtentDims(&size, nullptr);
            mpack.resize(size);
            attr.read(H5::PredType::NATIVE_B8, mpack.data());
        }

        return mpack;
    }

    static void DoDumpMetadata(const auto&                 h5,
                               const std::vector<uint8_t>& mpack)
    {
        if (h5.attrExists(metadata_name)) {
            h5.removeAttr(metadata_name);
        }

        hsize_t size = mpack.size();
        auto    attr = h5.createAttribute(metadata_name,
                                       H5::PredType::NATIVE_B8,
                                       H5::DataSpace(1, &size, nullptr));
        attr.write(H5::PredType::NATIVE_B8, mpack.data());
    }
};

} // namespace detail
} // namespace ds
