#include "rig.h"
#include "config.h"
#include "settings.h"
#include "cat.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

namespace {

const char* BUILTIN[] PROGMEM = {
R"J({"id":"ftx1","name":"Yaesu FTX-1 (TUNER/LINEAR, CAT-3)","author":"DK5DEN","version":1,"family":"ascii","baud":38400,"invert":false,
"wiring":"10-pin mini-DIN on the Field head: TXD (BAND A) -> level shifter -> RX pin, RXD (BAND B) <- level shifter <- TX pin, +13.8 V OUT, GND. 5 V CMOS, use a BSS138 shifter. Menu OPERATION SETTING > GENERAL > TUN/LIN PORT SELECT = CAT-3, CAT-3 RATE 38400. Not usable while a tuner, ATAS or the Optima is on the jack.",
"ascii":{"term":";","poll":"FA;FB;FT;","init":"AI1;","initEvery":10,"main":{"prefix":"FA","skip":0,"digits":9},"sub":{"prefix":"FB","skip":0,"digits":9},"info":[{"prefix":"IF","skip":5,"digits":9,"to":"main"},{"prefix":"OI","skip":5,"digits":9,"to":"sub"}],"tx":{"prefix":"FT","sub":"1"}}})J",
R"J({"id":"ftdx10","name":"Yaesu FT-DX10 (rear RS-232)","author":"DK5DEN","version":1,"family":"ascii","baud":38400,"invert":false,
"wiring":"DE-9 RS-232: pin 2 TXD -> MAX3232 -> RX pin, pin 3 RXD <- MAX3232 <- TX pin, pin 5 GND. Menu CAT RATE 38400, CAT RTS OFF.",
"ascii":{"term":";","poll":"FA;FB;FT;","init":"AI1;","initEvery":10,"main":{"prefix":"FA","skip":0,"digits":9},"sub":{"prefix":"FB","skip":0,"digits":9},"info":[{"prefix":"IF","skip":5,"digits":9,"to":"main"},{"prefix":"OI","skip":5,"digits":9,"to":"sub"}],"tx":{"prefix":"FT","sub":"1"}}})J",
R"J({"id":"ft891","name":"Yaesu FT-891 (CAT/LINEAR jack, TTL)","author":"DK5DEN","version":1,"family":"ascii","baud":4800,"invert":false,
"wiring":"8-pin mini-DIN CAT/LINEAR: TXD -> RX pin, RXD <- TX pin, GND. 5 V TTL, use a level shifter. Menu CAT RATE (default 4800).",
"ascii":{"term":";","poll":"FA;FB;FT;","init":"AI1;","initEvery":10,"main":{"prefix":"FA","skip":0,"digits":9},"sub":{"prefix":"FB","skip":0,"digits":9},"info":[{"prefix":"IF","skip":5,"digits":9,"to":"main"}],"tx":{"prefix":"FT","sub":"1"}}})J",
R"J({"id":"kenwood","name":"Kenwood TS-590 / TS-890 (COM RS-232)","author":"DK5DEN","version":1,"family":"ascii","baud":9600,"invert":false,
"wiring":"DE-9 RS-232 via MAX3232 (TXD pin 2 -> RX pin, RXD pin 3 <- TX pin, GND pin 5). Menu COM port baud rate.",
"ascii":{"term":";","poll":"FA;FB;FT;","init":"AI2;","initEvery":10,"main":{"prefix":"FA","skip":0,"digits":11},"sub":{"prefix":"FB","skip":0,"digits":11},"info":[{"prefix":"IF","skip":0,"digits":11,"to":"main"}],"tx":{"prefix":"FT","sub":"1"}}})J",
R"J({"id":"elecraft","name":"Elecraft KX2 / KX3 / K3 (ACC1)","author":"DK5DEN","version":1,"family":"ascii","baud":38400,"invert":false,
"wiring":"KX2/KX3 ACC1 3.5 mm jack, 3.3 V logic, connect directly (TXD -> RX pin, RXD <- TX pin, GND). Menu RS232 baud 38400.",
"ascii":{"term":";","poll":"FA;FB;FT;","init":"AI2;","initEvery":10,"main":{"prefix":"FA","skip":0,"digits":11},"sub":{"prefix":"FB","skip":0,"digits":11},"info":[{"prefix":"IF","skip":0,"digits":11,"to":"main"}],"tx":{"prefix":"FT","sub":"1"}}})J",
R"J({"id":"icom","name":"Icom CI-V (IC-705, IC-7300, IC-9700 ...)","author":"DK5DEN","version":1,"family":"civ","baud":19200,"invert":false,
"wiring":"CI-V 3.5 mm jack: tip = data (single wire, open drain, 5 V pull-up in the rig), sleeve GND. RX pin to the bus through a 3.3 V level shifter or 2k2/3k3 divider, TX pin through a diode (cathode at the TX pin, anode at the bus). set civaddr to the rig address (IC-705 a4, IC-7300 94, IC-9700 a2), CI-V baud 19200, CI-V transceive ON.",
"civ":{"addr":"a4","poll":["03","2501","0F"],"main":"03","sub":"2501","split":"0F","transceive":"00"}})J",
R"J({"id":"network","name":"No CAT, frequency from the network","author":"DK5DEN","version":1,"family":"none","baud":38400,"invert":false,
"wiring":"Frequency comes from a PC over UDP or HTTP (freq <hz>) or from this page. Use this with the Optima in the shack: a CAT program on the PC forwards the frequency."})J",
};
constexpr uint8_t BUILTIN_COUNT = sizeof(BUILTIN) / sizeof(BUILTIN[0]);

rig::Profile cur;
bool fsUp = false;

bool validId(const String& id) {
    if (id.length() < 2 || id.length() > 23) return false;
    for (size_t i = 0; i < id.length(); i++) {
        char c = id[i];
        if (!(isAlphaNumeric(c) || c == '-' || c == '_')) return false;
    }
    return true;
}

String path(const String& id) { return "/rigs/" + id + ".json"; }

const char* builtinDoc(const String& id) {
    for (uint8_t i = 0; i < BUILTIN_COUNT; i++) {
        // the id is the first string value in every document
        const char* d = BUILTIN[i];
        const char* p = strstr(d, "\"id\":\"");
        if (!p) continue;
        p += 6;
        size_t n = strcspn(p, "\"");
        if (n == id.length() && strncmp(p, id.c_str(), n) == 0) return d;
    }
    return nullptr;
}

bool spec(JsonVariantConst v, rig::Spec& s) {
    if (v.isNull()) { s.prefix[0] = 0; s.skip = s.digits = 0; return true; }
    const char* pre = v["prefix"] | "";
    if (!*pre || strlen(pre) > 3) return false;
    strlcpy(s.prefix, pre, sizeof(s.prefix));
    s.skip = v["skip"] | 0;
    s.digits = v["digits"] | 0;
    return true;
}

bool hexBytes(const char* s, uint8_t* out, uint8_t& n, uint8_t max) {
    n = 0;
    if (!s) return false;
    size_t len = strlen(s);
    if (len == 0 || len % 2) return false;
    for (size_t i = 0; i < len && n < max; i += 2) {
        char b[3] = {s[i], s[i + 1], 0};
        char* end;
        long v = strtol(b, &end, 16);
        if (*end) return false;
        out[n++] = (uint8_t)v;
    }
    return true;
}

bool applyProfile(const rig::Profile& p) {
    cur = p;
    strlcpy(settings.rig, p.id, sizeof(settings.rig));
    settings.catBaud = p.baud;
    settings.catInvert = p.invert;
    if (p.family == rig::FAM_CIV && p.civAddr) settings.civAddr = p.civAddr;
    cat::restart();
    return true;
}

}  // namespace

namespace rig {

const char* familyName(uint8_t f) {
    switch (f) {
        case FAM_ASCII: return "ascii";
        case FAM_CIV:   return "civ";
        default:        return "none";
    }
}

bool parse(const String& json, Profile& p, String& err) {
    JsonDocument doc;
    DeserializationError e = deserializeJson(doc, json);
    if (e) { err = String("json: ") + e.c_str(); return false; }
    memset(&p, 0, sizeof(p));
    String id = doc["id"] | "";
    if (!validId(id)) { err = "id: 2..23 characters a-z 0-9 - _"; return false; }
    strlcpy(p.id, id.c_str(), sizeof(p.id));
    const char* name = doc["name"] | "";
    if (!*name) { err = "name missing"; return false; }
    strlcpy(p.name, name, sizeof(p.name));
    String fam = doc["family"] | "";
    if (fam == "ascii") p.family = FAM_ASCII;
    else if (fam == "civ") p.family = FAM_CIV;
    else if (fam == "none") p.family = FAM_NONE;
    else { err = "family: ascii, civ or none"; return false; }
    p.baud = doc["baud"] | 38400;
    if (p.baud < 1200 || p.baud > 115200) { err = "baud 1200..115200"; return false; }
    p.invert = doc["invert"] | false;

    if (p.family == FAM_ASCII) {
        JsonVariantConst a = doc["ascii"];
        if (a.isNull()) { err = "ascii section missing"; return false; }
        const char* term = a["term"] | ";";
        p.term = term[0] ? term[0] : ';';
        strlcpy(p.poll, a["poll"] | "", sizeof(p.poll));
        strlcpy(p.init, a["init"] | "", sizeof(p.init));
        p.initEvery = a["initEvery"] | 10;
        if (!spec(a["main"], p.mainSpec) || !p.mainSpec.prefix[0]) { err = "ascii.main.prefix missing"; return false; }
        if (!spec(a["sub"], p.subSpec)) { err = "ascii.sub invalid"; return false; }
        JsonArrayConst info = a["info"];
        for (JsonVariantConst v : info) {
            if (p.infoCount >= 4) break;
            if (!spec(v, p.info[p.infoCount])) { err = "ascii.info invalid"; return false; }
            String to = v["to"] | "main";
            p.infoTo[p.infoCount] = to == "sub" ? 1 : 0;
            p.infoCount++;
        }
        JsonVariantConst tx = a["tx"];
        if (!tx.isNull()) {
            strlcpy(p.txPrefix, tx["prefix"] | "", sizeof(p.txPrefix));
            strlcpy(p.txSubVal, tx["sub"] | "1", sizeof(p.txSubVal));
        }
    } else if (p.family == FAM_CIV) {
        JsonVariantConst c = doc["civ"];
        if (c.isNull()) { err = "civ section missing"; return false; }
        const char* addr = c["addr"] | "a4";
        p.civAddr = (uint8_t)strtol(addr, nullptr, 16);
        JsonArrayConst poll = c["poll"];
        for (JsonVariantConst v : poll) {
            if (p.civPollCount >= 4) break;
            if (!hexBytes(v.as<const char*>(), p.civPoll[p.civPollCount], p.civPollLen[p.civPollCount], 4)) { err = "civ.poll: hex bytes"; return false; }
            p.civPollCount++;
        }
        uint8_t b[4], n;
        p.civMain = hexBytes(c["main"] | "03", b, n, 4) && n ? b[0] : 0x03;
        if (hexBytes(c["sub"] | "", b, n, 4) && n >= 1) { p.civSub[0] = b[0]; p.civSub[1] = n > 1 ? b[1] : 0; }
        p.civSplit = hexBytes(c["split"] | "", b, n, 4) && n ? b[0] : 0;
        p.civTransceive = hexBytes(c["transceive"] | "00", b, n, 4) && n ? b[0] : 0;
    }
    return true;
}

void begin() {
    fsUp = LittleFS.begin(true);
    if (fsUp && !LittleFS.exists("/rigs")) LittleFS.mkdir("/rigs");
    String err;
    Profile p;
    String doc = document(settings.rig);
    if (doc.length() == 0 || !parse(doc, p, err)) {
        Serial.printf("rig %s not usable (%s), falling back to ftx1\n", settings.rig, err.c_str());
        doc = document("ftx1");
        parse(doc, p, err);
        strlcpy(settings.rig, p.id, sizeof(settings.rig));
    }
    cur = p;
    // the stored baud/invert/civaddr may have been overridden with set, keep them
}

const Profile& current() { return cur; }

String document(const String& id) {
    if (!validId(id)) return "";
    if (fsUp && LittleFS.exists(path(id))) {
        File f = LittleFS.open(path(id), "r");
        if (f) { String s = f.readString(); f.close(); return s; }
    }
    const char* b = builtinDoc(id);
    return b ? String(b) : String();
}

bool select(const String& id, String& err) {
    String doc = document(id);
    if (doc.length() == 0) { err = "unknown rig, see rig list"; return false; }
    Profile p;
    if (!parse(doc, p, err)) return false;
    return applyProfile(p);
}

bool import(const String& json, String& err) {
    Profile p;
    if (!parse(json, p, err)) return false;
    if (!fsUp) { err = "no file system"; return false; }
    if (json.length() > 4000) { err = "document too large (4000 bytes)"; return false; }
    File f = LittleFS.open(path(p.id), "w");
    if (!f) { err = "cannot write"; return false; }
    f.print(json);
    f.close();
    if (String(settings.rig) == p.id) applyProfile(p);
    return true;
}

bool remove(const String& id) {
    if (!fsUp || !validId(id) || !LittleFS.exists(path(id))) return false;
    LittleFS.remove(path(id));
    if (String(settings.rig) == id) { String e; select(builtinDoc(id) ? id : "ftx1", e); }
    return true;
}

int list(Entry* buf, int max) {
    int n = 0;
    String err;
    Profile p;
    for (uint8_t i = 0; i < BUILTIN_COUNT && n < max; i++) {
        if (!parse(String(BUILTIN[i]), p, err)) continue;
        // a stored profile with the same id replaces the built-in one
        if (fsUp && LittleFS.exists(path(p.id))) continue;
        strlcpy(buf[n].id, p.id, sizeof(buf[n].id));
        strlcpy(buf[n].name, p.name, sizeof(buf[n].name));
        buf[n].family = p.family; buf[n].baud = p.baud; buf[n].stored = false;
        n++;
    }
    if (fsUp) {
        File dir = LittleFS.open("/rigs");
        File f = dir ? dir.openNextFile() : File();
        while (f && n < max) {
            String s = f.readString();
            f.close();
            if (parse(s, p, err)) {
                strlcpy(buf[n].id, p.id, sizeof(buf[n].id));
                strlcpy(buf[n].name, p.name, sizeof(buf[n].name));
                buf[n].family = p.family; buf[n].baud = p.baud; buf[n].stored = true;
                n++;
            }
            f = dir.openNextFile();
        }
    }
    return n;
}

}  // namespace rig
