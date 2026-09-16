#pragma once
#include <Arduino.h>

// Web UI, single page. Kept in its own file so web.cpp stays readable.
static const char PAGE[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>antenna-bridge</title>
<style>
:root{--bg:#14171c;--card:#1e2329;--line:#2c333c;--fg:#e6e9ee;--mut:#8b95a3;--acc:#4da3ff;--ok:#3ec97a;--warn:#f0b64a;--err:#f06a5a}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.45 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
header{display:flex;align-items:center;gap:12px;padding:10px 16px;border-bottom:1px solid var(--line);position:sticky;top:0;background:var(--bg);z-index:2;flex-wrap:wrap}
header h1{font-size:18px;margin:0;font-weight:600}
.pill{padding:2px 10px;border-radius:12px;font-size:13px;background:var(--line)}
.pill.ok{background:var(--ok);color:#000}.pill.bad{background:var(--err);color:#000}.pill.warn{background:var(--warn);color:#000}
.spacer{flex:1}#net{color:var(--mut);font-size:13px;text-align:right}
nav{display:flex;gap:4px;padding:8px 16px 0;border-bottom:1px solid var(--line);background:var(--bg);position:sticky;top:49px;z-index:2}
nav a{color:var(--mut);text-decoration:none;padding:8px 14px;border-radius:8px 8px 0 0;border:1px solid transparent;border-bottom:none}
nav a.on{color:var(--fg);background:var(--card);border-color:var(--line)}
main{max-width:960px;margin:0 auto;padding:12px}
.view{display:none;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:12px}.view.on{display:grid}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:14px}
.card h2{font-size:13px;margin:0 0 10px;color:var(--mut);text-transform:uppercase;letter-spacing:.05em}
.card h3{font-size:14px;margin:14px 0 6px}
.big{display:flex;gap:24px;flex-wrap:wrap;margin-bottom:10px}.big div{min-width:110px}
.big b{display:block;font-size:30px;font-weight:600;font-variant-numeric:tabular-nums;line-height:1.1}.big span{color:var(--mut);font-size:12px}
.row{display:flex;gap:6px;flex-wrap:wrap;align-items:center;margin:6px 0}
button{background:#2a3139;color:var(--fg);border:1px solid #3a424c;border-radius:7px;padding:9px 13px;font-size:14px;cursor:pointer;min-width:46px}
button:hover{background:#343c46}button.acc{background:var(--acc);color:#000;border-color:var(--acc)}button.warn{background:var(--warn);color:#000;border-color:var(--warn)}
button.err{background:transparent;color:var(--err);border-color:var(--err)}button:disabled{opacity:.4;cursor:default}button.sm{padding:4px 8px;font-size:12px;min-width:0}
input,select{background:#0f1216;color:var(--fg);border:1px solid #3a424c;border-radius:7px;padding:8px 10px;font-size:14px;width:130px}
input.w{width:100%}label{color:var(--mut);font-size:12px;display:block;margin-top:6px}
table{width:100%;border-collapse:collapse;font-size:14px;font-variant-numeric:tabular-nums}td,th{padding:5px 6px;border-bottom:1px solid var(--line);text-align:left;vertical-align:top}
th{color:var(--mut);font-weight:500;font-size:12px}td.r,th.r{text-align:right}
code,pre{font:12.5px/1.5 ui-monospace,Menlo,Consolas,monospace}code{background:#0f1216;padding:1px 5px;border-radius:4px}
pre{background:#0f1216;border:1px solid var(--line);border-radius:7px;padding:8px 10px;overflow:auto;margin:6px 0}
#log{background:#0f1216;border:1px solid var(--line);border-radius:7px;padding:8px;height:160px;overflow:auto;font:12px/1.5 ui-monospace,Menlo,Consolas,monospace;white-space:pre-wrap;color:#b9c2cc}
.full{grid-column:1/-1}.tag{font-size:12px;color:var(--mut)}.ok{color:var(--ok)}.bad{color:var(--err)}.warnc{color:var(--warn)}p{margin:6px 0}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:4px 12px}.kv{display:grid;grid-template-columns:auto 1fr;gap:2px 14px;font-size:14px}.kv span:nth-child(odd){color:var(--mut)}
.out{border:1px solid var(--line);border-radius:8px;padding:8px 10px;margin:6px 0;display:flex;gap:10px;align-items:center;flex-wrap:wrap}
.out .n{font-weight:600;min-width:90px}.out .t{font-size:11px;color:var(--mut);text-transform:uppercase}.out .st{min-width:60px}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;background:var(--line);margin-right:6px;vertical-align:middle}.dot.on{background:var(--ok)}.dot.off{background:#555}.dot.bad{background:var(--err)}
</style></head><body>
<header><h1>antenna-bridge</h1><span id="catpill" class="pill">…</span><span class="spacer"></span><div id="net">–</div></header>
<nav><a href="#ant" data-v="ant">Antennas</a><a href="#outs" data-v="outs">Outputs</a><a href="#rules" data-v="rules">Rules</a><a href="#settings" data-v="settings">Settings</a><a href="#help" data-v="help">Help</a></nav>
<main>

<!-- ===== Antennas ===== -->
<div class="view" id="v-ant">
<section class="card">
<h2>Frequency</h2>
<div class="big">
<div><b id="freq">–</b><span>kHz, source <span id="src">–</span></span></div>
<div><b id="fmain">–</b><span>main (FA)</span></div>
<div><b id="fsub">–</b><span>sub (FB)</span></div>
</div>
<div class="tag">CAT <span id="catst">–</span> · TX side <span id="txside">–</span> · last message <code id="catlast">–</code></div>
<div class="row" style="margin-top:8px"><input id="fkhz" type="number" step="0.1" placeholder="kHz" style="width:160px"><button class="acc" onclick="cmd('freq '+Math.round(v('fkhz')*1000))">Set by hand</button><button onclick="cmd('apply')">Re-apply</button></div>
<p class="tag">A manual frequency is used until the rig reports a change.</p>
</section>

<section class="card">
<h2>Antennas</h2>
<p class="tag">One antenna is active. Its outputs and the unassigned ones follow the rules, outputs of the other antennas are treated as off.</p>
<div id="ants"></div>
<div class="row" style="margin-top:8px"><input id="aname" placeholder="new antenna, e.g. efhw" style="width:180px"><button class="acc" onclick="cmd('ant add '+v('aname').trim());$('aname').value=''">Add antenna</button><button onclick="cmd('ant select -')">None active</button></div>
</section>

<section class="card full">
<h2>Outputs by antenna</h2>
<div id="outs"></div>
<p class="tag" id="outtip"></p>
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
<p class="tag">An output is active while the frequency lies inside one of its rules. Relays and GPIO pins switch on inside and off outside. UDP and line targets receive <code>freq &lt;hz&gt;</code> inside their rules, once per frequency change. Several rules per output are allowed.</p>
<table><thead><tr><th>#</th><th>output</th><th class="r">from kHz</th><th class="r">to kHz</th><th></th></tr></thead><tbody id="rules"></tbody></table>
<div class="row" style="margin-top:8px"><select id="rout"></select><input id="rmin" type="number" step="0.1" placeholder="from kHz"><input id="rmax" type="number" step="0.1" placeholder="to kHz"><button class="acc" onclick="ruleAdd()">Add rule</button></div>
<div class="row"><span class="tag">Band presets:</span><span id="bands"></span></div>
</section>
</div>

<!-- ===== Settings ===== -->
<div class="view" id="v-settings">
<section class="card">
<h2>Rig</h2>
<div class="row"><select id="rigsel" style="width:260px"></select><button class="acc" onclick="cmd('rig set '+v('rigsel'))">Apply preset</button></div>
<p class="tag" id="rigwire"></p>
<div class="grid2">
<div><label>Protocol</label><select id="s_proto"><option value="none">none (network)</option><option value="yaesu">Yaesu ASCII</option><option value="kenwood">Kenwood / Elecraft</option><option value="icom">Icom CI-V</option></select></div>
<div><label>CAT baud rate</label><select id="s_catbaud"><option>4800</option><option>9600</option><option>19200</option><option>38400</option><option>57600</option><option>115200</option></select></div>
<div><label>RX pin (rig TXD)</label><input id="s_catrx" type="number"></div>
<div><label>TX pin (rig RXD)</label><input id="s_cattx" type="number"></div>
<div><label>Invert levels</label><select id="s_catinv"><option value="0">no</option><option value="1">yes</option></select></div>
<div><label>CI-V address (hex)</label><input id="s_civaddr"></div>
</div>
<div class="row" style="margin-top:10px"><button class="acc" onclick="saveSettings()">Save</button></div>
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
<div class="row" style="margin-top:10px"><button class="acc" onclick="saveSettings()">Save</button></div>
</section>

<section class="card">
<h2>WiFi</h2>
<div id="wifiinfo" class="tag"></div>
<table style="margin-top:6px"><tbody id="nets"></tbody></table>
<div class="row" style="margin-top:8px"><input id="wssid" placeholder="SSID (case sensitive)" list="scanlist"><input id="wpass" type="password" placeholder="password"><button class="acc" onclick="wifiAdd()">Add</button><button onclick="scan()">Scan</button></div>
<datalist id="scanlist"></datalist>
<div id="scanres" class="tag"></div>
<p class="tag" style="margin-top:8px">Up to 5 networks. Without a connection the bridge opens the access point <b id="apssid">–</b> (password <code id="appass">–</code>, address 192.168.4.1) 20 s after boot. WiFi is only needed for UDP targets and this page; Bluetooth outputs work without it.</p>
</section>

<section class="card">
<h2>System</h2>
<div class="kv"><span>Firmware</span><span id="fw">–</span><span>Uptime</span><span id="uptime">–</span><span>Hostname</span><span id="host">–</span><span>UDP port</span><span id="udpp">–</span></div>
<div class="row" style="margin-top:10px"><button class="err" onclick="if(confirm('Reboot bridge?'))cmd('reboot')">Reboot</button></div>
</section>
</div>

<!-- ===== Help ===== -->
<div class="view" id="v-help">
<section class="card">
<h2>Wiring</h2>
<p class="tag">Current rig: <b id="rigname">–</b>. RX pin <b id="rxpin">–</b> takes the rig TXD line, TX pin <b id="txpin">–</b> drives the rig RXD line. Notes for every preset:</p>
<div id="wirings"></div>
<h3>Commands</h3>
<table>
<tr><td><code>status</code></td><td>one line summary</td></tr>
<tr><td><code>freq &lt;hz&gt;</code></td><td>manual frequency, <code>apply</code> re-applies the current one</td></tr>
<tr><td><code>out list</code> / <code>out add udp|relay|line|gpio …</code> / <code>out del &lt;name&gt;</code></td><td>outputs</td></tr>
<tr><td><code>rule list</code> / <code>rule add &lt;out&gt; &lt;fmin&gt; &lt;fmax&gt;</code> / <code>rule del &lt;i&gt;</code> / <code>rule clear</code></td><td>rules</td></tr>
<tr><td><code>ble scan</code> / <code>ble list</code> / <code>ble on|off|refresh &lt;name&gt;</code> / <code>ble send &lt;name&gt; &lt;text&gt;</code></td><td>Bluetooth</td></tr>
<tr><td><code>ant list</code> / <code>ant add &lt;name&gt;</code> / <code>ant del &lt;name&gt;</code> / <code>ant select &lt;name|-&gt;</code> / <code>out assign &lt;out&gt; &lt;antenna|-&gt;</code></td><td>antennas and assignment</td></tr>
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
function ruleAdd(){const o=v('rout'),a=v('rmin'),b=v('rmax');if(!o||!a||!b)return;cmd('rule add '+o+' '+Math.round(a*1000)+' '+Math.round(b*1000));}
function band(a,b){$('rmin').value=a;$('rmax').value=b;}
function saveSettings(){const keys=['proto','catbaud','catrx','cattx','catinv','civaddr','catpoll','catvfo','settle','udpport','wifion','blehold'];(async()=>{let n=0;for(const k of keys){const nv=String(v('s_'+k));if(S&&String(S.settings[k])!==nv){await cmd('set '+k+' '+nv);n++;}}dirty.clear();if(!n)log('nothing changed');poll();})();}
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
const asg='<select class="sm" style="width:110px;padding:4px" onchange="cmd(\'out assign '+o.name+' \'+this.value)"><option value="-"'+(o.antenna?'':' selected')+'>no antenna</option>'+S.ants.map(a=>'<option'+(a===o.antenna?' selected':'')+'>'+esc(a)+'</option>').join('')+'</select>';
return '<div class="out"'+(o.active?'':' style="opacity:.55"')+'><div><div class="n">'+esc(o.name)+'</div><div class="t">'+o.type+'</div></div><div class="st">'+st+'</div><div class="tag" style="flex:1">'+extra+'</div><div>'+asg+' '+btn+'<button class="sm err" onclick="if(confirm(\'Remove '+o.name+' and its rules?\'))cmd(\'out del '+o.name+'\')">✕</button></div></div>';}
function render(s){S=s;
$('freq').textContent=khz(s.freq);$('src').textContent=s.src;$('fmain').textContent=khz(s.cat.main);$('fsub').textContent=khz(s.cat.sub);
const cp=$('catpill');cp.textContent=s.cat.ok?'CAT ok':'CAT no link';cp.className='pill '+(s.cat.ok?'ok':'bad');
$('catst').innerHTML=s.cat.ok?'<span class="ok">linked</span> ('+s.cat.rxcount+' msgs)':'<span class="bad">no answer</span>';
$('txside').textContent=s.cat.tx<0?'?':(s.cat.tx?'sub':'main');$('catlast').textContent=s.cat.last||'–';
const groups=[...s.ants.map(a=>[a,s.outs.filter(o=>o.antenna===a)]),['',s.outs.filter(o=>!s.ants.includes(o.antenna))]];
$('outs').innerHTML=groups.filter(g=>g[1].length).map(g=>'<h3>'+(g[0]?esc(g[0])+(g[0]===s.active?' <span class="ok">active</span>':' <span class="tag">inactive</span>'):'Unassigned (always active)')+'</h3>'+g[1].map(outCard).join('')).join('')||'<span class="tag">no outputs yet, see the Outputs tab</span>';
$('outtip').textContent=s.blebusy?'Bluetooth operation running…':'';
$('scanst').textContent=s.scan.running?'scanning…':(s.scan.hits.length?s.scan.hits.length+' devices':'');
$('scan').innerHTML=s.scan.hits.map(h=>'<tr><td><code>'+h.addr+'</code></td><td>'+(h.atype?'random':'public')+'</td><td>'+esc(h.name)+'</td><td>'+(h.kind===1?'<span class="ok">BR1 relay</span>':(h.kind===2?'<span class="ok">line target</span>':'other'))+'</td><td class="r">'+h.rssi+'</td><td class="r"><button class="sm" onclick="useAddr(\''+h.addr+'\','+h.atype+','+h.kind+')">use</button></td></tr>').join('')||'<tr><td class="tag" colspan="6">no scan result</td></tr>';
$('addrlist').innerHTML=s.scan.hits.map(h=>'<option value="'+h.addr+'">'+esc(h.name)+'</option>').join('');
$('rules').innerHTML=s.rules.map((r,i)=>'<tr><td>'+i+'</td><td>'+esc(r.out)+'</td><td class="r">'+khz(r.fmin)+'</td><td class="r">'+khz(r.fmax)+'</td><td class="r"><button class="sm err" onclick="cmd(\'rule del '+i+'\')">✕</button></td></tr>').join('')||'<tr><td class="tag" colspan="5">no rules</td></tr>';
const ro=$('rout');const cur=ro.value;ro.innerHTML=s.outs.map(o=>'<option>'+esc(o.name)+'</option>').join('');if(cur)ro.value=cur;
$('bands').innerHTML=BANDS.map(b=>'<button class="sm" onclick="band('+b[1]+','+b[2]+')">'+b[0]+'</button>').join(' ');
const w=s.wifi;$('net').innerHTML=w.mode==='sta'?esc(w.ssid)+' · '+w.ip+' · '+w.rssi+' dBm':(w.mode==='ap'?'AP '+w.ap_ssid+' · '+w.ap_ip:w.mode);
$('wifiinfo').innerHTML=(w.mode==='sta'?'Connected to <b>'+esc(w.ssid)+'</b> as '+w.ip+' ('+w.rssi+' dBm), <a href="http://'+w.hostname+'/" style="color:var(--acc)">http://'+w.hostname+'</a>':'Not connected to a network')+(w.ap?'<br>Access point <b>'+w.ap_ssid+'</b> active at '+w.ap_ip:'');
$('apssid').textContent=w.ap_ssid;$('appass').textContent=w.ap_pass;$('host').textContent=w.hostname;$('fw').textContent=s.fw;$('uptime').textContent=fmtUp(s.uptime);$('udpp').textContent=s.settings.udpport;$('udpp2').textContent=s.settings.udpport;
$('nets').innerHTML=w.networks.map(n=>'<tr><td>'+esc(n)+(n===w.ssid?' <span class="ok">●</span>':'')+'</td><td class="r"><button class="err sm" onclick=\'wifiDel('+JSON.stringify(n).replace(/'/g,'&#39;')+')\'>remove</button></td></tr>').join('')||'<tr><td class="tag">no networks stored</td></tr>';
for(const k of ['proto','catbaud','catrx','cattx','catinv','civaddr','catpoll','catvfo','settle','udpport','wifion','blehold'])if(!dirty.has('s_'+k))$('s_'+k).value=s.settings[k];
const rs=$('rigsel');if(rs.options.length!==s.presets.length){rs.innerHTML=s.presets.map(p=>'<option value="'+p.name+'">'+esc(p.rig)+'</option>').join('');rs.value=s.settings.rig;}
const cp2=s.presets.find(p=>p.name===s.settings.rig);$('rigwire').textContent=cp2?cp2.wiring:'custom settings';$('rigname').textContent=cp2?cp2.rig:s.settings.rig;$('rxpin').textContent=s.settings.catrx;$('txpin').textContent=s.settings.cattx;
$('wirings').innerHTML=s.presets.map(p=>'<p class="tag"><b>'+esc(p.rig)+'</b> (<code>rig set '+p.name+'</code>, '+p.proto+', '+p.baud+' Bd): '+esc(p.wiring)+'</p>').join('');
$('ants').innerHTML=s.ants.map(a=>'<div class="out"><div class="n">'+esc(a)+(a===s.active?' <span class="ok">● active</span>':'')+'</div><div class="tag" style="flex:1">'+s.outs.filter(o=>o.antenna===a).map(o=>esc(o.name)).join(', ')+'</div><div>'+(a===s.active?'':'<button class="sm acc" onclick="cmd(\'ant select '+a+'\')">activate</button>')+'<button class="sm err" onclick="if(confirm(\'Remove antenna '+a+'? Its outputs stay, unassigned.\'))cmd(\'ant del '+a+'\')">✕</button></div></div>').join('')||'<span class="tag">no antennas yet</span>';}
async function poll(){try{const r=await fetch('/api/status',{cache:'no-store'});render(await r.json());}catch(e){$('catpill').textContent='offline';$('catpill').className='pill';}}
otype();poll();setInterval(poll,1000);
</script></body></html>
)HTML";
