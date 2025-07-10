#include "stage_pump.h"
#include <spdlog/spdlog.h>

namespace ds::depthscan {

StagePump::StagePump()
  : m_state(StageDSState::pump_idle)
  , m_stage(ui::CreateAsyncModel<Stage>())
{

}

StagePump::~StagePump() {}

void
StagePump::Initiate() noexcept
{

}

asio::awaitable<void>
StagePump::StartPump(async::Lifeguard guard, uint8_t dir, float ml_min)
{
    static float pre_speed = 0.0;

    if (ml_min != pre_speed) {    
        if (IsState(StageDSState::pump_busy))
            co_await m_stage->Stop(guard(), MotorRole::mtr_p);
        pre_speed = ml_min;
    }

    int speed = 0;

    float x_rpm = 0.0;
    float rpm   = 0.0;

    rpm = ml_min * 11.11;
    if (rpm != 0)
        x_rpm = (60 / rpm);
    else
        x_rpm = (60 / 0.1);

    speed = (200 * MICRO_STEP) / x_rpm;

    co_await m_stage->Go(
      guard(),
      MotorRole::mtr_p, dir, SingleMode::mode_inf, 0, 0, speed);
    SetState(StageDSState::pump_busy);

    co_return;
}
asio::awaitable<void>
StagePump::StopPump(async::Lifeguard guard)
{
    co_await m_stage->Stop(guard(), MotorRole::mtr_p);
    SetState(StageDSState::pump_idle);
    co_return;
}
void
StagePump::SetState(uint32_t state)
{
    if (StageDSState::pump_busy == state)
        this->m_state &= ~StageDSState::pump_idle;
    if (StageDSState::pump_idle == state)
        this->m_state &= ~StageDSState::pump_busy;

    this->m_state |= state;
}
bool
StagePump::IsState(uint32_t state) const
{
    if (m_state & state)
        return true;
    else
        return false;
}
}