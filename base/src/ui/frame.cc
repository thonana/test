#include "ds/ui/frame.h"
#include "ds/style.h"

namespace ds {

Frame::Frame()
  : wxFrame()
  , StyleView()
{
}

Frame::Frame(wxWindow*       parent,
             wxWindowID      id,
             const wxString& title,
             const wxPoint&  pos,
             const wxSize&   size,
             long            style)
  : Frame()
{
    Create(parent, id, title, pos, size, style);
}

Frame::~Frame() {}

bool
Frame::Create(wxWindow*       parent,
              wxWindowID      id,
              const wxString& title,
              const wxPoint&  pos,
              const wxSize&   size,
              long            style)
{
    if (wxFrame::Create(parent, id, title, pos, size, style)) {
        ActivateStyle("frame");

        auto sizer = new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);

        return true;
    }
    return false;
}

wxSizerItem*
Frame::Add(wxWindow* window,
           int       proportion,
           int       flag,
           int       border,
           wxObject* user_data)
{
    return GetSizer()->Add(window, proportion, flag, border, user_data);
}

wxSizerItem*
Frame::Add(wxSizer*  sizer,
           int       proportion,
           int       flag,
           int       border,
           wxObject* user_data)
{
    return GetSizer()->Add(sizer, proportion, flag, border, user_data);
}

void
Frame::SwitchSizer(int direction)
{
    if (direction == 0) { // wxVERTICAL -> wxHORIZONTAL
        this->SetSizer(NULL, false);
        auto sizer = new wxBoxSizer(wxVERTICAL);
        this->SetSizer(sizer);
    } else { // wxHORIZONTAL -> wxVERTICAL
        this->SetSizer(NULL, false);
        auto sizer = new wxBoxSizer(wxHORIZONTAL);
        this->SetSizer(sizer);
    }
}

} // namespace ds

// https://wiki.wxpython.org/TwoStageCreation