# AIoT-MOR-ALDC : Complete System Architecture, Edge AI, Compression & Operational Specification
**Project Name:** AIoT-MOR-ALDC (Artificial Intelligence Multi-Order Residual Adaptive Lossless Data Compression)  
**Target Hardware:** Renesas EK-RA8P1 Evaluation Kit (`R7FA8P1PFEAMD`)  
**Core Architecture:** ARM Cortex-M85 with Helium MVE @ 480 MHz  
**Display Hardware:** 4.3-inch Portrait TFT LCD (480 × 854) via 2-Lane MIPI-DSI & GLCDC  
**GUI Framework:** LVGL v9.x (Optimized Pure Software Engine `lv_draw_sw`)  
**Operating System:** FreeRTOS Kernel on Renesas Flexible Software Package (FSP v6.5+)  
**Document Revision:** 2.0 (Master Release)  
**Date:** September 2026  

---

## Table of Contents
1. [Executive Summary & System Objectives](#1-executive-summary--system-objectives)
2. [Hardware Architecture & Pinout Map](#2-hardware-architecture--pinout-map)
3. [Software & RTOS Pipeline Architecture](#3-software--rtos-pipeline-architecture)
4. [Sensor Acquisition Engine & Dual-Mode Mechanism](#4-sensor-acquisition-engine--dual-mode-mechanism)
5. [Edge AI Inference Engine](#5-edge-ai-inference-engine)
6. [MOR-ALDC Lossless Compression Engine](#6-mor-aldc-lossless-compression-engine)
7. [Display UI Architecture & Graphical Engine](#7-display-ui-architecture--graphical-engine)
8. [Data Ingestion & Laptop Streaming Guide](#8-data-ingestion--laptop-streaming-guide)
9. [Build, Flash, & Debug Instructions](#9-build-flash--debug-instructions)
10. [Troubleshooting & Verification Matrix](#10-troubleshooting--verification-matrix)

---

## 1. Executive Summary & System Objectives

The **AIoT-MOR-ALDC** system is a research-grade Edge AI and IoT telemetry platform developed for the Renesas EK-RA8P1 microcontroller. Modern industrial IoT sensor nodes face severe trade-offs between continuous real-time monitoring, RF communication energy expenditure, and memory limitations. Transmitting raw floating-point data over wireless links (Wi-Fi, Bluetooth, LoRa, or Cellular) consumes up to **80%** of an edge device's power budget.

The AIoT-MOR-ALDC system solves this dilemma by performing:
1. **Real-Time Environmental Acquisition:** Continuously collects temperature and relative humidity at 1 Hz from physical sensors or host datasets.
2. **On-Device Feature Extraction:** Derives an 8-dimensional statistical feature vector across a 100-sample sliding window.
3. **Edge AI Classification & Anomaly Detection:** Utilizes embedded machine learning models (Random Forest classifier and Isolation Forest surrogate decision tree) to classify temporal thermal patterns (`SMOOTH`, `PERIODIC`, `STEP`, `NOISY`, `RAMP`) and isolate thermal runaway events ($>40^\circ\text{C}$).
4. **Adaptive Lossless Data Compression (MOR-ALDC):** Recommends and applies an optimal multi-order residual transformation combined with adaptive Run-Length Encoding (RLE) to achieve a **2.14× compression ratio** (reducing 400-byte raw float windows down to ~187 bytes), saving **53.3%** of transmission energy.
5. **Interactive 480 × 854 Portrait Dashboard:** Displays live telemetry, thermal drift rates, change histories, AI inferences, and compression metrics on a high-resolution display with zero graphical artifacts.
6. **Dual Operational Modes:** Allows instant switching between physical DHT11 hardware sensing and USB data-driven streaming from a host laptop.

```
                      +------------------------------------------------+
                      |         AIoT-MOR-ALDC SYSTEM TOPOLOGY            |
                      +------------------------------------------------+
                                              |
             +--------------------------------+--------------------------------+
             |                                                                 |
   [MODE 1: LIVE SENSOR]                                             [MODE 2: DATA-DRIVEN]
   DHT11 Sensor on P410                                              Laptop via USB Debug Cable
   One-Wire Microsecond Protocol                                     RTT / COM Port Streamer
             |                                                                 |
             +--------------------------------+--------------------------------+
                                              |
                                              v
                              +-------------------------------+
                              |    Unified Sensor Subsystem   |
                              |     (100-Sample Ring Buffer)  |
                              +-------------------------------+
                                              |
                                              v
                              +-------------------------------+
                              |       Edge AI Pipeline        |
                              |  - 8 Statistical Features     |
                              |  - Random Forest Classifier   |
                              |  - Isolation Forest Detector  |
                              |  - Decision Tree Selector     |
                              +-------------------------------+
                                              |
                                              v
                              +-------------------------------+
                              |    MOR-ALDC Lossless Codec    |
                              |  - Multi-Order Residuals      |
                              |  - Fixed-Point Quantization   |
                              |  - Adaptive Run-Length (RLE)  |
                              +-------------------------------+
                                              |
                      +-----------------------+-----------------------+
                      |                                               |
                      v                                               v
       +------------------------------+               +-------------------------------+
       |   480 x 854 Portrait LVGL    |               |    Telemetry Data Output      |
       |      Display Dashboard       |               |  - SEGGER RTT JSON Telemetry  |
       |  - Dual-Mode Selector Boxes  |               |  - Ethernet TCP (192.168.x.x) |
       |  - Real-Time Thermal Cards   |               +-------------------------------+
       |  - AI & Compression Stats    |
       +------------------------------+
```

---

## 2. Hardware Architecture & Pinout Map

### 2.1 Microcontroller Core: Renesas RA8P1
* **Device Part Number:** `R7FA8P1PFEAMD`
* **CPU Core:** ARM Cortex-M85 with Helium (M-Profile Vector Extension)
* **Maximum Clock Frequency:** 480 MHz
* **On-Chip Memory:** 2 MB Dual-Bank Flash, 1 MB Internal SRAM
* **External Memory:** 32 MB External SDRAM mapped at `0x68000000` (used for dual display framebuffers)
* **Graphics Controller:** Renesas GLCDC (Graphics LCD Controller) integrated with 2-Lane MIPI-DSI Controller

### 2.2 Board Pinout & Peripheral Assignment
| Signal / Function | RA8P1 Pin | Connector / Hardware Location | Configuration | Logic Description |
| :--- | :--- | :--- | :--- | :--- |
| **Push Button S1** | `P009` | Board Button S1 (`USER_SW1`) | Input, Pull-up enabled | Active LOW. Selects **Live Sensor Mode** |
| **Push Button S2** | `P008` | Board Button S2 (`USER_SW2`) | Input, Pull-up enabled | Active LOW. Selects **Data-Driven Mode** |
| **User Push Button** | `P000` | Board Button S3 (`USER_SW_CFG_INT`) | Input, Pull-up enabled | Active LOW. Toggles active mode dynamically |
| **DHT11 Sensor Data**| `P410` | Expansion Header J18, Pin 2 | Bidirectional Open-Drain | One-wire timing bus with 4.7kΩ pull-up |
| **Display Backlight**| `P514` | MIPI-DSI Interface (`PIN_DISPLAY_BACKLIGHT`)| Output, Push-Pull | Active HIGH (Turns on LED backlight) |
| **Display Reset** | `P606` | MIPI-DSI Interface (`PIN_DISPLAY_RST`)| Output, Push-Pull | Active LOW pulse resets display driver IC |
| **MIPI DSI Lane 0/1**| Hardwired | DSI Physical Interface | High-Speed Differential | 2-Lane MIPI DSI Video Mode |
| **Debug / Telemetry**| Micro-USB | Connector J10 (SEGGER J-Link OB) | SWD / VCOM / RTT | Debugging, RTT Buffer 0, COM port |

---

## 3. Software & RTOS Pipeline Architecture

### 3.1 FreeRTOS Task Configuration
The system operates on FreeRTOS with preemption enabled and a tick rate of 1000 Hz (`configTICK_RATE_HZ = 1000`).

```
+-----------------------------------------------------------------------------------------------+
| Task Name      | Priority | Stack Size | Periodicity | Responsibilities                       |
+----------------+----------+------------+-------------+----------------------------------------+
| sensor_task    | High (4) | 1024 Bytes | 1000 ms     | Sample DHT11/USB, maintain window      |
| ai_task        | Norm (3) | 4096 Bytes | Per Window  | Feature extraction, RF, IF, Codec sel  |
| display_task   | Low  (2) | 8192 Bytes | 20 ms       | LVGL timer handler, screen rendering   |
| network_task   | Low  (2) | 4096 Bytes | Per Result  | JSON formatting over RTT / TCP socket  |
+-----------------------------------------------------------------------------------------------+
```

### 3.2 Inter-Task Communication and Synchronization
* **Sensor Queue (`g_sensor_queue`):** Carries full 100-sample float arrays (`sensor_window_t`) from `sensor_task` to `ai_task`.
* **Result Queue (`g_result_queue`):** Broadcasts calculated results (`result_message_t`) from `ai_task` to `display_task` and `network_task`.
* **VSYNC Semaphore (`g_vsync_sem`):** Synchronizes LVGL buffer swapping with the GLCDC line detection interrupt to eliminate screen tearing.

### 3.3 Main Loop Execution Sequence in `hal_entry.c`
1. **Hardware Initialization:**
   * RTC initialized via `R_RTC_Open(&g_rtc0_ctrl, &g_rtc0_cfg)` and seeded with compile timestamp.
   * Display controller initialized via `lv_init()` and `lv_port_display_init()`.
   * Push buttons configured as inputs with internal pull-up resistors (`P009`, `P008`, `P000`).
   * Edge AI pipeline initialized (`ai_pipeline_init()`).
   * LVGL UI layout created (`display_ui_init()`).
2. **Interactive Startup Mode Selection Modal:**
   * Launches a **20-second countdown** interactive modal on the TFT screen.
   * Polls S1 (`P009`), S2 (`P008`), USER_SW (`P000`), and console inputs.
   * If the user presses S1, Live DHT11 mode is engaged.
   * If the user presses S2 or no button is pressed before timeout, Data-Driven mode is selected by default.
   * Displays confirmation text (`>>> MODE CONFIRMED! LAUNCHING OS <<<`) for 700 ms before dismissing the modal.
3. **Continuous Execution Loop:**
   * **Button Polling:** Continually scans S1, S2, and USER_SW for instant runtime mode toggling.
   * **Sensor Ingestion (1 Hz):** Calls `sensor_source_read()`. If data-driven mode is active, ingests laptop data via RTT buffer 0 or steps through the embedded benchmark dataset.
   * **Sliding Window Update:** Shifts the 100-element FIFO buffer and appends the latest reading.
   * **Edge AI Execution:** Executes `ai_pipeline_run()` in ~3 ms.
   * **MOR-ALDC Compression:** If recommended by the AI model and the data is not anomalous, executes `compress_signal()`.
   * **UI Refresh:** Updates all screen labels, trend badges, drift rates, and elapsed timers.
   * **Network Telemetry:** Packs a `net_packet_t` and broadcasts JSON data over RTT and TCP.
   * **LVGL Rendering:** Executes `lv_timer_handler()` and yields 10 ms for fluid rendering.

---

## 4. Sensor Acquisition Engine & Dual-Mode Mechanism

The sensor subsystem (`src/sensors/sensor_source.c` and `sensor_source.h`) unifies multiple data sources behind a single abstraction:

```c
typedef struct {
    float temperature_c;
    float humidity_pct;
    bool  is_injected;    /* True if received from laptop USB */
    bool  is_simulated;   /* True if generated from embedded benchmark */
    bool  is_sensor;      /* True if read from physical DHT11 */
} sensor_reading_t;
```

### 4.1 Mode 1: Physical DHT11 Sensor (`SENSOR_MODE_LIVE_DHT11`)
* **Hardware Wiring:** Connect DHT11 signal pin to `P410` (Expansion Header J18 Pin 2). Provide 3.3V power and GND. A 4.7kΩ pull-up resistor to 3.3V is required on the data line.
* **Protocol Timing:**
  1. **Host Start Signal:** MCU pulls `P410` LOW for at least 18 ms, then brings it HIGH for 20–40 µs.
  2. **Sensor Response:** DHT11 pulls LOW for 80 µs, followed by HIGH for 80 µs.
  3. **Data Transmission (40 bits):**
     * Bit '0': 50 µs LOW + 26–28 µs HIGH.
     * Bit '1': 50 µs LOW + 70 µs HIGH.
  4. **Payload Bytes:** Byte 0 = Integral RH, Byte 1 = Decimal RH, Byte 2 = Integral Temp, Byte 3 = Decimal Temp, Byte 4 = Checksum (`Byte 0 + Byte 1 + Byte 2 + Byte 3`).
* **Fault Tolerance:** If no physical sensor is attached or if checksum verification fails, the driver transitions to simulated ambient drift ($24.5^\circ\text{C} \pm 0.3^\circ\text{C}$) to prevent system lockups.

### 4.2 Mode 2: Data-Driven Streaming (`SENSOR_MODE_DATA_DRIVEN`)
This mode allows custom data to be injected from a host computer or from the on-chip benchmark dataset:
1. **SEGGER RTT Input Stream:**
   * The board listens to Down-Buffer 0 in RAM (`_SEGGER_RTT.aDown[0]`).
   * When `laptop_sensor_feeder.py` sends readings, the board's embedded parser extracts temperature and humidity.
   * Accepted formats:
     * Standard comma-separated: `28.50, 62.0`
     * Tagged syntax: `T:28.50, H:62.0`
     * Single temperature value: `28.50` (humidity defaults to 55.0% RH)
2. **On-Chip Benchmark Dataset Fallback:**
   * If the laptop streamer is paused or disconnected, the board automatically cycles through `s_benchmark_dataset[]` (53 curated readings spanning 5 phases):
     * **Phase 1 (Baseline Ambient):** $24.20^\circ\text{C} \rightarrow 25.00^\circ\text{C}$ (Equilibrium, maximum compression).
     * **Phase 2 (Workload Heating):** $25.45^\circ\text{C} \rightarrow 33.50^\circ\text{C}$ (Dynamic thermal ramp).
     * **Phase 3 (High-Load Steady State):** $33.80^\circ\text{C} \rightarrow 34.30^\circ\text{C}$ (Continuous high compute).
     * **Phase 4 (Thermal Runaway Anomaly):** $37.20^\circ\text{C} \rightarrow 49.80^\circ\text{C}$ (Exceeds $40^\circ\text{C}$, triggers Isolation Forest alarm and turns display RED).
     * **Phase 5 (Cooling Recovery):** $43.00^\circ\text{C} \rightarrow 24.50^\circ\text{C}$ (Cooling back to normal baseline).

---

## 5. Edge AI Inference Engine

The Edge AI subsystem (`src/ai/ai_pipeline.c`) processes sensor windows without any external cloud or coprocessor dependencies.

### 5.1 Signal Normalization & 8-Feature Extraction
Each 100-sample sliding window $X = \{x_0, x_1, \dots, x_{N-1}\}$ ($N=100$) is normalized to the $[0, 1]$ interval:
$$x_{\text{norm}, i} = \frac{x_i - \min(X)}{\max(X) - \min(X)}$$

From this normalized window, 8 statistical features are calculated:
1. **Mean ($\mu$):**
   $$\mu = \frac{1}{N} \sum_{i=0}^{N-1} x_i$$
2. **Standard Deviation ($\sigma$):**
   $$\sigma = \sqrt{\frac{1}{N} \sum_{i=0}^{N-1} (x_i - \mu)^2}$$
3. **Minimum Value ($x_{\min}$):**
   $$x_{\min} = \min_{i} (x_i)$$
4. **Maximum Value ($x_{\max}$):**
   $$x_{\max} = \max_{i} (x_i)$$
5. **25th Percentile ($P_{25}$):**
   The value below which 25% of the sorted window readings fall.
6. **75th Percentile ($P_{75}$):**
   The value below which 75% of the sorted window readings fall.
7. **Mean of First Differences ($\Delta\mu$):**
   $$\Delta\mu = \frac{1}{N-1} \sum_{i=0}^{N-2} (x_{i+1} - x_i)$$
8. **Standard Deviation of First Differences ($\Delta\sigma$):**
   $$\Delta\sigma = \sqrt{\frac{1}{N-1} \sum_{i=0}^{N-2} ((x_{i+1} - x_i) - \Delta\mu)^2}$$

### 5.2 Feature Scaling
The features are scaled using Min-Max parameters established during offline model training (`src/ai/models/scaler_params.h`):
$$f_{\text{scaled}, j} = \frac{\text{feat}_j - \text{FEATURE\_MIN}_j}{\text{FEATURE\_MAX}_j - \text{FEATURE\_MIN}_j}, \quad j \in [0, 7]$$

### 5.3 Embedded Machine Learning Models
All models are converted to static C arrays via `emlearn` and compiled into Flash memory:
1. **Pattern Classifier (`classifier_model.h`):**
   * **Architecture:** Random Forest Ensemble (10–20 decision trees with majority voting).
   * **Output Classes:**
     * `0`: `SMOOTH` (Normal stable operating state)
     * `1`: `PERIODIC` (Cyclic HVAC or machine cycling)
     * `2`: `STEP` (Sudden environmental transition)
     * `3`: `NOISY` (Sensor vibration, electrical interference)
     * `4`: `RAMP` (Continuous heating or cooling load)
2. **Anomaly Detector (`anomaly_model.h`):**
   * **Architecture:** Isolation Forest surrogate Decision Tree.
   * **Decision Rule:** Identifies outliers based on path length across feature space.
   * **Output:** `0 = NORMAL`, `1 = ANOMALY_DETECTED`.
   * **System Behavior:** Triggers visual alarms (red UI text) when temperature spikes exceed $40^\circ\text{C}$ or undergo erratic divergence.
3. **Compression Mode Decision Tree (`compression_model.h`):**
   * **Architecture:** Compact binary decision tree.
   * **Output:** `1 = COMPRESS`, `0 = RAW`.
   * **Safety Rule:** Anomalous data is **never compressed**. If `is_anomaly == true`, compression is automatically bypassed to preserve 100% of the raw waveform for post-incident diagnostics.

---

## 6. MOR-ALDC Lossless Compression Engine

The Multi-Order Residual Adaptive Lossless Data Compression algorithm (`src/compression/residual_compress.c`) minimizes entropy before bitstream encoding.

### 6.1 Mathematical Formulation
Sensor data exhibits strong temporal autocorrelation. The MOR-ALDC engine transforms raw IEEE-754 floating-point values into compact integer residuals.

1. **Fixed-Point Quantization:**
   To eliminate floating-point representation overhead without loss of precision, values are scaled by 100 (preserving $0.01^\circ\text{C}$ sensor resolution):
   $$q_i = \text{round}(x_i \times 100), \quad q_i \in \mathbb{Z}$$

2. **Residual Differentiation:**
   The anchor sample is stored directly:
   $$r_0 = q_0$$
   Successive samples are converted into first-order residuals (differences):
   $$r_i = q_i - q_{i-1}, \quad i \in [1, N-1]$$
   In stable environmental conditions, $r_i$ clusters tightly around $0$ (e.g., $0, 0, +1, 0, -1, 0$).

3. **Adaptive Run-Length Encoding (RLE):**
   Consecutive identical residuals are encoded into a compact 3-byte tuple:
   $$\text{Tuple} = [\text{Run Length (1 Byte)}, \text{Value Low (1 Byte)}, \text{Value High (1 Byte)}]$$
   * Maximum run length per tuple: 255.
   * If a steady signal produces 50 identical readings, standard storage requires $50 \times 4 = 200$ bytes. RLE packs this into a single **3-byte tuple**, yielding a **66.6× localized reduction**.

### 6.2 Performance Metrics
* **Raw Window Payload:** $100 \times 4\text{ bytes} = 400\text{ bytes}$
* **Typical Compressed Payload:** ~187 bytes (under normal operating conditions)
* **Compression Ratio (CR):**
  $$\text{CR} = \frac{\text{Raw Size}}{\text{Compressed Size}} = \frac{400}{187} \approx 2.14\times$$
* **Bandwidth & RF Energy Conserved:**
  $$\Delta E = \left(1 - \frac{\text{Compressed Size}}{\text{Raw Size}}\right) \times 100\% = \left(1 - \frac{187}{400}\right) \times 100\% = 53.25\%$$
* **Inference + Compression Latency:** **3 ms** total execution time on the Cortex-M85 @ 480 MHz.

---

## 7. Display UI Architecture & Graphical Engine

The dashboard UI is built with LVGL v9.x on an EK-RA8P1 4.3-inch portrait TFT LCD (480 × 854 resolution).

```
+=============================================================================+
| [AIoT-MOR-ALDC]  EK-RA8P1 EDGE AI OS        [ DATA-DRIVEN ]         Pkt #0042 |
+=============================================================================+
| [ S1 ] LIVE SENSOR (P410)             | [ S2 ] DATA-DRIVEN (USB)            |
| Standby (Press S1)                    | [ * ACTIVE (USB) ]                  |
+---------------------------------------+-------------------------------------+
| [1] REAL-TIME SENSOR TELEMETRY                                              |
|                                                                             |
|   24.50 °C    STABLE                  HUMIDITY:  55.0 %RH                   |
|                                                                             |
|   Time: 12:45:10 PM                   Session Min: 24.20 °C                 |
|   Date: 30/09/2026                    Session Max: 34.30 °C                 |
|   Thermal Drift: +0.000 °C/min        Source: DATA-DRIVEN                   |
+-----------------------------------------------------------------------------+
| [2] THERMAL CHANGE TRACKER                                                  |
|   Last Event:  24.50 °C -> 32.10 °C (+7.60 °C)                              |
|   Event Time:  12:42:05 PM                                                  |
|   Time Since:  3 min 5 s                                                    |
+-----------------------------------------------------------------------------+
| [3] ON-DEVICE EDGE AI PIPELINE                                              |
|   Signal Pattern:  SMOOTH (Normal Profile)                                  |
|   Anomaly Status:  NORMAL - Clean Profile (No Anomaly)                      |
|   Codec Policy:    Adaptive Residual Codec (Active)                         |
|   Latency:         3 ms (ARM Cortex-M85 @ 480 MHz)                          |
+-----------------------------------------------------------------------------+
| [4] MOR-ALDC COMPRESSION BENCHMARKS                                         |
|   Payload:             400 Bytes (Raw) -> 187 Bytes (Compressed)            |
|   Compression Ratio:   2.14x  |  Bandwidth Saved: 53.3%                     |
|   Stream Link:         Active (SEGGER RTT / TCP 192.168.10.100)             |
|   [S1] Press S1 for Live Sensor   |   [S2] Press S2 for Data-Driven         |
+=============================================================================+
```

### 7.1 Key Screen Regions & Coordinates
* **Header Bar ($y = 0 \dots 50$, $w = 480$):** Deep navy background (`0x131E30`), electric teal bottom border (`0x00E5B8`), system title, active mode badge (`LIVE DHT11` or `DATA-DRIVEN`), and packet counter.
* **Dual-Mode Selector Boxes ($y = 54 \dots 132$, $h = 78$):**
  * **Box 1 ($x = 12, w = 222$):** Live DHT11 Sensor Box. Highlights in vivid green (`0x064E3B`) with active badge when selected.
  * **Box 2 ($x = 246, w = 222$):** Data-Driven Mode Box. Highlights in vibrant blue (`0x0C4A6E`) with active badge when selected.
* **Card 1: Sensor Telemetry ($y = 138 \dots 332$, $x = 12, w = 456, h = 194$):**
  * Primary temperature in 36pt font (`0xFF7A30` coral-orange).
  * Relative humidity in 20pt sky blue (`0x38BDF8`).
  * Real-time thermal drift rate in $^\circ\text{C}/\text{min}$ calculated from RTC timestamps.
  * Session minimum and maximum recorded temperatures.
* **Card 2: Thermal Change Tracker ($y = 338 \dots 480$, $x = 12, w = 456, h = 142$):**
  * Monitors thermal shifts exceeding a $\pm 1.0^\circ\text{C}$ threshold.
  * Displays baseline vs transition temperatures (e.g., `24.50 °C -> 32.10 °C (+7.60 °C)`).
  * Dynamic elapsed counter updating every second (e.g., `3 min 5 s`).
* **Card 3: Edge AI Pipeline ($y = 486 \dots 658$, $x = 12, w = 456, h = 172$):**
  * Classified waveform pattern (`SMOOTH`, `PERIODIC`, `STEP`, `NOISY`, `RAMP`).
  * Anomaly detection state (Green `NORMAL` vs Red `ANOMALY DETECTED`).
  * Active codec policy and execution latency.
* **Card 4: Compression Benchmarks ($y = 664 \dots 844$, $x = 12, w = 456, h = 180$):**
  * Raw vs compressed byte metrics.
  * Live compression ratio and percentage of RF bandwidth saved.
  * Physical button shortcuts reminder.

### 7.2 Graphics Engine Optimization & Artifact Fixes
* **Elimination of Three Vertical Cyan Lines:** Direct raw SDRAM framebuffer rectangle writes previously drawn at $x = 10, 11, 12$ were removed from `lv_port_display.c`. Solid background fill and pure LVGL software rendering (`lv_draw_sw`) ensure clean rendering.
* **Hardware 2D Bypass:** The Renesas D/AVE 2D hardware renderer was bypassed via `lv_subject_set_int(&dave2d_enable, 0)` to eliminate driver interrupt race conditions and ensure full font and card border rendering on the Cortex-M85.

---

## 8. Data Ingestion & Laptop Streaming Guide

The system supports multiple methods for injecting sensor data into the EK-RA8P1 board.

### Method 1: Double-Click Launcher (`feed_data.cmd`)
The easiest method on Windows:
1. Connect your laptop to the board's **J10 micro-USB connector** using the included USB cable.
2. In File Explorer, double-click:
   ```cmd
   c:\Users\udaya\e2_studio\workspace\AI_MOR_ALDC1\feed_data.cmd
   ```
3. A menu will appear with the following options:
   * **`[1]` Stream `sensor_data.txt` to board:** Streams readings at 1 sample per second.
   * **`[2]` Open `sensor_data.txt` in Windows Notepad:** Edit values, save, and exit.
   * **`[3]` Start interactive command line:** Type individual values manually.
   * **`[4]` Exit.**

### Method 2: Python Feeder Tool (`laptop_sensor_feeder.py`)
Run the Python feeder directly for fine-grained control:
```powershell
# Stream default sensor_data.txt over SEGGER RTT (port 19021) or auto-detected COM port
python laptop_sensor_feeder.py -f sensor_data.txt

# Stream at 2 Hz (0.5 seconds per sample)
python laptop_sensor_feeder.py -f sensor_data.txt -r 0.5

# Connect to a specific COM port
python laptop_sensor_feeder.py -p COM7 -f sensor_data.txt

# Start interactive CLI mode
python laptop_sensor_feeder.py
```

### Method 3: Editing `sensor_data.txt` in Notepad
Open `sensor_data.txt` in Notepad. Each non-comment line contains:
```text
<Temperature in Celsius>, <Relative Humidity in %RH>
```
Example dataset:
```text
# Normal Room Baseline
24.20, 54.0
24.30, 54.2
24.50, 55.0

# Dynamic Heating Ramp
28.00, 59.5
31.50, 62.0
34.00, 64.5

# Critical Thermal Runaway Anomaly (Triggers Isolation Forest Alarm)
42.50, 74.0
48.50, 79.5
49.80, 81.2

# Cooling Recovery
33.00, 64.0
25.20, 55.5
```

---

## 9. Build, Flash, & Debug Instructions

### 9.1 Development Prerequisites
* **IDE:** Renesas e² studio (v2024-01 or later)
* **Toolchain:** GNU Arm Embedded Toolchain (`arm-none-eabi-gcc` v13.2+)
* **FSP Version:** Renesas Flexible Software Package v6.5.0+
* **Debug Hardware:** On-board SEGGER J-Link OB on connector J10

### 9.2 Building the Firmware
1. Open e² studio and select workspace:
   `c:\Users\udaya\e2_studio\workspace\AI_MOR_ALDC1`
2. Right-click the project root in Project Explorer and select **Build Project** (or press `Ctrl + B`).
3. Successful build output in the console:
   ```text
   Invoking: GNU Arm Cross Print Size
   arm-none-eabi-size --format=berkeley "AI_MOR_ALDC1.elf"
      text    data     bss     dec     hex filename
    755028     436 1865324 2620788  27fc74 AI_MOR_ALDC1.elf
   Finished building: AI_MOR_ALDC1.siz
   ```

### 9.3 Flashing to the EK-RA8P1 Board
1. Plug the micro-USB cable into connector **J10** on the EK-RA8P1 board.
2. In e² studio, click the dropdown next to the green **Debug** button.
3. Select **AI_MOR_ALDC1 Debug_Flat.launch**.
4. The debugger will connect, erase Flash sectors, download `AI_MOR_ALDC1.elf`, and pause at `Reset_Handler`.
5. Press **Resume (F8)** to start execution.

---

## 10. Troubleshooting & Verification Matrix

| Symptom | Probable Cause | Verification & Corrective Action |
| :--- | :--- | :--- |
| **Display remains black on boot** | Backlight pin low or MIPI DSI initialization halted | Verify `BSP_IO_PORT_05_PIN_14` is driven HIGH in `lv_port_display.c`. Verify MIPI cable is fully seated in J49 connector. |
| **Three cyan vertical lines on left of screen** | Raw SDRAM card frame draws leaking into active area | Verify `lv_port_display.c` uses clean background fill `pre_draw_dashboard_layout()` without raw card borders. |
| **Startup modal does not respond to buttons** | Internal pull-ups unconfigured or wrong GPIOs | Confirm S1 is `P009`, S2 is `P008`, and USER_SW is `P000` with `IOPORT_CFG_PULLUP_ENABLE` active in `hal_entry.c`. |
| **DHT11 reads invalid or static numbers** | Missing 4.7kΩ pull-up resistor or wrong header pin | Ensure DHT11 signal pin is connected to `P410` (J18 Pin 2) with a 4.7kΩ–10kΩ pull-up resistor to 3.3V. |
| **Laptop feeder cannot connect** | Debugger holding COM port or Telnet not bound | Ensure e² studio debug session is running (exposing RTT port 19021). Alternatively, select the J-Link CDC COM port via `python laptop_sensor_feeder.py -p COMx`. |
| **Anomaly alarm does not trigger** | Temperature did not exceed threshold | In `sensor_data.txt`, add a line with temperature $>40.0^\circ\text{C}$ (e.g., `45.5, 75.0`). The Isolation Forest will trigger and turn the UI alarm RED. |
| **Screen tearing or jitter during updates** | Framebuffer swap not synchronized with VSYNC | Check that `my_flush_wait_cb` takes `g_vsync_sem` and `my_glcdc_callback` handles `DISPLAY_EVENT_LINE_DETECTION`. |

---
*End of AIoT-MOR-ALDC Master Technical Document.*
