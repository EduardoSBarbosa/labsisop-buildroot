# Linux Status API (Buildroot)

## Description

This project consists of building an embedded Linux image using Buildroot, including a Python 3 application that runs automatically at system startup.

The application implements an HTTP server exposing:

GET /status

This endpoint returns system information in JSON format, collected dynamically using only /proc and /sys.

The server listens on port 8080.

---

## Execution

To run the code add the folder custom-scripts with the files on the root directory os labsisop, also is needed to pass ta file .config to lassbsispo root directory

code to start QUEMU

sudo qemu-system-i386 --device e1000,netdev=eth0,mac=aa:bb:cc:dd:ee:ff    
                     --netdev tap,id=eth0,script=custom-scripts/qemu-ifup    
                     --kernel output/images/bzImage  
                     --hda output/images/rootfs.ext2 
                     --nographic     
                     --append "console=ttyS0 root=/dev/sda"

After boot:

http://<target-ip>:8080/status

Example:
curl http://192.168.1.10:8080/status

---

## Example Response

{
  "datetime": "2026-04-08 03:13:54.667272",
  "uptime_seconds": "18.74",
  "cpu": {
    "model": "QEMU Virtual CPU version 2.5+",
    "speed_mhz": "1992.079",
    "usage_percent": 0.0
  },
  "memory": {
    "total_mb": 116.58,
    "used_mb": 10.79
  },
  "os_version": "Linux version 5.15.18...",
  "processes": [
    { "pid": 1, "name": "init" },
    { "pid": 80, "name": "python3" }
  ],
  "disks": [
    { "device": "/dev/sda", "size_mb": 60 }
  ],
  "usb_devices": [],
  "network_adapters": [
    { "interface": "lo", "ip_address": "127.0.0.1" },
    { "interface": "eth0", "ip_address": "192.168.1.10" },
    { "interface": "sit0", "ip_address": null }
  ]
}

---

## How Data is Collected

### Uptime
/proc/uptime

### CPU
# File: 
  /proc/cpuinfo  
# Fields:
  model name -> provide CPU model name
  cpu MHz -> provide CPU speed
# File: 

# Fields:
  /proc/stat
# Method
  Read at two different time points and calculate the usage percent based on idle time vs total time

### Memory
# File: 
  /proc/meminfo
# ields used:
  MemTotal
  MemFree
  Buffers
  Cached
# Calculation:
  used = total - free - buffers - cached
  
### OS Version
/proc/version

### Processes
/proc/[pid]/comm
# Method
  Iterated each PID directory and  acessed the comm file to get the process name

### Disks
# File:
  /proc/partitions
# Method:
  Iterate over partitions Ignoring loop and ram devices
# Conversion:
  Blocks → MB

### USB Devices
# File
  /sys/bus/usb/devices/
# Method: 
  iterate over the directory and acess the file 'product'  to get the gadjet name

### Network Interfaces
# Directory: 
  /sys/class/net/
# Method 
  Iterates over directories to get the network interfaces
# File: 
  /proc/net/fib_trie
# Method:
  Ignore: the address 127.0.0.1, 0.0.0.0 and 255.255.255.255 
Association
# File: 
  /proc/net/route

Only interfaces with active routes receive IPs
---

## Project Structure

custom-scripts/
├── pre-build.sh
├── post-build.sh
├── S41network-config
├── qemu-ifup
└── systeminfo.py

