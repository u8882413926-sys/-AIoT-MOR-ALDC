#!/usr/bin/env python3
"""
tcp_server.py — AIoT-MOR-ALDC TCP Receiver
=========================================
Receives JSON sensor/AI data from the EK-RA8P1 board over Ethernet TCP.
Displays live readings on the console and logs everything to CSV.

Usage:
    python tcp_server.py                    # Default: 192.168.10.100:8888
    python tcp_server.py --host 0.0.0.0     # Listen on all interfaces
    python tcp_server.py --port 9999        # Custom port

Requirements:
    Python 3.7+ (no external packages needed)
"""

import socket
import json
import csv
import os
import sys
import argparse
import time
from datetime import datetime
from pathlib import Path

# ── Configuration ─────────────────────────────────────────────────────────
DEFAULT_HOST = "192.168.10.100"
DEFAULT_PORT = 8888
CSV_FILENAME = "sensor_log.csv"
BUFFER_SIZE  = 4096

# ── ANSI color codes for terminal output ──────────────────────────────────
class C:
    RESET   = "\033[0m"
    BOLD    = "\033[1m"
    DIM     = "\033[2m"
    RED     = "\033[91m"
    GREEN   = "\033[92m"
    YELLOW  = "\033[93m"
    BLUE    = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN    = "\033[96m"
    WHITE   = "\033[97m"
    BG_RED  = "\033[41m"
    BG_GREEN = "\033[42m"

def print_banner(host, port):
    """Print a styled startup banner."""
    print(f"""
{C.CYAN}{C.BOLD}╔══════════════════════════════════════════════════╗
║     AIoT-MOR-ALDC TCP Receiver v1.0               ║
║     Listening on {host}:{port:<21} ║
╚══════════════════════════════════════════════════╝{C.RESET}
""")

def anomaly_badge(is_anomaly):
    """Return a styled anomaly indicator."""
    if is_anomaly:
        return f"{C.BG_RED}{C.WHITE}{C.BOLD} ⚠ ANOMALY {C.RESET}"
    return f"{C.GREEN}NO{C.RESET}"

def format_packet(data):
    """Format a received JSON packet for console display."""
    pkt_id   = data.get("pkt", "?")
    temp     = data.get("temp", 0.0)
    hum      = data.get("hum", 0.0)
    cls_name = data.get("class", "?")
    anomaly  = data.get("anomaly", False)
    compress = data.get("compress", False)
    cr       = data.get("cr", 1.0)
    ms       = data.get("ms", 0)
    ts       = data.get("ts", "")
    uptime   = data.get("uptime", 0)

    # Temperature color coding
    if temp > 28:
        temp_color = C.RED
    elif temp < 22:
        temp_color = C.BLUE
    else:
        temp_color = C.GREEN

    line = (
        f"{C.DIM}{ts}{C.RESET} "
        f"{C.BOLD}PKT#{pkt_id:<5}{C.RESET} │ "
        f"{temp_color}{temp:6.2f}°C{C.RESET} │ "
        f"{C.CYAN}{hum:5.1f}%{C.RESET} │ "
        f"Class: {C.YELLOW}{cls_name:<8}{C.RESET} │ "
        f"Anomaly: {anomaly_badge(anomaly)} │ "
        f"CR: {C.MAGENTA}{cr:.2f}x{C.RESET} │ "
        f"{C.DIM}{ms}ms{C.RESET}"
    )
    return line

def init_csv(filepath):
    """Create CSV file with headers if it doesn't exist."""
    write_header = not os.path.exists(filepath)
    f = open(filepath, "a", newline="", encoding="utf-8")
    writer = csv.writer(f)
    if write_header:
        writer.writerow([
            "recv_time", "pkt_id", "temp_c", "humidity_pct",
            "signal_class", "class_id", "is_anomaly", "do_compress",
            "compression_ratio", "orig_bytes", "comp_bytes",
            "inference_ms", "board_timestamp", "uptime_sec",
            "feat_mean", "feat_std", "feat_min", "feat_max",
            "feat_pct25", "feat_pct75", "feat_diff_mean", "feat_diff_std"
        ])
        f.flush()
    return f, writer

def data_to_csv_row(data):
    """Convert a JSON packet to a CSV row."""
    features = data.get("feat", [0]*8)
    # Pad features to 8 if shorter
    while len(features) < 8:
        features.append(0.0)

    return [
        datetime.now().isoformat(),
        data.get("pkt", ""),
        data.get("temp", ""),
        data.get("hum", ""),
        data.get("class", ""),
        data.get("class_id", ""),
        data.get("anomaly", ""),
        data.get("compress", ""),
        data.get("cr", ""),
        data.get("orig", ""),
        data.get("comp", ""),
        data.get("ms", ""),
        data.get("ts", ""),
        data.get("uptime", ""),
    ] + [f"{f:.4f}" for f in features[:8]]

def handle_client(conn, addr, csv_writer, csv_file):
    """Handle a single board connection."""
    print(f"{C.GREEN}{C.BOLD}✅ Board connected from {addr[0]}:{addr[1]}{C.RESET}")
    print(f"{C.DIM}{'─' * 100}{C.RESET}")

    buffer = ""
    pkt_count = 0
    anomaly_count = 0
    start_time = time.time()

    try:
        while True:
            chunk = conn.recv(BUFFER_SIZE)
            if not chunk:
                break

            buffer += chunk.decode("utf-8", errors="replace")

            # Process all complete JSON lines in the buffer
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()
                if not line:
                    continue

                try:
                    data = json.loads(line)
                    pkt_count += 1

                    if data.get("anomaly", False):
                        anomaly_count += 1

                    # Print to console
                    print(f"  📊 {format_packet(data)}")

                    # Write to CSV
                    csv_writer.writerow(data_to_csv_row(data))
                    if pkt_count % 10 == 0:
                        csv_file.flush()

                except json.JSONDecodeError as e:
                    print(f"  {C.RED}⚠ Invalid JSON: {line[:80]}... ({e}){C.RESET}")

    except ConnectionResetError:
        pass
    except KeyboardInterrupt:
        raise
    finally:
        elapsed = time.time() - start_time
        print(f"\n{C.DIM}{'─' * 100}{C.RESET}")
        print(
            f"{C.YELLOW}📋 Session summary: "
            f"{pkt_count} packets in {elapsed:.1f}s "
            f"({pkt_count/max(elapsed,1):.1f} pkt/s), "
            f"{anomaly_count} anomalies detected{C.RESET}"
        )
        print(f"{C.RED}❌ Board disconnected from {addr[0]}{C.RESET}\n")
        csv_file.flush()
        conn.close()

def main():
    parser = argparse.ArgumentParser(description="AIoT-MOR-ALDC TCP Receiver")
    parser.add_argument("--host", default=DEFAULT_HOST,
                        help=f"Listen address (default: {DEFAULT_HOST})")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help=f"Listen port (default: {DEFAULT_PORT})")
    parser.add_argument("--csv", default=CSV_FILENAME,
                        help=f"CSV output file (default: {CSV_FILENAME})")
    args = parser.parse_args()

    # Resolve CSV path relative to script location
    script_dir = Path(__file__).parent
    csv_path = script_dir / args.csv

    # Initialize CSV logging
    csv_file, csv_writer = init_csv(str(csv_path))
    print(f"{C.DIM}📁 Logging to: {csv_path}{C.RESET}")

    print_banner(args.host, args.port)

    # Create TCP server socket
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server.bind((args.host, args.port))
    except OSError as e:
        if "requested address" in str(e).lower() or "can't assign" in str(e).lower():
            print(f"{C.RED}{C.BOLD}❌ Cannot bind to {args.host}:{args.port}{C.RESET}")
            print(f"{C.YELLOW}   Make sure your Ethernet adapter is set to IP: {args.host}{C.RESET}")
            print(f"{C.YELLOW}   Or use --host 0.0.0.0 to listen on all interfaces{C.RESET}")
            sys.exit(1)
        raise

    server.listen(1)

    try:
        while True:
            print(f"{C.CYAN}⏳ Waiting for board connection...{C.RESET}")
            conn, addr = server.accept()
            conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            handle_client(conn, addr, csv_writer, csv_file)

    except KeyboardInterrupt:
        print(f"\n{C.YELLOW}🛑 Server stopped by user (Ctrl+C){C.RESET}")
    finally:
        csv_file.close()
        server.close()
        print(f"{C.GREEN}💾 CSV data saved to: {csv_path}{C.RESET}")

if __name__ == "__main__":
    main()
