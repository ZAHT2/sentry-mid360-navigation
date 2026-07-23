#include "sentry_hardware/protocol.hpp"
#include <cstring>

namespace sentry_hardware
{

uint16_t crc16_calculate(const uint8_t * data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x0001) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
        }
    }
    return crc;
}

bool crc16_verify(const uint8_t * data, size_t total_len)
{
    if (total_len < 2) return false;
    uint16_t computed = crc16_calculate(data, total_len - 2);
    uint16_t received;
    memcpy(&received, data + total_len - 2, sizeof(uint16_t));
    return computed == received;
}

void crc16_fill(DownlinkFrame & frame)
{
    frame.crc16 = crc16_calculate(
        reinterpret_cast<const uint8_t *>(&frame),
        sizeof(frame) - sizeof(frame.crc16));
}

}  // namespace sentry_hardware
