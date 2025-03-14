#pragma once

#include <memory>
#include <mutex>
#include <stack>

namespace ds {

template<typename T>
class ObjectPool
{
    struct Impl
    {
        std::mutex                     mutex;
        std::stack<std::unique_ptr<T>> stack;
    };

    std::shared_ptr<Impl> m_impl;

public:
    ObjectPool()  = default;
    ~ObjectPool() = default;

    std::shared_ptr<T> Allocate()
    {
        std::unique_ptr<T> obj = nullptr;
        {
            auto lock = std::lock_guard<std::mutex>(m_impl->mutex);

            if (not m_impl->stack.empty()) {
                obj = std::move(m_impl->stack.top());
                m_impl->stack.pop();
            }
        }

        if (not obj) {
            obj = std::make_unique<T>();
        }

        return { obj.release(), [wp = std::weak_ptr<Impl>(m_impl)](T* p) {
                    if (auto impl = wp.lock()) {
                        auto lock = std::lock_guard<std::mutex>(impl->mutex);
                        impl->stack.emplace(p);
                    } else {
                        std::default_delete<T>()(p);
                    }
                } };
    }

    bool IsEmpty() const
    {
        auto lock = std::lock_guard<std::mutex>(m_impl->mutex);
        return m_impl->stack.empty();
    }

    size_t GetSize() const
    {
        auto lock = std::lock_guard<std::mutex>(m_impl->mutex);
        return m_impl->impl->stack.size();
    }
};

} // namespace ds