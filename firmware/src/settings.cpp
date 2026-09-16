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
    if (av < 2 && len > 0 && len % sizeof(Antenna) == 0 && len <= sizeof(ants)) {
        // blob written by the first build of the new layout, before the version key existed:
        // accept it when every record looks like text
        uint8_t buf[sizeof(ants)];
        prefs.getBytes("ants", buf, len);
        bool text = true;
        for (size_t i = 0; i < len && text; i++) {
            size_t off = i % sizeof(Antenna);
            uint8_t c = buf[i];
            if (off < 28 && c != 0 && (c < 0x20 || c > 0x7e)) text = false;
        }
        if (text) av = 2;
    }
    if (av >= 2 && len > 0 && len % sizeof(Antenna) == 0 && len <= sizeof(ants)) {
        prefs.getBytes("ants", ants, len);
        antCount = len / sizeof(Antenna);
    } else if (len > 0 && len % 24 == 0 && len / 24 <= ANT_MAX) {
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
    if (rv >= 2 && len > 0 && len % sizeof(Rule) == 0 && len <= sizeof(rules)) {
        prefs.getBytes("rules", rules, len);
        ruleCount = len / sizeof(Rule);
    } else if (len > 0 && len % 24 == 0 && len / 24 <= RULE_MAX) {
        // old layout without antenna: take it from the output it named
        uint8_t buf[RULE_MAX * 24];
        prefs.getBytes("rules", buf, len);
        ruleCount = len / 24;
        for (uint8_t i = 0; i < ruleCount; i++) {
            memset(&rules[i], 0, sizeof(Rule));
            memcpy(&rules[i], buf + i * 24, 24);
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
    prefs.putUChar("av", 2);
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
    prefs.putUChar("rv", 2);
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

bool Settings::ruleAdd(const String& ant, const String& out, uint32_t fmin, uint32_t fmax) {
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
    for (uint8_t r = 0; r < ruleCount; r++)
        if (ruleActive(rules[r]) && strcasecmp(rules[r].out, out) == 0 && hz >= rules[r].fmin && hz <= rules[r].fmax) return true;
    return false;
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
