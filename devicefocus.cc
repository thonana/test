
#include "devicefocus.h"

DeviceFocus::DeviceFocus()
  : m_up_btn(nullptr)
  , m_down_btn(nullptr)
  , m_right_btn(nullptr)
  , m_left_btn(nullptr)
  , m_init_btn(nullptr)
  , m_AF_btn(nullptr)
  , m_home_btn(nullptr)
  , Form()
  , m_stage(nullptr)
{
}

bool
DeviceFocus::Create(wxWindow*       parent,
                    wxWindowID      id,
                    const wxString& title,
                    const wxPoint&  pos,
                    const wxSize&   size,
                    long            style,
                    const wxString& name)
{
    if (Form::Create(parent, id, title, pos, size, style, name)) {
        SetColum(3);

        SetFont(
          wxFont(13, // wxNORMAL_FONT->GetPointSize(), 전체 폰트 사이즈 조절
                 wxFONTFAMILY_DEFAULT,
                 wxFONTSTYLE_NORMAL,
                 wxFONTWEIGHT_NORMAL,
                 false,
                 wxEmptyString));

        auto size = wxSize(60, 60);
        //
        m_up_btn =
          new ds::Button(this, wxID_ANY, _(L"▲"), wxDefaultPosition, size);

        m_up_btn->BindModel(&ds::StageAgent::MoveUp, m_stage);

        AddRow("", m_up_btn, "");
        //
        m_left_btn =
          new ds::Button(this, wxID_ANY, _(L"◀"), wxDefaultPosition, size);
        //m_left_btn->BindModel(&ds::Stage::Move_Left, m_stage);
        m_home_btn =
          new ds::Button(this, wxID_ANY, _("Home"), wxDefaultPosition, size);
        //m_home_btn->BindModel(&ds::Stage::Home, m_stage);
        m_right_btn =
          new ds::Button(this, wxID_ANY, _(L"▶"), wxDefaultPosition, size);
        //m_right_btn->BindModel(&ds::Stage::Move_Right, m_stage);
        AddRow(m_left_btn, m_home_btn, m_right_btn);

        //
        m_init_btn =
          new ds::Button(this, wxID_ANY, _("init."), wxDefaultPosition, size);
        //m_init_btn->BindModel(&ds::Stage::Init, m_stage);
        m_down_btn =
          new ds::Button(this, wxID_ANY, _(L"▼"), wxDefaultPosition, size);
        //m_down_btn->BindModel(&ds::Stage::Move_Down, m_stage);
        m_AF_btn = new ds::Button(
          this, wxID_ANY, _("Auto\nFocus"), wxDefaultPosition, size);
        //m_AF_btn->BindModel(&ds::Stage::AutoFocus, m_stage);
        AddRow(m_init_btn, m_down_btn, m_AF_btn);
    }
    return false;
}
