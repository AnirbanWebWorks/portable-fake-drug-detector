import pandas as pd
import os
import glob

# ================= CONFIGURATION =================
OUTPUT_FILE = 'Clustering_Fixed_Final.csv'
# =================================================

def fix_file():
    print("Status: Searching for your data file...")
    
    # Find the CSV or XLSX file automatically
    files = glob.glob("*.csv") + glob.glob("*.xlsx")
    files = [f for f in files if "Clustering_Fixed" not in f and "fix_timestamp" not in f]
    
    if not files:
        print("❌ Error: No data file found. Please put this script in the same folder as your data.")
        return

    input_file = files[0] # Take the first one found
    print(f"👉 Processing: {input_file}")

    # Read the file
    try:
        if input_file.endswith('.csv'):
            df = pd.read_csv(input_file)
        else:
            df = pd.read_excel(input_file)
    except Exception as e:
        print(f"❌ Error reading file: {e}")
        return

    # Check for Sensor Columns (R1-R64)
    sensor_cols = [f'R{i+1}' for i in range(64)]
    if not all(col in df.columns for col in sensor_cols):
        print("❌ Error: Columns R1-R64 are missing.")
        return

    # ---------------------------------------------------------
    # THE FIX: ADD SAMPLE_ID AND TIMESTAMP
    # ---------------------------------------------------------
    print("Status: Adding 'sample_id' and 'timestamp' columns...")

    # 1. Add sample_id (1, 2, 3... 1404)
    # This tells Edge Impulse that every row is a DIFFERENT sample.
    df['sample_id'] = range(1, len(df) + 1)

    # 2. Add timestamp (0)
    # Since these are flattened snapshots, everything happens at time 0.
    df['timestamp'] = 0

    # 3. Recalculate Max Amplitude (Just to be safe)
    df['Max_Amplitude'] = df[sensor_cols].max(axis=1)

    # 4. Ensure Label exists
    if 'Label' not in df.columns:
        # Try to regenerate label from Sample_Name if Label is missing
        name_col = 'Sample_Name' if 'Sample_Name' in df.columns else 'sample label'
        if name_col in df.columns:
            def get_label(name):
                name = str(name).lower()
                if 'fake' in name or 'salt' in name: return 'Fake'
                if 'tap water' in name and 'aspirin' not in name: return 'Fake'
                if 'aspirin' in name: return 'Aspirin'
                if 'paracetamol' in name: return 'Paracetamol'
                if 'vitamin' in name or 'vit c' in name: return 'Vitamin_C'
                return 'Fake'
            df['Label'] = df[name_col].apply(get_label)
        else:
            print("⚠️ Warning: No 'Label' or 'Sample_Name' column found.")

    # 5. Organize Columns (Edge Impulse Format)
    # Format: sample_id, timestamp, Label, ... features ...
    cols = ['sample_id', 'timestamp', 'Label', 'Max_Amplitude'] + sensor_cols
    df_final = df[cols]

    # Save
    df_final.to_csv(OUTPUT_FILE, index=False)
    print(f"✅ SUCCESS! Created: {OUTPUT_FILE}")
    print("   Upload this file to Edge Impulse.")
    print("   It will now recognize 1404 separate samples.")

if __name__ == "__main__":
    fix_file()