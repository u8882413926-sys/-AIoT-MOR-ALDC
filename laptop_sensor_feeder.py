#!/usr/bin/env python3
"""
laptop_sensor_feeder.py — AIoT-MOR-ALDC EK-RA8P1 Live Sensor Feeder & Dataset Streamer
=====================================================================================
Directly injects live temperature & humidity data from your laptop into the
Renesas EK-RA8P1 board over the standard USB debug cable (J10 SEGGER J-Link VCOM).

Features:
  - Notepad File Streaming: Read any .txt file (e.g. sensor_data.txt) simulating real-world sensor profiles.
  - Interactive Mode: Type individual values ('28.5', '30.2, 60.5'), 'anomaly' (a), or 'normal' (n).
  - Continuous Wave Stream: Automated sinusoidal thermal profile.
  - Live Telemetry: View board compression ratio, AI classification, and energy conserved in real-time.
  - Notepad Launcher: Type 'notepad' or 'edit' to immediately open sensor_data.txt in Windows Notepad.
"""

import sys
import os
import time
import math
import threading
import argparse
import subprocess
import socket

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[ERROR] 'pyserial' is not installed.")
    print("Please install it by running: pip install pyserial colorama")
    sys.exit(1)

try:
    import colorama
    colorama.init(autoreset=True)
    GREEN = colorama.Fore.GREEN
    RED = colorama.Fore.RED
    YELLOW = colorama.Fore.YELLOW
    CYAN = colorama.Fore.CYAN
    MAGENTA = colorama.Fore.MAGENTA
    WHITE = colorama.Fore.WHITE
    BRIGHT = colorama.Style.BRIGHT
    RESET = colorama.Style.RESET_ALL
except ImportError:
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    CYAN = "\033[96m"
    MAGENTA = "\033[95m"
    WHITE = "\033[97m"
    BRIGHT = "\033[1m"
    RESET = "\033[0m"


DEFAULT_DATASET = "sensor_data.txt"


def print_banner():
    print(f"{CYAN}{BRIGHT}+=============================================================================+{RESET}")
    print(f"{CYAN}{BRIGHT}|      AIoT-MOR-ALDC : LIVE LAPTOP SENSOR FEEDER & DATASET STREAMER             |{RESET}")
    print(f"{CYAN}{BRIGHT}|            Renesas EK-RA8P1 (ARM Cortex-M85 @ 480 MHz)                      |{RESET}")
    print(f"{CYAN}{BRIGHT}+=============================================================================+{RESET}")
    print(f"{WHITE} Connects to board VCOM over USB debug cable (J10). Streaming to Edge AI OS.{RESET}\n")


def find_jlink_port():
    """Auto-detect the J-Link CDC UART / Renesas COM Port."""
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None

    # Priority match for SEGGER / J-Link / Renesas
    for p in ports:
        desc = (p.description or "").lower()
        hwid = (p.hwid or "").lower()
        if "j-link" in desc or "jlink" in desc or "cdc" in desc or "renesas" in desc or "0451" in hwid or "1366" in hwid:
            return p.device, p.description

    # Fallback to the first available USB-serial port
    for p in ports:
        desc = (p.description or "").lower()
        if "usb" in desc or "serial" in desc:
            return p.device, p.description

    return ports[0].device, ports[0].description


def parse_dataset_file(filepath):
    """Parses a text / Notepad / CSV file for temperature & humidity readings.
    Supports formats:
      1. Standard 2-column: '24.50, 55.0' or 'T: 28.5, H: 62.0'
      2. Single column: '24.50' (humidity auto-synthesized)
      3. Intel Windows dataset CSV: 101 columns (window_id, sample_0 .. sample_99)
    Ignores blank lines, headers, and comments starting with '#' or '//'.
    """
    if not os.path.exists(filepath):
        # Also check current user directory and workspace
        alt1 = os.path.join(os.path.expanduser("~"), filepath)
        alt2 = os.path.join(os.path.dirname(os.path.abspath(__file__)), filepath)
        if os.path.exists(alt1):
            filepath = alt1
        elif os.path.exists(alt2):
            filepath = alt2
        else:
            return None, f"File not found: {filepath}"

    readings = []
    last_valid_temp = 20.0

    def synth_humidity(temp_c):
        h = 40.0 + (temp_c - 17.0) * 2.5
        return max(30.0, min(95.0, round(h, 1)))

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        for line_no, raw_line in enumerate(f, 1):
            line = raw_line.strip()
            if not line or line.startswith("#") or line.startswith("//"):
                continue

            # Strip units and labels if present
            cleaned = line.replace("C", "").replace("%", "").replace("T:", "").replace("H:", "").strip()
            parts = [p.strip() for p in cleaned.split(",") if p.strip()]
            if not parts:
                parts = cleaned.split()
            if not parts:
                continue

            # Check if this is a header line (non-numeric parts)
            try:
                float(parts[0])
            except ValueError:
                continue

            # Case A: Intel Windows CSV format (> 10 columns per row, e.g. 101 cols)
            if len(parts) >= 20:
                # Column 0 is window_id; columns 1..N are samples
                for col_val in parts[1:]:
                    try:
                        temp = float(col_val)
                        if temp >= 100.0 or temp < -40.0:  # Sensor fault marker
                            temp = last_valid_temp
                        else:
                            last_valid_temp = temp
                        hum = synth_humidity(temp)
                        readings.append((temp, hum, f"{temp:.2f}, {hum:.1f}"))
                    except ValueError:
                        continue
            # Case B: Standard 1 or 2-column sensor text format
            else:
                try:
                    temp = float(parts[0])
                    if temp >= 100.0 or temp < -40.0:
                        temp = last_valid_temp
                    else:
                        last_valid_temp = temp
                    hum = float(parts[1]) if len(parts) > 1 else synth_humidity(temp)
                    readings.append((temp, hum, f"{temp:.2f}, {hum:.1f}"))
                except ValueError:
                    continue

    return readings, filepath


class BoardBridge:
    def __init__(self, port=None, baudrate=115200, rtt_host="127.0.0.1", rtt_port=19021):
        self.port = port
        self.baudrate = baudrate
        self.rtt_host = rtt_host
        self.rtt_port = rtt_port
        self.ser = None
        self.sock = None
        self.conn_type = None  # 'rtt' or 'serial'
        self.running = False
        self.rx_thread = None

        # Streaming state
        self.stream_active = False
        self.stream_mode = None  # 'wave' or 'file'
        self.stream_thread = None
        self.stream_rate = 1.0  # seconds between samples
        self.stream_loop = True
        self.current_file = None
        self.file_readings = []

    def connect(self):
        # 1. Try SEGGER RTT Telnet (localhost:19021) first if no specific COM port forced
        if not self.port or str(self.port).upper() == "RTT":
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(1.0)
                s.connect((self.rtt_host, self.rtt_port))
                s.settimeout(0.2)
                self.sock = s
                self.conn_type = "rtt"
                self.running = True
                print(f"{GREEN}{BRIGHT}[CONNECTED]{RESET} {GREEN}Link established with EK-RA8P1 via SEGGER RTT (localhost:{self.rtt_port})!{RESET}\n")
                self.rx_thread = threading.Thread(target=self._rx_worker, daemon=True)
                self.rx_thread.start()
                return True
            except Exception:
                self.sock = None

        # 2. Try Serial COM port
        if self.port and str(self.port).upper() != "RTT":
            print(f"{YELLOW}Connecting to EK-RA8P1 on {self.port} at {self.baudrate} baud...{RESET}")
            try:
                self.ser = serial.Serial(
                    port=self.port,
                    baudrate=self.baudrate,
                    bytesize=serial.EIGHTBITS,
                    parity=serial.PARITY_NONE,
                    stopbits=serial.STOPBITS_ONE,
                    timeout=0.2
                )
                self.conn_type = "serial"
                self.running = True
                print(f"{GREEN}{BRIGHT}[CONNECTED]{RESET} {GREEN}USB Serial Link established with EK-RA8P1 ({self.port})!{RESET}\n")
                self.rx_thread = threading.Thread(target=self._rx_worker, daemon=True)
                self.rx_thread.start()
                return True
            except Exception as e:
                print(f"{RED}[ERROR] Failed to open {self.port}: {e}{RESET}")

        return False

    def send_line(self, line):
        payload = (line.strip() + "\r\n").encode("utf-8")
        if self.conn_type == "rtt" and self.sock:
            try:
                self.sock.sendall(payload)
            except Exception as e:
                print(f"{RED}[RTT TX ERROR] {e}{RESET}")
        elif self.conn_type == "serial" and self.ser and self.ser.is_open:
            try:
                self.ser.write(payload)
                self.ser.flush()
            except Exception as e:
                print(f"{RED}[SERIAL TX ERROR] {e}{RESET}")
        else:
            print(f"{RED}[ERROR] Not connected to board.{RESET}")

    def _rx_worker(self):
        """Continuously reads incoming console & telemetry messages from the board."""
        while self.running:
            try:
                raw_text = None
                if self.conn_type == "rtt" and self.sock:
                    try:
                        data = self.sock.recv(1024)
                        if data:
                            raw_text = data.decode("utf-8", errors="replace")
                    except socket.timeout:
                        continue
                    except Exception:
                        break
                elif self.conn_type == "serial" and self.ser and self.ser.is_open:
                    if self.ser.in_waiting:
                        raw_text = self.ser.readline().decode("utf-8", errors="replace")
                    else:
                        time.sleep(0.01)
                        continue

                if raw_text:
                    for line in raw_text.splitlines():
                        line = line.strip()
                        if not line:
                            continue
                        if "ANOMALY" in line or "! ANOMALY" in line:
                            print(f"{RED}{BRIGHT}>>> {line}{RESET}")
                        elif "[LAPTOP-RX]" in line:
                            print(f"{GREEN}{BRIGHT}{line}{RESET}")
                        elif line.startswith("[#"):
                            print(f"{CYAN}{line}{RESET}")
                        elif "AIoT-MOR-ALDC" in line:
                            print(f"{MAGENTA}{line}{RESET}")
                        else:
                            print(f"{WHITE}{line}{RESET}")
            except Exception:
                break

    def start_file_stream(self, filepath=DEFAULT_DATASET, rate=1.0, loop=True):
        """Streams sensor readings loaded from a text / Notepad file at the given rate."""
        readings, actual_path = parse_dataset_file(filepath)
        if not readings:
            print(f"{RED}[ERROR] {actual_path}{RESET}")
            return False

        self.stop_stream()
        self.file_readings = readings
        self.current_file = actual_path
        self.stream_rate = rate
        self.stream_loop = loop
        self.stream_mode = "file"
        self.stream_active = True

        self.stream_thread = threading.Thread(target=self._file_stream_worker, daemon=True)
        self.stream_thread.start()

        loop_str = "Enabled (will loop continuously)" if loop else "Disabled (single pass)"
        print(f"{GREEN}{BRIGHT}[FILE STREAM STARTED]{RESET} {GREEN}Loaded {len(readings)} readings from: {actual_path}{RESET}")
        print(f"{CYAN}Rate: {rate:.1f}s / sample  |  Loop: {loop_str}{RESET}")
        print(f"{YELLOW}Type 'stop' or 's' to pause streaming.{RESET}\n")
        return True

    def _file_stream_worker(self):
        readings = self.file_readings
        total = len(readings)

        while self.stream_active and self.running:
            for idx, (temp, hum, raw_line) in enumerate(readings, 1):
                if not self.stream_active or not self.running:
                    return

                # Send line to board
                msg = f"{temp:.2f}, {hum:.1f}"
                self.send_line(msg)

                # Anomaly highlight
                status_tag = f"{RED}[ANOMALY SPIKE]{RESET}" if temp >= 40.0 else f"{GREEN}[NORMAL]{RESET}"
                print(f"{WHITE}[STREAM {idx:03d}/{total:03d}] Sent: {CYAN}{temp:.2f} C{RESET}, {CYAN}{hum:.1f} %RH{RESET} {status_tag}")

                time.sleep(self.stream_rate)

            if not self.stream_loop:
                print(f"{YELLOW}[STREAM COMPLETED] Finished streaming {total} samples from {self.current_file}.{RESET}")
                self.stream_active = False
                break
            else:
                print(f"{MAGENTA}--- End of dataset reached: Looping back to beginning ---{RESET}")

    def start_wave_stream(self, rate=1.0):
        """Continuously streams synthetic sinusoidal thermal curve at 1.0 Hz."""
        self.stop_stream()
        self.stream_rate = rate
        self.stream_mode = "wave"
        self.stream_active = True
        self.stream_thread = threading.Thread(target=self._wave_worker, daemon=True)
        self.stream_thread.start()
        print(f"{GREEN}[STREAM STARTED] Streaming live sinusoidal thermal curve at {rate:.1f} Hz...{RESET}")
        print(f"{YELLOW}Type 'stop' or 's' to stop streaming.{RESET}\n")

    def _wave_worker(self):
        step = 0
        while self.stream_active and self.running:
            t = 25.5 + 3.5 * math.sin(step * 0.1)
            h = 55.0 + 8.0 * math.cos(step * 0.1)
            self.send_line(f"{t:.2f}, {h:.1f}")
            step += 1
            time.sleep(self.stream_rate)

    def stop_stream(self):
        if self.stream_active:
            self.stream_active = False
            self.stream_mode = None
            print(f"{YELLOW}[STREAM STOPPED] Stopped live streaming. Board will hold or auto-fallback.{RESET}\n")

    def close(self):
        self.stream_active = False
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
        print(f"\n{YELLOW}Closed serial connection.{RESET}")


def open_notepad(filepath=DEFAULT_DATASET):
    """Opens the dataset file in Windows Notepad."""
    target = filepath
    if not os.path.exists(target):
        alt = os.path.join(os.path.expanduser("~"), filepath)
        if os.path.exists(alt):
            target = alt
    print(f"{CYAN}Opening '{target}' in Windows Notepad...{RESET}")
    try:
        subprocess.Popen(["notepad.exe", target])
    except Exception as e:
        print(f"{RED}[ERROR] Could not launch Notepad: {e}{RESET}")


def interactive_cli(bridge):
    print(f"{WHITE}{BRIGHT}Interactive Commands:{RESET}")
    print(f"  {CYAN}f{RESET} or {CYAN}file [path]{RESET}   : Stream readings from Notepad file (default: {DEFAULT_DATASET})")
    print(f"  {CYAN}notepad{RESET} or {CYAN}edit{RESET}    : Open {DEFAULT_DATASET} in Windows Notepad")
    print(f"  {CYAN}<float>{RESET}            : Set Temperature (e.g. {GREEN}27.4{RESET} or {GREEN}31.5{RESET})")
    print(f"  {CYAN}<temp>, <hum>{RESET}     : Set Temp & Humidity (e.g. {GREEN}28.5, 62.0{RESET})")
    print(f"  {CYAN}a{RESET} or {CYAN}anomaly{RESET}     : Inject high thermal spike ({RED}48.5 degC{RESET}) -> Triggers Anomaly Alert & Red LED")
    print(f"  {CYAN}n{RESET} or {CYAN}normal{RESET}      : Reset to normal ambient baseline ({GREEN}24.5 degC, 55% RH{RESET})")
    print(f"  {CYAN}s{RESET} or {CYAN}stream{RESET}      : Toggle sinusoidal wave stream")
    print(f"  {CYAN}rate <seconds>{RESET}  : Change playback delay (e.g. 'rate 0.5' for 2Hz)")
    print(f"  {CYAN}loop{RESET}               : Toggle continuous looping on/off")
    print(f"  {CYAN}stop{RESET}               : Pause any active file or wave stream")
    print(f"  {CYAN}h{RESET} or {CYAN}help{RESET}        : Show command help")
    print(f"  {CYAN}q{RESET} or {CYAN}quit{RESET}        : Exit script\n")

    while bridge.running:
        try:
            cmd = input(f"{YELLOW}[LAPTOP-INPUT] > {RESET}").strip()
            if not cmd:
                continue

            low = cmd.lower()
            if low in ("q", "quit", "exit"):
                break
            elif low in ("h", "help", "?"):
                print("Commands:")
                print("  f [filename]   - Stream data from Notepad text file")
                print("  notepad / edit - Open sensor_data.txt in Windows Notepad")
                print("  <float>        - Set temperature (e.g. 27.5)")
                print("  <temp>, <hum>  - Set temp & humidity (e.g. 28.5, 60.0)")
                print("  a / anomaly    - Thermal runaway spike (48.5 C)")
                print("  n / normal     - Normal baseline (24.5 C, 55 %RH)")
                print("  s / stream     - Sine wave stream")
                print("  rate <sec>     - Playback interval (default: 1.0s)")
                print("  loop           - Toggle file looping")
                print("  stop           - Stop streaming")
                print("  quit           - Exit")
            elif low in ("notepad", "edit", "notepad.exe"):
                open_notepad(DEFAULT_DATASET)
            elif low.startswith("f ") or low.startswith("file ") or low in ("f", "file"):
                parts = cmd.split(maxsplit=1)
                filepath = parts[1].strip() if len(parts) > 1 else DEFAULT_DATASET
                bridge.start_file_stream(filepath=filepath, rate=bridge.stream_rate, loop=bridge.stream_loop)
            elif low.startswith("rate ") or low.startswith("delay "):
                parts = cmd.split()
                try:
                    r = float(parts[1])
                    if r > 0:
                        bridge.stream_rate = r
                        print(f"{GREEN}Stream rate updated to: {r:.2f} seconds per sample{RESET}")
                except (ValueError, IndexError):
                    print(f"{RED}Usage: rate <seconds> (e.g. 'rate 0.5'){RESET}")
            elif low in ("loop", "repeat"):
                bridge.stream_loop = not bridge.stream_loop
                st = "ON (continuous loop)" if bridge.stream_loop else "OFF (single pass)"
                print(f"{CYAN}Dataset loop is now: {st}{RESET}")
            elif low in ("a", "anomaly", "spike", "fire"):
                print(f"{RED}[SENDING] Injecting 48.5 degC thermal anomaly spike...{RESET}")
                bridge.send_line("ANOMALY")
            elif low in ("n", "normal", "reset", "cool"):
                print(f"{GREEN}[SENDING] Restoring 24.5 degC normal baseline...{RESET}")
                bridge.send_line("NORMAL")
            elif low in ("s", "stream"):
                if bridge.stream_active:
                    bridge.stop_stream()
                else:
                    bridge.start_wave_stream()
            elif low in ("stop", "halt", "pause"):
                bridge.stop_stream()
            else:
                # Direct number or format
                bridge.send_line(cmd)
                time.sleep(0.05)

        except (KeyboardInterrupt, EOFError):
            break

    bridge.close()


def main():
    print_banner()

    parser = argparse.ArgumentParser(description="AIoT-MOR-ALDC EK-RA8P1 Laptop Sensor Feeder")
    parser.add_argument("--port", type=str, default=None, help="COM port (e.g. COM19). Auto-detected if omitted.")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("-f", "--file", type=str, default=None, nargs="?", const=DEFAULT_DATASET,
                        help=f"Stream readings from Notepad dataset file (default: {DEFAULT_DATASET})")
    parser.add_argument("--rate", type=float, default=1.0, help="Seconds per sample when streaming (default: 1.0)")
    parser.add_argument("--no-loop", action="store_true", help="Do not loop dataset file after reaching end")
    parser.add_argument("--temp", type=float, default=None, help="Single-shot temperature injection and exit")
    parser.add_argument("--stream", action="store_true", help="Start in automated sinusoidal streaming mode immediately")
    args = parser.parse_args()

    # Determine connection method
    port = args.port
    bridge = BoardBridge(port, args.baud)

    # 1. Try connecting (will test RTT localhost:19021 first)
    if not bridge.connect():
        # 2. If RTT not open, check if a COM port is available
        res = find_jlink_port()
        if res:
            port, desc = res
            print(f"{CYAN}Trying detected board COM Port: {BRIGHT}{port}{RESET} ({desc})")
            bridge.port = port
            bridge.connect()

    if not bridge.running:
        print(f"\n{RED}[COULD NOT CONNECT TO BOARD]{RESET}")
        print(f"{YELLOW}How to connect and upload data:{RESET}")
        print(f"  Option 1 (e2 studio / RTT): Start a Debug session in e2 studio (this opens SEGGER RTT).")
        print(f"  Option 2 (USB Cable): Plug micro-USB into connector J10 (DEBUG_USB).")
        print(f"  Option 3 (Built-in Demo): When board boots, select [S2] DATA-DRIVEN mode;")
        print(f"           the board automatically loops the multi-scenario dataset with NO script needed!")
        print(f"  Option 4 (Flash Embed): Put your numbers in 's_benchmark_dataset[]' in 'src/sensors/sensor_source.c'.")
        sys.exit(1)

    # Single-shot mode
    if args.temp is not None:
        print(f"[TX] Injecting Temperature: {args.temp:.2f} degC")
        bridge.send_line(f"{args.temp:.2f}")
        time.sleep(0.5)
        bridge.close()
        return

    # Auto file stream mode
    if args.file:
        bridge.start_file_stream(filepath=args.file, rate=args.rate, loop=not args.no_loop)
    elif args.stream:
        bridge.start_wave_stream(rate=args.rate)

    # Interactive prompt
    try:
        interactive_cli(bridge)
    except KeyboardInterrupt:
        bridge.close()


if __name__ == "__main__":
    main()
