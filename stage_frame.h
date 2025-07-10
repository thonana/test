#pragma once
#include "async.h"
#include "ui.h"
#include "stage_base.h"
#include <camera.h>
#include "camera/record_filter.h"
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>
#include "stage_utility.h"
#include "stage_settings.h"

namespace ds::depthscan {

class ds::camera::Camera;
class ds::camera::RecordFilter;
struct ds::camera::Frame;


class TemplateFilter : public ds::camera::RecordFilter
{

public:
    TemplateFilter()
      : m_threshold(0.8)
    {
    }
    TemplateFilter(const cv::Mat& templateImg,
                   double threshold = 0.8)
      : m_template(templateImg)
      , m_that_gray(templateImg)
      , m_threshold(threshold)
      , m_meanValue(1.0)
      , m_first(true)
      , m_record(false)
    {
    }
    bool ShouldRecord(const ds::camera::Frame* frame) override;
    void SetFirst()
    {
        m_first = true;
        m_record = false;
        m_meanValue = 1.0;
    }

private:
    bool m_first;
    bool m_record;
   
    cv::Mat m_template;
    cv::Mat m_that_gray;
    double m_threshold;
    double m_meanValue;
};
class BrightnessFilter : public ds::camera::RecordFilter
{

public:
    BrightnessFilter()
      : m_threshold_entry(0.8)
    {
    }
    BrightnessFilter(const cv::Mat& templateImg,
                     double threshold_entry = 10.0,
                     double threshold_exit = -10.0,
                     double threshold_exit2 = 20.0)
    : m_that_gray(templateImg)
      , m_threshold_entry(threshold_entry)
      , m_threshold_exit(threshold_exit)
      , m_threshold_exit_2nd(threshold_exit2)
      , m_first(true)
      , m_record(false)
      , m_that_brigtness(0.0)
      , m_finished(false)
      , m_no_check(false)
      , m_idx(0)
    {
    }
    void SaveImages(const std::string& event_type, const cv::Mat& gray) const;
    bool ShouldRecord(const ds::camera::Frame* frame) override;
    void ManualRecord() {
        m_record = true;
        m_no_check = true;
        m_finished = false;
        m_first = false;
        m_idx = 0;
        spdlog::info("Manual Record");
    }
    void SetFirst()
    {
        m_first = true;
        m_record = false;
    }
    void SetNoCheck(bool no_check) { m_no_check = no_check; }

private:
    bool m_first;
    bool m_record;
    bool m_finished;
    bool m_no_check;

    int m_idx;

    double m_that_brigtness;
    double m_threshold_entry;
    double m_threshold_exit;
    double m_threshold_exit_2nd;
    cv::Mat m_that_gray;
};


class StageFrame : public async::Model<StageFrame>
{
public:
    StageFrame();
    ~StageFrame();

    void Initiate() noexcept override;

    std::shared_ptr<ds::camera::Camera> GetCamera() const
    { return m_camera;
    }

    asio::awaitable<std::shared_ptr<const ds::camera::Frame>> GetAsyncFrame(
      async::Lifeguard guard) const;


    void ArmRecording(std::shared_ptr<ds::camera::RecordFilter> filter,
                      const std::filesystem::path& path,
                      const std::string& gname)
    {
        m_camera->ArmRecording(filter, path, gname);
    }

    void StopRecording() { m_camera->StopRecording(); }
  
    ds::camera::CameraState GetState() const noexcept { return m_camera->GetState(); } 
    std::string GetCameraName() const noexcept
    {
        return m_camera->GetCameraName();
    }

private:
    std::shared_ptr<ds::camera::Camera> m_camera;
    std::shared_ptr<const ds::camera::Frame> m_frame;
    async::RawCondition m_cond;
    //static std::shared_ptr<ds::camera::Camera> s_mainCamera;
};

class StageLED : public async::Model<StageLED>
{
public:
    StageLED()
        :m_stage(ui::CreateAsyncModel<Stage>())
    {

    }
    ~StageLED(){};

    asio::awaitable<void> SetLEDCh(async::Lifeguard guard, uint32_t value)
    {
        co_await m_stage->SetLEDCh(guard(), value);
        co_return;
    }
    asio::awaitable<void> SetLEDPeriod(async::Lifeguard guard, uint32_t value)
    {
        co_await m_stage->SetLEDPeriod(guard(), value);
        co_return;
    }
    asio::awaitable<void> SetLEDCamOff(async::Lifeguard guard, uint32_t value)
    {
        co_await m_stage->SetLEDCamOff(guard(), value);
        co_return;
    }
    asio::awaitable<void> SetLEDOn(async::Lifeguard guard, uint32_t value)
    {
        co_await m_stage->SetLEDOn(guard(), value);
        co_return;
    }


private:
    std::shared_ptr<Stage> m_stage;
    async::RawCondition m_cond;
};
} // namespace ds