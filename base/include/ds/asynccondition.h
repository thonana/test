#pragma once

#include "ds/async.h"
#include <asio/experimental/awaitable_operators.hpp>
#include <chrono>

namespace ds {

class AsyncCondition
{
    asio::steady_timer m_cond;

public:
    AsyncCondition()
      : m_cond(Async::GetExecutor())
    {
        m_cond.expires_at(std::chrono::steady_clock::time_point::max());
    }

    AsyncCondition(const auto& executor)
      : m_cond(executor)
    {
        m_cond.expires_at(std::chrono::steady_clock::time_point::max());
    }

    ~AsyncCondition()
    {
        m_cond.expires_at(std::chrono::steady_clock::time_point::max());
    }

    // cancels all waiting timers
    void Notify()
    {
        m_cond.expires_at(std::chrono::steady_clock::time_point::max());
    }

    // returns always true
    asio::awaitable<bool> AsyncWait()
    {
        auto [ec] =
          co_await m_cond.async_wait(asio::as_tuple(asio::use_awaitable));
        co_return ec == asio::error::operation_aborted;
    }

    // returns false when timeout
    // asio::awaitable<bool> AsyncWait(int msecs)
    //    {
    // using namespace asio::experimental::awaitable_operators;

    // auto timer = asio::steady_timer(m_cond.get_executor());
    // timer.expires_after(std::chrono::milliseconds(msecs));

    // auto [ec] =
    //   co_await m_cond.async_wait(asio::as_tuple(asio::use_awaitable));
    // co_return ec == asio::error::operation_aborted;

    // try {
    //     co_await (m_cond.async_wait(asio::use_awaitable) ||
    //               timer.async_wait(asio::use_awaitable));
    // } catch (const std::system_error& e) {
    //     if (e.code() == asio::error::operation_aborted) {
    //         co_return true;
    //     }
    //     throw;
    // }
    // co_return false;
    //}
};

} // namespace ds