#include "sentry_hardware/serial_driver.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <cstring>
#include <stdexcept>

namespace sentry_hardware
{

SerialDriver::~SerialDriver()
{
    close();
}

bool SerialDriver::open(const std::string & port, int baudrate)
{
    fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        return false;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd_, &tty) != 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    speed_t speed = toBaudrate(baudrate);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    // 8N1，无流控
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~(OPOST | ONLCR);

    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN]  = 0;

    tcsetattr(fd_, TCSANOW, &tty);
    tcflush(fd_, TCIOFLUSH);
    return true;
}

void SerialDriver::close()
{
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

int SerialDriver::write(const uint8_t * data, size_t len)
{
    if (fd_ < 0) return -1;
    return static_cast<int>(::write(fd_, data, len));
}

int SerialDriver::read(uint8_t * buf, size_t max_len, int timeout_ms)
{
    if (fd_ < 0) return -1;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd_, &read_fds);

    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(fd_ + 1, &read_fds, nullptr, nullptr, &tv);
    if (ret < 0) return -1;
    if (ret == 0) return 0;  // timeout

    return static_cast<int>(::read(fd_, buf, max_len));
}

speed_t SerialDriver::toBaudrate(int baud)
{
    switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 921600: return B921600;
        default:     return B115200;
    }
}

}  // namespace sentry_hardware
