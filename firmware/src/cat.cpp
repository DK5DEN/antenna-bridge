#include "cat.h"
#include "config.h"
#include "settings.h"

namespace {

const cat::Preset PRESETS[] = {
    {"ftx1", PROTO_YAESU, 38400, 0, "Yaesu FTX-1 (TUNER/LINEAR, CAT-3)",
     "10-pin mini-DIN on the Field head: TXD (BAND A) -> level shifter -> RX pin, RXD (BAND B) <- level shifter <- TX pin, +13.8 V OUT, GND. 5 V CMOS, use a BSS138 shifter. Menu OPERATION SETTING > GENERAL > TUN/LIN PORT SELECT = CAT-3, CAT-3 RATE 38400. Not usable while a tuner, ATAS or the Optima is on the jack."},
    {"ftdx10", PROTO_YAESU, 38400, 0, "Yaesu FT-DX10 (rear RS-232)",
     "DE-9 RS-232: pin 2 TXD -> MAX3232 -> RX pin, pin 3 RXD <- MAX3232 <- TX pin, pin 5 GND. Menu CAT RATE 38400, CAT RTS OFF."},
    {"ft891", PROTO_YAESU, 4800, 0, "Yaesu FT-891 (CAT/LINEAR jack, TTL)",
     "8-pin mini-DIN CAT/LINEAR: TXD -> RX pin, RXD <- TX pin, GND. 5 V TTL, use a level shifter. Menu CAT RATE (default 4800)."},
    {"kenwood", PROTO_KENWOOD, 9600, 0, "Kenwood TS-590 / TS-890 (COM RS-232)",
     "DE-9 RS-232 via MAX3232 (TXD pin 2 -> RX pin, RXD pin 3 <- TX pin, GND pin 5). Menu COM port baud rate."},
    {"elecraft", PROTO_KENWOOD, 38400, 0, "Elecraft KX2 / KX3 / K3 (ACC1)",
     "KX2/KX3 ACC1 3.5 mm jack, 3.3 V logic, connect directly (TXD -> RX pin, RXD <- TX pin, GND). Menu RS232 baud 38400."},
    {"icom", PROTO_ICOM, 19200, DEF_CIV_ADDR, "Icom CI-V (IC-705, IC-7300, IC-9700 ...)",
     "CI-V 3.5 mm jack: tip = data (single wire, open drain, 5 V pull-up in the rig), sleeve GND. RX pin to the bus through a 3.3 V level shifter or 2k2/3k3 divider, TX pin through a diode (cathode at the TX pin, anode at the bus). set civaddr to the rig address (IC-705 a4, IC-7300 94, IC-9700 a2), CI-V baud 19200, CI-V transceive ON."},
    {"network", PROTO_NONE, 38400, 0, "No CAT, frequency from the network",
     "Frequency comes from a PC over UDP or HTTP (freq <hz>) or from this page. Use this with the Optima in the shack: a CAT program on the PC forwards the frequency."},
};
constexpr uint8_t PRESET_COUNT = sizeof(PRESETS) / sizeof(PRESETS[0]);

String   rxBuf;
uint8_t  frame[64];
uint8_t  frameLen = 0;
bool     inFrame = false;
uint32_t fMain = 0, fSub = 0;
int8_t   side = -1;
uint32_t lastRx = 0, lastPoll = 0, lastAi = 0, count = 0;
String   lastMsg;

constexpr uint32_t AI_INTERVAL_MS = 10000;   // AI is reset when the rig powers off, so repeat it

bool allDigits(const String& s, int from, int to) {
    if (from >= to || to > (int)s.length()) return false;
    for (int i = from; i < to; i++)
        if (!isDigit(s[i])) return false;
    return true;
}

void gotData() { lastRx = millis(); count++; }

// ---- ASCII (Yaesu / Kenwood / Elecraft) ----
void handleAscii(const String& m) {
    if (m.length() < 3) return;
    lastMsg = m;
    gotData();
    char a = m[0], b = m[1];
    if (a == 'F' && (b == 'A' || b == 'B') && allDigits(m, 2, m.length())) {
        uint32_t f = strtoul(m.c_str() + 2, nullptr, 10);
        if (b == 'A') fMain = f; else fSub = f;
    } else if (a == 'F' && b == 'T' && m.length() == 3 && isDigit(m[2])) {
        side = m[2] - '0';
    } else if (a == 'I' && b == 'F') {
        if (settings.proto == PROTO_KENWOOD && allDigits(m, 2, 13)) {
            fMain = strtoul(m.substring(2, 13).c_str(), nullptr, 10);
        } else if (settings.proto == PROTO_YAESU && m.length() >= 16 && allDigits(m, 7, 16)) {
            fMain = strtoul(m.substring(7, 16).c_str(), nullptr, 10);
        }
    } else if (a == 'O' && b == 'I' && settings.proto == PROTO_YAESU && m.length() >= 16 && allDigits(m, 7, 16)) {
        fSub = strtoul(m.substring(7, 16).c_str(), nullptr, 10);
    }
}

// ---- Icom CI-V ----
uint32_t bcdFreq(const uint8_t* d, uint8_t n) {
    uint32_t f = 0, mul = 1;
    for (uint8_t i = 0; i < n; i++) {
        f += ((d[i] & 0x0F) + (d[i] >> 4) * 10) * mul;
        mul *= 100;
    }
    return f;
}

void civSend(const uint8_t* payload, uint8_t n) {
    uint8_t b[72];
    uint8_t k = 0;
    b[k++] = 0xFE; b[k++] = 0xFE; b[k++] = settings.civAddr; b[k++] = CIV_OWN_ADDR;
    for (uint8_t i = 0; i < n && k < sizeof(b) - 1; i++) b[k++] = payload[i];
    b[k++] = 0xFD;
    Serial0.write(b, k);
}

void handleCiv(const uint8_t* f, uint8_t n) {
    // f: to from cmd [sub] data...
    if (n < 3) return;
    uint8_t from = f[1], cmd = f[2];
    if (from == CIV_OWN_ADDR) return;           // our own echo on the single-wire bus
    String h;
    for (uint8_t i = 0; i < n; i++) { char t[4]; snprintf(t, sizeof(t), "%02X ", f[i]); h += t; }
    h.trim();
    lastMsg = h;
    gotData();
    if ((cmd == 0x00 || cmd == 0x03) && n >= 8) {
        fMain = bcdFreq(f + 3, 5);
    } else if (cmd == 0x25 && n >= 9) {
        uint32_t v = bcdFreq(f + 4, 5);
        if (f[3] == 0x00) fMain = v; else fSub = v;
    } else if (cmd == 0x0F && n >= 4) {
        side = f[3] ? 1 : 0;                    // split on: transmit on the unselected VFO
    }
}

void civByte(uint8_t c) {
    if (c == 0xFE) {
        if (inFrame && frameLen == 0) return;   // second preamble byte
        inFrame = true;
        frameLen = 0;
        return;
    }
    if (!inFrame) return;
    if (c == 0xFD) {
        handleCiv(frame, frameLen);
        inFrame = false;
        frameLen = 0;
        return;
    }
    if (frameLen < sizeof(frame)) frame[frameLen++] = c;
}

void poll() {
    switch (settings.proto) {
        case PROTO_YAESU:
        case PROTO_KENWOOD:
            Serial0.print("FA;FB;FT;");
            break;
        case PROTO_ICOM: {
            uint8_t a[] = {0x03};             // operating frequency
            uint8_t b[] = {0x25, 0x01};       // unselected VFO (newer rigs, ignored by older ones)
            uint8_t c[] = {0x0F};             // split
            civSend(a, 1); civSend(b, 2); civSend(c, 1);
            break;
        }
        default: break;
    }
}

void autoInfo() {
    if (settings.proto == PROTO_YAESU) Serial0.print("AI1;");
    else if (settings.proto == PROTO_KENWOOD) Serial0.print("AI2;");
}

}  // namespace

namespace cat {

const Preset* presets(uint8_t& n) { n = PRESET_COUNT; return PRESETS; }

const Preset* preset(const String& name) {
    for (uint8_t i = 0; i < PRESET_COUNT; i++)
        if (name.equalsIgnoreCase(PRESETS[i].name)) return &PRESETS[i];
    return nullptr;
}

bool applyPreset(const String& name) {
    const Preset* p = preset(name);
    if (!p) return false;
    strlcpy(settings.rig, p->name, sizeof(settings.rig));
    settings.proto = p->proto;
    settings.catBaud = p->baud;
    if (p->civAddr) settings.civAddr = p->civAddr;
    restart();
    return true;
}

void begin() {
    fMain = fSub = 0;
    side = -1;
    lastRx = 0;
    rxBuf = "";
    inFrame = false;
    frameLen = 0;
    lastPoll = 0;
    lastAi = 0;
    if (settings.proto == PROTO_NONE) return;
    Serial0.begin(settings.catBaud, SERIAL_8N1, settings.catRx, settings.catTx, settings.catInvert);
}

void restart() {
    Serial0.end();
    begin();
}

void send(const String& s) {
    if (settings.proto == PROTO_NONE) return;
    if (settings.proto == PROTO_ICOM) {
        // hex bytes; a bare payload (not starting with FE) gets the CI-V header
        uint8_t b[64];
        uint8_t n = 0;
        const char* c = s.c_str();
        while (*c && n < sizeof(b)) {
            while (*c == ' ') c++;
            if (!*c) break;
            char* end;
            long v = strtol(c, &end, 16);
            if (end == c) break;
            b[n++] = (uint8_t)v;
            c = end;
        }
        if (n == 0) return;
        if (b[0] == 0xFE) Serial0.write(b, n);
        else civSend(b, n);
        return;
    }
    Serial0.print(s);
}

void loop() {
    if (settings.proto == PROTO_NONE) return;
    while (Serial0.available()) {
        uint8_t c = Serial0.read();
        if (settings.proto == PROTO_ICOM) {
            civByte(c);
        } else if (c == ';') {
            handleAscii(rxBuf);
            rxBuf = "";
        } else if (c >= 0x20 && rxBuf.length() < 80) {
            rxBuf += (char)c;
        } else if (c < 0x20) {
            rxBuf = "";
        }
    }
    uint32_t now = millis();
    if (now - lastPoll >= settings.catPollMs) {
        lastPoll = now;
        poll();
    }
    if (now - lastAi >= AI_INTERVAL_MS) {
        lastAi = now;
        autoInfo();
    }
}

bool linkOk() { return settings.proto != PROTO_NONE && lastRx != 0 && millis() - lastRx < settings.catPollMs * 3 + 1000; }
uint32_t lastRxAgeMs() { return lastRx ? millis() - lastRx : 0xFFFFFFFF; }
uint32_t freqMain() { return fMain; }
uint32_t freqSub() { return fSub; }
int8_t txSide() { return side; }
uint32_t rxCount() { return count; }
String lastMessage() { return lastMsg; }

uint32_t txFreq() {
    if (!linkOk()) return 0;
    uint8_t s;
    if (settings.catVfo == 1) s = 0;
    else if (settings.catVfo == 2) s = 1;
    else s = (side == 1) ? 1 : 0;
    uint32_t f = s ? fSub : fMain;
    if (f == 0 && s == 1) f = fMain;   // sub unknown (rig without a second VFO readout)
    return f;
}

}  // namespace cat
