#pragma once

#include <wx/dialog.h>
#include <wx/sizer.h>

namespace ds {

class Dialog : public wxDialog
{
public:
    static constexpr int default_proportion = 0;
    static constexpr int default_flag       = 0;
    static constexpr int default_border     = 5;

    Dialog();
    Dialog(wxWindow*       parent,
           wxWindowID      id    = wxID_ANY,
           const wxString& title = _(""),
           const wxPoint&  pos   = wxDefaultPosition,
           const wxSize&   size  = wxSize(-1, -1),
           long            style = wxDEFAULT_DIALOG_STYLE,
           const wxString& name  = wxDialogNameStr);

    ~Dialog() override;

    bool Create(wxWindow*       parent,
                wxWindowID      id    = wxID_ANY,
                const wxString& title = _(""),
                const wxPoint&  pos   = wxDefaultPosition,
                const wxSize&   size  = wxSize(-1, -1),
                long            style = wxDEFAULT_DIALOG_STYLE,
                const wxString& name  = wxDialogNameStr);

    wxSizerItem* Add(wxWindow* window,
                     int       proportion = default_proportion,
                     int       flag       = default_flag,
                     int       border     = default_border,
                     wxObject* user_data  = NULL);

    wxSizerItem* AddText(const std::string& label);

    wxBoxSizer* GetBoxSizer();
};

} // namespace ds