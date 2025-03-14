#pragma once

#include <memory>
#include <wx/filepicker.h>

#include "ds/ui/panel.h"

namespace ds {

class File : public ds::Panel
//, public wxFilePickerCtrl
{
    wxFilePickerCtrl* m_file;

public:
    File();

    ~File() override;

    bool Create(wxWindow*       parent,
                wxWindowID      id    = wxID_ANY,
                const wxString& title = _(""),
                const wxPoint&  pos   = wxDefaultPosition,
                const wxSize&   size  = wxDefaultSize,
                long            style = wxTAB_TRAVERSAL,
                const wxString& name  = wxPanelNameStr);

    // wxButton의 constructor 사용
    //using wxFilePickerCtrl::wxFilePickerCtrl;

    // model의 shared_ptr의 reference를 보고있기 때문에,
    // wxFilePickerCtrl의 model의 shared_ptr이 더 오래 살아있어야 한다.
    template<typename T>
    void BindModel(void (T::*setter)(), std::shared_ptr<T>& model)
    {
        Bind(wxEVT_FILEPICKER_CHANGED, //
             [setter, &model]([[maybe_unused]] wxFileDirPickerEvent& event) {
                 auto obj = model.get();
                 wxCHECK(obj);
                 std::invoke(setter, obj);
             });
    }
};

} // namespace ds
