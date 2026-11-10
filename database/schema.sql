-- ==============================================================================
-- Schema Cơ sở dữ liệu SQLite cho Hệ thống Cảnh báo Điểm mù Thông minh
-- Tên file cơ sở dữ liệu: blind_corner.db
-- ==============================================================================

-- 1. Bảng lưu trữ lịch sử sự kiện phát hiện chuyển động
CREATE TABLE IF NOT EXISTS motion_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    direction TEXT NOT NULL CHECK(direction IN ('LEFT', 'RIGHT')),
    event_type TEXT NOT NULL DEFAULT 'MOTION_DETECTED',
    device_timestamp INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Chỉ mục tối ưu tốc độ truy vấn theo thời gian và thiết bị
CREATE INDEX IF NOT EXISTS idx_motion_events_time ON motion_events(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_motion_events_device ON motion_events(device_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_motion_events_direction ON motion_events(direction);

-- 2. Bảng lưu trữ trạng thái hiện tại và sức khỏe của thiết bị (Heartbeat)
CREATE TABLE IF NOT EXISTS device_status (
    device_id TEXT PRIMARY KEY,
    is_online INTEGER DEFAULT 0,            -- 1: Online, 0: Offline
    last_heartbeat DATETIME,                -- Thời điểm nhận heartbeat gần nhất
    left_active INTEGER DEFAULT 0,          -- Trạng thái tức thời PIR Trái
    right_active INTEGER DEFAULT 0,         -- Trạng thái tức thời PIR Phải
    led_left_active INTEGER DEFAULT 0,      -- Trạng thái tức thời LED Trái
    led_right_active INTEGER DEFAULT 0,     -- Trạng thái tức thời LED Phải
    left_detections INTEGER DEFAULT 0,      -- Tổng lượt đếm xe Trái
    right_detections INTEGER DEFAULT 0,     -- Tổng lượt đếm xe Phải
    uptime INTEGER DEFAULT 0,               -- Thời gian hoạt động (giây)
    firmware_version TEXT,                  -- Phiên bản firmware
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Khởi tạo thiết bị mặc định
INSERT OR IGNORE INTO device_status (
    device_id, is_online, last_heartbeat, left_detections, right_detections, uptime, firmware_version
) VALUES (
    'blind-corner-001', 0, NULL, 0, 0, 0, '1.0.0'
);
