# HƯỚNG DẪN KẾT NỐI, ĐỌC VÀ GIẢI MÃ DỮ LIỆU LiDAR

## 1. Giới thiệu

Tài liệu này trình bày quá trình nghiên cứu và thử nghiệm một cảm biến LiDAR 2D không có tài liệu giao thức chính thức. Quá trình thực hiện bắt đầu từ việc xác định chức năng các chân kết nối, điều khiển động cơ quay, đọc dữ liệu UART thô, nhận dạng cấu trúc frame và cuối cùng giải mã các thông tin gồm:

* Tốc độ quay của LiDAR.
* Góc bắt đầu của frame.
* Khoảng cách của từng điểm đo.
* Cường độ hoặc chất lượng tín hiệu.
* Trạng thái hoạt động của cảm biến.
* Kiểm tra tính hợp lệ của frame bằng checksum.

> **Lưu ý:** Cấu trúc giao thức trong tài liệu được suy luận từ dữ liệu thực nghiệm, không phải từ tài liệu chính thức của nhà sản xuất. Một số hệ số chuyển đổi cần tiếp tục được kiểm chứng bằng phép đo thực tế.

---

## 2. Phần cứng sử dụng

Các phần tử chính gồm:

* LiDAR 2D sử dụng đầu nối 5 chân.
* Nguồn 5 V ổn định cho mạch LiDAR.
* Nguồn cấp cho động cơ quay LiDAR.
* Máy tính để thu và phân tích dữ liệu.
* Dây kết nối.

Đầu nối LiDAR có các ký hiệu:

```text
+   -   U   G   T
```

Chức năng được xác định qua quá trình thử nghiệm:

| Chân | Chức năng                        |
| ---- | -------------------------------- |
| `U`  | Nguồn 5 V cho mạch xử lý LiDAR   |
| `G`  | GND của mạch LiDAR               |
| `T`  | Ngõ ra dữ liệu UART TX của LiDAR |
| `+`  | Cực dương động cơ quay           |
| `-`  | Cực âm động cơ quay              |

Hai chân `+` và `-` được sử dụng riêng cho động cơ. Ba chân `U`, `G` và `T` thuộc phần nguồn và truyền dữ liệu của mạch đo.

---
Tài liệu tham khảo:
```text
https://makerspet.com/product/3irobotix-delta-2a-lidar-distance-sensor-used-refurb/
```

## 3. Thông số truyền UART

Thông số UART xác định được trong quá trình thử nghiệm:

```text
Baud rate: 115200 bit/s
Data bits: 8
Parity: None
Stop bits: 1
Flow control: None
```


## 4. Đọc dữ liệu trên máy tính bằng Python

Cài đặt thư viện `pyserial`:

```bash
pip install pyserial
```

Chương trình đọc và hiển thị dữ liệu dưới dạng HEX:

```python
import serial
import time

PORT = "/dev/ttyUSB0"          # Sửa lại theo cổng COM của bạn
BAUDRATE = 115200    # Baudrate LiDAR

def main():
    try:
        ser = serial.Serial(
            port=PORT,
            baudrate=BAUDRATE,
            timeout=1
        )

        print("===================================")
        print(" LiDAR HEX Reader")
        print("===================================")
        print(f"Port     : {PORT}")
        print(f"Baudrate : {BAUDRATE}")
        print("Press Ctrl+C to stop")
        print("===================================")

        while True:
            data = ser.read(64)  # đọc 64 byte mỗi lần

            if data:
                hex_str = " ".join(f"{b:02X}" for b in data)
                print(hex_str)

            time.sleep(0.01)

    except serial.SerialException as e:
        print("Không mở được cổng serial.")
        print("Lỗi:", e)
        print("Hãy kiểm tra lại COM port hoặc đóng phần mềm khác đang dùng cổng này.")

    except KeyboardInterrupt:
        print("\nĐã dừng chương trình.")

    finally:
        try:
            ser.close()
            print("Đã đóng cổng serial.")
        except:
            pass

if __name__ == "__main__":
    main()
```


## 5. Dữ liệu UART quan sát được

Luồng dữ liệu có các frame bắt đầu bằng byte:

```text
AA
```

Hai loại frame chính đã quan sát được là:

```text
AD – Frame dữ liệu đo
AE – Frame trạng thái hoặc health
```

Ví dụ frame trạng thái:

```text
AA 00 09 01 61 AE 00 01 A2 02 66
```

Ví dụ phần đầu của frame dữ liệu đo:

```text
AA 00 40 01 61 AD 00 38 ...
```

Trong đó:

|     Vị trí | Trường         | Ý nghĩa suy luận               |
| ---------: | -------------- | ------------------------------ |
|          0 | `AA`           | Header                         |
|        1–2 | Length         | Trường độ dài frame            |
|        3–4 | Device ID      | Mã thiết bị, thường là `01 61` |
|          5 | Command        | `AD` hoặc `AE`                 |
|        6–7 | Payload length | Số byte payload                |
|         8… | Payload        | Dữ liệu tương ứng với command  |
| Cuối frame | Checksum       | Tổng kiểm tra 16 bit           |

Frame `AD` thường có payload dài:

```text
00 38 = 0x38 = 56 byte
```

Do đó, tổng chiều dài frame được xác định bằng:

```text
8 byte phần đầu + 56 byte payload + 2 byte checksum
= 66 byte
```
> **Lưu ý:** Phần phân tích cấu trúc này dựa vào việc AI đọc chuỗi dữ liệu được gửi từ cảm biến và phân tích ra cấu trúc.
---

## 6. Cấu trúc frame dữ liệu AD

Cấu trúc payload 56 byte được suy luận như sau:

| Vị trí trong payload | Số byte | Nội dung            |
| -------------------: | ------: | ------------------- |
|                    0 |       1 | RPM raw             |
|                  1–2 |       2 | Angle offset        |
|                  3–4 |       2 | Start angle         |
|                 5–55 |      51 | 17 điểm đo × 3 byte |

Có thể biểu diễn như sau:

```text
+-------------+--------------+-------------+------------------------+
| RPM raw     | Angle offset | Start angle | 17 × Point data        |
| 1 byte      | 2 byte       | 2 byte      | 17 × 3 byte = 51 byte |
+-------------+--------------+-------------+------------------------+
```

Tổng cộng:

```text
1 + 2 + 2 + 51 = 56 byte
```

---

## 7. Giải mã tốc độ quay

Giá trị RPM raw nằm tại byte đầu tiên của payload.

Qua so sánh với tốc độ quay thực tế, hệ số chuyển đổi tạm thời được suy luận là:

```text
RPM ≈ RPM_raw × 3.2
```

Ví dụ:

```text
RPM_raw = 0x75 = 117

RPM ≈ 117 × 3.2
    ≈ 374.4 RPM
```

Giá trị này gần với tốc độ quan sát thực tế khoảng 372–400 RPM.

Tuy nhiên, hệ số trên mới được xác định bằng thực nghiệm. Cần tiếp tục so sánh với tốc độ tính từ thời gian hoàn thành một vòng quét:

```text
RPM_time = 60 / T_vòng
```

Sự khác nhau giữa hai kết quả có thể xuất phát từ:

* RPM trong frame đã được lượng tử hóa.
* Tốc độ động cơ thay đổi trong một vòng quay.
* Sai số đo thời gian trên máy tính.
* Mất hoặc trễ frame UART.
* Giá trị RPM trong frame có thể là giá trị đã lọc.

---

## 8. Giải mã góc

### 8.1. Góc bắt đầu

Hai byte `start angle` được đọc theo thứ tự big-endian và chia cho 100:

```text
Start angle = start_angle_raw / 100
```

Ví dụ:

```text
Start angle raw = 0x6978
                = 27000

Start angle = 27000 / 100
            = 270.00°
```

### 8.2. Angle offset

Hai byte angle offset được đọc dưới dạng số nguyên có dấu:

```text
Angle offset = signed_raw / 100
```

Ví dụ:

```text
Angle offset raw = 0xFFE0
```

Giải mã theo số nguyên 16 bit có dấu:

```text
0xFFE0 = -32

Angle offset = -32 / 100
             = -0.32°
```

### 8.3. Phân bố góc của các điểm đo

Qua quan sát, một vòng quét có khoảng 16 frame và góc bắt đầu giữa hai frame liên tiếp thay đổi khoảng:

```text
360° / 16 = 22.5°
```

Mỗi frame chứa 17 điểm, tương ứng với 16 khoảng góc. Vì vậy, bước góc danh định có thể ước lượng:

```text
Δθ = 22.5° / 16
   = 1.40625°
```

Góc của điểm thứ `i` có thể tạm tính:

```text
angle[i] = start_angle + angle_offset + i × 1.40625°
```

với:

```text
i = 0, 1, 2, ..., 16
```

Sau đó chuẩn hóa góc về khoảng 0–360°:

```python
angle = angle % 360.0
```

Điểm cuối của frame trước có thể trùng với điểm đầu của frame sau. Khi dựng bản đồ điểm, có thể loại bỏ một trong hai điểm trùng này.

---

## 9. Giải mã điểm đo

Mỗi điểm đo chiếm 3 byte. Cấu trúc tạm thời được suy luận:

```text
Byte 0: Quality hoặc signal strength
Byte 1: Distance high byte
Byte 2: Distance low byte
```

Khoảng cách raw được đọc theo big-endian:

```text
distance_raw = byte1 × 256 + byte2
```

Hệ số khoảng cách được suy luận từ tính liên tục của dữ liệu,

```text
distance_mm ≈ distance_raw / 4
```

Ví dụ một điểm:

```text
92 31 EA
```

Trong đó:

```text
Quality      = 0x92
Distance raw = 0x31EA
             = 12778

Distance     = 12778 / 4
             = 3194.5 mm
             ≈ 3.19 m
```

Các điểm có dữ liệu:

```text
00 00 00
```

được xem là không hợp lệ hoặc không nhận được tín hiệu phản xạ.

Một điểm có thể được giữ lại khi thỏa mãn:

```text
distance_raw != 0
```

và khoảng cách nằm trong phạm vi làm việc dự kiến của LiDAR:

```text
0.13 m ≤ distance ≤ 8 m
```

---


## 14. Chương trình Python tách và giải mã frame

```python
import serial
import serial.tools.list_ports
import math
import time
import csv


# =========================================================
# CẤU HÌNH
# =========================================================

PORT = "/dev/ttyUSB0"          # Đổi thành COM của USB-TTL
BAUDRATE = 115200      # LiDAR của bạn đang gửi ở 115200

SAVE_CSV = False       # Đổi thành True nếu muốn lưu dữ liệu ra file CSV

MIN_DISTANCE_MM = 50   # bỏ điểm quá gần
MIN_QUALITY = 1        # có thể tăng lên 30, 50 nếu muốn lọc nhiễu


# =========================================================
# LIỆT KÊ CỔNG COM
# =========================================================

def list_com_ports():
    print("Danh sách cổng COM hiện có:")
    for port in serial.tools.list_ports.comports():
        print(f"  {port.device} - {port.description}")


# =========================================================
# CHECKSUM
# =========================================================

def checksum_16_sum(data: bytes) -> int:
    """
    Checksum của LiDAR:
    cộng toàn bộ byte trước checksum, lấy 16 bit thấp.
    """
    return sum(data) & 0xFFFF


def check_crc(frame: bytes) -> bool:
    if len(frame) < 10:
        return False

    crc_received = (frame[-2] << 8) | frame[-1]
    crc_calculated = checksum_16_sum(frame[:-2])

    return crc_received == crc_calculated


# =========================================================
# ĐỌC FRAME TỪ SERIAL
# =========================================================

def read_frame(ser: serial.Serial):
    """
    Frame có dạng:

    AA LEN_H LEN_L ID_H ID_L CMD PAYLOAD_LEN_H PAYLOAD_LEN_L ... CRC_H CRC_L

    Ví dụ:
    AA 00 40 01 61 AD 00 38 ...
    
    length_field = 0x0040 = 64
    tổng frame = 64 + 2 = 66 byte
    """

    while True:
        b = ser.read(1)

        if not b:
            return None

        # Tìm header AA
        if b[0] != 0xAA:
            continue

        length_bytes = ser.read(2)

        if len(length_bytes) != 2:
            return None

        length_field = (length_bytes[0] << 8) | length_bytes[1]

        total_length = length_field + 2

        # Lọc frame sai
        if total_length < 10 or total_length > 256:
            continue

        remaining = total_length - 3
        rest = ser.read(remaining)

        if len(rest) != remaining:
            return None

        frame = b + length_bytes + rest

        return frame


# =========================================================
# GIẢI MÃ FRAME HEALTH AE
# =========================================================

def parse_health_frame(frame: bytes):
    payload_len = (frame[6] << 8) | frame[7]

    if payload_len < 1:
        return None

    rpm_raw = frame[8]
    rpm = rpm_raw * 3

    return {
        "type": "HEALTH",
        "rpm": rpm,
        "rpm_raw": rpm_raw
    }


# =========================================================
# GIẢI MÃ FRAME ĐO KHOẢNG CÁCH AD
# =========================================================

def parse_measurement_frame(frame: bytes):
    """
    Cấu trúc frame AD của LiDAR bạn:

    Byte 0      : AA
    Byte 1-2    : length = 00 40
    Byte 3-4    : ID = 01 61
    Byte 5      : command = AD
    Byte 6-7    : payload length = 00 38

    Payload:
    payload[0]      : rpm_raw
    payload[1:3]    : offset angle, có thể là số âm
    payload[3:5]    : start angle
    payload[5:]     : 17 điểm đo, mỗi điểm 3 byte

    Mỗi điểm:
    quality distance_H distance_L
    """

    payload_len = (frame[6] << 8) | frame[7]
    payload = frame[8:8 + payload_len]

    if payload_len < 6:
        return []

    rpm_raw = payload[0]
    rpm = rpm_raw * 3

    # Offset angle: signed 16-bit
    offset_raw = (payload[1] << 8) | payload[2]
    if offset_raw & 0x8000:
        offset_raw -= 0x10000

    offset_angle = offset_raw / 100.0

    # Start angle
    start_angle_raw = (payload[3] << 8) | payload[4]
    start_angle = start_angle_raw / 100.0

    sample_data = payload[5:]

    if len(sample_data) % 3 != 0:
        return []

    sample_count = len(sample_data) // 3

    if sample_count <= 1:
        return []

    # Dữ liệu thực tế của bạn cho thấy mỗi frame cách nhau 22.5 độ
    angle_step = 22.5 / (sample_count - 1)

    points = []

    for i in range(sample_count):
        index = i * 3

        quality = sample_data[index]
        distance_raw = (sample_data[index + 1] << 8) | sample_data[index + 2]

        # Khoảng cách theo mm
        distance_mm = distance_raw * 0.25

        # Góc của từng điểm
        angle_deg = start_angle + i * angle_step

        if angle_deg >= 360.0:
            angle_deg -= 360.0

        if angle_deg < 0:
            angle_deg += 360.0

        # Bỏ điểm không hợp lệ
        if distance_raw == 0:
            continue

        if distance_mm < MIN_DISTANCE_MM:
            continue

        if quality < MIN_QUALITY:
            continue

        angle_rad = math.radians(angle_deg)

        x_mm = distance_mm * math.cos(angle_rad)
        y_mm = distance_mm * math.sin(angle_rad)

        points.append({
            "angle_deg": angle_deg,
            "distance_mm": distance_mm,
            "quality": quality,
            "rpm": rpm,
            "x_mm": x_mm,
            "y_mm": y_mm,
            "start_angle": start_angle,
            "offset_angle": offset_angle
        })

    return points


# =========================================================
# XỬ LÝ FRAME CHUNG
# =========================================================

def parse_frame(frame: bytes):
    if not check_crc(frame):
        return {
            "type": "CRC_ERROR",
            "raw": frame.hex(" ")
        }

    command = frame[5]

    if command == 0xAE:
        return parse_health_frame(frame)

    if command == 0xAD:
        points = parse_measurement_frame(frame)
        return {
            "type": "MEASUREMENT",
            "points": points
        }

    return {
        "type": "UNKNOWN",
        "command": command,
        "raw": frame.hex(" ")
    }


# =========================================================
# MAIN
# =========================================================

def main():
    list_com_ports()

    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=1)
        print(f"\nĐã mở {PORT} ở baudrate {BAUDRATE}")
        print("Đang đọc LiDAR...\n")

    except serial.SerialException as e:
        print(f"\nKhông mở được cổng {PORT}")
        print("Hãy kiểm tra lại COM của USB-TTL.")
        print(e)
        return

    csv_file = None
    writer = None

    if SAVE_CSV:
        csv_file = open("lidar_data.csv", "w", newline="", encoding="utf-8")
        writer = csv.writer(csv_file)
        writer.writerow([
            "time",
            "angle_deg",
            "distance_mm",
            "quality",
            "rpm",
            "x_mm",
            "y_mm"
        ])

    try:
        while True:
            frame = read_frame(ser)

            if frame is None:
                continue

            result = parse_frame(frame)

            if result is None:
                continue

            if result["type"] == "CRC_ERROR":
                print("CRC sai, bỏ frame")
                continue

            if result["type"] == "HEALTH":
                print(f"HEALTH | RPM: {result['rpm']:.1f}")
                continue

            if result["type"] == "MEASUREMENT":
                for p in result["points"]:
                    print(
                        f"Góc: {p['angle_deg']:.2f}° | "
                        f"Khoảng cách: {p['distance_mm']:.1f} mm | "
                        f"Quality: {p['quality']} | "
                        f"RPM: {p['rpm']:.1f} | "
                        f"X: {p['x_mm']:.1f} mm | "
                        f"Y: {p['y_mm']:.1f} mm"
                    )

                    if writer is not None:
                        writer.writerow([
                            time.time(),
                            p["angle_deg"],
                            p["distance_mm"],
                            p["quality"],
                            p["rpm"],
                            p["x_mm"],
                            p["y_mm"]
                        ])
                        csv_file.flush()

            if result["type"] == "UNKNOWN":
                print(f"Frame khác | CMD = 0x{result['command']:02X}")

    except KeyboardInterrupt:
        print("\nĐã dừng chương trình.")

    finally:
        ser.close()

        if csv_file is not None:
            csv_file.close()


if __name__ == "__main__":
    main()
```
> **Lưu ý:** Chương trình được AI viết dựa trên cấu trúc của frame được phân tích ở trên
---




## 9. Những nội dung cần tiếp tục kiểm chứng

Do không có tài liệu giao thức chính thức, các nội dung sau cần được kiểm tra thêm:

1. Hệ số chính xác chuyển đổi `RPM_raw` sang RPM.
2. Ý nghĩa chính xác của trường angle offset.
3. Hệ số chuyển đổi khoảng cách có đúng bằng `1/4` hay không.
4. Byte quality là cường độ tín hiệu, trạng thái hay chứa thêm bit cờ.
5. Số điểm duy nhất trong một vòng sau khi loại bỏ điểm trùng.
6. Ảnh hưởng của tốc độ quay đến số điểm và tần số lấy mẫu.
7. Độ chính xác khoảng cách tại các bề mặt có độ phản xạ khác nhau.

Các hệ số nên được xác minh bằng cách đặt vật cản phẳng tại những khoảng cách biết trước, chẳng hạn:

```text
0.5 m, 1 m, 2 m, 3 m và 5 m
```

Sau đó so sánh giá trị raw với khoảng cách thực để xây dựng hàm chuyển đổi chính xác.

---

## 20. Kết luận

Quá trình nghiên cứu cho thấy có thể đọc và giải mã dữ liệu từ LiDAR ngay cả khi không có tài liệu giao thức chính thức. Phương pháp thực hiện dựa trên việc thu dữ liệu UART thô, quan sát sự lặp lại của frame, sau đó đối chiếu các trường dữ liệu.

Kết quả hiện tại đã đủ để tái tạo dữ liệu quét 2D cơ bản và là nền tảng cho các bước phát triển tiếp theo như xây dựng ROS 2 driver, xuất dữ liệu `LaserScan`, hiển thị trên RViz và tích hợp LiDAR vào mobile robot phục vụ nghiên cứu SLAM.
