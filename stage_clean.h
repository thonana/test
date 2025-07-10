#pragma once
#include "ui.h"
#include "async.h"
#include "stage_base.h"
#include "stage_pump.h"


namespace ds::depthscan {
class StageClean 
  : public async::Model<StageClean>
{
public:
    StageClean();
    ~StageClean();

    void Initiate() noexcept override;

    void                  SetState(uint32_t state);
    bool                  IsState(uint32_t state) const;
    asio::awaitable<void> StartClean(async::Lifeguard guard);
    asio::awaitable<void> CancelClean(async::Lifeguard guard);
    asio::awaitable<void> DoClean(async::Lifeguard guard);
    int                   GetProgress() const { return m_send_progress; }
    wxString              GetLabel() const { return m_send_label;}

private:
    std::shared_ptr<StagePump>   m_pump;

    std::chrono::high_resolution_clock::time_point m_start_now;
    std::chrono::high_resolution_clock::time_point m_progress_now;

    uint32_t           m_state;
    uint32_t           m_clean_secods;
    uint32_t           m_clean_steps;
    float              m_clean_speed;
    int                m_send_progress;
    wxString           m_send_label;
    bool               m_cancel;
};
} // namespace ds