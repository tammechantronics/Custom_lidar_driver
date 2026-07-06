# custom_lidar_sdk

SDK lõi cho LiDAR tự chế dùng frame `AA ... AD`.

Module này không phụ thuộc ROS2 runtime. Nó làm:

- mở cổng serial `/dev/ttyUSB0`
- đọc frame `AA LEN ... CRC`
- kiểm tra checksum tổng 16-bit
- parse frame `AD`
- đổi dữ liệu thành scan 360 độ

API chính:

```cpp
custom_lidar_sdk::CustomLidarDriver lidar;
custom_lidar_sdk::LidarConfig config;
config.port = "/dev/ttyUSB0";
config.baudrate = 115200;

lidar.setConfig(config);
lidar.connect();
lidar.startScan();

custom_lidar_sdk::LaserScanData scan;
lidar.grabScanData(scan);
```
