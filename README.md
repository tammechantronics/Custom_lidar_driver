# Custom LiDAR ROS2 Driver

Gói này mô phỏng kiến trúc kiểu `YDLidar-SDK + ydlidar_ros2_driver`, nhưng dùng protocol riêng của LiDAR tự chế:

```text
AA 00 40 01 61 AD 00 38 ... CRC_H CRC_L
```

Kiến trúc:

```text
custom_lidar_sdk             # thư viện lõi: serial + parser + scan assembler
custom_lidar_ros2_driver     # ROS2 wrapper: publish sensor_msgs/LaserScan ra /scan
```

## Dây USB-TTL

```text
LiDAR T  -> RX USB-TTL
LiDAR G  -> GND USB-TTL
LiDAR U  -> 5V ổn định
```

Motor LiDAR vẫn điều khiển riêng bằng L298N/Arduino.

## Build

Copy 2 package vào workspace ROS2 của bạn:

```bash
cd ~/your_ws/src
# copy custom_lidar_sdk và custom_lidar_ros2_driver vào đây
cd ~/your_ws
colcon build --symlink-install --packages-select custom_lidar_sdk custom_lidar_ros2_driver
source install/setup.bash
```

## Chạy driver

```bash
ros2 launch custom_lidar_ros2_driver lidar.launch.py
```

Hoặc chạy trực tiếp:

```bash
ros2 run custom_lidar_ros2_driver custom_lidar_ros2_driver_node --ros-args \
  -p port:=/dev/ttyUSB0 \
  -p baudrate:=115200 \
  -p frame_id:=laser_frame
```

## Test RViz độc lập

```bash
ros2 launch custom_lidar_ros2_driver test_view.launch.py
rviz2
```

Trong RViz:

```text
Fixed Frame: world
Add -> LaserScan -> Topic: /scan
```

## Kiểm tra topic

```bash
ros2 topic list
ros2 topic hz /scan
ros2 topic echo /scan --once
```


## Tham số chính

File:

```text
custom_lidar_ros2_driver/params/lidar.yaml
```

Các tham số quan trọng:

```yaml
port: /dev/ttyUSB0
baudrate: 115200
frame_id: laser_frame
scan_topic: /scan
range_min: 0.05
range_max: 8.0
scan_size: 256
angle_offset_deg: 0.0
invert_angle: false
rpm_factor: 2.5
sector_angle_deg: 22.5
```

Nếu hướng scan trong RViz bị lệch, chỉnh `angle_offset_deg`.
Nếu trái/phải bị ngược, đổi `invert_angle: true`.
