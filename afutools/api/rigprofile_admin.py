"""Administration page for the rig profile catalog (/admin/rigs).

Built with the helpers of adminseite.py so it looks like the other
administration pages: a table of pending submissions first, everything
already decided below, release / reject / delete as form buttons and a
plain view of the JSON document.
"""
import adminseite as a

STAND = {
    "neu": ("busy", "wartet"),
    "frei": ("ok", "freigegeben"),
    "abgelehnt": ("no", "abgelehnt"),
}


def _zeile(p):
    farbe, wort = STAND.get(p["status"], ("", p["status"]))
    knoepfe = [a.stift_knopf(f"/admin/rigs/{p['id']}", titel="Dokument ansehen")]
    if p["status"] != "frei":
        knoepfe.append(a.zeichen_knopf(f"/admin/rigs/{p['id']}/frei", a.HAKEN,
                                       "Für alle freigeben", klasse="btn-ja"))
    if p["status"] != "abgelehnt":
        knoepfe.append(a.zeichen_knopf(f"/admin/rigs/{p['id']}/abgelehnt", a.VERBOT,
                                       "Ablehnen und ausblenden", klasse="btn-nein"))
    knoepfe.append(a.weg_knopf(f"/admin/rigs/{p['id']}/loeschen", titel="Profil löschen"))
    profil = (f"<strong>{a._t(p['name'])}</strong><br><span class=\"mono muted\">{a._t(p['id'])}</span>"
              + (f'<br><span class="muted geraet-notiz">{a._t(p["wiring"][:160])}</span>' if p["wiring"] else ""))
    return ("<tr>"
            + a._zelle("profil", profil, "wrap", p["name"])
            + a._zelle("family", a._t(p["family"]), "mono")
            + a._zelle("baud", p["baud"], "num mono")
            + a._zelle("rufzeichen", a._t(p["rufzeichen"] or "—"), "mono")
            + a._zelle("version", p["version"], "num mono")
            + a._zelle("geaendert", a._zeit(p["updated"]), "mono", p["updated"] or "")
            + a._zelle("zustand", f'<span class="badge {farbe}">{wort}</span>'
                       + (f'<br><span class="muted">{a._t(p["entschieden_von"])}</span>' if p["entschieden_von"] else ""),
                       "", wort)
            + f'<td class="knoepfe">{"".join(knoepfe)}</td>'
            + "</tr>")


def liste(*, benutzer, profile, **rest):
    offen = [p for p in profile if p["status"] == "neu"]
    entschieden = [p for p in profile if p["status"] != "neu"]

    def tabelle(name, zeilen, leer, filter=()):
        return a._tabelle(
            name,
            [("Profil", "profil", "wrap"), ("Familie", "family"), ("Baud", "baud", "num"),
             ("eingereicht von", "rufzeichen"), ("Version", "version", "num"),
             ("geändert", "geaendert"), ("Zustand", "zustand"), ("", None)],
            [_zeile(p) for p in zeilen], leer, filter=filter)

    inhalt = f"""
<h2>Wartet auf Freigabe</h2>
<p>Ein Rig-Profil sagt der Antenna Bridge, wie sie mit einem Funkgerät spricht
  (Protokollfamilie, Baudrate, welche Antwort die Frequenz trägt) und wie die Buchse
  verdrahtet wird. Freigegeben heißt: steht im Katalog auf der Seite und lässt sich
  von jeder Bridge mit einem Klick installieren. Das Dokument ist geprüft (Aufbau,
  Größe), inhaltlich stimmt es nur, wenn der Einreicher es ausprobiert hat.</p>
{tabelle("rigs-offen", offen, "Nichts offen.")}

<h2>Schon entschieden</h2>
{tabelle("rigs", entschieden, "Noch keine Profile.",
         filter=[a.suchfilter("profil", "Name oder Kennung"),
                 a.mehrfachfilter("zustand", "Zustand",
                                  (("freigegeben", "freigegeben"), ("abgelehnt", "abgelehnt")))])}
<p class="field-hint">Ablehnen blendet das Profil aus der Liste der anderen aus, der
  Einreicher sieht es weiter. Löschen nimmt es ganz weg.</p>
"""
    return a._rahmen(aktiv="/admin/rigs", titel="Rig-Profile", benutzer=benutzer,
                     kopf="CAT-Profile, die jemand für die Antenna Bridge eingereicht hat.",
                     inhalt=inhalt, **rest)


def dokument(*, benutzer, pid, text, **rest):
    inhalt = f"""
<p class="btn-row"><a class="btn" href="/admin/rigs">Zurück zur Liste</a></p>
<pre class="mono" style="white-space:pre-wrap">{a._t(text)}</pre>
"""
    return a._rahmen(aktiv="/admin/rigs", titel=f"Rig-Profil {pid}", benutzer=benutzer,
                     kopf="Das Dokument, wie es die Bridge bekommt.", inhalt=inhalt, **rest)
