#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "custom_lidar_sdk/lidar_types.hpp"
#include "custom_lidar_sdk/serial_port.hpp"

namespace custom_lidar_sdk
{

class CustomLidarDriver
{
public:
  CustomLidarDriver();
  ~CustomLidarDriver();

  void setConfig(const LidarConfig & config);
  const LidarConfig & getConfig() const;

  bool connect();
  void disconnect();
  bool isConnected() const;

  // Với LiDAR TX-only, start/stop hiện chỉ bật/tắt logic đọc dữ liệu.
  bool startScan();
  bool stopScan();
  bool isScanning() const;

  // Trả về true khi đã gom đủ một vòng 360 độ.
  bool grabScanData(LaserScanData & scan, int timeout_ms = 1000);

private:
  enum class FrameType
  {
    None,
    Health,
    Measurement,
    BadChecksum,
    Unknown
  };

  bool readFrame(std::vector<uint8_t> & frame, int timeout_ms);
  FrameType processFrame(const std::vector<uint8_t> & frame, bool & completed_scan);

  bool checkChecksum(const std::vector<uint8_t> & frame) const;
  static uint16_t checksum16Sum(const std::vector<uint8_t> & data, std::size_t length);

  void parseHealthFrame(const std::vector<uint8_t> & frame);
  void parseMeasurementFrame(const std::vector<uint8_t> & frame, bool & completed_scan);

  void clearAccumulatedScan();
  void fillOutputScan(LaserScanData & scan) const;
  void insertPoint(float angle_deg, float distance_m, float quality);

private:
  LidarConfig config_;
  SerialPort serial_;
  bool scanning_{false};

  std::vector<float> ranges_;
  std::vector<float> intensities_;

  bool has_last_start_angle_{false};
  float last_start_angle_{0.0F};
  float current_rpm_{0.0F};
};

}  // namespace custom_lidar_sdk
