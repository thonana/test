#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <tuple>
#include <variant>
#include <mutex>
#include "async.h"
#include "serial.h"
#include "ui.h"
#include "stage_settings.h"

namespace ds::depthscan {
using namespace std::literals::chrono_literals;

constexpr auto MICRO_STEP = 256;
constexpr auto POS_MAX    = 2000000;
constexpr auto POS_MIN    = -2000000;
constexpr auto CH_OFFSET  = 32;

class StageDSAddress
{
public:
    //< System 0x00000000 ~ 0x000000FF
    static constexpr uint32_t ADDR_SYS_BASE      = 0x00000000;
    static constexpr uint32_t ADDR_SYS_ID        = (ADDR_SYS_BASE + 0x00);
    static constexpr uint32_t ADDR_SYS_TIMESTAMP = (ADDR_SYS_BASE + 0x04);
    ; //< Depth Scan 0x00000100 ~ 0x000001FF
    static constexpr uint32_t ADDR_DS_BASE    = 0x00000100;
    static constexpr uint32_t ADDR_DS_VERSION = (ADDR_DS_BASE + 0x00);
    static constexpr uint32_t ADDR_DS_STAT    = (ADDR_DS_BASE + 0x04);
    static constexpr uint32_t ADDR_DS_POWER   = (ADDR_DS_BASE + 0x08);
    static constexpr uint32_t ADDR_DS_CONFIG  = (ADDR_DS_BASE + 0x0C);
    static constexpr uint32_t ADDR_DS_FREE0   = (ADDR_DS_BASE + 0x10);
    static constexpr uint32_t ADDR_DS_FREE1   = (ADDR_DS_BASE + 0x14);
    static constexpr uint32_t ADDR_DS_FREE2   = (ADDR_DS_BASE + 0x18);
    static constexpr uint32_t ADDR_DS_FREE3   = (ADDR_DS_BASE + 0x1C);
    static constexpr uint32_t ADDR_DS_LED_CH  = (ADDR_DS_BASE + 0x20);
    static constexpr uint32_t ADDR_DS_LED_HZ  = (ADDR_DS_BASE + 0x30);
    static constexpr uint32_t ADDR_DS_LED_CAM_OFF = (ADDR_DS_BASE + 0x34);
    static constexpr uint32_t ADDR_DS_LED_ON = (ADDR_DS_BASE + 0x38);
    static constexpr uint32_t ADDR_DS_LED_OFF = (ADDR_DS_BASE + 0x3C);
    //< Stepper 0 0x00000200 ~ 0x000002FF
    static constexpr uint32_t ADDR_STEPPER0_BASE = 0x00000200;
    static constexpr uint32_t ADDR_STEPPER0_STAT = (ADDR_STEPPER0_BASE + 0x00);
    static constexpr uint32_t ADDR_STEPPER0_CTRL = (ADDR_STEPPER0_BASE + 0x04);
    static constexpr uint32_t ADDR_STEPPER0_ELAPSED =
      (ADDR_STEPPER0_BASE + 0x10);
    static constexpr uint32_t ADDR_STEPPER0_POS_L = (ADDR_STEPPER0_BASE + 0x40);
    static constexpr uint32_t ADDR_STEPPER0_POS_H = (ADDR_STEPPER0_BASE + 0x44);
    static constexpr uint32_t ADDR_STEPPER0_ENC_L = (ADDR_STEPPER0_BASE + 0x60);
    static constexpr uint32_t ADDR_STEPPER0_ENC_H = (ADDR_STEPPER0_BASE + 0x64);

    static constexpr uint32_t ADDR_STEPPER0_CONF  = (ADDR_STEPPER0_BASE + 0x80);
    static constexpr uint32_t ADDR_STEPPER0_ENC   = (ADDR_STEPPER0_BASE + 0x84);
    static constexpr uint32_t ADDR_STEPPER0_MODE  = (ADDR_STEPPER0_BASE + 0x88);
    static constexpr uint32_t ADDR_STEPPER0_ACCEL = (ADDR_STEPPER0_BASE + 0x8C);
    static constexpr uint32_t ADDR_STEPPER0_INIT  = (ADDR_STEPPER0_BASE + 0x90);
    static constexpr uint32_t ADDR_STEPPER0_LAST  = (ADDR_STEPPER0_BASE + 0x94);
    static constexpr uint32_t ADDR_STEPPER0_DEST_L =
      (ADDR_STEPPER0_BASE + 0x98);
    static constexpr uint32_t ADDR_STEPPER0_DEST_H =
      (ADDR_STEPPER0_BASE + 0x9C);
};
class StageDSState
{
public:
    static constexpr uint32_t idle        = 0b00000001;
    static constexpr uint32_t home_busy   = 0b00000010;
    static constexpr uint32_t home_idle   = 0b00000100;
    static constexpr uint32_t focus_busy  = 0b00001000;
    static constexpr uint32_t focus_idle  = 0b00010000;
    static constexpr uint32_t move_busy_x = 0b00100000;
    static constexpr uint32_t move_idle_x = 0b01000000;
    static constexpr uint32_t move_busy_y = 0b10000000;
    static constexpr uint32_t move_idle_y = 0b100000000;
    static constexpr uint32_t pump_busy   = 0b1000000000;
    static constexpr uint32_t pump_idle   = 0b10000000000;
    static constexpr uint32_t clean_busy  = 0b100000000000;
    static constexpr uint32_t clean_idle  = 0b1000000000000;
    static constexpr uint32_t auto_busy   = 0b10000000000000;
    static constexpr uint32_t auto_idle   = 0b100000000000000;
    static constexpr uint32_t home_done   = 0b1000000000000000;
    static constexpr uint32_t exposure    = 0b10000000000000000;
};

class MotorRole
{
public:
    static constexpr uint8_t mtr_1   = 0;
    static constexpr uint8_t mtr_2   = 1;
    static constexpr uint8_t mtr_3   = 2;
    static constexpr uint8_t mtr_4   = 3;
    static constexpr uint8_t mtr_5   = 4;
    static constexpr uint8_t mtr_x   = mtr_1;
    static constexpr uint8_t mtr_y   = mtr_2;
    static constexpr uint8_t mtr_p   = mtr_4;
    static constexpr uint8_t mtr_t   = mtr_3;
    static constexpr uint8_t mtr_r   = mtr_5;
    static constexpr uint8_t mtr_max = mtr_5 + 1;
};
class SingleMode
{
public:
    static constexpr uint8_t mode_inf  = 0;
    static constexpr uint8_t mode_rel  = 1;
    static constexpr uint8_t mode_cnt  = 2;
    static constexpr uint8_t mode_enc  = 3;
    static constexpr uint8_t mode_home = 4;
    static constexpr uint8_t mode_max  = mode_home + 1;
};

class XMotorConfig
{
public:
    static constexpr uint8_t mask_upper     = 0x4;
    static constexpr uint8_t mask_home      = 0x1;
    static constexpr uint8_t mask_lower     = 0x2;
    static constexpr uint8_t mask_alarm     = 0x8;
    static constexpr uint8_t mask_limit_xor = 0xf;
    static constexpr uint8_t delay_x        = 0x8;
    static constexpr uint8_t puls_x         = 0x0;
    static constexpr uint8_t dir_x          = 0x0;
    static constexpr uint8_t onep_x         = 0x1;
};

class YMotorConfig
{
public:
    static constexpr uint8_t mask_upper     = 0x4;
    static constexpr uint8_t mask_home      = 0x1;
    static constexpr uint8_t mask_lower     = 0x2;
    static constexpr uint8_t mask_alarm     = 0x8;
    static constexpr uint8_t mask_limit_xor = 0xf;
    static constexpr uint8_t delay_x        = 0x8;
    static constexpr uint8_t puls_x         = 0x0;
    static constexpr uint8_t dir_x          = 0x0;
    static constexpr uint8_t onep_x         = 0x1;
};

class PumpConfig
{
public:
    static constexpr uint8_t mask_upper     = 0x4;
    static constexpr uint8_t mask_home      = 0x1;
    static constexpr uint8_t mask_lower     = 0x2;
    static constexpr uint8_t mask_alarm     = 0x8;
    static constexpr uint8_t mask_limit_xor = 0x8;
    static constexpr uint8_t delay_x        = 0x8;
    static constexpr uint8_t puls_x         = 0x0;
    static constexpr uint8_t dir_x          = 0x0;
    static constexpr uint8_t onep_x         = 0x1;
};
class MotorCon
{
public:
    static constexpr uint8_t ctrl_start = 0;
    static constexpr uint8_t ctrl_stop  = 8;
};
class MotorStat
{
public:
    static constexpr uint8_t stat_busy = 0;
    static constexpr uint8_t stat_home = 12;
};
class MotorDir
{
public:
    static constexpr uint8_t motdir_cw      = 0x1;
    static constexpr uint8_t motdir_ccw     = 0x2;
    static constexpr uint8_t pump_dir_clean = motdir_ccw;
    static constexpr uint8_t pump_dir_prime = motdir_ccw;
};

class Stage
  : public async::Model<Stage>
  , public StageDSState
  , public MotorRole
  , public SingleMode
  , public MotorStat
  , public MotorCon
  , public MotorDir
  , public StageDSAddress
{
    ds::async::RawCondition m_cond;

public:
    Stage();
    ~Stage();

    void                  Initiate() noexcept override;
    asio::awaitable<void> InitMotorConfig(async::Lifeguard guard);
    asio::awaitable<void> MoveHome(async::Lifeguard guard);
    asio::awaitable<void> MoveInitPos(async::Lifeguard guard);
    asio::awaitable<void> MoveLastPos(async::Lifeguard guard,int x,int y);

    asio::awaitable<void> SingleMode(async::Lifeguard guard,
                                     uint8_t mtr,
                                     uint8_t  mode,
                                     uint32_t retry,
                                     uint32_t bound);
    asio::awaitable<void> Go(async::Lifeguard guard,
                                uint8_t mtr,
                                uint8_t  dir,
                                uint8_t  mode,
                                uint32_t retry,
                                uint32_t bound,
                                uint32_t speed);
    asio::awaitable<void> Stop(async::Lifeguard guard,uint8_t mtr);
    int                   ReadStatus(uint8_t mtr) const;
    asio::awaitable<void> SetDir(async::Lifeguard guard,uint8_t mtr,
                                 uint8_t dir);
    asio::awaitable<void> SetDriveSpeed(async::Lifeguard guard,uint8_t mtr,
                                        uint32_t speed);
    asio::awaitable<bool> GetNotBusy(async::Lifeguard guard);
    asio::awaitable<bool> IsHome(async::Lifeguard guard,uint8_t mtr);
    asio::awaitable<void> SetPos(async::Lifeguard guard,uint8_t mtr,
                                 long long pos);
    asio::awaitable<int> GetPos(async::Lifeguard guard,uint8_t mtr);

    asio::awaitable<void> ValidCheck(async::Lifeguard guard);

    asio::awaitable<void> SetLEDCh(async::Lifeguard guard, uint16_t value)
    {
        if (m_avalon) {
            uint32_t base = (0xC0u << 16);
            int ch = 17; // channel
            int ld = 21; // load
            uint32_t setValue = base | (2 << ch) | (1 << ld) | value;
            co_await m_avalon->AsyncWrite(
              guard(), ADDR_DS_LED_CH, { setValue - 1u }); // 3 ch
        }
        co_return;
    }
    asio::awaitable<void> SetLEDPeriod(async::Lifeguard guard, uint32_t value)
    {
        if (m_avalon) {         
            double setValue = 32000000 * (value / 1000.0) ;

            co_await m_avalon->AsyncWrite(
              guard(), ADDR_DS_LED_HZ, { uint32_t(setValue) - 1u });
        }
        co_return;
    }
    asio::awaitable<void> SetLEDCamOff(async::Lifeguard guard, uint32_t value)
    {
        if (m_avalon) {
            double setValue = 32000000 * (value / 1000.0);
            co_await m_avalon->AsyncWrite(
              guard(), ADDR_DS_LED_CAM_OFF, { uint32_t(setValue) - 1u });
        }
        co_return;
    }
    asio::awaitable<void> SetLEDOn(async::Lifeguard guard, uint32_t value)
    {
        if (m_avalon) {
            double ontime = 32000000 * (value / 1000.0);
            co_await m_avalon->AsyncWrite(
              guard(), ADDR_DS_LED_ON, { uint32_t(ontime) - 1u });
            co_await m_avalon->AsyncWrite(
              guard(), ADDR_DS_LED_OFF, { uint32_t(ontime + 64.0) - 1u });
        }
        co_return;
    }


    unsigned int LoadSavedPowerMode(void) const { return m_saved_power; }
    void SavePowerMode(unsigned int bit_mode) { m_saved_power = bit_mode; }

protected:

    asio::awaitable<void> InitDacConfig(async::Lifeguard guard,auto& dac);
    asio::awaitable<void> SetAccel(async::Lifeguard guard,
                                    uint8_t mtr,
                                    uint32_t accel);
    asio::awaitable<void> SetInitSpeed(async::Lifeguard guard,uint8_t mtr,
                                       uint32_t speed);
    asio::awaitable<void> SetMotorConfig(async::Lifeguard guard,uint8_t mtr,
                                         uint8_t mask_upper,
                                         uint8_t mask_home,
                                         uint8_t mask_lower,
                                         uint8_t maks_alarm,
                                         uint8_t limit_xor,
                                         uint8_t delay,
                                         uint8_t pulx,
                                         uint8_t dirx,
                                         uint8_t onepulse);
    asio::awaitable<void> PowerOn(async::Lifeguard guard,uint8_t mtr);
    asio::awaitable<void> PowerOff(async::Lifeguard guard,uint8_t mtr);
    void SetInitPos(const int x, const int y) { 
        m_init_x_pos = x;
        m_init_y_pos = y;
    }

private:
    uint32_t                m_state;
    uint32_t                m_saved_power;
    int                     m_init_x_pos;  
    int                     m_init_y_pos;   
    int                     m_last_x_pos;
    int                     m_last_y_pos; 
    std::map<int, int>      m_dac; // dac motor config
    asio::awaitable<void> InitConfig(async::Lifeguard guard);

    std::shared_ptr<serial::Avalon> m_avalon;  
    std::string m_camera_name;

};

} // namespace ds