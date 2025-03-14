#include "ds/ui/form.h"
#include <wx/stattext.h>

namespace ds {

Form::Form()
  : Panel()
  , m_grid(nullptr)
{
}

Form::Form(wxWindow*       parent,
           wxWindowID      id,
           const wxString& title,
           const wxPoint&  pos,
           const wxSize&   size,
           long            style,
           const wxString& name)
  : Form()
{
    Create(parent, id, title, pos, size, style, name);
}

Form::~Form() {}

bool
Form::Create(wxWindow*       parent,
             wxWindowID      id,
             const wxString& title,
             const wxPoint&  pos,
             const wxSize&   size,
             long            style,
             const wxString& name)
{
    if (Panel::Create(parent, id, title, pos, size, style, name)) {
        ActivateStyle("form");
        //m_grid->SetFlexibleDirection(wxBOTH);
        //m_grid->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

        m_grid = new wxFlexGridSizer(0, 2, 0, 0);

        auto sizer = new wxStaticBoxSizer(wxVERTICAL, this);
        sizer->Add(m_grid, 0, wxEXPAND | wxALL, 0);

        SetSizer(sizer);
        SetTitle(title);
        return true;
    }

    return false;
}

wxSizerItem*
Form::Add(wxWindow* window,
          int       proportion,
          int       flag,
          int       border,
          wxObject* user_data)
{
    return m_grid->Add(window, proportion, flag, border, user_data);
}

wxSizerItem*
Form::Add(wxSizer*  sizer,
          int       proportion,
          int       flag,
          int       border,
          wxObject* user_data)
{
    return m_grid->Add(sizer, proportion, flag, border, user_data);
}

std::pair<wxSizerItem*, wxSizerItem*>
Form::AddRow(const std::string& label, const std::string& ctrl)
{
    auto text = new wxStaticText(this, wxID_ANY, ctrl);
    return AddRow(label, text);
}

std::pair<wxSizerItem*, wxSizerItem*>
Form::AddRow(const std::string& label, wxWindow* ctrl)
{
    auto text = new wxStaticText(this, wxID_ANY, label);
    return AddRow(text, ctrl);
}

std::pair<wxSizerItem*, wxSizerItem*>
Form::AddRow(wxWindow* label, wxWindow* ctrl)
{
    auto a = Add(label, 0, wxALL | wxALIGN_RIGHT, 0);
    auto b = Add(ctrl, 1, wxALL | wxEXPAND, 0);

    return { a, b };
}

std::pair<wxSizerItem*, wxSizerItem*>
Form::AddRow(const std::string& label, wxSizer* sizer)
{
    auto text = new wxStaticText(this, wxID_ANY, label);
    return AddRow(text, sizer);
}

std::pair<wxSizerItem*, wxSizerItem*>
Form::AddRow(wxWindow* label, wxSizer* sizer)
{
    auto a = Add(label, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 0);
    auto b = Add(sizer, 0, wxEXPAND, 0);
    //auto b = Add(sizer, 1, wxALL | wxEXPAND, default_border);

    return { a, b };
}

std::tuple<wxSizerItem*, wxSizerItem*, wxSizerItem*>
Form::AddRow(wxWindow* a, wxWindow* b, wxWindow* c)
{
    auto row_a = Add(a, 0, wxALL | wxALIGN_LEFT);
    auto row_b = Add(b, 0, wxALL | wxEXPAND);
    auto row_c = Add(c, 0, wxALL | wxALIGN_RIGHT);

    return { row_a, row_b, row_c };
}

std::tuple<wxSizerItem*, wxSizerItem*, wxSizerItem*>
Form::AddRow(const std::string& a, wxWindow* b, const std::string& c)
{
    auto text  = new wxStaticText(this, wxID_ANY, a);
    auto text1 = new wxStaticText(this, wxID_ANY, c);

    auto row_a = Add(text, 0, wxALL | wxEXPAND);
    auto row_b = Add(b, 0, wxALL | wxEXPAND);
    auto row_c = Add(text1, 0, wxALL | wxEXPAND);

    return { row_a, row_b, row_c };
}

void
Form::SetColum(int num)
{
    m_grid->SetCols(num); 
}

wxStaticBox*
Form::GetStaticBox()
{
    auto sizer = static_cast<wxStaticBoxSizer*>(GetSizer());
    return sizer->GetStaticBox();
}

} // namespace ds

// https://wiki.wxpython.org/TwoStageCreation