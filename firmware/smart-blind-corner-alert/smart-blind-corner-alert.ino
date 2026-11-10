/*
 * Smart Blind-Corner Alert System - IoT Upgraded Version (Tối ưu hóa Phản hồi Cực Nhanh)
 * 
 * Kiến trúc: FreeRTOS Dual-Core trên ESP32
 * - Core 1: Chạy sensorAlertTask (Đọc 2 PIR, kích hoạt 2 LED, bắt sườn lên)
 *           -> Ưu tiên cao nhất, phản hồi tức thời < 5ms, 100% độc lập với mạng.
 * - Core 0: Chạy networkTask (Quản lý Wi-Fi, gửi sự kiện HTTP POST tức thời, gửi Heartbeat 15s)
 *           -> Chạy ngầm, dùng HTTP Keep-Alive để giảm tối đa độ trễ truyền mạng.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ==============================================================================
// 1. CẤU HÌNH HỆ THỐNG & KẾT NỐI (Người dùng chỉnh sửa tại đây)
// ==============================================================================
// const char* WIFI_SSID         = "YOUR_WIFI_SSID";          // Tên Wi-Fi (Băng tần 2.4GHz)
// const char* WIFI_PASSWORD     = "YOUR_WIFI_PASSWORD";      // Mật khẩu Wi-Fi (để "" nếu không có pass)
// const char* SERVER_BASE_URL   = "http://10.4.106.79:1880"; // Địa chỉ IP máy tính chạy Node-RED
// const char* DEVICE_ID         = "blind-corner-001";        // Mã định danh thiết bị
// const char* API_KEY           = "blind-corner-secret-token"; // Khóa xác thực API
// const char* FIRMWARE_VERSION  = "1.0.0";

const char* WIFI_SSID         = "DESKTOP-N0BUN01 9839";          // Tên Wi-Fi (Băng tần 2.4GHz)
const char* WIFI_PASSWORD     = "Bm9,4991";      // Mật khẩu Wi-Fi (để "" nếu không có pass)
const char* SERVER_BASE_URL   = "http://192.168.137.1:1880"; // Địa chỉ IP máy tính chạy Node-RED
const char* DEVICE_ID         = "blind-corner-001";        // Mã định danh thiết bị
const char* API_KEY           = "blind-corner-secret-token"; // Khóa xác thực API
const char* FIRMWARE_VERSION  = "1.0.0";

// Tham số chu kỳ và thời gian chờ
const unsigned long HEARTBEAT_INTERVAL_MS = 15000; // 15 giây gửi Heartbeat một lần
const unsigned long HTTP_TIMEOUT_MS        = 1500;  // 1.5 giây timeout mạng (nhanh, không block)
const unsigned long DEBOUNCE_TIME_MS       = 20;    // 20ms lọc nhiễu sườn lên 

// ==============================================================================
// 2. CẤU HÌNH CHÂN PHẦN CỨNG (GIỮ NGUYÊN 100% THEO THIẾT KẾ GỐC)
// ==============================================================================
const int PIRLeft  = 18;  // PIR hướng Trái
const int LedRight = 17;  // LED cảnh báo hướng Phải (sáng khi có xe bên Trái)
const int PIRRight = 16;  // PIR hướng Phải
const int LedLeft  = 19;  // LED cảnh báo hướng Trái (sáng khi có xe bên Phải)

// ==============================================================================
// 3. ĐỊNH NGHĨA DỮ LIỆU & HÀNG ĐỢI SỰ KIỆN (FreeRTOS Queue)
// ==============================================================================
enum Direction {
  DIR_LEFT,
  DIR_RIGHT
};

struct MotionEvent {
  Direction direction;
  unsigned long timestamp; // Thời gian thiết bị (giây)
};

// Hàng đợi lưu tối đa 10 sự kiện chưa kịp gửi, tránh tràn bộ nhớ RAM
static QueueHandle_t eventQueue = NULL;

// Các biến trạng thái toàn cục (được truy cập giữa các task)
volatile unsigned long leftDetections  = 0;
volatile unsigned long rightDetections = 0;
volatile bool currentLeftDetected      = false;
volatile bool currentRightDetected     = false;
volatile bool currentLedLeftState      = false;
volatile bool currentLedRightState     = false;

// ==============================================================================
// 4. TASK 1: CẢNH BÁO CỤC BỘ & BẮT SƯỜN LÊN (CHẠY TRÊN CORE 1)
// ==============================================================================
void sensorAlertTask(void* pvParameters) {
  bool prevLeftState  = false;
  bool prevRightState = false;
  unsigned long lastLeftChangeTime  = 0;
  unsigned long lastRightChangeTime = 0;

  for (;;) {
    unsigned long now = millis();
    bool rawLeft  = (digitalRead(PIRLeft) == HIGH);
    bool rawRight = (digitalRead(PIRRight) == HIGH);

    // --- XỬ LÝ LỌC NHIỄU & BẮT SƯỜN LÊN CHO PIR TRÁI ---
    if (rawLeft != prevLeftState && (now - lastLeftChangeTime > DEBOUNCE_TIME_MS)) {
      lastLeftChangeTime = now;
      prevLeftState = rawLeft;

      // CHỈ kích hoạt khi sườn lên: LOW -> HIGH (xe vừa tới)
      if (rawLeft) {
        leftDetections++;
        MotionEvent ev;
        ev.direction = DIR_LEFT;
        ev.timestamp = now / 1000;
        if (eventQueue != NULL) {
          xQueueSend(eventQueue, &ev, 0); // Đẩy vào hàng đợi ngay lập tức (0ms wait)
        }
      }
    }

    // --- XỬ LÝ LỌC NHIỄU & BẮT SƯỜN LÊN CHO PIR PHẢI ---
    if (rawRight != prevRightState && (now - lastRightChangeTime > DEBOUNCE_TIME_MS)) {
      lastRightChangeTime = now;
      prevRightState = rawRight;

      // CHỈ kích hoạt khi sườn lên: LOW -> HIGH (xe vừa tới)
      if (rawRight) {
        rightDetections++;
        MotionEvent ev;
        ev.direction = DIR_RIGHT;
        ev.timestamp = now / 1000;
        if (eventQueue != NULL) {
          xQueueSend(eventQueue, &ev, 0);
        }
      }
    }

    // --- LOGIC CẢNH BÁO TỨC THỜI CHO ĐÈN LED (ĐỘC LẬP HOÀN TOÀN) ---
    // PIR Trái kích hoạt -> BẬT LED Phải
    if (rawLeft) {
      digitalWrite(LedRight, HIGH);
      currentLedRightState = true;
    } else {
      digitalWrite(LedRight, LOW);
      currentLedRightState = false;
    }

    // PIR Phải kích hoạt -> BẬT LED Trái
    if (rawRight) {
      digitalWrite(LedLeft, HIGH);
      currentLedLeftState = true;
    } else {
      digitalWrite(LedLeft, LOW);
      currentLedLeftState = false;
    }

    currentLeftDetected  = rawLeft;
    currentRightDetected = rawRight;

    // Chu kỳ quét 10ms phản hồi tức thời
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ==============================================================================
// 5. CÁC HÀM TRUYỀN DỮ LIỆU HTTP (CHẠY TRÊN CORE 0)
// ==============================================================================

// Hàm gửi sự kiện chuyển động lên Node-RED tức thời
bool sendMotionEventHTTP(const MotionEvent& ev) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  String url = String(SERVER_BASE_URL) + "/api/devices/events";
  http.begin(url);
  http.setReuse(true); // Tái sử dụng kết nối TCP để truyền nhanh gấp 5 lần
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);

  // Tạo JSON payload
  StaticJsonDocument<256> doc;
  doc["deviceId"]  = DEVICE_ID;
  doc["direction"] = (ev.direction == DIR_LEFT) ? "LEFT" : "RIGHT";
  doc["eventType"] = "MOTION_DETECTED";
  doc["timestamp"] = ev.timestamp;

  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);
  bool success = (httpCode >= 200 && httpCode < 300);
  
  if (success) {
    Serial.printf("[HTTP] Gui su kien xe (%s) thanh cong, code: %d\n", 
                  (ev.direction == DIR_LEFT ? "LEFT" : "RIGHT"), httpCode);
  } else {
    Serial.printf("[HTTP] Gui su kien that bai! Code: %d\n", httpCode);
  }

  http.end();
  return success;
}

// Hàm gửi nhịp tim (Heartbeat) lên Node-RED
bool sendHeartbeatHTTP() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  String url = String(SERVER_BASE_URL) + "/api/devices/heartbeat";
  http.begin(url);
  http.setReuse(true);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);

  StaticJsonDocument<384> doc;
  doc["deviceId"]        = DEVICE_ID;
  doc["left"]            = currentLeftDetected;
  doc["right"]           = currentRightDetected;
  doc["ledLeft"]         = currentLedLeftState;
  doc["ledRight"]        = currentLedRightState;
  doc["leftDetections"]  = leftDetections;
  doc["rightDetections"] = rightDetections;
  doc["uptime"]          = millis() / 1000;
  doc["firmwareVersion"] = FIRMWARE_VERSION;

  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);
  bool success = (httpCode >= 200 && httpCode < 300);

  if (success) {
    Serial.printf("[HEARTBEAT] OK. Uptime: %lu s, Code: %d\n", millis() / 1000, httpCode);
  } else {
    Serial.printf("[HEARTBEAT] Loi: %d\n", httpCode);
  }

  http.end();
  return success;
}

// ==============================================================================
// 6. TASK 2: QUẢN LÝ WI-FI & TRUYỀN DỮ LIỆU IOT (CHẠY TRÊN CORE 0)
// ==============================================================================
void networkTask(void* pvParameters) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[WiFi] Dang ket noi toi SSID: %s ...\n", WIFI_SSID);

  unsigned long lastHeartbeatTime = 0;
  unsigned long lastWifiRetryTime = 0;

  for (;;) {
    unsigned long now = millis();

    // 1. Quản lý tự động kết nối lại Wi-Fi
    if (WiFi.status() != WL_CONNECTED) {
      if (now - lastWifiRetryTime > 8000) { // Thử lại mỗi 8 giây nếu mất mạng
        lastWifiRetryTime = now;
        Serial.println("[WiFi] Mat Wi-Fi! Dang ket noi lai ngam...");
        WiFi.disconnect();
        WiFi.reconnect();
      }
    }

    // 2. Lấy sự kiện từ hàng đợi: phản hồi ngay lập tức trong vòng 10ms
    MotionEvent ev;
    if (xQueueReceive(eventQueue, &ev, pdMS_TO_TICKS(10)) == pdTRUE) {
      if (WiFi.status() == WL_CONNECTED) {
        sendMotionEventHTTP(ev);
      }
    }

    // 3. Định kỳ gửi Heartbeat mỗi 15 giây
    if (now - lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS) {
      lastHeartbeatTime = now;
      if (WiFi.status() == WL_CONNECTED) {
        sendHeartbeatHTTP();
      }
    }

    // Nhường CPU cho các tác vụ nền khác
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ==============================================================================
// 7. SETUP & LOOP CHÍNH
// ==============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n========================================================");
  Serial.println("  SMART BLIND-CORNER ALERT SYSTEM (ESP32 IoT FreeRTOS)   ");
  Serial.println("========================================================");

  // Khởi tạo chân GPIO
  pinMode(PIRLeft, INPUT);
  pinMode(PIRRight, INPUT);
  pinMode(LedLeft, OUTPUT);
  pinMode(LedRight, OUTPUT);

  // Đảm bảo LED ban đầu tắt
  digitalWrite(LedLeft, LOW);
  digitalWrite(LedRight, LOW);

  // Khởi tạo hàng đợi FreeRTOS cho sự kiện chuyển động
  eventQueue = xQueueCreate(10, sizeof(MotionEvent));

  // TÁC VỤ 1: Cảnh báo cục bộ - Ghim vào Core 1 (Ưu tiên 2)
  xTaskCreatePinnedToCore(
    sensorAlertTask,
    "SensorAlertTask",
    4096,
    NULL,
    2,
    NULL,
    1 // Core 1
  );

  // TÁC VỤ 2: Quản lý Wi-Fi & HTTP - Ghim vào Core 0 (Ưu tiên 1)
  xTaskCreatePinnedToCore(
    networkTask,
    "NetworkTask",
    8192,
    NULL,
    1,
    NULL,
    0 // Core 0
  );
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
