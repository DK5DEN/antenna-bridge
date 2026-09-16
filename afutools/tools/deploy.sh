#!/usr/bin/env bash
# Deploy the rig profile catalog to afu.tools. Run from the repo root:
#   bash afutools/tools/deploy.sh          # API module + routes + page + menu entry
#   bash afutools/tools/deploy.sh --site   # page and script only
set -euo pipefail
HOST=root@vmd174444.contaboserver.net
REMOTE=/opt/docker/afu.tools
STAMP=$(date +%Y%m%d-%H%M)
cd "$(dirname "$0")/.."

echo "== Seite und Skript"
scp site/antenna-bridge.html "$HOST:$REMOTE/site/"
scp site/js/antenna-bridge.js "$HOST:$REMOTE/site/js/"
ssh "$HOST" "cd $REMOTE/site && chown umzug:umzug antenna-bridge.html js/antenna-bridge.js"

if [ "${1:-}" != "--site" ]; then
  echo "== API: Modul, Adminseite, Routen (app.py und adminseite.py mit Backup)"
  scp api/rigprofile.py api/rigprofile_admin.py "$HOST:$REMOTE/api/"
  scp api/app_rig_patch.py "$HOST:/tmp/app_rig_patch.py"
  ssh "$HOST" "cd $REMOTE && cp api/app.py api/app.py.bak-$STAMP && cp api/adminseite.py api/adminseite.py.bak-$STAMP \
    && python3 /tmp/app_rig_patch.py api/app.py api/adminseite.py \
    && chown umzug:umzug api/rigprofile.py api/rigprofile_admin.py api/app.py api/adminseite.py \
    && docker compose build -q api && docker compose up -d api && sleep 4 \
    && docker compose logs --tail 5 api && curl -s -o /dev/null -w 'health %{http_code}\n' http://127.0.0.1/api/v1/health \
    && curl -s -o /dev/null -w 'rigs %{http_code}\n' http://127.0.0.1/api/v1/antenna-bridge/rigs"

  echo "== Menüpunkt in builder/navigation.py, Kopfzeile neu erzeugen"
  scp tools/patch-navigation.py "$HOST:/tmp/patch-navigation-ab.py"
  ssh "$HOST" "cd $REMOTE && cp builder/navigation.py builder/navigation.py.bak-$STAMP \
    && python3 /tmp/patch-navigation-ab.py builder/navigation.py \
    && docker compose build -q builder && docker compose up -d builder \
    && docker compose exec -T builder python3 /app/build.py --only navigation"
fi
echo "fertig: https://afu.tools/antenna-bridge"
