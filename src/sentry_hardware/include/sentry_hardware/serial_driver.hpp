#pragma once
#include <string>
#include <cstdint>
#include <cstddef>
#include <termios.h>

namespace sentry_hardware
{

class SerialDriver
{
public:
    SerialDriver() = default;
    ~SerialDriver();

    bool open(const std::string & port, int baudrate);
    void close();
    bool isOpen() const { return fd_ >= 0; }

    // 返回实际写入字节数，失败返回-1
    int write(const uint8_t * data, size_t len);

    // 带超时的非阻塞读，返回实际读取字节数，超时返回0，失败返回-1
    int read(uint8_t * buf, size_t max_len, int timeout_ms = 10);

private:
    int fd_ = -1;

    speed_t toBaudrate(int baud);
};

}  // namespace sentry_hardware
