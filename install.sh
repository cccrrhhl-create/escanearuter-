#!/bin/bash

# =============================================
# INSTALADOR - ESCANEARUTER v3.0
# Para Termux + Ubuntu (proot-distro)
# =============================================

VERDE='\033[0;32m'
AMARILLO='\033[1;33m'
ROJO='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}"
echo "╔════════════════════════════════════════════════════╗"
echo "║         INSTALADOR - ESCANEARUTER v3.0             ║"
echo "╚════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Verificar que estamos en Ubuntu/Debian
if ! command -v apt &> /dev/null; then
    echo -e "${ROJO}[!] Este instalador requiere apt (Ubuntu/Debian)${NC}"
    echo -e "${AMARILLO}    Ejecuta dentro de: proot-distro login ubuntu${NC}"
    exit 1
fi

echo -e "${AMARILLO}[*] Actualizando repositorios...${NC}"
apt update -y 2>/dev/null

echo -e "${AMARILLO}[*] Instalando dependencias...${NC}"
apt install -y \
    curl \
    iproute2 \
    iputils-ping \
    dnsutils \
    libc-bin \
    bash \
    git

echo -e "${AMARILLO}[*] Instalando escanearuter...${NC}"
mkdir -p ~/.local/bin
cp escanearuter ~/.local/bin/escanearuter
chmod +x ~/.local/bin/escanearuter

# Añadir al PATH si no está
if ! echo $PATH | grep -q ".local/bin"; then
    echo 'export PATH=$PATH:~/.local/bin' >> ~/.bashrc
    export PATH=$PATH:~/.local/bin
fi

echo -e "${VERDE}"
echo "╔════════════════════════════════════════════════════╗"
echo "║         ✅ INSTALACIÓN COMPLETADA                  ║"
echo "╚════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo -e "${VERDE}[+] Ejecuta con:${NC} escanearuter"
