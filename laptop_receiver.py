#!/usr/bin/env python3
"""
laptop_receiver.py — AIoT-MOR-ALDC EK-RA8P1 Real-Time Live Receiver & Feeder
===========================================================================
Connects your laptop directly to the Renesas EK-RA8P1 board over the standard
USB debug cable (J10 SEGGER J-Link VCOM) to receive live sensor, Edge AI,
and MOR-ALDC lossless compression data.

Supports:
  1. Serial / COM Port  (via J-Link CDC UART at 115200 baud)
  2. Live bidirectional communication (receive telemetry + inject sensor values)
  3. Real-time high-visibility colorized dashboard
  4. Automatic CSV logging with full timestamping
  5. Interactive keyboard shortcuts (type values or trigger anomalies)
"""

import sys
import os
import re
import json
import time
import socket
import datetime
import argparse
import csv
import threading

# Color support for Windows console
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


def list_com_ports():
    """List available serial/COM ports."""
    try:
        import serial.tools.list_ports
        ports = list(serial.tools.list_ports.comports())
        return ports
    except ImportError:
        return []


def print_banner():
    print(f"\n{CYAN}{BRIGHT}+=============================================================================+{RESET}")
    print(f"{CYAN}{BRIGHT}|          AIoT-MOR-ALDC EK-RA8P1 REAL-TIME LAPTOP RECEIVER & FEEDER            |{RESET}")
    print(f"{CYAN}{BRIGHT}|     Edge AI Anomaly Detection & Lossless Adaptive Compression OS            |{RESET}")
    print(f"{CYAN}{BRIGHT}+=============================================================================+{RESET}\n")


def display_dashboard(pkt_data, total_packets, anomalies_count, csv_filename):
    """Render a clean, high-visibility dashboard for each received packet."""
    pkt_id    = pkt_data.get("pkt", 0)
    src_mode  = pkt_data.get("src", "UNKNOWN")
    temp_c    = pkt_data.get("temp", 0.0)
    hum_pct   = pkt_data.get("hum", 0.0)
    ai_class  = pkt_data.get("class", "NORMAL")
    is_anomaly= pkt_data.get("anomaly", False)
    cr        = pkt_data.get("cr", 2.14)
    energy_mj = pkt_data.get("energy_used_mj", 0.0)
    saved_mj  = pkt_data.get("saved_mj", 0.0)
    saved_pct = pkt_data.get("saved_pct", 53.3)

    if is_anomaly:
        anomaly_str = f"{RED}{BRIGHT} [!! ANOMALY DETECTED !!] {RESET}"
    else:
        anomaly_str = f"{GREEN}{BRIGHT} [ SYSTEM: OPTIMAL ] {RESET}"

    comp_status = f"{GREEN}ACTIVE ({cr:.2f}x ratio, -{saved_pct:.1f}% bandwidth){RESET}"

    print(f"\r{CYAN}+----------------------- EK-RA8P1 TELEMETRY ------------------------+{RESET}")
    print(f"|  {WHITE}Packet ID{RESET}  : {BRIGHT}#{pkt_id:04d}{RESET}  |  {WHITE}Source{RESET} : {MAGENTA}{src_mode}{RESET}")
    print(f"|  {WHITE}Temperature{RESET}: {YELLOW}{temp_c:6.2f} degC{RESET}   |  {WHITE}Humidity{RESET} : {CYAN}{hum_pct:6.1f} % RH{RESET}")
    print(f"|  {WHITE}Edge AI{RESET}    : {MAGENTA}{ai_class:<10}{RESET}       Status: {anomaly_str}")
    print(f"|  {WHITE}Compression{RESET}: {comp_status}")
    print(f"|  {WHITE}Energy Used{RESET}: {WHITE}{energy_mj:.2f} mJ{RESET}       |  {WHITE}Energy Saved{RESET}: {GREEN}+{saved_mj:.2f} mJ ({saved_pct:.1f}%){RESET}")
    print(f"{CYAN}+-------------------------------------------------------------------+{RESET}")
    print(f"  Total: {total_packets} pkts | Anomalies: {RED if anomalies_count else GREEN}{anomalies_count}{RESET} | Log: {csv_filename}\n")


def init_csv_file():
    """Create timestamped CSV file and write header."""
    now_str = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"sensor_ai_data_{now_str}.csv"
    headers = [
        "Packet_ID", "Timestamp", "Data_Source",
        "Temperature_C", "Humidity_Pct",
        "AI_Class", "Is_Anomaly",
        "Compression_Ratio", "Energy_Used_mJ", "Saved_mJ", "Saved_Pct"
    ]
    with open(filename, mode="w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(headers)
    return filename


def append_to_csv(filename, pkt_data):
    """Append one data packet to the CSV log."""
    row = [
        pkt_data.get("pkt", 0),
        datetime.datetime.now().isoformat(),
        pkt_data.get("src", "UNKNOWN"),
        pkt_data.get("temp", 0.0),
        pkt_data.get("hum", 0.0),
        pkt_data.get("class", "NORMAL"),
        1 if pkt_data.get("anomaly", False) else 0,
        pkt_data.get("cr", 1.0),
        pkt_data.get("energy_used_mj", 0.0),
        pkt_data.get("saved_mj", 0.0),
        pkt_data.get("saved_pct", 0.0)
    ]
    with open(filename, mode="a", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(row)


def parse_telemetry_line(line):
    """
    Parses line format:
    [#0001] [SRC: LAPTOP] Temp: 28.50 C | Hum: 55.0 % | Pattern: SMOOTH | Anomaly: NO | CR: 2.14x | Energy Used: 0.22 mJ | SAVED: +0.26 mJ (53.3%)
    """
    match = re.search(
        r"\[#(\d+)\]\s*\[SRC:\s*([^\]]+)\]\s*Temp:\s*([0-9.-]+)\s*C\s*\|\s*Hum:\s*([0-9.-]+)\s*%\s*\|\s*Pattern:\s*([^|]+)\|\s*Anomaly:\s*([^|]+)\|\s*CR:\s*([0-9.-]+)x\s*\|\s*Energy Used:\s*([0-9.-]+)\s*mJ\s*\|\s*SAVED:\s*\+?([0-9.-]+)\s*mJ\s*\(([0-9.-]+)%\)",
        line
    )
    if not match:
        return None

    return {
        "pkt": int(match.group(1)),
        "src": match.group(2).strip(),
        "temp": float(match.group(3)),
        "hum": float(match.group(4)),
        "class": match.group(5).strip(),
        "anomaly": (match.group(6).strip().upper() == "YES"),
        "cr": float(match.group(7)),
        "energy_used_mj": float(match.group(8)),
        "saved_mj": float(match.group(9)),
        "saved_pct": float(match.group(10))
    }


def run_serial_receiver(port, baudrate, csv_filename):
    """Receive data via Serial / COM port and allow sending live data."""
    import serial
    print(f"{GREEN}Connecting to {port} at {baudrate} baud...{RESET}")
    ser = serial.Serial(port=port, baudrate=baudrate, timeout=0.2)
    ser.reset_input_buffer()
    print(f"{GREEN}{BRIGHT}Connected! Waiting for board telemetry...{RESET}")
    print(f"{YELLOW}Type any temperature (e.g. '28.5' or 'ANOMALY') and press Enter to feed data to board!{RESET}\n")

    total_packets = 0
    anomalies_count = 0
    running = True

    def tx_input_loop():
        nonlocal running
        while running:
            try:
                cmd = input()
                if not cmd.strip():
                    continue
                if cmd.lower() in ("q", "quit", "exit"):
                    running = False
                    break
                ser.write((cmd.strip() + "\r\n").encode("utf-8"))
                ser.flush()
                print(f"{MAGENTA}>>> Sent to Board: {cmd.strip()}{RESET}")
            except (EOFError, KeyboardInterrupt):
                running = False
                break

    input_thread = threading.Thread(target=tx_input_loop, daemon=True)
    input_thread.start()

    try:
        while running:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not line:
                continue

            # Check for JSON packet
            if line.startswith("{") and line.endswith("}"):
                try:
                    pkt_data = json.loads(line)
                    total_packets += 1
                    if pkt_data.get("anomaly", False):
                        anomalies_count += 1
                    append_to_csv(csv_filename, pkt_data)
                    display_dashboard(pkt_data, total_packets, anomalies_count, csv_filename)
                    continue
                except json.JSONDecodeError:
                    pass

            # Check for Formatted Telemetry Line
            pkt = parse_telemetry_line(line)
            if pkt:
                total_packets += 1
                if pkt["anomaly"]:
                    anomalies_count += 1
                append_to_csv(csv_filename, pkt)
                display_dashboard(pkt, total_packets, anomalies_count, csv_filename)
            else:
                # Other console outputs (e.g. boot banners, acknowledgements)
                if "[LAPTOP-RX]" in line:
                    print(f"{GREEN}{BRIGHT}{line}{RESET}")
                elif "ANOMALY" in line:
                    print(f"{RED}{BRIGHT}{line}{RESET}")
                else:
                    print(f"{WHITE}{line}{RESET}")

    except KeyboardInterrupt:
        pass
    finally:
        running = False
        ser.close()
        print(f"\n{YELLOW}Stopping receiver. All {total_packets} packets saved to {csv_filename}.{RESET}")


def main():
    print_banner()

    parser = argparse.ArgumentParser(description="AIoT-MOR-ALDC EK-RA8P1 Data Receiver & Feeder")
    parser.add_argument("--port", type=str, default=None,
                        help="COM port name (e.g. COM3). Auto-detected if not specified.")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    args = parser.parse_args()

    csv_file = init_csv_file()
    print(f"{GREEN}Logging all data to: {BRIGHT}{csv_file}{RESET}\n")

    target_port = args.port
    if not target_port:
        ports = list_com_ports()
        if not ports:
            print(f"{RED}No COM ports detected on your laptop!{RESET}")
            print(f"{YELLOW}1. Make sure EK-RA8P1 is plugged into your laptop via the USB debug cable (J10).{RESET}")
            sys.exit(1)

        # Look for J-Link or Renesas port
        selected = None
        for p in ports:
            desc = (p.description or "").lower()
            if "j-link" in desc or "cdc" in desc or "renesas" in desc or "serial" in desc:
                selected = p.device
                print(f"{CYAN}Found board COM port: {BRIGHT}{p.device}{RESET} ({p.description})")
                break

        if not selected:
            selected = ports[0].device
            print(f"{CYAN}Selecting COM port: {BRIGHT}{selected}{RESET} ({ports[0].description})")

        target_port = selected

    run_serial_receiver(target_port, args.baud, csv_file)


if __name__ == "__main__":
    main()
