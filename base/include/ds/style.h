#pragma once

#include "ds/json.h"
#include "ds/model.h"
#include <filesystem>
#include <memory>
#include <string>

class wxWindow;
class wxWrapSizer;
class wxGridSizer;

namespace ds {

class Style : public Model<Style>
{
    Json m_cache;

public:
    using Ptr = std::shared_ptr<Style>;

    Style();
    ~Style();

    void Clear();

    void Add(const Json& j);

    bool IsEmpty() const { return m_cache.empty(); }

    template<typename T>
    void GetTo(const std::string& name, T& value) const
    {
        if (m_cache.contains(name)) {
            m_cache[name].get_to(value);
        }
    }

    const Json& GetJson() { return m_cache; }
};

class StyleView : public View<Style>
{
    std::shared_ptr<Style> m_style;

public:
    StyleView();

    void ActivateStyle(const std::string& cls);

    void Render(const Style& style) override;

    virtual wxWindow* GetWindow() { return nullptr; }

    virtual wxGridSizer* GetGridSizer() { return nullptr; }

    virtual wxWrapSizer* GetWrapSizer() { return nullptr; }
};

class SystemStyle
{
    Json                                          m_cache;
    std::map<std::string, std::shared_ptr<Style>> m_styles;

public:
    static void Load(const std::filesystem::path& path)
    {
        GetInstance().DoLoad(path);
    }

    static std::shared_ptr<Style> GetStyle(const std::string& cls)
    {
        return GetInstance().DoGetStyle(cls);
    }

    static void Update() { return GetInstance().DoUpdate(); }

private:
    static SystemStyle& GetInstance()
    {
        static SystemStyle inst;
        return inst;
    }

    SystemStyle();

    ~SystemStyle();

    void DoLoad(const std::filesystem::path& path);

    Json&                  DoMake(const std::string& cls, Json& body, Json& j);
    std::shared_ptr<Style> DoGetStyle(const std::string& cls);
    void                   DoUpdate();
};

} // namespace ds