#pragma once
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <time.h>
#include <sstream>
#include <string>
#include <spdlog/spdlog.h>
#include <opencv2/opencv.hpp>

#include "stage_frame.h"

namespace fs = std::filesystem;

class ProgressCalculator
{
public:
    ProgressCalculator(int total_time)
      : m_total_time(total_time)
    {
    }
    ~ProgressCalculator() = default;

    int GetPogress(int current_time) const
    {
        double progress =
          (static_cast<double>(current_time) / m_total_time) * 100;
        return static_cast<int>(std::ceil(progress / 5.0) * 5);
    }

private:
    int m_total_time;
};

class StageFileHandle
{
public:
    explicit StageFileHandle(const std::string& folder_path)
      : m_folder_path(folder_path)
    {
        if (not fs::exists(m_folder_path)) {
            fs::create_directories(m_folder_path);
        }
        if (not fs::is_directory(m_folder_path)) {
            throw std::invalid_argument("Path '" + m_folder_path +
                                        "' is not a directory");
        }
    }
    void DeleteWholeFiles()
    {
        try {
            for (const auto& entry : fs::directory_iterator(m_folder_path)) {
                if (fs::is_regular_file(entry.path())) {
                    fs::remove(entry.path());
                }
            }
        } catch (const fs::filesystem_error& e) {
            spdlog::info("Error deleting files:{} " ,e.what());
        }
    }
    template<typename... Args>
    std::string GetFileName(const std::string& time,
                            const std::string& extension,
                            Args... args)
    {
        std::stringstream ss;
        ss << m_folder_path;
        if (not m_folder_path.empty() && m_folder_path.back() != '/' &&
            m_folder_path.back() != '\\') {
            ss << '/';
        }
        ss << time;
        ((ss << "_" << args), ...);
        ss << extension;

        return ss.str();
    }

    std::string GetPngFile()
    {
        for (const auto& entry : fs::directory_iterator(m_folder_path)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                if (ext == ".png" || ext == ".PNG") {
                    return entry.path().string(); 
                }
            }
        }
        return ""; 
    }

private:
    std::string m_folder_path;
};

class StageDateTimeFormat
{
public:
    std::string GetTime()
    {
        const auto now = std::chrono::system_clock::now();
        return std::format("{0:%Y%m%d}_{0:%H%M%S}", now);
    }
};

class StageProcessImage
{
public:
    cv::Mat RotateImage(const cv::Mat& image, double angle)
    {
        cv::Point2f center(image.cols / 2.0f, image.rows / 2.0f);
        cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, -angle, 1.0);
        double  radians         = angle * CV_PI / 180.0;
        double  sin_theta       = std::abs(std::sin(radians));
        double  cos_theta       = std::abs(std::cos(radians));

        int new_width =
          static_cast<int>(image.rows * sin_theta + image.cols * cos_theta);
        int new_height =
          static_cast<int>(image.rows * cos_theta + image.cols * sin_theta);
        rotation_matrix.at<double>(0, 2) += (new_width - image.cols) / 2.0;
        rotation_matrix.at<double>(1, 2) += (new_height - image.rows) / 2.0;

        cv::Mat rotated_image;
        cv::warpAffine(image,
                       rotated_image,
                       rotation_matrix,
                       cv::Size(new_width, new_height),
                       cv::INTER_LINEAR,
                       cv::BORDER_CONSTANT,
                       cv::Scalar(255, 255, 255));

        return rotated_image;
    }

    void SaveImages(const std::string& name, const cv::Mat& image, double angle)
    {
        if (angle != 0) {
            cv::Mat rotated_image = RotateImage(image, angle);
            cv::imwrite(
              name, rotated_image);
        } else {
            cv::imwrite(name, image);
        }
    }

    cv::Mat CropImage(auto image, auto x, auto y, auto w, auto h)
    {
        cv::Rect roi(x, y, w, h);
        return image(roi);
    }

    cv::Mat CalculateHistogram(const cv::Mat& image)
    {
        cv::Mat hist;
        int histSize = 256;
        float range[] = { 0, 256 };
        const float* histRange = { range };
        cv::calcHist(&image, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);

        return hist;
    }

    cv::Mat ImageRegions(const cv::Mat& src,
                                         int cropX,
                                         int cropY,
                                         int cropWidth,
                                         int cropHeight,
                                         int rotate)
    {
        StageProcessImage Image;

        return Image.RotateImage(
          Image.CropImage(src, cropX, cropY, cropWidth, cropHeight), rotate);
    }

    double Sharpness(const cv::Mat& image,bool is_gray)
    {
        cv::Mat gray;
        if (not is_gray) {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = image;
        }   
        cv::Mat laplacian;
        cv::Laplacian(gray, laplacian, CV_64F);
        cv::Scalar mean, stddev;
        cv::meanStdDev(laplacian, mean, stddev);

        double variance = stddev.val[0] * stddev.val[0];
        return variance;
    }
};
