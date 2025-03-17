#include "ds/dev/stage.h"
namespace ds {

Stage::Stage()
  //: m_context(1)
  //, m_timer(m_context)
  //, m_cancel()
  //, m_run()
  :m_state(StageState::idle)
{
    //m_cancel.clear();
    //m_run.clear();

    //asio::co_spawn(m_context, Routine(), asio::detached);
}

Stage::~Stage()
{
    //m_timer.cancel();
    //m_context.join();
}

//asio::awaitable<void>
//Stage::Routine()
//{
//    for (;;) {
//        while (not m_cancel.test() && m_run.test()) {
//            CallAfter(&Stage::Do_10msTimer);
//            m_timer.expires_after(std::chrono::milliseconds(10));
//        }
//
//        if (m_cancel.test()) {
//            break;
//        }
//
//        m_timer.expires_after(std::chrono::microseconds(100));
//        try {
//            co_await m_timer.async_wait(asio::use_awaitable);
//        } catch (std::system_error& e) {
//            if (e.code() == asio::error::operation_aborted) {
//                break;
//            }
//            throw;
//        }
//    }
//    co_return;
//}

//void 
//Stage::Do_10msTimer()
//{
//    if (not IsState(StageState::idle) and
//        not IsState(StageState::move_busy) and
//        not IsState(StageState::home_busy) and
//        not IsState(StageState::focus_busy)) {
//        SetState(StageState::idle);
//    }
//}
//void
//Stage::Render()
//{
//    Model<Stage>::Render();
//}

void
Stage::Init()
{
}

void
Stage::Move_Up()
{
    SetState(StageState::move_busy);
}

void
Stage::SetState(uint32_t state)
{
    m_state |= state;
}
void
Stage::ClearState(uint32_t state)
{
    m_state &= ~state;
}
bool
Stage::IsState(uint32_t state) const
{
    return m_state & state;
}
} // namespace ds