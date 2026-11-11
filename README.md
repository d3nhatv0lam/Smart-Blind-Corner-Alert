# 🚦 Smart Blind-Corner Alert System

> 🇬🇧 [English version below](#-english-version) · 🇻🇳 Phiên bản tiếng Việt phía trên

Hệ thống cảnh báo điểm mù thông minh sử dụng **ESP32 + PIR AM312 + Node-RED + SQLite**, giúp cảnh báo an toàn tại các góc khuất giao thông nguy hiểm (ngã ba, góc tường...).

---

## ✨ Tính năng

| Tính năng | Mô tả |
|---|---|
| ⚡ Phản hồi cực nhanh | Cảnh báo đèn LED tại chỗ < 5ms (FreeRTOS Dual-Core) |
| 📡 Giám sát từ xa | Truyền sự kiện lên Node-RED qua HTTP ngay lập tức |
| 💓 Heartbeat | Báo cáo trạng thái thiết bị mỗi 15 giây |
| 📊 Dashboard web | Giao diện theo dõi thời gian thực và phân tích lưu lượng |
| 💾 Lịch sử sự kiện | Lưu toàn bộ sự kiện xe vào SQLite |
| 🔌 Chế độ Standalone | Hoạt động độc lập không cần mạng (bản dự phòng) |

---

## 🏗️ Kiến trúc hệ thống

```
┌─────────────────────────────────────────┐
│              ESP32 (Firmware)           │
│  Core 1: sensorAlertTask                │
│    PIR Trái ──▶ LED Phải (< 5ms)       │
│    PIR Phải ──▶ LED Trái (< 5ms)       │
│  Core 0: networkTask                    │
│    ──▶ POST /api/devices/events         │
│    ──▶ POST /api/devices/heartbeat      │
└─────────────────┬───────────────────────┘
                  │ Wi-Fi (HTTP)
                  ▼
┌─────────────────────────────────────────┐
│          Node-RED (Backend)             │
│    Nhận sự kiện ──▶ Lưu SQLite         │
│    Dashboard: http://localhost:1880/    │
│              dashboard/home             │
└─────────────────────────────────────────┘
```

**Sơ đồ kết nối phần cứng:**

```
GPIO 18 ◀── PIR Trái
GPIO 17 ──▶ LED Phải (cảnh báo cho xe bên Phải khi có xe bên Trái)
GPIO 16 ◀── PIR Phải
GPIO 19 ──▶ LED Trái (cảnh báo cho xe bên Trái khi có xe bên Phải)
```

---

## 📁 Cấu trúc thư mục

```
Smart-Blind-Corner-Alert/
├── firmware/
│   ├── smart-blind-corner-alert/
│   │   └── smart-blind-corner-alert.ino  # Firmware chính (IoT + FreeRTOS)
│   └── standalone/
│       └── standalone.ino                # Firmware dự phòng (không cần mạng)
├── backend/
│   └── node-red/
│       └── flows.json                    # Luồng Node-RED (import vào Node-RED)
├── database/
│   └── schema.sql                        # Schema SQLite (tự tạo khi chạy)
└── docs/
    └── api-specification.md              # Đặc tả REST API
```

---

## 🛠️ Yêu cầu hệ thống

### Phần cứng
- **Vi điều khiển:** ESP32 (bất kỳ board nào có Wi-Fi)
- **Cảm biến:** 2× PIR AM312 (hoặc tương đương)
- **Đèn cảnh báo:** 2× LED (kèm điện trở hạn dòng ~220Ω)
- **Nguồn điện:** 5V USB hoặc adapter

### Phần mềm
- [Arduino IDE](https://www.arduino.cc/en/software) ≥ 2.0 hoặc PlatformIO
- [Node.js](https://nodejs.org/) ≥ 18 (để chạy Node-RED)
- [Node-RED](https://nodered.org/) ≥ 3.0

---

## 🚀 Hướng dẫn cài đặt

### Bước 1 — Cài đặt Node-RED và các Node cần thiết

```bash
# Cài Node-RED (nếu chưa có)
npm install -g --unsafe-perm node-red

# Khởi động Node-RED lần đầu để tạo thư mục cấu hình
node-red
# Nhấn Ctrl+C để dừng sau khi thấy dòng "Server now running at ..."

# Cài các node bổ sung cần thiết
cd ~/.node-red        # Windows: cd %USERPROFILE%\.node-red
npm install node-red-node-sqlite
npm install @flowfuse/node-red-dashboard
```

> **Lưu ý (Windows):** Thư mục cấu hình Node-RED nằm tại `%USERPROFILE%\.node-red`

### Bước 2 — Import luồng Node-RED

1. Khởi động Node-RED: `node-red`
2. Mở trình duyệt: [http://localhost:1880](http://localhost:1880)
3. Click menu ☰ (góc trên phải) → **Import**
4. Click **"select a file to import"** → chọn file `backend/node-red/flows.json`
5. Click **Import** → **Deploy** (nút đỏ góc trên phải)

### Bước 3 — Nạp firmware vào ESP32

#### 3.1 Cài đặt thư viện Arduino

Mở Arduino IDE → **Sketch → Include Library → Manage Libraries**, tìm và cài:

| Thư viện | Phiên bản khuyến nghị |
|---|---|
| `ArduinoJson` by Benoit Blanchon | ≥ 7.x |

> **WiFi** và **HTTPClient** đã tích hợp sẵn trong ESP32 Arduino Core.

#### 3.2 Cài ESP32 Board vào Arduino IDE

1. Mở **File → Preferences**
2. Thêm URL sau vào **"Additional boards manager URLs"**:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Mở **Tools → Board → Boards Manager**, tìm `esp32`, cài **"esp32 by Espressif Systems"**

#### 3.3 Chỉnh sửa thông tin kết nối trong firmware

Mở file [`firmware/smart-blind-corner-alert/smart-blind-corner-alert.ino`](firmware/smart-blind-corner-alert/smart-blind-corner-alert.ino), sửa phần cấu hình:

```cpp
// === THAY ĐỔI CÁC GIÁ TRỊ NÀY ===
const char* WIFI_SSID       = "TEN_WIFI_CUA_BAN";       // Tên Wi-Fi 2.4GHz
const char* WIFI_PASSWORD   = "MAT_KHAU_WIFI";           // Mật khẩu Wi-Fi
const char* SERVER_BASE_URL = "http://192.168.x.x:1880"; // IP máy tính chạy Node-RED
const char* DEVICE_ID       = "blind-corner-001";         // ID thiết bị (giữ nguyên)
const char* API_KEY         = "blind-corner-secret-token"; // API key (giữ nguyên)
```

> **Lấy IP máy tính:**  
> - Windows: chạy `ipconfig` trong CMD, lấy địa chỉ **IPv4**  
> - Linux/macOS: chạy `ip addr` hoặc `ifconfig`  
> - ESP32 và máy tính phải cùng mạng Wi-Fi 2.4GHz

#### 3.4 Nạp firmware

1. Kết nối ESP32 vào máy tính qua cáp USB
2. Chọn **Tools → Board → ESP32 Arduino → "ESP32 Dev Module"** (hoặc board phù hợp)
3. Chọn **Tools → Port** → chọn cổng COM của ESP32
4. Nhấn **Upload** (Ctrl+U)
5. Mở **Serial Monitor** (Ctrl+Shift+M), baud rate **115200** để theo dõi log

---

## 📱 Sử dụng hệ thống

### Truy cập Dashboard

Sau khi Node-RED đang chạy và ESP32 đã kết nối Wi-Fi thành công:

```
http://localhost:1880/dashboard/home
```

Dashboard hiển thị:
- **Chỉ số tổng quan:** Tổng xe bên Trái / Phải, trạng thái Online/Offline
- **Sơ đồ ngã 3:** Trực quan hóa top-down vị trí xe theo thời gian thực
- **Biểu đồ phân tích:** Lưu lượng xe theo giờ, giờ cao điểm
- **Lịch sử sự kiện:** 50 sự kiện gần nhất

### Kiểm tra hoạt động qua Serial Monitor

Khi firmware chạy đúng, Serial Monitor sẽ hiển thị:

```
========================================================
  SMART BLIND-CORNER ALERT SYSTEM (ESP32 IoT FreeRTOS)
========================================================
[WiFi] Dang ket noi toi SSID: TEN_WIFI ...
[WiFi] Da ket noi! IP: 192.168.x.x
[HEARTBEAT] OK. Uptime: 15 s, Code: 200
[HTTP] Gui su kien xe (LEFT) thanh cong, code: 200
```

### Chế độ Standalone (không cần mạng)

Nếu không cần giám sát từ xa, nạp firmware dự phòng:

```
firmware/standalone/standalone.ino
```

Chế độ này chỉ điều khiển LED cục bộ, không kết nối Wi-Fi, phù hợp để:
- Kiểm tra phần cứng (PIR + LED)
- Dùng làm bản dự phòng khi mất mạng

---

## 🔌 REST API

Xem chi tiết tại [`docs/api-specification.md`](docs/api-specification.md).

| Endpoint | Method | Mô tả |
|---|---|---|
| `/api/devices/events` | POST | ESP32 gửi sự kiện phát hiện xe |
| `/api/devices/heartbeat` | POST | ESP32 gửi báo cáo trạng thái mỗi 15s |
| `/api/devices/status` | GET | Tra cứu trạng thái thiết bị |
| `/api/devices/history` | GET | Lấy 50 sự kiện gần nhất |
| `/api/devices/statistics` | GET | Thống kê lưu lượng xe theo chiều |

### Kiểm thử API nhanh

```bash
# Gửi sự kiện xe bên Trái (giả lập ESP32)
curl -X POST http://localhost:1880/api/devices/events \
  -H "Content-Type: application/json" \
  -H "X-API-Key: blind-corner-secret-token" \
  -d "{\"deviceId\": \"blind-corner-001\", \"direction\": \"LEFT\", \"eventType\": \"MOTION_DETECTED\", \"timestamp\": 1715000000}"

# Kiểm tra trạng thái thiết bị
curl http://localhost:1880/api/devices/status
```

---

## 🗄️ Cơ sở dữ liệu

SQLite database (`blind_corner.db`) được Node-RED tự tạo tại thư mục làm việc. Schema gồm 2 bảng chính:

- **`motion_events`** — Lịch sử toàn bộ sự kiện xe qua lại
- **`device_status`** — Trạng thái hiện tại và sức khỏe thiết bị (cập nhật theo Heartbeat)

Xem schema đầy đủ tại [`database/schema.sql`](database/schema.sql).

---

## ❓ Xử lý sự cố thường gặp

| Triệu chứng | Nguyên nhân | Giải pháp |
|---|---|---|
| ESP32 không kết nối được Wi-Fi | Sai SSID/password hoặc dùng Wi-Fi 5GHz | Kiểm tra SSID/pass, dùng băng tần 2.4GHz |
| `[HTTP] Gui su kien that bai! Code: -1` | Sai IP máy chủ hoặc Node-RED chưa chạy | Kiểm tra `SERVER_BASE_URL` và khởi động Node-RED |
| Dashboard không hiện dữ liệu | Node `@flowfuse/node-red-dashboard` chưa cài | Chạy `npm install @flowfuse/node-red-dashboard` trong `~/.node-red` |
| PIR không phát hiện chuyển động | Cảm biến cần thời gian khởi động | Chờ 30-60 giây sau khi cấp nguồn cho PIR AM312 |
| `node-red-node-sqlite` lỗi | Chưa cài node SQLite | Chạy `npm install node-red-node-sqlite` trong `~/.node-red` |

---

## 📄 Giấy phép

Dự án phục vụ mục đích học tập và nghiên cứu.

---

---

# 🇬🇧 English Version

> 🇻🇳 [Phiên bản tiếng Việt ở trên](#-smart-blind-corner-alert-system)

A smart blind-corner alert system built with **ESP32 + PIR AM312 + Node-RED + SQLite** to warn drivers of approaching vehicles at dangerous blind intersections (T-junctions, hidden corners, etc.).

---

## ✨ Features

| Feature | Description |
|---|---|
| ⚡ Ultra-fast response | Local LED alert < 5ms (FreeRTOS Dual-Core) |
| 📡 Remote monitoring | Instant event push to Node-RED over HTTP |
| 💓 Heartbeat | Device status report every 15 seconds |
| 📊 Web dashboard | Real-time monitoring and traffic analytics |
| 💾 Event history | Full event log stored in SQLite |
| 🔌 Standalone mode | Works offline without any network (fallback) |

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────┐
│              ESP32 (Firmware)           │
│  Core 1: sensorAlertTask                │
│    PIR Left  ──▶ LED Right (< 5ms)     │
│    PIR Right ──▶ LED Left  (< 5ms)     │
│  Core 0: networkTask                    │
│    ──▶ POST /api/devices/events         │
│    ──▶ POST /api/devices/heartbeat      │
└─────────────────┬───────────────────────┘
                  │ Wi-Fi (HTTP)
                  ▼
┌─────────────────────────────────────────┐
│          Node-RED (Backend)             │
│    Receive events ──▶ Store in SQLite   │
│    Dashboard: http://localhost:1880/    │
│              dashboard/home             │
└─────────────────────────────────────────┘
```

**Hardware wiring:**

```
GPIO 18 ◀── PIR Left   (detects vehicles approaching from the left)
GPIO 17 ──▶ LED Right  (warns right-side drivers when left PIR triggers)
GPIO 16 ◀── PIR Right  (detects vehicles approaching from the right)
GPIO 19 ──▶ LED Left   (warns left-side drivers when right PIR triggers)
```

---

## 📁 Project Structure

```
Smart-Blind-Corner-Alert/
├── firmware/
│   ├── smart-blind-corner-alert/
│   │   └── smart-blind-corner-alert.ino  # Main firmware (IoT + FreeRTOS)
│   └── standalone/
│       └── standalone.ino                # Offline fallback firmware
├── backend/
│   └── node-red/
│       └── flows.json                    # Node-RED flow (import this)
├── database/
│   └── schema.sql                        # SQLite schema (auto-created at runtime)
└── docs/
    └── api-specification.md              # REST API specification
```

---

## 🛠️ Requirements

### Hardware
- **Microcontroller:** ESP32 (any Wi-Fi-capable board)
- **Sensors:** 2× PIR AM312 (or equivalent)
- **Alert lights:** 2× LED (with ~220Ω current-limiting resistors)
- **Power supply:** 5V USB or DC adapter

### Software
- [Arduino IDE](https://www.arduino.cc/en/software) ≥ 2.0 or PlatformIO
- [Node.js](https://nodejs.org/) ≥ 18 (required to run Node-RED)
- [Node-RED](https://nodered.org/) ≥ 3.0

---

## 🚀 Installation

### Step 1 — Install Node-RED and required nodes

```bash
# Install Node-RED globally (skip if already installed)
npm install -g --unsafe-perm node-red

# Start Node-RED once to generate its config directory, then stop it
node-red
# Press Ctrl+C once you see "Server now running at ..."

# Install required additional nodes
cd ~/.node-red        # Windows: cd %USERPROFILE%\.node-red
npm install node-red-node-sqlite
npm install @flowfuse/node-red-dashboard
```

> **Windows note:** The Node-RED config directory is at `%USERPROFILE%\.node-red`

### Step 2 — Import the Node-RED flow

1. Start Node-RED: `node-red`
2. Open your browser: [http://localhost:1880](http://localhost:1880)
3. Click the ☰ menu (top-right) → **Import**
4. Click **"select a file to import"** → select `backend/node-red/flows.json`
5. Click **Import** → then click **Deploy** (red button, top-right)

### Step 3 — Flash the firmware to ESP32

#### 3.1 Install Arduino library

Open Arduino IDE → **Sketch → Include Library → Manage Libraries**, search and install:

| Library | Recommended version |
|---|---|
| `ArduinoJson` by Benoit Blanchon | ≥ 7.x |

> **WiFi** and **HTTPClient** are built into the ESP32 Arduino Core — no separate install needed.

#### 3.2 Add ESP32 board to Arduino IDE

1. Open **File → Preferences**
2. Add the following URL to **"Additional boards manager URLs"**:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Open **Tools → Board → Boards Manager**, search `esp32`, install **"esp32 by Espressif Systems"**

#### 3.3 Edit connection settings in the firmware

Open [`firmware/smart-blind-corner-alert/smart-blind-corner-alert.ino`](firmware/smart-blind-corner-alert/smart-blind-corner-alert.ino) and update the config block:

```cpp
// === EDIT THESE VALUES ===
const char* WIFI_SSID       = "YOUR_WIFI_SSID";          // 2.4GHz Wi-Fi name
const char* WIFI_PASSWORD   = "YOUR_WIFI_PASSWORD";       // Wi-Fi password
const char* SERVER_BASE_URL = "http://192.168.x.x:1880";  // IP of the PC running Node-RED
const char* DEVICE_ID       = "blind-corner-001";          // Device ID (keep as-is)
const char* API_KEY         = "blind-corner-secret-token"; // API key (keep as-is)
```

> **How to find your PC's IP address:**
> - **Windows:** Run `ipconfig` in CMD → look for **IPv4 Address**
> - **Linux/macOS:** Run `ip addr` or `ifconfig`
> - The ESP32 and your PC must be on the **same 2.4GHz Wi-Fi network**

#### 3.4 Upload the firmware

1. Connect the ESP32 to your PC via USB
2. Select **Tools → Board → ESP32 Arduino → "ESP32 Dev Module"** (or your specific board)
3. Select **Tools → Port** → choose the ESP32's COM port
4. Click **Upload** (Ctrl+U)
5. Open **Serial Monitor** (Ctrl+Shift+M), set baud rate to **115200** to view logs

---

## 📱 Usage

### Access the Dashboard

Once Node-RED is running and the ESP32 has connected to Wi-Fi:

```
http://localhost:1880/dashboard/home
```

The dashboard displays:
- **KPI overview:** Total left / right vehicle counts, Online/Offline status
- **Intersection map:** Top-down real-time visualization of vehicle positions
- **Traffic analytics:** Hourly traffic chart and peak-hour analysis
- **Event history:** 50 most recent events

### Monitor via Serial Monitor

When the firmware is running correctly, the Serial Monitor output looks like:

```
========================================================
  SMART BLIND-CORNER ALERT SYSTEM (ESP32 IoT FreeRTOS)
========================================================
[WiFi] Dang ket noi toi SSID: YOUR_WIFI ...
[WiFi] Da ket noi! IP: 192.168.x.x
[HEARTBEAT] OK. Uptime: 15 s, Code: 200
[HTTP] Gui su kien xe (LEFT) thanh cong, code: 200
```

### Standalone Mode (no network required)

If remote monitoring is not needed, flash the fallback firmware instead:

```
firmware/standalone/standalone.ino
```

This version controls the LEDs locally with no Wi-Fi, ideal for:
- Hardware testing (PIR sensors + LEDs)
- Fallback operation when the network is unavailable

---

## 🔌 REST API

Full specification: [`docs/api-specification.md`](docs/api-specification.md)

| Endpoint | Method | Description |
|---|---|---|
| `/api/devices/events` | POST | ESP32 sends a motion detection event |
| `/api/devices/heartbeat` | POST | ESP32 sends a status report every 15s |
| `/api/devices/status` | GET | Query current device status |
| `/api/devices/history` | GET | Retrieve the 50 most recent events |
| `/api/devices/statistics` | GET | Traffic statistics by direction |

### Quick API test

```bash
# Simulate a LEFT vehicle detection event
curl -X POST http://localhost:1880/api/devices/events \
  -H "Content-Type: application/json" \
  -H "X-API-Key: blind-corner-secret-token" \
  -d "{\"deviceId\": \"blind-corner-001\", \"direction\": \"LEFT\", \"eventType\": \"MOTION_DETECTED\", \"timestamp\": 1715000000}"

# Check device status
curl http://localhost:1880/api/devices/status
```

---

## 🗄️ Database

The SQLite database (`blind_corner.db`) is automatically created by Node-RED in its working directory. It contains two tables:

- **`motion_events`** — Full history of all vehicle detection events
- **`device_status`** — Current device state and health, updated on each Heartbeat

Full schema: [`database/schema.sql`](database/schema.sql)

---

## ❓ Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| ESP32 cannot connect to Wi-Fi | Wrong SSID/password, or 5GHz band used | Verify credentials; use 2.4GHz network |
| `[HTTP] Gui su kien that bai! Code: -1` | Wrong server IP or Node-RED not running | Check `SERVER_BASE_URL`; start Node-RED |
| Dashboard shows no data | `@flowfuse/node-red-dashboard` not installed | Run `npm install @flowfuse/node-red-dashboard` inside `~/.node-red` |
| PIR sensor not detecting motion | Sensor warm-up time needed | Wait 30–60 seconds after powering on the PIR AM312 |
| `node-red-node-sqlite` error | SQLite node not installed | Run `npm install node-red-node-sqlite` inside `~/.node-red` |

---

## 📄 License

This project is intended for educational and research purposes.
