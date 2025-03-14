#include "ds/image.h"
#include <opencv2/imgproc.hpp>

namespace ds {

Image::Image()
  : m_format(ImageFormat::INVALID)
  , m_mat()
{
}

Image::~Image() = default;

Image::Image(int width, int height, ImageFormat format)
  : Image()
{
    Create(width, height, format);
}

Image::Image(const Image& image, const cv::Rect& roi)
  : Image()
{
    Create(image, roi);
}

Image::Image(const Image& image, ImageFormat format)
  : Image()
{
    Create(image, format);
}

cv::Mat&
Image::Create(int width, int height, ImageFormat format)
{
    m_format = format;
    if (format == ImageFormat::RGB) {
        m_mat.create(height, width, CV_8UC3);
    } else {
        m_mat.create(height, width, CV_8UC1);
    }
    return m_mat;
}

cv::Mat&
Image::Create(const Image& image, const cv::Rect& roi)
{
    m_format = image.GetFormat();
    m_mat    = cv::Mat(image.GetMat(), roi);
    return m_mat;
}

cv::Mat&
Image::Create(const Image& image, ImageFormat format)
{
    if (format != ImageFormat::INVALID && format == image.GetFormat()) {
        m_format = format;

        auto& src = image.GetMat();
        if (src.isContinuous()) {
            m_mat = src;
        } else {
            m_mat = image.GetMat().clone();
        }

    } else if (format == ImageFormat::GRAY) {
        switch (image.GetFormat()) {
            case ImageFormat::RGB: {
                m_format = format;
                cv::cvtColor(image.GetMat(), m_mat, cv::COLOR_RGB2GRAY);
                break;
            }
            case ImageFormat::BayerBG: {
                m_format = format;
                cv::cvtColor(image.GetMat(), m_mat, cv::COLOR_BayerBG2GRAY);
                break;
            }
            case ImageFormat::BayerGB: {
                m_format = format;
                cv::cvtColor(image.GetMat(), m_mat, cv::COLOR_BayerGB2GRAY);
                break;
            }
            case ImageFormat::BayerGR: {
                m_format = format;
                cv::cvtColor(image.GetMat(), m_mat, cv::COLOR_BayerGR2GRAY);
                break;
            }
            case ImageFormat::BayerRG: {
                m_format = format;
                cv::cvtColor(image.GetMat(), m_mat, cv::COLOR_BayerRG2GRAY);
                break;
            }
            default: {
                // TODO: invalid conversion
            }
        }
    } else if (format == ImageFormat::RGB) {
        switch (image.GetFormat()) {
            case ImageFormat::GRAY: {
                m_format = format;
                cv::cvtColor(image.GetMat(), m_mat, cv::COLOR_GRAY2RGB);
                break;
            }
            case ImageFormat::BayerBG: {
                m_format = format;
                cv::cvtColor(image.GetMat(), m_mat, cv::COLOR_BayerBG2RGB);
                break;
            }
            case ImageFormat::BayerGB: {
                m_format = format;
                cv::cvtColor(image.GetMat(), m_mat, cv::COLOR_BayerGB2RGB);
                break;
            }
            case ImageFormat::BayerGR: {
                m_format = format;
                cv::cvtColor(image.GetMat(), m_mat, cv::COLOR_BayerGR2RGB);
                break;
            }
            case ImageFormat::BayerRG: {
                m_format = format;
                cv::cvtColor(image.GetMat(), m_mat, cv::COLOR_BayerRG2RGB);
                break;
            }
            default: {
                // TODO: invalid conversion
            }
        }
    } else {
        // TODO: invalid conversion
    }

    return m_mat;
}

wxBitmap
Image::CreateBitmap() const
{
    auto rgb   = Image(*this, ImageFormat::RGB);
    auto image = wxImage(GetWidth(), GetHeight(), rgb.GetMat().data, true);

    return wxBitmap(image);
}

} // namespace ds
