/* Rig profile catalog for the antenna-bridge (afu.tools/antenna-bridge).
   Lists released profiles from the API, shows one, and turns the form into a
   profile document that is submitted with the browser's reporter id
   (`afu.tools:melder`, the same id the harmonics and report pages use). */
(function () {
  "use strict";
  const API = "/api/v1/antenna-bridge/rigs";
  const KENNUNG_KEY = "afu.tools:melder";
  const $ = (id) => document.getElementById(id);
  let profile = [];
  let aktuell = null;

  function kennung(anlegen = false) {
    let wert = null;
    try { wert = localStorage.getItem(KENNUNG_KEY); } catch { wert = null; }
    if (wert || !anlegen) return wert;
    const roh = new Uint8Array(16);
    crypto.getRandomValues(roh);
    wert = btoa(String.fromCharCode(...roh)).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");
    try { localStorage.setItem(KENNUNG_KEY, wert); } catch { /* privater Modus */ }
    return wert;
  }

  const esc = (s) => String(s ?? "").replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
  const zustand = { neu: ["busy", "wartet"], frei: ["ok", "freigegeben"], abgelehnt: ["no", "abgelehnt"] };

  // ------------------------------------------------------------ Katalog
  async function lade() {
    const m = kennung();
    try {
      const res = await fetch(API + (m ? "?melder=" + encodeURIComponent(m) : ""));
      const daten = await res.json();
      profile = daten.rigs || [];
    } catch (err) {
      $("ab-zeilen").innerHTML = '<tr><td colspan="6" class="empty">Katalog nicht erreichbar.</td></tr>';
      return;
    }
    zeige();
    zeigeEigene();
  }

  function zeige() {
    const q = ($("ab-suche").value || "").trim().toLowerCase();
    const liste = profile.filter((p) => !q || (p.name + " " + p.id + " " + (p.wiring || "")).toLowerCase().includes(q));
    $("ab-zahl").textContent = liste.length === profile.length ? `${profile.length} Profile` : `${liste.length} von ${profile.length}`;
    $("ab-zeilen").innerHTML = liste.map((p) => {
      const z = p.status !== "frei" ? zustand[p.status] || ["", p.status] : null;
      return `<tr><td class="wrap"><strong>${esc(p.name)}</strong> <span class="mono muted">${esc(p.id)}</span>${z ? ` <span class="badge ${z[0]}">${z[1]}</span>` : ""}${p.wiring ? `<br><span class="muted">${esc(p.wiring.slice(0, 140))}${p.wiring.length > 140 ? " …" : ""}</span>` : ""}</td>` +
        `<td class="mono">${esc(p.family)}</td><td class="num mono">${p.baud}</td><td class="mono">${esc(p.rufzeichen || "—")}</td><td class="num mono">${p.version}</td>` +
        `<td><button class="btn-sm" type="button" data-id="${esc(p.id)}">ansehen</button></td></tr>`;
    }).join("") || '<tr><td colspan="6" class="empty">Noch kein Profil im Katalog. Das erste kommt von dir?</td></tr>';
  }

  async function oeffne(id) {
    const m = kennung();
    let daten;
    try {
      daten = await (await fetch(`${API}/${encodeURIComponent(id)}${m ? "?melder=" + encodeURIComponent(m) : ""}`)).json();
    } catch { return; }
    if (!daten.profile) return;
    aktuell = daten;
    const p = daten.profile, meta = daten.meta;
    $("ab-detail-titel").textContent = p.name;
    $("ab-detail-daten").innerHTML = [["Kennung", p.id], ["Familie", p.family], ["Baud", p.baud + (p.invert ? ", Pegel invertiert" : "")],
      ["eingereicht von", meta.rufzeichen || "—"], ["Version", p.version], ["geändert", (meta.updated || "").slice(0, 10)]]
      .map(([k, v]) => `<dt>${esc(k)}</dt><dd>${esc(v)}</dd>`).join("");
    $("ab-detail-wiring").textContent = p.wiring || "keine Angabe";
    $("ab-detail-json").textContent = JSON.stringify(p, null, 1);
    $("ab-detail").hidden = false;
    $("ab-detail").scrollIntoView({ behavior: "smooth", block: "start" });
  }

  // ------------------------------------------------------------ Formular
  const felder = ["id", "name", "ruf", "family", "baud", "invert", "wiring", "term", "poll", "init", "initevery",
    "main", "mainskip", "maindig", "sub", "subskip", "subdig", "tx", "txsub", "addr", "cpoll", "cmain", "csub", "csplit", "ctrx"];
  let ausJson = false;   // true while the JSON field is being edited by hand

  function familieZeigen() {
    const fam = $("f-family").value;
    $("fam-ascii").classList.toggle("an", fam === "ascii");
    $("fam-civ").classList.toggle("an", fam === "civ");
  }

  function dokumentAusFeldern() {
    const v = (id) => $("f-" + id).value.trim();
    const n = (id) => parseInt(v(id) || "0", 10) || 0;
    const fam = v("family");
    const doc = { id: v("id").toLowerCase(), name: v("name"), author: v("ruf").toUpperCase(), version: 1,
      family: fam, baud: parseInt(v("baud"), 10), invert: v("invert") === "true", wiring: v("wiring") };
    if (aktuell && aktuell.profile && aktuell.profile.id === doc.id) doc.version = (aktuell.profile.version || 1);
    if (fam === "ascii") {
      doc.ascii = { term: v("term") || ";", poll: v("poll"), init: v("init"), initEvery: n("initevery"),
        main: { prefix: v("main"), skip: n("mainskip"), digits: n("maindig") } };
      if (v("sub")) doc.ascii.sub = { prefix: v("sub"), skip: n("subskip"), digits: n("subdig") };
      if (v("tx")) doc.ascii.tx = { prefix: v("tx"), sub: v("txsub") || "1" };
    } else if (fam === "civ") {
      doc.civ = { addr: v("addr").toLowerCase(), poll: v("cpoll").split(",").map((s) => s.trim()).filter(Boolean),
        main: v("cmain"), transceive: v("ctrx") };
      if (v("csub")) doc.civ.sub = v("csub");
      if (v("csplit")) doc.civ.split = v("csplit");
    }
    return doc;
  }

  function felderAusDokument(doc) {
    const set = (id, val) => { const e = $("f-" + id); if (e && val !== undefined && val !== null) e.value = String(val); };
    set("id", doc.id); set("name", doc.name); set("ruf", doc.author); set("family", doc.family || "ascii");
    set("baud", doc.baud || 38400); set("invert", doc.invert ? "true" : "false"); set("wiring", doc.wiring);
    const a = doc.ascii || {};
    set("term", a.term ?? ";"); set("poll", a.poll ?? ""); set("init", a.init ?? ""); set("initevery", a.initEvery ?? 10);
    set("main", a.main?.prefix ?? ""); set("mainskip", a.main?.skip ?? 0); set("maindig", a.main?.digits ?? 0);
    set("sub", a.sub?.prefix ?? ""); set("subskip", a.sub?.skip ?? 0); set("subdig", a.sub?.digits ?? 0);
    set("tx", a.tx?.prefix ?? ""); set("txsub", a.tx?.sub ?? "1");
    const c = doc.civ || {};
    set("addr", c.addr ?? "a4"); set("cpoll", (c.poll || []).join(",")); set("cmain", c.main ?? "03");
    set("csub", c.sub ?? ""); set("csplit", c.split ?? ""); set("ctrx", c.transceive ?? "00");
    familieZeigen();
  }

  function jsonAktualisieren() {
    if (ausJson) return;
    $("f-json").value = JSON.stringify(dokumentAusFeldern(), null, 1);
  }

  function status(text, art) {
    const e = $("ab-form-status");
    e.textContent = text;
    e.className = "field-hint" + (art === "fehler" ? " no" : "");
  }

  async function einreichen(ev) {
    ev.preventDefault();
    let doc;
    try { doc = JSON.parse($("f-json").value); } catch (err) { status("Das Dokument ist kein gültiges JSON: " + err.message, "fehler"); return; }
    status("wird eingereicht …");
    try {
      const res = await fetch(API, { method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ melder: kennung(true), rufzeichen: $("f-ruf").value.trim().toUpperCase(), profile: doc }) });
      const daten = await res.json().catch(() => ({}));
      if (!res.ok) throw new Error(daten.detail || daten.error || `HTTP ${res.status}`);
      status("");
      const h = $("ab-hinweis");
      h.hidden = false;
      h.innerHTML = `<strong>Danke.</strong> Das Profil <code>${esc(daten.id)}</code> ist eingereicht und wartet auf die Freigabe. Du siehst es unten bei deinen Einreichungen und kannst es jederzeit unter derselben Kennung neu einreichen oder löschen.`;
      lade();
    } catch (err) {
      status("Nicht angenommen: " + err.message, "fehler");
    }
  }

  async function loeschen(id) {
    if (!confirm(`Profil ${id} zurückziehen?`)) return;
    try {
      const res = await fetch(`${API}/${encodeURIComponent(id)}?melder=${encodeURIComponent(kennung() || "")}`, { method: "DELETE" });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      lade();
    } catch (err) { alert("Löschen fehlgeschlagen: " + err.message); }
  }

  function zeigeEigene() {
    const eigene = profile.filter((p) => p.eigen);
    $("ab-eigene").hidden = eigene.length === 0;
    $("ab-eigene-zeilen").innerHTML = eigene.map((p) => {
      const z = zustand[p.status] || ["", p.status];
      return `<tr><td class="wrap"><strong>${esc(p.name)}</strong> <span class="mono muted">${esc(p.id)}</span></td><td><span class="badge ${z[0]}">${z[1]}</span></td><td class="num mono">${p.version}</td>` +
        `<td><button class="btn-sm" type="button" data-id="${esc(p.id)}">ansehen</button> <button class="btn-sm" type="button" data-weg="${esc(p.id)}">zurückziehen</button></td></tr>`;
    }).join("");
  }

  // Prefilled by the bridge: #teilen=<base64 of the JSON document>
  function vorbelegung() {
    const m = location.hash.match(/#teilen=(.+)$/);
    if (!m) return;
    try {
      const text = decodeURIComponent(escape(atob(decodeURIComponent(m[1]))));
      const doc = JSON.parse(text);
      felderAusDokument(doc);
      $("f-json").value = JSON.stringify(doc, null, 1);
      status("Profil aus der Bridge übernommen. Kennung und Name prüfen, dann einreichen.");
      $("ab-form").scrollIntoView({ behavior: "smooth", block: "start" });
      history.replaceState(null, "", location.pathname);
    } catch { /* kein brauchbarer Anker */ }
  }

  // ------------------------------------------------------------ Verdrahtung
  document.addEventListener("click", (ev) => {
    const b = ev.target.closest("button[data-id]");
    if (b) { oeffne(b.dataset.id); return; }
    const w = ev.target.closest("button[data-weg]");
    if (w) loeschen(w.dataset.weg);
  });
  $("ab-suche").addEventListener("input", zeige);
  $("ab-detail-zu").addEventListener("click", () => { $("ab-detail").hidden = true; });
  $("ab-kopieren").addEventListener("click", async () => {
    try { await navigator.clipboard.writeText($("ab-detail-json").textContent); $("ab-kopieren").textContent = "kopiert"; setTimeout(() => { $("ab-kopieren").textContent = "Dokument kopieren"; }, 1500); } catch { /* kein Zugriff */ }
  });
  $("ab-bearbeiten").addEventListener("click", () => {
    if (!aktuell) return;
    const doc = JSON.parse(JSON.stringify(aktuell.profile));
    if (!aktuell.meta.eigen) { doc.id = doc.id + "-2"; doc.version = 1; }
    felderAusDokument(doc);
    ausJson = false;
    jsonAktualisieren();
    $("ab-form").scrollIntoView({ behavior: "smooth", block: "start" });
  });
  felder.forEach((f) => { const e = $("f-" + f); if (e) e.addEventListener("input", () => { ausJson = false; if (f === "family") familieZeigen(); jsonAktualisieren(); }); });
  $("f-family").addEventListener("change", () => { familieZeigen(); ausJson = false; jsonAktualisieren(); });
  $("f-json").addEventListener("input", () => { ausJson = true; try { felderAusDokument(JSON.parse($("f-json").value)); } catch { /* halbfertig */ } });
  $("ab-form").addEventListener("submit", einreichen);
  $("ab-leeren").addEventListener("click", () => { $("ab-form").reset(); aktuell = null; ausJson = false; familieZeigen(); jsonAktualisieren(); $("ab-hinweis").hidden = true; status(""); });

  familieZeigen();
  jsonAktualisieren();
  vorbelegung();
  lade();
})();
