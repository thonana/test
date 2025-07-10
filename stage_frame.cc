#include "stage_frame.h"

namespace ds::depthscan {
//std::shared_ptr<ds::camera::Camera> StageFrame::s_mainCamera = nullptr;
StageFrame::StageFrame()
  : m_camera(nullptr)
{
    //if (s_mainCamera == nullptr) {
    //    s_mainCamera = ds::camera::GetCamera("main");
    //}
    //m_camera = s_mainCamera;
    m_camera = ds::camera::GetCamera("main");
}
void
StageFrame::Initiate() noexcept
{
}

StageFrame::~StageFrame()
{
}

asio::awaitable<std::shared_ptr<const ds::camera::Frame>>
StageFrame::GetAsyncFrame(async::Lifeguard guard) const
{
    co_return co_await m_camera->AsyncGetFrame(guard());
}
bool
TemplateFilter::ShouldRecord(const ds::camera::Frame* frame) 
{
    cv::Mat gray = frame->CreateGray();
    cv::Mat roi = gray(cv::Rect(0, int(gray.rows / 4), gray.cols, 100));
    if (m_first) {
        m_first = false;
        m_record = false;
        m_template = roi.clone();
        m_that_gray = gray.clone();
        //spdlog::info("similarity [{}]", m_meanValue);
    }
    cv::Mat result;
    //auto start = std::chrono::high_resolution_clock::now();
    cv::matchTemplate(roi, m_template, result, cv::TM_CCOEFF_NORMED);
    m_meanValue = std::round(cv::mean(result)[0] * 100000.0) / 100000.0;
    //auto end = std::chrono::high_resolution_clock::now();
    //auto duration =
    //  std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    //spdlog::info(
    //  " duration [{}] us", m_meanValue, duration.count());
    if (m_meanValue < m_threshold)
    {
        StageProcessImage Image;
        StageDateTimeFormat Time;
        std::string TimeStamp = Time.GetTime();
        StageFileHandle File(PATH_TO_AUTOMODE);
        //spdlog::info("similarity [{}]", m_meanValue);
        Image.SaveImages(
            File.GetFileName(TimeStamp, ".png", "this", "entry"), gray, 0);
        Image.SaveImages(File.GetFileName(TimeStamp, ".png", "prev", " entry"),
                         m_that_gray,
            0);
        m_record = true;
    } 
    m_template = roi.clone();
    m_that_gray = gray.clone();
    

    return m_record;
}
void
BrightnessFilter::SaveImages(const std::string& event_type, const cv::Mat& gray) const
{
    StageProcessImage Image;
    StageDateTimeFormat Time;
    std::string TimeStamp = Time.GetTime();
    StageFileHandle File(PATH_TO_AUTOMODE);

    Image.SaveImages(
      File.GetFileName(TimeStamp, ".png", "this", event_type.c_str()), gray, 0);
    Image.SaveImages(
      File.GetFileName(TimeStamp, ".png", "prev", event_type.c_str()),
      m_that_gray,
      0);
}
bool
BrightnessFilter::ShouldRecord(const ds::camera::Frame* frame)
{
    if (m_finished)  return false;

    cv::Mat gray =
      m_record
        ? frame->CreateSubGray(100, frame->height - 100, frame->width - 100, 30)
        : frame->CreateSubGray(100, 200, frame->width - 100, 30);
 
    double currentBrightness = cv::mean(gray)[0];
    double brightnessDiff = currentBrightness - m_that_brigtness;

    if (m_first) {
        m_first = false;
        m_record = false;
        m_finished = false;
        m_no_check = false;
        m_that_brigtness = currentBrightness;
        m_that_gray = gray.clone();
        m_idx = 0;
        return false;
    } 
    m_idx++;
    //if (abs(brightnessDiff) > 2)
    //    spdlog::info(" br[{}] [{},{}] ",m_idx, int(m_that_brigtness),int(currentBrightness));
    if (m_no_check) {
        m_record = true;
    } else if (!m_record) {
        if (brightnessDiff >= m_threshold_entry) {
            SaveImages("entry_1", gray);
            m_record = true;
            m_no_check = true;
        }
    } else if (brightnessDiff <= m_threshold_exit) {
        cv::Mat gray_front = frame->CreateSubGray(
          100, 100, frame->width - 100, 30);
        double currentBrightness_front = cv::mean(gray_front)[0];
        double brightnessDiff_front =
          currentBrightness_front - currentBrightness;
        //spdlog::info(" br[{}] [{},{},{}] ",
        //             m_idx,
        //             int(m_that_brigtness),
        //             int(currentBrightness),
        //             int(currentBrightness_front));
        if (brightnessDiff_front <= m_threshold_exit_2nd) {
            m_record = false;
            m_finished = true;  
            SaveImages("exit_1", gray);
            SaveImages("exit_f", gray_front);
            spdlog::info("exit:{}_{}_{}",
                         int(m_that_brigtness),
                         int(currentBrightness),
                         int(currentBrightness_front));
        } 
   
     } 
    
    m_that_brigtness = currentBrightness;
    m_that_gray = gray.clone();

    return m_record;
}
} // namespace ds