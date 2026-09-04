#!/usr/bin/env python3

import fcntl
import json
import os
import socket
import struct
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from datetime import datetime

def get_datetime():
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def get_uptime():
    with open("/proc/uptime", "r") as file:
        info = file.read()
    uptime, _ = info.split()
    return int(float(uptime))

def _get_cpu_usage():
    with open("/proc/stat", "r") as file:
        l = file.readline()
    valores = [int(x) for x in l.split()[1:]]
    idle = valores[3] + valores[4]
    total = sum(valores)
    return idle, total

def get_cpu_info():
    model = "desconhecido"
    speed = 0.0

    with open("/proc/cpuinfo", "r") as file:
        for l in file:
            if l.startswith("model name") and model == "desconhecido":
                model = l.split(":")[1].strip()
            elif l.startswith("cpu MHz") and speed == 0.0:
                speed = float(l.split(":")[1].strip())
            if model != "desconhecido" and speed != 0.0:
                break
    idle1, total1 = _get_cpu_usage()
    time.sleep(1)
    idle2, total2 = _get_cpu_usage()

    delta_idle = idle2 - idle1
    delta_total = total2 - total1

    if delta_total == 0:
        return {
                "model": model,
                "speed_mhz": speed,
                "usage_percent": 0.0
                }

    else:
        uso = (1 - delta_idle / delta_total) * 100
    return {
        "model": model,
        "speed_mhz": speed,
        "usage_percent": round(uso, 2)
    }

def get_memory_info():
    total_kb = 0
    available_kb = 0

    with open("/proc/meminfo", "r") as file:
        for l in file:
            if l.startswith("MemTotal:"):
                total_kb = int(l.split()[1])
            elif l.startswith("MemAvailable:"):
                available_kb = int(l.split()[1])
            if total_kb and available_kb:
                break

    used_kb = total_kb - available_kb

    return {
        "total_mb": total_kb // 1024,
        "used_mb": used_kb // 1024
    }

def get_os_version():
    with open("/proc/version", "r") as file:
        version = file.read().strip()
    return version

def get_process_list():
    processos, pids = [], []
    for entrada in os.listdir("/proc"):
        if entrada.isdigit():
            pids.append(int(entrada))
    for pid in pids:
        with open(f"/proc/{pid}/comm", "r") as file:
            name = file.read().strip()
        processos.append({"pid": pid, "name": name})
    return processos

def get_disks():
    discos = []
    for nome in os.listdir("/sys/block"):
        if nome.startswith(("loop", "ram", "zram")):
            continue
        with open(f"/sys/block/{nome}/size", "r") as file:
            setores = int(file.read().strip())
        size_mb = (setores * 512) / (1024 * 1024)
        discos.append({"device": nome, "size_mb": int(size_mb)})
    return discos

def _description(caminho):
    try:
        with open(os.path.join(caminho, "product"), "r") as file:
            return file.read().strip()
    except FileNotFoundError:
        return "desconhecido"

def get_usb_devices():
    dispositivos = []
    for nome in os.listdir("/sys/bus/usb/devices"):
        if "-" in nome and ":" not in nome:
            caminho = f"/sys/bus/usb/devices/{nome}"
            dispositivos.append({
                "port": nome,
                "description": _description(caminho)
            })
    return dispositivos

def get_network_adapters():
    adaptadores = []
    for interface in os.listdir("/sys/class/net"):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            try:
                pacote = fcntl.ioctl(s.fileno(), 0x8915, struct.pack("256s", interface.encode()[:15]))
                adaptadores.append({"interface": interface, "ip_address": socket.inet_ntoa(pacote[20:24])})
            except OSError:
                pass
    return adaptadores

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