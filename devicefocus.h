#include "ds/ui/form.h"
#include "ds/ui/button.h"
#include <wx/wx.h>
#include "ds/dev/stage.h"
#include "ds/view.h"

class DeviceFocus
  : public ds::Form
  , public ds::View<ds::StageAgent>
//, public ds::ui::AsyncWindow<ImageListView>
{
    // std::shared_ptr<ImageList> m_list;
    ds::Button* m_up_btn;
    ds::Button* m_down_btn;
    ds::Button* m_right_btn;
    ds::Button* m_left_btn;
    ds::Button* m_init_btn;
    ds::Button* m_AF_btn;
    ds::Button* m_home_btn;

    std::shared_ptr<ds::StageAgent> m_stage;

public:
    DeviceFocus();

    bool Create(wxWindow*       parent,
                wxWindowID      id    = wxID_ANY,
                const wxString& title = _(""),
                const wxPoint&  pos   = wxDefaultPosition,
                const wxSize&   size  = wxDefaultSize,
                long            style = wxTAB_TRAVERSAL,
                const wxString& name  = wxPanelNameStr);

    void SetStage(const std::shared_ptr<ds::StageAgent>& stage)
    {
        m_stage = stage;
        m_stage->Register(this);
    }
    void Render(const ds::StageAgent& stage) override { 

    }
};
