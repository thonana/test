#include "ds/ui/panel.h"
#include <wx/statbox.h>
#include <wx/stattext.h>

namespace ds {

Panel::Panel()
  : wxPanel()
  , StyleView()
{
}

Panel::Panel(wxWindow*       parent,
             wxWindowID      id,
             const wxString& title,
             const wxPoint&  pos,
             const wxSize&   size,
             long            style,
             const wxString& name)
  : Panel()
{
    Create(parent, id, title, pos, size, style, name);
}

Panel::~Panel() {}

bool
Panel::Create(wxWindow*       parent,
              wxWindowID      id,
              const wxString& title,
              const wxPoint&  pos,
              const wxSize&   size,
              long            style,
              const wxString& name)
{

    if (wxPanel::Create(parent, id, pos, size, style, name)) {
        ActivateStyle("panel");

        SetSizer(new wxStaticBoxSizer(wxVERTICAL, this, title));
        return true;
    }
    return false;
}

void
Panel::SetTitle(const std::string& title)
{
    auto sizer = static_cast<wxStaticBoxSizer*>(GetSizer());
    sizer->GetStaticBox()->SetLabel(title);
}

void
Panel::SetTitleMarkup(const std::string& title)
{
    auto sizer = static_cast<wxStaticBoxSizer*>(GetSizer());
    sizer->GetStaticBox()->SetLabelMarkup(title);
}

void
Panel::SetTitleText(const std::string& title)
{
    auto sizer = static_cast<wxStaticBoxSizer*>(GetSizer());
    sizer->GetStaticBox()->SetLabelText(title);
}

wxStaticBox*
Panel::GetStaticBox()
{
    auto sizer = static_cast<wxStaticBoxSizer*>(GetSizer());
    return sizer->GetStaticBox();
}

wxSizerItem*
Panel::Add(wxWindow* window,
           int       proportion,
           int       flag,
           int       border,
           wxObject* user_data)
{
    return GetSizer()->Add(window, proportion, flag, border, user_data);
}

wxSizerItem*
Panel::Add(wxSizer*  sizer,
           int       proportion,
           int       flag,
           int       border,
           wxObject* user_data)
{
    return GetSizer()->Add(sizer, proportion, flag, border, user_data);
}

wxSizerItem*
Panel::AddText(const std::string& label)
{
    auto text = new wxStaticText(this, wxID_ANY, label);
    return Add(text, 0, wxALL | wxEXPAND, 5);
}

} // namespace ds

// https://wiki.wxpython.org/TwoStageCreation