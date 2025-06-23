import serial
import sqlite3
from datetime import datetime

# === SETTINGS ===
SERIAL_PORT = 'COM3'  # Replace with your Arduino Mega COM port (e.g., COM3, COM4, /dev/ttyUSB0)
BAUD_RATE = 9600
DB_NAME = 'traffic_data.db'

# === CONNECT TO SERIAL PORT ===
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"🔌 Connected to {SERIAL_PORT} at {BAUD_RATE} baud")
except Exception as e:
    print(f"❌ Could not connect to serial port {SERIAL_PORT}. Error: {e}")
    exit(1)

# === CONNECT TO SQLITE DATABASE ===
conn = sqlite3.connect(DB_NAME)
cursor = conn.cursor()

# === CREATE TABLE IF NOT EXISTS ===
cursor.execute('''
CREATE TABLE IF NOT EXISTS traffic_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp_ms INTEGER,
    road TEXT,
    type TEXT,
    value REAL,
    status TEXT,
    log_time TEXT
)
''')
conn.commit()

print("📊 Logging started... Press Ctrl+C to stop.\n")

# === MAIN LOOP: READ FROM SERIAL AND STORE ===
try:
    while True:
        line = ser.readline().decode().strip()
        if line.startswith("LOG"):
            parts = line.split(",")
            if len(parts) >= 5:
                timestamp_ms = int(parts[1])
                road = parts[2]
                data_type = parts[3]
                value = float(parts[4])
                status = parts[5] if data_type == "SPEED" else None
                log_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

                # Insert into database
                cursor.execute('''
                    INSERT INTO traffic_log (timestamp_ms, road, type, value, status, log_time)
                    VALUES (?, ?, ?, ?, ?, ?)
                ''', (timestamp_ms, road, data_type, value, status, log_time))
                conn.commit()

                print(f"📥 Logged: {road} | {data_type} | {value} | {status}")
except KeyboardInterrupt:
    print("\n🛑 Logging stopped by user.")
except Exception as e:
    print(f"❗ Error occurred: {e}")
finally:
    conn.close()
    ser.close()
    print("✅ Serial and DB connections closed.")
