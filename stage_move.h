#pragma once

#include "stage_base.h"
#include "ui.h"

namespace ds::depthscan {

class StageMove 
    : public async::Model<StageMove>
{
public:
    StageMove();
    ~StageMove();

    void Initiate() noexcept override;

    asio::awaitable<void> MoveInitPos(async::Lifeguard guard);
    asio::awaitable<void> MoveLastPos(async::Lifeguard guard,int x,int y);
    asio::awaitable<void> Move(async::Lifeguard guard,
                               uint8_t mtr,
                               uint8_t  mode,
                               uint32_t speed,
                               int pos);

    asio::awaitable<void> StopMove(async::Lifeguard guard, uint8_t mtr);

    asio::awaitable<void> MoveHome(async::Lifeguard guard);
    asio::awaitable<void> StopHome(async::Lifeguard guard);
    asio::awaitable<void> DoneHome(async::Lifeguard guard);
    asio::awaitable<void> GetNotBusy(async::Lifeguard guard);
    asio::awaitable<bool> IsBusy(async::Lifeguard guard);

    asio::awaitable<int> GetPos(async::Lifeguard guard,uint8_t mtr);
    void                 SetState(uint32_t state);
    bool                 IsState(uint32_t state) const;
    uint32_t             GetState() const;
    int                  GetLastPos(uint8_t mtr) const
    {
        if (mtr == MotorRole::mtr_x)
            return m_last_x_pos;
        else if (mtr == MotorRole::mtr_y)
            return m_last_y_pos;
        else
            return 0;
    }
    void                SetLastPos(uint8_t mtr, int pos)
    {
        if (mtr == MotorRole::mtr_x)
            m_last_x_pos = pos;
        else if (mtr == MotorRole::mtr_y)
            m_last_y_pos = pos;
    }

private:
    std::shared_ptr<Stage> m_stage;
    async::RawCondition m_cond;

    uint32_t           m_state;

    int m_last_x_pos;
    int m_last_y_pos;
};
} // namespace ds