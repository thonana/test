
#include <fstream>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>
#include <chrono>
#include "label_strings.h"
#include "stage_autofocus.h"
#include "stage_settings.h"
#include "stage_utility.h"

namespace ds::depthscan {

constexpr auto FOCUS = Method_Focus::ROI_LINE; /// ROI

static cv::Mat
CovertGaussianBlur(const cv::Mat& image, auto kernelSize = (9, 9))
{
    cv::Mat imageBlur;
    cv::GaussianBlur(image, imageBlur, kernelSize, 0);
    return imageBlur;
}
static double
calculateSlope(const std::vector<double>& x, const std::vector<double>& y)
{
    int n = x.size();
    if (n != y.size() || n == 0) {
        throw std::invalid_argument(
          "Vectors x and y must have the same non-zero length");
    }

    double sum_x = std::accumulate(x.begin(), x.end(), 0.0);
    double sum_y = std::accumulate(y.begin(), y.end(), 0.0);
    double sum_xx = std::inner_product(x.begin(), x.end(), x.begin(), 0.0);
    double sum_xy = std::inner_product(x.begin(), x.end(), y.begin(), 0.0);

    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
    return slope;
}
static std::vector<int>
find_local_minima(const std::vector<double>& values, int order)
{
    std::vector<int> local_minima_indices;

    for (int i = order; i < values.size() - order; ++i) {
        bool is_local_minima = true;

        for (int j = 1; j <= order; ++j) {
            if (values[i] >= values[i - j] || values[i] >= values[i + j]) {
                is_local_minima = false;
                break;
            }
        }

        if (is_local_minima) {
            std::vector<double> x = { static_cast<double>(i - order),
                                      static_cast<double>(i),
                                      static_cast<double>(i + order) };
            std::vector<double> y = { values[i - order],
                                      values[i],
                                      values[i + order] };
            double slope = calculateSlope(x, y);
            // spdlog::info("slope[{}] : {}", i, slope);
            if (std::abs(slope) >= 0.000001)
                local_minima_indices.push_back(i);
        }
    }

    return local_minima_indices;
}
static std::vector<int>
find_local_maxima(const std::vector<double>& values, int order)
{
    std::vector<int> local_maxima_indices;

    for (int i = order; i < values.size() - order; ++i) {

        std::vector<double> x = { static_cast<double>(i - order),
                                  static_cast<double>(i),
                                  static_cast<double>(i + order) };
        std::vector<double> y = { values[i - order],
                                  values[i],
                                  values[i + order] };
        double slope = calculateSlope(x, y);

        if (slope > 0.005) {
            // spdlog::info("slope[{}] : {}", i, slope);
            local_maxima_indices.push_back(i);
        }
    }

    return local_maxima_indices;
}

static int
CalculateCalibration(int idx, bool fine, int width)
{
    int calibration = 0;
    int center_idx = width / 2 ;

    calibration = (idx - center_idx);

    if (fine) {
        calibration = std::clamp(calibration, -50, 50);
    } else {
        calibration = std::clamp(calibration, -(width / 2), (width / 2));
    }
    return calibration;
}
static void
SaveCenteringImages(const std::string path,const cv::Mat& img,
                    int pos,
                    int num)
{
    StageDateTimeFormat Time;
    std::string TimeStamp = Time.GetTime();

    StageFileHandle File(path);
    StageProcessImage Image;

    Image.SaveImages(
      File.GetFileName(TimeStamp, ".png", pos, num, "C"), img, 0);
}

static cv::Mat
CovertGaussianBlur(const cv::Mat& image, cv::Size kernelSize = cv::Size(31, 31))
{
    cv::Mat imageBlur;
    cv::GaussianBlur(image, imageBlur, kernelSize, 5.0, 5.0, cv::BORDER_DEFAULT);
    return imageBlur;
}
static auto
FocusingImageRegions(
  const cv::Mat& src, int x, int y, int width, int height, int offset)
{
    StageProcessImage Image;

    return std::make_pair(
      Image.CropImage(src, x, y, width, height),
      Image.CropImage(src, x + offset, y + offset, width, height));
}
static void
SaveFocusingImages(const std::string path,const cv::Mat& image, int pos, int num, double angle)
{
    StageDateTimeFormat Time;
    std::string TimeStamp = Time.GetTime();

    StageFileHandle File(path);
    StageProcessImage Image;

    Image.SaveImages(
      File.GetFileName(TimeStamp, ".png", pos, num), image, angle);
}

static bool
IsFocusMethodROI(Method_Focus method)
{
    if (FOCUS == method)
        return true;
    else
        return false;
}
static bool
IsFinal(int final_num, int now_step)
{
    if (now_step >= (final_num - 1))
        return true;
    else
        return false;
}
static int
DetermineIndex(const std::vector<int>& local_indices,
               const std::vector<double>& templates,
               bool minMethod)
{
    int idx = 0;
    if (local_indices.size() >= 2) {
        std::vector<std::pair<int, double>> sorted_ma;
        for (int index : local_indices) {
            sorted_ma.emplace_back(index, templates[index]);
        }
        if (minMethod) {
            std::sort(sorted_ma.begin(),
                      sorted_ma.end(),
                      [](const std::pair<int, double>& a,
                         const std::pair<int, double>& b) {
                          return a.second < b.second;
                      });
        } else {
            std::sort(sorted_ma.begin(),
                      sorted_ma.end(),
                      [](const std::pair<int, double>& a,
                         const std::pair<int, double>& b) {
                          return a.second > b.second;
                      });
        }
        idx = sorted_ma[0].first;
        // int second_idx = sorted_ma[1].first;
        //  int decision_idx    = (min_idx + second_min_idx) / 2;

    } else {
        auto element_iter =
          std::min_element(templates.begin(), templates.end() - 1);

        if (minMethod)
            element_iter =
              std::min_element(templates.begin(), templates.end() - 1);
        else
            element_iter =
              std::max_element(templates.begin(), templates.end() - 1);

        if (element_iter != templates.end()) {
            idx = std::distance(templates.begin(), element_iter);
        }
    }
    spdlog::info("local:({})", idx);
    return idx;
}
static std::tuple<int, int, int, int>
FinalizeFocus(const std::vector<double>& templates,
              const std::vector<double>& positions,
              bool overall)
{
    auto local_minima_indices = find_local_minima(templates, 3);
    int decision_min_idx =
      DetermineIndex(local_minima_indices, templates, true);
    // decision_idx += 15; /// 15steps is external mark

    auto local_maxima_indices = find_local_maxima(templates, 2);
    int decision_max_idx =
      DetermineIndex(local_maxima_indices, templates, false);

    int decision_idx = 0;

    if (IsFocusMethodROI(Method_Focus::ROI_LINE)) {
        if (overall)
            decision_idx = decision_min_idx + 4;
        else
            // decision_idx = decision_max_idx+0;
            decision_idx = decision_min_idx + 3;
    }
    else if (IsFocusMethodROI(Method_Focus::ROI_MARKER)) {
        //if (decision_min_idx > 35)
        //    decision_idx = decision_min_idx - 35;
        decision_idx = decision_min_idx + 70;
    } else {
            decision_idx += decision_min_idx;
    }
    int decision_pos = 0;
    if (decision_idx >= positions.size()) {
        decision_pos =
          positions[positions.size() - 2] + (decision_idx - positions.size()) * 256;
        spdlog::info(
          "over:({},{},{})", decision_idx, positions.size(), decision_pos);
        decision_idx = positions.size() - 1;
    } else if (decision_idx < 0) {
        decision_idx = 0;
        decision_pos = positions[decision_idx];
    } else {
        decision_pos = positions[decision_idx];
    }

    return { decision_min_idx, decision_max_idx, decision_idx, decision_pos };
}
static void
SaveFocusCsv(const std::string& path,
             const std::vector<double>& values,
             const std::vector<double>& positions,
             int min_idx,
             int second_min_idx,
             int decision_idx,
             int decision_pos,
             int length)
{
    std::ofstream file(path);

    if (not file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    file << "Index,Position,Value\n";

    for (size_t i = 0; i < length; i++) {
        file << i << "," << positions[i] << "," << values[i] << "\n";
    }
    file << "First:" << min_idx << ",Second:" << second_min_idx
         << ",Decision:" << decision_idx << "," << decision_pos << "\n";
    file.close();
}
static void
SaveFocusLog(const std::string& path,
             int num,
             bool bubble,
             int min_idx,
             int max_idx,
             int x_pos,
             int y_pos)
{
    std::ofstream file(path, std::ios::app);

    if (not file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    file << num << "," << bubble << "," << min_idx << "," << max_idx << ","
         << x_pos << "," << y_pos << "\n";
    file.close();
}
static std::tuple<cv::Mat, int>
DetectVerticalLines(const cv::Mat& image, const int width)
{
    cv::Mat vertical_kernel =
      cv::getStructuringElement(cv::MORPH_RECT, cv::Size(20, 30));
    cv::Mat vertical_lines;
    cv::morphologyEx(image, vertical_lines, cv::MORPH_OPEN, vertical_kernel);

    cv::Mat vertical_edge;
    cv::Canny(vertical_lines, vertical_edge, 50, 100, 3, false);
    std::vector<cv::Vec4i> lines;

    // cv::HoughLinesP(vertical_edge, lines, 1, CV_PI / 180, 50, 100, 10);
    cv::HoughLinesP(vertical_edge, lines, 1, CV_PI / 180, 100, 150, 10);
    cv::Mat output;
    cv::cvtColor(image, output, cv::COLOR_GRAY2BGR);

    int line_color_size = 8;

    std::vector<int> y_coords;
    if (not lines.empty()) {
        for (const auto& line : lines) {
            if (std::abs(line[0] - line[2]) < 10) {
                cv::line(output,
                         cv::Point(line[0], line[1]),
                         cv::Point(line[2], line[3]),
                         cv::Scalar(0, 0, 255), /* red */
                         line_color_size);
                y_coords.push_back((line[0] + line[2]) / 2);
            }
        }
        spdlog::info("edge coords: ({},{})", lines.size(), y_coords.size());
        std::sort(y_coords.begin(), y_coords.end());
        std::vector<std::vector<int>> clusters;
        std::vector<int> current_cluster;
        for (int x : y_coords) {
            if (current_cluster.empty()) {
                current_cluster.push_back(x);
            } else {
                if (x - current_cluster.back() < 50) {
                    current_cluster.push_back(x);
                } else {
                    clusters.push_back(current_cluster);
                    current_cluster.clear();
                    current_cluster.push_back(x);
                }
            }
        }
        if (not current_cluster.empty()) {
            clusters.push_back(current_cluster);
        }

        std::vector<int> edges;
        for (const auto& cluster : clusters) {
            int _edge = cluster[cluster.size() / 2]; // take the middle value
            edges.push_back(_edge);
        }
        for (const auto& edge : edges) {
            cv::line(output,
                     cv::Point(edge, 0),
                     cv::Point(edge, image.rows),
                     cv::Scalar(255, 0, 0), /* blue */
                     line_color_size);
        }
        spdlog::info("edge: {}", edges.size());
        std::sort(edges.begin(), edges.end());
        int left_edge = -1;
        int right_edge = -1;
        if (edges.size() >= 2) {
            left_edge = edges.front();
            right_edge = edges.back();
            if ((right_edge - left_edge) < 10) {
                right_edge = -1;
            }
        } else if (edges.size() == 1) {
            int edge = edges[0];
            if (edge < width / 2) {
                left_edge = edge;
            } else {
                right_edge = edge;
            }
        }
        spdlog::info("left_edge: {}, right_edge: {}, size: {}, width: {}",
                     left_edge,
                     right_edge,
                     y_coords.size(),
                       width);
        std::vector<int> decision_lines;
        if (left_edge != -1) {
            cv::line(output,
                     cv::Point(left_edge, 0),
                     cv::Point(left_edge, image.rows),
                     cv::Scalar(0, 255, 0), /* green */
                     line_color_size);
            decision_lines.push_back(left_edge);
        }
        if (right_edge != -1) {
            cv::line(output,
                     cv::Point(right_edge, 0),
                     cv::Point(right_edge, image.rows),
                     cv::Scalar(0, 255, 0), /* green */
                     line_color_size);
            decision_lines.push_back(right_edge);
        }
        int center = 0;
        int center_idx = width / 2;
        int edge_idx = center_idx;
        if (decision_lines.size() >= 2) {
            if (abs(decision_lines[0] - decision_lines[1]) < (CHANNEL_WIDTH / 2))
            {
                if (decision_lines[0] < center_idx) {
                    edge_idx -= CHANNEL_WIDTH / 2;
                    if (edge_idx < decision_lines[0])
                        center = width / 2 - (decision_lines[0] - edge_idx);
                    else
                        center = width / 2;
                } else {
                    edge_idx += CHANNEL_WIDTH / 2;
                    if (edge_idx > decision_lines[0])
                        center = width / 2 + (edge_idx - decision_lines[0]);
                    else
                        center = width / 2;
                }
            } else {
                center = (decision_lines[1] + decision_lines[0]) / 2;
                if (center > center_idx)
                    center = center_idx - (center - center_idx);
                else
                    center = center_idx + (center_idx - center);
            }

        } else if (decision_lines.size() == 1) {


            if (right_edge != -1) {
                edge_idx += CHANNEL_WIDTH / 2;
                if (edge_idx > decision_lines[0])
                    center = width / 2 + (edge_idx - decision_lines[0]);
                else
                    center = width / 2;

            } else {
                edge_idx -= CHANNEL_WIDTH / 2;
                if (edge_idx < decision_lines[0])
                    center = width / 2 - (decision_lines[0] - edge_idx);
                else
                    center = width / 2;
            }
        } else {
            spdlog::info(" no edge detected");
            center = width / 2;
        }
        spdlog::info("center : {} ", center);
        return { output, center };
    } else {
        spdlog::info(" no line detected");
        return { output, width / 2 }; // no changed.
    }
}
StageAutoFocus::StageAutoFocus()
  : m_cancel(false)
  , m_frame(ui::CreateAsyncModel<StageFrame>())
  , m_move(ui::CreateAsyncModel<StageMove>())
  , m_pump(ui::CreateAsyncModel<StagePump>())
  , m_automode(ui::CreateAsyncModel<StageAutoMode>())
  , m_state(StageDSState::focus_idle)
  , m_center_idx(0)
  , m_need_water(false)
  , m_min_idx(0)
  , m_max_idx(0)
  , m_overall_focusing(false)
  , m_stop(false)
  , m_need_focusing(false)
  , m_ok_user_water(false)
{

    auto storage = StageSettingStorage::GetInstance();
    if (storage) {
        m_step =
          std::get<int>(storage->GetSettings(StageConfigKeys::FOCUS_STEP));
        m_total_steps =
          std::get<int>(storage->GetSettings(StageConfigKeys::FOCUS_NUMOFSTEP));
        m_start_pos = m_step * int(m_total_steps / 2);
        m_init_x_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::INIT_X_POS));

    }
}
StageAutoFocus::~StageAutoFocus() {}

void
StageAutoFocus::Initiate() noexcept
{
}


asio::awaitable<void>
StageAutoFocus::InitSetup(async::Lifeguard guard,
                          bool fine,
                          std::string path) ///< ready
{
    m_send_progress = 0;
    m_send_label = LabelStrings::Waiting;
    SetState(StageDSState::focus_busy);

    co_await m_move->GetNotBusy(guard());
    m_send_label = LabelStrings::Start;
    m_num_focus = 0;
    m_need_water = false;
    if (not fine) {
        StageFileHandle File(path);
        File.DeleteWholeFiles();

        int x_pos=0;
        int y_pos=0;
        auto storage = StageSettingStorage::GetInstance();
        if (storage) {
            x_pos = std::get<int>(storage->GetSettings("last_x_pos"));
            y_pos = std::get<int>(storage->GetSettings("last_y_pos"));
        }
        co_await m_move->MoveLastPos(guard(), x_pos, y_pos);
    }

    co_return;
}

asio::awaitable<void>
StageAutoFocus::CenterPosition(async::Lifeguard guard,
                               bool fine,
                               std::string path) // Get frame and processing the frame
{
    bool done = false;
    StageProcessImage Image;

    auto timer = ds::async::Timer();

    while (not done and not m_cancel) {
        auto frame = co_await m_frame->GetAsyncFrame(guard());
        if (frame) {
            m_send_label = LabelStrings::Checking;
            // 1. Image pre-process
            const int height = frame->height;
            const int width = frame->width;
            // gray

            cv::Mat src = frame->CreateGray();
            // 2. ROI & Crop
            const int roi_start_x = 0;
            const int roi_start_y = 0;
            const int roi_width = width ;
            const int roi_height = height;
            spdlog::info("w,h=({},{})", width, height);
            cv::Mat source = Image.ImageRegions(
              src, roi_start_x, roi_start_y, roi_width, roi_height, ROTATE);

            cv::Mat binary;
            cv::threshold(source, binary, 100, 255, cv::THRESH_BINARY_INV);
            SaveCenteringImages(path, binary, 0, 0);
            // 3. Vertical line detect
            auto [img_centering, center_idx] =
              DetectVerticalLines(source, width);

            // 4. compute the calibration
            int calibration = CalculateCalibration(center_idx, fine, width);

            m_center_idx = center_idx - calibration;

            // save images
            int y_pos = co_await m_move->GetPos(guard(), MotorRole::mtr_y);
            int x_pos = co_await m_move->GetPos(guard(), MotorRole::mtr_x);
            spdlog::info("x,y=({},{})", x_pos, y_pos);
            SaveCenteringImages(path,img_centering, x_pos, m_num_focus);

            // 5. y centering
            if (calibration != 0) {
                y_pos += (calibration * MICRO_STEP_MUILPLIER);
                co_await m_move->Move(guard(),
                                      MotorRole::mtr_y,
                                      SingleMode::mode_cnt,
                                      200 * MICRO_STEP,
                                      y_pos);
                m_move->SetLastPos(MotorRole::mtr_y, y_pos);
                spdlog::info("offset,y=({},{})", calibration, y_pos);
            }
            // 5-1. Detect the line thickness
            if ((not fine) and IsFocusMethodROI(Method_Focus::ROI_LINE)) {

                cv::Mat line_binary = Image.ImageRegions(
                  binary,
                  m_center_idx - (CHANNEL_WIDTH / 2) - LINE_ROI_OFFSET,
                  height / 2,
                  LINE_ROI_OFFSET * 2,
                  200,
                  ROTATE);                

                // 5-2. 각 열의 픽셀 합산 (히스토그램)
                std::vector<int> vertical_hist(line_binary.cols, 0);
                for (int col = 0; col < line_binary.cols; ++col) {
                    for (int row = 0; row < line_binary.rows; ++row) {
                        if (line_binary.at<uchar>(row, col) > 0)
                            vertical_hist[col]++;
                    }
                }
                SaveCenteringImages(path, line_binary, 0, 0);
                // 5-3. 라인 두께 측정 (임계값 이상인 연속 구간의 길이)
                int threshold = static_cast<int>(line_binary.rows *
                                   0.5); // 전체 픽셀의 50% 이상을 라인으로 간주
                int start = -1, end = -1;
                for (int col = 0; col < line_binary.cols; ++col) {
                    if (vertical_hist[col] > threshold) {
                        if (start == -1)
                            start = col;
                        end = col;
                    }
                }
                if (start != -1 && end != -1) {
                    int thickness = end - start + 1;
                    spdlog::info("thickness: {}", thickness);
                    if (thickness < 15*2) {
                        m_need_water = false;
                    } else {
                        m_need_water = true;
                        m_send_label = LabelStrings::FillWithWater;
                    }
                } else {
                    spdlog::info("thickness is not found" );
                    m_need_water = false;
                }
            }

            // 6. x move
            auto storage = StageSettingStorage::GetInstance();
            if (storage) {
                m_init_x_pos = std::get<int>(
                  storage->GetSettings(StageConfigKeys::INIT_X_POS));
            }
            if (m_overall_focusing)
                m_pos = BIG_FOCUS_START_POS;
            else
                m_pos = m_init_x_pos - m_start_pos;

            if (fine) {
                // finish the fine auto-focus mode.
                m_move->SetLastPos(MotorRole::mtr_x, x_pos);
                m_move->SetLastPos(MotorRole::mtr_y, y_pos);
                m_send_label = LabelStrings::Completed;
            } else {
                co_await m_move->GetNotBusy(guard());
                if (IsFocusMethodROI(Method_Focus::ROI_MARKER) ||
                    IsFocusMethodROI(Method_Focus::ROI_EXTERNAL)) {    
                    co_await m_move->MoveInitPos(guard());
                    co_await m_move->GetNotBusy(guard());
                }
                co_await m_move->Move(guard(),
                                      MotorRole::mtr_x,
                                      SingleMode::mode_cnt,
                                      200 * MICRO_STEP,
                                      m_pos);
            }
            if (storage) {
                storage->SaveSettingToJson(StageConfigKeys::LAST_Y_POS, y_pos);
            }
            done = true;
        }
        co_await timer.AsyncSleepFor(guard(), 1ms);
        // co_return;
    }
    co_return;
}

asio::awaitable<void>
StageAutoFocus::SearchFlow(async::Lifeguard guard)
{
    StageProcessImage Image;

    cv::Mat gray;
    cv::Mat that_gray;

    bool done = false;
    float clean_speed = 2.0f;
    bool first_image = true;
    auto storage = StageSettingStorage::GetInstance();
    if (storage) {
        clean_speed =
          std::get<float>(storage->GetSettings(StageConfigKeys::CLEAN_SPEED));
    }
    auto timer = ds::async::Timer();
    co_await m_pump->StartPump(guard(), MotorDir::pump_dir_clean, clean_speed);

    m_templates.clear();
    while (not done and not m_cancel) {
        auto frame = co_await m_frame->GetAsyncFrame(guard());
        if (frame) {
            // 1. Preprocessing
            cv::Mat src = frame->CreateGray();
            // 2. ROI & Crop
            gray = Image.ImageRegions(
              src, 0, 0, frame->width, frame->height, ROTATE);
            if (first_image) {
                first_image = false;
                that_gray = gray;
            }
            // 3. Template matching
            cv::Mat result;
            cv::matchTemplate(gray, that_gray, result, cv::TM_CCOEFF_NORMED);
            double meanValue =
              std::round(cv::mean(result)[0] * 100000.0) / 100000.0;

            //spdlog::info("similarity [{}]", meanValue);
            m_templates.push_back(meanValue);

            if (meanValue < FOCUS_WATER_ENTRY_TH) {

                co_await m_pump->StopPump(guard());

                done = true;
            }
            that_gray = gray;
        }
        co_await timer.AsyncSleepFor(guard(), 1ms);
    }
    co_return;
}

asio::awaitable<void>
StageAutoFocus::PumpUntilFlow(async::Lifeguard guard)
{
    StageProcessImage Image;

    cv::Mat gray;
    cv::Mat that_gray;

    bool done = false;
    int clean_speed = 2;
    bool first_image = true;
    auto storage = StageSettingStorage::GetInstance();
    if (storage) {
        clean_speed = std::get<float>(storage->GetSettings("clean_speed"));
    }
    auto timer = ds::async::Timer();
    co_await m_pump->StartPump(guard(), MotorDir::pump_dir_clean, clean_speed);
    m_send_label = LabelStrings::DrainTheWater;
    m_templates.clear();
    while (not done) {
        auto frame = co_await m_frame->GetAsyncFrame(guard());
        if (frame) {
            // 1. Preprocessing
            cv::Mat src = frame->CreateGray();
            // 2. ROI & Crop
            gray = Image.ImageRegions(
              src, 0, 0, frame->width, frame->height, ROTATE);
            if (first_image) {
                first_image = false;
                that_gray = gray;
            }
            // 3. Template matching
            cv::Mat result;
            cv::matchTemplate(gray, that_gray, result, cv::TM_CCOEFF_NORMED);
            double meanValue =
              std::round(cv::mean(result)[0] * 100000.0) / 100000.0;

            //spdlog::info("similarity [{}]", meanValue);
            m_templates.push_back(meanValue);

            if (meanValue < FOCUS_WATER_EXIT_TH) {

                co_await m_pump->StopPump(guard());

                done = true;
            }
            that_gray = gray;
        }
        co_await timer.AsyncSleepFor(guard(), 1ms);
    }
    co_return;
}

asio::awaitable<void>
StageAutoFocus::Focusing(async::Lifeguard guard, std::string path)
{
    m_templates.resize(m_total_steps, 0.0); //< Do not use vector.clear()
    m_positions.resize(m_total_steps, 0.0); //< Do not use vector.clear()

    StageDateTimeFormat Time;
    std::string TimeStamp = Time.GetTime();
    StageFileHandle File(path);


    auto timer = ds::async::Timer();
    ProgressCalculator progress(m_total_steps);

    while (not IsFinal(m_total_steps, m_num_focus) and not m_cancel) {
        auto frame = co_await m_frame->GetAsyncFrame(guard());
        if (frame) {
            m_send_progress = progress.GetPogress(m_num_focus);
            if (m_send_progress == 100)
                m_send_progress = 99;
            m_send_label = LabelStrings::Focusing;
            // 1. Image pre-process
            const int height = frame->height;
            const int width = frame->width;
            // gray
            cv::Mat src = frame->CreateGray();
            // blur
            //cv::Mat blurred = CovertGaussianBlur(src, cv::Size(91, 91));
            //cv::Mat blurred = src;

            // 2. Crop
            cv::Mat roi_source;
            cv::Mat tmplate;
            if (IsFocusMethodROI(Method_Focus::ROI_CHANNEL)) {
                std::tie(roi_source, tmplate) = FocusingImageRegions(
                  src, 0, height / 2 - 700, width - 8, 1400, 8); //
            } else if (IsFocusMethodROI(Method_Focus::ROI_LINE)) {
                std::tie(roi_source, tmplate) = FocusingImageRegions(
                  src,
                  m_center_idx - (CHANNEL_WIDTH / 2) - LINE_ROI_OFFSET,
                                       height / 2,
                  LINE_ROI_OFFSET*2,
                                       200,
                                       8);
            } else if (IsFocusMethodROI(Method_Focus::ROI_MARKER)) {
                std::tie(roi_source, tmplate) =
                  FocusingImageRegions(src, 0, height - 410, 800, 400, 8);
                // std::tie(roi_source, tmplate) = FocusingImageRegions(blurred,
                //                        width / 2 + width / 5 + 300,
                //                        height / 2 + height / 4 - 150,
                //                        200,
                //                        200,
                //                        8);
            } else if (IsFocusMethodROI(Method_Focus::ROI_EXTERNAL)) {
                std::tie(roi_source, tmplate) =
                  FocusingImageRegions(src, 1000, 500, 200, 500, 8);
            } else {
                std::tie(roi_source, tmplate) =
                  FocusingImageRegions(src, 0, 0, width - 8, height - 8, 8);
            }
            cv::Mat blurred_src =
              CovertGaussianBlur(roi_source, cv::Size(31, 31));
            cv::Mat blurred_tmpl =
              CovertGaussianBlur(tmplate, cv::Size(31, 31));
            // 3. Save images
            cv::Rect save_roi(width / 4, 0, width / 2, height);
            SaveFocusingImages(path, src, m_pos, m_num_focus, ROTATE);
            //SaveFocusingImages(path, source, m_pos, m_num_focus, ROTATE);

            // 4. Template matching
            cv::Mat result;
            cv::matchTemplate(
              blurred_src, blurred_tmpl, result, cv::TM_CCOEFF_NORMED);
            double meanValue =
              std::round(cv::mean(result)[0] * 100000.0) / 100000.0;

            // save the template values
            m_templates[m_num_focus] = meanValue;
            m_positions[m_num_focus] = m_pos;

            m_pos += m_step;
            m_num_focus++;    
            // move next
            co_await m_move->Move(guard(),
                                    MotorRole::mtr_x,
                                    SingleMode::mode_cnt,
                                    200 * MICRO_STEP,
                                    m_pos);
            co_await m_move->GetNotBusy(guard());
        }
        co_await timer.AsyncSleepFor(guard(), 1ms);
        // co_return;
    }
    if (not m_cancel) {
        auto [min_idx, max_idx, decision_idx, decision_pos] =
          FinalizeFocus(m_templates, m_positions,false);

        m_min_idx = min_idx;
        m_max_idx = max_idx;

        m_move->SetLastPos(MotorRole::mtr_x, decision_pos);

        co_await m_move->Move(guard(),
                              MotorRole::mtr_x,
                              SingleMode::mode_cnt,
                              200 * MICRO_STEP,
                              decision_pos);
        co_await m_move->GetNotBusy(guard());

        std::string save_file;

        save_file = File.GetFileName(TimeStamp, ".csv", "focus");
        
        m_num_focus = 0;
        SaveFocusCsv(save_file,
                     m_templates,
                     m_positions,
                     min_idx,
                     max_idx,
                     decision_idx,
                     decision_pos,
                     m_total_steps - 1);
    }
    co_return;
}

asio::awaitable<void>
StageAutoFocus::OverallFocusing(async::Lifeguard guard, std::string path)
{
    const int final_num = BIG_FOCUS_TOTAL_STEPS;
    const int step_size = BIG_FOCUS_STEP;

    m_templates.resize(final_num, 0.0); //< Do not use vector.clear()
    m_positions.resize(final_num, 0.0); //< Do not use vector.clear()

    StageDateTimeFormat Time;
    std::string TimeStamp = Time.GetTime();
    StageFileHandle File(path);

    auto timer = ds::async::Timer();

    while (not IsFinal(final_num, m_num_focus) and not m_cancel) {
        auto frame = co_await m_frame->GetAsyncFrame(guard());
        if (frame) {
            m_send_label = LabelStrings::Focusing;
            // 1. Image pre-process
            const int height = frame->height;
            const int width = frame->width;
            // gray
            cv::Mat src = frame->CreateGray();
            // blur
            // cv::Mat blurred = CovertGaussianBlur(src);
            //cv::Mat blurred = src;

            // 2. Crop
            cv::Mat roi_source;
            cv::Mat tmplate;
            if (IsFocusMethodROI(Method_Focus::ROI_CHANNEL)) {
                std::tie(roi_source, tmplate) = FocusingImageRegions(
                  src, 0, height / 2 - 700, width - 8, 1400, 8); //
            } else if (IsFocusMethodROI(Method_Focus::ROI_LINE)) {
                std::tie(roi_source, tmplate) = FocusingImageRegions(
                  src,
                  m_center_idx - (CHANNEL_WIDTH / 2) - LINE_ROI_OFFSET,
                                       height / 2,
                  LINE_ROI_OFFSET*2,
                                       200,
                                       8);
            } else if (IsFocusMethodROI(Method_Focus::ROI_MARKER)) {
                std::tie(roi_source, tmplate) =
                  FocusingImageRegions(src, 0, height - 410, 800, 400, 8);
                // std::tie(roi_source, tmplate) = FocusingImageRegions(blurred,
                //                        width / 2 + width / 5 + 300,
                //                        height / 2 + height / 4 - 150,
                //                        200,
                //                        200,
                //                        8);
            } else if (IsFocusMethodROI(Method_Focus::ROI_EXTERNAL)) {
                std::tie(roi_source, tmplate) =
                  FocusingImageRegions(src, 1000, 500, 200, 500, 8);
            } else {
                std::tie(roi_source, tmplate) =
                  FocusingImageRegions(src, 0, 0, width - 8, height - 8, 8);
            }

            // 3. Save images
            if (m_num_focus == 0) {
                SaveFocusingImages(
                  path, roi_source, m_pos, m_num_focus, ROTATE);
            }
            SaveFocusingImages(path, roi_source, m_pos, m_num_focus, ROTATE);

            cv::Mat blurred_src =
              CovertGaussianBlur(roi_source, cv::Size(31, 31));
            cv::Mat blurred_tmpl =
              CovertGaussianBlur(tmplate, cv::Size(31, 31));
            // 4. Template matching
            cv::Mat result;
            cv::matchTemplate(
              blurred_src, blurred_tmpl, result, cv::TM_CCOEFF_NORMED);
            double meanValue =
              std::round(cv::mean(result)[0] * 100000.0) / 100000.0;

            // save the template values
            m_templates[m_num_focus] = meanValue;
            m_positions[m_num_focus] = m_pos;

            m_pos += step_size;
            m_num_focus++;
            co_await m_move->Move(guard(),
                                  MotorRole::mtr_x,
                                  SingleMode::mode_cnt,
                                  200 * MICRO_STEP,
                                  m_pos);

        }
        co_await timer.AsyncSleepFor(guard(), 1ms);
        // co_return;
    }
    if (not m_cancel) {
        auto [min_idx, max_idx, decision_idx, decision_pos] =
          FinalizeFocus(m_templates, m_positions,true);

        m_init_x_pos = decision_pos;
        auto storage = StageSettingStorage::GetInstance();
        if (storage) {
            storage->SaveSettingToJson(StageConfigKeys::INIT_X_POS,
                                       m_init_x_pos);
            storage->SaveSettingToJson(StageConfigKeys::LAST_X_POS,
                                       m_init_x_pos);
        }

        spdlog::info("init_x_pos: {}", m_init_x_pos);

        m_move->SetLastPos(MotorRole::mtr_x, decision_pos);

        co_await m_move->Move(guard(),
                              MotorRole::mtr_x,
                              SingleMode::mode_cnt,
                              200 * MICRO_STEP,
                              decision_pos);
        co_await m_move->GetNotBusy(guard());

        std::string save_file;

        save_file = File.GetFileName(TimeStamp, ".csv", "focus");

        m_num_focus = 0;
        SaveFocusCsv(save_file,
                     m_templates,
                     m_positions,
                     min_idx,
                     max_idx,
                     decision_idx,
                     decision_pos,
                     final_num - 1);
    }
    co_return;
}
asio::awaitable<void>
StageAutoFocus::SaveLog(async::Lifeguard guard, std::string path)
{
    bool done = false;
    StageProcessImage Image;

    while (not done and not m_cancel) {
        auto frame = co_await m_frame->GetAsyncFrame(guard());
        if (frame) {
            const int height = frame->height;
            const int width = frame->width;

            cv::Mat src = frame->CreateGray();

            cv::Mat source = Image.ImageRegions(
              src, 0, 0, width, height, ROTATE);

            int x_focus_pos = m_move->GetLastPos(MotorRole::mtr_x);
            int y_focus_pos = m_move->GetLastPos(MotorRole::mtr_y);

            auto storage = StageSettingStorage::GetInstance();
            if (storage) {
                storage->SaveSettingToJson(StageConfigKeys::LAST_X_POS,
                                           x_focus_pos);
                storage->SaveSettingToJson(StageConfigKeys::LAST_Y_POS,
                                           y_focus_pos);
            }

            SaveFocusingImages(path, source, x_focus_pos, 999, 0);
            std::string last_img_path = PATH_TO_FOCUS + "/last";
            StageFileHandle Last_File(last_img_path);
            Last_File.DeleteWholeFiles();
            SaveFocusingImages(last_img_path, source, x_focus_pos, 999, 0);

            static int count = 0;

            std::string save_path = PATH_TO_HISTORY + "/focus";
            StageFileHandle File(save_path);
            std::string save_file =
              File.GetFileName("0000", ".csv", "focus_log");
            if ((m_max_idx < m_positions.size()) and
                (m_min_idx < m_positions.size())) {

                SaveFocusLog(save_file,
                             ++count,
                             m_need_water,
                             m_positions[m_min_idx],
                             m_positions[m_max_idx],
                             x_focus_pos,
                             y_focus_pos);
            } else {
                SaveFocusLog(
                  save_file, ++count, 0, 0, 0, x_focus_pos, y_focus_pos);
            }
            m_num_focus = 0;
            done = true;
        }
    }
    co_return;
}

asio::awaitable<void>
StageAutoFocus::StartAutoFocus(async::Lifeguard guard, bool fine)
{   
    auto storage = StageSettingStorage::GetInstance();
    if (storage) {
        m_step =
          std::get<int>(storage->GetSettings(StageConfigKeys::FOCUS_STEP));
        m_total_steps =
          std::get<int>(storage->GetSettings(StageConfigKeys::FOCUS_NUMOFSTEP));
        m_start_pos = m_step * int(m_total_steps / 2);
        m_init_x_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::INIT_X_POS));
    }
    auto timer = ds::async::Timer();
    m_ok_user_water = false;
    m_cancel = false;

    m_overall_focusing = false;
    std::string path = PATH_TO_FOCUS + "/regular";

    co_await InitSetup(guard(), fine, path);
    co_await m_move->GetNotBusy(guard());
    co_await CenterPosition(guard(), fine, path);
    co_await m_move->GetNotBusy(guard());

    if (IsFocusMethodROI(Method_Focus::ROI_LINE) and (m_need_water)) {
        while (not m_ok_user_water and not m_cancel) {
            co_await timer.AsyncSleepFor(guard(), 500ms);
        }        
        co_await SearchFlow(guard());
    }
    m_ok_user_water = false;
    co_await Focusing(guard(), path);
    co_await m_move->GetNotBusy(guard());

    if (IsFocusMethodROI(Method_Focus::ROI_MARKER) ||
        IsFocusMethodROI(Method_Focus::ROI_EXTERNAL)) {
        co_await m_move->MoveLastPos(guard(),
                                     m_move->GetLastPos(MotorRole::mtr_x),
                                     m_move->GetLastPos(MotorRole::mtr_y));
    }
    co_await CenterPosition(guard(), true, path);
    co_await m_move->GetNotBusy(guard());
    if (not m_cancel)
        co_await SaveLog(guard(), path);
    if (IsFocusMethodROI(Method_Focus::ROI_LINE) and (m_need_water) and
        (not m_cancel))
        co_await PumpUntilFlow(guard());
    m_send_progress = 100;
    co_await timer.AsyncSleepFor(guard(), 200ms);
    m_automode->SetNeedRefocus(false);
    SetState(StageDSState::focus_idle);
    co_return;
}
asio::awaitable<void>
StageAutoFocus::StartOverAllAutoFocus(async::Lifeguard guard)
{
    auto storage = StageSettingStorage::GetInstance();
    if (storage) {
        m_step =
          std::get<int>(storage->GetSettings(StageConfigKeys::FOCUS_STEP));
        m_total_steps =
          std::get<int>(storage->GetSettings(StageConfigKeys::FOCUS_NUMOFSTEP));
        m_start_pos = m_step * int(m_total_steps / 2);
        m_init_x_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::INIT_X_POS));
    }
    auto timer = ds::async::Timer();
    m_cancel = false;
    m_overall_focusing = true;

    std::string path = PATH_TO_FOCUS + "/overall";
    co_await InitSetup(guard(), false, path);
    co_await m_move->GetNotBusy(guard());
    co_await CenterPosition(guard(), false, path);
    co_await m_move->GetNotBusy(guard());
    co_await OverallFocusing(guard(), path);
    co_await m_move->GetNotBusy(guard());

    // temporary because the overall focusing is not finished yet.
    m_send_progress = 100;
    co_await timer.AsyncSleepFor(guard(), 200ms);
    m_automode->SetNeedRefocus(false);
    SetState(StageDSState::focus_idle);
    //co_await StartAutoFocus(guard(), false);

    co_return;
}
asio::awaitable<void>
StageAutoFocus::CancelAutoFocus(async::Lifeguard guard)
{
    m_cancel = true;
    m_cond.Notify();
    co_await m_move->StopMove(guard(), MotorRole::mtr_x);
    co_await m_move->StopMove(guard(), MotorRole::mtr_y);
    co_await m_move->GetNotBusy(guard());
    co_await m_move->MoveLastPos(guard(),
                                 m_move->GetLastPos(MotorRole::mtr_x),
                                 m_move->GetLastPos(MotorRole::mtr_y));
    co_await m_move->GetNotBusy(guard());
    co_await m_pump->StopPump(guard());

    SetState(StageDSState::focus_idle);
}
void
StageAutoFocus::SetState(uint32_t state)
{
    if (StageDSState::focus_busy == state)
        m_state &= ~(StageDSState::idle | StageDSState::focus_idle);
    if (StageDSState::focus_idle == state)
        m_state &= ~(StageDSState::idle | StageDSState::focus_busy);

    m_state |= state;
}
bool
StageAutoFocus::IsState(uint32_t state) const
{
    if (m_state & state)
        return true;
    else
        return false;
}

}
