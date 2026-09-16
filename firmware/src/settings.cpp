#include "settings.h"
#include <Preferences.h>

Settings settings;
static Preferences prefs;
static const char* NS = "antbridge";

const char* outTypeName(uint8_t t) {
    switch (t) {
        case OUT_UDP:   return "udp";
        case OUT_RELAY: return "relay";
        case OUT_LINE:  return "line";
        case OUT_GPIO:  return "gpio";
    }
    return "?";
}

bool outTypeParse(const String& s, uint8_t& t) {
    String l = s; l.toLowerCase();
    if (l == "udp")   { t = OUT_UDP;   return true; }
    if (l == "relay") { t = OUT_RELAY; return true; }
    if (l == "line")  { t = OUT_LINE;  return true; }
    if (l == "gpio")  { t = OUT_GPIO;  return true; }
    return false;
}

void Settings::load() {
    bool migrated = false;
    prefs.begin(NS, true);
    String r = prefs.getString("rig", "ftx1");
    strlcpy(rig, r.c_str(), sizeof(rig));
    catRx     = prefs.getChar("catRx", PIN_CAT_RX);
    catTx     = prefs.getChar("catTx", PIN_CAT_TX);
    catInvert = prefs.getBool("catInv", false);
    civAddr   = prefs.getUChar("civAddr", DEF_CIV_ADDR);
    catBaud   = prefs.getUInt("catBaud", DEF_CAT_BAUD);
    catPollMs = prefs.getUInt("catPoll", DEF_CAT_POLL_MS);
    catVfo    = prefs.getUChar("catVfo", 0);
    settleMs  = prefs.getUInt("settle", DEF_SETTLE_MS);
    udpPort   = prefs.getUShort("udpPort", DEF_UDP_PORT);
    wifiOn    = prefs.getBool("wifiOn", true);
    bleHold   = prefs.getBool("bleHold", true);
    wifiCount = 0;
    for (uint8_t i = 0; i < WIFI_MAX; i++) {
        String ks = "ssid" + String(i), kp = "pass" + String(i);
        String ssid = prefs.getString(ks.c_str(), "");
        if (ssid.length() == 0) continue;
        wifi[wifiCount].ssid = ssid;
        wifi[wifiCount].pass = prefs.getString(kp.c_str(), "");
        wifiCount++;
    }
    outCount = 0;
    size_t len = prefs.getBytesLength("outs");
    if (len > 0 && len % sizeof(Output) == 0 && len <= sizeof(outs)) {
        prefs.getBytes("outs", outs, len);
        outCount = len / sizeof(Output);
    }
    antCount = 0;
    len = prefs.getBytesLength("ants");
    uint8_t av = prefs.getUChar("av", 1);   // layout version of the ants blob
    if (av < 2 && len > 0 && len % 32 == 0 && len <= ANT_MAX * 32) {
        // blob written by the first build of the new layout, before the version key existed:
        // accept it when every record looks like text
        uint8_t buf[sizeof(ants)];
        prefs.getBytes("ants", buf, len);
        bool text = true;
        for (size_t i = 0; i < len && text; i++) {
            size_t off = i % 32;
            uint8_t c = buf[i];
            if (off < 28 && c != 0 && (c < 0x20 || c > 0x7e)) text = false;
        }
        if (text) av = 2;
    }
    if (av >= 3 && len > 0 && len % sizeof(Antenna) == 0 && len <= sizeof(ants)) {
        prefs.getBytes("ants", ants, len);
        antCount = len / sizeof(Antenna);
    } else if (av == 2 && len > 0 && len % 32 == 0 && len / 32 <= ANT_MAX) {
        // version 2: name[16] + type[12] + 4 reserved, no bands
        uint8_t buf[ANT_MAX * 32];
        prefs.getBytes("ants", buf, len);
        antCount = len / 32;
        for (uint8_t i = 0; i < antCount; i++) {
            memset(&ants[i], 0, sizeof(Antenna));
            memcpy(ants[i].name, buf + i * 32, 16);
            memcpy(ants[i].type, buf + i * 32 + 16, 12);
        }
        migrated = true;
    } else if (av < 2 && len > 0 && len % 24 == 0 && len / 24 <= ANT_MAX) {
        // old layout: name[16] + 8 reserved bytes
        uint8_t buf[ANT_MAX * 24];
        prefs.getBytes("ants", buf, len);
        antCount = len / 24;
        for (uint8_t i = 0; i < antCount; i++) {
            memset(&ants[i], 0, sizeof(Antenna));
            memcpy(ants[i].name, buf + i * 24, 16);
        }
        migrated = true;
    }
    String act = prefs.getString("active", "");
    strlcpy(activeAnt, act.c_str(), sizeof(activeAnt));
    ruleCount = 0;
    len = prefs.getBytesLength("rules");
    uint8_t rv = prefs.getUChar("rv", 1);   // layout version of the rules blob
    if (rv >= 3 && len > 0 && len % sizeof(Rule) == 0 && len <= sizeof(rules)) {
        prefs.getBytes("rules", rules, len);
        ruleCount = len / sizeof(Rule);
    } else if (rv == 2 && len > 0 && len % 40 == 0 && len / 40 <= RULE_MAX) {
        // version 2: no state byte, every rule meant "on"
        uint8_t buf[RULE_MAX * 40];
        prefs.getBytes("rules", buf, len);
        ruleCount = len / 40;
        for (uint8_t i = 0; i < ruleCount; i++) {
            memset(&rules[i], 0, sizeof(Rule));
            memcpy(&rules[i], buf + i * 40, 40);
            rules[i].state = 1;
        }
        migrated = true;
    } else if (rv < 2 && len > 0 && len % 24 == 0 && len / 24 <= RULE_MAX) {
        // old layout without antenna: take it from the output it named
        uint8_t buf[RULE_MAX * 24];
        prefs.getBytes("rules", buf, len);
        ruleCount = len / 24;
        for (uint8_t i = 0; i < ruleCount; i++) {
            memset(&rules[i], 0, sizeof(Rule));
            memcpy(&rules[i], buf + i * 24, 24);
            rules[i].state = 1;
            int oi = outIndex(rules[i].out);
            if (oi >= 0) strlcpy(rules[i].ant, outs[oi].legacyAnt, sizeof(rules[i].ant));
        }
        migrated = true;
    }
    prefs.end();
    if (catBaud == 0) catBaud = DEF_CAT_BAUD;
    if (catPollMs < 100) catPollMs = 100;
    if (catVfo > 2) catVfo = 0;
    for (uint8_t i = 0; i < outCount; i++) {
        outs[i].name[sizeof(outs[i].name) - 1] = 0;
        outs[i].host[sizeof(outs[i].host) - 1] = 0;
    }
    for (uint8_t i = 0; i < ruleCount; i++) rules[i].out[sizeof(rules[i].out) - 1] = 0;
    for (uint8_t i = 0; i < antCount; i++) ants[i].name[sizeof(ants[i].name) - 1] = 0;
    // drop antennas without a name (left over from a broken migration)
    { uint8_t w = 0; for (uint8_t i = 0; i < antCount; i++) { if (ants[i].name[0]) ants[w++] = ants[i]; } if (w != antCount) { antCount = w; migrated = true; } }
    // drop rules that name no known output
    { uint8_t w = 0; for (uint8_t i = 0; i < ruleCount; i++) { if (outIndex(rules[i].out) >= 0) rules[w++] = rules[i]; } if (w != ruleCount) { ruleCount = w; migrated = true; } }
    for (uint8_t i = 0; i < outCount; i++) outs[i].legacyAnt[sizeof(outs[i].legacyAnt) - 1] = 0;
    for (uint8_t i = 0; i < ruleCount; i++) rules[i].ant[sizeof(rules[i].ant) - 1] = 0;
    if (migrated) { saveAnts(); saveRules(); }
}

void Settings::save() {
    prefs.begin(NS, false);
    prefs.putString("rig", rig);
    prefs.putChar("catRx", catRx);
    prefs.putChar("catTx", catTx);
    prefs.putBool("catInv", catInvert);
    prefs.putUChar("civAddr", civAddr);
    prefs.putUInt("catBaud", catBaud);
    prefs.putUInt("catPoll", catPollMs);
    prefs.putUChar("catVfo", catVfo);
    prefs.putUInt("settle", settleMs);
    prefs.putUShort("udpPort", udpPort);
    prefs.putBool("wifiOn", wifiOn);
    prefs.putBool("bleHold", bleHold);
    prefs.end();
    saveOuts();
    saveRules();
    saveAnts();
    saveWifi();
}

void Settings::saveAnts() {
    prefs.begin(NS, false);
    if (antCount == 0) prefs.remove("ants");
    else prefs.putBytes("ants", ants, antCount * sizeof(Antenna));
    prefs.putUChar("av", 3);
    prefs.putString("active", activeAnt);
    prefs.end();
}

void Settings::saveOuts() {
    prefs.begin(NS, false);
    if (outCount == 0) prefs.remove("outs");
    else prefs.putBytes("outs", outs, outCount * sizeof(Output));
    prefs.end();
}

void Settings::saveRules() {
    prefs.begin(NS, false);
    if (ruleCount == 0) prefs.remove("rules");
    else prefs.putBytes("rules", rules, ruleCount * sizeof(Rule));
    prefs.putUChar("rv", 3);
    prefs.end();
}

void Settings::saveWifi() {
    prefs.begin(NS, false);
    for (uint8_t i = 0; i < WIFI_MAX; i++) {
        String ks = "ssid" + String(i), kp = "pass" + String(i);
        if (i < wifiCount) {
            prefs.putString(ks.c_str(), wifi[i].ssid);
            prefs.putString(kp.c_str(), wifi[i].pass);
        } else {
            prefs.remove(ks.c_str());
            prefs.remove(kp.c_str());
        }
    }
    prefs.end();
}

int Settings::outIndex(const String& name) const {
    for (uint8_t i = 0; i < outCount; i++)
        if (name.equalsIgnoreCase(outs[i].name)) return i;
    return -1;
}

bool Settings::outAdd(const Output& o) {
    int i = outIndex(o.name);
    if (i >= 0) { outs[i] = o; return true; }
    if (outCount >= OUT_MAX) return false;
    outs[outCount++] = o;
    return true;
}

bool Settings::outDel(const String& name) {
    int i = outIndex(name);
    if (i < 0) return false;
    for (uint8_t j = i; j + 1 < outCount; j++) outs[j] = outs[j + 1];
    outCount--;
    memset(&outs[outCount], 0, sizeof(Output));
    // drop the rules that pointed at it
    uint8_t w = 0;
    for (uint8_t r = 0; r < ruleCount; r++) {
        if (name.equalsIgnoreCase(rules[r].out)) continue;
        rules[w++] = rules[r];
    }
    ruleCount = w;
    return true;
}

bool Settings::ruleAdd(const String& ant, const String& out, uint32_t fmin, uint32_t fmax, bool on) {
    if (ruleCount >= RULE_MAX) return false;
    int i = outIndex(out);
    if (i < 0) return false;
    int a = -1;
    if (ant.length() && ant != "-") { a = antIndex(ant); if (a < 0) return false; }
    if (fmin > fmax) { uint32_t t = fmin; fmin = fmax; fmax = t; }
    Rule& r = rules[ruleCount++];
    memset(&r, 0, sizeof(r));
    strlcpy(r.out, outs[i].name, sizeof(r.out));
    if (a >= 0) strlcpy(r.ant, ants[a].name, sizeof(r.ant));
    r.fmin = fmin;
    r.fmax = fmax;
    r.state = on ? 1 : 0;
    return true;
}

bool Settings::ruleDel(uint8_t i) {
    if (i >= ruleCount) return false;
    for (uint8_t j = i; j + 1 < ruleCount; j++) rules[j] = rules[j + 1];
    ruleCount--;
    return true;
}

bool Settings::ruleActive(const Rule& r) const {
    return r.ant[0] == 0 || strcasecmp(r.ant, activeAnt) == 0;
}

bool Settings::ruleMatch(const char* out, uint32_t hz) const {
    bool on = false;
    for (uint8_t r = 0; r < ruleCount; r++) {
        if (!ruleActive(rules[r]) || strcasecmp(rules[r].out, out) != 0 || hz < rules[r].fmin || hz > rules[r].fmax) continue;
        if (!rules[r].state) return false;   // an explicit off wins
        on = true;
    }
    return on;
}

bool Settings::outUsedBy(const char* out, const char* ant) const {
    for (uint8_t r = 0; r < ruleCount; r++)
        if (strcasecmp(rules[r].out, out) == 0 && strcasecmp(rules[r].ant, ant) == 0) return true;
    return false;
}

int Settings::antIndex(const String& name) const {
    for (uint8_t i = 0; i < antCount; i++)
        if (name.equalsIgnoreCase(ants[i].name)) return i;
    return -1;
}

bool Settings::antAdd(const String& name, const String& type) {
    if (name.length() == 0 || name.length() >= sizeof(ants[0].name) || name == "-") return false;
    int i = antIndex(name);
    if (i >= 0) { if (type.length()) strlcpy(ants[i].type, type.c_str(), sizeof(ants[i].type)); return true; }
    if (antCount >= ANT_MAX) return false;
    memset(&ants[antCount], 0, sizeof(Antenna));
    strlcpy(ants[antCount].name, name.c_str(), sizeof(ants[antCount].name));
    strlcpy(ants[antCount].type, type.c_str(), sizeof(ants[antCount].type));
    antCount++;
    return true;
}

bool Settings::antType(const String& name, const String& type) {
    int i = antIndex(name);
    if (i < 0) return false;
    strlcpy(ants[i].type, type.c_str(), sizeof(ants[i].type));
    return true;
}

bool Settings::antDel(const String& name) {
    int i = antIndex(name);
    if (i < 0) return false;
    for (uint8_t j = i; j + 1 < antCount; j++) ants[j] = ants[j + 1];
    antCount--;
    memset(&ants[antCount], 0, sizeof(Antenna));
    uint8_t w = 0;
    for (uint8_t r = 0; r < ruleCount; r++) {
        if (name.equalsIgnoreCase(rules[r].ant)) continue;
        rules[w++] = rules[r];
    }
    ruleCount = w;
    if (name.equalsIgnoreCase(activeAnt)) activeAnt[0] = 0;
    return true;
}

bool Settings::antRename(const String& name, const String& newName) {
    int i = antIndex(name);
    if (i < 0 || newName.length() == 0 || newName.length() >= sizeof(ants[0].name) || newName == "-") return false;
    int other = antIndex(newName);
    if (other >= 0 && other != i) return false;
    for (uint8_t r = 0; r < ruleCount; r++)
        if (name.equalsIgnoreCase(rules[r].ant)) strlcpy(rules[r].ant, newName.c_str(), sizeof(rules[r].ant));
    if (name.equalsIgnoreCase(activeAnt)) strlcpy(activeAnt, newName.c_str(), sizeof(activeAnt));
    strlcpy(ants[i].name, newName.c_str(), sizeof(ants[i].name));
    return true;
}

bool Settings::antBandAdd(const String& name, uint32_t fmin, uint32_t fmax) {
    int i = antIndex(name);
    if (i < 0) return false;
    Antenna& a = ants[i];
    if (fmin > fmax) { uint32_t t = fmin; fmin = fmax; fmax = t; }
    for (uint8_t b = 0; b < a.bandCount; b++)
        if (a.bands[b].fmin == fmin && a.bands[b].fmax == fmax) return true;
    if (a.bandCount >= 8) return false;
    a.bands[a.bandCount++] = { fmin, fmax };
    // keep them sorted, the UI lists them in that order
    for (uint8_t x = 1; x < a.bandCount; x++) {
        Band key = a.bands[x]; int y = x - 1;
        while (y >= 0 && a.bands[y].fmin > key.fmin) { a.bands[y + 1] = a.bands[y]; y--; }
        a.bands[y + 1] = key;
    }
    return true;
}

bool Settings::antBandDel(const String& name, uint8_t idx) {
    int i = antIndex(name);
    if (i < 0 || idx >= ants[i].bandCount) return false;
    Antenna& a = ants[i];
    for (uint8_t b = idx; b + 1 < a.bandCount; b++) a.bands[b] = a.bands[b + 1];
    a.bandCount--;
    return true;
}

int Settings::antDirect(const Antenna& a, uint32_t hz) const {
    if (a.bandCount == 0) return -1;
    for (uint8_t b = 0; b < a.bandCount; b++)
        if (hz >= a.bands[b].fmin && hz <= a.bands[b].fmax) return 1;
    return 0;
}

bool Settings::antSelect(const String& name) {
    if (name.length() == 0 || name == "-") { activeAnt[0] = 0; return true; }
    int i = antIndex(name);
    if (i < 0) return false;
    strlcpy(activeAnt, ants[i].name, sizeof(activeAnt));
    return true;
}

bool Settings::wifiAdd(const String& ssid, const String& pass) {
    if (ssid.length() == 0 || ssid.length() > 32) return false;
    for (uint8_t i = 0; i < wifiCount; i++) {
        if (wifi[i].ssid == ssid) { wifi[i].pass = pass; saveWifi(); return true; }
    }
    if (wifiCount >= WIFI_MAX) return false;
    wifi[wifiCount].ssid = ssid;
    wifi[wifiCount].pass = pass;
    wifiCount++;
    saveWifi();
    return true;
}

bool Settings::wifiDel(const String& ssid) {
    for (uint8_t i = 0; i < wifiCount; i++) {
        if (wifi[i].ssid == ssid) {
            for (uint8_t j = i; j + 1 < wifiCount; j++) wifi[j] = wifi[j + 1];
            wifiCount--;
            wifi[wifiCount].ssid = ""; wifi[wifiCount].pass = "";
            saveWifi();
            return true;
        }
    }
    return false;
}
