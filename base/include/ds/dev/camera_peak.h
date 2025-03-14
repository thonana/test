#pragma once

#include "ds/dev/camera.h"
#include "ds/model.h"
//#include <ds/array.h>
#include <memory>
#include <set>

namespace ds {

class CameraPeak : public Camera
{
public:
    CameraPeak(size_t idev);
    ~CameraPeak() override;

    double                             SetExposureTime(double sec) override;
    double                             SetGain(double gain) override;
    std::tuple<double, double, double> SetGain(double r,
                                               double g,
                                               double b) override;
    std::shared_ptr<const CameraFrame> CaptureFrame() { return DoCapture(); }

protected:
    std::shared_ptr<CameraFrame> DoCapture() override;

private:
    struct Impl;
    std::shared_ptr<Impl> m_impl;
};

} // namespace ds
