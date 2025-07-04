import serial
import sqlite3
from datetime import datetime

# === SETTINGS ===
SERIAL_PORT = 'COM3'
BAUD_RATE = 9600
DB_NAME = 'traffic_data.db'

# === CONNECT TO SERIAL PORT ===
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud")
except Exception as e:
    print(f"Could not connect to serial port: {e}")
    exit(1)

# === CONNECT TO SQLITE DATABASE ===
conn = sqlite3.connect(DB_NAME)
cursor = conn.cursor()

# === CREATE TABLES ===
cursor.execute('''
CREATE TABLE IF NOT EXISTS vehicle_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp_ms INTEGER,
    road TEXT,
    speed REAL,
    status TEXT,
    log_time TEXT
)
''')

cursor.execute('''
CREATE TABLE IF NOT EXISTS junction_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    road TEXT,
    count INTEGER,
    log_time TEXT
)
''')

conn.commit()
print("Logging started... Press Ctrl+C to stop.\n")

# === MAIN LOOP ===
try:
    while True:
        line = ser.readline().decode().strip()

        if line.startswith("LOG"):
            parts = line.split(",")

            if len(parts) == 6 and parts[3] == "SPEED":
                timestamp_ms = int(parts[1])
                road = parts[2]
                speed = float(parts[4])
                status = parts[5]
                log_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

                cursor.execute('''
                    INSERT INTO vehicle_log (timestamp_ms, road, speed, status, log_time)
                    VALUES (?, ?, ?, ?, ?)
                ''', (timestamp_ms, road, speed, status, log_time))
                conn.commit()
                print(f"SPEED Logged: {road} | {speed:.2f} km/h | {status}")

            elif len(parts) == 5 and parts[3] == "COUNT":
                road = parts[2]
                count = int(parts[4])
                log_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

                cursor.execute('''
                    INSERT INTO junction_log (road, count, log_time)
                    VALUES (?, ?, ?)
                ''', (road, count, log_time))
                conn.commit()
                print(f"COUNT Logged: Road {road} | Vehicles: {count}")

except KeyboardInterrupt:
    print("\nLogging stopped by user.")
except Exception as e:
    print(f"Error occurred: {e}")
finally:
    conn.close()
    ser.close()
    print("Serial and DB connections closed.")
