#!/usr/bin/env python3
"""
convert_intel_csv.py — Convert Intel Windows thermal dataset to sensor_data.txt
================================================================================
Reads the intel_windows.csv (344 windows × 100 samples each = 34,400 readings)
and flattens them into a simple "temperature, humidity" format that
laptop_sensor_feeder.py and the EK-RA8P1 board can consume directly.

The CSV has:
  - Column 0: window_id (0..343)
  - Columns 1..100: sample_0 .. sample_99 (temperature in °C)
  - Values like 122.153 are sensor-off/fault markers → replaced with previous valid reading

Humidity is not in the original dataset, so we synthesize a correlated value:
  humidity = 40.0 + (temp - 17.0) * 2.5   (clamped 30..95 %RH)
"""

import csv
import sys
import os

INPUT_CSV = r"C:\Users\udaya\Downloads\intel_windows.csv"
OUTPUT_TXT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sensor_data.txt")

# Threshold: values >= 100°C are sensor-fault markers in Intel dataset
FAULT_THRESHOLD = 100.0

def synth_humidity(temp_c):
    """Generate a plausible correlated humidity from temperature."""
    h = 40.0 + (temp_c - 17.0) * 2.5
    return max(30.0, min(95.0, h))

def main():
    if not os.path.exists(INPUT_CSV):
        print(f"[ERROR] Input file not found: {INPUT_CSV}")
        sys.exit(1)

    readings = []
    last_valid_temp = 20.0  # fallback for first fault value

    with open(INPUT_CSV, "r", encoding="utf-8") as f:
        reader = csv.reader(f)
        header = next(reader)  # skip header row

        for row_num, row in enumerate(reader):
            if len(row) < 101:
                continue
            # Columns 1..100 are sample_0 .. sample_99
            for col_idx in range(1, 101):
                try:
                    temp = float(row[col_idx])
                except ValueError:
                    temp = last_valid_temp

                # Replace fault markers
                if temp >= FAULT_THRESHOLD:
                    temp = last_valid_temp
                else:
                    last_valid_temp = temp

                readings.append(temp)

    print(f"[INFO] Extracted {len(readings)} temperature readings from {INPUT_CSV}")

    # Select a representative subset for the board
    # The full 34,400 readings are too many for streaming at 1 Hz (would take 9.5 hours).
    # We'll create three versions:
    #   1. sensor_data.txt: A curated 500-sample excerpt covering interesting phases
    #   2. intel_windows_full.txt: The entire flattened dataset for overnight streaming

    # --- Find interesting phases in the data ---
    # Phase 1: First 200 readings (cold baseline, ~18-19°C)
    # Phase 2: A warming ramp (search for the biggest temperature rise)
    # Phase 3: High-temperature plateau
    # Phase 4: Any anomaly spikes

    # Simple approach: take readings at strategic intervals + key transitions
    curated = []

    # First 150 readings (cold baseline period)
    curated.extend(readings[:150])

    # Find the warming transition (biggest sustained rise)
    # Readings around window 19 show 17.6 → 20.6 transition
    start_warm = 19 * 100  # window 19
    if start_warm + 200 <= len(readings):
        curated.extend(readings[start_warm:start_warm + 200])

    # Mid-range readings around window 50-60 (if different from above)
    mid_start = 50 * 100
    if mid_start + 100 <= len(readings):
        curated.extend(readings[mid_start:mid_start + 100])

    # Late readings showing cooling or different pattern
    late_start = 100 * 100
    if late_start + 100 <= len(readings):
        curated.extend(readings[late_start:late_start + 100])

    # Trim to 600 max for reasonable streaming time (~10 minutes)
    curated = curated[:600]

    # Write curated sensor_data.txt
    with open(OUTPUT_TXT, "w", encoding="utf-8") as f:
        f.write("# ==============================================================================\n")
        f.write("# AIoT-MOR-ALDC : INTEL THERMAL SENSOR DATASET FOR EK-RA8P1\n")
        f.write("# ==============================================================================\n")
        f.write("# Source: intel_windows.csv (Intel CPU/Server thermal monitoring dataset)\n")
        f.write("# Format: <Temperature in Celsius>, <Relative Humidity in %RH>\n")
        f.write("# Humidity is synthesized from temperature correlation\n")
        f.write(f"# Total readings: {len(curated)} samples (streams at ~1 Hz = {len(curated)//60} min)\n")
        f.write("#\n")
        f.write("# Thermal Profile Phases:\n")
        f.write("#   [Phase 1] Cold Baseline (~18-19 °C) — System idle / ambient\n")
        f.write("#   [Phase 2] Warming Transition (~17-21 °C) — Workload startup ramp\n")
        f.write("#   [Phase 3] Mid-Range Operation (~20-22 °C) — Sustained compute load\n")
        f.write("#   [Phase 4] Late Profile (~various) — Dynamic workload variation\n")
        f.write("# ==============================================================================\n\n")

        f.write("# --- PHASE 1: Cold Baseline (First 150 readings from windows 0-1) ---\n")
        for i, temp in enumerate(curated[:150]):
            hum = synth_humidity(temp)
            f.write(f"{temp:.2f}, {hum:.1f}\n")
            if i == 149:
                f.write("\n# --- PHASE 2: Warming Transition (Window 19 — Workload Ramp) ---\n")

        for i, temp in enumerate(curated[150:350]):
            hum = synth_humidity(temp)
            f.write(f"{temp:.2f}, {hum:.1f}\n")
            if i == 199:
                f.write("\n# --- PHASE 3: Mid-Range Operation (Window 50-51) ---\n")

        for i, temp in enumerate(curated[350:450]):
            hum = synth_humidity(temp)
            f.write(f"{temp:.2f}, {hum:.1f}\n")
            if i == 99:
                f.write("\n# --- PHASE 4: Late Dynamic Profile (Window 100-101) ---\n")

        for i, temp in enumerate(curated[450:]):
            hum = synth_humidity(temp)
            f.write(f"{temp:.2f}, {hum:.1f}\n")

    print(f"[OK] Wrote {len(curated)} readings to: {OUTPUT_TXT}")

    # Write full dataset for overnight / extended streaming
    full_path = os.path.join(os.path.dirname(OUTPUT_TXT), "intel_windows_full.txt")
    with open(full_path, "w", encoding="utf-8") as f:
        f.write(f"# AIoT-MOR-ALDC : FULL Intel Thermal Dataset ({len(readings)} readings)\n")
        f.write("# Stream with: python laptop_sensor_feeder.py -f intel_windows_full.txt\n")
        f.write("# At 1 Hz this takes ~9.5 hours to complete\n\n")
        for temp in readings:
            hum = synth_humidity(temp)
            f.write(f"{temp:.2f}, {hum:.1f}\n")

    print(f"[OK] Wrote {len(readings)} readings to: {full_path}")
    print(f"\n[USAGE] To stream to the board:")
    print(f"  python laptop_sensor_feeder.py -f sensor_data.txt          # ~10 min curated subset")
    print(f"  python laptop_sensor_feeder.py -f intel_windows_full.txt   # ~9.5 hr full dataset")

if __name__ == "__main__":
    main()
