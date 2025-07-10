#pragma once

#include "stage_base.h"
#include "async.h"
#include "ui.h"

namespace ds::depthscan {
class StagePump
  : public async::Model<StagePump>
{
public:
    StagePump();
    ~StagePump();

    void Initiate() noexcept override;

    asio::awaitable<void> StartPump(async::Lifeguard guard,uint8_t dir,
                                    float ml_min);
    asio::awaitable<void> StopPump(async::Lifeguard guard);

    void SetState(uint32_t state);
    bool IsState(uint32_t state) const;

private:
    std::shared_ptr<Stage>  m_stage;
    async::RawCondition m_cond;
    uint32_t                m_state;
};
}