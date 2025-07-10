
#include "ds/depthscan/stage_move.h"
#include "ds/depthscan/stage_settings.h"

constexpr auto move_speed = 200;
constexpr auto move_max   = 2000000;
constexpr auto move_min   = -2000000;

namespace ds::depthscan {

StageMove::StageMove()
  : m_last_x_pos(0)
  , m_last_y_pos(0)
  , m_stage(ui::CreateAsyncModel<Stage>())
  , m_state(StageDSState::move_idle_x | StageDSState::move_idle_y)
{

    try {
        auto storage = StageSettingStorage::GetInstance();
        if (storage)
        {
            m_last_x_pos =
              std::get<int>(storage->GetSettings(StageConfigKeys::LAST_X_POS));
            m_last_y_pos =
              std::get<int>(storage->GetSettings(StageConfigKeys::LAST_Y_POS));
        }

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("No instance : ") + e.what());
    }
}
StageMove::~StageMove() {}

void
StageMove::Initiate() noexcept
{
}


asio::awaitable<void>
StageMove::MoveInitPos(async::Lifeguard guard)
{
    SetState(StageDSState::home_busy);
    co_await m_stage->MoveInitPos(guard());
    SetState(StageDSState::home_done);
    co_return;
}

asio::awaitable<void>
StageMove::MoveLastPos(async::Lifeguard guard,int x,int y)
{
    SetState(StageDSState::home_busy);
    co_await m_stage->MoveLastPos(guard(),x,y);
    SetState(StageDSState::home_done);
    co_return;
}

asio::awaitable<void>
StageMove::Move(async::Lifeguard guard,uint8_t mtr,
                uint8_t mode,
                uint32_t speed,
                int pos)
{
    co_await m_stage->SetPos(guard(), mtr, pos);

    co_await m_stage->Go(
          guard(), mtr, MotorDir::motdir_cw, mode, 0, 0, speed);


    if (mtr & MotorRole::mtr_x)
        SetState(StageDSState::move_busy_x);
    if (mtr & MotorRole::mtr_y)
        SetState(StageDSState::move_busy_y);
    co_return;
}

asio::awaitable<void>
StageMove::StopMove(async::Lifeguard guard, uint8_t mtr)
{
    co_await m_stage->Stop(guard(), mtr);

    if (mtr & MotorRole::mtr_x)
        SetState(StageDSState::move_idle_x);
    if (mtr & MotorRole::mtr_y)
        SetState(StageDSState::move_idle_y);
    co_return;
}

asio::awaitable<void>
StageMove ::MoveHome(async::Lifeguard guard)
{

    co_await m_stage->SetPos(guard(), MotorRole::mtr_x, 0);
    co_await m_stage->SetPos(guard(), MotorRole::mtr_y, 0);
    co_await m_stage->Go(guard(),
                            MotorRole::mtr_x,
                                        MotorDir::motdir_cw,
                                        SingleMode::mode_home,
                                        0,
                                        0,
                                        200 * MICRO_STEP);
    co_await m_stage->Go(guard(),
                            MotorRole::mtr_y,
                                        MotorDir::motdir_cw,
                                        SingleMode::mode_home,
                                        0,
                                        0,
                                        200 * MICRO_STEP);

    SetState(StageDSState::home_busy);
    co_return;
}
asio::awaitable<void>
StageMove::StopHome(async::Lifeguard guard)
{
    co_await m_stage->Stop(guard(), MotorRole::mtr_x);
    co_await m_stage->Stop(guard(), MotorRole::mtr_y);

    SetState(StageDSState::home_idle);
    co_return;
}
asio::awaitable<void>
StageMove::DoneHome(async::Lifeguard guard)
{
    bool done = false;

    while (not done) {
        done = co_await m_stage->IsHome(guard(),
                                        MotorRole::mtr_x |
                                                    MotorRole::mtr_y);
    }
    SetState(StageDSState::home_done);
    co_return;
}
asio::awaitable<void>
StageMove::GetNotBusy(async::Lifeguard guard)
{
    bool done = false;
    auto timer = ds::async::Timer();
    while (not done) {
        done = co_await m_stage->GetNotBusy(guard());
        if (!done)
            co_await timer.AsyncSleepFor(guard(), 10ms);
    }
    co_return;
}

asio::awaitable<bool>
StageMove::IsBusy(async::Lifeguard guard)
{
    co_return co_await m_stage->GetNotBusy(guard());
}

asio::awaitable<int>
StageMove::GetPos(async::Lifeguard guard,uint8_t mtr)
{
    int pos = co_await m_stage->GetPos(guard(), mtr);
    co_return pos;
}

void
StageMove::SetState(uint32_t state)
{
    if (StageDSState::move_busy_x == state)
        this->m_state &= ~StageDSState::move_idle_x;
    if (StageDSState::move_idle_x == state)
        this->m_state &= ~StageDSState::move_busy_x;
    if (StageDSState::move_busy_y == state)
        this->m_state &= ~StageDSState::move_idle_y;
    if (StageDSState::move_idle_y == state)
        this->m_state &= ~StageDSState::move_busy_y;
    if (StageDSState::home_busy == state)
        this->m_state &= ~(StageDSState::home_idle | StageDSState::home_done);
    if (StageDSState::home_idle == state)
        this->m_state &= ~(StageDSState::home_busy | StageDSState::home_done);
    if (StageDSState::home_done == state)
        this->m_state &= ~(StageDSState::home_busy | StageDSState::home_idle);

    this->m_state |= state;
}
bool
StageMove::IsState(uint32_t state) const
{
    if (this->m_state & state)
        return true;
    else
        return false;
}
uint32_t
StageMove::GetState() const
{
    return this->m_state;
}
} // namespace ds