# AIoT-MOR-ALDC

**Artificial Intelligence of Things — Multi-Order Residual Adaptive Lossless Data Compression**

An Edge AI-powered IoT sensor telemetry platform running on the **Renesas EK-RA8P1** (ARM Cortex-M85 @ 480 MHz). Performs on-device signal classification, anomaly detection, and adaptive lossless compression — reducing wireless transmission energy by **53%+** while maintaining 100% data fidelity.

---

## Key Features

| Feature | Description |
|---|---|
| **Edge AI Inference** | Random Forest classifier + Isolation Forest anomaly detector running in ~3 ms on Cortex-M85 |
| **Lossless Compression** | Multi-Order Residual + Adaptive RLE achieves **2.14× compression ratio** (400 → 187 bytes) |
| **Dual Sensor Modes** | Live DHT11 hardware sensor **or** laptop-streamed datasets via USB — switchable at runtime |
| **480×854 LVGL Dashboard** | Real-time temperature, humidity, AI classification, anomaly alerts, and compression stats |
| **Interactive Boot Selector** | 20-second countdown modal with physical button (S1/S2) and console selection |

## System Architecture

```
  DHT11 Sensor / Laptop USB
            │
            ▼
  ┌─────────────────────┐
  │  Sensor Subsystem    │  100-sample sliding window @ 1 Hz
  └─────────┬───────────┘
            ▼
  ┌─────────────────────┐
  │  Edge AI Pipeline    │  8 statistical features → RF Classifier + Isolation Forest
  └─────────┬───────────┘
            ▼
  ┌─────────────────────┐
  │  MOR-ALDC Codec      │  Residual transform → Fixed-point quantization → Adaptive RLE
  └─────────┬───────────┘
            ▼
  ┌─────────────────────┐
  │  480×854 LVGL UI     │  Real-time dashboard + SEGGER RTT / TCP telemetry output
  └─────────────────────┘
```

## Hardware Requirements

- **Board:** Renesas EK-RA8P1 Evaluation Kit (R7FA8P1PFEAMD)
- **Display:** 4.3-inch MIPI-DSI TFT LCD (480 × 854 portrait)
- **Sensor (optional):** DHT11 on GPIO P410 (J18 Pin 2) with 4.7kΩ pull-up
- **Debug Cable:** Micro-USB to J10 (SEGGER J-Link OB)

## Software Requirements

- Renesas e² studio (v2024-01+)
- Renesas FSP v6.5.0+
- GNU Arm Embedded Toolchain (arm-none-eabi-gcc v13.2+)
- Python 3.8+ with `pyserial` (for laptop data streaming)

## Quick Start

1. **Clone & Open:**
   ```bash
   git clone https://github.com/<your-username>/AIoT-MOR-ALDC.git
   ```
   Open in e² studio → Import Existing Projects into Workspace.

2. **Build:** Right-click project → Build Project (`Ctrl+B`).

3. **Flash:** Connect USB to J10 → Debug as → `AI_MOR_ALDC1 Debug_Flat.launch` → Resume (F8).

4. **Select Mode:** On the 20-second boot screen, press **S1** (Live Sensor) or **S2** (Data-Driven). Default: Data-Driven with embedded benchmark dataset.

5. **Stream Laptop Data (optional):**
   ```bash
   python laptop_sensor_feeder.py -f sensor_data.txt
   ```

## Project Structure

```
├── src/
│   ├── hal_entry.c                 # Main application entry, boot sequence, main loop
│   ├── ai/
│   │   ├── ai_pipeline.c/h         # 8-feature extraction, RF classifier, IF anomaly, DT codec
│   │   └── models/                  # Embedded ML model headers (emlearn export)
│   ├── compression/
│   │   └── residual_compress.c/h    # MOR-ALDC residual + RLE lossless codec
│   ├── display/
│   │   ├── display_ui.c/h           # LVGL 9 dashboard layout (4 cards + mode selector)
│   │   └── lv_port_display.c/h      # GLCDC + MIPI-DSI display driver port
│   ├── sensors/
│   │   ├── sensor_source.c/h        # Unified dual-mode sensor abstraction
│   │   └── dht11_driver.c/h         # DHT11 one-wire GPIO driver
│   └── network/
│       └── network_client.c/h       # SEGGER RTT + UART telemetry output
├── laptop_sensor_feeder.py          # PC-side data streamer (RTT / Serial)
├── laptop_receiver.py               # PC-side live telemetry receiver
├── sensor_data.txt                  # Editable dataset (6 thermal test phases)
├── feed_data.cmd                    # Windows double-click launcher
└── AI_MOR_ALDC_MASTER_DOCUMENTATION.md  # Full technical reference
```

## Documentation

See [AI_MOR_ALDC_MASTER_DOCUMENTATION.md](AI_MOR_ALDC_MASTER_DOCUMENTATION.md) for the complete technical specification covering hardware pinout, RTOS architecture, AI algorithms, compression mathematics, UI layout, and troubleshooting.

## License

MIT License — See [LICENSE](LICENSE) for details.
