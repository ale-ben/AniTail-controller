#!/usr/bin/env python3
"""
Basic script to send POST requests at regular intervals
Usage: python basicMove.py
"""

import urllib.request
import urllib.error
import time
import sys

# Configuration
IP_ADDRESS = "192.168.1.51"  # Default ESP32 AP mode IP
PORT = 80
ENDPOINT = "/tcode"
INTERVAL_SECONDS = 1  # Send request every N seconds


def send_command(ip, port, endpoint, body):
    """Send a POST request with the specified body"""
    url = f"http://{ip}:{port}{endpoint}"
    try:
        data = body.encode('utf-8')
        req = urllib.request.Request(url, data=data, method='POST')
        with urllib.request.urlopen(req, timeout=2) as response:
            response_data = response.read().decode('utf-8')
            print(f"✓ Sent: {body}")
            print(f"  Response: {response.status} - {response_data}")
        return True
    except urllib.error.URLError as e:
        if hasattr(e, 'reason'):
            print(f"✗ Connection failed - {e.reason}")
        else:
            print(f"✗ Request timed out")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def main():
    print(f"Sending POST requests to http://{IP_ADDRESS}:{PORT}{ENDPOINT}")
    print(f"Interval: {INTERVAL_SECONDS} seconds")
    print(f"Press Ctrl+C to stop\n")
    
    count = 0
    try:
        while True:
            count += 1
            print(f"[{count}] ", end="")
            send_command(IP_ADDRESS, PORT, ENDPOINT, "G0 A0 B180")
            time.sleep(INTERVAL_SECONDS)
            send_command(IP_ADDRESS, PORT, ENDPOINT, "G0 A180 B0")
            time.sleep(INTERVAL_SECONDS)
    except KeyboardInterrupt:
        print(f"\n\nStopped after {count} requests")
        sys.exit(0)

if __name__ == "__main__":
    main()
