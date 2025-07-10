#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <shared_mutex>
#include <stdexcept>
#include <variant>

namespace ds::depthscan {

constexpr auto BIG_FOCUS_STEP = 1280;
constexpr auto BIG_FOCUS_TOTAL_STEPS = 44;
constexpr auto BIG_FOCUS_START_POS = -52840;
constexpr auto PLAY_WATER_ENTRY_TH = 3.0;
constexpr auto PLAY_WATER_EXIT_TH = -3.0;
constexpr auto FOCUS_WATER_ENTRY_TH = 0.5;
constexpr auto FOCUS_WATER_EXIT_TH = 0.6;

constexpr auto LINE_ROI_OFFSET = 200;

constexpr int ROTATE = 0;

extern std::string PATH_TO_FOCUS; 
extern std::string PATH_TO_HISTORY; 
extern std::string PATH_TO_AUTOMODE;
extern std::string PATH_TO_AUTOEXPOSURE;

using SettingType = std::variant<std::string, int, bool, float>;

class StageSettingStorage /// Singleton pattern
{
private:
    static std::shared_ptr<StageSettingStorage> instance;
    std::map<std::string, SettingType> m_settings;
    std::shared_mutex m_mutex;
    std::string m_camera_name;
    int m_lens = 4;
    int m_exposure_time = 2000; // in microseconds
    float m_pixel_size = 3.45f;

public:
    static std::shared_ptr<StageSettingStorage> GetInstance();

    StageSettingStorage() = default;

    void        AddSettings(const std::string& key, const SettingType& value);
    SettingType GetSettings(const std::string& key);
    void LoadSettingsFromJson(std::string cameraname) const;
    void LoadCameraSettingsFromJson(std::string cameraname);
    template<typename T>
    void        SaveSettingToJson(const std::string& key,T value);
    std::string file_path = "c:/ltis/depthscan/resources/settings/stage.json";
    std::string GetCameraName() const { return m_camera_name; }
    void SetCameraName(const std::string& camera_name)
    {
        m_camera_name = camera_name;
    }
    int GetLens() const { return m_lens; }
    float GetPixelSize() const { return m_pixel_size; }
    int GetExposureTime() const { return m_exposure_time; }
};

namespace StageConfigKeys {
constexpr const char* AUTO_DETECT = "auto_detect";
constexpr const char* INIT_X_POS = "init_x_pos";
constexpr const char* INIT_Y_POS = "init_y_pos";
constexpr const char* LAST_X_POS = "last_x_pos";
constexpr const char* LAST_Y_POS = "last_y_pos";
constexpr const char* CLEAN_SECONDS = "clean_seconds";
constexpr const char* CLEAN_SPEED = "clean_speed";
constexpr const char* FOCUS_STEP = "focus_step";
constexpr const char* FOCUS_NUMOFSTEP = "focus_numofstep";
constexpr const char* HIGH_SPEED = "high_speed";
constexpr const char* NORMAL_SPEED = "normal_speed";
constexpr const char* HIGH_SECONDS = "high_seconds";
constexpr const char* NORMAL_SECONDS = "normal_seconds";
constexpr const char* REFOCUS = "refocus";
constexpr const char* THRESHOLD_ENTRY = "thshd_entry";
constexpr const char* THRESHOLD_EXIT = "thsd_exit";
constexpr const char* THRESHOLD_EXIT2 = "thsd_exit2";
}

} // namespace ds