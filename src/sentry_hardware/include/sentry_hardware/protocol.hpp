#pragma once
#include <cstdint>
#include <cstddef>

namespace sentry_hardware
{

static constexpr uint8_t FRAME_SOF      = 0xA5;
static constexpr uint8_t CMD_UPLINK     = 0x01;  // C板 → MiniPC
static constexpr uint8_t CMD_DOWNLINK   = 0x02;  // MiniPC → C板

#pragma pack(push, 1)

// 24 bytes: C板上报底盘状态
struct UplinkFrame {
    uint8_t  sof    = FRAME_SOF;
    uint8_t  cmd_id = CMD_UPLINK;
    float    vx;       // 当前x速度 m/s（底盘坐标系）
    float    vy;       // 当前y速度 m/s
    float    yaw;      // 全局航向角 rad（IMU融合后）
    float    pos_x;    // 全局位置x m（里程计融合后）
    float    pos_y;    // 全局位置y m
    uint16_t crc16;    // CRC覆盖 sof~pos_y
};
static_assert(sizeof(UplinkFrame) == 24, "UplinkFrame size mismatch");

// 16 bytes: MiniPC下发速度指令
struct DownlinkFrame {
    uint8_t  sof    = FRAME_SOF;
    uint8_t  cmd_id = CMD_DOWNLINK;
    float    vx;       // 目标x速度 m/s
    float    vy;       // 目标y速度 m/s
    float    wz;       // 目标角速度 rad/s
    uint16_t crc16;
};
static_assert(sizeof(DownlinkFrame) == 16, "DownlinkFrame size mismatch");

#pragma pack(pop)

// CRC-16/IBM (poly=0xA001, init=0xFFFF)
uint16_t crc16_calculate(const uint8_t * data, size_t len);

// 校验整帧（CRC字段是帧的最后2字节）
bool crc16_verify(const uint8_t * data, size_t total_len);

// 填充DownlinkFrame的CRC字段
void crc16_fill(DownlinkFrame & frame);

}  // namespace sentry_hardware
