#pragma once

#include <memory>
#include <opencv2/core.hpp>
#include <wx/bitmap.h>

namespace ds {

enum class ImageFormat
{
    INVALID,
    GRAY,
    RGB,
    BayerBG,
    BayerGB,
    BayerGR,
    BayerRG
};

class Image
{
    ImageFormat m_format;
    cv::Mat     m_mat;

public:
    Image();
    ~Image();

    Image(int width, int height, ImageFormat format);
    Image(const Image& image, const cv::Rect& roi);
    Image(const Image& image, ImageFormat format);

    cv::Mat& Create(int width, int height, ImageFormat format);
    cv::Mat& Create(const Image& image, const cv::Rect& roi);
    cv::Mat& Create(const Image& image, ImageFormat format);

    wxBitmap CreateBitmap() const;

    const int         GetWidth() const { return m_mat.cols; }
    const int         GetHeight() const { return m_mat.rows; }
    const ImageFormat GetFormat() const { return m_format; }
    const cv::Mat&    GetMat() const { return m_mat; }
};

} // namespace ds
