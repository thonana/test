#pragma once

#include "ds/errc.h"
#include <H5Cpp.h>
#include <optional>
#include <spdlog/spdlog.h>

namespace ds {
namespace detail {

template<typename Task, typename Handler>
class AsyncImpl
{
    using result_type = std::invoke_result_t<Task>;
    Task    m_task;
    Handler m_handler;

public:
    AsyncImpl(Task&& task, Handler&& handler)
      : m_task(std::move(task))
      , m_handler(std::move(handler))
    {
    }

    ~AsyncImpl() = default;

    void operator()() noexcept
    {
        if constexpr (std::is_void_v<result_type>) {
            try {
                m_task();
                m_handler(std::error_code());

            } catch (const std::system_error& e) {
                spdlog::error("caught system exception: '{}'", e.what());
                m_handler(e.code());

            } catch (const std::exception& e) {
                spdlog::error("caught exception: '{}'", e.what());
                m_handler(Errc::exception);

            } catch (const H5::Exception& e) {
                spdlog::error("caught hdf5 exception: '{}'", e.getDetailMsg());
                m_handler(Errc::hdf5_error);

            } catch (...) {
                spdlog::error("unknown exception");
                m_handler(Errc::unknown);
            }

        } else {
            try {
                m_handler(std::error_code(), m_task());

            } catch (const std::system_error& e) {
                spdlog::error("caught system exception: '{}'", e.what());
                m_handler(e.code(), result_type());

            } catch (const std::exception& e) {
                spdlog::error("caught exception: '{}'", e.what());
                m_handler(Errc::exception, result_type());

            } catch (const H5::Exception& e) {
                spdlog::error("caught hdf5 exception: '{}'", e.getDetailMsg());
                m_handler(Errc::hdf5_error, result_type());

            } catch (...) {
                spdlog::error("unknown exception occurred");
                m_handler(Errc::unknown, result_type());
            }
        }
    }
};

} // namespace detail
} // namespace ds

// https://lastviking.eu/asio_composed.html
// https://think-async.com/Asio/asio-1.30.2/doc/asio/overview/composition/compose.html
// https://think-async.com/Asio/asio-1.30.2/doc/asio/overview/model/completion_tokens.html
