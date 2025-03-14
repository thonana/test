#pragma once

#include <memory>
#include <wx/button.h>

namespace ds {

class Button : public wxButton
{
public:
    // wxButton의 constructor 사용
    using wxButton::wxButton;

    // model의 shared_ptr의 reference를 보고있기 때문에,
    // wxButton이 model의 shared_ptr이 더 오래 살아있어야 한다.
    template<typename T>
    void BindModel(void (T::*setter)(), std::shared_ptr<T>& model)
    {
        Bind(wxEVT_BUTTON, //
             [setter, &model]([[maybe_unused]] wxCommandEvent& event) {
                 auto obj = model.get();
                 wxCHECK(obj);
                 std::invoke(setter, obj);
             });
    }
};

} // namespace ds
