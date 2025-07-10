#include "label_strings.h"
#include "stage_clean.h"
#include "stage_settings.h"
#include "stage_utility.h"

namespace ds::depthscan {

StageClean::StageClean()
  : m_state(StageDSState::clean_idle)
  , m_clean_secods(0)
  , m_clean_steps(0)
  , m_clean_speed(0)
  , m_send_progress(0)
  , m_cancel(true)
  , m_pump(ui::CreateAsyncModel<StagePump>())
{
    auto storage = StageSettingStorage::GetInstance();
    if (storage) {
        m_clean_secods =
          std::get<int>(storage->GetSettings(StageConfigKeys::CLEAN_SECONDS));
        m_clean_speed =
          std::get<float>(storage->GetSettings(StageConfigKeys::CLEAN_SPEED));
    }
}
StageClean::~StageClean()
{
    Terminate();
}

void
StageClean::Initiate() noexcept
{
}


asio::awaitable<void>
StageClean::StartClean(async::Lifeguard guard)
{
    SetState(StageDSState::clean_busy);
    m_start_now    = std::chrono::high_resolution_clock::now();
    m_progress_now = m_start_now;
    m_send_label = LabelStrings::Cleaning;
    m_send_progress = 0;
    m_cancel = false;

    co_await m_pump->StartPump(
      guard(), MotorDir::pump_dir_clean, m_clean_speed);
    co_await DoClean(guard());

    co_return;
}
asio::awaitable<void>
StageClean::CancelClean(async::Lifeguard guard)
{
    co_await m_pump->StopPump(guard());
    SetState(StageDSState::clean_idle);
    m_cancel = true;
    co_return;
}

asio::awaitable<void>
StageClean::DoClean(async::Lifeguard guard)
{
    auto timer = ds::async::Timer();
    bool done = false;
    while (not done and not m_cancel) {

        std::chrono::high_resolution_clock::time_point end =
          std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
          end - m_start_now);

        auto total_time = (m_clean_secods * 1000);

        if (duration.count() >= total_time) {
            m_send_progress = 100;
            m_send_label = LabelStrings::Completed;
            co_await m_pump->StopPump(guard());
            done = true;
        } else {
            ProgressCalculator progress(total_time);

            m_send_progress = progress.GetPogress(duration.count());
            m_send_label.clear();
            m_send_label << "Cleaning...(" << duration.count()/1000 << " / "
                         << total_time/1000 << " sec)";

        }
        co_await timer.AsyncSleepFor(guard(), 100ms);
    }
    co_return;
}
void
StageClean::SetState(uint32_t state)
{
    if (StageDSState::clean_busy == state)
        this->m_state &= ~(StageDSState::idle | StageDSState::clean_idle);
    if (StageDSState::clean_idle == state)
        this->m_state &= ~(StageDSState::idle | StageDSState::clean_busy);

    this->m_state |= state;
}
bool
StageClean::IsState(uint32_t state) const
{
    if (this->m_state & state)
        return true;
    else
        return false;
}
} // namespace ds