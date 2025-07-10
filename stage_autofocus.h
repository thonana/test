#pragma once
#include "ui.h"
#include "async.h"
#include "stage_base.h"
#include "stage_frame.h"
#include "stage_move.h"
#include "stage_pump.h"
#include "stage_automode.h"

namespace ds::depthscan {

enum struct Method_Focus
{
    ROI_CHANNEL = 0,
    ROI_LINE = 1,
    ROI_MARKER = 2,
    ROI_EXTERNAL = 3,
};

constexpr int MICRO_STEP_MUILPLIER = 21; //43; // 42.67
// constexpr int REFERNCE_INDEX       = 60;    // reference postion index
// (chipshot type)
constexpr int REFERENC_INDEX = 1000; // reference postion index (firefly type)
//constexpr int CHANNEL_WIDTH = 1000;
 constexpr int CHANNEL_WIDTH = 2800;

class StageAutoFocus : public async::Model<StageAutoFocus>
{
public:
    StageAutoFocus();
    ~StageAutoFocus();

    void Initiate() noexcept override;


    asio::awaitable<void> StartAutoFocus(async::Lifeguard guard, bool fine);
    asio::awaitable<void> StartOverAllAutoFocus(async::Lifeguard guard);
    asio::awaitable<void> CancelAutoFocus(async::Lifeguard guard);
    asio::awaitable<void> ConfirmWater(async::Lifeguard guard)
    {
        m_ok_user_water = true;
        co_return;
    }
    int GetProgress() const { return m_send_progress; }
    wxString GetLabel() const { return m_send_label; }

    void SetState(uint32_t state);
    bool IsState(uint32_t state) const;

    asio::awaitable<void> InitSetup(async::Lifeguard guard, bool fine,std::string path);
    asio::awaitable<void> CenterPosition(async::Lifeguard guard,
                                         bool fine,
                                         std::string path);
    asio::awaitable<void> SearchFlow(async::Lifeguard guard);
    asio::awaitable<void> PumpUntilFlow(async::Lifeguard guard);
    asio::awaitable<void> Focusing(async::Lifeguard guard,std::string path);
    asio::awaitable<void> OverallFocusing(async::Lifeguard guard,std::string path);
    asio::awaitable<void> SaveLog(async::Lifeguard guard,std::string path);

private:
    std::shared_ptr<StageFrame> m_frame;
    std::shared_ptr<StageMove> m_move;
    std::shared_ptr<StagePump> m_pump;
    std::shared_ptr<StageAutoMode> m_automode;

    uint32_t m_state;

    int m_num_focus;
    int m_pos;
    int m_start_pos;
    int m_step;
    int m_total_steps;
    int m_center_idx;
    int m_min_idx;
    int m_max_idx;
    int m_init_x_pos;
    bool m_cancel;
    bool m_need_water;
    bool m_inside_water;
    bool m_overall_focusing;
    bool m_stop;
    bool m_need_focusing;
    bool m_ok_user_water;

    std::chrono::high_resolution_clock::time_point m_progressNow;
    int m_send_progress;
    wxString m_send_label;

    ds::async::RawCondition m_cond;

    std::vector<double> m_templates;
    std::vector<double> m_positions;
};
}
