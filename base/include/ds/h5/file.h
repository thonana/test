#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace ds {
namespace h5 {

enum class Mode
{
    readonly,
    writable,
    trucate,
};

class File
{
    struct Impl;
    std::shared_ptr<Impl> m_impl;

public:
    File(const std::filesystem::path& path);
    ~File();

    bool Exists(const std::string& loc);

    template<typename T>
    void SetAttribute(const std::string& key, T&& value)
    {
    }

    template<typename T>
    T GetAttribute(const std::string& key)
    {
    }
};

} // namespace h5
} // namespace ds
