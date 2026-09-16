#include "commands.h"
#include "config.h"
#include "settings.h"
#include "cat.h"
#include "engine.h"
#include "ble.h"
#include "net.h"
#include <WiFi.h>

static int tokenize(String& line, String* tok, int maxTok) {
    int n = 0;
    int start = 0;
    while (n < maxTok) {
        while (start < (int)line.length() && line[start] == ' ') start++;
        if (start >= (int)line.length()) break;
        int end = line.indexOf(' ', start);
        if (end < 0) end = line.length();
        tok[n++] = line.substring(start, end);
        start = end;
    }
    return n;
}

static bool parseLong(const String& s, long& v) {
    if (s.length() == 0) return false;
    char* end;
    v = strtol(s.c_str(), &end, 10);
    return *end == 0;
}

bool parseFreq(const String& s, uint32_t& hz) {
    if (s.length() == 0) return false;
    char* end;
    double v = strtod(s.c_str(), &end);
    if (end == s.c_str()) return false;
    if (*end == 'k' || *end == 'K') v *= 1e3;
    else if (*end == 'm' || *end == 'M') v *= 1e6;
    else if (*end != 0) return false;
    if (v < 0 || v > 4e9) return false;
    hz = (uint32_t)llround(v);
    return true;
}

static bool validAddr(String& a) {
    a.toLowerCase();
    if (a.length() != 17) return false;
    for (int i = 0; i < 17; i++) {
        char c = a[i];
        if (i % 3 == 2) { if (c != ':') return false; }
        else if (!isHexadecimalDigit(c)) return false;
    }
    return true;
}

static bool validPin(long p) {
    for (uint8_t x : GPIO_ALLOWED) if (x == p) return true;
    return false;
}

static String ago(uint32_t t) {
    if (!t) return "never";
    uint32_t s = (millis() - t) / 1000;
    if (s < 60) return String(s) + "s";
    if (s < 3600) return String(s / 60) + "m";
    return String(s / 3600) + "h";
}

void printStatus(Print& out) {
    out.printf("freq=%lu src=%s ant=%s cat=%s main=%lu sub=%lu tx=%s catrx=%lu outs=%u rules=%u ble=%s wifi=%s ssid=%s ip=%s rssi=%d ap=%d uptime=%lu\n",
               (unsigned long)engine::freq(), engine::source(), settings.activeAnt[0] ? settings.activeAnt : "-", cat::linkOk() ? "ok" : "no-link",
               (unsigned long)cat::freqMain(), (unsigned long)cat::freqSub(),
               cat::txSide() < 0 ? "?" : (cat::txSide() ? "sub" : "main"), (unsigned long)cat::rxCount(),
               settings.outCount, settings.ruleCount, ble::busy() ? "busy" : "idle",
               net::stateText().c_str(),
               net::staConnected() ? WiFi.SSID().c_str() : "-",
               net::ip().toString().c_str(),
               net::staConnected() ? WiFi.RSSI() : 0,
               net::apActive() ? 1 : 0,
               (unsigned long)(millis() / 1000));
}

void printOutputs(Print& out) {
    uint32_t f = engine::freq();
    for (uint8_t i = 0; i < settings.outCount; i++) {
        const Output& o = settings.outs[i];
        bool match = f && settings.ruleMatch(o.name, f);
        out.printf("out %s type=%s ants=", o.name, outTypeName(o.type));
        bool first = true;
        for (uint8_t a = 0; a < settings.antCount; a++) {
            if (!settings.outUsedBy(o.name, settings.ants[a].name)) continue;
            out.printf("%s%s", first ? "" : ",", settings.ants[a].name);
            first = false;
        }
        if (settings.outUsedBy(o.name, "")) { out.printf("%s*", first ? "" : ","); first = false; }
        out.printf("%s ", first ? "-" : "");
        if (o.type == OUT_UDP) {
            out.printf("host=%s port=%u sent=%lu reply=\"%s\" err=%s\n", o.host, o.port,
                       (unsigned long)engine::lastSent(i), engine::lastUdpReply(i).c_str(),
                       engine::udpError(i).length() ? engine::udpError(i).c_str() : "-");
        } else if (o.type == OUT_RELAY) {
            ble::DevState s = ble::state(i);
            out.printf("addr=%s atype=%u want=%s state=%s sense=%s batt=%.2f rssi=%d lastok=%s link=%u conn=%u/%u err=%s\n", o.host, o.addrType,
                       s.desired < 0 ? "-" : (s.desired ? "on" : "off"),
                       s.actual < 0 ? "?" : (s.actual ? "on" : "off"),
                       s.sense < 0 ? "?" : (s.sense ? "1" : "0"),
                       s.battCv / 100.0, s.rssi, ago(s.lastOk).c_str(), s.link, s.tries - s.connFails, s.tries, s.err[0] ? s.err : "-");
        } else if (o.type == OUT_LINE) {
            ble::DevState s = ble::state(i);
            out.printf("addr=%s atype=%u conn=%d sent=%lu reply=\"%s\" rssi=%d lastok=%s err=%s\n", o.host, o.addrType,
                       s.actual == 1 ? 1 : 0, (unsigned long)engine::lastSent(i), s.reply, s.rssi,
                       ago(s.lastOk).c_str(), s.err[0] ? s.err : "-");
        } else {
            out.printf("pin=%u inv=%u state=%s\n", o.pin, o.invert, match ? "on" : "off");
        }
    }
}

static void printHelp(Print& out) {
    out.println(F("antenna-bridge commands (frequencies in Hz, suffix k or M allowed):"));
    out.println(F("  status                       one line summary"));
    out.println(F("  freq <hz>                    set frequency by hand (CAT overrides on next change)"));
    out.println(F("  apply                        re-apply current frequency to all outputs"));
    out.println(F("  out list                     outputs with live state"));
    out.println(F("  out add udp <name> <host> <port>        udp target, gets 'freq <hz>'"));
    out.println(F("  out add relay <name> <addr> [atype]     BR1 bluetooth relay (addr from ble scan)"));
    out.println(F("  out add line <name> <addr> [atype]      bluetooth uart target, gets 'freq <hz>'"));
    out.println(F("  out add gpio <name> <pin> [inv]         local pin, high inside its rules"));
    out.println(F("  out del <name>               remove output and its rules"));
    out.println(F("  ant list                     antennas with type, * marks the active one"));
    out.println(F("  ant add <name> [type]        type: efhw, dipole, vertical, loop, beam, wire, other"));
    out.println(F("  ant type <name> <type> | ant del <name>"));
    out.println(F("  ant select <name|->          activate an antenna; only its rules and the global ones (-) drive the outputs"));
    out.println(F("  rule list [antenna|-]        rules, optionally only those of one antenna (- = global)"));
    out.println(F("  rule add <antenna|-> <out> <fmin> <fmax>   output is on between fmin and fmax while that antenna is active"));
    out.println(F("  rule set <i> <fmin> <fmax> | rule del <i> | rule clear"));
    out.println(F("  ble scan                     scan 6 s for BR1 relays and uart targets"));
    out.println(F("  ble list                     last scan result"));
    out.println(F("  ble on|off <name>            switch a relay by hand"));
    out.println(F("  ble refresh <name>           read relay state and battery, reconnect line"));
    out.println(F("  ble send <name> <text>       send a line to a uart target"));
    out.println(F("  cat                          CAT link status"));
    out.println(F("  cat <cmd>                    raw CAT command, e.g. cat FA; (hex bytes for CI-V)"));
    out.println(F("  rig                          current rig and wiring notes"));
    out.println(F("  rig list                     rig presets"));
    out.println(F("  rig set <preset>             apply a preset (protocol, baud rate, CI-V address)"));
    out.println(F("  set                          show settings"));
    out.println(F("  set proto none|yaesu|kenwood|icom   CAT protocol family"));
    out.println(F("  set catbaud <bps>            CAT baud rate"));
    out.println(F("  set catrx <pin> | set cattx <pin>   UART pins (default 20/21)"));
    out.println(F("  set catinv 0|1               invert UART levels"));
    out.println(F("  set civaddr <hex>            Icom CI-V address of the rig"));
    out.println(F("  set catpoll <ms>             CAT poll interval"));
    out.println(F("  set catvfo 0|1|2             which side sets the frequency: 0 transmitting side (FT), 1 main, 2 sub"));
    out.println(F("  set settle <ms>              frequency must be stable this long before outputs switch"));
    out.println(F("  set udpport <port>           own udp command port (needs reboot)"));
    out.println(F("  set wifi 0|1                 radio off for portable use, Bluetooth only (needs reboot)"));
    out.println(F("  set blehold 0|1              keep BR1 links open (default 1, reliable and fast) or connect per switch (phone app usable)"));
    out.println(F("  wifi list | wifi add <ssid> <pass> | wifi del <ssid> | wifi ssid <i> <ssid>"));
    out.println(F("  save | reboot | help"));
}

static void printSettings(Print& out) {
    const Settings& s = settings;
    out.printf("rig=%s proto=%s catbaud=%lu catrx=%d cattx=%d catinv=%d civaddr=%02x catpoll=%lu catvfo=%u settle=%lu udpport=%u wifion=%d blehold=%d wifi=%u outs=%u rules=%u ants=%u\n",
               s.rig, protoName(s.proto), (unsigned long)s.catBaud, s.catRx, s.catTx, s.catInvert, s.civAddr,
               (unsigned long)s.catPollMs, s.catVfo, (unsigned long)s.settleMs,
               s.udpPort, s.wifiOn, s.bleHold, s.wifiCount, s.outCount, s.ruleCount, s.antCount);
}

static void cmdOut(String* t, int n, Print& out) {
    String sub = n > 1 ? t[1] : "list";
    sub.toLowerCase();
    if (sub == "list") {
        printOutputs(out);
        out.printf("OK %u outputs\n", settings.outCount);
        return;
    }
    if (sub == "del" && n >= 3) {
        if (!settings.outDel(t[2])) { out.println("ERR not found"); return; }
        settings.saveOuts();
        settings.saveRules();
        engine::outputsChanged();
        out.println("OK removed");
        return;
    }
    if (sub != "add" || n < 5) {
        out.println("ERR usage: out list | out del <name> | out add udp <name> <host> <port> | out add relay|line <name> <addr> [atype] | out add gpio <name> <pin> [inv]");
        return;
    }
    Output o;
    memset(&o, 0, sizeof(o));
    if (!outTypeParse(t[2], o.type)) { out.println("ERR type udp|relay|line|gpio"); return; }
    if (t[3].length() == 0 || t[3].length() >= sizeof(o.name)) { out.println("ERR name 1..15 chars"); return; }
    strlcpy(o.name, t[3].c_str(), sizeof(o.name));
    long v;
    if (o.type == OUT_UDP) {
        if (n < 6 || !parseLong(t[5], v) || v < 1 || v > 65535) { out.println("ERR usage: out add udp <name> <host> <port>"); return; }
        if (t[4].length() >= sizeof(o.host)) { out.println("ERR host too long"); return; }
        strlcpy(o.host, t[4].c_str(), sizeof(o.host));
        o.port = v;
    } else if (o.type == OUT_RELAY || o.type == OUT_LINE) {
        String a = t[4];
        if (!validAddr(a)) { out.println("ERR address aa:bb:cc:dd:ee:ff"); return; }
        strlcpy(o.host, a.c_str(), sizeof(o.host));
        o.addrType = o.type == OUT_RELAY ? 1 : 0;   // BR1: random static, ESP32: public
        if (n >= 6) {
            if (!parseLong(t[5], v) || v < 0 || v > 1) { out.println("ERR atype 0|1"); return; }
            o.addrType = v;
        }
    } else {
        if (!parseLong(t[4], v) || !validPin(v)) { out.println("ERR pin 0-7 or 10"); return; }
        o.pin = v;
        if (n >= 6) { long inv; if (!parseLong(t[5], inv)) { out.println("ERR inv 0|1"); return; } o.invert = inv != 0; }
    }
    if (!settings.outAdd(o)) { out.println("ERR list full (8)"); return; }
    settings.saveOuts();
    engine::outputsChanged();
    out.printf("OK out %s type=%s\n", o.name, outTypeName(o.type));
}

static void cmdRule(String* t, int n, Print& out) {
    String sub = n > 1 ? t[1] : "list";
    sub.toLowerCase();
    if (sub == "list") {
        String filt = n > 2 ? t[2] : "";
        uint8_t shown = 0;
        for (uint8_t i = 0; i < settings.ruleCount; i++) {
            const Rule& r = settings.rules[i];
            const char* ant = r.ant[0] ? r.ant : "-";
            if (filt.length() && !filt.equalsIgnoreCase(ant)) continue;
            out.printf("rule %u %s %s %lu %lu%s\n", i, ant, r.out, (unsigned long)r.fmin, (unsigned long)r.fmax,
                       settings.ruleActive(r) ? "" : " (inactive)");
            shown++;
        }
        out.printf("OK %u rules\n", shown);
    } else if (sub == "add" && n >= 6) {
        uint32_t a, b;
        if (!parseFreq(t[4], a) || !parseFreq(t[5], b)) { out.println("ERR frequencies"); return; }
        if (settings.outIndex(t[3]) < 0) { out.println("ERR unknown output"); return; }
        if (t[2] != "-" && settings.antIndex(t[2]) < 0) { out.println("ERR unknown antenna"); return; }
        if (!settings.ruleAdd(t[2], t[3], a, b)) { out.println("ERR list full (32)"); return; }
        settings.saveRules();
        engine::applyNow();
        const Rule& r = settings.rules[settings.ruleCount - 1];
        out.printf("OK rule %u %s %s %lu %lu\n", settings.ruleCount - 1, r.ant[0] ? r.ant : "-", r.out, (unsigned long)a, (unsigned long)b);
    } else if (sub == "set" && n >= 5) {
        long i; uint32_t a, b;
        if (!parseLong(t[2], i) || i < 0 || i >= settings.ruleCount) { out.println("ERR index"); return; }
        if (!parseFreq(t[3], a) || !parseFreq(t[4], b)) { out.println("ERR frequencies"); return; }
        if (a > b) { uint32_t x = a; a = b; b = x; }
        settings.rules[i].fmin = a;
        settings.rules[i].fmax = b;
        settings.saveRules();
        engine::applyNow();
        out.printf("OK rule %ld %s %s %lu %lu\n", i, settings.rules[i].ant[0] ? settings.rules[i].ant : "-", settings.rules[i].out, (unsigned long)a, (unsigned long)b);
    } else if (sub == "del" && n >= 3) {
        long i;
        if (!parseLong(t[2], i) || !settings.ruleDel(i)) { out.println("ERR index"); return; }
        settings.saveRules();
        engine::applyNow();
        out.println("OK removed");
    } else if (sub == "clear") {
        settings.ruleClear();
        settings.saveRules();
        engine::applyNow();
        out.println("OK rules cleared");
    } else {
        out.println("ERR usage: rule list [antenna|-] | rule add <antenna|-> <out> <fmin> <fmax> | rule set <i> <fmin> <fmax> | rule del <i> | rule clear");
    }
}

static void cmdBle(String* t, int n, Print& out, const String& line) {
    String sub = n > 1 ? t[1] : "list";
    sub.toLowerCase();
    if (sub == "scan") {
        ble::scanStart();
        out.println("OK scanning 6 s, then: ble list");
    } else if (sub == "list") {
        ble::ScanHit hits[16];
        int m = ble::scanResults(hits, 16);
        for (int i = 0; i < m; i++)
            out.printf("ble %s atype=%u rssi=%d kind=%s name=%s\n", hits[i].addr, hits[i].type, hits[i].rssi,
                       hits[i].kind == 1 ? "relay" : (hits[i].kind == 2 ? "line" : "other"), hits[i].name);
        out.printf("OK %d devices%s\n", m, ble::scanning() ? " (scan running)" : "");
    } else if ((sub == "on" || sub == "off" || sub == "refresh" || sub == "send") && n >= 3) {
        int i = settings.outIndex(t[2]);
        if (i < 0) { out.println("ERR unknown output"); return; }
        uint8_t type = settings.outs[i].type;
        if (sub == "refresh") {
            if (type != OUT_RELAY && type != OUT_LINE) { out.println("ERR not a bluetooth output"); return; }
            ble::refresh(i);
            out.println("OK refresh queued");
        } else if (sub == "send") {
            if (type != OUT_LINE) { out.println("ERR not a line output"); return; }
            int p = line.indexOf(t[2], line.indexOf(t[1]) + t[1].length());
            String text = line.substring(p + t[2].length());
            text.trim();
            if (!text.length()) { out.println("ERR text"); return; }
            ble::sendLine(i, text);
            out.println("OK queued");
        } else {
            if (type != OUT_RELAY) { out.println("ERR not a relay output"); return; }
            ble::setDesired(i, sub == "on");
            out.printf("OK %s %s queued\n", settings.outs[i].name, sub.c_str());
        }
    } else {
        out.println("ERR usage: ble scan | ble list | ble on|off|refresh <name> | ble send <name> <text>");
    }
}

static void cmdAnt(String* t, int n, Print& out) {
    String sub = n > 1 ? t[1] : "list";
    sub.toLowerCase();
    if (sub == "list") {
        for (uint8_t i = 0; i < settings.antCount; i++) {
            const Antenna& a = settings.ants[i];
            out.printf("ant %s%s type=%s outs=", a.name, strcasecmp(a.name, settings.activeAnt) == 0 ? " *" : "", a.type[0] ? a.type : "-");
            bool first = true;
            for (uint8_t o = 0; o < settings.outCount; o++) {
                if (!settings.outUsedBy(settings.outs[o].name, a.name)) continue;
                out.printf("%s%s", first ? "" : ",", settings.outs[o].name);
                first = false;
            }
            out.println(first ? "-" : "");
        }
        out.printf("OK %u antennas, active=%s\n", settings.antCount, settings.activeAnt[0] ? settings.activeAnt : "-");
    } else if (sub == "add" && n >= 3) {
        if (!settings.antAdd(t[2], n > 3 ? t[3] : "")) { out.println("ERR name 1..15 chars or list full (8)"); return; }
        settings.saveAnts();
        out.printf("OK ant %s\n", t[2].c_str());
    } else if (sub == "type" && n >= 4) {
        if (!settings.antType(t[2], t[3])) { out.println("ERR unknown antenna"); return; }
        settings.saveAnts();
        out.printf("OK ant %s type=%s\n", t[2].c_str(), t[3].c_str());
    } else if (sub == "del" && n >= 3) {
        if (!settings.antDel(t[2])) { out.println("ERR not found"); return; }
        settings.saveAnts();
        settings.saveRules();
        engine::applyNow();
        out.println("OK removed, its rules too");
    } else if (sub == "select" && n >= 3) {
        if (!settings.antSelect(t[2])) { out.println("ERR unknown antenna"); return; }
        settings.saveAnts();
        engine::applyNow();
        out.printf("OK active=%s\n", settings.activeAnt[0] ? settings.activeAnt : "-");
    } else {
        out.println("ERR usage: ant list | ant add <name> [type] | ant type <name> <type> | ant del <name> | ant select <name|->");
    }
}

static void cmdSet(String* t, int n, Print& out) {
    Settings& s = settings;
    if (n < 3) { printSettings(out); return; }
    String key = t[1]; key.toLowerCase();
    long v = 0;
    if (key != "proto" && key != "civaddr" && !parseLong(t[2], v)) { out.println("ERR value"); return; }

    if (key == "proto") {
        uint8_t p;
        if (!protoParse(t[2], p)) { out.println("ERR none|yaesu|kenwood|icom"); return; }
        s.proto = p; strlcpy(s.rig, "custom", sizeof(s.rig)); cat::restart();
    } else if (key == "catrx" || key == "cattx") {
        if (!validPin(v)) { out.println("ERR pin 0-7, 10, 20, 21"); return; }
        if (key == "catrx") s.catRx = v; else s.catTx = v;
        cat::restart();
    } else if (key == "catinv") {
        s.catInvert = v != 0; cat::restart();
    } else if (key == "civaddr") {
        long a = strtol(t[2].c_str(), nullptr, 16);
        if (a < 1 || a > 0xDF) { out.println("ERR hex 01..df"); return; }
        s.civAddr = a;
    } else if (key == "catbaud") {
        if (v < 1200 || v > 115200) { out.println("ERR 1200..115200"); return; }
        s.catBaud = v; cat::restart();
    } else if (key == "catpoll") {
        if (v < 100 || v > 60000) { out.println("ERR 100..60000"); return; }
        s.catPollMs = v;
    } else if (key == "catvfo") {
        if (v < 0 || v > 2) { out.println("ERR 0..2"); return; }
        s.catVfo = v;
    } else if (key == "settle") {
        if (v < 0 || v > 10000) { out.println("ERR 0..10000"); return; }
        s.settleMs = v;
    } else if (key == "wifi") {
        s.wifiOn = v != 0;
    } else if (key == "blehold") {
        s.bleHold = v != 0;
    } else if (key == "udpport") {
        if (v < 1 || v > 65534) { out.println("ERR 1..65534"); return; }
        s.udpPort = v;
    } else {
        out.println("ERR unknown key");
        return;
    }
    s.save();
    out.print("OK ");
    printSettings(out);
}

void handleCommand(String line, Print& out) {
    line.trim();
    if (line.length() == 0) return;
    String t[7];
    int n = tokenize(line, t, 7);
    String cmd = t[0];
    cmd.toLowerCase();

    if (cmd == "help" || cmd == "?") {
        printHelp(out);
    } else if (cmd == "status" || cmd == "s") {
        printStatus(out);
    } else if (cmd == "freq" || cmd == "f") {
        uint32_t hz;
        if (n < 2 || !parseFreq(t[1], hz)) { out.println("ERR usage: freq <hz>"); return; }
        engine::setFreq(hz, "manual");
        out.printf("OK freq %lu\n", (unsigned long)hz);
    } else if (cmd == "apply") {
        engine::applyNow();
        out.println("OK");
    } else if (cmd == "out") {
        cmdOut(t, n, out);
    } else if (cmd == "rule") {
        cmdRule(t, n, out);
    } else if (cmd == "ant") {
        cmdAnt(t, n, out);
    } else if (cmd == "ble") {
        cmdBle(t, n, out, line);
    } else if (cmd == "cat") {
        if (n >= 2) {
            String raw = line.substring(line.indexOf(t[1]));
            raw.trim();
            if (settings.proto != PROTO_ICOM && !raw.endsWith(";")) raw += ";";
            cat::send(raw);
            out.printf("OK sent %s\n", raw.c_str());
        } else {
            out.printf("cat=%s rig=%s proto=%s baud=%lu rxage=%lums rxcount=%lu main=%lu sub=%lu tx=%s last=%s\n",
                       cat::linkOk() ? "ok" : "no-link", settings.rig, protoName(settings.proto), (unsigned long)settings.catBaud,
                       (unsigned long)cat::lastRxAgeMs(), (unsigned long)cat::rxCount(),
                       (unsigned long)cat::freqMain(), (unsigned long)cat::freqSub(),
                       cat::txSide() < 0 ? "?" : (cat::txSide() ? "sub" : "main"), cat::lastMessage().c_str());
        }
    } else if (cmd == "rig") {
        String sub = n > 1 ? t[1] : ""; sub.toLowerCase();
        uint8_t m;
        const cat::Preset* p = cat::presets(m);
        if (sub == "list") {
            for (uint8_t i = 0; i < m; i++)
                out.printf("rig %-9s proto=%-7s baud=%-6lu %s\n", p[i].name, protoName(p[i].proto), (unsigned long)p[i].baud, p[i].rigName);
            out.printf("OK %u presets, current=%s\n", m, settings.rig);
        } else if (sub == "set" && n >= 3) {
            if (!cat::applyPreset(t[2])) { out.println("ERR unknown preset, see rig list"); return; }
            settings.save();
            out.printf("OK rig=%s proto=%s baud=%lu civaddr=%02x\n", settings.rig, protoName(settings.proto), (unsigned long)settings.catBaud, settings.civAddr);
        } else if (sub == "") {
            const cat::Preset* cur = cat::preset(settings.rig);
            out.printf("rig=%s proto=%s baud=%lu rx=%d tx=%d inv=%d civaddr=%02x link=%s\n", settings.rig, protoName(settings.proto),
                       (unsigned long)settings.catBaud, settings.catRx, settings.catTx, settings.catInvert, settings.civAddr, cat::linkOk() ? "ok" : "no");
            if (cur) { out.println(cur->rigName); out.println(cur->wiring); }
        } else {
            out.println("ERR usage: rig | rig list | rig set <preset>");
        }
    } else if (cmd == "set") {
        cmdSet(t, n, out);
    } else if (cmd == "wifi") {
        String sub = n > 1 ? t[1] : "list"; sub.toLowerCase();
        if (sub == "list") {
            for (uint8_t i = 0; i < settings.wifiCount; i++)
                out.printf("wifi %u %s%s\n", i, settings.wifi[i].ssid.c_str(),
                           net::staConnected() && WiFi.SSID() == settings.wifi[i].ssid ? " (connected)" : "");
            out.printf("OK %u networks, mode=%s ip=%s\n", settings.wifiCount, net::stateText().c_str(), net::ip().toString().c_str());
        } else if (sub == "add" && n >= 3) {
            // password is the rest of the line, may contain spaces
            int i = line.indexOf(t[2], line.indexOf(t[1]) + t[1].length());
            String pass = line.substring(i + t[2].length()); pass.trim();
            if (!settings.wifiAdd(t[2], pass)) { out.println("ERR ssid invalid or list full (5)"); return; }
            net::reconnect();
            out.printf("OK wifi stored %s, connecting\n", t[2].c_str());
        } else if (sub == "ssid" && n >= 4) {
            long i;
            if (!parseLong(t[2], i) || i < 0 || i >= settings.wifiCount) { out.println("ERR index"); return; }
            settings.wifi[i].ssid = t[3];
            settings.saveWifi();
            net::reconnect();
            out.printf("OK wifi %ld renamed to %s, connecting\n", i, t[3].c_str());
        } else if (sub == "del" && n >= 3) {
            if (!settings.wifiDel(t[2])) { out.println("ERR not found"); return; }
            net::reconnect();
            out.println("OK removed");
        } else {
            out.println("ERR usage: wifi list | wifi add <ssid> <pass> | wifi del <ssid> | wifi ssid <i> <ssid>");
        }
    } else if (cmd == "save") {
        settings.save();
        out.println("OK saved");
    } else if (cmd == "reboot") {
        out.println("OK rebooting");
        delay(100);
        ESP.restart();
    } else {
        out.println("ERR unknown command, try help");
    }
}
