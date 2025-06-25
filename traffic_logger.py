import serial
import sqlite3
from datetime import datetime

# === SETTINGS ===
SERIAL_PORT = 'COM3'  # Replace with your Arduino Mega COM port
BAUD_RATE = 9600
DB_NAME = 'traffic_data.db'

# === CONNECT TO SERIAL PORT ===
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud")
except Exception as e:
    print(f"Could not connect to serial port {SERIAL_PORT}. Error: {e}")
    exit(1)

# === CONNECT TO SQLITE DATABASE ===
conn = sqlite3.connect(DB_NAME)
cursor = conn.cursor()

# === CREATE TABLES ===

# Table for individual vehicle speed logs
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

# Table for junction count snapshot (after each full traffic cycle)
cursor.execute('''
CREATE TABLE IF NOT EXISTS junction_counts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    A INTEGER,
    B INTEGER,
    C INTEGER,
    D INTEGER,
    log_time TEXT
)
''')

conn.commit()
print("Logging started... Press Ctrl+C to stop.\n")

# === Helper Function to Extract Counts from Summary Line ===
def extract_counts(summary_line):
    try:
        parts = summary_line.split('|')
        counts = []
        for part in parts:
            if "Road" in part:
                count = int(part.strip().split(':')[1].strip())
                counts.append(count)
        if len(counts) == 4:
            return counts
        else:
            return None
    except Exception as e:
        print(f"Failed to parse summary: {e}")
        return None

# === MAIN LOOP: READ FROM SERIAL AND STORE ===
try:
    while True:
        line = ser.readline().decode().strip()

        if line.startswith("LOG"):
            parts = line.split(",")

            # Individual vehicle speed logs
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

        elif "Vehicle Count Summary" in line:
            # Read the next line for actual count data
            summary_line = ser.readline().decode().strip()
            counts = extract_counts(summary_line)
            if counts:
                log_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                cursor.execute('''
                    INSERT INTO junction_counts (A, B, C, D, log_time)
                    VALUES (?, ?, ?, ?, ?)
                ''', (*counts, log_time))
                conn.commit()
                print(f"Junction Snapshot: A={counts[0]} B={counts[1]} C={counts[2]} D={counts[3]}")
except KeyboardInterrupt:
    print("\nLogging stopped by user.")
except Exception as e:
    print(f"Error occurred: {e}")
finally:
    conn.close()
    ser.close()
    print("Serial and DB connections closed.")
