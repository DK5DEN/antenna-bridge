# antenna-bridge

CAT to antenna control bridge on an ESP32-C3 Super Mini. Sits at the rig,
reads the operating frequency over CAT and drives whatever the antennas need
through a small rule table:

- IK6BAK **BR1 Bluetooth relays** (EFHW remote switch), on inside a rule,
  off outside — protocol in [docs/br1-protocol.md](docs/br1-protocol.md)
- **magloop-tune** over WiFi/UDP or over Bluetooth (Nordic UART Service),
  receives `freq <hz>` while the frequency is inside its rule
- any other **Bluetooth UART** target or **UDP** target with the same line
  protocol
- local **GPIO** pins for relays or band data

Everything is configured at runtime (console, UDP, web UI), nothing is
hard-wired to a specific antenna or rig:

- **Antennas** are sets of rules. One antenna is active; only its rules and
  the global rules drive the outputs, so the loop controller and the wire
  antenna relays never act together. An output may appear in the rules of
  several antennas with different ranges. Antennas carry a type (efhw,
  dipole, vertical, loop, beam, wire, other) for the overview.
- **Rig profiles** describe how to talk to a radio: protocol family (line
  based ASCII as Yaesu, Kenwood and Elecraft use it, or Icom CI-V), baud
  rate, which answers carry the frequency, and the wiring of the jack. Seven
  are built in, own ones are small JSON documents stored on the bridge, and
  a catalog on [afu.tools/antenna-bridge](https://afu.tools/antenna-bridge)
  lets people share theirs. Pins and levels are settings, so the same board
  works on the FTX-1 TUNER/LINEAR jack, an RS-232 port through a MAX3232, an
  Elecraft ACC1 or a CI-V bus.
- A manual `freq` command (or the `network` rig preset) replaces CAT when
  the frequency comes from a PC.

## Hardware

| Signal | ESP32-C3 GPIO | Note |
| --- | --- | --- |
| CAT RX | 20 (`set catrx`) | UART0, takes the rig TXD line |
| CAT TX | 21 (`set cattx`) | UART0, drives the rig RXD line |
| LED | 8 | on-board LED: short blink every 2 s with CAT link, every 0.5 s without, solid during a Bluetooth operation |
| GPIO outputs | 0–7, 10 | free for local relays, configured with `out add gpio` |

The console uses the native USB CDC port, UART0 is free for CAT. All rig
jacks need a level adaption to the 3.3 V ESP: 5 V CMOS/TTL (Yaesu jacks,
CI-V) through a BSS138 shifter or a divider, RS-232 through a MAX3232,
Elecraft 3.3 V direct.

### Rig profiles

| Built in | Family | Baud | Jack |
| --- | --- | --- | --- |
| `ftx1` | ascii | 38400 | FTX-1 TUNER/LINEAR (CAT-3), 5 V CMOS, see below |
| `ftdx10` | ascii | 38400 | FT-DX10 rear RS-232, MAX3232 |
| `ft891` | ascii | 4800 | FT-891 CAT/LINEAR mini-DIN, TTL |
| `kenwood` | ascii | 9600 | TS-590/TS-890 COM RS-232, MAX3232, 11 digit frequencies, `AI2;` |
| `elecraft` | ascii | 38400 | KX2/KX3/K3 ACC1, 3.3 V direct |
| `icom` | civ | 19200 | CI-V single wire, open drain; `set civaddr` (IC-705 `a4`, IC-7300 `94`, IC-9700 `a2`) |
| `network` | none | – | no CAT, frequency from UDP/HTTP |

A profile is a JSON document (`firmware/src/rig.h` has the full format):

```json
{"id":"ftx1","name":"Yaesu FTX-1 (TUNER/LINEAR, CAT-3)","author":"DK5DEN","version":1,
 "family":"ascii","baud":38400,"invert":false,"wiring":"how the jack is connected",
 "ascii":{"term":";","poll":"FA;FB;FT;","init":"AI1;","initEvery":10,
          "main":{"prefix":"FA","skip":0,"digits":9},"sub":{"prefix":"FB","skip":0,"digits":9},
          "info":[{"prefix":"IF","skip":5,"digits":9,"to":"main"}],
          "tx":{"prefix":"FT","sub":"1"}},
 "civ":{"addr":"a4","poll":["03","2501","0F"],"main":"03","sub":"2501","split":"0F","transceive":"00"}}
```

The `ascii` family covers every line based CAT dialect: `poll` is sent every
`catpoll` ms, `init` (auto information) every `initEvery` s, answers are
matched by prefix and parsed as `digits` digits after `skip` characters
(`digits` 0 = all remaining), `tx` names the answer that tells which side
transmits. The `civ` family polls the listed commands and accepts transceive
broadcasts. `rig set <id>` uses a profile and takes baud rate, inversion and
CI-V address from it; `set catbaud|catrx|cattx|catinv|civaddr` override
single values afterwards.

Own profiles: `rig import <json>` on the console, `POST /api/rig` with the
document as body, or the Import button on the Settings page; they live in
LittleFS under `/rigs/<id>.json`, a stored profile with the id of a built-in
one replaces it. `rig show [id]` / `GET /api/rig?id=…` exports, `rig del <id>`
removes.

Sharing: the Settings page has "Catalog from afu.tools" (lists released
profiles, one click installs) and "Share on afu.tools" (opens the catalog
page with the current profile prefilled). Submissions are visible to their
submitter at once and to everyone after a release by the site
administration. The server side lives in `afutools/` (FastAPI module,
administration page, site page, deploy script).

### FTX-1 TUNER/LINEAR jack (10-pin mini-DIN, Field head)

| Pin | Use |
| --- | --- |
| +13.8 V OUT | switched with the rig; 13.8 V → 5 V regulator → ESP 5V pin |
| GND | common ground |
| TXD (BAND A) | rig → bridge, 5 V CMOS → level shifter → GPIO20 |
| RXD (BAND B) | bridge → rig, GPIO21 → level shifter → 5 V CMOS |

Use a bidirectional level shifter module (BSS138 type, HV = 5 V from the
regulator, LV = 3.3 V from the ESP). A 2k2/3k3 divider on TXD alone works
for receiving; the ESP 3.3 V TX high level is marginal for a 5 V CMOS input,
so shift that direction as well.

Rig menu: `OPERATION SETTING → GENERAL → TUN/LIN PORT SELECT = CAT-3`,
`CAT-3 RATE = 38400` (default). The jack cannot be used for CAT while an
external tuner, an ATAS or the Optima unit is attached — in the shack with
the Optima, feed the frequency from the PC instead (UDP `freq <hz>` from a
CAT program, or the USB CAT port with a small script).

## Build and flash

PlatformIO, environment `esp32c3_supermini`, upload and monitor on the USB
CDC port (`COM17` in `platformio.ini`, adjust as needed). The partition table
is `huge_app.csv`; NimBLE, WiFi and the web server do not fit the default
1.25 MB app slot.

```
cd firmware
pio run -t upload
pio device monitor
```

## Configuration

Rig, antennas, outputs and rules, on the console, by UDP or in the web UI:

```
rig set ftx1                          # protocol, baud rate, wiring notes

ant add efhw efhw                     # wire antenna with the BR1 switches
ant add loop loop                     # magnetic loop with magloop-tune

ble scan                              # 6 s, then:
ble list                              # ble d1:e2:...  atype=1 rssi=-60 kind=relay name=BR1
out add relay coil80 d1:e2:f3:a4:b5:c6  # BR1 relay, address from the scan
rule add efhw coil80 3500k 3800k      # on the efhw: relay on between 3.5 and 3.8 MHz, off elsewhere

out add udp loopctl magloop.local 4210  # magloop-tune over WiFi
rule add loop loopctl 6.9M 7.3M       # on the loop: gets "freq <hz>" on 40 m

out add line loop-bt 34:85:18:aa:bb:cc  # magloop-tune over Bluetooth (kind=line in the scan)
rule add loop loop-bt 6.9M 7.3M

out add gpio coax 5                   # coax relay selecting the loop feed line
rule add loop coax 0 999M             # on whenever the loop is the active antenna

out add gpio lpf20 6
rule add - lpf20 14000k 14350k        # global rule: with every antenna

ant select efhw                       # switch antennas; loop rules are now inactive
```

The web UI has a frequency map for this: outputs as rows, rules as bars on
a logarithmic axis with the amateur bands shaded; drag bars or their edges
(`rule set`), click into a band to add a rule, ✕ removes one.

Several rules per output are allowed. Relay and GPIO outputs are switched
on when any of their rules covers the frequency and off otherwise. UDP and
line outputs receive `freq <hz>` once per frequency change while inside a
rule. The frequency has to be stable for `settle` ms (default 300) before
anything switches, so turning the dial across a band edge does not chatter
the relays.

Bluetooth relays keep their link open (`blehold`, default on): the BR1
firmware accepts only about every second connection attempt (a PC shows
the same, the phone app hides it behind retries), so connecting per switch
made switching slow and occasionally failed for a minute. With a held link
a switch is a single write and takes 1–2 s; the link runs with slave
latency so the switch wakes as rarely as it would for advertising, and a
lost link is rebuilt with up to four attempts per try. While the bridge
holds the link the phone app cannot connect — `set blehold 0` returns to
connect-per-switch. Line targets keep their connection open as well.

## Command interface

Line-based ASCII, identical on USB serial (115200), UDP (default port 4220,
reply goes back to the sender) and the web console. `help` prints the full
list.

| Command | Meaning |
| --- | --- |
| `status` | frequency, source, active antenna, CAT link, counts, WiFi |
| `freq <hz>` | manual frequency, used until the rig reports a change |
| `apply` | re-apply the current frequency to all outputs |
| `rig` / `rig list` / `rig set <id>` / `rig show [id]` / `rig import <json>` / `rig del <id>` | rig profiles |
| `ant list` / `ant add <name> [type]` / `ant type <name> <type>` / `ant del <name>` / `ant select <name\|->` | antennas |
| `out list` / `out add …` / `out del <name>` | outputs, see above |
| `rule list [antenna\|-]` / `rule add <antenna\|-> <out> <fmin> <fmax>` / `rule set <i> <fmin> <fmax>` / `rule del <i>` / `rule clear` | rules, `-` = global |
| `ble scan` / `ble list` | scan for BR1 relays and UART targets |
| `ble on\|off <name>` / `ble refresh <name>` / `ble send <name> <text>` | manual Bluetooth operations |
| `cat` / `cat FA;` / `cat 03` | CAT link status, raw CAT command (hex bytes for CI-V) |
| `set <key> <value>` | settings, see below |
| `wifi list` / `wifi add <ssid> <pass>` / `wifi del <ssid>` / `wifi ssid <i> <ssid>` | stored networks, max 5 |
| `save`, `reboot` | |

Settings (persisted in NVS):

| Key | Default | Meaning |
| --- | --- | --- |
| `catbaud` | 38400 | CAT baud rate |
| `catrx` / `cattx` | 20 / 21 | UART pins |
| `catinv` | 0 | invert UART levels |
| `civaddr` | a4 | Icom CI-V address of the rig (hex) |
| `catpoll` | 1000 | ms between `FA;FB;FT;` polls; `AI1;` is sent as well so changes arrive immediately |
| `catvfo` | 0 | frequency source: 0 transmitting side (`FT`), 1 main (`FA`), 2 sub (`FB`) |
| `settle` | 300 | ms the frequency must be stable before outputs switch |
| `udpport` | 4220 | own UDP command port (reboot) |
| `wifi` | 1 | 0 keeps the WiFi radio off: portable use, Bluetooth only (reboot) |
| `blehold` | 1 | keep BR1 links open; 0 connects per switch so the phone app stays usable |

## Web UI and WiFi

Port 80, single page (`firmware/src/page.h`): frequency and CAT state,
antennas with type and activate button, outputs with live state (relay
state, link, battery voltage, RSSI, last reply, which antennas use them),
the frequency map, output setup with Bluetooth scan and one-click add,
rules per antenna with band presets, rig profiles with wiring notes, import,
export and the afu.tools catalog, settings, WiFi, help and console. Extra
endpoints: `GET /api/rig?id=…` and `POST /api/rig` for profile documents. HTTP API: `GET /api/status`, `POST /api/cmd` (`line`),
`POST /api/wifi`, `GET /api/scan`.

WiFi behaviour is the same as magloop-tune: up to five stored networks,
strongest first; without a connection 20 s after boot the access point
`antenna-bridge` (password `antenna-bridge`, same as the SSID, 192.168.4.1, captive portal) comes
up. Reachable as `http://antbridge.local` on a network. WiFi is only needed
for UDP targets and the web UI; Bluetooth outputs and CAT work without it,
which is the portable case with the FTX-1 Field.

## Status

2026-09-16: flashed and tested on the bench with two BR1 switches
(`Dennis_01`, `Dennis_02`): scan finds them as `kind=relay`, manual and
rule-driven switching works, relay sense and battery voltage read back
(3.03 V / 2.98 V), UDP forwarding and GPIO outputs follow the rules. A first
connect attempt fails now and then (BR1 advertises every 1.5 s), the second
attempt or the backoff retry catches it. First flash crashed in a loop with
`wifi:Error! Should enable WiFi modem sleep when both WiFi and Bluetooth are
enabled`; modem sleep is now left on. Antennas and rig presets added the
same evening and tested over the HTTP API: `ant select` switches the BR1
relays off and on as expected, `rig set icom|ftx1` reopens the UART with
the new protocol. Open: level shifter wiring and CAT-3 test on the FTX-1,
Icom/Kenwood parsers untested against a rig, magloop-tune over Bluetooth,
web UI clicked through in a browser.

Later that evening: connect-per-switch turned out unreliable. Measured
against `Dennis_01`: ESP32 40–48 % of connect attempts fail with HCI 0x3E
(connection failed to be established), regardless of WiFi on/off,
coexistence preference, TX power or initial connection interval; a Windows
PC with bleak managed 3 of 10. The BR1 itself is the weak part (Zephyr
controller on the RC low-frequency clock). Held links (`blehold`) fix it:
16/16 and 6/6 switches in 1.2–1.8 s, one connection, stable with WiFi and
web polling. Retry backoff is capped at 8 s.
