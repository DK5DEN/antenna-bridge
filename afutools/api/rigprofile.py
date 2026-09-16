"""Catalog of rig profiles for the antenna-bridge (https://github.com/DK5DEN/antenna-bridge).

A rig profile is a small JSON document that tells the bridge how to talk CAT
to a radio: protocol family, baud rate, which answers carry the frequency,
how the jack is wired. The bridge ships a few built in; this catalog lets
people share their own so the next owner of the same radio does not have to
work it out again.

There are no accounts on afu.tools. Ownership works like the harmonics
device list: the browser keeps a random reporter id (`melder`,
`afu.tools:melder` in localStorage) and sends it along. A submitted profile
is visible to its submitter at once and to everyone else after the
administration released it (`status` neu -> frei | abgelehnt). Updating a
released profile puts it back to `neu`.

The document is validated with the same rules the firmware applies, stored
as text and handed out unchanged.
"""
import json
import os
import re
import sqlite3
import threading
from datetime import datetime, timezone

DB_PATH = os.environ.get("AFU_RIGPROFILE_DB", "/private/rigprofile.db")

KENNUNG = re.compile(r"^[A-Za-z0-9_-]{22,120}$")
RUFZEICHEN = re.compile(r"^[A-Z0-9]{1,4}(/[A-Z0-9]{1,4})?[0-9][A-Z0-9]{0,4}(/[A-Z0-9]{1,4})?$")
PROFIL_ID = re.compile(r"^[A-Za-z0-9_-]{2,23}$")
HEX = re.compile(r"^([0-9A-Fa-f]{2})+$")

STATUS = ("neu", "frei", "abgelehnt")
FAMILIEN = ("ascii", "civ", "none")
# ids of the profiles built into the firmware; a shared profile needs its own id
RESERVIERT = {"ftx1", "ftdx10", "ft891", "kenwood", "elecraft", "icom", "network"}

DOKUMENT_GRENZE = 4000      # bytes, the firmware refuses larger documents as well

_lock = threading.Lock()

SCHEMA = """
CREATE TABLE IF NOT EXISTS profil (
    id             TEXT    PRIMARY KEY,
    name           TEXT    NOT NULL,
    family         TEXT    NOT NULL,
    baud           INTEGER NOT NULL,
    wiring         TEXT,
    rufzeichen     TEXT,
    melder         TEXT    NOT NULL,
    status         TEXT    NOT NULL DEFAULT 'neu',
    version        INTEGER NOT NULL DEFAULT 1,
    dokument       TEXT    NOT NULL,
    hinweis        TEXT,
    created_at     TEXT    NOT NULL,
    updated_at     TEXT    NOT NULL,
    entschieden_at TEXT,
    entschieden_von TEXT
);
CREATE INDEX IF NOT EXISTS idx_profil_melder ON profil (melder);
"""


class Fehler(Exception):
    def __init__(self, status: int, text: str):
        super().__init__(text)
        self.status = status
        self.text = text


def _jetzt() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def _db():
    con = sqlite3.connect(DB_PATH, timeout=10)
    con.row_factory = sqlite3.Row
    con.execute("PRAGMA journal_mode=WAL")
    return con


def init() -> None:
    ordner = os.path.dirname(DB_PATH)
    if ordner:
        os.makedirs(ordner, exist_ok=True)
    with _lock, _db() as con:
        con.executescript(SCHEMA)


# ---------------------------------------------------------------- validation

def _spec(v, feld):
    if v is None:
        return
    if not isinstance(v, dict):
        raise Fehler(400, f"{feld}: object expected")
    prefix = v.get("prefix")
    if not isinstance(prefix, str) or not 1 <= len(prefix) <= 3:
        raise Fehler(400, f"{feld}.prefix: 1..3 characters")
    for k in ("skip", "digits"):
        if k in v and not (isinstance(v[k], int) and 0 <= v[k] <= 40):
            raise Fehler(400, f"{feld}.{k}: 0..40")


def _text(doc, feld, maxlen, pflicht=False):
    v = doc.get(feld)
    if v is None:
        if pflicht:
            raise Fehler(400, f"{feld} missing")
        return ""
    if not isinstance(v, str) or len(v) > maxlen:
        raise Fehler(400, f"{feld}: text up to {maxlen} characters")
    if pflicht and not v.strip():
        raise Fehler(400, f"{feld} missing")
    return v


def pruefe(doc: dict) -> dict:
    """Validate a profile document, return the normalised document."""
    if not isinstance(doc, dict):
        raise Fehler(400, "profile: object expected")
    pid = doc.get("id")
    if not isinstance(pid, str) or not PROFIL_ID.match(pid):
        raise Fehler(400, "id: 2..23 characters a-z 0-9 - _")
    pid = pid.lower()
    if pid in RESERVIERT:
        raise Fehler(400, f"id {pid} is a built-in profile, choose your own id")
    name = _text(doc, "name", 47, pflicht=True)
    family = doc.get("family")
    if family not in FAMILIEN:
        raise Fehler(400, "family: ascii, civ or none")
    baud = doc.get("baud", 38400)
    if not isinstance(baud, int) or not 1200 <= baud <= 115200:
        raise Fehler(400, "baud: 1200..115200")
    invert = bool(doc.get("invert", False))
    wiring = _text(doc, "wiring", 600)
    author = _text(doc, "author", 20)
    notes = _text(doc, "notes", 600)
    version = doc.get("version", 1)
    if not isinstance(version, int) or not 1 <= version <= 1000:
        raise Fehler(400, "version: 1..1000")

    out = {"id": pid, "name": name.strip(), "author": author.strip(), "version": version,
           "family": family, "baud": baud, "invert": invert, "wiring": wiring.strip()}
    if notes.strip():
        out["notes"] = notes.strip()

    if family == "ascii":
        a = doc.get("ascii")
        if not isinstance(a, dict):
            raise Fehler(400, "ascii section missing")
        term = a.get("term", ";")
        if not isinstance(term, str) or len(term) != 1:
            raise Fehler(400, "ascii.term: one character")
        poll = _text(a, "poll", 39)
        init = _text(a, "init", 23)
        every = a.get("initEvery", 10)
        if not isinstance(every, int) or not 0 <= every <= 3600:
            raise Fehler(400, "ascii.initEvery: 0..3600 s")
        if not isinstance(a.get("main"), dict):
            raise Fehler(400, "ascii.main missing")
        _spec(a.get("main"), "ascii.main")
        _spec(a.get("sub"), "ascii.sub")
        info = a.get("info", [])
        if not isinstance(info, list) or len(info) > 4:
            raise Fehler(400, "ascii.info: up to 4 entries")
        for i, e in enumerate(info):
            _spec(e, f"ascii.info[{i}]")
            if e.get("to", "main") not in ("main", "sub"):
                raise Fehler(400, f"ascii.info[{i}].to: main or sub")
        tx = a.get("tx")
        if tx is not None:
            if not isinstance(tx, dict) or not isinstance(tx.get("prefix", ""), str) \
                    or len(tx.get("prefix", "")) > 3 or len(str(tx.get("sub", "1"))) > 3:
                raise Fehler(400, "ascii.tx: prefix up to 3 characters, sub up to 3")
        out["ascii"] = {"term": term, "poll": poll, "init": init, "initEvery": every,
                        "main": a["main"]}
        if a.get("sub"):
            out["ascii"]["sub"] = a["sub"]
        if info:
            out["ascii"]["info"] = info
        if tx:
            out["ascii"]["tx"] = {"prefix": tx.get("prefix", ""), "sub": str(tx.get("sub", "1"))}
    elif family == "civ":
        c = doc.get("civ")
        if not isinstance(c, dict):
            raise Fehler(400, "civ section missing")
        addr = str(c.get("addr", "a4")).lower()
        if not re.match(r"^[0-9a-f]{2}$", addr):
            raise Fehler(400, "civ.addr: two hex digits")
        poll = c.get("poll", [])
        if not isinstance(poll, list) or len(poll) > 4 or not all(isinstance(p, str) and HEX.match(p) and len(p) <= 8 for p in poll):
            raise Fehler(400, "civ.poll: up to 4 hex strings")
        out["civ"] = {"addr": addr, "poll": poll}
        for k in ("main", "sub", "split", "transceive"):
            if k in c:
                if not isinstance(c[k], str) or not HEX.match(c[k]) or len(c[k]) > 8:
                    raise Fehler(400, f"civ.{k}: hex bytes")
                out["civ"][k] = c[k]
    text = json.dumps(out, ensure_ascii=False, separators=(",", ":"))
    if len(text.encode("utf-8")) > DOKUMENT_GRENZE:
        raise Fehler(400, f"document larger than {DOKUMENT_GRENZE} bytes")
    return out


def _melder(melder: str) -> str:
    melder = (melder or "").strip()
    if not KENNUNG.match(melder):
        raise Fehler(400, "reporter id missing or invalid")
    return melder


def _rufzeichen(call: str) -> str:
    call = (call or "").strip().upper()
    if call and not RUFZEICHEN.match(call):
        raise Fehler(400, "callsign: letters, digits, optional prefix/suffix")
    return call


# ---------------------------------------------------------------- public api

def _zeile_json(z, melder: str = "") -> dict:
    return {
        "id": z["id"], "name": z["name"], "family": z["family"], "baud": z["baud"],
        "wiring": z["wiring"] or "", "rufzeichen": z["rufzeichen"] or "",
        "status": z["status"], "version": z["version"],
        "updated": z["updated_at"], "eigen": bool(melder) and z["melder"] == melder,
    }


def liste(melder: str = "") -> list[dict]:
    """Released profiles plus the submitter's own pending ones."""
    melder = (melder or "").strip()
    with _lock, _db() as con:
        zeilen = con.execute(
            "SELECT * FROM profil WHERE status = 'frei' OR (melder = ? AND ? != '') "
            "ORDER BY name COLLATE NOCASE", (melder, melder)).fetchall()
    return [_zeile_json(z, melder) for z in zeilen]


def eine(pid: str, melder: str = "") -> dict:
    melder = (melder or "").strip()
    with _lock, _db() as con:
        z = con.execute("SELECT * FROM profil WHERE id = ?", (pid.lower(),)).fetchone()
    if not z or (z["status"] != "frei" and not (melder and z["melder"] == melder)):
        raise Fehler(404, "no such profile")
    return {"meta": _zeile_json(z, melder), "profile": json.loads(z["dokument"])}


def speichere(melder: str, rufzeichen: str, doc: dict) -> dict:
    melder = _melder(melder)
    call = _rufzeichen(rufzeichen)
    norm = pruefe(doc)
    if call and not norm.get("author"):
        norm["author"] = call
    text = json.dumps(norm, ensure_ascii=False, separators=(",", ":"))
    jetzt = _jetzt()
    with _lock, _db() as con:
        alt = con.execute("SELECT melder, version FROM profil WHERE id = ?", (norm["id"],)).fetchone()
        if alt and alt["melder"] != melder:
            raise Fehler(409, f"id {norm['id']} is taken, choose another id")
        if alt:
            version = max(norm["version"], alt["version"] + 1)
            norm["version"] = version
            text = json.dumps(norm, ensure_ascii=False, separators=(",", ":"))
            con.execute(
                "UPDATE profil SET name=?, family=?, baud=?, wiring=?, rufzeichen=?, status='neu', "
                "version=?, dokument=?, updated_at=?, entschieden_at=NULL, entschieden_von=NULL WHERE id=?",
                (norm["name"], norm["family"], norm["baud"], norm["wiring"], call, version, text, jetzt, norm["id"]))
        else:
            con.execute(
                "INSERT INTO profil (id, name, family, baud, wiring, rufzeichen, melder, status, version, "
                "dokument, created_at, updated_at) VALUES (?,?,?,?,?,?,?,'neu',?,?,?,?)",
                (norm["id"], norm["name"], norm["family"], norm["baud"], norm["wiring"], call, melder,
                 norm["version"], text, jetzt, jetzt))
    return {"id": norm["id"], "status": "neu", "version": norm["version"]}


def loesche(pid: str, melder: str) -> None:
    melder = _melder(melder)
    with _lock, _db() as con:
        z = con.execute("SELECT melder FROM profil WHERE id = ?", (pid.lower(),)).fetchone()
        if not z or z["melder"] != melder:
            raise Fehler(404, "no such profile of yours")
        con.execute("DELETE FROM profil WHERE id = ?", (pid.lower(),))


# ---------------------------------------------------------------- administration

def verwaltung_liste() -> list[dict]:
    with _lock, _db() as con:
        zeilen = con.execute(
            "SELECT * FROM profil ORDER BY CASE status WHEN 'neu' THEN 0 ELSE 1 END, updated_at DESC").fetchall()
    return [dict(_zeile_json(z), melder=z["melder"][:8] + "…", hinweis=z["hinweis"] or "",
                 created=z["created_at"], entschieden=z["entschieden_at"] or "",
                 entschieden_von=z["entschieden_von"] or "") for z in zeilen]


def verwaltung_dokument(pid: str) -> str:
    with _lock, _db() as con:
        z = con.execute("SELECT dokument FROM profil WHERE id = ?", (pid.lower(),)).fetchone()
    if not z:
        raise Fehler(404, "no such profile")
    return json.dumps(json.loads(z["dokument"]), ensure_ascii=False, indent=1)


def verwaltung_status(pid: str, status: str, benutzer: str) -> None:
    if status not in STATUS:
        raise Fehler(400, "unknown status")
    with _lock, _db() as con:
        n = con.execute(
            "UPDATE profil SET status=?, entschieden_at=?, entschieden_von=? WHERE id=?",
            (status, _jetzt(), benutzer, pid.lower())).rowcount
    if not n:
        raise Fehler(404, "no such profile")


def verwaltung_loeschen(pid: str) -> None:
    with _lock, _db() as con:
        n = con.execute("DELETE FROM profil WHERE id = ?", (pid.lower(),)).rowcount
    if not n:
        raise Fehler(404, "no such profile")
