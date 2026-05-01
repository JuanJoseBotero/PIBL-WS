#!/bin/bash
# tws/tests/test_basic.sh

BASE="http://localhost:8081"
PASS=0
FAIL=0

ok()   { echo "PASO $1"; PASS=$((PASS+1)); }
fail() { echo "ERROR $1"; FAIL=$((FAIL+1)); }

# 200 GET html
curl -s -o /dev/null -w "%{http_code}" $BASE/index.html | grep -q "200" && ok "GET index.html 200" || fail "GET index.html 200"

# 404
curl -s -o /dev/null -w "%{http_code}" $BASE/noexiste.html | grep -q "404" && ok "GET noexiste 404" || fail "GET noexiste 404"

# HEAD sin body
BODY=$(curl -s -X HEAD $BASE/index.html)
[ -z "$BODY" ] && ok "HEAD sin body" || fail "HEAD sin body"

# HEAD con Content-Length
curl -s -I $BASE/index.html | grep -q "Content-Length" && ok "HEAD tiene Content-Length" || fail "HEAD tiene Content-Length"

# POST con body
RESP=$(curl -s -X POST $BASE/index.html -H "Content-Type: application/json" -d '{"test":"ok"}')
echo $RESP | grep -q "POST recibido" && ok "POST responde correctamente" || fail "POST responde correctamente"

# 400 — petición malformada
RESP=$(echo -e "INVALID REQUEST\r\n\r\n" | nc -q1 localhost 8081)
echo $RESP | grep -q "400" && ok "Petición inválida 400" || fail "Petición inválida 400"

# Path traversal bloqueado
curl -s -o /dev/null -w "%{http_code}" "$BASE/../../etc/passwd" | grep -qE "400|404" && ok "Path traversal bloqueado" || fail "Path traversal bloqueado"

# PNG — Content-Type correcto
curl -s -I $BASE/imagen.png | grep -q "image/png" && ok "PNG Content-Type correcto" || fail "PNG Content-Type correcto — ¿existe imagen.png en webapp?"

# Raíz sirve index.html
curl -s -o /dev/null -w "%{http_code}" $BASE/ | grep -q "200" && ok "Raíz sirve index.html" || fail "Raíz sirve index.html"

echo ""
echo "Resultado: $PASS pasaron, $FAIL fallaron"