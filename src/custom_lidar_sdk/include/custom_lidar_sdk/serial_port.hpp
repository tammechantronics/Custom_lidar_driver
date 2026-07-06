#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <termios.h>

namespace custom_lidar_sdk
{

class SerialPort
{
public:
  SerialPort() = default;
  ~SerialPort();

  bool openPort(const std::string & port, int baudrate);
  void closePort();
  bool isOpen() const;

  bool readByte(uint8_t & byte, int timeout_ms);
  bool readExact(std::vector<uint8_t> & out, std::size_t n, int timeout_ms);

private:
  int fd_{-1};

  static speed_t baudToTermios(int baudrate);
  bool configurePort(int baudrate);
};

}  // namespace custom_lidar_sdk
