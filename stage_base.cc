
#include <iostream>

#include "serial.h"
#include "stage_base.h"
#include "stage_settings.h"
#include <spdlog/spdlog.h>

namespace ds::depthscan {

static int
ConvertVolt2Dac(double voltage)
{
    double dac = (voltage / 3.3) * 255; // 8bit
    return (int)dac;
}
static void
LoadMotorConfigDac(auto& dac)
{
    dac.insert(std::pair<int, int>(1, ConvertVolt2Dac(3.3)));  // 3.3V
    dac.insert(std::pair<int, int>(2, ConvertVolt2Dac(2.0)));  // 2.0V
    dac.insert(std::pair<int, int>(3, ConvertVolt2Dac(0.0)));  // 0V
    dac.insert(std::pair<int, int>(4, ConvertVolt2Dac(0.0)));  // 0V
    dac.insert(std::pair<int, int>(5, ConvertVolt2Dac(0.0)));  // 0V
    dac.insert(std::pair<int, int>(6, ConvertVolt2Dac(2.0)));  // 3.3V
    dac.insert(std::pair<int, int>(7, ConvertVolt2Dac(3.3)));  // 3.3V
    dac.insert(std::pair<int, int>(8, ConvertVolt2Dac(2.0)));  // 2.0V
    dac.insert(std::pair<int, int>(9, ConvertVolt2Dac(0.0)));  // 0V
    dac.insert(std::pair<int, int>(10, ConvertVolt2Dac(0.0))); // 0V
    dac.insert(std::pair<int, int>(11, ConvertVolt2Dac(0.0))); // 0V
    dac.insert(std::pair<int, int>(12, ConvertVolt2Dac(2.0))); // 3.3V
}

static std::tuple<uint8_t,
                  uint8_t,
                  uint8_t,
                  uint8_t,
                  uint8_t,
                  uint8_t,
                  uint8_t,
                  uint8_t,
                  uint8_t>
GetMotorConfig(uint8_t mtr)
{
    switch (mtr) {
        case (MotorRole::mtr_x): {
            return std::make_tuple(XMotorConfig::mask_upper,
                                   XMotorConfig::mask_home,
                                   XMotorConfig::mask_lower,
                                   XMotorConfig::mask_alarm,
                                   XMotorConfig::mask_limit_xor,
                                   XMotorConfig::delay_x,
                                   XMotorConfig::puls_x,
                                   XMotorConfig::dir_x,
                                   XMotorConfig::onep_x);
            break;
        }
        case (MotorRole::mtr_y): {
            return std::make_tuple(YMotorConfig::mask_upper,
                                   YMotorConfig::mask_home,
                                   YMotorConfig::mask_lower,
                                   YMotorConfig::mask_alarm,
                                   YMotorConfig::mask_limit_xor,
                                   YMotorConfig::delay_x,
                                   YMotorConfig::puls_x,
                                   YMotorConfig::dir_x,
                                   YMotorConfig::onep_x);
            break;
        }
        case (MotorRole::mtr_p): {
            return std::make_tuple(PumpConfig::mask_upper,
                                   PumpConfig::mask_home,
                                   PumpConfig::mask_lower,
                                   PumpConfig::mask_alarm,
                                   PumpConfig::mask_limit_xor,
                                   PumpConfig::delay_x,
                                   PumpConfig::puls_x,
                                   PumpConfig::dir_x,
                                   PumpConfig::onep_x);
            break;
        }
        default: {
            return std::make_tuple(XMotorConfig::mask_upper,
                                   XMotorConfig::mask_home,
                                   XMotorConfig::mask_lower,
                                   XMotorConfig::mask_alarm,
                                   XMotorConfig::mask_limit_xor,
                                   XMotorConfig::delay_x,
                                   XMotorConfig::puls_x,
                                   XMotorConfig::dir_x,
                                   XMotorConfig::onep_x);
            break;
        }
    }
}

Stage::Stage()
  : m_state(idle)
  , m_saved_power(0)
  , m_init_x_pos(0)
  , m_init_y_pos(0)
  , m_last_x_pos(0)
  , m_last_y_pos(0)
  , m_avalon(nullptr)
  , m_camera_name("U3-300xSE-C")

{
    auto storage = StageSettingStorage::GetInstance();
    
    if (storage) {
        storage->LoadSettingsFromJson(m_camera_name);
        m_init_x_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::INIT_X_POS));
        m_init_y_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::INIT_Y_POS));
        m_last_x_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::LAST_X_POS));
        m_last_y_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::LAST_Y_POS));
    }
    // if you dont want to call this,
    //  remove this line
    m_avalon = serial::GetAvalon("main");
        
    LoadMotorConfigDac(m_dac);
}

Stage::~Stage()
{
}
void
Stage::Initiate() noexcept
{
    static bool called = false;
    if (m_avalon and not called) {
        called = true;
        //if you dont want to call this,
        // remove this line
        Start<&Stage::ValidCheck>(NewLife());
    }
}

asio::awaitable<void>
Stage::InitConfig(async::Lifeguard guard)
{
    co_await SetLEDCh(guard(), 65535);           // all on
    co_await SetLEDPeriod(guard(), 20);         // 20ms, 50FPS
    co_await SetLEDCamOff(guard(), 1);          // 1ms
    co_await SetLEDOn(guard(), 1);              // 1ms

    co_await InitDacConfig(guard(), m_dac);
    co_await InitMotorConfig(guard());
    co_await MoveHome(guard());
    co_await MoveLastPos(guard(),m_last_x_pos,m_last_y_pos);

    auto storage = StageSettingStorage::GetInstance();
    if (storage->GetCameraName() != "U3-300xSE-C") {
        m_camera_name = storage->GetCameraName();
        storage->LoadSettingsFromJson(m_camera_name);
        m_init_x_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::INIT_X_POS));
        m_init_y_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::INIT_Y_POS));
        m_last_x_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::LAST_X_POS));
        m_last_y_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::LAST_Y_POS));
        co_await MoveLastPos(guard(), m_last_x_pos, m_last_y_pos);
    }


    co_return;
}
asio::awaitable<void>
Stage::InitDacConfig(async::Lifeguard guard,auto& dac)
{
    uint32_t              data    = 0;
    uint32_t              address = ADDR_DS_CONFIG;
    std::vector<uint32_t> writeBuffer;

    for (auto iter = dac.begin(); iter != dac.end(); iter++) {
        data = (unsigned int)(0xf << 12) | (unsigned int)(iter->first << 8) |
               (unsigned int)(iter->second & 0xff);

        writeBuffer.clear();
        writeBuffer.push_back(data);
        co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    }
    co_return;
}
asio::awaitable<void>
Stage::SetAccel(async::Lifeguard guard,uint8_t mtr, uint32_t accel)
{
    std::vector<uint32_t> writeBuffer;
    uint32_t              data    = accel;
    uint32_t              address = ADDR_STEPPER0_ACCEL + CH_OFFSET * mtr;

    if (0 == data)
        data = 1;

    writeBuffer.push_back(data);
    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    co_return;
}
asio::awaitable<void>
Stage::SetInitSpeed(async::Lifeguard guard,uint8_t mtr, uint32_t speed)
{
    std::vector<uint32_t> writeBuffer;
    uint32_t              data    = speed;
    uint32_t              address = ADDR_STEPPER0_INIT + CH_OFFSET * mtr;

    if (0 == data)
        data = 1;

    writeBuffer.push_back(data);
    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    co_return;
}

asio::awaitable<void>
Stage::SetMotorConfig(async::Lifeguard guard,
                      uint8_t mtr,
                      uint8_t mask_upper,
                      uint8_t mask_home,
                      uint8_t mask_lower,
                      uint8_t maks_alarm,
                      uint8_t limit_xor,
                      uint8_t delay,
                      uint8_t pulx,
                      uint8_t dirx,
                      uint8_t onepulse)
{
    std::vector<uint32_t> writeBuffer;
    uint32_t              data    = 0;
    uint32_t              address = ADDR_STEPPER0_CONF + CH_OFFSET * mtr;

    data = (unsigned int)(mask_upper) | (unsigned int)(mask_home << 4) |
           (unsigned int)(mask_lower << 8) | (unsigned int)(maks_alarm << 12) |
           (unsigned int)(limit_xor << 16) | (unsigned int)(delay << 20) |
           (unsigned int)(pulx << 28) | (unsigned int)(dirx << 29) |
           (unsigned int)(onepulse << 31);

    writeBuffer.push_back(data);
    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    co_return;
}

asio::awaitable<void>
Stage::InitMotorConfig(async::Lifeguard guard)
{
    uint8_t i = 0;

    for (i = MotorRole::mtr_1; i < MotorRole::mtr_max; i++) {

        auto [upper, home, lower, alarm, limit_xor, delay, pulx, dirx, one] =
          GetMotorConfig(i);
        co_await SetMotorConfig(guard(),
          i, upper, home, lower, alarm, limit_xor, delay, pulx, dirx, one);
        co_await SetAccel(guard(), i, 200 * MICRO_STEP);
        co_await SetInitSpeed(guard(), i, 200 * MICRO_STEP);
        co_await SetDriveSpeed(guard(), i, 200 * MICRO_STEP);
        co_await Stop(guard(), i);
    }
    co_return;
}
asio::awaitable<void>
Stage::MoveHome(async::Lifeguard guard)
{
    // Move to home position
    co_await SetPos(guard(), MotorRole::mtr_x, 0);
    co_await SetPos(guard(), MotorRole::mtr_y, 0);
    co_await Go(guard(),
                   MotorRole::mtr_x,
                            MotorDir::motdir_cw,
                            SingleMode::mode_home,
                            0,
                            0,
                            200 * MICRO_STEP);
    co_await Go(guard(),
                   MotorRole::mtr_y,
                            MotorDir::motdir_cw,
                            SingleMode::mode_home,
                            0,
                            0,
                            200 * MICRO_STEP);

    bool done = false;
    auto timer = ds::async::Timer();
    while (not done)
    {
        done = co_await IsHome(guard(), MotorRole::mtr_x | MotorRole::mtr_y);
        co_await timer.AsyncSleepFor(guard(), 1ms);
    }
    co_return;
}

asio::awaitable<void>
Stage::MoveInitPos(async::Lifeguard guard)
{
    auto storage = StageSettingStorage::GetInstance();
    if (storage) {
        storage->LoadSettingsFromJson(m_camera_name);
        m_init_x_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::INIT_X_POS));
        m_init_y_pos =
          std::get<int>(storage->GetSettings(StageConfigKeys::INIT_Y_POS));
    }
    co_await SetPos(guard(), MotorRole::mtr_x, m_init_x_pos);
    co_await SetPos(guard(), MotorRole::mtr_y, m_init_y_pos);
    co_await Go(guard(),
                MotorRole::mtr_x,
                MotorDir::motdir_cw,
                SingleMode::mode_cnt,
                0,
                0,
                200 * MICRO_STEP);
    co_await Go(guard(),
                MotorRole::mtr_y,
                MotorDir::motdir_cw,
                SingleMode::mode_cnt,
                0,
                0,
                200 * MICRO_STEP);
    bool done = false;
    while (not done) {
        done = co_await GetNotBusy(guard());
    }
}

asio::awaitable<void>
Stage::MoveLastPos(async::Lifeguard guard,int x, int y)
{
    co_await SetPos(guard(), MotorRole::mtr_x, x);
    co_await SetPos(guard(), MotorRole::mtr_y, y);
    co_await Go(guard(),
                MotorRole::mtr_x,
                MotorDir::motdir_cw,
                SingleMode::mode_cnt,
                0,
                0,
                200 * MICRO_STEP);
    co_await Go(guard(),
                MotorRole::mtr_y,
                MotorDir::motdir_cw,
                SingleMode::mode_cnt,
                0,
                0,
                200 * MICRO_STEP);
    bool done = false;
    while (not done) {
        done = co_await GetNotBusy(guard());
    }
}
asio::awaitable<void>
Stage::PowerOn(async::Lifeguard guard,uint8_t mtr)
{
    std::vector<uint32_t> writeBuffer;
    uint32_t              address     = ADDR_DS_POWER;
    uint32_t              saved_power = LoadSavedPowerMode();

    saved_power |= (1 << mtr);
    SavePowerMode(saved_power);
    writeBuffer.push_back(saved_power);

    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    co_return;
}
asio::awaitable<void>
Stage::PowerOff(async::Lifeguard guard,uint8_t mtr)
{
    std::vector<uint32_t> writeBuffer;
    uint32_t              address     = ADDR_DS_POWER;
    uint32_t              saved_power = LoadSavedPowerMode();

    saved_power &= ~(1 << mtr);
    SavePowerMode(saved_power);
    writeBuffer.push_back(saved_power);

    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    co_return;
}

asio::awaitable<void>
Stage::SingleMode(async::Lifeguard guard,uint8_t mtr,
                  uint8_t mode,
                  uint32_t retry,
                  uint32_t bound)
{
    std::vector<uint32_t> writeBuffer;
    uint32_t              address = ADDR_STEPPER0_MODE + CH_OFFSET * mtr;
    uint32_t              data    = 0;

    if (retry > 0xF)
        retry = 0xF;
    if (bound > 0xFFFF)
        bound = 0xFFFF;

    data = (unsigned int)(mode) | (unsigned int)(retry << 4) |
           (unsigned int)(bound << 16);

    writeBuffer.push_back(data);

    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    co_return;
}
asio::awaitable<void>
Stage::Go(async::Lifeguard guard,
             uint8_t mtr,
             uint8_t  dir,
             uint8_t  mode,
             uint32_t retry,
             uint32_t bound,
             uint32_t speed)
{
    co_await PowerOn(guard(), mtr);
    co_await SingleMode(guard(), mtr, mode, retry, bound);
    //co_await SetAccel(guard,mtr, 100 * MICRO_STEP);
    co_await SetInitSpeed(guard(), mtr, speed);
    co_await SetDriveSpeed(guard(), mtr, speed);
    if (mtr == mtr_p)
        co_await SetDir(guard(), mtr, dir);

    std::vector<uint32_t> writeBuffer;
    uint32_t              address = ADDR_STEPPER0_CTRL;
    uint32_t              data    = ((1 << ctrl_start) << mtr);

    writeBuffer.push_back(data);

    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    co_return;
}
asio::awaitable<void>
Stage::Stop(async::Lifeguard guard,uint8_t mtr)
{
    co_await PowerOff(guard(), mtr);
    std::vector<uint32_t> writeBuffer;
    uint32_t              address = ADDR_STEPPER0_CTRL;
    uint32_t              data    = ((1 << ctrl_stop) << mtr);

    writeBuffer.push_back(data);
    spdlog::info("cmd stop: {}", mtr);
    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    co_return;
}
int
Stage::ReadStatus([[maybe_unused]] uint8_t mtr) const
{
    return false;
}
asio::awaitable<void>
Stage::SetDir(async::Lifeguard guard,uint8_t mtr, uint8_t dir)
{
    auto [upper, home, lower, alarm, limit_xor, delay, pulx, dirx, one] =
      GetMotorConfig(mtr);

    if (dir == motdir_cw)
        co_await SetMotorConfig(guard(),
          mtr, upper, home, lower, alarm, limit_xor, delay, pulx, 0, one);
    else
        co_await SetMotorConfig(guard(),
          mtr, upper, home, lower, alarm, limit_xor, delay, pulx, 1, one);

    co_return;
}
asio::awaitable<void>
Stage::SetDriveSpeed(async::Lifeguard guard,uint8_t mtr, uint32_t speed)
{
    std::vector<uint32_t> writeBuffer;
    uint32_t              address = ADDR_STEPPER0_LAST + CH_OFFSET * mtr;
    uint32_t              data    = speed;

    if (0 == data)
        data = 1;

    writeBuffer.push_back(data);

    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    co_return;
}
asio::awaitable<bool>
Stage::GetNotBusy(async::Lifeguard guard)
{
    bool result = false;

    std::vector<uint32_t> data =
      co_await m_avalon->AsyncRead(guard(), ADDR_STEPPER0_STAT, 1);
    if (not data.empty()) {
        uint32_t mot_mask = (1u << mtr_x) | (1u << mtr_y);
        uint32_t bit_mask = ((mot_mask << stat_busy) & 0x1f);
        uint32_t status   = (data.at(0) & bit_mask) >> stat_busy;
        if (status & mot_mask)
            result = false;
        else
            result = true;
    } else {
        result = false;
    }

    co_return result;
}

asio::awaitable<bool>
Stage::IsHome(async::Lifeguard guard,uint8_t mtr)
{
    std::vector<uint32_t> data =
      co_await m_avalon->AsyncRead(guard(), ADDR_STEPPER0_STAT, 1);
    if (not data.empty()) {
        uint32_t mot_mask = (1 << mtr_x) | (1 << mtr_y);
        uint32_t bit_mask = ((mot_mask << stat_home) & 0xf000u);
        uint32_t status   = (data.at(0) & bit_mask) >> stat_home;
        if (status == mot_mask)
            co_return true;
        else
            co_return false;
    }
    co_return false;
}
asio::awaitable<void>
Stage::SetPos(async::Lifeguard guard,uint8_t mtr, long long pos)
{
    std::vector<uint32_t> writeBuffer;
    uint32_t              address;
    uint32_t              pos_L = (pos & 0xFFFFFFFF);
    uint32_t              pos_H = (pos >> 32) & 0xFFFFFFFF;

    address = ADDR_STEPPER0_DEST_L + CH_OFFSET * mtr;
    writeBuffer.push_back(pos_L);
    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);

    writeBuffer.clear();
    address = ADDR_STEPPER0_DEST_H + CH_OFFSET * mtr;
    writeBuffer.push_back(pos_H);
    co_await m_avalon->AsyncWrite(guard(), address, writeBuffer);
    co_return;
}

asio::awaitable<int>
Stage::GetPos(async::Lifeguard guard,uint8_t mtr)
{
    uint32_t address;

    address = ADDR_STEPPER0_POS_L + 8 * mtr;
    std::vector<uint32_t> data;
    bool                  done = false;
    while (not done) {
        data = co_await m_avalon->AsyncRead(guard(), address, 1);
        if (data.size()) {
            done = true;
            co_return data.at(0);
        }
    }
}
asio::awaitable<void>
Stage::ValidCheck(async::Lifeguard guard)
{
    co_await m_avalon->AsyncPing(guard());
    std::vector<uint32_t> data =
      co_await m_avalon->AsyncRead(guard(), ADDR_SYS_BASE, 1);

    if (not data.empty()) {
        if (data.at(0) == 0xabcd1234) {
            co_await InitConfig(guard());
        }
    }
    
    co_return;
}
} // ds namespace