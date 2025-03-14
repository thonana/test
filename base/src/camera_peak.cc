#include "ds/dev/camera_peak.h"
#include "ds/objectpool.h"
#include <ids_peak_comfort_c/ids_peak_comfort_c.h>
#include <spdlog/spdlog.h>

namespace ds {

class PeakLibrary
{
    PeakLibrary()
    {
        peak_status status = PEAK_STATUS_ERROR;
        try {
            status = peak_Library_Init();
        } catch (...) {
            // ignore, do nothing
        }

        if (PEAK_ERROR(status)) {
            size_t size = 0;
            peak_Library_GetLastError(nullptr, nullptr, &size);
            std::string msg(size, '\0');
            peak_Library_GetLastError(&status, msg.data(), &size);
            spdlog::critical("{}", msg);
        }
    }

    ~PeakLibrary() { peak_Library_Exit(); };

public:
    static const PeakLibrary& Instance()
    {
        static PeakLibrary instance;
        return instance;
    }
};

struct CompareFrame
{
    using is_transparent = void;

    bool operator()(const std::unique_ptr<CameraFrame>& lhs,
                    const std::unique_ptr<CameraFrame>& rhs) const
    {
        return lhs->blob.data() < rhs->blob.data();
    }

    bool operator()(const std::unique_ptr<CameraFrame>& lhs,
                    const uint8_t*                      rhs) const
    {
        return lhs->blob.data() < rhs;
    }

    bool operator()(const uint8_t*                      lhs,
                    const std::unique_ptr<CameraFrame>& rhs) const
    {
        return lhs < rhs->blob.data();
    }
};

struct CameraPeak::Impl
{
    const peak_camera_handle camera;
    ObjectPool<CameraFrame>  pool;

    Impl(peak_camera_handle camera)
      : camera(camera)
    {
    }

    ~Impl() { peak_Camera_Close(camera); }
};

CameraPeak::CameraPeak(std::size_t idev)
  : m_impl(nullptr)
{
    PeakLibrary::Instance();
    peak_CameraList_Update(nullptr);

    size_t count;
    peak_CameraList_Get(nullptr, &count);

    std::vector<peak_camera_descriptor> desc(count);
    peak_CameraList_Get(desc.data(), &count);

    peak_camera_handle hcam;
    peak_Camera_Open(desc.at(idev).cameraID, &hcam);

    // serial number
    m_serial_number = desc.at(idev).serialNumber;

    // reset to default setting
    peak_Camera_ResetToDefaultSettings(hcam);

    // ROI
    peak_roi roi;
    peak_ROI_Get(hcam, &roi);
    m_width  = roi.size.width;
    m_height = roi.size.height;

    // pixel format
    peak_pixel_format format = PEAK_PIXEL_FORMAT_INVALID;
    peak_PixelFormat_Get(hcam, &format);

    switch (format) {
        case PEAK_PIXEL_FORMAT_BAYER_GR8:
            m_format = CameraFormat::BayerGR8;
            break;
        case PEAK_PIXEL_FORMAT_BAYER_RG8:
            m_format = CameraFormat::BayerRG8;
            break;
        case PEAK_PIXEL_FORMAT_BAYER_GB8:
            m_format = CameraFormat::BayerGB8;
            break;
        case PEAK_PIXEL_FORMAT_BAYER_BG8:
            m_format = CameraFormat::BayerBG8;
            break;
        default:
            m_format = CameraFormat::GRAY;
            break;
    }

    size_t size = 0;
    peak_Acquisition_Buffer_GetRequiredSize(hcam, &size);
    m_size = size;
    m_impl = std::make_shared<CameraPeak::Impl>(hcam);

    //SetExposureTime(0.05);

    peak_Acquisition_Start(hcam, PEAK_INFINITE);
    StartAcquisition();
}

CameraPeak::~CameraPeak()
{
    StopAcquisition();
    if(m_impl != PEAK_INVALID_HANDLE)
        peak_Acquisition_Stop(m_impl->camera);
}

double
CameraPeak::SetExposureTime(double msec)
{
    peak_ExposureTime_Set(m_impl->camera, msec * 1e3);
    peak_ExposureTime_Get(m_impl->camera, &msec);
    Render(); // 상태 변경 후 View 업데이트
    return msec * 1e-3;
}

double
CameraPeak::SetGain(double gain)
{
    peak_Gain_Set(
      m_impl->camera, PEAK_GAIN_TYPE_ANALOG, PEAK_GAIN_CHANNEL_MASTER, gain);
    peak_Gain_Get(
      m_impl->camera, PEAK_GAIN_TYPE_ANALOG, PEAK_GAIN_CHANNEL_MASTER, &gain);
    Render(); // 상태 변경 후 View 업데이트
    return gain;
}

std::tuple<double, double, double>
CameraPeak::SetGain(double r, double g, double b)
{
    peak_Gain_Set(
      m_impl->camera, PEAK_GAIN_TYPE_ANALOG, PEAK_GAIN_CHANNEL_RED, r);
    peak_Gain_Set(
      m_impl->camera, PEAK_GAIN_TYPE_ANALOG, PEAK_GAIN_CHANNEL_GREEN, g);
    peak_Gain_Set(
      m_impl->camera, PEAK_GAIN_TYPE_ANALOG, PEAK_GAIN_CHANNEL_BLUE, b);

    peak_Gain_Get(
      m_impl->camera, PEAK_GAIN_TYPE_ANALOG, PEAK_GAIN_CHANNEL_RED, &r);
    peak_Gain_Get(
      m_impl->camera, PEAK_GAIN_TYPE_ANALOG, PEAK_GAIN_CHANNEL_GREEN, &g);
    peak_Gain_Get(
      m_impl->camera, PEAK_GAIN_TYPE_ANALOG, PEAK_GAIN_CHANNEL_BLUE, &b);

    Render(); // 상태 변경 후 View 업데이트
    return { r, g, b };
}

std::shared_ptr<CameraFrame>
CameraPeak::DoCapture()
{
    peak_frame_handle            hframe = PEAK_INVALID_HANDLE;
    std::shared_ptr<CameraFrame> frame  = nullptr;

    auto status = peak_Acquisition_WaitForFrame(m_impl->camera, 500, &hframe);
    if (!PEAK_ERROR(status)) {
        frame = m_impl->pool.Allocate();

        peak_buffer buffer = { nullptr, 0, nullptr };
        peak_Frame_Buffer_Get(hframe, &buffer);
        peak_Frame_ID_Get(hframe, &frame->seq);
        peak_Frame_Timestamp_Get(hframe, &frame->timestamp);

        frame->blob.assign(buffer.memoryAddress,
                           buffer.memoryAddress + buffer.memorySize);
        frame->format = m_format;
        frame->width  = m_width;
        frame->height = m_height;

        peak_Frame_Release(m_impl->camera, hframe);

        Render(); // 캡처된 데이터 렌더링 후 업데이트
    }

    return frame;
}

} // namespace ds
