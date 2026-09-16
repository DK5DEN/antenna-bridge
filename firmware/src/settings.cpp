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

const char* protoName(uint8_t p) {
    switch (p) {
        case PROTO_NONE:    return "none";
        case PROTO_YAESU:   return "yaesu";
        case PROTO_KENWOOD: return "kenwood";
        case PROTO_ICOM:    return "icom";
    }
    return "?";
}

bool protoParse(const String& s, uint8_t& p) {
    String l = s; l.toLowerCase();
    if (l == "none")    { p = PROTO_NONE;    return true; }
    if (l == "yaesu")   { p = PROTO_YAESU;   return true; }
    if (l == "kenwood" || l == "elecraft") { p = PROTO_KENWOOD; return true; }
    if (l == "icom" || l == "civ") { p = PROTO_ICOM; return true; }
    return false;
}

void Settings::load() {
    prefs.begin(NS, true);
    String r = prefs.getString("rig", "ftx1");
    strlcpy(rig, r.c_str(), sizeof(rig));
    proto     = prefs.getUChar("proto", PROTO_YAESU);
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
    if (len > 0 && len % sizeof(Antenna) == 0 && len <= sizeof(ants)) {
        prefs.getBytes("ants", ants, len);
        antCount = len / sizeof(Antenna);
    }
    String act = prefs.getString("active", "");
    strlcpy(activeAnt, act.c_str(), sizeof(activeAnt));
    ruleCount = 0;
    len = prefs.getBytesLength("rules");
    if (len > 0 && len % sizeof(Rule) == 0 && len <= sizeof(rules)) {
        prefs.getBytes("rules", rules, len);
        ruleCount = len / sizeof(Rule);
    }
    prefs.end();
    if (catBaud == 0) catBaud = DEF_CAT_BAUD;
    if (catPollMs < 100) catPollMs = 100;
    if (catVfo > 2) catVfo = 0;
    if (proto > PROTO_ICOM) proto = PROTO_YAESU;
    for (uint8_t i = 0; i < outCount; i++) {
        outs[i].name[sizeof(outs[i].name) - 1] = 0;
        outs[i].host[sizeof(outs[i].host) - 1] = 0;
    }
    for (uint8_t i = 0; i < ruleCount; i++) rules[i].out[sizeof(rules[i].out) - 1] = 0;
    for (uint8_t i = 0; i < antCount; i++) ants[i].name[sizeof(ants[i].name) - 1] = 0;
    for (uint8_t i = 0; i < outCount; i++) outs[i].antenna[sizeof(outs[i].antenna) - 1] = 0;
}

void Settings::save() {
    prefs.begin(NS, false);
    prefs.putString("rig", rig);
    prefs.putUChar("proto", proto);
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

bool Settings::ruleAdd(const String& out, uint32_t fmin, uint32_t fmax) {
    if (ruleCount >= RULE_MAX) return false;
    int i = outIndex(out);
    if (i < 0) return false;
    if (fmin > fmax) { uint32_t t = fmin; fmin = fmax; fmax = t; }
    Rule& r = rules[ruleCount++];
    memset(&r, 0, sizeof(r));
    strlcpy(r.out, outs[i].name, sizeof(r.out));
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

bool Settings::ruleMatch(const char* out, uint32_t hz) const {
    for (uint8_t r = 0; r < ruleCount; r++)
        if (strcasecmp(rules[r].out, out) == 0 && hz >= rules[r].fmin && hz <= rules[r].fmax) return true;
    return false;
}

int Settings::antIndex(const String& name) const {
    for (uint8_t i = 0; i < antCount; i++)
        if (name.equalsIgnoreCase(ants[i].name)) return i;
    return -1;
}

bool Settings::antAdd(const String& name) {
    if (name.length() == 0 || name.length() >= sizeof(ants[0].name) || name == "-") return false;
    if (antIndex(name) >= 0) return true;
    if (antCount >= ANT_MAX) return false;
    memset(&ants[antCount], 0, sizeof(Antenna));
    strlcpy(ants[antCount].name, name.c_str(), sizeof(ants[antCount].name));
    antCount++;
    return true;
}

bool Settings::antDel(const String& name) {
    int i = antIndex(name);
    if (i < 0) return false;
    for (uint8_t j = i; j + 1 < antCount; j++) ants[j] = ants[j + 1];
    antCount--;
    memset(&ants[antCount], 0, sizeof(Antenna));
    for (uint8_t o = 0; o < outCount; o++)
        if (name.equalsIgnoreCase(outs[o].antenna)) outs[o].antenna[0] = 0;
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

bool Settings::outAssign(const String& out, const String& ant) {
    int o = outIndex(out);
    if (o < 0) return false;
    if (ant.length() == 0 || ant == "-") { outs[o].antenna[0] = 0; return true; }
    int a = antIndex(ant);
    if (a < 0) return false;
    strlcpy(outs[o].antenna, ants[a].name, sizeof(outs[o].antenna));
    return true;
}

bool Settings::outActive(const Output& o) const {
    if (o.antenna[0] == 0) return true;
    return strcasecmp(o.antenna, activeAnt) == 0;
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
