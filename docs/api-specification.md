# Đặc tả Kỹ thuật API (API Specification)

Hệ thống cung cấp các REST API cho giao tiếp giữa thiết bị biên (ESP32) và máy chủ giám sát (Node-RED).

---

## 1. Thiết bị gửi sự kiện phát hiện chuyển động

- **Endpoint:** `/api/devices/events`
- **Method:** `POST`
- **Headers:**
  - `Content-Type: application/json`
  - `X-API-Key: blind-corner-secret-token`

### Payload Mẫu:
```json
{
  "deviceId": "blind-corner-001",
  "direction": "LEFT",
  "eventType": "MOTION_DETECTED",
  "timestamp": 1794300000
}
```

### Chi tiết các trường:
| Trường | Kiểu dữ liệu | Bắt buộc | Mô tả |
|---|---|:---:|---|
| `deviceId` | String | Có | Mã định danh thiết bị (`blind-corner-001`) |
| `direction` | String | Có | Hướng xe phát hiện: `"LEFT"` hoặc `"RIGHT"` |
| `eventType` | String | Có | Loại sự kiện: `"MOTION_DETECTED"` |
| `timestamp` | Number | Có | Mốc thời gian (Unix timestamp tính bằng giây) |

### Phản hồi:
- **HTTP 200 OK:**
```json
{
  "status": "success",
  "message": "Event recorded"
}
```

### Lệnh cURL kiểm thử mẫu:
```bash
curl -X POST http://localhost:1880/api/devices/events \
  -H "Content-Type: application/json" \
  -H "X-API-Key: blind-corner-secret-token" \
  -d "{\"deviceId\": \"blind-corner-001\", \"direction\": \"LEFT\", \"eventType\": \"MOTION_DETECTED\", \"timestamp\": 1715000000}"
```

---

## 2. Thiết bị gửi báo cáo nhịp tim (Heartbeat)

- **Endpoint:** `/api/devices/heartbeat`
- **Method:** `POST`
- **Tần suất:** Định kỳ mỗi 15 giây
- **Headers:**
  - `Content-Type: application/json`
  - `X-API-Key: blind-corner-secret-token`

### Payload Mẫu:
```json
{
  "deviceId": "blind-corner-001",
  "left": true,
  "right": false,
  "ledLeft": false,
  "ledRight": true,
  "leftDetections": 5,
  "rightDetections": 3,
  "uptime": 120,
  "firmwareVersion": "1.0.0"
}
```

### Chi tiết các trường:
| Trường | Kiểu dữ liệu | Mô tả |
|---|---|---|
| `deviceId` | String | Mã định danh thiết bị |
| `left` | Boolean | Trạng thái tức thời của PIR Trái (`true` nếu đang có xe) |
| `right` | Boolean | Trạng thái tức thời của PIR Phải (`true` nếu đang có xe) |
| `ledLeft` | Boolean | Trạng thái tức thời của LED Trái |
| `ledRight` | Boolean | Trạng thái tức thời của LED Phải |
| `leftDetections` | Number | Tổng số lượt xe bên Trái từ khi khởi động |
| `rightDetections` | Number | Tổng số lượt xe bên Phải từ khi khởi động |
| `uptime` | Number | Thời gian thiết bị đã chạy liên tục (giây) |
| `firmwareVersion` | String | Phiên bản firmware (ví dụ `"1.0.0"`) |

### Phản hồi:
- **HTTP 200 OK:** Ghi nhận nhịp tim thành công, gia hạn bộ đếm Watchdog 45 giây.

### Lệnh cURL kiểm thử mẫu:
```bash
curl -X POST http://localhost:1880/api/devices/heartbeat \
  -H "Content-Type: application/json" \
  -H "X-API-Key: blind-corner-secret-token" \
  -d "{\"deviceId\": \"blind-corner-001\", \"left\": false, \"right\": false, \"ledLeft\": false, \"ledRight\": false, \"leftDetections\": 12, \"rightDetections\": 8, \"uptime\": 360, \"firmwareVersion\": \"1.0.0\"}"
```

---

## 3. Tra cứu trạng thái thiết bị

- **Endpoint:** `/api/devices/status`
- **Method:** `GET`
- **Mô tả:** Đọc trạng thái Online/Offline, bộ đếm xe và uptime hiện tại từ SQLite.

### Phản hồi mẫu (HTTP 200 OK):
```json
[
  {
    "device_id": "blind-corner-001",
    "is_online": 1,
    "last_heartbeat": "2026-11-10 21:40:00",
    "left_active": 0,
    "right_active": 0,
    "led_left_active": 0,
    "led_right_active": 0,
    "left_detections": 12,
    "right_detections": 8,
    "uptime": 360,
    "firmware_version": "1.0.0",
    "updated_at": "2026-11-10 21:40:00"
  }
]
```

---

## 4. Tra cứu lịch sử sự kiện

- **Endpoint:** `/api/devices/history`
- **Method:** `GET`
- **Mô tả:** Lấy danh sách 50 sự kiện xe qua lại gần nhất được lưu trong cơ sở dữ liệu.

### Phản hồi mẫu (HTTP 200 OK):
```json
[
  {
    "id": 1,
    "device_id": "blind-corner-001",
    "direction": "LEFT",
    "event_type": "MOTION_DETECTED",
    "device_timestamp": 1715000000,
    "created_at": "2026-11-10 21:35:12"
  }
]
```

---

## 5. Tra cứu thống kê lưu lượng

- **Endpoint:** `/api/devices/statistics`
- **Method:** `GET`
- **Mô tả:** Tính tổng số lượt xe từng chiều dựa trên lịch sử trong SQLite.

### Phản hồi mẫu (HTTP 200 OK):
```json
[
  {
    "direction": "LEFT",
    "total_events": 14,
    "earliest_event": "2026-11-10 20:00:00",
    "latest_event": "2026-11-10 21:35:12"
  },
  {
    "direction": "RIGHT",
    "total_events": 9,
    "earliest_event": "2026-11-10 20:05:00",
    "latest_event": "2026-11-10 21:34:00"
  }
]
```
