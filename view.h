#pragma once

#include <memory>
#include <set>
#include <wx/app.h>
#include <wx/sizer.h>
#include <wx/wrapsizer.h>

namespace ds {

template<typename T>
class View;

template<typename T>
struct ViewProxy
{
    View<T>* view;
};

// T is Model<T>
template<typename T>
class View
{
protected:
    // lifetime and Render() placeholder
    std::shared_ptr<ViewProxy<T>> m_proxy;

public:
    View()
      : m_proxy(std::make_shared<ViewProxy<T>>(this))
    {
    }

    std::weak_ptr<ViewProxy<T>> GetViewProxy() const { return m_proxy; }

    virtual void Render(const T& model) = 0;
};

} // namespace ds
