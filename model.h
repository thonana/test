#pragma once

#include "ds/view.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <wx/app.h>

namespace ds {

template<typename T>
class Model : public std::enable_shared_from_this<T>
{
    using proxy_weak = std::weak_ptr<ViewProxy<T>>;
    std::set<proxy_weak, std::owner_less<proxy_weak>> m_proxies;

    bool m_fired;

public:
    Model()
      : m_fired(false)
    {
    }

    virtual ~Model() = default;

    void Register(const View<T>* view)
    {
        wxCHECK(view, /* void */);
        m_proxies.emplace(view->GetViewProxy());
        Render();
    }

    void Unregister(const View<T>* view)
    {
        wxCHECK(view, /* void */);
        m_proxies.erase(view->GetViewProxy());
    }

    virtual void Render()
    {
        if (wxTheApp && not m_fired) {
            m_fired = true;

            wxTheApp->CallAfter(
              [self = this->shared_from_this()]() { self->DoRender(); });
        }
    }

    template<typename... Ts, typename... Args>
    void CallAfter(void (T::*method)(Ts...), Args&&... args)
    {
        if (wxTheApp) {
            wxTheApp->CallAfter([method,
                                 self     = this->shared_from_this(),
                                 ... args = std::forward<Args>(args)]() {
                std::invoke(method, self.get(), std::move(args)...); 
            });
        }
    }

protected:
    virtual void DoRender()
    {
        m_fired = false;

        auto it = m_proxies.begin();
        while (it != m_proxies.end()) {
            if (auto proxy = it->lock()) {
                proxy->view->Render(*static_cast<T*>(this));
                ++it;
            } else {
                it = m_proxies.erase(it);
            }
        }
    }
};

} // namespace ds
