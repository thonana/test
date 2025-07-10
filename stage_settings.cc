#include <iostream>
#include <windows.h>
#include <fstream>
#include <wx/chartype.h>
#include <spdlog/spdlog.h>
#include "ds/depthscan/stage_settings.h"

namespace ds::depthscan {

std::string PATH_TO_FOCUS = "c:/ltis/depthscan/resources/log/focus";
std::string PATH_TO_AUTOMODE = "c:/ltis/depthscan/resources/log/play";
std::string PATH_TO_HISTORY = "c:/ltis/depthscan/resources/history";
std::string PATH_TO_AUTOEXPOSURE = "c:/ltis/depthscan/resources/log/exposure";

static nlohmann::json
LoadConfig(const std::string& config_file)
{
    nlohmann::json config;

    //if ((_waccess(_T(".\\setting"), 0)) == -1)
    //    CreateDirectory(_T(".\\setting"), NULL);

    //if ((_waccess(_T(".\\setting\\focus"), 0)) == -1)
    //    CreateDirectory(_T(".\\setting\\focus"), NULL);

    std::ifstream file(config_file);

    if (!file.is_open()) {
        nlohmann::json j;
        std::ofstream  newFile(config_file);

        j[StageConfigKeys::AUTO_DETECT] = false;
        j[StageConfigKeys::CLEAN_SECONDS] = 120;
        j[StageConfigKeys::CLEAN_SPEED] = 2;
        j[StageConfigKeys::FOCUS_NUMOFSTEP] = 120;
        j[StageConfigKeys::FOCUS_STEP] = 256;
        j[StageConfigKeys::HIGH_SECONDS] = 10;
        j[StageConfigKeys::HIGH_SPEED] = 0.3;
        j[StageConfigKeys::INIT_X_POS] = 0;
        j[StageConfigKeys::INIT_Y_POS] = 0;
        j[StageConfigKeys::NORMAL_SECONDS] = 180;
        j[StageConfigKeys::NORMAL_SPEED] = 0.1;
        j[StageConfigKeys::REFOCUS] = false;
        j[StageConfigKeys::LAST_X_POS] = 0;
        j[StageConfigKeys::LAST_Y_POS] = 0;
        j[StageConfigKeys::THRESHOLD_ENTRY] = 3.0f;
        j[StageConfigKeys::THRESHOLD_EXIT] = -3.0f;
        j[StageConfigKeys::THRESHOLD_EXIT2] = -20.0f;
        newFile << j.dump(4);
        newFile.close();
        spdlog::info("Init json ");

        std::ifstream file2(config_file);
        file2 >> config;

    } else {

        file >> config;
    }
    return config;
}
static void
UpdateStageSettings(const std::string& config_file,std::string cameraname)
{
    try {
        nlohmann::json config = LoadConfig(config_file);
        //nlohmann::json config = Resource::LoadSettings(config_file);
        if (cameraname == "")
            cameraname = "U3-300xSE-C";
        auto& stage = config[cameraname];
        bool auto_mode = stage[StageConfigKeys::AUTO_DETECT];
        int init_x = stage[StageConfigKeys::INIT_X_POS];
        int init_y = stage[StageConfigKeys::INIT_Y_POS];
        int last_x = stage[StageConfigKeys::LAST_X_POS];
        int last_y = stage[StageConfigKeys::LAST_Y_POS];
        int clean_seconds = stage[StageConfigKeys::CLEAN_SECONDS];
        float clean = stage[StageConfigKeys::CLEAN_SPEED];
        int step = stage[StageConfigKeys::FOCUS_STEP];
        int numofstep = stage[StageConfigKeys::FOCUS_NUMOFSTEP];
        float high = stage[StageConfigKeys::HIGH_SPEED];
        float normal = stage[StageConfigKeys::NORMAL_SPEED];
        int prime_seconds = stage[StageConfigKeys::HIGH_SECONDS];
        int camera_seconds = stage[StageConfigKeys::NORMAL_SECONDS];
        bool re_focus = stage[StageConfigKeys::REFOCUS];
        float threshold_entry = stage[StageConfigKeys::THRESHOLD_ENTRY];
        float threshold_exit = stage[StageConfigKeys::THRESHOLD_EXIT];
        float threshold_exit2 = stage[StageConfigKeys::THRESHOLD_EXIT2];

        auto storage = StageSettingStorage::GetInstance();
        // Validate the values
        int abs_diff_step_x = abs(last_x - init_x) / step;
        if (abs_diff_step_x >= int(numofstep/5)) {
            init_x -= (int(numofstep/6) * step);
            storage->SaveSettingToJson(StageConfigKeys::INIT_X_POS, init_x);
        }


        if (storage) {
            storage->AddSettings(StageConfigKeys::AUTO_DETECT,
                                 static_cast<SettingType>(auto_mode));
            storage->AddSettings(StageConfigKeys::INIT_X_POS,
                                 static_cast<SettingType>(init_x));
            storage->AddSettings(StageConfigKeys::INIT_Y_POS,
                                 static_cast<SettingType>(init_y));
            storage->AddSettings(StageConfigKeys::LAST_X_POS,
                                 static_cast<SettingType>(last_x));
            storage->AddSettings(StageConfigKeys::LAST_Y_POS,
                                 static_cast<SettingType>(last_y));
            storage->AddSettings(StageConfigKeys::CLEAN_SECONDS,
                                 static_cast<SettingType>(clean_seconds));
            storage->AddSettings(StageConfigKeys::CLEAN_SPEED,
                                 static_cast<SettingType>(clean));
            storage->AddSettings(StageConfigKeys::FOCUS_STEP,
                                 static_cast<SettingType>(step));
            storage->AddSettings(StageConfigKeys::FOCUS_NUMOFSTEP,
                                 static_cast<SettingType>(numofstep));
            storage->AddSettings(StageConfigKeys::HIGH_SPEED,
                                 static_cast<SettingType>(high));
            storage->AddSettings(StageConfigKeys::NORMAL_SPEED,
                                 static_cast<SettingType>(normal));
            storage->AddSettings(StageConfigKeys::HIGH_SECONDS,
                                 static_cast<SettingType>(prime_seconds));
            storage->AddSettings(StageConfigKeys::NORMAL_SECONDS,
                                 static_cast<SettingType>(camera_seconds));
            storage->AddSettings(StageConfigKeys::REFOCUS,
                                 static_cast<SettingType>(re_focus));
            storage->AddSettings(StageConfigKeys::THRESHOLD_ENTRY,
                                 static_cast<SettingType>(threshold_entry));
            storage->AddSettings(StageConfigKeys::THRESHOLD_EXIT,
                                 static_cast<SettingType>(threshold_exit));
            storage->AddSettings(StageConfigKeys::THRESHOLD_EXIT2,
                                 static_cast<SettingType>(threshold_exit2));
        }

    } catch (const std::exception& e) {
        spdlog::error("json error {}", e.what());
    }
}

std::shared_ptr<StageSettingStorage>
StageSettingStorage::GetInstance()
{
    static auto shared = std::make_shared<StageSettingStorage>();
    return shared;
}
void
StageSettingStorage::AddSettings(const std::string& key,
                                 const SettingType& value)
{

    if (key.empty())
        throw std::invalid_argument("key cannot be empty");
    try {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_settings[key] = value;
    } catch (const std::exception& e) {
        std::cerr << "Exception during AddSettings:" << e.what() << std::endl;
    }
}

SettingType
StageSettingStorage::GetSettings(const std::string& key)
{
    std::shared_lock<std::shared_mutex>lock(m_mutex);
    if (m_settings.find(key) != m_settings.end()) {
        return m_settings[key];
    } else {
        throw std::out_of_range("Stage Setting Key not found.");
    }
}

void
StageSettingStorage::LoadSettingsFromJson(std::string cameraname) const
{
    UpdateStageSettings(file_path, cameraname);
}
void
StageSettingStorage::LoadCameraSettingsFromJson(std::string cameraname) 
{
    nlohmann::json config;
    std::ifstream file("c:/ltis/depthscan/resources/settings/camera.json");

    if (!file.is_open()) {
        nlohmann::json j;
        std::ofstream newFile(
          "c:/ltis/depthscan/resources/settings/camera.json");

        j["lens"] = 4;
        j["pixel_size"] = 3.45;
        j["exposure_time"] = 2000;

        newFile << j.dump(4);
        newFile.close();

        std::ifstream file2("c:/ltis/depthscan/resources/settings/camera.json");
        file2 >> config;

    } else {

        file >> config;
    }

    auto& cam = config[cameraname];
    if (cam.empty())
        spdlog::error("Camera settings for {} not found in JSON", cameraname);

    m_lens = cam["lens"].get<int>();
    m_pixel_size = cam["pixel_size"].get<float>();
    m_exposure_time = cam["exposure_time"].get<int>();
}
template<typename T>
void
StageSettingStorage::SaveSettingToJson(const std::string& key, T value)
{
    try {
        nlohmann::json config = LoadConfig(file_path);
        // nlohmann::json config = Resource::LoadSettings("stage.json");

        auto storage = StageSettingStorage::GetInstance();
        if (storage) {
            config[m_camera_name][key] = value;

            std::ofstream file(file_path);
            if (file.is_open()) {
                file << config.dump(4);
                file.close();
            }
        }

    } catch (const std::exception& e) {
        spdlog::error("json error {}", e.what());
    }
}
template void
StageSettingStorage::SaveSettingToJson<int>(const std::string& key, 
                                              int value);
template void
StageSettingStorage::SaveSettingToJson<float>(const std::string& key,
                                              float              value);
template void
StageSettingStorage::SaveSettingToJson<bool>(const std::string& key,
                                             bool               value);
template void
StageSettingStorage::SaveSettingToJson<std::string>(const std::string& key,
                                                    std::string        value);
} // namespace ds::depthscan