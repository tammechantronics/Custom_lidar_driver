#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace custom_lidar_sdk
{

struct LidarPoint
{
  float angle_deg{0.0F};
  float distance_m{std::numeric_limits<float>::infinity()};
  float quality{0.0F};
};

struct LaserScanData
{
  std::vector<float> ranges;
  std::vector<float> intensities;

  float angle_min{0.0F};
  float angle_max{0.0F};
  float angle_increment{0.0F};

  float range_min{0.05F};
  float range_max{8.0F};

  float rpm{0.0F};
  float scan_time{0.0F};
};

struct LidarConfig
{
  std::string port{"/dev/ttyUSB0"};
  int baudrate{115200};

  float range_min{0.05F};
  float range_max{8.0F};

  int scan_size{256};
  float angle_offset_deg{0.0F};
  bool invert_angle{false};

  // Thực nghiệm với dữ liệu của bạn: raw 0x96 ~= 150, /scan ~= 6.25 Hz => 375 RPM.
  // Vì vậy mặc định là 2.5. Nếu muốn giống parser cũ, đặt 3.0.
  float rpm_factor{2.5F};

  // Protocol thực tế: frame AD bắt đầu cách nhau 22.5 độ, 17 mẫu/frame.
  float sector_angle_deg{22.5F};
};

}  // namespace custom_lidar_sdk
