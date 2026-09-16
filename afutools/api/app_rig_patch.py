# Adds the rig profile catalog routes to api/app.py and the navigation entry
# to api/adminseite.py on the server (idempotent).
# Usage: python3 app_rig_patch.py /opt/docker/afu.tools/api/app.py /opt/docker/afu.tools/api/adminseite.py
import io
import sys

app_path, admin_path = sys.argv[1], sys.argv[2]

# ------------------------------------------------------------- adminseite.py
s = io.open(admin_path, encoding="utf-8").read()
if '"/admin/rigs"' not in s:
    old = '    ("/admin/geraete", "Geräte"),\n'
    assert old in s, "adminseite: Geräte entry not found"
    s = s.replace(old, old + '    ("/admin/rigs", "Rig-Profile"),\n', 1)
    io.open(admin_path, "w", encoding="utf-8", newline="\n").write(s)
    print("adminseite.py: Rig-Profile entry added")
else:
    print("adminseite.py: Rig-Profile entry already present")

# -------------------------------------------------------------------- app.py
s = io.open(app_path, encoding="utf-8").read()

# The bridge's own web page (served by the ESP32, any LAN address) reads the
# catalog: let GET requests to it through without an API key, the same way
# feeds are let through. Everything else on the API keeps its rules.
OPEN_OLD = '    if not pfad.startswith(BASE) or pfad in OFFEN or request.method == "OPTIONS":\n'
OPEN_NEW = ('    if not pfad.startswith(BASE) or pfad in OFFEN or request.method == "OPTIONS" \\\n'
            '            or (request.method == "GET" and pfad.startswith(BASE + "/antenna-bridge/")):\n')
if OPEN_NEW not in s:
    assert OPEN_OLD in s, "middleware anchor not found"
    s = s.replace(OPEN_OLD, OPEN_NEW, 1)
    io.open(app_path, "w", encoding="utf-8", newline="\n").write(s)
    print("app.py: catalog GET opened")

if "rigprofile" in s:
    print("app.py: rig profile routes already present")
    sys.exit(0)

IMPORT_AFTER = "import js8spots\n"
assert IMPORT_AFTER in s, "import anchor not found"
s = s.replace(IMPORT_AFTER, IMPORT_AFTER + "import rigprofile\nimport rigprofile_admin\n", 1)

INIT_AFTER = "messungsablage.init()\n"
assert INIT_AFTER in s, "init anchor not found"
s = s.replace(INIT_AFTER, INIT_AFTER + "rigprofile.init()\n", 1)

ROUTES = '''

# ------------------------------------------------------- Rig-Profile (antenna-bridge)
#
# Katalog der CAT-Profile fuer die Antenna Bridge (github.com/DK5DEN/antenna-bridge).
# Einreichen darf jeder, sichtbar fuer alle wird ein Profil erst nach der
# Freigabe unter /admin/rigs. Wem ein Eintrag gehoert, entscheidet die
# Melderkennung aus dem Browser, wie bei den Oberwellen-Geraeten.

RATE_RIGS = int(os.environ.get("AFU_RATE_RIGS", "6"))


class RigProfilIn(BaseModel):
    melder: str
    rufzeichen: str = Field(default="", max_length=20)
    profile: dict


def _rig_fehler(fehler):
    return HTTPException(status_code=fehler.status, detail=fehler.text)


@app.get(BASE + "/antenna-bridge/rigs", include_in_schema=False)
def rig_liste(request: Request, melder: str = Query("", max_length=120)):
    limit(request, "data", RATE_DEFAULT)
    return {"rigs": rigprofile.liste(melder)}


@app.get(BASE + "/antenna-bridge/rigs/{pid}", include_in_schema=False)
def rig_eines(request: Request, pid: str, melder: str = Query("", max_length=120)):
    limit(request, "data", RATE_DEFAULT)
    try:
        return rigprofile.eine(pid, melder)
    except rigprofile.Fehler as fehler:
        raise _rig_fehler(fehler) from fehler


@app.post(BASE + "/antenna-bridge/rigs", include_in_schema=False)
def rig_einreichen(request: Request, data: RigProfilIn):
    limit(request, "rigs", RATE_RIGS)
    try:
        return rigprofile.speichere(data.melder, data.rufzeichen, data.profile)
    except rigprofile.Fehler as fehler:
        raise _rig_fehler(fehler) from fehler


@app.delete(BASE + "/antenna-bridge/rigs/{pid}", include_in_schema=False)
def rig_loeschen(request: Request, pid: str, melder: str = Query("", max_length=120)):
    limit(request, "rigs", RATE_RIGS)
    try:
        rigprofile.loesche(pid, melder)
    except rigprofile.Fehler as fehler:
        raise _rig_fehler(fehler) from fehler
    return {"ok": True}


@app.get("/admin/rigs", include_in_schema=False)
def admin_rigs(request: Request):
    benutzer = admin_benutzer(request)
    return HTMLResponse(rigprofile_admin.liste(
        benutzer=benutzer, profile=rigprofile.verwaltung_liste(), **_melde(request)))


@app.get("/admin/rigs/{pid}", include_in_schema=False)
def admin_rig(request: Request, pid: str):
    benutzer = admin_benutzer(request)
    try:
        text = rigprofile.verwaltung_dokument(pid)
    except rigprofile.Fehler as fehler:
        return _zurueck("/admin/rigs", "fehler", fehler.text)
    return HTMLResponse(rigprofile_admin.dokument(benutzer=benutzer, pid=pid, text=text, **_melde(request)))


@app.post("/admin/rigs/{pid}/{aktion}", include_in_schema=False)
def admin_rig_aktion(request: Request, pid: str, aktion: str):
    benutzer = admin_benutzer(request)
    try:
        if aktion == "loeschen":
            rigprofile.verwaltung_loeschen(pid)
            text = f"Profil {pid} gelöscht."
        else:
            rigprofile.verwaltung_status(pid, aktion, benutzer)
            text = {"frei": f"Profil {pid} steht jetzt im Katalog.",
                    "neu": f"Profil {pid} wartet wieder.",
                    "abgelehnt": f"Profil {pid} ist ausgeblendet."}.get(aktion, "Erledigt.")
    except rigprofile.Fehler as fehler:
        return _zurueck("/admin/rigs", "fehler", fehler.text)
    return _zurueck("/admin/rigs", "ok", text)
'''

ANCHOR = "\n\n# ------------------------------------------------------- Oberwellenmessung"
assert ANCHOR in s, "Oberwellen section anchor not found"
s = s.replace(ANCHOR, ROUTES + ANCHOR, 1)
io.open(app_path, "w", encoding="utf-8", newline="\n").write(s)
print("app.py: rig profile routes added")
