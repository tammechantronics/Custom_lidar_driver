# Custom LiDAR ROS2 Driver

## 1. Giới thiệu

`Custom_lidar_driver` là bộ driver ROS2 dùng để đọc dữ liệu từ LiDAR giao tiếp UART/USB-TTL và xuất dữ liệu quét 2D dưới dạng topic `/scan`.

Driver này không dùng trực tiếp `YDLidar-SDK`, mà được chia thành 2 package riêng:

```text
Custom_lidar_driver/
├── src/
│   ├── custom_lidar_sdk/              # Thư viện lõi: mở serial, đọc frame, kiểm tra checksum, parse dữ liệu
│   └── custom_lidar_ros2_driver/      # ROS2 wrapper: publish sensor_msgs/LaserScan ra /scan
└── README.md
```

Kiến trúc xử lý tổng quát:

```text
LiDAR UART
   │
   ▼
USB-TTL / /dev/ttyUSB0
   │
   ▼
custom_lidar_sdk
   ├── Mở cổng serial
   ├── Đọc frame AA ... CRC
   ├── Kiểm tra checksum 16-bit
   ├── Parse frame AD
   └── Gom dữ liệu thành một vòng quét 360°
   │
   ▼
custom_lidar_ros2_driver
   ├── Chuyển dữ liệu sang sensor_msgs/msg/LaserScan
   ├── Publish topic /scan
   └── Cung cấp service start/stop scan
```

Mục tiêu của package là dùng LiDAR tự chế hoặc LiDAR có protocol riêng cho các bài toán:

* Hiển thị dữ liệu LiDAR trên RViz2.
* Tích hợp với robot ROS2.
* Dùng làm đầu vào cho SLAM, ví dụ `slam_toolbox` hoặc `cartographer`.
* Dùng làm cảm biến tránh vật cản hoặc xây dựng bản đồ 2D.

---

## 2. Phần cứng cần có

### 2.1. Thiết bị

* LiDAR giao tiếp UART.
* Module USB-TTL, nên dùng loại ổn định như CP2102, FT232 hoặc CH340.
* Máy tính Ubuntu / Jetson Nano / Raspberry Pi có cài ROS2.
* Nguồn 5V ổn định cho LiDAR.

### 2.2. Sơ đồ nối dây

Với LiDAR có các chân `T`, `G`, `U`, `+`, `-`, đấu dây như sau:

| Chân LiDAR | Kết nối        | Ghi chú                             |
| ---------- | -------------- | ----------------------------------- |
| `T`        | RX của USB-TTL | Đây là chân LiDAR truyền dữ liệu ra |
| `G`        | GND USB-TTL    | Mass chung                          |
| `U`        | 5V ổn định     | Nguồn cấp                           |
| `+`        | 5V             | Nguồn cấp                           |
| `-`        | GND            | Mass nguồn                          |

Sơ đồ đơn giản:

```text
LiDAR T  ─────────► RX USB-TTL
LiDAR G  ─────────► GND USB-TTL
LiDAR U  ─────────► 5V
LiDAR +  ─────────► 5V
LiDAR -  ─────────► GND
```

Lưu ý:

* Phải nối chung GND giữa LiDAR, USB-TTL và nguồn.
* Nếu LiDAR quay nhưng không có dữ liệu, kiểm tra lại chân TX/RX.
* Nếu cổng `/dev/ttyUSB0` không xuất hiện, kiểm tra driver USB-TTL.
* Nếu dùng Jetson Nano, nên cấp nguồn LiDAR riêng ổn định, tránh lấy nguồn yếu từ cổng USB.

---

## 3. Yêu cầu phần mềm

Driver được thiết kế cho ROS2, khuyến nghị dùng:

```text
Ubuntu 22.04
ROS2 Humble
colcon
CMake >= 3.8
C++17
```

Cài các gói cần thiết:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  git \
  python3-colcon-common-extensions \
  ros-humble-rclcpp \
  ros-humble-sensor-msgs \
  ros-humble-std-srvs \
  ros-humble-tf2-ros \
  ros-humble-rviz2
```

Nạp môi trường ROS2:

```bash
source /opt/ros/humble/setup.bash
```

Có thể thêm vào `~/.bashrc` để tự động source mỗi lần mở terminal:

```bash
echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

---

## 4. Clone project và build

### 4.1. Tạo workspace ROS2

```bash
mkdir -p ~/custom_lidar_ws/src
cd ~/custom_lidar_ws/src
```

Clone project:

```bash
git clone https://github.com/tammechantronics/Custom_lidar_driver.git
```

Sau khi clone, cấu trúc sẽ là:

```text
~/custom_lidar_ws/src/Custom_lidar_driver/
├── src/
│   ├── custom_lidar_sdk/
│   └── custom_lidar_ros2_driver/
└── README.md
```

Vì bên trong repo đã có thư mục `src/`, cần copy hoặc symlink 2 package con ra trực tiếp trong workspace:

```bash
cd ~/custom_lidar_ws/src
cp -r Custom_lidar_driver/src/custom_lidar_sdk .
cp -r Custom_lidar_driver/src/custom_lidar_ros2_driver .
```

Khi đó workspace đúng phải có dạng:

```text
~/custom_lidar_ws/
└── src/
    ├── custom_lidar_sdk/
    ├── custom_lidar_ros2_driver/
    └── Custom_lidar_driver/
```


### 4.2. Build package

```bash
cd ~/custom_lidar_ws
colcon build --symlink-install --packages-select custom_lidar_sdk custom_lidar_ros2_driver
```

Source workspace:

```bash
source install/setup.bash
```

Có thể thêm vào `~/.bashrc`:

```bash
echo "source ~/custom_lidar_ws/install/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

Kiểm tra package đã nhận chưa:

```bash
ros2 pkg list | grep custom_lidar
```

Kết quả mong muốn:

```text
custom_lidar_sdk
custom_lidar_ros2_driver
```

---

## 5. Kiểm tra cổng serial của LiDAR

Cắm USB-TTL vào máy rồi kiểm tra:

```bash
ls /dev/ttyUSB*
```

Thường LiDAR sẽ nằm ở:

```text
/dev/ttyUSB0
```

Xem log kernel khi cắm thiết bị:

```bash
dmesg | grep tty
```

Nếu bị lỗi quyền truy cập serial, thêm user vào nhóm `dialout`:

```bash
sudo usermod -a -G dialout $USER
```

Sau đó đăng xuất rồi đăng nhập lại, hoặc reboot máy.

Kiểm tra quyền nhanh:

```bash
ls -l /dev/ttyUSB0
```

Nếu cần test tạm thời:

```bash
sudo chmod 666 /dev/ttyUSB0
```

---

## 6. Cấu hình tham số LiDAR

File cấu hình chính:

```text
custom_lidar_ros2_driver/params/lidar.yaml
```

Nội dung khuyến nghị:

```yaml
custom_lidar_ros2_driver:
  ros__parameters:
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
    auto_start: true
```

Ý nghĩa các tham số:

| Tham số            | Giá trị mặc định | Ý nghĩa                                |
| ------------------ | ---------------: | -------------------------------------- |
| `port`             |   `/dev/ttyUSB0` | Cổng serial của USB-TTL                |
| `baudrate`         |         `115200` | Tốc độ UART                            |
| `frame_id`         |    `laser_frame` | Tên hệ tọa độ của LiDAR                |
| `scan_topic`       |          `/scan` | Topic LaserScan được publish           |
| `range_min`        |           `0.05` | Khoảng cách nhỏ nhất, đơn vị mét       |
| `range_max`        |            `8.0` | Khoảng cách lớn nhất, đơn vị mét       |
| `scan_size`        |            `256` | Số tia trong một vòng quét             |
| `angle_offset_deg` |            `0.0` | Bù góc nếu hướng quét bị lệch          |
| `invert_angle`     |          `false` | Đảo chiều quét nếu trái/phải bị ngược  |
| `rpm_factor`       |            `2.5` | Hệ số đổi giá trị tốc độ raw sang RPM  |
| `sector_angle_deg` |           `22.5` | Góc mỗi sector dữ liệu                 |
| `auto_start`       |           `true` | Tự động bắt đầu đọc scan khi chạy node |

Nếu ảnh scan trên RViz bị xoay lệch, chỉnh:

```yaml
angle_offset_deg: 90.0
```

Nếu trái/phải bị ngược, đổi:

```yaml
invert_angle: true
```

---

## 7. Chạy driver

### 7.1. Chạy bằng launch file

```bash
cd ~/custom_lidar_ws
source install/setup.bash
ros2 launch custom_lidar_ros2_driver lidar.launch.py
```

Nếu chạy thành công, terminal sẽ có log tương tự:

```text
Opened LiDAR serial port /dev/ttyUSB0 at 115200
LiDAR scan started
```

### 7.2. Chạy trực tiếp node

```bash
ros2 run custom_lidar_ros2_driver custom_lidar_ros2_driver_node --ros-args \
  -p port:=/dev/ttyUSB0 \
  -p baudrate:=115200 \
  -p frame_id:=laser_frame \
  -p scan_topic:=/scan
```

Cách này phù hợp khi cần đổi nhanh tham số mà không sửa file `lidar.yaml`.

---

## 8. Kiểm tra topic ROS2

Mở terminal mới:

```bash
cd ~/custom_lidar_ws
source install/setup.bash
```

Liệt kê topic:

```bash
ros2 topic list
```

Kết quả cần có:

```text
/scan
```

Kiểm tra tần số publish:

```bash
ros2 topic hz /scan
```

Kết quả mong muốn thường khoảng 5–10 Hz, tùy tốc độ quay thực tế của LiDAR.

Xem một message `/scan`:

```bash
ros2 topic echo /scan --once
```

Kiểm tra kiểu dữ liệu topic:

```bash
ros2 topic info /scan
```

Kết quả mong muốn:

```text
Type: sensor_msgs/msg/LaserScan
```

---

## 9. Test LiDAR với RViz2

### 9.1. Chạy launch test RViz độc lập

```bash
cd ~/custom_lidar_ws
source install/setup.bash
ros2 launch custom_lidar_ros2_driver test_view.launch.py
```

Launch file này chạy driver và publish thêm static TF:

```text
world → laser_frame
```

Mở RViz2:

```bash
rviz2
```

Trong RViz2:

```text
Fixed Frame: world
Add → LaserScan
Topic: /scan
```

Nếu dữ liệu đúng, bạn sẽ thấy các điểm LiDAR xuất hiện xung quanh gốc tọa độ.

### 9.2. Nếu RViz báo lỗi Fixed Frame

Nếu RViz báo lỗi kiểu:

```text
No transform from [laser_frame] to [world]
```

hãy kiểm tra lại:

```bash
ros2 run tf2_ros tf2_echo world laser_frame
```

Hoặc chạy static transform thủ công:

```bash
ros2 run tf2_ros static_transform_publisher \
  0 0 0 0 0 0 world laser_frame
```

---



## 10. Topic và service

### 10.1. Topic publish

| Topic   | Kiểu dữ liệu                | Mô tả                 |
| ------- | --------------------------- | --------------------- |
| `/scan` | `sensor_msgs/msg/LaserScan` | Dữ liệu quét laser 2D |

### 10.2. Service

| Service       | Kiểu dữ liệu         | Chức năng              |
| ------------- | -------------------- | ---------------------- |
| `/start_scan` | `std_srvs/srv/Empty` | Bật đọc dữ liệu LiDAR  |
| `/stop_scan`  | `std_srvs/srv/Empty` | Dừng đọc dữ liệu LiDAR |

Gọi service dừng scan:

```bash
ros2 service call /stop_scan std_srvs/srv/Empty
```

Gọi service bắt đầu scan:

```bash
ros2 service call /start_scan std_srvs/srv/Empty
```

---

## 11. Protocol dữ liệu LiDAR

Driver hiện xử lý frame có dạng tổng quát:

```text
AA LEN_H LEN_L ... CMD ... PAYLOAD ... CRC_H CRC_L
```

Trong đó:

| Thành phần    | Ý nghĩa                             |
| ------------- | ----------------------------------- |
| `AA`          | Header bắt đầu frame                |
| `LEN_H LEN_L` | Độ dài frame                        |
| `CMD`         | Loại frame                          |
| `AD`          | Frame chứa dữ liệu đo khoảng cách   |
| `AE`          | Frame health / thông tin trạng thái |
| `CRC_H CRC_L` | Checksum 16-bit                     |

Luồng xử lý trong `custom_lidar_sdk`:

```text
readFrame()
  └── tìm header AA
  └── đọc length
  └── đọc payload
  └── kiểm tra checksum
  └── phân loại frame AD / AE
  └── parse khoảng cách, góc, chất lượng tín hiệu
  └── gom đủ 360° thì trả về một LaserScanData
```

---

## 12. Sử dụng với SLAM

Sau khi `/scan` đã hiển thị ổn định trong RViz2, có thể dùng cho SLAM.


## 13. Lỗi thường gặp và cách xử lý

### Lỗi 1: Không mở được cổng `/dev/ttyUSB0`

Thông báo có thể gặp:

```text
Cannot open LiDAR serial port /dev/ttyUSB0 at 115200
```

Cách xử lý:

```bash
ls /dev/ttyUSB*
sudo chmod 666 /dev/ttyUSB0
```

Nếu vẫn lỗi:

```bash
sudo usermod -a -G dialout $USER
```

Sau đó logout/login hoặc reboot.

---

### Lỗi 2: Không thấy topic `/scan`

Kiểm tra node đã chạy chưa:

```bash
ros2 node list
```

Kiểm tra package:

```bash
ros2 pkg list | grep custom_lidar
```

Chạy lại:

```bash
source ~/custom_lidar_ws/install/setup.bash
ros2 launch custom_lidar_ros2_driver lidar.launch.py
```

---

### Lỗi 3: Có `/scan` nhưng không thấy điểm trên RViz

Kiểm tra Fixed Frame trong RViz:

```text
Fixed Frame: world
```
Có thể thử đổi phần Reliability Policy trong phần Topic của LaserScan thành System Default:
```text
Reliability Policy: System Default
```
Nếu test độc lập, chạy:

```bash
ros2 launch custom_lidar_ros2_driver test_view.launch.py
```

Hoặc publish static TF:

```bash
ros2 run tf2_ros static_transform_publisher \
  0 0 0 0 0 0 world laser_frame
```

---

### Lỗi 4: Scan bị ngược trái/phải

Sửa file:

```text
custom_lidar_ros2_driver/params/lidar.yaml
```

Đổi:

```yaml
invert_angle: true
```

Sau đó build lại nếu cần và chạy lại driver.

---

### Lỗi 5: Scan bị xoay lệch hướng

Chỉnh tham số:

```yaml
angle_offset_deg: 90.0
```

Có thể thử các giá trị:

```text
0.0
90.0
180.0
-90.0
```

đến khi hướng scan trên RViz đúng với hướng thực tế của LiDAR.

---

### Lỗi 6: Tần số `/scan` thấp hoặc không ổn định

Kiểm tra:

```bash
ros2 topic hz /scan
```

Nguyên nhân có thể là:

* Tốc độ quay LiDAR không ổn định.
* Nguồn 5V yếu.
* USB-TTL kém chất lượng.
* Baudrate sai.
* Dữ liệu frame bị lỗi checksum nhiều.
* LiDAR bị rung cơ khí.

Cách xử lý:

* Dùng nguồn 5V riêng ổn định.
* Dùng USB-TTL CP2102 hoặc FT232.
* Kiểm tra lại baudrate `115200`.
* Cố định chắc LiDAR trên robot.
* Kiểm tra dây GND chung.

---

### Lỗi 7: Build lỗi do thiếu thư mục `rviz`

Nếu gặp lỗi liên quan đến:

```text
install DIRECTORY given no DESTINATION
file INSTALL cannot find .../rviz
```

Tạo thư mục `rviz`:

```bash
mkdir -p ~/custom_lidar_ws/src/custom_lidar_ros2_driver/rviz
```

Hoặc sửa `CMakeLists.txt`:

```cmake
install(DIRECTORY launch params DESTINATION share/${PROJECT_NAME})
```

Sau đó build lại:

```bash
cd ~/custom_lidar_ws
rm -rf build install log
colcon build --symlink-install --packages-select custom_lidar_sdk custom_lidar_ros2_driver
source install/setup.bash
```

---

## 15. Gợi ý kiểm tra theo từng bước

Nên kiểm tra theo thứ tự sau:

```text
Bước 1: Cắm LiDAR và kiểm tra /dev/ttyUSB0
Bước 2: Kiểm tra quyền serial
Bước 3: Build custom_lidar_sdk
Bước 4: Build custom_lidar_ros2_driver
Bước 5: Chạy lidar.launch.py
Bước 6: Kiểm tra ros2 topic list
Bước 7: Kiểm tra ros2 topic hz /scan
Bước 8: Mở RViz2 để xem LaserScan
Bước 9: Cấu hình lại angle_offset_deg / invert_angle nếu cần
Bước 10: Tích hợp vào robot hoặc SLAM
```

---



## 16. Ghi chú phát triển

Các phần có thể phát triển tiếp:

* Thêm file RViz config sẵn để mở là thấy `/scan`.
* Thêm udev rule để cố định tên cổng, ví dụ `/dev/custom_lidar`.
* Thêm launch file tích hợp với robot TF.
* Thêm launch file chạy cùng `slam_toolbox`.
* Thêm log thống kê số frame lỗi checksum.
* Thêm mode debug để in frame hex khi parser lỗi.
* Viết tài liệu protocol chi tiết hơn cho frame `AD`.
* Thêm hình minh họa sơ đồ dây và cây TF.
