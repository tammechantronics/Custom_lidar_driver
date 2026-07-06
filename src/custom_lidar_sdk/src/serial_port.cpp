#include "custom_lidar_sdk/serial_port.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace custom_lidar_sdk
{

SerialPort::~SerialPort()
{
  closePort();
}

speed_t SerialPort::baudToTermios(int baudrate)
{
  switch (baudrate) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
#ifdef B460800
    case 460800: return B460800;
#endif
#ifdef B500000
    case 500000: return B500000;
#endif
#ifdef B921600
    case 921600: return B921600;
#endif
    default: return B115200;
  }
}

bool SerialPort::openPort(const std::string & port, int baudrate)
{
  closePort();

  fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd_ < 0) {
    return false;
  }

  if (!configurePort(baudrate)) {
    closePort();
    return false;
  }

  // Sau khi cấu hình, đưa về blocking mode. Đọc có timeout bằng select().
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags >= 0) {
    fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK);
  }

  tcflush(fd_, TCIOFLUSH);
  return true;
}

bool SerialPort::configurePort(int baudrate)
{
  if (fd_ < 0) {
    return false;
  }

  struct termios tty;
  std::memset(&tty, 0, sizeof(tty));

  if (tcgetattr(fd_, &tty) != 0) {
    return false;
  }

  cfmakeraw(&tty);

  speed_t speed = baudToTermios(baudrate);
  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  tty.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD);
  tty.c_cflag &= static_cast<tcflag_t>(~CSIZE);
  tty.c_cflag |= CS8;
  tty.c_cflag &= static_cast<tcflag_t>(~PARENB);
  tty.c_cflag &= static_cast<tcflag_t>(~CSTOPB);
  tty.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);

  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    return false;
  }

  return true;
}

void SerialPort::closePort()
{
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

bool SerialPort::isOpen() const
{
  return fd_ >= 0;
}

bool SerialPort::readByte(uint8_t & byte, int timeout_ms)
{
  std::vector<uint8_t> buffer;
  if (!readExact(buffer, 1, timeout_ms)) {
    return false;
  }
  byte = buffer[0];
  return true;
}

bool SerialPort::readExact(std::vector<uint8_t> & out, std::size_t n, int timeout_ms)
{
  out.clear();
  out.reserve(n);

  if (fd_ < 0) {
    return false;
  }

  while (out.size() < n) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd_, &set);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int rv = select(fd_ + 1, &set, nullptr, nullptr, &tv);
    if (rv <= 0) {
      return false;
    }

    uint8_t b = 0;
    ssize_t r = ::read(fd_, &b, 1);
    if (r == 1) {
      out.push_back(b);
    } else if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      return false;
    }
  }

  return true;
}

}  // namespace custom_lidar_sdk
