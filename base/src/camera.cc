#include "ds/dev/camera.h"

namespace ds {

Camera::Camera()
  : m_stop(false)
  , m_record(false)
  , m_last_id(0)
{
    StartAcquisition();
}

Camera::~Camera()
{
    StopAcquisition();
}

size_t
Camera::Subscribe(const callback_type& callback, size_t msec)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t                      id = ++m_last_id;
    m_callbacks[id]                = { callback,
                                       std::chrono::nanoseconds(msec * 1'000'000),
                                       std::chrono::steady_clock::now() };
    return id;
}

void
Camera::Unsubscribe(size_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.erase(id);
}

void
Camera::StartRecord(const std::filesystem::path& path)
{
    m_record     = true;
    //m_frames     = std::make_unique<ArrayStore<BlobPtr>>();
    //m_timestamps = std::make_unique<ArrayStore<uint64_t>>();
}

void
Camera::StopRecord()
{
    m_record = false;
    //m_frames.reset();
    //m_timestamps.reset();
}

std::shared_ptr<const CameraFrame>
Camera::LastFrame()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_frame;
}

const std::string&
Camera::SerialNumber() const
{
    return m_serial_number;
}
size_t
Camera::Size() const
{
    return m_size;
}
size_t
Camera::Height() const
{
    return m_height;
}
size_t
Camera::Width() const
{
    return m_width;
}
CameraFormat
Camera::Format() const
{
    return m_format;
}

void
Camera::StartAcquisition()
{
    m_stop = false;
}

void
Camera::StopAcquisition()
{
    m_stop = true;
}

void
Camera::Render()
{
    Model<Camera>::Render();
}

auto
Camera::StartCoroutine(auto&& task, auto&& token)
{
    return asio::co_spawn(m_context,
                          std::forward<decltype(task)>(task),
                          std::forward<decltype(token)>(token));
}

auto
Camera::Post(auto&& task, auto&& token)
{
    return AsyncPost(m_context,
                     std::forward<decltype(task)>(task),
                     std::forward<decltype(token)>(token));
}

} // namespace ds
