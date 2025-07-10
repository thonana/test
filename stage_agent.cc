#include "devicefocus.h"
#include "stage_settings.h"
#include "stage_agent.h"

namespace ds::depthscan {
StageAgent::StageAgent()
  : m_pump(ui::CreateAsyncModel<StagePump>())
  , m_move(ui::CreateAsyncModel<StageMove>())
  , m_auto_focus(ui::CreateAsyncModel<StageAutoFocus>())
  , m_frame(ui::CreateAsyncModel<StageFrame>())
  , m_clean(ui::CreateAsyncModel<StageClean>())
  , m_auto(ui::CreateAsyncModel<StageAutoMode>())
  , m_auto_exposure(ui::CreateAsyncModel<StageAutoExposure>())
  , m_led(ui::CreateAsyncModel<StageLED>())
  , m_progress(0)
  , m_stop(false)
  , m_device_state(StageDSState::idle)
{
    std::string camera_name = m_frame->GetCameraName();
    auto storage = StageSettingStorage::GetInstance();
    storage->SetCameraName(camera_name);
    storage->LoadCameraSettingsFromJson(camera_name);
}

void
StageAgent::Initiate() noexcept
{
    Start<&StageAgent::TaskStatus>(NewLife());
    Start<&StageAgent::TaskEvent>(NewLife());
}

StageAgent::~StageAgent() 
{
}

void
StageAgent::CommitState() noexcept
{
    SetSnapshot(m_device_state,
                m_progress_label,
                m_progress,
                m_position_label,
                m_status_label,
                m_warning_label);
}

asio::awaitable<void>
StageAgent::DoFocusDevice(async::Lifeguard guard, Device_Cmd cmd)
{   
    auto timer = ds::async::Timer();
    bool done = false;
    switch (cmd) {
        case Device_Cmd::MOVE_UP:
            if (not(m_device_state & StageDSState::move_busy_x)) {
                while (!done) {
                    co_await m_move->GetNotBusy(guard());
                    co_await m_move->Move(guard(),
                                          MotorRole::mtr_x,
                                          SingleMode::mode_cnt,
                                          100 * MICRO_STEP,
                                          POS_MAX);
                    m_device_state &= ~StageDSState::home_done;
                    m_device_state |= StageDSState::move_busy_x;
                    done = true;
                }              
            }
            break;

        case Device_Cmd::MOVE_DOWN:
            if (not(m_device_state & StageDSState::move_busy_x)) {
                while (!done) {
                    co_await m_move->GetNotBusy(guard());
                    co_await m_move->Move(guard(),
                                          MotorRole::mtr_x,
                                          SingleMode::mode_cnt,
                                          100 * MICRO_STEP,
                                          POS_MIN);
                    m_device_state &= ~StageDSState::home_done;
                    m_device_state |= StageDSState::move_busy_x;
                    done = true;
                }
            }
            break;

        case Device_Cmd::MOVE_STOP_X:
            while (!done) {
                co_await m_move->StopMove(guard(), MotorRole::mtr_x);
                m_device_state &= ~StageDSState::move_busy_x;
                m_device_state |= StageDSState::move_idle_x;
                {
                    int pos_x = 0, pos_y = 0;
                    co_await m_move->GetNotBusy(guard());
                    pos_x = co_await m_move->GetPos(guard(), MotorRole::mtr_x);
                    pos_y = co_await m_move->GetPos(guard(), MotorRole::mtr_y);
                    m_position_label =
                      wxString::Format("[%d,%d]", pos_x, pos_y);
                }
                done = true;
            }

            break;

        case Device_Cmd::MOVE_LEFT:
            if (not(m_device_state & StageDSState::move_busy_y)) {
                while (!done) {
                    co_await m_move->GetNotBusy(guard());
                    co_await m_move->Move(guard(),
                                          MotorRole::mtr_y,
                                          SingleMode::mode_cnt,
                                          100 * MICRO_STEP,
                                          POS_MIN);
                    m_device_state &= ~StageDSState::home_done;
                    m_device_state |= StageDSState::move_busy_y;
                    done = true;
                }
            }
            break;

        case Device_Cmd::MOVE_RIGHT:
            if (not(m_device_state & StageDSState::move_busy_y)) {
                while (!done) {
                    co_await m_move->GetNotBusy(guard());
                    co_await m_move->Move(guard(),
                                          MotorRole::mtr_y,
                                          SingleMode::mode_cnt,
                                          100 * MICRO_STEP,
                                          POS_MAX);
                    m_device_state &= ~StageDSState::home_done;
                    m_device_state |= StageDSState::move_busy_y;
                    done = true;
                }
            }
            break;

        case Device_Cmd::MOVE_STOP_Y:
            while (!done) {
                co_await m_move->StopMove(guard(), MotorRole::mtr_y);
                m_device_state &= ~StageDSState::move_busy_y;
                m_device_state |= StageDSState::move_idle_y;
                {
                    int pos_x = 0, pos_y = 0;
                    co_await m_move->GetNotBusy(guard());
                    pos_x = co_await m_move->GetPos(guard(), MotorRole::mtr_x);
                    pos_y = co_await m_move->GetPos(guard(), MotorRole::mtr_y);
                    m_position_label =
                      wxString::Format("[%d,%d]", pos_x, pos_y);
                }
                done = true;
            }

            break;

        case Device_Cmd::MOVE_HOME:
            co_await m_move->MoveHome(guard());
            m_device_state &= ~StageDSState::home_done;
            m_device_state |= StageDSState::home_busy;
            co_await m_move->DoneHome(guard());
            m_device_state &= ~StageDSState::home_busy;
            m_device_state |= StageDSState::home_done;
            break;

        case Device_Cmd::AUTO_FOUCS:
            m_device_state &= ~StageDSState::home_done;
            m_device_state |= StageDSState::focus_busy;
            co_await m_auto_focus->StartOverAllAutoFocus(guard());
            m_device_state &= ~StageDSState::focus_busy;
            m_device_state |= StageDSState::idle;
            break;

        case Device_Cmd::SAVE_INIT: {
            int pos_x = 0, pos_y = 0;
            co_await m_move->GetNotBusy(guard());
            pos_x = co_await m_move->GetPos(guard(), MotorRole::mtr_x);
            pos_y = co_await m_move->GetPos(guard(), MotorRole::mtr_y);

            auto storage = StageSettingStorage::GetInstance();
            if (storage) {
                storage->SaveSettingToJson(StageConfigKeys::INIT_X_POS, pos_x);
                storage->SaveSettingToJson(StageConfigKeys::INIT_Y_POS, pos_y);
            }
            break;
        }

        case Device_Cmd::AUTO_EXPOSURE:
            m_device_state &= ~StageDSState::home_done;
            m_device_state |= StageDSState::exposure;
            co_await m_auto_exposure->StartAutoExposure(guard());
            m_device_state &= ~StageDSState::exposure;
            m_device_state |= StageDSState::idle;

            break;

        case Device_Cmd::MOVE_UP_STOP:
            if (not(m_device_state & StageDSState::move_busy_x)) {
                co_await m_move->GetNotBusy(guard());
                co_await m_move->Move(guard(),
                                      MotorRole::mtr_x,
                                      SingleMode::mode_cnt,
                                      100 * MICRO_STEP,
                                      POS_MAX);
                co_await timer.AsyncSleepFor(guard(), 10ms);
                co_await m_move->StopMove(guard(), MotorRole::mtr_x);
                m_device_state &= ~StageDSState::home_done;
                m_device_state |= StageDSState::move_busy_x;
            }
            break;
        case Device_Cmd::MOVE_DOWN_STOP:
            if (not(m_device_state & StageDSState::move_busy_x)) {
                co_await m_move->GetNotBusy(guard());
                co_await m_move->Move(guard(),
                                      MotorRole::mtr_x,
                                      SingleMode::mode_cnt,
                                      100 * MICRO_STEP,
                                      POS_MIN);
                co_await timer.AsyncSleepFor(guard(), 10ms);
                co_await m_move->StopMove(guard(), MotorRole::mtr_x);
                m_device_state &= ~StageDSState::home_done;
                m_device_state |= StageDSState::move_busy_x;
            }
            break;
        case Device_Cmd::MOVE_LEFT_STOP:
            if (not(m_device_state & StageDSState::move_busy_y)) {
                co_await m_move->GetNotBusy(guard());
                co_await m_move->Move(guard(),
                                      MotorRole::mtr_y,
                                      SingleMode::mode_cnt,
                                      100 * MICRO_STEP,
                                      POS_MIN);
                co_await timer.AsyncSleepFor(guard(), 10ms);
                co_await m_move->StopMove(guard(), MotorRole::mtr_y);
                m_device_state &= ~StageDSState::home_done;
                m_device_state |= StageDSState::move_busy_y;
            }
            break;

        case Device_Cmd::MOVE_RIGHT_STOP:
            if (not(m_device_state & StageDSState::move_busy_y)) {
                co_await m_move->GetNotBusy(guard());
                co_await m_move->Move(guard(),
                                      MotorRole::mtr_y,
                                      SingleMode::mode_cnt,
                                      100 * MICRO_STEP,
                                      POS_MAX);
                co_await timer.AsyncSleepFor(guard(), 10ms);
                co_await m_move->StopMove(guard(), MotorRole::mtr_y);
                m_device_state &= ~StageDSState::home_done;
                m_device_state |= StageDSState::move_busy_y;
            }
            break;
    }
    co_return;
}

asio::awaitable<void>
StageAgent::DoPumpDeivce(async::Lifeguard guard, Pump_Cmd cmd,wxString flow)
{
    float flow_rate = static_cast<float>(wxAtof(flow));
    switch (cmd) {
        case Pump_Cmd::PUMP_CCW: {
            m_device_state &= ~StageDSState::pump_idle;
            m_device_state |= StageDSState::pump_busy;
            co_await m_pump->StartPump(
              guard(), MotorDir::motdir_ccw, flow_rate);
            break;
        }
        case Pump_Cmd::PUMP_CW: {   
            m_device_state &= ~StageDSState::pump_idle;
            m_device_state |= StageDSState::pump_busy;
            co_await m_pump->StartPump(
                guard(), MotorDir::motdir_cw, flow_rate);
            break;
        }
        case Pump_Cmd::PUMP_STOP:
            m_device_state &= ~StageDSState::pump_busy;
            m_device_state |= StageDSState::pump_idle;
            co_await m_pump->StopPump(guard());
            break;
    }
    co_return;
}

asio::awaitable<void>
StageAgent::DoMainDeivce(async::Lifeguard guard, Main_Cmd cmd)
{
    switch (cmd) {
        case Main_Cmd::CLEAN_START: {
            if (not(m_device_state & StageDSState::clean_busy)) {
                m_device_state |= StageDSState::clean_busy;
                co_await m_clean->StartClean(guard());
                m_device_state &= ~StageDSState::clean_busy;
            }
            break;
        }
        case Main_Cmd::CLEAN_CANCEL: {
            m_device_state &= ~StageDSState::clean_busy;
            co_await m_clean->CancelClean(guard());
            break;
        }
        case Main_Cmd::AUTOFOCUS_START: {
            if (not(m_device_state & StageDSState::focus_busy)) {
                m_device_state |= StageDSState::focus_busy;
                co_await m_auto_focus->StartAutoFocus(guard(), false);
                m_device_state &= ~StageDSState::focus_busy;
            } else {
                co_await m_auto_focus->ConfirmWater(guard());
            }
            break;
        }

        case Main_Cmd::AUTOFOCUS_CANCEL: {
            m_device_state &= ~StageDSState::focus_busy;
            co_await m_auto_focus->CancelAutoFocus(guard());
            break;
        }
        case Main_Cmd::AUTOMODE_START: {
            if (not(m_device_state & StageDSState::auto_busy)) {
                m_device_state |= StageDSState::auto_busy;
                co_await m_auto->StartAutoMode(guard(),false);
                m_device_state &= ~StageDSState::auto_busy;
            }
            break;
        }
        case Main_Cmd::AUTOMODE_CANCEL: {
            m_device_state &= ~StageDSState::auto_busy;
            co_await m_auto->CancelAutoMode(guard());
            break;
        }
        case Main_Cmd::AUTOMODE_RECORD: {
            if (not(m_device_state & StageDSState::auto_busy)) {
                m_device_state |= StageDSState::auto_busy;
                co_await m_auto->StartAutoMode(guard(),true);
                m_device_state &= ~StageDSState::auto_busy;
            }
            break;
        }
    }
    co_return;
}

asio::awaitable<void>
StageAgent::SetLED(async::Lifeguard guard,
                   LED_Ch ch,
                   LED_Cmd cmd,
                   uint32_t value)
{
    (void)ch; // Unused parameter, but kept for compatibility
    switch (cmd) {
        case LED_Cmd::LED_CH:
            co_await m_led->SetLEDCh(guard(), value);
            break;
        case LED_Cmd::LED_ON:
            co_await m_led->SetLEDOn(guard(), value);
            break;
        case LED_Cmd::LED_PERIOD:
            co_await m_led->SetLEDPeriod(guard(), value);
            break;
        case LED_Cmd::LED_CAM_OFF:
            co_await m_led->SetLEDCamOff(guard(), value);
            break;
    }
    co_return;
}

asio::awaitable<void>
StageAgent::TaskStatus(async::Lifeguard guard)
{
    static bool changedState = false;
    auto timer = ds::async::Timer();
    while (true) {
        co_await m_cond.AsyncWait(guard(), [this]() { return not m_stop; });

        if (m_device_state & StageDSState::clean_busy) {
            m_progress = m_clean->GetProgress();
            m_progress_label = m_clean->GetLabel();
            changedState = true;
            m_status_label = "  Clean";
            m_warning_label = "";
        } else if (m_device_state & StageDSState::focus_busy){
            m_progress = m_auto_focus->GetProgress();
            m_progress_label = m_auto_focus->GetLabel();
            changedState = true;
            m_status_label = "  Focus";
            m_warning_label = "";
        } else if (m_device_state & StageDSState::auto_busy){
            m_progress = m_auto->GetProgress();
            m_progress_label = m_auto->GetLabel();            
            changedState = true;
            m_status_label = "  Measure";
            m_warning_label = "";
        } else if (m_device_state & StageDSState::exposure) {
            m_status_label = "  Searching auto exposure time";  
            m_warning_label = "";
        } else if (m_device_state & (StageDSState::move_busy_y | StageDSState::move_busy_x)) {
            m_status_label = "  Moving";
            m_warning_label = "";
        }else
            {
            m_device_state = StageDSState::idle;
            if (changedState) {
                changedState = false;
                m_progress = 0;
            } else {
                m_progress_label = "";
            }
            m_status_label = "  Ready";
            if (m_auto->GetNeedRefocus())
                m_warning_label = "Auto Fousing is required!";
            else
                m_warning_label = "No warning";
        }
        Render();
        
        co_await timer.AsyncSleepFor(guard(), 50ms);
    }
    co_return;
}
asio::awaitable<void>
StageAgent::TaskEvent(async::Lifeguard guard)
{    
    auto timer = ds::async::Timer();
    while (true){
        switch (m_app_event) {
            case AppEvent::APP_EVT_CLEAN: {
                if (not(m_device_state & StageDSState::clean_busy)) {
                    m_device_state |= StageDSState::clean_busy;
                    co_await m_clean->StartClean(guard());
                    m_device_state &= ~StageDSState::clean_busy;
                }
            }
                m_app_event = AppEvent::APP_EVT_NO;
                break;
            case AppEvent::APP_EVT_FOCUS: {
                if (not(m_device_state & StageDSState::focus_busy)) {
                    m_device_state |= StageDSState::focus_busy;
                    co_await m_auto_focus->StartAutoFocus(guard(), false);
                    m_device_state &= ~StageDSState::focus_busy;
                } else {
                    co_await m_auto_focus->ConfirmWater(guard());
                }
            }
                m_app_event = AppEvent::APP_EVT_NO;
                break;
            case AppEvent::APP_EVT_AUTO:
            case AppEvent::APP_EVT_RECORD: {
                if (not(m_device_state & StageDSState::auto_busy)) {
                    m_device_state |= StageDSState::auto_busy;
                    if (m_app_event == AppEvent::APP_EVT_RECORD)
                        co_await m_auto->StartAutoMode(guard(),true);
                    else
                        co_await m_auto->StartAutoMode(guard(), false);
                    m_device_state &= ~StageDSState::auto_busy;
                }
            }
                m_app_event = AppEvent::APP_EVT_NO;
                break;
        }

        co_await timer.AsyncSleepFor(guard(), 100ms);
    }
    co_return;
}

asio::awaitable<void>
StageAgent::ShutDown(async::Lifeguard guard)
{
    co_await m_move->StopMove(guard(), MotorRole::mtr_y);
    co_await m_move->StopMove(guard(), MotorRole::mtr_x);
    co_await m_pump->StopPump(guard());
    co_return;
}

} // namespace ds