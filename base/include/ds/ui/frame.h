#pragma once

#include "ds/style.h"
#include <memory>
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/toolbar.h>

namespace ds {

class Frame
  : public wxFrame
  , public StyleView
{
public:
    Frame();
    Frame(wxWindow*       parent,
          wxWindowID      id    = wxID_ANY,
          const wxString& title = _(""),
          const wxPoint&  pos   = wxDefaultPosition,
          const wxSize&   size  = wxSize(-1, -1),
          long            style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

    ~Frame() override;

    bool Create(wxWindow*       parent,
                wxWindowID      id    = wxID_ANY,
                const wxString& title = _(""),
                const wxPoint&  pos   = wxDefaultPosition,
                const wxSize&   size  = wxSize(-1, -1),
                long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

    wxSizerItem* Add(wxWindow* window,
                     int       proportion = 0,
                     int       flag       = 0,
                     int       border     = 0,
                     wxObject* user_data  = NULL);

    wxSizerItem* Add(wxSizer*  sizer,
                     int       proportion = 0,
                     int       flag       = 0,
                     int       border     = 0,
                     wxObject* user_data  = NULL);

    void SwitchSizer(int direction);

    // wxStaticBox* GetStaticBox();

    // template<typename T>
    // void BindTool(wxToolBarToolBase* tool,
    //               void (T::*setter)(),
    //               std::shared_ptr<T>& model)
    //{
    // }

    // model의 shared_ptr의 reference를 보고있기 때문에,
    // wxTollBase이 model의 shared_ptr이 더 오래 살아있어야 한다.
    template<typename T>
    void BindTool(void (T::*setter)(), std::shared_ptr<T>& model)
    {
        Bind(wxEVT_TOOL, //
             [setter, &model]([[maybe_unused]] wxCommandEvent& event) {
                 auto obj = model.get();
                 wxCHECK(obj);
                 std::invoke(setter, obj);
             });
    }
};

} // namespace ds