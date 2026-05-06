#!/bin/bash
# Setup script para la instancia PIBL
# Ejecutar dentro de la instancia PIBL

set -e

REPO_URL="https://github.com/JuanJoseBotero/PIBL-WS.git"
CONFIG_FILE="config-pibl.json"

echo "=== Setup PIBL ==="

# Instalar dependencias
echo "[1/4] Instalando dependencias..."
sudo apt update -y
sudo apt install -y git gcc make screen

# Clonar repositorio
echo "[2/4] Clonando repositorio..."
if [ -d "$HOME/PIBL-WS" ]; then
    echo "Repositorio ya existe, actualizando..."
    cd "$HOME/PIBL-WS" && git pull origin main
else
    git clone "$REPO_URL" "$HOME/PIBL-WS"
fi

# Compilar PIBL
echo "[3/4] Compilando PIBL..."
cd "$HOME/PIBL-WS/pibl"
make

# Crear directorio de caché
mkdir -p cache_storage

# Lanzar PIBL
echo "[4/4] Lanzando PIBL..."
pkill pibl 2>/dev/null || true
sleep 1
screen -dmS pibl ./pibl "$CONFIG_FILE"

echo ""
echo "=== Setup completo ==="
echo "PIBL corriendo con configuración: $CONFIG_FILE"
ps aux | grep pibl | grep -v grep