#pragma once
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "async.h"
#include "ui.h"
#include "stage_base.h"
#include "stage_frame.h"
#include "stage_move.h"
#include "stage_pump.h"

namespace ds::depthscan {
class StageAutoMode : public async::Model<StageAutoMode>
{
public:
    StageAutoMode();
    ~StageAutoMode();
    
    void Initiate() noexcept override;

    void                  SetState(uint32_t state);
    bool                  IsState(uint32_t state) const;
    asio::awaitable<void> StartAutoMode(async::Lifeguard guard,bool manual_recording);
    asio::awaitable<void> CancelAutoMode(async::Lifeguard guard);
    asio::awaitable<void> InitSetup(async::Lifeguard guard);
    asio::awaitable<void> SearchFlow(async::Lifeguard guard,
                                     bool manual_recording);
    asio::awaitable<void> Recording(async::Lifeguard guard);
    asio::awaitable<void> Complete(async::Lifeguard guard);
   
    bool CheckFocusNeed(cv::Mat& image);

    int GetProgress() const { return m_send_progress; }
    wxString GetLabel() const { return m_send_label; }

    bool CheckEntry(double thres) const;
    bool CheckExit(double thres) const;
    bool GetNeedRefocus() const;
    void SetNeedRefocus(bool refocus);


private:
    std::shared_ptr<StageMove>      m_move;
    std::shared_ptr<StagePump>      m_pump;
    std::shared_ptr<StageFrame>     m_frame;

    uint32_t           m_state;

    ds::async::RawCondition m_cond;
    bool                 m_cancel;
    bool                 m_auto_check;  
    bool                 m_first_image;   
    bool                 m_manual_recording;

    std::chrono::high_resolution_clock::time_point m_start_now;
    std::chrono::high_resolution_clock::time_point m_progressNow;
    int                                            m_send_progress;
    wxString                                       m_send_label;
    int                                            m_prime_seconds;
    int                                            m_camera_seconds;
    float                                          m_high_speed;
    float                                          m_normal_speed;
    float m_threshold_entry;
    float m_threshold_exit;

    std::vector<double> m_templates;
    std::shared_ptr<BrightnessFilter> m_filter;
};
} // namespace ds