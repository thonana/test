#pragma once

#include "ds/detail/async_impl.h"
#include "ds/errc.h"
#include <asio.hpp>
#include <chrono>
#include <memory>
#include <spdlog/spdlog.h>

namespace ds {

template<typename Executor>
class AsyncEvent
{
    asio::steady_timer m_timer;
    bool               m_flag;

public:
    AsyncEvent(Executor& ex)
      : m_timer(ex, std::chrono::time_point<std::chrono::steady_clock>::max())
      , m_flag(false)
    {
    }

    ~AsyncEvent() = default;

    bool Set()
    {
        m_flag = true;
        m_timer.expires_at(
          std::chrono::time_point<std::chrono::steady_clock>::max());
    }

    void Clear() { m_flag = false; }
    bool IsSet() { return m_flag; }

    asio::awaitable<bool> AsyncWait()
    {
        try {
            co_await m_timer.async_wait(asio::use_awaitable);
        } catch (std::system_error& e) {
            if (e.code() != asio::error::operation_aborted) {
                throw;
            }
        }

        co_return m_flag;
    }
};

template<typename Task, typename Handler>
class AsyncOp;

// 핸들러를 io_service 내부에 추가
template<typename Executor, typename Task, typename Token>
auto
AsyncPost(Executor& executor, Task&& task, Token&& token)
{
    using result_type = std::invoke_result_t<Task>;
    if constexpr (std::is_void_v<result_type>) {
        return asio::async_initiate<Token, void(std::error_code)>(
          [&executor, task = std::forward<Task>(task)](auto&& handler) mutable {
              asio::post( //
                executor,
                AsyncOp(std::move(task), std::move(handler)));
          },
          std::forward<Token>(token));
    } else {
        return asio::async_initiate<Token, void(std::error_code, result_type)>(
          [&executor, task = std::forward<Task>(task)](auto&& handler) mutable {
              asio::post( //
                executor,
                AsyncOp(std::move(task), std::move(handler)));
          },
          std::forward<Token>(token));
    }
}
// dispatch한 핸들러를 io_service 내부에 추가하지 않고, 즉시 실행.
template<typename Executor, typename Task, typename Token>
auto
AsyncDispatch(Executor& executor, Task&& task, Token&& token)
{
    using result_type = std::invoke_result_t<decltype(task)>;
    if constexpr (std::is_void_v<result_type>) {
        return asio::async_initiate<decltype(token), void(std::error_code)>(
          [&executor,
           task = std::forward<decltype(task)>(task)](auto&& handler) mutable {
              asio::dispatch( //
                executor,
                detail::AsyncImpl(std::move(task), std::move(handler)));
          },
          token);
    } else {
        return asio::async_initiate<decltype(token),
                                    void(std::error_code, result_type)>(
          [&executor,
           task = std::forward<decltype(task)>(task)](auto&& handler) mutable {
              asio::dispatch( //
                executor,
                detail::AsyncImpl(std::move(task), std::move(handler)));
          },
          token);
    }
}

// 실행을 위해 완료 함수 전달
template<typename Executor, typename Task, typename Token>
auto
AsyncDefer(Executor& executor, Task&& task, Token&& token)
{
    using result_type = std::invoke_result_t<decltype(task)>;
    if constexpr (std::is_void_v<result_type>) {
        return asio::async_initiate<decltype(token), void(std::error_code)>(
          [&executor,
           task = std::forward<decltype(task)>(task)](auto&& handler) mutable {
              asio::defer( //
                executor,
                detail::AsyncImpl(std::move(task), std::move(handler)));
          },
          token);
    } else {
        return asio::async_initiate<decltype(token),
                                    void(std::error_code, result_type)>(
          [&executor,
           task = std::forward<decltype(task)>(task)](auto&& handler) mutable {
              asio::defer( //
                executor,
                detail::AsyncImpl(std::move(task), std::move(handler)));
          },
          token);
    }
}

// system-wide async thread pool
class Async
{
    asio::thread_pool
      m_pool; // 사전에 명시적으로 수행하지 않은 경우 자동 중지 후 연결

    static Async& GetInstance()
    {
        static Async s_instance;
        return s_instance;
    }

public:
    using executor_type = asio::thread_pool::executor_type;

    static asio::thread_pool& GetContext() noexcept
    {
        return GetInstance().m_pool;
    }

    static asio::thread_pool::executor_type GetExecutor() noexcept
    {
        return GetContext().get_executor();
    }

    static void Join() { GetContext().join(); }

    static auto StartCoroutine(auto&& awaitable, auto&& token)
    {
        return asio::co_spawn(GetExecutor(),
                              std::forward<decltype(awaitable)>(awaitable),
                              std::forward<decltype(token)>(token));
    }

    static auto Dispatch(auto&& task, auto&& token)
    {
        return AsyncDispatch(GetExecutor(),
                             std::forward<decltype(task)>(task),
                             std::forward<decltype(token)>(token));
    }

    static auto Post(auto&& task, auto&& token)
    {
        return AsyncPost(GetExecutor(),
                         std::forward<decltype(task)>(task),
                         std::forward<decltype(token)>(token));
    }

    static auto Defer(auto&& task, auto&& token)
    {
        return AsyncDefer(GetExecutor(),
                          std::forward<decltype(task)>(task),
                          std::forward<decltype(token)>(token));
    }

private:
    Async()
      : m_pool()
    {
    }

    ~Async() { m_pool.join(); }
};

template<typename Task, typename Handler>
class AsyncOp
{
    using result_type = std::invoke_result_t<Task>;
    Task    m_task;
    Handler m_handler;

public:
    AsyncOp(Task&& task, Handler&& handler)
      : m_task(std::move(task))
      , m_handler(std::move(handler))
    {
    }

    ~AsyncOp() = default;

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

            } catch (...) {
                spdlog::error("unknown exception occurred");
                m_handler(Errc::unknown, result_type());
            }
        }
    }
};

} // namespace ds

// https://lastviking.eu/asio_composed.html
// https://think-async.com/Asio/asio-1.30.2/doc/asio/overview/composition/compose.html
// https://think-async.com/Asio/asio-1.30.2/doc/asio/overview/model/completion_tokens.html
