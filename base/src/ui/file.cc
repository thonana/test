#include "ds/ui/file.h"

namespace ds {

File::File()
  : Panel()
  , m_file(nullptr)
{
}

File::~File() {}

bool
File::Create(wxWindow*       parent,
             wxWindowID      id,
             const wxString& title,
             const wxPoint&  pos,
             const wxSize&   size,
             long            style,
             const wxString& name)
{
    if (Panel::Create(parent, id, title, pos, size, style, name)) {
        m_file = new wxFilePickerCtrl(
          GetStaticBox(),
          wxID_ANY,
          wxEmptyString,
          _("Select a file"),
          _("RAW files (*.raw)|*.raw|All files (*.*)|*.*"));

        Add(m_file, 0, wxEXPAND | wxALL);

        return true;
    }
    return false;
}

} //ds