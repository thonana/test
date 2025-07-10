
#include <fstream>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>

#include "stage_settings.h"
#include "stage_utility.h"
#include "stage_autoexposure.h"
#include <stage_autofocus.h>

namespace ds::depthscan {
constexpr int iteration = 10;

StageAutoExposure::StageAutoExposure() 
  : m_frame(ui::CreateAsyncModel<StageFrame>())
  , m_move(ui::CreateAsyncModel<StageMove>())
  , m_iteration(0)
  , m_cancel(false)
  , m_exposure_value(50us)
{
}
StageAutoExposure::~StageAutoExposure()
{
}
void
StageAutoExposure::Initiate() noexcept
{
}

static void
SaveExposureCsv(const std::string& path,
                const std::vector<ExposureResult>& values)
{
    std::ofstream file(path);

    if (not file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    file << "Exposure,Bright,Contrast,Over,Under,Score\n";

    for (size_t i = 0; i < values.size(); i++) {
        file << values[i].exposure_time << "," << values[i].brightness << ","
             << values[i].contrast << "," << values[i].overexposed_ratio << ","
             << values[i].underexposed_ratio << "," << values[i].quality_score
             << "\n";
    }

    file.close();
}
asio::awaitable<void>
StageAutoExposure::InitSetup(async::Lifeguard guard)
{
    StageFileHandle File(PATH_TO_AUTOEXPOSURE);
    File.DeleteWholeFiles();
    m_iteration = 0;
    m_exposure_data.resize(m_iteration);
    m_exposure_value = 50us;
    co_return;
}
asio::awaitable<void>
StageAutoExposure::Processing(async::Lifeguard guard)
{
    StageProcessImage Image;

    StageDateTimeFormat Time;
    std::string TimeStamp = Time.GetTime();
    StageFileHandle File(PATH_TO_AUTOEXPOSURE);
    m_exposure_data.resize(iteration);
    auto timer = ds::async::Timer();

    m_frame->GetCamera()->SetExposureTime(m_exposure_value);
    co_await timer.AsyncSleepFor(guard(), 1000ms);

    while ((m_iteration < iteration) and (not m_cancel)){
        auto frame = co_await m_frame->GetAsyncFrame(guard());
        if (frame) {
            auto gray = frame->CreateGray();
            //1. Brightness
            m_exposure_data[m_iteration].brightness = cv::mean(gray)[0];
            //2. Contrast
            cv::Scalar mean, stddev;
            cv::meanStdDev(gray, mean, stddev);
            m_exposure_data[m_iteration].contrast = stddev[0];
            //3. Histogram -> over/under exposure
            auto hist = Image.CalculateHistogram(gray);
            double total = gray.rows * gray.cols;
            hist /= total;

            double over_exposed = 0.0f;
            for (int i = 240; i < 256; i++) {
                over_exposed += hist.at<float>(i);
            }
            m_exposure_data[m_iteration].overexposed_ratio = over_exposed * 100.0;

            double under_exposed = 0.0;
            for (int i = 0; i < 15; i++) {
                under_exposed += hist.at<float>(i);
            }
            m_exposure_data[m_iteration].underexposed_ratio = under_exposed * 100.0;
            // 4. score
            double brightness_score =
              1.0 -
              std::abs(m_exposure_data[m_iteration].brightness - 128.0) / 128.0;
            double exposure_penalty =
              5.0 * (m_exposure_data[m_iteration].overexposed_ratio / 100.0 +
                     m_exposure_data[m_iteration].underexposed_ratio / 100.0);

            m_exposure_data[m_iteration].quality_score =
              (0.4 * brightness_score +
               0.3 *
                 std::min(m_exposure_data[m_iteration].contrast / 50.0, 1.0) -
               0.3 * exposure_penalty) *
              100.0;

            m_exposure_data[m_iteration].quality_score =
              std::max(0.0, m_exposure_data[m_iteration].quality_score);

            m_exposure_data[m_iteration].exposure_time = m_exposure_value;
            Image.SaveImages(
              File.GetFileName(
                TimeStamp, ".png", m_exposure_value, m_iteration),
              gray,
              90);
            m_exposure_value += 10us;
            m_iteration++;
            m_frame->GetCamera()->SetExposureTime(m_exposure_value);

            co_await timer.AsyncSleepFor(guard(), 1000ms);
        }
    }
    co_return;
}
asio::awaitable<void>
StageAutoExposure::Complete(async::Lifeguard guard)
{
    StageProcessImage Image;

    StageDateTimeFormat Time;
    std::string TimeStamp = Time.GetTime();
    StageFileHandle File(PATH_TO_AUTOEXPOSURE);

    auto best_it =
      std::max_element(m_exposure_data.begin(),
                       m_exposure_data.end(),
                       [](const ExposureResult& a, const ExposureResult& b) {
                           return a.quality_score < b.quality_score;
                       });
    m_exposure_value = best_it->exposure_time;

    auto timer = ds::async::Timer();

    m_frame->GetCamera()->SetExposureTime(m_exposure_value);
    co_await timer.AsyncSleepFor(guard(), 1000ms);

    auto frame = co_await m_frame->GetAsyncFrame(guard());
    if (frame) {
        auto gray = frame->CreateGray();
        Image.SaveImages(
          File.GetFileName(TimeStamp, ".png", m_exposure_value, "best"),
          gray,
          ROTATE);
    }
    std::string save_file = File.GetFileName("0000", ".csv", "exposure_log");

    SaveExposureCsv(save_file, m_exposure_data);

    co_return;
}

asio::awaitable<void>
StageAutoExposure::StartAutoExposure(async::Lifeguard guard)
{
    m_cancel = false;
    co_await InitSetup(guard());
    auto timer = ds::async::Timer();
    co_await timer.AsyncSleepFor(guard(), 1000ms);
    co_await Processing(guard());
    co_await Complete(guard());

    co_return;
}
asio::awaitable<void>
StageAutoExposure::CancelAutoExposure(async::Lifeguard guard)
{
    m_cancel = true;
    m_cond.Notify();
    co_return;
}
}