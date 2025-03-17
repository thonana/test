#pragma once
#include <cstdint>
#include <unordered_set>
#include "ds/model.h"
#include "ds/view.h"
#include "ds/async.h"

namespace ds {
class StageState
{
public:
    static constexpr uint32_t idle       = 0b00000001; // 1
    static constexpr uint32_t home_busy  = 0b00000010; // 2
    static constexpr uint32_t home_done  = 0b00000100; // 4
    static constexpr uint32_t focus_busy = 0b00001000; // 8
    static constexpr uint32_t focus_done = 0b00010000; // 16
    static constexpr uint32_t move_busy  = 0b00100000; // 32
    static constexpr uint32_t pump_cw    = 0b10000000; // 128
    static constexpr uint32_t pump_ccw   = 0b100000000; // 256
};

class Stage
{
    //asio::thread_pool  m_context;
    //asio::steady_timer m_timer;
    //std::atomic_flag   m_cancel;
    //std::atomic_flag   m_run;
    uint32_t           m_state;

public:
    Stage();
    ~Stage();

    //void Do_10msTimer();
    //void Render() override;

    void Init();
    void Move_Up();

    void SetState(uint32_t state);
    void ClearState(uint32_t state);
    bool IsState(uint32_t state) const;

private:
    //asio::awaitable<void> Routine();
};

class StageAgent : public Model<StageAgent>,public Stage
{
public:
    StageAgent(){}
    ~StageAgent(){}

    void MoveUp() { Stage::Move_Up(); }


    void Render() override {}

private:

};
} // namespace ds