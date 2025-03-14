#include "ds/style.h"
#include <fstream>
#include <map>
#include <wx/app.h>
#include <wx/settings.h>
#include <wx/sizer.h>

namespace ds {

// ----------------------------------------------------------------------------
// Style
// ----------------------------------------------------------------------------
Style::Style()
{
    m_cache["bgcolor"] = wxSystemSettings::GetColour( //
                           wxSystemColour::wxSYS_COLOUR_WINDOW)
                           .GetAsString();

    m_cache["fgcolor"] = wxSystemSettings::GetColour( //
                           wxSystemColour::wxSYS_COLOUR_WINDOW)
                           .GetAsString();

    m_cache["font"] = wxSystemSettings::GetFont( //
                        wxSystemFont::wxSYS_SYSTEM_FONT)
                        .GetNativeFontInfoDesc();

    m_cache["border"] = 5;

    m_cache["grid_border"] = 5;
    m_cache["grid_vgap"]   = 5;
    m_cache["grid_hgap"]   = 5;
}

Style::~Style() = default;

void
Style::Clear()
{
    m_cache.clear();
}

void
Style::Add(const Json& j)
{
    m_cache.merge_patch(j);
    Render();
}

// ----------------------------------------------------------------------------
// StyleView
// ----------------------------------------------------------------------------
StyleView::StyleView()
  : m_style(nullptr)
{
}

void
StyleView::ActivateStyle(const std::string& cls)
{
    if (m_style) {
        m_style->Unregister(this);
    }

    m_style = SystemStyle::GetStyle(cls);
    m_style->Register(this);
}

void
StyleView::Render(const Style& style)
{
    if (auto window = GetWindow()) {
        std::string font = window->GetFont().GetNativeFontInfoDesc();
        style.GetTo("font", font);
        window->SetFont(wxFont(font));

        std::string bgcolor = window->GetBackgroundColour().GetAsString();
        style.GetTo("bgcolor", bgcolor);
        window->SetBackgroundColour(wxColour(bgcolor));

        std::string fgcolor = window->GetForegroundColour().GetAsString();
        style.GetTo("fgcolor", fgcolor);
        window->SetForegroundColour(wxColour(fgcolor));

        if (auto sizer = window->GetSizer()) {
            for (size_t i = 0; i < sizer->GetItemCount(); ++i) {
                auto item = sizer->GetItem(i);

                int border = item->GetBorder();
                style.GetTo("border", border);
                item->SetBorder(border);
            }
        }
    }

    if (auto sizer = GetWrapSizer()) {
        for (size_t i = 0; i < sizer->GetItemCount(); ++i) {
            auto item = sizer->GetItem(i);

            int border = item->GetBorder();
            style.GetTo("wrap_border", border);
            item->SetBorder(border);
        }
    }

    if (auto sizer = GetGridSizer()) {
        int vgap = sizer->GetVGap();
        style.GetTo("grid_vgap", vgap);
        sizer->SetVGap(vgap);

        int hgap = sizer->GetHGap();
        style.GetTo("grid_vgap", hgap);
        sizer->SetHGap(hgap);

        for (size_t i = 0; i < sizer->GetItemCount(); ++i) {
            auto item = sizer->GetItem(i);

            int border = item->GetBorder();
            style.GetTo("grid_border", border);
            item->SetBorder(border);
        }
    }
}

// ----------------------------------------------------------------------------
// SystemStyle
// ----------------------------------------------------------------------------
SystemStyle::SystemStyle()
{
    // TODO: load default style
}

SystemStyle::~SystemStyle() = default;

void
SystemStyle::DoLoad(const std::filesystem::path& path)
{
    auto f = std::ifstream(path);
    auto j = Json::parse(f);

    m_cache.clear();

    {
        auto it = j.find("*");
        if (it != j.end()) {
            m_cache["*"] = std::move(it.value());
            j.erase(it);
        }
    }

    for (auto it = j.begin(); it != j.end();) {
        std::string cls  = it.key();
        Json        body = std::move(it.value());

        // to prevent cyclic reference
        it = j.erase(it);

        if (not cls.empty()) {
            auto style = DoGetStyle(cls);
            style->Add(DoMake(cls, body, j));
        }
    }

    if (true) {
        std::ofstream of("a.json");
        of << std::setw(4) << m_cache << std::endl;
    }

    DoUpdate();
}

Json&
SystemStyle::DoMake(const std::string& cls, Json& body, Json& j)
{
    if (not m_cache.contains(cls)) {
        Json data = m_cache["*"];

        if (body.contains("*")) {
            auto key = body["*"].get<std::string>();

            auto it = j.find(key);
            if (it != j.end()) {
                data.merge_patch(DoMake(key, it.value(), j));
                j.erase(it);
            }
        }

        data.merge_patch(body);
        m_cache[cls] = data;
    }

    return m_cache[cls];
}

std::shared_ptr<Style>
SystemStyle::DoGetStyle(const std::string& cls)
{
    if (m_styles.contains(cls)) {
        return m_styles[cls];
    }

    auto style    = std::make_shared<Style>();
    m_styles[cls] = style;

    return style;
}

void
SystemStyle::DoUpdate()
{
    if (wxTheApp) {
        wxTheApp->CallAfter([]() {
            if (auto top = wxTheApp->GetTopWindow()) {
                top->Fit();
                top->Layout();
                top->Refresh();
                top->Update();
            }
        });
    }
}

} // namespace ds
