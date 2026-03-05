# HeartRate-SpO2-Monitor 💓

## 🇬🇧 English

A low-cost IoT-based heart rate and blood oxygen (SpO₂) monitoring system built
with ESP32, MAX30102 sensor, and OLED display — developed as a Biomedical
Electronics course project at Ho Chi Minh City University of Science.

### Overview

This project designs and implements a real-time physiological monitoring device
that measures heart rate (BPM) and blood oxygen saturation (SpO₂) using
photoplethysmography (PPG). Data is displayed locally on an OLED screen and
streamed to the cloud via ThingSpeak and Blynk IoT platforms.

### Hardware Components
| Component | Role |
|---|---|
| ESP32 (WROVER CAM) | Main microcontroller, WiFi, data processing |
| MAX30102 | Optical PPG sensor (RED 660nm + IR 880nm) |
| OLED 0.96" 128×64 | Real-time local display |
| RGB LED | System status indicator |
| Buzzer | Abnormal value alert |

### Features
- Real-time heart rate detection via IR peak detection algorithm
- SpO₂ calculation using RED/IR absorption ratio: `SpO2 = 104 - 17 × R`
- OLED display with animated heart icon
- RGB LED status: cyan (startup), yellow blink (WiFi error), rainbow (normal)
- Buzzer alert when BPM < 40 or > 100, or SpO₂ < 90%
- IoT data streaming to ThingSpeak (every 10s) and Blynk (every 5s)
- Finger detection and signal saturation check

### Algorithms
- **Heart Rate:** Peak detection on IR signal, averaged over 4 samples
- **SpO₂:** RMS-based AC/DC ratio of RED and IR channels over 100 samples

### IoT Integration
- **ThingSpeak:** Field 1 → BPM, Field 2 → SpO₂ (time-series charts)
- **Blynk:** V3 → BPM, V4 → SpO₂ (real-time gauges on mobile app)

### Technologies
- Arduino / ESP32 (C++)
- Libraries: MAX30105, Adafruit_SSD1306, BlynkSimpleEsp32, Wire
- Platforms: ThingSpeak, Blynk IoT

### Authors
- Lê Trần Anh Kiệt – 22207116
- Lê Hoàng Bảo Ngọc – 22207119
- Lê Thanh Vy – 22207107

**Course:** Biomedical Electronics — HCMUS  
**Instructor:** Lê Đức Hùng | **Group:** Nhóm 3

---

## 🇻🇳 Tiếng Việt

Hệ thống đo nhịp tim và nồng độ oxy trong máu (SpO₂) chi phí thấp, sử dụng
ESP32, cảm biến MAX30102 và màn hình OLED — thực hiện trong khuôn khổ đồ án
môn Điện Tử Y Sinh tại Trường Đại học Khoa học Tự nhiên TP.HCM.

### Tổng quan

Đề tài thiết kế và triển khai thiết bị giám sát sinh hiệu theo thời gian thực,
đo nhịp tim (BPM) và độ bão hòa oxy trong máu (SpO₂) theo nguyên lý đo mạch
bằng ánh sáng (PPG). Dữ liệu được hiển thị trực tiếp trên màn hình OLED và
truyền lên cloud qua ThingSpeak và Blynk.

### Linh kiện phần cứng
| Linh kiện | Vai trò |
|---|---|
| ESP32 (WROVER CAM) | Vi điều khiển chính, WiFi, xử lý dữ liệu |
| MAX30102 | Cảm biến quang PPG (LED đỏ 660nm + IR 880nm) |
| OLED 0.96" 128×64 | Hiển thị kết quả thời gian thực |
| LED RGB | Chỉ thị trạng thái hệ thống |
| Buzzer | Cảnh báo khi chỉ số bất thường |

### Tính năng
- Đo nhịp tim theo thời gian thực bằng thuật toán phát hiện đỉnh (peak detection)
- Tính SpO₂ qua tỷ lệ hấp thụ RED/IR: `SpO2 = 104 - 17 × R`
- Hiển thị OLED kèm icon tim nhịp đập
- LED RGB báo trạng thái: cyan (khởi động), vàng nhấp nháy (mất WiFi), cầu vồng (bình thường)
- Buzzer cảnh báo khi BPM < 40 hoặc > 100, hoặc SpO₂ < 90%
- Gửi dữ liệu lên ThingSpeak (mỗi 10s) và Blynk (mỗi 5s)
- Kiểm tra đặt ngón tay và phát hiện tín hiệu bão hòa

### Thuật toán
- **Nhịp tim:** Phát hiện đỉnh tín hiệu IR, trung bình hóa 4 mẫu gần nhất
- **SpO₂:** Tính AC/DC theo RMS cho kênh RED và IR trên 100 mẫu liên tiếp

### Kết nối IoT
- **ThingSpeak:** Field 1 → BPM, Field 2 → SpO₂ (biểu đồ thời gian thực)
- **Blynk:** V3 → BPM, V4 → SpO₂ (đồng hồ gauge trên ứng dụng di động)

### Công nghệ
- Arduino / ESP32 (C++)
- Thư viện: MAX30105, Adafruit_SSD1306, BlynkSimpleEsp32, Wire
- Nền tảng IoT: ThingSpeak, Blynk

### Thành viên nhóm
- Lê Trần Anh Kiệt – 22207116
- Lê Hoàng Bảo Ngọc – 22207119
- Lê Thanh Vy – 22207107

**Môn học:** Điện Tử Y Sinh — ĐHKHTN TP.HCM  
**Giảng viên:** Lê Đức Hùng | **Nhóm:** Nhóm 3
