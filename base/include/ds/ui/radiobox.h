#pragma once

#include <wx/radiobox.h>

namespace ds {

class RadioBox : public wxRadioBox
{
public:
    using wxRadioBox::wxRadioBox;

    template<typename T>
    void BindModel(void (T::*setter)(int), const std::shared_ptr<T>& model)
    {
        Bind(wxEVT_RADIOBOX, //
             [setter, &model](wxCommandEvent& event) {
                 std::invoke(setter, model.get(), event.GetInt());
             });
    }

    template<typename T>
    void BindModel(void                      (T::*setter)(const std::string&), //
              const std::shared_ptr<T>& model)
    {
        Bind(wxEVT_RADIOBOX, //
             [this, setter, &model](wxCommandEvent& event) {
                 std::invoke(setter, model.get(), GetString(event.GetInt()));
             });
    }
};

} // namespace ds
