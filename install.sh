#!/bin/bash

# =============================================
# INSTALADOR - ESCANEARUTER v3.0
# Compatible con Termux puro y Ubuntu (proot)
# =============================================

VERDE='\033[0;32m'
AMARILLO='\033[1;33m'
ROJO='\033[0;31m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m'

clear
echo -e "${CYAN}"
echo "╔════════════════════════════════════════════════════╗"
echo "║         INSTALADOR - ESCANEARUTER v3.0             ║"
echo "╚════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Detectar entorno
if [ -d "/data/data/com.termux" ] && ! grep -q "ubuntu" /etc/os-release 2>/dev/null; then
    ENTORNO="termux"
else
    ENTORNO="ubuntu"
fi

echo -e "${AMARILLO}[*] Entorno detectado: $ENTORNO${NC}\n"

# Elegir versión
echo -e "${CYAN}¿Qué versión deseas instalar?${NC}"
echo -e "1) 📱 Termux puro ${AMARILLO}(sin Ubuntu, más simple)${NC}"
echo -e "2) 🐧 Ubuntu/proot ${AMARILLO}(más herramientas disponibles)${NC}"
echo -e "3) ⚡ C++ ${AMARILLO}(más rápido, requiere compilación)${NC}"
echo ""
echo -n -e "${VERDE}Elige [1-3]: ${NC}"
read VERSION

case $VERSION in
    1)
        SCRIPT="escanearuter-termux"
        NOMBRE="escanearuter"
        echo -e "\n${CYAN}[*] Instalando versión Termux puro...${NC}"

        if [ "$ENTORNO" != "termux" ]; then
            echo -e "${AMARILLO}[!] Advertencia: estás en Ubuntu pero instalando versión Termux${NC}"
        fi

        echo -e "${AMARILLO}[*] Instalando dependencias Termux...${NC}"
        pkg update -y 2>/dev/null
        pkg install -y curl iproute2 dnsutils bash 2>/dev/null

        if [ ! -f "$SCRIPT" ]; then
            echo -e "${ROJO}[!] No se encontró el archivo: $SCRIPT${NC}"
            exit 1
        fi

        mkdir -p ~/.local/bin
        cp $SCRIPT ~/.local/bin/$NOMBRE
        chmod +x ~/.local/bin/$NOMBRE
        ;;
    2)
        SCRIPT="escanearuter"
        NOMBRE="escanearuter"
        echo -e "\n${CYAN}[*] Instalando versión Ubuntu/proot...${NC}"

        if [ "$ENTORNO" != "ubuntu" ]; then
            echo -e "${AMARILLO}[!] Advertencia: ejecuta dentro de: proot-distro login ubuntu${NC}"
        fi

        echo -e "${AMARILLO}[*] Instalando dependencias Ubuntu...${NC}"
        apt update -y 2>/dev/null
        apt install -y curl iproute2 iputils-ping dnsutils libc-bin bash git 2>/dev/null

        if [ ! -f "$SCRIPT" ]; then
            echo -e "${ROJO}[!] No se encontró el archivo: $SCRIPT${NC}"
            exit 1
        fi

        mkdir -p ~/.local/bin
        cp $SCRIPT ~/.local/bin/$NOMBRE
        chmod +x ~/.local/bin/$NOMBRE
        ;;
    3)
        echo -e "\n${CYAN}[*] Instalando versión C++...${NC}"

        if [ "$ENTORNO" != "ubuntu" ]; then
            echo -e "${AMARILLO}[!] La versión C++ requiere Ubuntu (proot-distro login ubuntu)${NC}"
            exit 1
        fi

        if [ ! -f "escanearuterCpp.cpp" ]; then
            echo -e "${ROJO}[!] No se encontró escanearuterCpp.cpp${NC}"
            exit 1
        fi

        echo -e "${AMARILLO}[*] Instalando dependencias C++...${NC}"
        apt update -y 2>/dev/null
        apt install -y g++ iputils-ping iproute2 dnsutils 2>/dev/null

        echo -e "${AMARILLO}[*] Compilando...${NC}"
        g++ -o escanearuterCpp escanearuterCpp.cpp -lpthread

        if [ $? -ne 0 ]; then
            echo -e "${ROJO}[!] Error al compilar${NC}"
            exit 1
        fi

        mkdir -p ~/.local/bin
        cp escanearuterCpp ~/.local/bin/escanearuterCpp
        chmod +x ~/.local/bin/escanearuterCpp

        echo -e "${VERDE}"
        echo "╔════════════════════════════════════════════════════╗"
        echo "║         ✅ INSTALACIÓN COMPLETADA                  ║"
        echo "╚════════════════════════════════════════════════════╝"
        echo -e "${NC}"
        echo -e "${VERDE}[+] Versión instalada:${NC} C++"
        echo -e "${VERDE}[+] Ejecuta con:${NC} escanearuterCpp"
        echo -e "${AMARILLO}[!] Si no funciona:${NC} bash ~/.local/bin/escanearuterCpp"
        exit 0
        ;;
    *)
        echo -e "${ROJO}[!] Opción no válida${NC}"
        exit 1
        ;;
esac

# Añadir al PATH
if ! echo $PATH | grep -q ".local/bin"; then
    echo 'export PATH=$PATH:~/.local/bin' >> ~/.bashrc
    export PATH=$PATH:~/.local/bin
fi

echo -e "${VERDE}"
echo "╔════════════════════════════════════════════════════╗"
echo "║         ✅ INSTALACIÓN COMPLETADA                  ║"
echo "╚════════════════════════════════════════════════════╝"
echo -e "${NC}"

if [ "$VERSION" = "1" ]; then
    echo -e "${VERDE}[+] Versión instalada:${NC} Termux puro"
else
    echo -e "${VERDE}[+] Versión instalada:${NC} Ubuntu/proot"
fi

echo -e "${VERDE}[+] Ejecuta con:${NC} escanearuter"
echo -e "${AMARILLO}[!] Si no funciona:${NC} bash ~/.local/bin/escanearuter"
