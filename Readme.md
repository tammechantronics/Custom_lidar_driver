# Custom LiDAR Driver

Dự án nghiên cứu giao thức truyền dữ liệu và xây dựng driver ROS 2 cho cảm biến LiDAR sử dụng giao tiếp UART.

## Tài liệu dự án

### 1. Phân tích và giải mã giao thức LiDAR

Tài liệu trình bày quá trình:

* Xác định chức năng các chân kết nối.
* Kết nối LiDAR với USB–TTL.
* Thu dữ liệu UART thô.
* Phân tích cấu trúc frame.
* Kiểm tra checksum.
* Giải mã dữ liệu góc, khoảng cách và chất lượng tín hiệu.
* Chuyển dữ liệu quét sang tọa độ Descartes.

➡️ [Đọc tài liệu phân tích giao thức LiDAR](./giaimalidar.md)

### 2. Hướng dẫn sử dụng ROS 2 Driver

Tài liệu trình bày:

* Cấu trúc SDK và ROS 2 driver.
* Build package bằng `colcon`.
* Cấu hình cổng serial.
* Chạy node LiDAR.
* Publish topic `/scan`.
* Hiển thị dữ liệu trên RViz2.
* Tích hợp LiDAR với mobile robot và SLAM.

➡️ [Đọc tài liệu hướng dẫn ROS 2 Driver](./xaydungdriver.md)




## Lưu ý

Giao thức LiDAR trong dự án được phân tích từ dữ liệu thực nghiệm do chưa có tài liệu giao thức chính thức từ nhà sản xuất.

Một số trường và hệ số chuyển đổi vẫn cần tiếp tục được kiểm chứng, bao gồm:

* Hệ số chuyển đổi tốc độ quay.
* Ý nghĩa chính xác của trường góc.
* Bước góc giữa các điểm đo.
* Hệ số chuyển đổi khoảng cách.
* Ý nghĩa của byte chất lượng tín hiệu.
