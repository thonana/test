#pragma once

#include "ds/async.h"
#include "ds/asynccondition.h"
#include <memory>
#include <optional>
#include <wx/app.h>

namespace ds {

template<typename T>
class AsyncWindow;

template<typename T>
class AsyncWindowProxy
{
    template<typename T>
    friend class AsyncWindow;

    struct Impl
    {
        T* window; // only accessed in GUI thread

        Impl(AsyncWindow<T>* window)
          : window(static_cast<T*>(window))
        {
        }
    };

    std::shared_ptr<Impl> m_impl;

public:
    AsyncWindowProxy(AsyncWindow<T>* window)
      : m_impl(std::make_shared<Impl>(window))
    {
    }

    ~AsyncWindowProxy() = default;

    template<typename... Ts, typename... Args>

    void CallAfter(void (T::*method)(Ts...), Args&&... args)
    {
        if (wxTheApp) {
            wxTheApp->CallAfter(
              [method, impl = m_impl, ... args = std::forward<Args>(args)]() {
                  if (impl->window) {
                      (impl->window->*method)(std::move(args)...);
                  }
              });
        }
    }
};

template<typename T>
class AsyncWindow
{
    AsyncWindowProxy<T> m_proxy;
    AsyncCondition      m_cancel;

public:
    AsyncWindow()
      : m_proxy(this)
    {
    }

    virtual ~AsyncWindow()
    {
        m_proxy.m_impl->window = nullptr;
        m_cancel.Notify();
    }

    AsyncWindowProxy<T> GetAsyncWindowProxy() { return m_proxy; }

    auto StartCoroutine(auto&& awaitable)
    {
        using namespace asio::experimental::awaitable_operators;

        return Async::StartCoroutine(
          std::forward<decltype(awaitable)>(awaitable) || m_cancel.AsyncWait(),
          asio::detached);
    }

    template<typename... Ts, typename... Args>
    void AsyncCallAfter(void (T::*method)(Ts...), Args&&... args)
    {
        m_proxy.CallAfter(method, std::forward<Args>(args)...);
    }

    template<typename... Ts>
    auto AsyncCallback(void (T::*method)(Ts...))
    {
        return [proxy = m_proxy, method](auto&& ...args) {
            proxy.CallAfter(method, std::forward<decltype(args)>(args)...);
        };
    }
};

} // namespace ds
