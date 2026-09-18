# 💊 Portable AI-Enabled Electrochemical Drug Authenticator

**Cyclic Voltammetry + TinyML for Rapid, Handheld Pharmaceutical Authentication**

*Sep 2025 – Dec 2025 · Narula Institute of Technology (NIT)*

---

## 📌 Overview

An estimated 1 in 10 medical products in low- and middle-income countries is substandard or falsified (WHO). Lab-grade verification — HPLC, Mass Spectrometry — is accurate but expensive, bulky, and slow, leaving a dangerous gap at the "last mile": pharmacies, rural clinics, and patients' homes.

This project is a **portable, battery-operated drug authenticator** that runs a simplified **Cyclic Voltammetry (CV)** scan on a dissolved tablet sample and classifies the resulting electrochemical signature using an on-device (TinyML) machine learning model — giving a PASS/FAKE result in seconds, with no lab, no internet, and no specialist training required.

Unlike simple conductivity meters (which counterfeiters can fool by adding table salt to mimic conductivity), this device looks at the **shape** of the full voltammetric curve — the same feature space a lab instrument would use — rather than a single threshold value.

---

## 🚀 Key Features

- **Cyclic Voltammetry on a $10 microcontroller** — ESP32's internal DAC generates a 0→3.3V→0V voltage sweep across a two-electrode probe; the internal ADC captures the resulting current-response curve (64 points per scan)
- **On-device ML classification** — a lightweight neural network (trained offline, deployed as a plain C header) classifies each scan as Aspirin / Paracetamol / Vitamin C / Fake
- **Standalone OLED UI** — menu-driven interface, immediate high-contrast PASS/FAKE feedback, no companion app required
- **Cloud-assisted data collection** — a separate data-logging firmware mode streams raw scans to Google Sheets over Wi-Fi for building the training dataset
- **Adversarial-aware** — specifically trained to distinguish real drug oxidation curves from ionic salt/sugar solutions used to fake conductivity

---

## 🎯 Objectives

- Design a robust, reusable two-electrode probe for Cyclic Voltammetry on dissolved drug samples
- Use the ESP32's onboard DAC/ADC to generate voltage sweeps and capture current-response curves without external potentiostat circuitry
- Classify samples as Authentic / Fake using an on-device ML model trained on real electrochemical signatures
- Provide immediate PASS/FAIL feedback via an integrated OLED display — no external computer or internet dependency at inference time
- Calibrate and validate against real drugs (Aspirin, Paracetamol, Vitamin C) and common adulterants (salt, sugar, tap water)

---

## 🔬 How It Works

**1. Voltage sweep & acquisition** — the ESP32 drives a DAC pin from 0→255→0 (in steps of 8) across a probe dipped in the dissolved sample, reading the ADC at each step: 32 points on the forward sweep + 32 on the reverse = **64 raw current readings** per scan, plus the peak (max) amplitude as a 65th engineered feature.

**2. Feature signature** — different substances produce different peak heights and curve shapes:

![CV Curves — Paracetamol vs Aspirin vs Vitamin C](images/Fig4_CV_Curves.png)

Peak current differs meaningfully by substance — Paracetamol ≈2717, Aspirin ≈2977, Vitamin C ≈3181 (arbitrary units):

![Peak Current Comparison](images/Fig5_Peak_Current_Comparison.png)

**3. Model training (offline)** — scans are logged to a CSV (`sample_id, timestamp, Label, Max_Amplitude, R1..R64`), then a small neural network (65 → 16 → 8 → 4, ReLU activations) is trained with scikit-learn's `MLPClassifier` on standardized features, and exported as a plain C header (`drug_model.h`) containing the scaler mean/scale and all layer weights — no ML framework needed on-device.

**4. On-device inference** — the ESP32 runs the same 64-point sweep at test time, normalizes it with the exported scaler, forward-passes it through the exported weights, and cross-checks the predicted class against a calibrated peak-current range before declaring PASS or FAKE.

![Annotated Electrochemical Signature](images/Fig6_Electrochemical_Signature.png)

---

## 🧠 Software / ML Pipeline

| Stage | Script | Role |
|---|---|---|
| Data collection | `esp32_code_data_logging.ino` | Runs the CV sweep, uploads each scan (unlabeled) to Google Sheets via a Google Apps Script Web App |
| Cleanup | `data_corrector.py` | Adds `sample_id`/`timestamp`, recomputes `Max_Amplitude`, backfills `Label` from sample names, outputs `Clustering_Fixed_Final.csv` in Edge-Impulse-ready format |
| Training | `build_model.py` | Trains a scikit-learn `MLPClassifier` (65→16→8→4) on the cleaned CSV, exports weights/scaler as a C header |
| Inference | `drug_model.h` + `esp32_code_oled_display_only.ino` | `predict_drug()` runs the exported network on-device; OLED sketch drives the menu, sweep, and PASS/FAKE display |
| (Alt. local logger) | `datalogger.py` | Serial-based logger alternative — reads `DATA_START,...` lines over USB instead of Wi-Fi |
| (Exploration) | `diagram.py` | Standalone plotting script for generating the CV-curve figures from the raw Excel dataset |

> **Note:** at the time of this review, `esp32_code_oled_display_only.ino`'s probe-reading function is a placeholder and still needs the real DAC/ADC sweep wired in to match the training-time acquisition; see the project's open items before treating it as production-ready.

---

## 🎯 Target Analytes

- **Aspirin** (Acetylsalicylic Acid)
- **Paracetamol** (Acetaminophen)
- **Vitamin C** (Ascorbic Acid)
- Tested against common adulterants: table salt, sugar, tap water

---

## 🔧 Hardware

- **ESP32 DevKit** — DAC output for voltage sweep generation, ADC input for current sensing
- **Two-electrode electrochemical probe** (galvanized/stainless steel in the prototype)
- **SSD1306 OLED display** (128×64, I2C) for the standalone UI
- Tactile buttons (Up / Down / Select) for menu navigation
- Status LEDs (green = pass, red = fail)
- 3.7V–11.1V battery power

---

## 🏆 Applications

- Point-of-sale drug verification in pharmacies and rural clinics
- Field screening for health workers and NGOs in low-resource settings
- Patient/consumer-level spot-checking of OTC medication
- Rapid triage tool ahead of formal lab confirmation (HPLC/Mass Spec)

---

## 📈 Future Scope

- Temperature-drift compensation (electrochemical readings can shift ~2%/°C)
- Expanded drug/adulterant library beyond the current 3-drug + fake classes
- On-device model retraining or OTA model updates as more field data is collected
- Migration from probe-based electrodes to inert (platinum/gold) electrodes to reduce oxidation drift over repeated use

## 👨‍💻 Author

**Anirban Saha**
Narula Institute of Technology

**Connect With Me**
- GitHub: [AnirbanWebWorks](https://github.com/AnirbanWebWorks)
- LinkedIn: *(Add your LinkedIn URL here)*
