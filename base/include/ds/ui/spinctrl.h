#pragma once

#include <memory>
#include <wx/spinctrl.h>

namespace ds {

class SpinCtrl : public wxSpinCtrl
{
public:
    static const long default_style =
      wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER | wxALIGN_RIGHT;

    SpinCtrl(wxWindow*       parent,
             wxWindowID      id      = wxID_ANY,
             const wxString& value   = wxEmptyString,
             const wxPoint&  pos     = wxDefaultPosition,
             const wxSize&   size    = wxDefaultSize,
             long            style   = default_style,
             int             min     = 0,
             int             max     = 100,
             int             initial = 0,
             const wxString& name    = "wxSpinCtrl")
      : wxSpinCtrl(parent, id, value, pos, size, style, min, max, initial, name)
    {
    }

    bool Create(wxWindow*       parent,
                wxWindowID      id      = wxID_ANY,
                const wxString& value   = wxEmptyString,
                const wxPoint&  pos     = wxDefaultPosition,
                const wxSize&   size    = wxDefaultSize,
                long            style   = default_style,
                int             min     = 0,
                int             max     = 100,
                int             initial = 0,
                const wxString& name    = "wxSpinCtrl")
    {
        wxSpinCtrl::Create(
          parent, id, value, pos, size, style, min, max, initial, name);
    }

    template<typename T>
    void BindModel(void (T::*setter)(int), const std::shared_ptr<T>& model)
    {
        Bind(wxEVT_SPINCTRL, //
             [setter, &model]([[maybe_unused]] wxSpinEvent& event) {
                 auto obj = model.get();
                 wxCHECK(obj);
                 // std::invoke(setter, model.get(), event.GetValue());
                 std::invoke(setter, obj, event.GetValue());
             });
    }
};

} // namespace ds
