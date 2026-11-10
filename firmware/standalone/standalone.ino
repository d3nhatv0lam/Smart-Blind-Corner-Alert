/*
 * Smart Blind-Corner Alert System - STANDALONE VERSION (Thuần Offline)
 * 
 * Phiên bản này chỉ thực hiện chức năng cảnh báo an toàn tại chỗ:
 * - Đọc 2 cảm biến PIR AM312
 * - Bật tắt 2 đèn LED cảnh báo chéo góc
 * - KHÔNG sử dụng Wi-Fi, KHÔNG cần Server hay kết nối Internet
 * - Dùng làm bản gốc đối chứng, kiểm tra phần cứng hoặc làm bản dự phòng khi bảo vệ đồ án
 */

// Định nghĩa chân GPIO (Giữ nguyên cấu hình phần cứng gốc)
const int PIRLeft  = 18;  // Cảm biến PIR hướng Trái
const int LedRight = 17;  // Đèn LED cảnh báo hướng Phải
const int PIRRight = 16;  // Cảm biến PIR hướng Phải
const int LedLeft  = 19;  // Đèn LED cảnh báo hướng Trái

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== SMART BLIND-CORNER ALERT - STANDALONE VERSION ===");
  Serial.println("Khoi tao he thong canh bao cuc bo (Khong dung mang)...");

  pinMode(PIRLeft, INPUT);
  pinMode(PIRRight, INPUT);
  pinMode(LedLeft, OUTPUT);
  pinMode(LedRight, OUTPUT);

  // Khởi tạo ban đầu: tắt 2 LED
  digitalWrite(LedLeft, LOW);
  digitalWrite(LedRight, LOW);

  Serial.println("He thong da san sang hoat dong!");
}

void loop() {
  bool left  = digitalRead(PIRLeft);
  bool right = digitalRead(PIRRight);

  // Logic cảnh báo chéo:
  // Xe bên Trái tới -> Sáng đèn Phải để cảnh báo cho xe bên Phải biết
  if (left) {
    digitalWrite(LedRight, HIGH);
  } else {
    digitalWrite(LedRight, LOW);
  }

  // Xe bên Phải tới -> Sáng đèn Trái để cảnh báo cho xe bên Trái biết
  if (right) {
    digitalWrite(LedLeft, HIGH);
  } else {
    digitalWrite(LedLeft, LOW);
  }

  delay(50); 
}
