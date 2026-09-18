import pandas as pd
import numpy as np
from sklearn.neural_network import MLPClassifier
from sklearn.preprocessing import StandardScaler
import os

# --- 1. CONFIGURATION ---
INPUT_FILE = 'Clustering_Fixed_Final.csv' 
OUTPUT_FILE = 'drug_model.h'

def build():
    print("--- DRUG MODEL BUILDER ---")
    
    # 1. Load Data
    print(f"Loading {INPUT_FILE}...")
    if not os.path.exists(INPUT_FILE):
        print(f"Error: {INPUT_FILE} not found.")
        return

    df = pd.read_csv(INPUT_FILE)

    # 2. Extract Features
    # We take columns R1 to R64
    sensor_cols = [f'R{i+1}' for i in range(64)]
    X = df[sensor_cols].copy()
    
    # Add Max Amplitude (Crucial Feature!)
    # We recalculate it just to be safe
    X['Max_Amplitude'] = X.max(axis=1)

    # 3. Create Labels
    # Your file already has a 'Label' column with: 'Aspirin', 'Paracetamol', 'Fake', 'Vitamin_C'
    print("Mapping Labels...")
    label_map = {
        'Aspirin': 0, 
        'Fake': 1, 
        'Paracetamol': 2, 
        'Vitamin_C': 3
    }
    
    # Map the text labels to numbers (0, 1, 2, 3)
    if 'Label' not in df.columns:
        print("Error: 'Label' column missing from CSV.")
        return
        
    y = df['Label'].map(label_map)
    
    # Check for any unmapped labels
    if y.isnull().any():
        print("Warning: Some labels could not be mapped. Checking unique values:")
        print(df['Label'].unique())
        y = y.fillna(1) # Default to Fake if unknown

    # 4. Train Neural Network
    print("Training AI Model...")
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)

    # Neural Network: 65 inputs -> 16 hidden -> 8 hidden -> 4 outputs
    clf = MLPClassifier(hidden_layer_sizes=(16, 8), max_iter=2000, random_state=42)
    clf.fit(X_scaled, y)
    
    accuracy = clf.score(X_scaled, y)
    print(f"✅ Training Complete. Accuracy: {accuracy:.2%}")

    # 5. Generate C++ Header File
    print(f"Generating {OUTPUT_FILE}...")
    
    # Helper to write arrays
    def to_c(arr, name):
        flat = arr.flatten() if len(arr.shape) > 1 else arr
        return f"const float {name}[{len(flat)}] = {{ " + ", ".join([f"{x:.6f}" for x in flat]) + " };\n"

    # Extract Weights
    w1, b1 = clf.coefs_[0], clf.intercepts_[0]
    w2, b2 = clf.coefs_[1], clf.intercepts_[1]
    w3, b3 = clf.coefs_[2], clf.intercepts_[2]

    content = f"""#ifndef DRUG_MODEL_H
#define DRUG_MODEL_H

// --- AUTO-GENERATED MODEL ---
// Input File: {INPUT_FILE}
// Accuracy: {accuracy:.2%}
// Classes: 0=Aspirin, 1=Fake, 2=Paracetamol, 3=Vitamin_C

{to_c(scaler.mean_, "SCALER_MEAN")}
{to_c(scaler.scale_, "SCALER_SCALE")}
{to_c(w1, "W1")}
{to_c(b1, "B1")}
{to_c(w2, "W2")}
{to_c(b2, "B2")}
{to_c(w3, "W3")}
{to_c(b3, "B3")}

float relu(float x) {{ return (x > 0) ? x : 0; }}

int predict_drug(float* raw_64_sensors) {{
    // 1. Calculate Max Amp
    float max_amp = 0;
    for(int i=0; i<64; i++) if(raw_64_sensors[i] > max_amp) max_amp = raw_64_sensors[i];

    // 2. Normalize
    float input[65];
    for(int i=0; i<64; i++) input[i] = (raw_64_sensors[i] - SCALER_MEAN[i]) / SCALER_SCALE[i];
    input[64] = (max_amp - SCALER_MEAN[64]) / SCALER_SCALE[64];

    // 3. Layer 1
    float l1[16];
    for(int i=0; i<16; i++) {{
        l1[i] = B1[i];
        for(int j=0; j<65; j++) l1[i] += input[j] * W1[j * 16 + i];
        l1[i] = relu(l1[i]);
    }}

    // 4. Layer 2
    float l2[8];
    for(int i=0; i<8; i++) {{
        l2[i] = B2[i];
        for(int j=0; j<16; j++) l2[i] += l1[j] * W2[j * 8 + i];
        l2[i] = relu(l2[i]);
    }}

    // 5. Output
    float output[4];
    int best = 0; float max_v = -999.0;
    for(int i=0; i<4; i++) {{
        output[i] = B3[i];
        for(int j=0; j<8; j++) output[i] += l2[j] * W3[j * 4 + i];
        if(output[i] > max_v) {{ max_v = output[i]; best = i; }}
    }}
    return best;
}}
#endif
"""
    with open(OUTPUT_FILE, 'w') as f:
        f.write(content)
    print("✅ Done! File saved.")

if __name__ == "__main__":
    build()