#pragma once

#include <unordered_set>
#include "async.h"
#include "ui.h"
#include "serial.h"

#include "stage_autofocus.h"
#include "stage_automode.h"
#include "stage_autoexposure.h"
#include "stage_base.h"
#include "stage_clean.h"
#include "stage_frame.h"
#include "stage_move.h"
#include "stage_pump.h"

namespace ds::depthscan {


enum struct Device_Cmd
{
    MOVE_UP,
    MOVE_DOWN,
    MOVE_STOP_X,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_STOP_Y,
    MOVE_HOME,
    AUTO_FOUCS,
    AUTO_EXPOSURE,
    SAVE_INIT,
    MOVE_UP_STOP,
    MOVE_DOWN_STOP,
    MOVE_LEFT_STOP,
    MOVE_RIGHT_STOP
};
enum struct Pump_Cmd
{
    PUMP_CCW,
    PUMP_CW,
    PUMP_STOP
};    
enum struct Main_Cmd
{
    CLEAN_START,
    CLEAN_CANCEL,
    AUTOFOCUS_START,
    AUTOFOCUS_CANCEL,
    AUTOMODE_START,
    AUTOMODE_CANCEL,
    AUTOMODE_RECORD
};
enum struct LED_Cmd
{
    LED_CH,
    LED_ON,
    LED_OFF,
    LED_PERIOD,
    LED_CAM_OFF
};
enum struct LED_Ch
{
    LED_CH1,
    LED_CH2,
    LED_CH3,
    LED_CH4
};
enum struct AppEvent : uint32_t
{
    APP_EVT_NO = 0,
    APP_EVT_CLEAN,
    APP_EVT_FOCUS,
    APP_EVT_AUTO,
    APP_EVT_RECORD
};
class StageAgent
  : public async::Model<StageAgent>
  , public async::
      SnapshotMixin<uint32_t, wxString, int, wxString, wxString, wxString>
{
    async::RawCondition m_cond;

public:
    StageAgent();
    ~StageAgent();
    
    void Initiate() noexcept override;
    void CommitState() noexcept override;

    /// <summary>
    asio::awaitable<void> TaskStatus(async::Lifeguard guard);
    asio::awaitable<void> TaskEvent(async::Lifeguard guard);
    asio::awaitable<void> ShutDown(async::Lifeguard guard);

    /// device focus UI
    asio::awaitable<void> DoFocusDevice(async::Lifeguard guard, Device_Cmd cmd);


    /// device pump UI
    asio::awaitable<void> DoPumpDeivce(async::Lifeguard guard,
                                       Pump_Cmd cmd,
                                       wxString flow);

   
    /// main UI
    asio::awaitable<void> DoMainDeivce(async::Lifeguard guard, Main_Cmd cmd);

    /// <LED control>
    asio::awaitable<void> SetLED(async::Lifeguard guard,
                                 LED_Ch ch,
                                 LED_Cmd cmd,           
                                 uint32_t value);

///  < App Event>
    void SetAppEvent(AppEvent event) { m_app_event = event; }

private: 
    bool m_stop; 

    std::shared_ptr<StagePump>      m_pump;
    std::shared_ptr<StageMove>      m_move;
    std::shared_ptr<StageAutoFocus> m_auto_focus;
    std::shared_ptr<StageFrame>     m_frame;
    std::shared_ptr<StageClean>     m_clean;
    std::shared_ptr<StageAutoMode>  m_auto;
    std::shared_ptr<StageAutoExposure> m_auto_exposure;
    std::shared_ptr<StageLED>       m_led;

    int m_progress;
    wxString m_progress_label;
    wxString m_position_label;
    wxString m_status_label;
    wxString m_warning_label;
    uint32_t m_device_state;

    AppEvent m_app_event;

};

} // namespace ds