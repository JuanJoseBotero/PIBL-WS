#!/bin/bash
# Setup script para instancias backend (TWS)
# Ejecutar dentro de cada instancia backend

set -e

REPO_URL="https://github.com/JuanJoseBotero/PIBL-WS.git"
TWS_PORT=8081
DOCUMENT_ROOT="/home/ubuntu/www"
LOG_FILE="/home/ubuntu/tws.log"

echo "=== Setup Backend TWS ==="

# Instalar dependencias
echo "[1/5] Instalando dependencias..."
sudo apt update -y
sudo apt install -y git gcc make screen

# Clonar repositorio
echo "[2/5] Clonando repositorio..."
if [ -d "$HOME/PIBL-WS" ]; then
    echo "Repositorio ya existe, actualizando..."
    cd "$HOME/PIBL-WS" && git pull origin main
else
    git clone "$REPO_URL" "$HOME/PIBL-WS"
fi

# Compilar TWS
echo "[3/5] Compilando TWS..."
cd "$HOME/PIBL-WS/tws"
gcc -Wall -Wextra -pthread -g -o tws \
    src/main.c src/server.c src/http_parser.c src/logger.c

# Crear DocumentRoot y recursos de prueba
echo "[4/5] Creando recursos web..."
mkdir -p "$DOCUMENT_ROOT"

cat > "$DOCUMENT_ROOT/index.html" << 'EOF'
<html><body><h1>Hola desde TWS</h1></body></html>
EOF

cat > "$DOCUMENT_ROOT/caso1.html" << 'EOF'
<!DOCTYPE html>
<html>
<head><title>Caso 1 - TWS</title></head>
<body>
  <h1>Caso 1: Página con hipertextos e imagen</h1>
  <p><a href="/caso2.html">Ir al Caso 2</a></p>
  <p><a href="/archivo.bin">Descargar archivo</a></p>
  <img src="/foto.jpg" alt="imagen de prueba">
</body>
</html>
EOF

cat > "$DOCUMENT_ROOT/caso2.html" << 'EOF'
<!DOCTYPE html>
<html>
<head><title>Caso 2 - TWS</title></head>
<body>
  <h1>Caso 2: Múltiples imágenes</h1>
  <img src="/img1.jpg" alt="imagen 1">
  <img src="/img2.jpg" alt="imagen 2">
  <img src="/img3.jpg" alt="imagen 3">
  <img src="/img4.jpg" alt="imagen 4">
</body>
</html>
EOF

cat > "$DOCUMENT_ROOT/caso4.html" << 'EOF'
<!DOCTYPE html>
<html>
<head><title>Caso 4 - TWS</title></head>
<body>
  <h1>Caso 4: Múltiples archivos ~1MB total</h1>
  <a href="/parte1.bin">Parte 1</a>
  <a href="/parte2.bin">Parte 2</a>
  <a href="/parte3.bin">Parte 3</a>
  <a href="/parte4.bin">Parte 4</a>
</body>
</html>
EOF

dd if=/dev/urandom bs=1K count=50  > "$DOCUMENT_ROOT/foto.jpg"
dd if=/dev/urandom bs=1K count=100 > "$DOCUMENT_ROOT/img1.jpg"
dd if=/dev/urandom bs=1K count=100 > "$DOCUMENT_ROOT/img2.jpg"
dd if=/dev/urandom bs=1K count=100 > "$DOCUMENT_ROOT/img3.jpg"
dd if=/dev/urandom bs=1K count=100 > "$DOCUMENT_ROOT/img4.jpg"
dd if=/dev/urandom bs=1M count=1   > "$DOCUMENT_ROOT/archivo.bin"
dd if=/dev/urandom bs=1K count=256 > "$DOCUMENT_ROOT/parte1.bin"
dd if=/dev/urandom bs=1K count=256 > "$DOCUMENT_ROOT/parte2.bin"
dd if=/dev/urandom bs=1K count=256 > "$DOCUMENT_ROOT/parte3.bin"
dd if=/dev/urandom bs=1K count=256 > "$DOCUMENT_ROOT/parte4.bin"

# Lanzar TWS
echo "[5/5] Lanzando TWS..."
pkill tws 2>/dev/null || true
sleep 1
screen -dmS tws ./tws --port "$TWS_PORT" --root "$DOCUMENT_ROOT"

echo ""
echo "=== Setup completo ==="
echo "TWS corriendo en puerto $TWS_PORT"
echo "DocumentRoot: $DOCUMENT_ROOT"
ps aux | grep tws | grep -v grep