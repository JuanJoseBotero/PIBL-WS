# cd ~/PIBL-WS/tws
# chmod +x tests/test_completo.sh
# bash tests/test_completo.sh

BASE="http://localhost:8081"
PASS=0
FAIL=0

ok()   { echo "[PASO] $1"; PASS=$((PASS+1)); }
fail() { echo "[FALLO] $1"; FAIL=$((FAIL+1)); }

echo "================================================"
echo " TWS — Test Completo"
echo "================================================"

# ─── Conectividad básica ───────────────────
echo ""
echo "--- Conectividad ---"

curl -s -o /dev/null -w "%{http_code}" $BASE/index.html | grep -q "200" \
    && ok "Servidor responde" || fail "Servidor no responde"

# ─── Parser HTTP ───────────────────────────
echo ""
echo "--- Parser ---"

# Petición malformada
RESP=$(echo -e "ESTO NO ES HTTP\r\n\r\n" | nc -q1 localhost 8081 2>/dev/null)
echo "$RESP" | grep -q "400" \
    && ok "Petición malformada → 400" || fail "Petición malformada → 400"

# Path traversal
curl -s -o /dev/null -w "%{http_code}" "$BASE/../../etc/passwd" | grep -qE "400|404" \
    && ok "Path traversal bloqueado" || fail "Path traversal no bloqueado"

# Versión HTTP inválida
RESP=$(echo -e "GET / HTTP/9.9\r\n\r\n" | nc -q1 localhost 8081 2>/dev/null)
echo "$RESP" | grep -q "400" \
    && ok "Versión HTTP inválida → 400" || fail "Versión HTTP inválida → 400"

# ─── Métodos y códigos ─────────────────────
echo ""
echo "--- Métodos y códigos ---"

# GET 200
curl -s -o /dev/null -w "%{http_code}" $BASE/index.html | grep -q "200" \
    && ok "GET archivo existe → 200" || fail "GET archivo existe → 200"

# GET 404
curl -s -o /dev/null -w "%{http_code}" $BASE/noexiste.html | grep -q "404" \
    && ok "GET archivo no existe → 404" || fail "GET archivo no existe → 404"

# GET raíz sirve index.html
curl -s -o /dev/null -w "%{http_code}" $BASE/ | grep -q "200" \
    && ok "GET raíz → index.html" || fail "GET raíz → index.html"

# HEAD sin body
BODY=$(curl -s -X HEAD $BASE/index.html)
[ -z "$BODY" ] \
    && ok "HEAD sin body en respuesta" || fail "HEAD tiene body — no debería"

# HEAD tiene Content-Length
curl -s -I $BASE/index.html | grep -q "Content-Length" \
    && ok "HEAD tiene Content-Length" || fail "HEAD sin Content-Length"

# HEAD archivo no existe → 404
curl -s -o /dev/null -w "%{http_code}" -I $BASE/noexiste.html | grep -q "404" \
    && ok "HEAD archivo no existe → 404" || fail "HEAD archivo no existe → 404"

# POST
RESP=$(curl -s -X POST $BASE/index.html \
    -H "Content-Type: application/json" \
    -d '{"test":"ok"}')
echo "$RESP" | grep -q "POST recibido" \
    && ok "POST → respuesta correcta" || fail "POST → respuesta incorrecta"

# Content-Type HTML
curl -s -I $BASE/index.html | grep -q "text/html" \
    && ok "Content-Type text/html correcto" || fail "Content-Type text/html incorrecto"

# Content-Type PNG
if [ -f webapp/imagen.png ]; then
    curl -s -I $BASE/imagen.png | grep -q "image/png" \
        && ok "Content-Type image/png correcto" || fail "Content-Type image/png incorrecto"
else
    echo "imagen.png no existe — saltando test de Content-Type PNG"
fi

# Archivo grande 1MB
if [ -f webapp/archivo_grande.bin ]; then
    curl -s -o /dev/null -w "%{http_code}" $BASE/archivo_grande.bin | grep -q "200" \
        && ok "GET archivo 1MB → 200" || fail "GET archivo 1MB → 200"
else
    echo "archivo_grande.bin no existe — créalo con:"
    echo "dd if=/dev/urandom of=webapp/archivo_grande.bin bs=1M count=1"
fi

# ─── Concurrencia y logging ────────────────
echo ""
echo "--- Concurrencia y logging ---"

# 10 clientes simultáneos
for i in $(seq 1 10); do
    curl -s -o /dev/null $BASE/index.html &
done
wait
ok "10 clientes simultáneos sin crash"

# Verificar que el log se generó
if [ -f tws.log ]; then
    LINES=$(wc -l < tws.log)
    [ "$LINES" -gt 0 ] \
        && ok "tws.log generado con $LINES líneas" || fail "tws.log está vacío"
else
    fail "tws.log no existe"
fi

# Verificar formato del log
if [ -f tws.log ]; then
    tail -1 tws.log | grep -qE "\[[0-9]{4}-[0-9]{2}-[0-9]{2}" \
        && ok "Formato de log correcto" || fail "Formato de log incorrecto"
fi

# Archivo grande con cliente simultáneo
if [ -f webapp/archivo_grande.bin ]; then
    curl -s -o /dev/null $BASE/archivo_grande.bin &
    curl -s -o /dev/null -w "%{http_code}" $BASE/index.html | grep -q "200" \
        && ok "Concurrencia con archivo grande" || fail "Concurrencia con archivo grande"
    wait
fi

# ─── Resumen ──────────────────────────────────────────
echo ""
echo "================================================"
echo " Resultado: $PASS pasaron — $FAIL fallaron"
echo "================================================"