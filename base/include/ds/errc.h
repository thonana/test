#pragma once

#include <system_error>

namespace ds {

enum class Errc
{
    unknown = 1,
    exception,
    h5_exception,
};

class ErrorCategory : public std::error_category
{
public:
    const char* name() const noexcept override { return "ds"; }
    std::string message(int condition) const override
    {
        switch (static_cast<Errc>(condition)) {
            case Errc::exception:
                return "exception occurred";
            case Errc::h5_exception:
                return "hdf5 error";
            default:
                return "unknown error";
        }
    }
};

inline std::error_code
make_error_code(Errc e) noexcept
{
    static auto ec = ErrorCategory();
    return { static_cast<int>(e), ec };
}

} // namespace ds

namespace std {
template<>
struct is_error_code_enum<ds::Errc> : true_type
{};

} // namespace std

// https://breese.github.io/2016/06/18/unifying-error-codes.html
// https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/