#pragma once

#include "ds/async.h"
#include "ds/model.h"
#include <atomic>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

namespace ds {

enum class CameraFormat
{
    INVALID,
    GRAY,
    BayerGR8,
    BayerRG8,
    BayerGB8,
    BayerBG8,
    BayerRGB,
};

struct CameraFrame
{
    uint64_t             seq;
    uint64_t             timestamp;
    size_t               width;
    size_t               height;
    CameraFormat         format;
    std::vector<uint8_t> blob;
};

class Camera : public Model<Camera>
{
public:
    using callback_type =
      std::function<void(std::shared_ptr<const CameraFrame>)>;

    Camera();
    virtual ~Camera();

    virtual double                             SetExposureTime(double sec) = 0;
    virtual double                             SetGain(double gain)        = 0;
    virtual std::tuple<double, double, double> SetGain(double r,
                                                       double g,
                                                       double b)           = 0;

    size_t Subscribe(const callback_type& callback, size_t msec);
    void   Unsubscribe(size_t id);
    void   StartRecord(const std::filesystem::path& path);
    void   StopRecord();

    std::shared_ptr<const CameraFrame> LastFrame();

    const std::string& SerialNumber() const;
    size_t             Size() const;
    size_t             Height() const;
    size_t             Width() const;
    CameraFormat       Format() const;

protected:
    auto StartCoroutine(auto&& task, auto&& token);
    auto Post(auto&& task, auto&& token);

    void StartAcquisition();
    void StopAcquisition();
    void Render() override;

    virtual std::shared_ptr<CameraFrame> DoCapture() = 0;

    std::string  m_serial_number;
    size_t       m_size;
    size_t       m_width;
    size_t       m_height;
    CameraFormat m_format;

private:
    struct CallbackInfo
    {
        callback_type                         callback;
        std::chrono::nanoseconds              interval;
        std::chrono::steady_clock::time_point nexttime;
    };

    std::mutex        m_mutex;
    asio::thread_pool m_context;
    std::atomic<bool> m_stop;

    std::shared_ptr<const CameraFrame> m_frame;
    size_t                             m_last_id;
    std::map<size_t, CallbackInfo>     m_callbacks;

    bool m_record;
    // std::unique_ptr<ArrayStore<BlobPtr>>  m_frames;
    // std::unique_ptr<ArrayStore<uint64_t>> m_timestamps;
};

} // namespace ds
