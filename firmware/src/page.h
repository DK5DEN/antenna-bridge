#pragma once
#include <Arduino.h>

// Web UI, single page. Kept in its own file so web.cpp stays readable.
static const char PAGE[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>antenna-bridge</title>
<style>
/* Design tokens of afu.tools (site/style.css): dark by default, light follows the system. */
:root{color-scheme:dark;--bg:#0a0e1f;--bg2:#151b35;--fg:#eef0fa;--mut:#8a93b3;--acc:#6aa6ff;--acc2:#a78bfa;--line:#2a3358;--card:linear-gradient(180deg,rgba(28,36,68,.85),rgba(21,27,53,.85));--card-hover:rgba(106,166,255,.09);--header:rgba(10,14,31,.72);--on-acc:#0a0e1f;--shadow:0 10px 30px rgba(0,0,0,.35);--radius:14px;--ok:#10b981;--warn:#f59e0b;--err:#ef4444;--field:#151b35;--h:36px;--hs:28px;--gap:8px}
@media (prefers-color-scheme:light){:root{color-scheme:light;--bg:#f4f6fb;--bg2:#fff;--fg:#1a2235;--mut:#5b6478;--acc:#2f6fed;--acc2:#7c5cff;--line:#d6dbe8;--card:#fff;--card-hover:rgba(47,111,237,.06);--header:rgba(255,255,255,.85);--on-acc:#fff;--shadow:0 10px 30px rgba(20,30,60,.10);--field:#fff}}
*{box-sizing:border-box}[hidden]{display:none!important}
body{margin:0;color:var(--fg);font:15px/1.55 "Inter",system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;-webkit-font-smoothing:antialiased;background:radial-gradient(1200px 600px at 8% -10%,rgba(106,166,255,.13),transparent 60%),radial-gradient(900px 500px at 105% 5%,rgba(167,139,250,.12),transparent 60%),var(--bg);background-attachment:fixed}
a{color:var(--acc)}
header{display:flex;align-items:center;gap:12px;padding:10px 18px;border-bottom:1px solid var(--line);position:sticky;top:0;background:var(--header);backdrop-filter:blur(10px);z-index:3;flex-wrap:wrap}
header h1{font-size:1.05rem;margin:0;font-weight:700;letter-spacing:-.01em}
header h1::before{content:"";display:inline-block;width:10px;height:10px;border-radius:50%;background:linear-gradient(135deg,var(--acc),var(--acc2));margin-right:8px;vertical-align:-1px}
.pill{display:inline-flex;align-items:center;height:22px;padding:0 .55rem;border-radius:999px;font-size:.76rem;font-weight:600;letter-spacing:.02em;border:1px solid var(--line);color:var(--mut);background:var(--bg2)}
.pill.ok{color:var(--ok);border-color:color-mix(in srgb,var(--ok) 45%,transparent);background:color-mix(in srgb,var(--ok) 12%,transparent)}
.pill.bad{color:var(--err);border-color:color-mix(in srgb,var(--err) 45%,transparent);background:color-mix(in srgb,var(--err) 12%,transparent)}
.pill.warn{color:var(--warn);border-color:color-mix(in srgb,var(--warn) 45%,transparent);background:color-mix(in srgb,var(--warn) 12%,transparent)}
.spacer{flex:1}#net{color:var(--mut);font-size:.82rem;text-align:right}
nav{display:flex;flex-wrap:wrap;gap:.4rem;padding:.7rem 18px .2rem;position:sticky;top:50px;z-index:2;background:var(--header);backdrop-filter:blur(10px)}
nav a{display:inline-flex;align-items:center;height:var(--h);font-size:.88rem;padding:0 .9rem;border-radius:999px;border:1px solid var(--line);text-decoration:none;color:var(--mut)}
nav a:hover{border-color:var(--acc);color:var(--fg)}nav a.on{background:var(--card-hover);color:var(--fg);border-color:var(--acc)}
main{width:min(100% - 2rem,1440px);margin:0 auto;padding:14px 0 40px}
.view{display:none;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:14px}.view.on{display:grid}#v-ant.on{grid-template-columns:repeat(4,minmax(0,1fr))}@media (max-width:1100px){#v-ant.on{grid-template-columns:repeat(2,minmax(0,1fr))}}@media (max-width:640px){#v-ant.on{grid-template-columns:1fr}}
.card{background:var(--card);border:1px solid var(--line);border-radius:var(--radius);padding:1.1rem 1.2rem;box-shadow:none}.card>*:last-child{margin-bottom:0}
.card h2{font-size:.8rem;margin:0 0 .8rem;color:var(--mut);text-transform:uppercase;letter-spacing:.07em;font-weight:600;padding-left:.7rem;border-left:3px solid transparent;border-image:linear-gradient(180deg,var(--acc),var(--acc2)) 1}
.card h3{font-size:.98rem;margin:1rem 0 .4rem;letter-spacing:-.01em}
.big{display:flex;gap:24px;flex-wrap:wrap;margin-bottom:var(--gap)}.big div{min-width:110px}
.big b{display:block;font-size:1.9rem;font-weight:700;font-variant-numeric:tabular-nums;line-height:1.1;letter-spacing:-.02em}.big span{color:var(--mut);font-size:.78rem}
.row{display:flex;gap:var(--gap);flex-wrap:wrap;align-items:center;margin:var(--gap) 0}.row+.row{margin-top:var(--gap)}
button{display:inline-flex;align-items:center;justify-content:center;height:var(--h);padding:0 .95rem;border-radius:999px;border:1px solid var(--line);background:var(--bg2);color:var(--fg);font:inherit;font-size:.9rem;line-height:1;cursor:pointer;vertical-align:middle;white-space:nowrap}
button:hover{border-color:var(--acc);background:var(--card-hover)}
button.acc{background:linear-gradient(135deg,var(--acc),var(--acc2));color:var(--on-acc);border-color:transparent;font-weight:600}button.acc:hover{filter:brightness(1.08)}
button.warn{background:color-mix(in srgb,var(--warn) 18%,var(--bg2));color:var(--warn);border-color:color-mix(in srgb,var(--warn) 45%,transparent)}
button.err{background:transparent;color:var(--err);border-color:color-mix(in srgb,var(--err) 45%,transparent)}button.err:hover{background:color-mix(in srgb,var(--err) 12%,transparent)}
button.ico{width:var(--hs);height:var(--hs);padding:0;line-height:0}button.ico svg{width:1rem;height:1rem;display:block}button.ico.weg{color:var(--err);border-color:color-mix(in srgb,var(--err) 40%,var(--line))}button.ico.weg:hover{border-color:var(--err);background:color-mix(in srgb,var(--err) 12%,transparent)}button.ico.ja{color:#22c55e;border-color:color-mix(in srgb,#22c55e 40%,var(--line))}button.ico.ja:hover{border-color:#22c55e;background:color-mix(in srgb,#22c55e 12%,transparent)}button.ico.stift:hover{color:var(--fg)}
button:disabled{opacity:.4;cursor:default}button.sm{height:var(--hs);padding:0 .65rem;font-size:.78rem}
input,select,textarea{background:var(--field);color:var(--fg);border:1px solid var(--line);border-radius:9px;padding:0 .7rem;font:inherit;font-size:.93rem;width:130px;height:var(--h);line-height:calc(var(--h) - 2px);vertical-align:middle}select{padding-right:.4rem}textarea{height:auto;line-height:1.45;padding:.5rem .7rem}
input:focus,select:focus,textarea:focus{outline:2px solid var(--acc);outline-offset:1px}
input.w{width:100%}label{font-size:.72rem;text-transform:uppercase;letter-spacing:.07em;color:var(--mut);font-weight:600;display:block;margin:0 0 4px}.grid2 input,.grid2 select{width:100%}
table{width:100%;border-collapse:collapse;font-size:.9rem;font-variant-numeric:tabular-nums}td,th{padding:.5rem .6rem;border-bottom:1px solid var(--line);text-align:left;vertical-align:top}
th{color:var(--mut);font-weight:600;font-size:.74rem;text-transform:uppercase;letter-spacing:.06em}td.r,th.r{text-align:right}
code,pre{font:.82rem/1.5 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}code{background:var(--bg2);padding:1px 5px;border-radius:4px;border:1px solid var(--line)}
pre{background:var(--bg2);border:1px solid var(--line);border-radius:9px;padding:8px 10px;overflow:auto;margin:6px 0}
#log{background:var(--bg2);border:1px solid var(--line);border-radius:9px;padding:8px;height:160px;overflow:auto;font:.78rem/1.5 ui-monospace,Menlo,Consolas,monospace;white-space:pre-wrap;color:var(--mut)}
.full{grid-column:1/-1}.wide{grid-column:span 2}@media (max-width:760px){.wide{grid-column:auto}}.tag{font-size:.8rem;color:var(--mut)}.ok{color:var(--ok)}.bad{color:var(--err)}.warnc{color:var(--warn)}p{margin:6px 0}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:10px 14px}.grid2>div{display:grid;grid-template-rows:1fr var(--h);align-items:end}.grid2 label{margin:0 0 4px;line-height:1.15}.kv{display:grid;grid-template-columns:auto 1fr;gap:2px 14px;font-size:.9rem}.kv span:nth-child(odd){color:var(--mut)}
.out{border:1px solid var(--line);border-radius:10px;padding:10px 12px;margin:var(--gap) 0;display:flex;gap:12px;align-items:center;flex-wrap:wrap;background:var(--bg2)}.out>div:last-child{display:flex;gap:6px;align-items:center}
.out .n{font-weight:600;min-width:90px}.out .t{font-size:.7rem;color:var(--mut);text-transform:uppercase;letter-spacing:.06em}.out .st{min-width:60px}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;background:var(--line);margin-right:6px;vertical-align:middle}.dot.on{background:var(--ok)}.dot.off{background:color-mix(in srgb,var(--mut) 50%,transparent)}.dot.bad{background:var(--err)}
#flow{width:100%;display:block}#flow text{font:12px "Inter",system-ui,sans-serif;fill:var(--fg)}#flow .sub{font-size:10px;fill:var(--mut)}#flow .node{fill:var(--bg2);stroke:var(--line);stroke-width:1}#flow .node.act{stroke:var(--ok);stroke-width:2}#flow .node.dim{opacity:.5}#flow .node.click{cursor:pointer}#flow foreignObject select{background:var(--field);color:var(--fg);border:1px solid var(--line);border-radius:9px;padding:0 .5rem}#flow .edge{fill:none;stroke:var(--line);stroke-width:2}#flow .edge.on{stroke:var(--ok)}#flow .edge.off{stroke:var(--err)}#flow .edge.glob{stroke-dasharray:5 4}#flow .lbl{font-size:10px;fill:var(--mut)}#flow .lbl.on{fill:var(--ok)}#flow .lbl.off{fill:var(--err)}
.mx{font-size:.86rem}.mx td,.mx th{text-align:center;padding:4px 6px}.mx td.f,.mx th.f{text-align:left;white-space:nowrap}.mx .c{cursor:pointer;border-radius:999px;padding:0 4px;height:var(--hs);min-width:52px;display:inline-flex;align-items:center;justify-content:center;background:var(--bg2);border:1px solid var(--line);color:var(--mut);font-size:.76rem;font-weight:600}.mx .c.on{background:color-mix(in srgb,var(--ok) 15%,transparent);border-color:color-mix(in srgb,var(--ok) 45%,transparent);color:var(--ok)}.mx .c.off{background:color-mix(in srgb,var(--err) 15%,transparent);border-color:color-mix(in srgb,var(--err) 45%,transparent);color:var(--err)}.mx tr.cur td{background:var(--card-hover)}
#map .bar.off{fill:color-mix(in srgb,var(--err) 55%,transparent);stroke:var(--err)}
.chip{display:inline-flex;align-items:center;height:var(--hs);padding:0 .8rem;border-radius:999px;border:1px solid var(--line);background:var(--bg2);cursor:pointer;font-size:.84rem;margin:0;color:var(--mut)}#mapants,#mxants,#rigchips{display:inline-flex;flex-wrap:wrap;gap:6px}.chip:hover{border-color:var(--acc);color:var(--fg)}.chip.on{background:var(--card-hover);color:var(--fg);border-color:var(--acc)}.chip.act{box-shadow:0 0 0 2px color-mix(in srgb,var(--ok) 55%,transparent)}
#map{width:100%;touch-action:none;user-select:none;display:block}#map text{font:11px "Inter",system-ui,sans-serif;fill:var(--mut)}#map .band{fill:color-mix(in srgb,var(--fg) 4%,transparent)}#map .bandl{fill:var(--mut);font-size:10px}#map .row{fill:color-mix(in srgb,var(--fg) 3%,transparent)}#map .rowl{fill:var(--fg);font-size:12px}
#map .bar{fill:color-mix(in srgb,var(--acc) 60%,transparent);stroke:var(--acc);stroke-width:1;cursor:grab}#map .bar.inact{fill:color-mix(in srgb,var(--mut) 33%,transparent);stroke:var(--mut)}#map .bar.hit{fill:color-mix(in srgb,var(--ok) 75%,transparent);stroke:var(--ok)}#map .hnd{fill:transparent;cursor:ew-resize}#map .cur{stroke:var(--err);stroke-width:1.5}#map .del{fill:var(--err);font-size:11px;cursor:pointer}
</style></head><body>
<header><h1>antenna-bridge</h1><span id="catpill" class="pill">…</span><span class="spacer"></span><div id="net">–</div></header>
<nav><a href="#ant" data-v="ant">Antennas</a><a href="#map" data-v="map">Map</a><a href="#matrix" data-v="matrix">Matrix</a><a href="#outs" data-v="outs">Outputs</a><a href="#rules" data-v="rules">Rules</a><a href="#settings" data-v="settings">Settings</a><a href="#help" data-v="help">Help</a></nav>
<main>

<!-- ===== Antennas ===== -->
<div class="view" id="v-ant">
<section class="card full">
<h2>Signal flow</h2>
<svg id="flow" viewBox="0 0 1000 120"></svg>
<p class="tag">Live picture: the rig reports the frequency, the active antenna's rules (and the global ones, dashed) decide the outputs. Green = on, red = forced off by an OFF rule, grey = no rule for this frequency. Rig and antenna are chosen in their boxes; the outputs shown are the ones with rules for this antenna or global rules. Edit the rules on the Map, Matrix or Rules tab.</p>
</section>

<section class="card">
<h2>Frequency</h2>
<div class="big"><div><b id="freq">–</b><span>kHz, source <span id="src">–</span></span></div></div>
<div class="kv"><span>main (FA)</span><span id="fmain">–</span><span>sub (FB)</span><span id="fsub">–</span></div>
<div class="tag">CAT <span id="catst">–</span> · TX side <span id="txside">–</span> · last message <code id="catlast">–</code></div>
<div class="row"><input id="fkhz" type="number" step="0.1" placeholder="kHz" style="width:160px"><button class="acc" onclick="cmd('freq '+Math.round(v('fkhz')*1000))">Set by hand</button><button onclick="cmd('apply')">Re-apply</button></div>
<p class="tag">A manual frequency is used until the rig reports a change.</p>
</section>

<section class="card">
<h2>Antennas</h2>
<p class="tag">One antenna is active. Only its rules and the global rules drive the outputs; the same output may be used by several antennas with different rules.</p>
<div id="ants"></div>
<div class="row"><input id="aname" placeholder="new antenna, e.g. efhw" style="width:150px"><select id="atype" style="width:120px"><option>efhw</option><option>dipole</option><option>vertical</option><option>loop</option><option>beam</option><option>wire</option><option>other</option></select><button class="acc" onclick="cmd('ant add '+v('aname').trim()+' '+v('atype'));$('aname').value=''">Add antenna</button><button onclick="cmd('ant select -')">None active</button></div>
</section>

<section class="card wide">
<h2>Outputs</h2>
<div id="outs"></div>
<p class="tag" id="outtip"></p>
</section>
</div>

<!-- ===== Map ===== -->
<div class="view" id="v-map">
<section class="card full">
<h2>Frequency map</h2>
<div class="row" id="mapants"></div>
<p class="tag">Rows are the outputs, bars are the rules of the selected antenna on a logarithmic frequency axis with the amateur bands shaded. Drag a bar to move it, drag its edges to change the range, click into a band on an empty spot to add a rule for that band (click outside the bands for a narrow rule), ✕ removes a rule. The red line is the current frequency; green bars are the rules that currently match. Changes are stored immediately.</p>
<svg id="map" viewBox="0 0 1000 100" preserveAspectRatio="none"></svg>
</section>
</div>

<!-- ===== Matrix ===== -->
<div class="view" id="v-matrix">
<section class="card full">
<h2>Switching matrix</h2>
<div class="row" id="mxants"></div>
<p class="tag">Rows are frequency ranges, columns are the outputs. Click a cell: <b>–</b> (no rule) → <b class="ok">ON</b> → <b class="bad">OFF</b> → –. ON means the output is active in that range, OFF forces it off even if a wider ON rule covers the frequency. Relays and GPIO switch accordingly, UDP and line targets receive <code>freq</code> only while ON. Only the rules of the active antenna and the global ones act.</p>
<div style="overflow:auto"><table class="mx"><thead id="mxhead"></thead><tbody id="mxbody"></tbody></table></div>
<div class="row"><label style="margin:0">Add range</label><span id="mxbands"></span><input id="mxmin" type="number" step="0.1" placeholder="from kHz" style="width:110px"><input id="mxmax" type="number" step="0.1" placeholder="to kHz" style="width:110px"><button class="acc" onclick="mxRow(v('mxmin')*1000,v('mxmax')*1000)">Add</button></div>
</section>
</div>

<!-- ===== Outputs ===== -->
<div class="view" id="v-outs">
<section class="card full">
<h2>Add output</h2>
<div class="row">
<select id="otype" onchange="otype()"><option value="relay">BR1 relay (Bluetooth)</option><option value="line">Bluetooth line target (magloop-tune)</option><option value="udp">UDP target (magloop-tune)</option><option value="gpio">Local GPIO</option></select>
<input id="oname" placeholder="name" style="width:120px">
<span id="o-ble"><input id="oaddr" placeholder="aa:bb:cc:dd:ee:ff" style="width:170px" list="addrlist"><select id="oatype" style="width:110px"><option value="1">random</option><option value="0">public</option></select></span>
<span id="o-udp" hidden><input id="ohost" placeholder="host or ip" style="width:170px" value="magloop.local"><input id="oport" type="number" placeholder="port" value="4210" style="width:90px"></span>
<span id="o-gpio" hidden><input id="opin" type="number" placeholder="pin" style="width:80px"><select id="oinv" style="width:120px"><option value="0">active high</option><option value="1">active low</option></select></span>
<button class="acc" onclick="outAdd()">Add</button>
</div>
<datalist id="addrlist"></datalist>
<div class="row"><button onclick="cmd('ble scan')">Scan Bluetooth</button><span class="tag" id="scanst"></span></div>
<table><thead><tr><th>address</th><th>type</th><th>name</th><th>kind</th><th class="r">dBm</th><th></th></tr></thead><tbody id="scan"></tbody></table>
</section>
</div>

<!-- ===== Rules ===== -->
<div class="view" id="v-rules">
<section class="card full">
<h2>Rules</h2>
<p class="tag">One card per antenna. An output is active while an ON rule of the active antenna (or a global one) covers the frequency and no OFF rule does — OFF wins. Relays and GPIO pins switch accordingly, UDP and line targets receive <code>freq &lt;hz&gt;</code> while active, once per frequency change. Several rules per output are allowed; <code>0 – 999999 kHz</code> means "whenever this antenna is active". The Matrix tab shows the same rules as a grid.</p>
</section>
<div id="rulecards" class="full" style="display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:14px"></div>
</div>

<!-- ===== Settings ===== -->
<div class="view" id="v-settings">
<section class="card">
<h2>Rig</h2>
<div class="row"><select id="rigsel" style="width:260px"></select><button class="acc" onclick="cmd('rig set '+v('rigsel'))">Use</button><button class="ico weg" id="rigdel" title="Remove stored profile" onclick="if(confirm('Remove stored profile '+v('rigsel')+'?'))cmd('rig del '+v('rigsel'))" hidden><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M4 7h16"/><path d="M10 11.5v5"/><path d="M14 11.5v5"/><path d="M6 7l.9 11.1A2 2 0 0 0 8.9 20h6.2a2 2 0 0 0 2-1.9L18 7"/><path d="M9.5 7V5.4a1 1 0 0 1 1-1h3a1 1 0 0 1 1 1V7"/></svg></button></div>
<p class="tag" id="rigwire"></p>
<div class="row"><button onclick="rigExport()">Export JSON</button><button onclick="$('rigimp').hidden=!$('rigimp').hidden">Import JSON</button><button onclick="rigCatalog()">Catalog from afu.tools</button><button onclick="rigShare()">Share on afu.tools</button></div>
<div id="rigimp" hidden><textarea id="rigjson" style="width:100%;height:120px;font:.8rem ui-monospace,monospace" placeholder='{"id":"myrig","name":"…","family":"ascii","baud":38400,"ascii":{…}}'></textarea><div class="row"><button class="acc sm" onclick="rigImport()">Store profile</button><span class="tag">Format: see Help. A stored profile with the id of a built-in one replaces it.</span></div></div>
<div id="rigcat" hidden><table><thead><tr><th>profile</th><th>family</th><th class="r">baud</th><th>by</th><th></th></tr></thead><tbody id="rigcatrows"></tbody></table><p class="tag" id="rigcatinfo"></p></div>
<div class="grid2">
<div><label>CAT baud rate</label><select id="s_catbaud"><option>4800</option><option>9600</option><option>19200</option><option>38400</option><option>57600</option><option>115200</option></select></div>
<div><label>RX pin (rig TXD)</label><input id="s_catrx" type="number"></div>
<div><label>TX pin (rig RXD)</label><input id="s_cattx" type="number"></div>
<div><label>Invert levels</label><select id="s_catinv"><option value="0">no</option><option value="1">yes</option></select></div>
<div><label>CI-V address (hex)</label><input id="s_civaddr"></div>
</div>
<div class="row"><button class="acc" onclick="saveSettings()">Save</button></div>
</section>

<section class="card">
<h2>Timing</h2>
<div class="grid2">
<div><label>CAT poll interval ms</label><input id="s_catpoll" type="number"></div>
<div><label>Frequency source side</label><select id="s_catvfo"><option value="0">transmitting side (FT)</option><option value="1">main (FA)</option><option value="2">sub (FB)</option></select></div>
<div><label>Settle time ms</label><input id="s_settle" type="number"></div>
<div><label>UDP command port (reboot)</label><input id="s_udpport" type="number"></div>
<div><label>WiFi radio (reboot)</label><select id="s_wifion"><option value="1">on</option><option value="0">off, Bluetooth only</option></select></div>
<div><label>BR1 links</label><select id="s_blehold"><option value="1">keep open (reliable, fast)</option><option value="0">connect per switch (phone app usable)</option></select></div>
</div>
<div class="row"><button class="acc" onclick="saveSettings()">Save</button></div>
</section>

<section class="card">
<h2>WiFi</h2>
<div id="wifiinfo" class="tag"></div>
<table><tbody id="nets"></tbody></table>
<div class="row"><input id="wssid" placeholder="SSID (case sensitive)" list="scanlist"><input id="wpass" type="password" placeholder="password"><button class="acc" onclick="wifiAdd()">Add</button><button onclick="scan()">Scan</button></div>
<datalist id="scanlist"></datalist>
<div id="scanres" class="tag"></div>
<p class="tag">Up to 5 networks. Without a connection the bridge opens the access point <b id="apssid">–</b> (password <code id="appass">–</code>, address 192.168.4.1) 20 s after boot. WiFi is only needed for UDP targets and this page; Bluetooth outputs work without it.</p>
</section>

<section class="card">
<h2>System</h2>
<div class="kv"><span>Firmware</span><span id="fw">–</span><span>Uptime</span><span id="uptime">–</span><span>Hostname</span><span id="host">–</span><span>UDP port</span><span id="udpp">–</span></div>
<div class="row"><button class="err" onclick="if(confirm('Reboot bridge?'))cmd('reboot')">Reboot</button></div>
</section>
</div>

<!-- ===== Help ===== -->
<div class="view" id="v-help">
<section class="card">
<h2>Wiring</h2>
<p class="tag">Current rig: <b id="rigname">–</b>. RX pin <b id="rxpin">–</b> takes the rig TXD line, TX pin <b id="txpin">–</b> drives the rig RXD line.</p>
<p class="tag" id="rigwire2"></p>
<h3>Rig profile format</h3>
<pre>{"id":"ftx1","name":"Yaesu FTX-1 (TUNER/LINEAR, CAT-3)","author":"DK5DEN","version":1,
 "family":"ascii",            ascii | civ | none
 "baud":38400,"invert":false,
 "wiring":"how the jack is connected",
 "ascii":{"term":";","poll":"FA;FB;FT;","init":"AI1;","initEvery":10,
   "main":{"prefix":"FA","skip":0,"digits":9},   frequency answer: prefix, skipped chars, digits (0 = rest)
   "sub":{"prefix":"FB","skip":0,"digits":9},
   "info":[{"prefix":"IF","skip":5,"digits":9,"to":"main"}],
   "tx":{"prefix":"FT","sub":"1"}},               answer value that means "sub transmits"
 "civ":{"addr":"a4","poll":["03","2501","0F"],"main":"03","sub":"2501","split":"0F","transceive":"00"}}</pre>
<p class="tag">Built-in profiles: <code>rig list</code>. A profile is stored with <code>rig import &lt;json&gt;</code>, <code>POST /api/rig</code> or the Import button; <code>rig show [id]</code> / <code>GET /api/rig?id=…</code> exports it. The catalog on afu.tools lists profiles other people shared; the Share button hands the current profile to that page.</p>
<h3>Commands</h3>
<table>
<tr><td><code>status</code></td><td>one line summary</td></tr>
<tr><td><code>freq &lt;hz&gt;</code></td><td>manual frequency, <code>apply</code> re-applies the current one</td></tr>
<tr><td><code>out list</code> / <code>out add udp|relay|line|gpio …</code> / <code>out del &lt;name&gt;</code></td><td>outputs</td></tr>
<tr><td><code>rule list [antenna|-]</code> / <code>rule add &lt;antenna|-&gt; &lt;out&gt; &lt;fmin&gt; &lt;fmax&gt; [on|off]</code> / <code>rule set &lt;i&gt; &lt;fmin&gt; &lt;fmax&gt; [on|off]</code> / <code>rule del &lt;i&gt;</code> / <code>rule clear</code></td><td>rules per antenna, <code>-</code> = global, OFF wins over ON</td></tr>
<tr><td><code>ble scan</code> / <code>ble list</code> / <code>ble on|off|refresh &lt;name&gt;</code> / <code>ble send &lt;name&gt; &lt;text&gt;</code></td><td>Bluetooth</td></tr>
<tr><td><code>ant list</code> / <code>ant add &lt;name&gt; [type]</code> / <code>ant type &lt;name&gt; &lt;type&gt;</code> / <code>ant del &lt;name&gt;</code> / <code>ant select &lt;name|-&gt;</code></td><td>antennas; a rule belongs to an antenna, outputs are shared</td></tr>
<tr><td><code>rig</code> / <code>rig list</code> / <code>rig set &lt;preset&gt;</code></td><td>rig preset: protocol, baud rate, wiring notes</td></tr>
<tr><td><code>cat</code> / <code>cat FA;</code></td><td>CAT link status, raw command (hex bytes for CI-V)</td></tr>
<tr><td><code>set …</code>, <code>wifi …</code>, <code>save</code>, <code>reboot</code>, <code>help</code></td><td></td></tr>
</table>
<p class="tag">Same commands on the USB console (115200 baud), by UDP to port <b id="udpp2">–</b> and in the console below. <code>help</code> prints everything.</p>
<h3>HTTP API</h3>
<table>
<tr><td><code>GET /api/status</code></td><td>JSON with everything this page shows</td></tr>
<tr><td><code>POST /api/cmd</code></td><td>form field <code>line</code>, one command, text reply</td></tr>
<tr><td><code>POST /api/wifi</code></td><td>form fields <code>action=add|del</code>, <code>ssid</code>, <code>pass</code></td></tr>
<tr><td><code>GET /api/scan</code></td><td>start or poll a WiFi scan</td></tr>
</table>
</section>

<section class="card">
<h2>Console</h2>
<div class="row"><input id="cline" class="w" placeholder="command, e.g. status" onkeydown="if(event.key==='Enter'){cmd(this.value);this.value=''}"><button onclick="cmd(document.getElementById('cline').value)">Send</button></div>
<div id="log"></div>
</section>
</div>
</main>
<script>
const $=id=>document.getElementById(id);const v=id=>$(id).value;
const ICO_WEG='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M4 7h16"/><path d="M10 11.5v5"/><path d="M14 11.5v5"/><path d="M6 7l.9 11.1A2 2 0 0 0 8.9 20h6.2a2 2 0 0 0 2-1.9L18 7"/><path d="M9.5 7V5.4a1 1 0 0 1 1-1h3a1 1 0 0 1 1 1V7"/></svg>';
const ICO_STIFT='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M4 20h4.2l9.4-9.4a2.1 2.1 0 0 0 0-3l-1.2-1.2a2.1 2.1 0 0 0-3 0L4 15.8V20Z"/><path d="M14.5 6.5l3 3"/></svg>';
const ICO_JA='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M5 12.5l4.5 4.5L19 7.5"/></svg>';
const weg=(cmdline,title)=>'<button class="ico weg" title="'+esc(title||'Remove')+'" aria-label="'+esc(title||'Remove')+'" onclick="if(confirm(\''+esc(title||'Remove')+'?\'))cmd(\''+cmdline+'\')">'+ICO_WEG+'</button>';
let S=null;
const BANDS=[['160m',1810,2000],['80m',3500,3800],['60m',5351.5,5366.5],['40m',7000,7200],['30m',10100,10150],['20m',14000,14350],['17m',18068,18168],['15m',21000,21450],['12m',24890,24990],['10m',28000,29700],['6m',50000,52000],['2m',144000,146000],['70cm',430000,440000]];
function show(name){document.querySelectorAll('.view').forEach(e=>e.classList.toggle('on',e.id==='v-'+name));document.querySelectorAll('nav a').forEach(a=>a.classList.toggle('on',a.dataset.v===name));}
window.addEventListener('hashchange',()=>show(location.hash.slice(1)||'ant'));show(location.hash.slice(1)||'ant');
function log(t){const l=$('log');l.textContent+=t.replace(/\s+$/,'')+"\n";l.scrollTop=l.scrollHeight;}
async function cmd(line){if(!line)return;log('> '+line);try{const r=await fetch('/api/cmd',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({line})});log(await r.text());}catch(e){log('ERR '+e);}setTimeout(poll,150);}
function esc(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}
function khz(hz){return hz?(hz/1000).toFixed(1):'–';}
function fmtUp(s){const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return (d?d+'d ':'')+h+'h '+m+'m';}
function otype(){const t=v('otype');$('o-ble').hidden=!(t==='relay'||t==='line');$('o-udp').hidden=t!=='udp';$('o-gpio').hidden=t!=='gpio';$('oatype').value=t==='relay'?'1':'0';}
function outAdd(){const t=v('otype'),n=v('oname').trim();if(!n){alert('name');return;}let c='out add '+t+' '+n+' ';
if(t==='relay'||t==='line')c+=v('oaddr').trim()+' '+v('oatype');else if(t==='udp')c+=v('ohost').trim()+' '+v('oport');else c+=v('opin')+' '+v('oinv');cmd(c);}
function useAddr(a,ty,k){$('oaddr').value=a;$('oatype').value=ty;$('otype').value=k===2?'line':'relay';otype();location.hash='#outs';}
function ruleAdd(k){const o=v('rout'+k),a=v('rmin'+k),b=v('rmax'+k);if(!o||!a||!b)return;cmd('rule add '+k+' '+o+' '+Math.round(a*1000)+' '+Math.round(b*1000));}
function band(k,a,b){$('rmin'+k).value=a;$('rmax'+k).value=b;}
function ruleCard(k,title,sub,outs,active){const rows=s=>s.rules.map((r,i)=>[r,i]).filter(x=>outs.includes(x[0].out));
return{k,title,sub,outs,active,rows};}
function renderRules(s){const groups=[...s.ants.map(a=>({k:a.name,title:a.name+(a.type?' <span class="t" style="font-size:11px;color:var(--mut)">'+esc(a.type)+'</span>':''),sub:a.name===s.active?'<span class="ok">active</span>':'<span class="tag">inactive</span>',ant:a.name})),{k:'-',title:'Global',sub:'<span class="tag">every antenna</span>',ant:''}];
const keep={};document.querySelectorAll('#rulecards select,#rulecards input').forEach(e=>keep[e.id]=e.value);const allOuts=s.outs.map(o=>o.name);
$('rulecards').innerHTML=groups.map(g=>{const rows=s.rules.map((r,i)=>[r,i]).filter(x=>x[0].ant===g.ant);
return '<section class="card"><h2>'+g.title+' '+g.sub+'</h2><table><thead><tr><th>output</th><th class="r">from kHz</th><th class="r">to kHz</th><th>state</th><th></th></tr></thead><tbody>'+(rows.map(x=>'<tr><td>'+esc(x[0].out)+'</td><td class="r">'+khz(x[0].fmin)+'</td><td class="r">'+khz(x[0].fmax)+'</td><td><span class="'+(x[0].on?'ok':'bad')+'" style="cursor:pointer" onclick="cmd(\'rule set '+x[1]+' '+x[0].fmin+' '+x[0].fmax+' '+(x[0].on?'off':'on')+'\')">'+(x[0].on?'ON':'OFF')+'</span></td><td class="r">'+weg('rule del '+x[1],'Remove rule')+'</td></tr>').join('')||'<tr><td class="tag" colspan="5">no rules</td></tr>')+'</tbody></table>'
+(allOuts.length?'<div class="row"><select id="rout'+g.k+'">'+allOuts.map(o=>'<option'+(keep['rout'+g.k]===o?' selected':'')+'>'+esc(o)+'</option>').join('')+'</select><input id="rmin'+g.k+'" type="number" step="0.1" placeholder="from kHz" value="'+(keep['rmin'+g.k]||'')+'"><input id="rmax'+g.k+'" type="number" step="0.1" placeholder="to kHz" value="'+(keep['rmax'+g.k]||'')+'">'+bandSel('rmin'+g.k,'rmax'+g.k)+'<button class="acc" onclick="ruleAdd(\''+g.k+'\')">Add rule</button>'
+'</div>':'<p class="tag">add outputs first (Outputs tab)</p>')+'</section>';}).join('');}
function saveSettings(){const keys=['catbaud','catrx','cattx','catinv','civaddr','catpoll','catvfo','settle','udpport','wifion','blehold'];(async()=>{let n=0;for(const k of keys){const nv=String(v('s_'+k));if(S&&String(S.settings[k])!==nv){await cmd('set '+k+' '+nv);n++;}}dirty.clear();if(!n)log('nothing changed');poll();})();}
async function wifiAdd(){const ssid=v('wssid'),pass=v('wpass');if(!ssid)return;log('> wifi add '+ssid);const r=await fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({action:'add',ssid,pass})});log(await r.text());$('wpass').value='';}
async function wifiDel(ssid){if(!confirm('Remove '+ssid+'?'))return;const r=await fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({action:'del',ssid})});log(await r.text());}
async function scan(){$('scanres').textContent='scanning…';for(let i=0;i<15;i++){const r=await (await fetch('/api/scan')).json();if(!r.running){const dl=$('scanlist');dl.innerHTML='';r.networks.sort((a,b)=>b.rssi-a.rssi);$('scanres').innerHTML=r.networks.map(n=>`<a href="#settings" onclick="$('wssid').value=${JSON.stringify(n.ssid).replace(/"/g,'&quot;')};return false">${esc(n.ssid)}</a> ${n.rssi} dBm${n.enc?'':' (open)'}`).join(' · ')||'nothing found';r.networks.forEach(n=>{const o=document.createElement('option');o.value=n.ssid;dl.appendChild(o);});return;}await new Promise(r=>setTimeout(r,700));}$('scanres').textContent='scan timeout';}
const dirty=new Set();document.addEventListener('input',e=>{if(e.target.id&&e.target.id.startsWith('s_'))dirty.add(e.target.id);});
function outCard(o){let st='',extra='',btn='';
const dot=x=>'<span class="dot '+(x===1?'on':(x===0?'off':''))+'"></span>';
if(o.type==='relay'){st=dot(o.state)+(o.state<0?'unknown':(o.state?'on':'off'))+(o.want>=0&&o.want!==o.state?' <span class="warnc">→ '+(o.want?'on':'off')+'</span>':'');
extra=(o.link?'<span class="ok">linked</span> · ':'<span class="tag">no link</span> · ')+(o.batt?(o.batt/100).toFixed(2)+' V · ':'')+(o.rssi?o.rssi+' dBm · ':'')+esc(o.addr)+(o.lastok>=0?' · ok '+o.lastok+'s ago':'')+' · connects '+o.conn+(o.err?' · <span class="bad">'+esc(o.err)+'</span>':'');
btn='<button class="sm" onclick="cmd(\'ble on '+o.name+'\')">on</button><button class="sm" onclick="cmd(\'ble off '+o.name+'\')">off</button><button class="sm" onclick="cmd(\'ble refresh '+o.name+'\')">refresh</button>';}
else if(o.type==='line'){st=dot(o.state===1?1:0)+(o.state===1?'connected':'not connected');
extra=(o.sent?'sent '+khz(o.sent)+' kHz · ':'')+(o.reply?'reply <code>'+esc(o.reply)+'</code> · ':'')+esc(o.addr)+(o.err?' · <span class="bad">'+esc(o.err)+'</span>':'');
btn='<button class="sm" onclick="cmd(\'ble refresh '+o.name+'\')">reconnect</button><button class="sm" onclick="cmd(\'ble send '+o.name+' status\')">status</button>';}
else if(o.type==='udp'){st=dot(o.match?1:0)+(o.match?'in range':'idle');
extra=esc(o.host)+':'+o.port+(o.sent?' · sent '+khz(o.sent)+' kHz':'')+(o.reply?' · reply <code>'+esc(o.reply)+'</code>':'')+(o.err?' · <span class="bad">'+esc(o.err)+'</span>':'');}
else{st=dot(o.match?1:0)+(o.match?'on':'off');extra='GPIO '+o.pin+(o.inv?' active low':'');}
const used=o.used.length?'used by '+o.used.map(a=>a==='-'?'<b>global</b>':(a===S.active?'<b class="ok">'+esc(a)+'</b>':esc(a))).join(', '):'<span class="warnc">no rules yet</span>';
return '<div class="out"><div><div class="n">'+esc(o.name)+'</div><div class="t">'+o.type+'</div></div><div class="st">'+st+'</div><div class="tag" style="flex:1">'+extra+'<br>'+used+'</div><div>'+btn+weg('out del '+o.name,'Remove '+o.name+' and its rules')+'</div></div>';}
function render(s){S=s;
$('freq').textContent=khz(s.freq);$('src').textContent=s.src;$('fmain').textContent=khz(s.cat.main);$('fsub').textContent=khz(s.cat.sub);
const cp=$('catpill');cp.textContent=s.cat.ok?'CAT ok':'CAT no link';cp.className='pill '+(s.cat.ok?'ok':'bad');
$('catst').innerHTML=s.cat.ok?'<span class="ok">linked</span> ('+s.cat.rxcount+' msgs)':'<span class="bad">no answer</span>';
$('txside').textContent=s.cat.tx<0?'?':(s.cat.tx?'sub':'main');$('catlast').textContent=s.cat.last||'–';

$('outs').innerHTML=s.outs.map(outCard).join('')||'<span class="tag">no outputs yet, see the Outputs tab</span>';
$('outtip').textContent=s.blebusy?'Bluetooth operation running…':'';
$('scanst').textContent=s.scan.running?'scanning…':(s.scan.hits.length?s.scan.hits.length+' devices':'');
$('scan').innerHTML=s.scan.hits.map(h=>'<tr><td><code>'+h.addr+'</code></td><td>'+(h.atype?'random':'public')+'</td><td>'+esc(h.name)+'</td><td>'+(h.kind===1?'<span class="ok">BR1 relay</span>':(h.kind===2?'<span class="ok">line target</span>':'other'))+'</td><td class="r">'+h.rssi+'</td><td class="r"><button class="sm" onclick="useAddr(\''+h.addr+'\','+h.atype+','+h.kind+')">use</button></td></tr>').join('')||'<tr><td class="tag" colspan="6">no scan result</td></tr>';
$('addrlist').innerHTML=s.scan.hits.map(h=>'<option value="'+h.addr+'">'+esc(h.name)+'</option>').join('');
renderRules(s);renderMatrix(s);renderFlow(s);if(!drag)renderMap(s);
const w=s.wifi;$('net').innerHTML=w.mode==='sta'?esc(w.ssid)+' · '+w.ip+' · '+w.rssi+' dBm':(w.mode==='ap'?'AP '+w.ap_ssid+' · '+w.ap_ip:w.mode);
$('wifiinfo').innerHTML=(w.mode==='sta'?'Connected to <b>'+esc(w.ssid)+'</b> as '+w.ip+' ('+w.rssi+' dBm), <a href="http://'+w.hostname+'/" style="color:var(--acc)">http://'+w.hostname+'</a>':'Not connected to a network')+(w.ap?'<br>Access point <b>'+w.ap_ssid+'</b> active at '+w.ap_ip:'');
$('apssid').textContent=w.ap_ssid;$('appass').textContent=w.ap_pass;$('host').textContent=w.hostname;$('fw').textContent=s.fw;$('uptime').textContent=fmtUp(s.uptime);$('udpp').textContent=s.settings.udpport;$('udpp2').textContent=s.settings.udpport;
$('nets').innerHTML=w.networks.map(n=>'<tr><td>'+esc(n)+(n===w.ssid?' <span class="ok">●</span>':'')+'</td><td class="r"><button class="ico weg" title="Remove network" onclick=\'wifiDel('+JSON.stringify(n).replace(/'/g,'&#39;')+')\'>'+ICO_WEG+'</button></td></tr>').join('')||'<tr><td class="tag">no networks stored</td></tr>';
for(const k of ['catbaud','catrx','cattx','catinv','civaddr','catpoll','catvfo','settle','udpport','wifion','blehold'])if(!dirty.has('s_'+k))$('s_'+k).value=s.settings[k];
const rs=$('rigsel');const rk=s.rigs.map(r=>r.id+(r.stored?'*':'')).join(',');if(rs.dataset.k!==rk){rs.dataset.k=rk;rs.innerHTML=s.rigs.map(r=>'<option value="'+r.id+'">'+esc(r.name)+(r.stored?' (stored)':'')+'</option>').join('');rs.value=s.settings.rig;}
const cr=s.rigs.find(r=>r.id===rs.value);$('rigdel').hidden=!(cr&&cr.stored);
$('rigname').textContent=s.cat.rigname||s.settings.rig;$('rxpin').textContent=s.settings.catrx;$('txpin').textContent=s.settings.cattx;rigWiring(s.settings.rig);
$('ants').innerHTML=s.ants.map(a=>'<div class="out"><div><div class="n">'+esc(a.name)+(a.name===s.active?' <span class="ok">● active</span>':'')+'</div><div class="t">'+esc(a.type||'')+'</div></div><div class="tag" style="flex:1">'+(s.outs.filter(o=>o.used.includes(a.name)).map(o=>esc(o.name)).join(', ')||'no rules yet')+'<br>direct: '+(a.bands.length?a.bands.map(b=>bandName(b[0],b[1])).join(', '):'<span class="tag">not set</span>')+'</div><div>'+(a.name===s.active?'':'<button class="ico ja" title="Activate" onclick="cmd(\'ant select '+a.name+'\')">'+ICO_JA+'</button>')+'<button class="ico stift" title="Edit" onclick="antEdit(\''+a.name+'\')">'+ICO_STIFT+'</button>'+weg('ant del '+a.name,'Remove antenna '+a.name+' and its rules')+'</div></div>'+(editAnt===a.name?antEditor(a):'')).join('')||'<span class="tag">no antennas yet</span>';}
const wireCache={};async function rigWiring(id){if(wireCache[id]===undefined){wireCache[id]='…';try{const d=await (await fetch('/api/rig?id='+encodeURIComponent(id))).json();wireCache[id]=d.wiring||'';}catch(e){wireCache[id]='';}}$('rigwire').textContent=wireCache[id];$('rigwire2').textContent=wireCache[id];}
function rigExport(){window.open('/api/rig?id='+encodeURIComponent(v('rigsel')),'_blank');}
async function rigImport(){const t=v('rigjson').trim();if(!t)return;try{JSON.parse(t);}catch(e){log('ERR not valid JSON: '+e.message);return;}log('> rig import …');const r=await fetch('/api/rig',{method:'POST',headers:{'Content-Type':'application/json'},body:t});log(await r.text());if(r.ok){$('rigjson').value='';$('rigimp').hidden=true;}poll();}
const CATALOG='https://afu.tools/api/v1/antenna-bridge/rigs';
async function rigCatalog(){const b=$('rigcat');b.hidden=false;$('rigcatinfo').textContent='loading…';try{const d=await (await fetch(CATALOG)).json();const rows=d.rigs||d.profile||[];$('rigcatrows').innerHTML=rows.map(r=>'<tr><td><b>'+esc(r.name)+'</b><br><span class="tag">'+esc(r.id)+(r.wiring?' · '+esc(r.wiring).slice(0,120):'')+'</span></td><td>'+esc(r.family)+'</td><td class="r">'+r.baud+'</td><td>'+esc(r.rufzeichen||r.author||'')+'</td><td class="r"><button class="sm acc" onclick="rigInstall(\''+esc(r.id)+'\')">install</button></td></tr>').join('')||'<tr><td class="tag" colspan="5">no profiles yet</td></tr>';$('rigcatinfo').innerHTML=rows.length+' profiles from <a href="https://afu.tools/antenna-bridge" target="_blank" style="color:var(--acc)">afu.tools/antenna-bridge</a>';}catch(e){$('rigcatinfo').textContent='catalog not reachable (internet needed in the browser): '+e;}}
async function rigInstall(id){try{const d=await (await fetch(CATALOG+'/'+encodeURIComponent(id))).json();const doc=d.profile||d;log('> install '+id+' from afu.tools');const r=await fetch('/api/rig',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(doc)});log(await r.text());poll();}catch(e){log('ERR '+e);}}
async function rigShare(){try{const d=await (await fetch('/api/rig?id='+encodeURIComponent(v('rigsel')))).text();window.open('https://afu.tools/antenna-bridge#teilen='+encodeURIComponent(btoa(unescape(encodeURIComponent(d)))),'_blank');}catch(e){log('ERR '+e);}}
let editAnt=null;
function bandName(a,b){const x=BANDS.find(q=>q[1]*1000===a&&q[2]*1000===b);return x?x[0]:khz(a)+'–'+khz(b)+' kHz';}
function antEdit(name){editAnt=editAnt===name?null:name;render(S);}
function antEditor(a){const n=a.name;return '<div class="out" style="flex-direction:column;align-items:stretch;background:var(--card-hover)"><div class="row"><span class="tag">Name</span><input id="ae-name" value="'+esc(n)+'" style="width:140px"><button onclick="cmd(\'ant rename '+n+' \'+v(\'ae-name\').trim());editAnt=v(\'ae-name\').trim()">Rename</button><span class="tag">Type</span><select id="ae-type" style="width:120px" onchange="cmd(\'ant type '+n+' \'+this.value)">'+['efhw','dipole','vertical','loop','beam','wire','other'].map(t=>'<option'+(t===a.type?' selected':'')+'>'+t+'</option>').join('')+'</select></div>'
+'<div class="row"><span class="tag">Direct ranges (no tuner):</span>'+(a.bands.map((b,i)=>'<span class="chip on" onclick="cmd(\'ant band '+n+' del '+i+'\')" title="remove">'+bandName(b[0],b[1])+' ✕</span>').join('')||'<span class="tag">none — the antenna is then shown without a tuner hint</span>')+'</div>'
+'<div class="row"><label style="margin:0">Add</label>'+bandSel('ae-min','ae-max')+'<input id="ae-min" type="number" step="0.1" placeholder="from kHz" style="width:110px"><input id="ae-max" type="number" step="0.1" placeholder="to kHz" style="width:110px"><button class="acc" onclick="cmd(\'ant band '+n+' add \'+Math.round(v(\'ae-min\')*1000)+\' \'+Math.round(v(\'ae-max\')*1000))">Add</button><button onclick="editAnt=null;render(S)">Done</button></div></div>';}
function decide(s,ant,out){const f=s.freq;const rs=s.rules.filter(r=>r.out===out&&(r.ant===''||r.ant===ant));const hit=f?rs.filter(r=>f>=r.fmin&&f<=r.fmax):[];const off=hit.find(r=>!r.on),on=hit.find(r=>r.on);
return {rules:rs,st:off?'off':(on?'on':'none'),by:off||on||null};}
function renderFlow(s){const m=$('flow');if(m.contains(document.activeElement)&&document.activeElement.tagName==='SELECT')return;   // a dropdown is open, keep it
const ant=s.active||'';const aObj=s.ants.find(a=>a.name===ant);
const outs=s.outs.filter(o=>s.rules.some(r=>r.out===o.name&&(r.ant===ant||r.ant==='')));   // the antenna's outputs and those with global rules
const RH=52;const rows=Math.max(outs.length,1);const H=Math.max(30+rows*RH,134);m.setAttribute('viewBox','0 0 1000 '+H);
const midY=Math.max(20+(rows*RH-40)/2,42);let g='';
const rigX=10,rigW=270,antX=340,antW=250,outX=650,outW=340;
const fo=(x,y,w,h,html)=>'<foreignObject x="'+x+'" y="'+y+'" width="'+w+'" height="'+h+'"><div xmlns="http://www.w3.org/1999/xhtml" style="display:flex;align-items:center;height:100%">'+html+'</div></foreignObject>';
// rig node: dropdown over all profiles, status and frequency below
g+='<rect class="node" x="'+rigX+'" y="'+(midY-22)+'" width="'+rigW+'" height="84" rx="8"/>';
g+=fo(rigX+10,midY-16,rigW-20,36,'<select style="width:100%;height:32px;font-size:.86rem" onchange="cmd(\'rig set \'+this.value)">'+s.rigs.map(r=>'<option value="'+r.id+'"'+(r.id===s.settings.rig?' selected':'')+'>'+esc(r.name)+(r.stored?' (stored)':'')+'</option>').join('')+'</select>');
g+='<text class="sub" x="'+(rigX+12)+'" y="'+(midY+36)+'">'+(s.cat.ok?'<tspan fill="#10b981">CAT linked</tspan>':(s.cat.family==='none'?'network':'<tspan fill="#ef4444">no CAT link</tspan>'))+' · '+esc(s.src)+'</text><text x="'+(rigX+12)+'" y="'+(midY+56)+'" style="font-size:15px;font-weight:600">'+(s.freq?khz(s.freq)+' kHz':'–')+'</text>';
// antenna node: dropdown over all antennas, type and tuner hint below
g+='<rect class="node act" x="'+antX+'" y="'+(midY-22)+'" width="'+antW+'" height="84" rx="8"/>';
g+=fo(antX+10,midY-16,antW-20,36,'<select style="width:100%;height:32px;font-size:.86rem" onchange="cmd(\'ant select \'+this.value)"><option value="-"'+(ant?'':' selected')+'>no antenna</option>'+s.ants.map(a=>'<option value="'+esc(a.name)+'"'+(a.name===ant?' selected':'')+'>'+esc(a.name)+(a.type?' ('+esc(a.type)+')':'')+'</option>').join('')+'</select>');
g+='<text class="sub" x="'+(antX+12)+'" y="'+(midY+36)+'">'+(aObj?esc(aObj.type||'antenna')+(aObj.direct===1?' · <tspan fill="#10b981">direct, no tuner</tspan>':(aObj.direct===0?' · <tspan fill="#f59e0b">needs a tuner here</tspan>':'')):'choose the active antenna')+'</text>';
g+='<text class="sub" x="'+(antX+12)+'" y="'+(midY+56)+'">'+(aObj?(outs.length+' output'+(outs.length===1?'':'s')+(aObj.bands.length?' · direct: '+aObj.bands.map(b=>bandName(b[0],b[1])).join(', '):'')):'')+'</text>';
g+='<path class="edge on" d="M'+(rigX+rigW)+' '+(midY+20)+' C '+(rigX+rigW+40)+' '+(midY+20)+', '+(antX-40)+' '+(midY+20)+', '+antX+' '+(midY+20)+'"/>';
outs.forEach((o,i)=>{const y=20+i*RH;const d=decide(s,ant,o.name);const glob=d.by&&d.by.ant==='';
let link='';if(o.type==='relay')link=(o.link?'linked':'no link')+(o.batt?' · '+(o.batt/100).toFixed(2)+' V':'')+' · BLE relay';else if(o.type==='line')link=(o.state===1?'connected':'not connected')+' · BLE line';else if(o.type==='udp')link=esc(o.host)+':'+o.port+' · UDP';else link='GPIO '+o.pin;
const stTxt=d.st==='on'?'<tspan fill="#10b981">ON</tspan>':(d.st==='off'?'<tspan fill="#ef4444">OFF (forced)</tspan>':'<tspan fill="#8a93b3">off</tspan>');
const why=d.by?((d.by.fmax>=999e6?'always':khz(d.by.fmin)+'–'+khz(d.by.fmax)+' kHz')+(glob?' (global)':'')):'no rule for this frequency';
g+='<rect class="node" x="'+outX+'" y="'+y+'" width="'+outW+'" height="40" rx="8"/><circle cx="'+(outX+16)+'" cy="'+(y+20)+'" r="6" fill="'+(d.st==='on'?'#10b981':(d.st==='off'?'#ef4444':'#555'))+'"/><text x="'+(outX+30)+'" y="'+(y+17)+'">'+esc(o.name)+' <tspan class="sub">'+o.type+'</tspan>   '+stTxt+'</text><text class="sub" x="'+(outX+30)+'" y="'+(y+32)+'">'+link+' · '+why+'</text>';
const cls=d.st==='none'?'':d.st;g+='<path class="edge '+cls+(glob?' glob':'')+'" d="M'+(antX+antW)+' '+(midY+20)+' C '+(antX+antW+50)+' '+(midY+20)+', '+(outX-50)+' '+(y+20)+', '+outX+' '+(y+20)+'"/>';});
if(!outs.length)g+='<text class="sub" x="'+outX+'" y="'+(midY+24)+'">'+(ant?'no outputs with rules for this antenna yet':'')+'</text>';
m.innerHTML=g;}
function bandSel(minId,maxId){return '<select style="width:150px" onchange="if(this.value){const p=this.value.split(\',\');$(\''+minId+'\').value=p[0];$(\''+maxId+'\').value=p[1];}"><option value="">band…</option>'+BANDS.map(b=>'<option value="'+b[1]+','+b[2]+'">'+b[0]+' ('+b[1]+'–'+b[2]+' kHz)</option>').join('')+'<option value="0,999999">always (0–999999 kHz)</option></select>';}
let mxAnt=null;const mxExtra={};
function mxKey(r){return r.fmin+'-'+r.fmax;}
function mxRow(a,b){a=Math.round(a);b=Math.round(b);if(!a||!b||a>b)return;const k=mxAnt||'-';(mxExtra[k]=mxExtra[k]||{})[a+'-'+b]=[a,b];renderMatrix(S);}
function mxCell(ant,out,a,b){const r=S.rules.map((x,i)=>[x,i]).find(x=>x[0].ant===(ant==='-'?'':ant)&&x[0].out===out&&x[0].fmin===a&&x[0].fmax===b);
(mxExtra[ant]=mxExtra[ant]||{})[a+'-'+b]=[a,b];
if(!r)cmd('rule add '+ant+' '+out+' '+a+' '+b+' on');else if(r[0].on)cmd('rule set '+r[1]+' '+a+' '+b+' off');else cmd('rule del '+r[1]);}
function mxDelRow(ant,a,b){const idx=S.rules.map((x,i)=>[x,i]).filter(x=>x[0].ant===(ant==='-'?'':ant)&&x[0].fmin===a&&x[0].fmax===b).map(x=>x[1]).sort((x,y)=>y-x);(async()=>{for(const i of idx)await cmd('rule del '+i);const k=ant;if(mxExtra[k])delete mxExtra[k][a+'-'+b];poll();})();}
function renderMatrix(s){if(!s.ants.length&&!s.outs.length){$('mxhead').innerHTML='';$('mxbody').innerHTML='<tr><td class="tag">add antennas and outputs first</td></tr>';$('mxants').innerHTML='';return;}
if(!mxAnt||!(mxAnt==='-'||s.ants.some(a=>a.name===mxAnt)))mxAnt=s.active||(s.ants[0]&&s.ants[0].name)||'-';
$('mxants').innerHTML=s.ants.map(a=>'<span class="chip'+(a.name===mxAnt?' on':'')+(a.name===s.active?' act':'')+'" onclick="mxAnt=\''+a.name+'\';renderMatrix(S)">'+esc(a.name)+'</span>').join('')+'<span class="chip'+(mxAnt==='-'?' on':'')+'" onclick="mxAnt=\'-\';renderMatrix(S)">global</span>';
const ant=mxAnt==='-'?'':mxAnt;const rows={};s.rules.filter(r=>r.ant===ant).forEach(r=>{rows[mxKey(r)]=[r.fmin,r.fmax];});Object.assign(rows,mxExtra[mxAnt]||{});
const keys=Object.keys(rows).sort((x,y)=>rows[x][0]-rows[y][0]);
$('mxhead').innerHTML='<tr><th class="f">range</th>'+s.outs.map(o=>'<th>'+esc(o.name)+'<br><span class="tag" style="font-size:10px">'+o.type+'</span></th>').join('')+'<th></th></tr>';
$('mxbody').innerHTML=keys.map(k=>{const [a,b]=rows[k];const band=BANDS.find(x=>x[1]*1000===a&&x[2]*1000===b);const cur=s.freq&&s.freq>=a&&s.freq<=b;
return '<tr'+(cur?' class="cur"':'')+'><td class="f"><b>'+(band?band[0]:'')+'</b> '+(b>=999e6?'always':khz(a)+' – '+khz(b)+' kHz')+'</td>'+s.outs.map(o=>{const r=s.rules.find(x=>x.ant===ant&&x.out===o.name&&x.fmin===a&&x.fmax===b);const st=r?(r.on?'on':'off'):'';
return '<td><span class="c '+st+'" onclick="mxCell(\''+mxAnt+'\',\''+esc(o.name)+'\','+a+','+b+')">'+(st?st.toUpperCase():'–')+'</span></td>';}).join('')+'<td><button class="ico weg" title="Remove range" onclick="mxDelRow(\''+mxAnt+'\','+a+','+b+')">'+ICO_WEG+'</button></td></tr>';}).join('')||'<tr><td class="tag" colspan="'+(s.outs.length+2)+'">no ranges yet, add one below</td></tr>';
if(!$('mxbands').innerHTML)$('mxbands').innerHTML=bandSel('mxmin','mxmax');}
let mapAnt=null,drag=null;
const FLO=Math.log10(1.5e6),FHI=Math.log10(5e8),MW=1000,LW=110,RH=36,TOP=26;
const fx=f=>LW+(Math.min(Math.max(Math.log10(Math.max(f,1)),FLO),FHI)-FLO)/(FHI-FLO)*(MW-LW-10);
const xf=x=>Math.round(Math.pow(10,FLO+(Math.min(Math.max(x,LW),MW-10)-LW)/(MW-LW-10)*(FHI-FLO)));
function svgPt(ev){const m=$('map'),r=m.getBoundingClientRect();return{x:(ev.clientX-r.left)/r.width*MW,y:(ev.clientY-r.top)/r.height*m.viewBox.baseVal.height};}
function bandAt(f){return BANDS.find(b=>f>=b[1]*1000&&f<=b[2]*1000);}
function renderMap(s){if(!s.ants.length){$('map').setAttribute('viewBox','0 0 1000 40');$('map').innerHTML='<text x="20" y="25">add an antenna first</text>';$('mapants').innerHTML='';return;}
if(!mapAnt||!(mapAnt==='-'||s.ants.some(a=>a.name===mapAnt)))mapAnt=s.active||s.ants[0].name;
$('mapants').innerHTML=s.ants.map(a=>'<span class="chip'+(a.name===mapAnt?' on':'')+(a.name===s.active?' act':'')+'" onclick="mapAnt=\''+a.name+'\';renderMap(S)">'+esc(a.name)+(a.type?' <small>'+esc(a.type)+'</small>':'')+'</span>').join('')+'<span class="chip'+(mapAnt==='-'?' on':'')+'" onclick="mapAnt=\'-\';renderMap(S)">global</span>'+(mapAnt!=='-'&&mapAnt!==s.active?' <button class="sm acc" onclick="cmd(\'ant select '+mapAnt+'\')">activate '+esc(mapAnt)+'</button>':'');
const ant=mapAnt==='-'?'':mapAnt;const outs=s.outs;const aObj=s.ants.find(x=>x.name===ant);const dirRow=aObj&&aObj.bands.length?1:0;const H=TOP+(outs.length+dirRow)*RH+8;const m=$('map');m.setAttribute('viewBox','0 0 1000 '+H);
let g='';BANDS.forEach(b=>{const x1=fx(b[1]*1000),x2=fx(b[2]*1000);g+='<rect class="band" x="'+x1+'" y="'+TOP+'" width="'+Math.max(x2-x1,2)+'" height="'+(H-TOP-8)+'"/><text class="bandl" x="'+((x1+x2)/2)+'" y="'+(TOP-8)+'" text-anchor="middle">'+b[0]+'</text>';});
if(dirRow){const y=TOP;g+='<text class="rowl" x="8" y="'+(y+RH/2+1)+'">'+esc(ant)+'</text><text x="8" y="'+(y+RH/2+13)+'" style="font-size:9px">direct, no tuner</text>';aObj.bands.forEach(b=>{const x1=fx(b[0]),x2=Math.max(fx(b[1]),x1+4);g+='<rect x="'+x1+'" y="'+(y+6)+'" width="'+(x2-x1)+'" height="'+(RH-16)+'" rx="3" fill="#f59e0b55" stroke="#f59e0b"/>';});}
outs.forEach((o,i0)=>{const i=i0+dirRow;const y=TOP+i*RH;g+='<rect class="row" x="'+LW+'" y="'+y+'" width="'+(MW-LW-10)+'" height="'+(RH-4)+'" data-out="'+esc(o.name)+'"/><text class="rowl" x="8" y="'+(y+RH/2+1)+'">'+esc(o.name)+'</text><text x="8" y="'+(y+RH/2+13)+'" style="font-size:9px">'+o.type+'</text>';
s.rules.forEach((r,ri)=>{if(r.ant!==ant||r.out!==o.name)return;const x1=fx(r.fmin),x2=Math.max(fx(r.fmax),x1+6);const hit=s.freq&&r.active&&r.on&&s.freq>=r.fmin&&s.freq<=r.fmax;
g+='<g data-rule="'+ri+'"><rect class="bar'+(r.on?'':' off')+(hit?' hit':(r.active?'':' inact'))+'" x="'+x1+'" y="'+(y+4)+'" width="'+(x2-x1)+'" height="'+(RH-12)+'" rx="3"/><rect class="hnd" data-edge="l" x="'+(x1-4)+'" y="'+(y+4)+'" width="8" height="'+(RH-12)+'"/><rect class="hnd" data-edge="r" x="'+(x2-4)+'" y="'+(y+4)+'" width="8" height="'+(RH-12)+'"/><text class="del" x="'+(x2-9)+'" y="'+(y+RH/2+1)+'" data-del="'+ri+'">✕</text>'+((x2-x1)>60?'<text x="'+(x1+4)+'" y="'+(y+RH/2+1)+'" style="fill:var(--fg);font-size:10px;pointer-events:none">'+(r.fmax>=999e6?'always':khz(r.fmin)+'–'+khz(r.fmax))+'</text>':'')+'</g>';});});
if(s.freq){const x=fx(s.freq);g+='<line class="cur" x1="'+x+'" y1="'+(TOP-4)+'" x2="'+x+'" y2="'+(H-6)+'"/>';}
m.innerHTML=g;}
function mapDown(ev){const m=$('map');const t=ev.target;const pt=svgPt(ev);if(t.dataset.del!==undefined){cmd('rule del '+t.dataset.del);return;}
const gr=t.closest('[data-rule]');if(gr){const ri=+gr.dataset.rule,r=S.rules[ri];drag={ri,edge:t.dataset.edge||'m',x0:pt.x,fmin:r.fmin,fmax:r.fmax,lo:Math.log10(r.fmin||1),hi:Math.log10(r.fmax||1),moved:false};m.setPointerCapture(ev.pointerId);return;}
if(t.dataset.out&&mapAnt){const f=xf(pt.x),b=bandAt(f);const a=b?b[1]*1000:Math.round(f*0.97),c=b?b[2]*1000:Math.round(f*1.03);cmd('rule add '+mapAnt+' '+t.dataset.out+' '+a+' '+c);}}
function mapMove(ev){if(!drag)return;const pt=svgPt(ev);const dl=(pt.x-drag.x0)/(MW-LW-10)*(FHI-FLO);if(Math.abs(pt.x-drag.x0)>2)drag.moved=true;let lo=drag.lo,hi=drag.hi;if(drag.edge==='l')lo=Math.min(drag.lo+dl,hi-0.005);else if(drag.edge==='r')hi=Math.max(drag.hi+dl,lo+0.005);else{lo+=dl;hi+=dl;}
drag.cur=[Math.round(Math.pow(10,lo)),Math.round(Math.pow(10,hi))];const g=$('map').querySelector('[data-rule="'+drag.ri+'"]');if(g){const x1=fx(drag.cur[0]),x2=Math.max(fx(drag.cur[1]),x1+6);const b=g.querySelector('.bar');b.setAttribute('x',x1);b.setAttribute('width',x2-x1);g.querySelector('[data-edge=l]').setAttribute('x',x1-4);g.querySelector('[data-edge=r]').setAttribute('x',x2-4);g.querySelector('.del').setAttribute('x',x2-9);}}
function mapUp(ev){if(!drag)return;const d=drag;drag=null;if(d.moved&&d.cur){let a=d.cur[0],c=d.cur[1];const ba=bandAt(a),bc=bandAt(c);if(d.edge!=='m'){if(d.edge==='l'&&ba&&Math.abs(Math.log10(a)-Math.log10(ba[1]*1000))<0.012)a=ba[1]*1000;if(d.edge==='r'&&bc&&Math.abs(Math.log10(c)-Math.log10(bc[2]*1000))<0.012)c=bc[2]*1000;}cmd('rule set '+d.ri+' '+a+' '+c);}}
$('map').addEventListener('pointerdown',mapDown);$('map').addEventListener('pointermove',mapMove);$('map').addEventListener('pointerup',mapUp);$('map').addEventListener('pointercancel',()=>{drag=null;});
async function poll(){try{const r=await fetch('/api/status',{cache:'no-store'});render(await r.json());}catch(e){$('catpill').textContent='offline';$('catpill').className='pill';}}
otype();poll();setInterval(poll,1000);
</script></body></html>
)HTML";
