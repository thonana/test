
#include <fstream>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <chrono>
#include "label_strings.h"
#include "stage_automode.h"
#include "stage_settings.h"
#include "stage_utility.h"
#include "test_information.h"
#include "../../h5/include/h5.h"

namespace ds::depthscan {

static void
SaveAutoModeCsv(const std::string& path,
             const std::vector<double>& values,
             int length)
{
    std::ofstream file(path);

    if (not file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    file << "Value\n";

    for (size_t i = 0; i < length; i++) {
        file << i << "," << values[i] << "\n";
    }
    file.close();
}

StageAutoMode::StageAutoMode()
  : m_cancel(false)
  , m_pump(ui::CreateAsyncModel<StagePump>())
  , m_move(ui::CreateAsyncModel<StageMove>())
  , m_frame(ui::CreateAsyncModel<StageFrame>())
  , m_state(StageDSState::auto_idle)
  , m_high_speed(0)
  , m_normal_speed(0)
  , m_prime_seconds(0)
  , m_camera_seconds(0)
  , m_auto_check(false)
  , m_first_image(true)
  , m_filter(std::make_shared<BrightnessFilter>())
  , m_threshold_entry(PLAY_WATER_ENTRY_TH)
  , m_threshold_exit(PLAY_WATER_EXIT_TH)
  , m_manual_recording(false)
{
    auto storage = StageSettingStorage::GetInstance();
    if (storage) {
        m_auto_check =
          std::get<bool>(storage->GetSettings(StageConfigKeys::AUTO_DETECT));
        m_high_speed =
          std::get<float>(storage->GetSettings(StageConfigKeys::HIGH_SPEED));
        m_normal_speed =
          std::get<float>(storage->GetSettings(StageConfigKeys::NORMAL_SPEED));
        m_prime_seconds =
          std::get<int>(storage->GetSettings(StageConfigKeys::HIGH_SECONDS));
        m_camera_seconds =
          std::get<int>(storage->GetSettings(StageConfigKeys::NORMAL_SECONDS));
        m_threshold_entry = std::get<float>(
          storage->GetSettings(StageConfigKeys::THRESHOLD_ENTRY));
        m_threshold_exit = std::get<float>(
          storage->GetSettings(StageConfigKeys::THRESHOLD_EXIT));
    }
}
StageAutoMode::~StageAutoMode() 
{
}

void
StageAutoMode::Initiate() noexcept
{
}
bool
StageAutoMode::CheckEntry(double thres) const
{
    std::chrono::high_resolution_clock::time_point end =
      std::chrono::high_resolution_clock::now();

    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start_now);

    auto total_time = (m_prime_seconds * 1000);

    if (m_auto_check) {
        if (thres < PLAY_WATER_ENTRY_TH)
            return true;
        else
            return false;
    } else {
        if (duration.count() >= total_time)
            return true;
        else
            return false;    
    }
}

bool
StageAutoMode::CheckExit(double thres) const
{
    std::chrono::high_resolution_clock::time_point end =
      std::chrono::high_resolution_clock::now();

    auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start_now);

    auto total_time = (m_camera_seconds * 1000);

    if (m_auto_check) {
        if (thres < PLAY_WATER_EXIT_TH)
            return true;
        else
            return false;
    } else {
        if (duration.count() >= total_time)
            return true;
        else
            return false;
    }
}
asio::awaitable<void>
StageAutoMode::InitSetup(async::Lifeguard guard)
{
    m_send_progress = 0;
    m_send_label = LabelStrings::Start;
    SetState(StageDSState::auto_busy);
    co_await m_move->GetNotBusy(guard());
    StageFileHandle File(PATH_TO_AUTOMODE);
    File.DeleteWholeFiles();
}

asio::awaitable<void>
StageAutoMode::SearchFlow(async::Lifeguard guard, bool manual_recording)
{    
    bool done = false;
    auto timer = ds::async::Timer();
    StageDateTimeFormat Time;
    std::string TimeStamp = Time.GetTime();

    bool first_image = true;
    m_send_label = LabelStrings::Priming;
    co_await m_pump->StartPump(guard(), MotorDir::pump_dir_prime, m_high_speed);

    auto frame = co_await m_frame->GetAsyncFrame(guard());
    cv::Mat gray = frame->CreateGray();

    auto storage = StageSettingStorage::GetInstance();
    auto threshold_exit_2nd = -20.0;
    if (storage) {
        m_threshold_entry = std::get<float>(
          storage->GetSettings(StageConfigKeys::THRESHOLD_ENTRY));
        m_threshold_exit = std::get<float>(
          storage->GetSettings(StageConfigKeys::THRESHOLD_EXIT));
        threshold_exit_2nd = std::get<float>(
          storage->GetSettings(StageConfigKeys::THRESHOLD_EXIT2));
    }
    m_filter = std::make_shared<BrightnessFilter>(
      gray, m_threshold_entry, m_threshold_exit, threshold_exit_2nd);

    m_filter->SetFirst();

    nlohmann::json test_config;
    std::ifstream file(JSON_FILE_PATH);
    wxString currentName;
    if (file.is_open()) {
        file >> test_config;
        file.close();

        if (test_config.contains("current")) {
            currentName = wxString::FromUTF8(
              test_config["current"]["sampleName"].get<std::string>());

        } else {
            currentName = wxString::FromUTF8("test.a5");
        }
    }
    std::size_t pos = TimeStamp.find('.');
    if (pos != std::string::npos)
        TimeStamp = TimeStamp.substr(0, pos);
    currentName << "_";
    currentName << TimeStamp;
    currentName << ".dsf";

    // make today folder
    std::size_t underscorePos = TimeStamp.find('_');
    if (underscorePos != std::string::npos)
        TimeStamp = TimeStamp.substr(0, underscorePos);
    wxString makefolder = "C:\\ltis\\depthscan\\Data\\";
    makefolder += wxString::FromUTF8(TimeStamp.c_str());
    std::filesystem::path folder_path(makefolder);
    if (std::filesystem::exists(folder_path)) {
        //
    } else {
        std::filesystem::create_directory(folder_path);
    }

    folder_path += "\\" + currentName;

    //std::filesystem::path fs_path(currentName);
    m_frame->ArmRecording(m_filter, folder_path, "/table");
    spdlog::info("current sample name: {}", currentName.ToStdString());

    if (manual_recording) {
        co_await timer.AsyncSleepFor(guard(), 100ms);
        m_filter->ManualRecord();
    }

    while (not done and not m_cancel) {
        frame = co_await m_frame->GetAsyncFrame(guard());
        if (frame) {
            if (m_frame->GetState() == ds::camera::CameraState::recording) {
                spdlog::info("start recording");
                gray = frame->CreateGray();
                SetNeedRefocus(CheckFocusNeed(gray));

                if (m_high_speed != m_normal_speed) {
                    co_await m_pump->StartPump(
                      guard(), MotorDir::pump_dir_prime, m_normal_speed);
                }

                m_send_label = LabelStrings::Recording;

                done = true;
                co_return;
            }
        }
        //that_gray = gray.clone();
        co_await timer.AsyncSleepFor(guard(), 1ms);
    }
    co_return;
}
static 
void SaveImages(
  const std::string path, const cv::Mat& image, double angle)
{
    StageDateTimeFormat Time;
    std::string TimeStamp = Time.GetTime();

    StageFileHandle File(path);
    StageProcessImage Image;

    Image.SaveImages(
      File.GetFileName(TimeStamp, ".png"), image, angle);
}
asio::awaitable<void>
StageAutoMode::Recording(async::Lifeguard guard)
{
    bool done = false;
    bool first_image = true;
    auto timer = ds::async::Timer();
    cv::Mat gray;
    cv::Mat that_gray;
    cv::Mat that_roi;
    m_templates.clear();
    m_filter->SetNoCheck(false);
    while (not done and not m_cancel) {
        auto frame = co_await m_frame->GetAsyncFrame(guard());
        if (frame) {
            if (m_frame->GetState() == ds::camera::CameraState::arm) {

                spdlog::info("stop recording");
                co_await timer.AsyncSleepFor(guard(), 100ms);
                co_await m_pump->StopPump(guard());

                m_frame->StopRecording();

                done = true;
            }
        }
        co_await timer.AsyncSleepFor(guard(), 1ms);
    }
    co_return;
}
asio::awaitable<void>
StageAutoMode::Complete(async::Lifeguard guard)
{
    auto timer = ds::async::Timer();
    m_send_progress = 100;
    m_send_label = LabelStrings::Completed;
    co_await timer.AsyncSleepFor(guard(), 200ms);
      
    SetState(StageDSState::auto_idle);
    co_return;
}

asio::awaitable<void>
StageAutoMode::StartAutoMode(async::Lifeguard guard, bool manual_recording)
{
    auto timer = ds::async::Timer();
    m_cancel = false;
    co_await InitSetup(guard());
    co_await SearchFlow(guard(), manual_recording);
    if (m_cancel) {
        co_await timer.AsyncSleepFor(guard(), 100ms);
    } else {
        co_await timer.AsyncSleepFor(guard(), 3000ms);
    }
    co_await Recording(guard());
    co_await Complete(guard());

    co_return;
}
asio::awaitable<void>
StageAutoMode::CancelAutoMode(async::Lifeguard guard)
{
    m_cancel = true;
    if ((m_frame->GetState() == ds::camera::CameraState::arm) ||
        (m_frame->GetState() == ds::camera::CameraState::recording)) {  
        m_frame->StopRecording();
    }

    co_await m_pump->StopPump(guard());
    co_return;
}

bool
StageAutoMode::CheckFocusNeed(cv::Mat& src)
{
    std::string last_focus_path = PATH_TO_FOCUS + "/last";
    StageFileHandle File(last_focus_path);
    std::string file_name = File.GetPngFile();

    if (!std::filesystem::exists(file_name)) {
        spdlog::info("need to refocus : no file");
        return true;
    }

    cv::Mat base_image = cv::imread(file_name);

    if (base_image.empty()) {
        spdlog::info("need to refocus : invalid image");
        return true;
    }

    StageProcessImage Image;
    double base_sharpness = Image.Sharpness(base_image, true);
    double curr_sharpness = Image.Sharpness(src, true);

    spdlog::info("sharpness [{}->{}]", base_sharpness, curr_sharpness);
    if ((base_sharpness - curr_sharpness) > 1) {
        spdlog::info("need to refocus");
        return true;
    } else {
        spdlog::info("no need to refocus");
        return false;
    }
    return false;
}

bool
StageAutoMode::GetNeedRefocus() const
{
    auto storage = StageSettingStorage::GetInstance();
    bool refocus = false;
    if (storage) {
        refocus =
          std::get<bool>(storage->GetSettings(StageConfigKeys::REFOCUS));
    }
    return refocus;
}
void
StageAutoMode::SetNeedRefocus(bool refocus)
{
    auto storage = StageSettingStorage::GetInstance();
    if (storage) {
        storage->SaveSettingToJson(StageConfigKeys::REFOCUS, refocus);
        storage->AddSettings(StageConfigKeys::REFOCUS, refocus);
    }
}

void
StageAutoMode::SetState(uint32_t state)
{
    if (StageDSState::auto_busy == state)
        m_state &= ~(StageDSState::idle | StageDSState::auto_idle);
    if (StageDSState::auto_idle == state)
        m_state &= ~(StageDSState::idle | StageDSState::auto_busy);

    m_state |= state;
}
bool
StageAutoMode::IsState(uint32_t state) const
{
    if (m_state & state)
        return true;
    else
        return false;
}
} // namespace ds