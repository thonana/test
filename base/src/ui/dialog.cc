#include "ds/ui/dialog.h"
#include <wx/stattext.h>

namespace ds {

Dialog::Dialog()
  : wxDialog()
{
}

Dialog::Dialog(wxWindow*       parent,
               wxWindowID      id,
               const wxString& title,
               const wxPoint&  pos,
               const wxSize&   size,
               long            style,
               const wxString& name)
  : Dialog()
{
    Create(parent, id, title, pos, size, style, name);
}

Dialog::~Dialog() {}

bool
Dialog::Create(wxWindow*       parent,
               wxWindowID      id,
               const wxString& title,
               const wxPoint&  pos,
               const wxSize&   size,
               long            style,
               const wxString& name)
{
    if (wxDialog::Create(parent, id, title, pos, size, style, name)) {
        auto sizer = new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);
        SetTitle(title);
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        return true;
    }
    return false;
}

wxSizerItem*
Dialog::Add(wxWindow* window,
            int       proportion,
            int       flag,
            int       border,
            wxObject* user_data)
{
    return GetSizer()->Add(window, proportion, flag, border, user_data);
}

wxSizerItem*
Dialog::AddText(const std::string& label)
{
    //auto sizer = static_cast<wxBoxSizer*>(GetSizer());
    auto text  = new wxStaticText(this, wxID_ANY, label);
    return Add(text, 0, wxALL | wxEXPAND, 5);
}

wxBoxSizer*
Dialog::GetBoxSizer()
{
    auto sizer = static_cast<wxBoxSizer*>(GetSizer());
    return sizer;
}

} // namespace ds