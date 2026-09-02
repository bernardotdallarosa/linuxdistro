#!/usr/bin/env python3

import fcntl
import json
import os
import re
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
    with open("/proc/cpuinfo", "r") as file:
        for l in file:
            if l.startswith("model name"):
                model = l.split(":")[1].strip()
            elif l.startswith("cpu MHz"):
                speed = float(l.split(":")[1].strip())
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
    valores = {}
    with open("/proc/meminfo", "r") as file:
        for linha in file:
            chave, resto = linha.split(":", 1)
            numero_kb = int(resto.strip().split()[0])
            valores[chave] = numero_kb

    total_kb = valores["MemTotal"]
    available_kb = valores.get("MemAvailable", valores["MemFree"])
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
    with open("/proc/partitions", "r") as file:
        linhas = file.readlines()[2:]
        for linha in linhas:
            partes = linha.split()
            if len(partes) != 4:
                continue
            _, _, blocos, nome = partes
            size_mb = int(blocos) / 1024
            discos.append({"device": nome, "size_mb": round(size_mb, 2)})
    return discos

_PADRAO_PORTA = re.compile(r"^\d+-[\d.]+$")

def get_usb_devices() -> list[dict]:
    base = "/sys/bus/usb/devices"
    dispositivos = []

    for nome in os.listdir(base):
        if not _PADRAO_PORTA.match(nome):
            continue  # ignora usb1, usb2 (root hubs) e outras entradas (ex: 1-1:1.0, interfaces)

        caminho = os.path.join(base, nome)
        descricao = _obter_descricao(caminho)

        dispositivos.append({
            "port": nome,
            "description": descricao,
        })

    return dispositivos


def _obter_descricao(caminho: str) -> str:
    # 1ª tentativa: campo "product" (string legível, ex: "USB Optical Mouse")
    try:
        with open(os.path.join(caminho, "product"), "r") as file:
            return file.read().strip()
    except FileNotFoundError:
        pass

    # fallback: monta descrição a partir de vendor/product ID em hex
    try:
        with open(os.path.join(caminho, "idVendor"), "r") as file:
            vendor = file.read().strip()
        with open(os.path.join(caminho, "idProduct"), "r") as file:
            product = file.read().strip()
        return f"Vendor {vendor}:Product {product}"
    except FileNotFoundError:
        return "desconhecido"

def _listar_interfaces() -> list[str]:
    return os.listdir("/sys/class/net")

SIOCGIFADDR = 0x8915  # constante do kernel Linux para "get interface address"

def _obter_ip(interface: str) -> str | None:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        pacote = fcntl.ioctl(
            s.fileno(),
            SIOCGIFADDR,
            struct.pack("256s", interface.encode("utf-8")[:15])
        )
        return socket.inet_ntoa(pacote[20:24])
    except OSError:
        return None  # interface sem IPv4 (down, ou só IPv6)
    finally:
        s.close()

def get_network_adapters() -> list[dict]:
    adaptadores = []
    for interface in _listar_interfaces():
        ip = _obter_ip(interface)
        if ip is not None:
            adaptadores.append({
                "interface": interface,
                "ip_address": ip,
            })
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