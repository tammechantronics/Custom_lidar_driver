#include "custom_lidar_sdk/custom_lidar_driver.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace custom_lidar_sdk
{

namespace
{
constexpr uint8_t kHeader = 0xAA;
constexpr uint8_t kCmdHealth = 0xAE;
constexpr uint8_t kCmdMeasurement = 0xAD;
constexpr float kPi = 3.14159265358979323846F;
}

CustomLidarDriver::CustomLidarDriver()
{
  clearAccumulatedScan();
}

CustomLidarDriver::~CustomLidarDriver()
{
  disconnect();
}

void CustomLidarDriver::setConfig(const LidarConfig & config)
{
  config_ = config;
  clearAccumulatedScan();
}

const LidarConfig & CustomLidarDriver::getConfig() const
{
  return config_;
}

bool CustomLidarDriver::connect()
{
  return serial_.openPort(config_.port, config_.baudrate);
}

void CustomLidarDriver::disconnect()
{
  scanning_ = false;
  serial_.closePort();
}

bool CustomLidarDriver::isConnected() const
{
  return serial_.isOpen();
}

bool CustomLidarDriver::startScan()
{
  if (!isConnected()) {
    return false;
  }
  scanning_ = true;
  clearAccumulatedScan();
  return true;
}

bool CustomLidarDriver::stopScan()
{
  scanning_ = false;
  return true;
}

bool CustomLidarDriver::isScanning() const
{
  return scanning_;
}

void CustomLidarDriver::clearAccumulatedScan()
{
  int scan_size = std::max(1, config_.scan_size);
  ranges_.assign(static_cast<std::size_t>(scan_size), std::numeric_limits<float>::infinity());
  intensities_.assign(static_cast<std::size_t>(scan_size), 0.0F);
  has_last_start_angle_ = false;
  last_start_angle_ = 0.0F;
}

uint16_t CustomLidarDriver::checksum16Sum(const std::vector<uint8_t> & data, std::size_t length)
{
  uint32_t sum = 0;
  length = std::min(length, data.size());
  for (std::size_t i = 0; i < length; ++i) {
    sum += data[i];
  }
  return static_cast<uint16_t>(sum & 0xFFFFU);
}

bool CustomLidarDriver::checkChecksum(const std::vector<uint8_t> & frame) const
{
  if (frame.size() < 10) {
    return false;
  }

  uint16_t received = static_cast<uint16_t>((frame[frame.size() - 2] << 8U) | frame[frame.size() - 1]);
  uint16_t calculated = checksum16Sum(frame, frame.size() - 2);
  return received == calculated;
}

bool CustomLidarDriver::readFrame(std::vector<uint8_t> & frame, int timeout_ms)
{
  frame.clear();

  if (!serial_.isOpen()) {
    return false;
  }

  uint8_t b = 0;
  while (scanning_) {
    if (!serial_.readByte(b, timeout_ms)) {
      return false;
    }

    if (b != kHeader) {
      continue;
    }

    std::vector<uint8_t> len_bytes;
    if (!serial_.readExact(len_bytes, 2, timeout_ms)) {
      return false;
    }

    uint16_t length_field = static_cast<uint16_t>((len_bytes[0] << 8U) | len_bytes[1]);
    std::size_t total_length = static_cast<std::size_t>(length_field) + 2U;

    if (total_length < 10U || total_length > 256U) {
      continue;
    }

    std::vector<uint8_t> rest;
    if (!serial_.readExact(rest, total_length - 3U, timeout_ms)) {
      return false;
    }

    frame.reserve(total_length);
    frame.push_back(kHeader);
    frame.push_back(len_bytes[0]);
    frame.push_back(len_bytes[1]);
    frame.insert(frame.end(), rest.begin(), rest.end());
    return true;
  }

  return false;
}

void CustomLidarDriver::parseHealthFrame(const std::vector<uint8_t> & frame)
{
  if (frame.size() < 9U) {
    return;
  }

  uint16_t payload_len = static_cast<uint16_t>((frame[6] << 8U) | frame[7]);
  if (payload_len < 1U || frame.size() < 9U) {
    return;
  }

  uint8_t rpm_raw = frame[8];
  current_rpm_ = static_cast<float>(rpm_raw) * config_.rpm_factor;
}

void CustomLidarDriver::insertPoint(float angle_deg, float distance_m, float quality)
{
  if (distance_m < config_.range_min || distance_m > config_.range_max) {
    return;
  }

  if (config_.invert_angle) {
    angle_deg = 360.0F - angle_deg;
  }

  angle_deg += config_.angle_offset_deg;
  angle_deg = std::fmod(angle_deg, 360.0F);
  if (angle_deg < 0.0F) {
    angle_deg += 360.0F;
  }

  int scan_size = std::max(1, config_.scan_size);
  int index = static_cast<int>(std::lround(angle_deg / 360.0F * static_cast<float>(scan_size))) % scan_size;

  if (index < 0 || index >= scan_size) {
    return;
  }

  std::size_t idx = static_cast<std::size_t>(index);
  if (distance_m < ranges_[idx]) {
    ranges_[idx] = distance_m;
    intensities_[idx] = quality;
  }
}

void CustomLidarDriver::parseMeasurementFrame(const std::vector<uint8_t> & frame, bool & completed_scan)
{
  completed_scan = false;

  if (frame.size() < 14U) {
    return;
  }

  uint16_t payload_len = static_cast<uint16_t>((frame[6] << 8U) | frame[7]);
  if (payload_len < 6U || frame.size() < static_cast<std::size_t>(8U + payload_len + 2U)) {
    return;
  }

  const std::size_t payload_start = 8U;
  uint8_t rpm_raw = frame[payload_start];
  current_rpm_ = static_cast<float>(rpm_raw) * config_.rpm_factor;

  uint16_t start_angle_raw = static_cast<uint16_t>((frame[payload_start + 3U] << 8U) | frame[payload_start + 4U]);
  float start_angle = static_cast<float>(start_angle_raw) / 100.0F;

  if (has_last_start_angle_ && start_angle < last_start_angle_ - 100.0F) {
    completed_scan = true;
  }

  has_last_start_angle_ = true;
  last_start_angle_ = start_angle;

  std::size_t sample_data_start = payload_start + 5U;
  std::size_t sample_data_len = static_cast<std::size_t>(payload_len) - 5U;
  if (sample_data_len % 3U != 0U) {
    return;
  }

  std::size_t sample_count = sample_data_len / 3U;
  if (sample_count <= 1U) {
    return;
  }

  float angle_step = config_.sector_angle_deg / static_cast<float>(sample_count - 1U);

  for (std::size_t i = 0; i < sample_count; ++i) {
    std::size_t base = sample_data_start + i * 3U;
    uint8_t quality = frame[base];
    uint16_t distance_raw = static_cast<uint16_t>((frame[base + 1U] << 8U) | frame[base + 2U]);

    if (distance_raw == 0U) {
      continue;
    }

    float distance_m = static_cast<float>(distance_raw) * 0.25F / 1000.0F;
    float angle_deg = start_angle + static_cast<float>(i) * angle_step;
    if (angle_deg >= 360.0F) {
      angle_deg -= 360.0F;
    }

    insertPoint(angle_deg, distance_m, static_cast<float>(quality));
  }
}

CustomLidarDriver::FrameType CustomLidarDriver::processFrame(
  const std::vector<uint8_t> & frame,
  bool & completed_scan)
{
  completed_scan = false;

  if (frame.size() < 10U || frame[0] != kHeader) {
    return FrameType::None;
  }

  if (!checkChecksum(frame)) {
    return FrameType::BadChecksum;
  }

  uint8_t command = frame[5];
  if (command == kCmdHealth) {
    parseHealthFrame(frame);
    return FrameType::Health;
  }

  if (command == kCmdMeasurement) {
    parseMeasurementFrame(frame, completed_scan);
    return FrameType::Measurement;
  }

  return FrameType::Unknown;
}

void CustomLidarDriver::fillOutputScan(LaserScanData & scan) const
{
  int scan_size = std::max(1, config_.scan_size);

  scan.ranges = ranges_;
  scan.intensities = intensities_;
  scan.range_min = config_.range_min;
  scan.range_max = config_.range_max;
  scan.rpm = current_rpm_;
  scan.scan_time = current_rpm_ > 1.0F ? 60.0F / current_rpm_ : 0.16F;

  scan.angle_min = 0.0F;
  scan.angle_increment = 2.0F * kPi / static_cast<float>(scan_size);
  scan.angle_max = 2.0F * kPi - scan.angle_increment;
}

bool CustomLidarDriver::grabScanData(LaserScanData & scan, int timeout_ms)
{
  if (!scanning_ || !serial_.isOpen()) {
    return false;
  }

  while (scanning_) {
    std::vector<uint8_t> frame;
    if (!readFrame(frame, timeout_ms)) {
      return false;
    }

    bool completed_scan = false;
    FrameType type = processFrame(frame, completed_scan);
    if (type == FrameType::BadChecksum) {
      continue;
    }

    if (completed_scan) {
      fillOutputScan(scan);
      clearAccumulatedScan();
      return true;
    }
  }

  return false;
}

}  // namespace custom_lidar_sdk
