import serial
import csv
import time
import os
import winsound # Beep sound for Windows (Remove if on Mac/Linux)

# ================= CONFIGURATION =================
SERIAL_PORT = 'COM3'   # <--- CHECK YOUR PORT in Arduino IDE!
BAUD_RATE = 9600       # MATCHING YOUR CODE'S Serial.begin(9600)
FILENAME = "Drug_Data_Log.csv"
# =================================================

def get_serial_connection():
    try:
        # Connect to ESP32
        s = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        print(f"✅ Connected to {SERIAL_PORT} at {BAUD_RATE} baud")
        s.flushInput() # Clear old data
        return s
    except Exception as e:
        print(f"❌ Connection Error: {e}")
        print("   -> Tip: Close the Arduino Serial Monitor!")
        return None

def main():
    ser = get_serial_connection()
    if not ser: return

    # --- INPUT METADATA ---
    print("\n=== DRUG DATA LOGGER ===")
    print("Enter details for the sample you are testing:")
    
    sample_name = input("Sample Name (e.g. Aspirin): ")
    sample_label = input("Label (REAL / FAKE): ").upper()
    
    # Check if file exists to write headers
    file_exists = os.path.isfile(FILENAME)
    
    with open(FILENAME, mode='a', newline='') as file:
        writer = csv.writer(file)
        if not file_exists:
            # Create Headers: Timestamp, Name, Label, and R1...R64 for the sensor values
            headers = ["Timestamp", "Sample_Name", "Label"] + [f"R{i}" for i in range(1, 65)]
            writer.writerow(headers)
            print(f"📄 Created new file: {FILENAME}")

    print(f"\n🚀 LOGGING STARTED for '{sample_name}'.")
    print("👉 Dip the probe. Data will be saved automatically.")
    print("   (Press Ctrl+C to stop)\n")

    sample_count = 0

    try:
        while True:
            # Read a line from the ESP32
            # errors='ignore' prevents crashing if a weird character comes in
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            
            # Look for the identifier from your C++ code
            if line.startswith("DATA_START"):
                # Clean up the data string
                raw_values = line.replace("DATA_START,", "").split(",")
                
                # Check if it's a complete scan (approx 64 data points)
                if len(raw_values) >= 60:
                    sample_count += 1
                    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                    
                    # SAVE TO CSV
                    with open(FILENAME, mode='a', newline='') as file:
                        writer = csv.writer(file)
                        row = [timestamp, sample_name, sample_label] + raw_values
                        writer.writerow(row)
                        
                        # Force write to disk immediately (Real-time save)
                        file.flush()
                        os.fsync(file.fileno())
                    
                    # Feedback to User
                    print(f"💾 Saved Sample #{sample_count} at {timestamp}")
                    
                    # Optional: Beep to confirm save
                    try: winsound.Beep(1000, 200) 
                    except: pass
                    
                else:
                    print("⚠️ Incomplete data packet received (Ignored)")
                    
    except KeyboardInterrupt:
        print(f"\n🛑 Stopped. Total samples collected: {sample_count}")
        ser.close()

if __name__ == "__main__":
    main()