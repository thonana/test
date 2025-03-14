#pragma once

#include "ds/style.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbox.h>

namespace ds {

class Panel
  : public wxPanel
  , public StyleView
{
public:
    Panel();
    Panel(wxWindow*       parent,
          wxWindowID      id    = wxID_ANY,
          const wxString& title = _(""),
          const wxPoint&  pos   = wxDefaultPosition,
          const wxSize&   size  = wxDefaultSize,
          long            style = wxTAB_TRAVERSAL,
          const wxString& name  = wxPanelNameStr);

    ~Panel() override;

    bool Create(wxWindow*       parent,
                wxWindowID      id    = wxID_ANY,
                const wxString& title = _(""),
                const wxPoint&  pos   = wxDefaultPosition,
                const wxSize&   size  = wxDefaultSize,
                long            style = wxTAB_TRAVERSAL,
                const wxString& name  = wxPanelNameStr);

    void SetTitle(const std::string& title);
    void SetTitleMarkup(const std::string& title);
    void SetTitleText(const std::string& title);

    wxStaticBox* GetStaticBox();

    virtual wxSizerItem* Add(wxWindow* window,
                             int       proportion = 0,
                             int       flag       = 0,
                             int       border     = 0,
                             wxObject* user_data  = NULL);

    virtual wxSizerItem* Add(wxSizer*  sizer,
                             int       proportion = 0,
                             int       flag       = 0,
                             int       border     = 0,
                             wxObject* user_data  = NULL);

    wxSizerItem* AddText(const std::string& label);

    wxWindow* GetWindow() override { return this; }


    //bind model
    using wxPanel::wxPanel;

    template<typename T>
    void BindModel(void (T::*setter)(), std::shared_ptr<T>& model)
    {
        Bind(wxEVT_PAINT, //
             [setter, &model]([[maybe_unused]] wxCommandEvent& event) {
                 auto obj = model.get();
                 wxCHECK(obj);
                 std::invoke(setter, obj);
             });
    }
};

} // namespace ds