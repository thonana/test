#pragma once
#include "async.h"
#include "stage_base.h"
#include "stage_frame.h"
#include "stage_move.h"
#include "ui.h"

namespace ds::depthscan {

struct ExposureResult
{
    std::chrono::nanoseconds exposure_time{0us };
    double brightness{ 0.0 };
    double contrast{ 0.0 };
    double overexposed_ratio{ 0.0 };
    double underexposed_ratio{ 0.0 };
    double quality_score{ 0.0 };
};

class StageAutoExposure : public async::Model<StageAutoExposure>
{
public:
    StageAutoExposure();
    ~StageAutoExposure();

    void Initiate() noexcept override;

    asio::awaitable<void> InitSetup(async::Lifeguard guard);
    asio::awaitable<void> Processing(async::Lifeguard guard);
    asio::awaitable<void> Complete(async::Lifeguard guard);

    asio::awaitable<void> StartAutoExposure(async::Lifeguard guard);
    asio::awaitable<void> CancelAutoExposure(async::Lifeguard guard);

private:
    ds::async::RawCondition m_cond;

    std::shared_ptr<StageFrame> m_frame;
    std::shared_ptr<StageMove> m_move;

    std::vector<ExposureResult> m_exposure_data;

    bool   m_cancel;
    int    m_iteration;
    std::chrono::nanoseconds m_exposure_value;
};
}