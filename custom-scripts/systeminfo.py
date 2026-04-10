#!/usr/bin/env python3

import json
import time
import os
from http.server import BaseHTTPRequestHandler, HTTPServer
from datetime import datetime

# --- Alunos devem implementar as funções abaixo --- #
def read_cpu():
        with open("/proc/stat", "r") as f:
            line = f.readline()
            values = list(map(int, line.split()[1:]))
            
            idle = values[3] + values[4]      # idle + iowait
            total = sum(values)
            
            return idle, total
        
def get_cpu_usage():
    idle1, total1 = read_cpu()
    time.sleep(0.5)
    idle2, total2 = read_cpu()

    idle_delta  = idle2 - idle1
    total_delta = total2 - total1

    usage = 100.0 * (1 - (idle_delta / total_delta))
    return round(usage, 2)

def get_datetime():
    return str(datetime.now())

def get_uptime():
    with open("/proc/uptime", "r") as f:
        return f.readline().split()[0]

def get_cpu_info():
    cpu_info = dict()
    with open("/proc/cpuinfo", "r") as f:
        for line in f:
            striped_line = line.strip()
            if striped_line.startswith("model name"):
                cpu_info["model"] = striped_line.split(":")[1].strip()
            
            if striped_line.startswith("cpu MHz"):
                cpu_info["speed_mhz"] = striped_line.split(":")[1].strip()
    
    cpu_info["usage_percent"] = get_cpu_usage()

    return cpu_info

def get_memory_info():
    meminfo = {}

    with open("/proc/meminfo", "r") as f:
        for line in f:
            key, value = line.split(":")
            meminfo[key] = int(value.strip().split()[0])  # em kB

    total   = meminfo.get("MemTotal", 0)
    free    = meminfo.get("MemFree", 0)
    buffers = meminfo.get("Buffers", 0)
    cached  = meminfo.get("Cached", 0)
    used    = total - free - buffers - cached

    return {
        "total_mb": round(total / 1024, 2),
        "used_mb": round(used / 1024, 2)
    }

def get_os_version():    
    with open("/proc/version", "r") as f:
        return f.read().split(",")[0].strip()

def get_process_list():
    processes = []

    for pid in os.listdir("/proc"):
        if pid.isdigit():
            try:
                with open(f"/proc/{pid}/comm", "r") as f:
                    name = f.read().strip()

                processes.append({
                    "pid": int(pid),
                    "name": name
                })
            except:
                continue

    return processes

def get_disks():
    disks = []

    with open("/proc/partitions", "r") as f:
        for line in f.readlines()[2:]:
            parts = line.split()
            if len(parts) == 4:
                blocks = int(parts[2])
                name = parts[3]
                
                if name.startswith("loop") or name.startswith("ram"):
                    continue
                
                size_mb = blocks / 1024  # blocks em KB

                disks.append({
                    "device": f"/dev/{name}",
                    "size_mb": int(size_mb)
                })

    return disks

def get_usb_devices():
    devices = []
    base_path = "/sys/bus/usb/devices"

    for dev in os.listdir(base_path):
        dev_path = os.path.join(base_path, dev)

        try:
            with open(os.path.join(dev_path, "product"), "r") as f:
                product = f.read().strip()

            devices.append({
                "port": dev,
                "description": product
            })
        except:
            continue

    return devices

#def get_network_adapters():
#    return ({
#            "interface": "iface",
#            "ip_address": "ip"
#        })

import os

def get_network_adapters():
    adapters = []
    interfaces = os.listdir("/sys/class/net")

    # --- pegar IPs válidos ---
    ips = []
    last_ip = None

    with open("/proc/net/fib_trie", "r") as f:
        for line in f:
            line = line.strip()

            if line.startswith("|--"):
                parts = line.split()
                if len(parts) >= 2:
                    last_ip = parts[-1]

            elif "host LOCAL" in line and last_ip:
                if (
                    last_ip != "127.0.0.1" and
                    not last_ip.endswith(".0") and
                    not last_ip.endswith(".255")
                ):
                    ips.append(last_ip)

    # remove duplicados mantendo ordem
    seen = set()
    ips = [ip for ip in ips if not (ip in seen or seen.add(ip))]

    # --- interfaces que possuem rota (logo têm IP) ---
    ifaces_with_ip = set()

    with open("/proc/net/route", "r") as f:
        next(f)  # pula header
        for line in f:
            parts = line.split()
            iface = parts[0]
            ifaces_with_ip.add(iface)

    # --- associação correta ---
    ip_index = 0

    for iface in interfaces:
        if iface == "lo":
            ip = "127.0.0.1"

        elif iface in ifaces_with_ip:
            ip = ips[ip_index] if ip_index < len(ips) else None
            ip_index += 1

        else:
            ip = None

        adapters.append({
            "interface": iface,
            "ip_address": ip
        })

    return adapters

# --- Servidor HTTP --- #

class StatusHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/status":
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")
            return

        response = {
            "datetime": get_datetime(),
            "uptime_seconds": get_uptime(),
            "cpu": get_cpu_info(),
            "memory": get_memory_info(),
            "os_version": get_os_version(),
            "processes": get_process_list(),
            "disks": get_disks(),
            "usb_devices": get_usb_devices(),
            "network_adapters": get_network_adapters()
        }
        print(response)
        data = json.dumps(response, indent=2).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

def run_server(port=8080):
    print(f"Servidor disponível em http://0.0.0.0:{port}/status")
    server = HTTPServer(("0.0.0.0", port), StatusHandler)
    server.serve_forever()

if __name__ == "__main__":
    run_server()
