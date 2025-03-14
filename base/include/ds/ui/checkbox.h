#pragma once

#include <wx/button.h>

namespace ds {

class CheckBox : public wxCheckBox
{
public:
    using wxCheckBox::wxCheckBox;

    template<typename T>
    void BindModel(void (T::*setter)(), const std::shared_ptr<T>& model)
    {
        Bind(wxEVT_CHECKBOX, //
             [ctrl, setter, &model](wxCommandEvent& event) {
                 std::invoke(setter, model.get(), event.IsChecked());
             });
    }
};

} // namespace ds
