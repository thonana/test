#pragma once

#include "ds/ui/panel.h"
#include <wx/wx.h>

namespace ds {

class Form : public Panel
{
    wxFlexGridSizer* m_grid;

public:
    Form();
    Form(wxWindow*       parent,
         wxWindowID      id    = wxID_ANY,
         const wxString& title = _(""),
         const wxPoint&  pos   = wxDefaultPosition,
         const wxSize&   size  = wxDefaultSize,
         long            style = wxTAB_TRAVERSAL,
         const wxString& name  = wxPanelNameStr);

    ~Form() override;

    bool Create(wxWindow*       parent,
                wxWindowID      id    = wxID_ANY,
                const wxString& title = _(""),
                const wxPoint&  pos   = wxDefaultPosition,
                const wxSize&   size  = wxDefaultSize,
                long            style = wxTAB_TRAVERSAL,
                const wxString& name  = wxPanelNameStr);

    wxSizerItem* Add(wxWindow* window,
                     int       proportion = 0,
                     int       flag       = 0,
                     int       border     = 0,
                     wxObject* user_data  = NULL) override;

    wxSizerItem* Add(wxSizer*  sizer,
                     int       proportion = 0,
                     int       flag       = 0,
                     int       border     = 0,
                     wxObject* user_data  = NULL);

    std::pair<wxSizerItem*, wxSizerItem*> AddRow(const std::string& label,
                                                 const std::string& ctrl);

    std::pair<wxSizerItem*, wxSizerItem*> AddRow(const std::string& label,
                                                 wxWindow*          ctrl);

    std::pair<wxSizerItem*, wxSizerItem*> AddRow(wxWindow* label,
                                                 wxWindow* ctrl);

    std::pair<wxSizerItem*, wxSizerItem*> AddRow(const std::string& label,
                                                 wxSizer*           sizer);

    std::pair<wxSizerItem*, wxSizerItem*> AddRow(wxWindow* label,
                                                 wxSizer*  sizer);

    std::tuple<wxSizerItem*, wxSizerItem*, wxSizerItem*> AddRow(wxWindow* a,
                                                                wxWindow* b,
                                                                wxWindow* c);

    std::tuple<wxSizerItem*, wxSizerItem*, wxSizerItem*>
    AddRow(const std::string& a, wxWindow* b, const std::string& c);

    void SetColum(int num);

    wxStaticBox* GetStaticBox();

    wxGridSizer* GetGridSizer() override { return m_grid; }
};

} // namespace ds

// https://docs.wxwidgets.org/3.2/overview_sizer.html