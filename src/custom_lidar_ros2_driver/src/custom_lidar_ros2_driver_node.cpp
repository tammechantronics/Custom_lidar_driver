#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "std_srvs/srv/empty.hpp"

#include "custom_lidar_sdk/custom_lidar_driver.hpp"
#include "custom_lidar_sdk/lidar_types.hpp"

class CustomLidarRos2Driver : public rclcpp::Node
{
public:
  CustomLidarRos2Driver()
  : Node("custom_lidar_ros2_driver")
  {
    declare_parameter<std::string>("port", "/dev/ttyUSB0");
    declare_parameter<int>("baudrate", 115200);
    declare_parameter<std::string>("frame_id", "laser_frame");
    declare_parameter<std::string>("scan_topic", "/scan");

    declare_parameter<double>("range_min", 0.05);
    declare_parameter<double>("range_max", 8.0);
    declare_parameter<int>("scan_size", 256);
    declare_parameter<double>("angle_offset_deg", 0.0);
    declare_parameter<bool>("invert_angle", false);
    declare_parameter<double>("rpm_factor", 2.5);
    declare_parameter<double>("sector_angle_deg", 22.5);
    declare_parameter<bool>("auto_start", true);

    frame_id_ = get_parameter("frame_id").as_string();
    auto scan_topic = get_parameter("scan_topic").as_string();

    custom_lidar_sdk::LidarConfig config;
    config.port = get_parameter("port").as_string();
    config.baudrate = get_parameter("baudrate").as_int();
    config.range_min = static_cast<float>(get_parameter("range_min").as_double());
    config.range_max = static_cast<float>(get_parameter("range_max").as_double());
    config.scan_size = static_cast<int>(get_parameter("scan_size").as_int());
    config.angle_offset_deg = static_cast<float>(get_parameter("angle_offset_deg").as_double());
    config.invert_angle = get_parameter("invert_angle").as_bool();
    config.rpm_factor = static_cast<float>(get_parameter("rpm_factor").as_double());
    config.sector_angle_deg = static_cast<float>(get_parameter("sector_angle_deg").as_double());

    lidar_.setConfig(config);

    scan_pub_ = create_publisher<sensor_msgs::msg::LaserScan>(scan_topic, rclcpp::SensorDataQoS());

    start_srv_ = create_service<std_srvs::srv::Empty>(
      "start_scan",
      [this](const std::shared_ptr<std_srvs::srv::Empty::Request>,
             std::shared_ptr<std_srvs::srv::Empty::Response>) {
        startScan();
      });

    stop_srv_ = create_service<std_srvs::srv::Empty>(
      "stop_scan",
      [this](const std::shared_ptr<std_srvs::srv::Empty::Request>,
             std::shared_ptr<std_srvs::srv::Empty::Response>) {
        stopScan();
      });

    if (!lidar_.connect()) {
      RCLCPP_ERROR(get_logger(), "Cannot open LiDAR serial port %s at %d",
        config.port.c_str(), config.baudrate);
      throw std::runtime_error("Cannot open LiDAR serial port");
    }

    RCLCPP_INFO(get_logger(), "Opened LiDAR serial port %s at %d",
      config.port.c_str(), config.baudrate);

    running_.store(true);

    if (get_parameter("auto_start").as_bool()) {
      startScan();
    }

    worker_ = std::thread(&CustomLidarRos2Driver::workerLoop, this);
  }

  ~CustomLidarRos2Driver() override
  {
    running_.store(false);
    lidar_.stopScan();
    lidar_.disconnect();

    if (worker_.joinable()) {
      worker_.join();
    }
  }

private:
  void startScan()
  {
    if (lidar_.startScan()) {
      active_.store(true);
      RCLCPP_INFO(get_logger(), "LiDAR scan started");
    } else {
      RCLCPP_ERROR(get_logger(), "Cannot start LiDAR scan");
    }
  }

  void stopScan()
  {
    active_.store(false);
    lidar_.stopScan();
    RCLCPP_INFO(get_logger(), "LiDAR scan stopped");
  }

  void workerLoop()
  {
    while (rclcpp::ok() && running_.load()) {
      if (!active_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        continue;
      }

      custom_lidar_sdk::LaserScanData sdk_scan;
      if (!lidar_.grabScanData(sdk_scan, 500)) {
        RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
          "No complete scan received from LiDAR");
        continue;
      }

      publishScan(sdk_scan);
    }
  }

  void publishScan(const custom_lidar_sdk::LaserScanData & sdk_scan)
  {
    auto msg = sensor_msgs::msg::LaserScan();

    msg.header.stamp = now();
    msg.header.frame_id = frame_id_;

    msg.angle_min = sdk_scan.angle_min;
    msg.angle_max = sdk_scan.angle_max;
    msg.angle_increment = sdk_scan.angle_increment;

    msg.time_increment = sdk_scan.scan_time > 0.0F && !sdk_scan.ranges.empty() ?
      sdk_scan.scan_time / static_cast<float>(sdk_scan.ranges.size()) : 0.0F;
    msg.scan_time = sdk_scan.scan_time;

    msg.range_min = sdk_scan.range_min;
    msg.range_max = sdk_scan.range_max;

    msg.ranges = sdk_scan.ranges;
    msg.intensities = sdk_scan.intensities;

    scan_pub_->publish(msg);
  }

private:
  custom_lidar_sdk::CustomLidarDriver lidar_;

  rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr scan_pub_;
  rclcpp::Service<std_srvs::srv::Empty>::SharedPtr start_srv_;
  rclcpp::Service<std_srvs::srv::Empty>::SharedPtr stop_srv_;

  std::string frame_id_{"laser_frame"};
  std::atomic_bool running_{false};
  std::atomic_bool active_{false};
  std::thread worker_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  try {
    auto node = std::make_shared<CustomLidarRos2Driver>();
    rclcpp::spin(node);
  } catch (const std::exception & e) {
    fprintf(stderr, "custom_lidar_ros2_driver error: %s\n", e.what());
  }

  rclcpp::shutdown();
  return 0;
}
