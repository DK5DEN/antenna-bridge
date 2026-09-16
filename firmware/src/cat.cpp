#include "cat.h"
#include "config.h"
#include "settings.h"
#include "rig.h"

namespace {

String   rxBuf;
uint8_t  frame[64];
uint8_t  frameLen = 0;
bool     inFrame = false;
uint32_t fMain = 0, fSub = 0;
int8_t   side = -1;
uint32_t lastRx = 0, lastPoll = 0, lastInit = 0, count = 0;
String   lastMsg;
bool     portOpen = false;

void gotData() { lastRx = millis(); count++; }

// ---- ASCII families ----

// Parse a frequency answer described by a spec: prefix, skipped characters,
// then digits (0 = all remaining).
bool specFreq(const rig::Spec& s, const String& m, uint32_t& f) {
    if (!s.prefix[0]) return false;
    size_t pl = strlen(s.prefix);
    if (m.length() < pl + s.skip + 1 || strncmp(m.c_str(), s.prefix, pl) != 0) return false;
    size_t from = pl + s.skip;
    size_t to = s.digits ? from + s.digits : m.length();
    if (to > m.length()) return false;
    for (size_t i = from; i < to; i++)
        if (!isDigit(m[i])) return false;
    f = strtoul(m.substring(from, to).c_str(), nullptr, 10);
    return true;
}

void handleAscii(const String& m) {
    const rig::Profile& p = rig::current();
    if (m.length() < 2) return;
    lastMsg = m;
    gotData();
    uint32_t f;
    if (specFreq(p.mainSpec, m, f)) { fMain = f; return; }
    if (specFreq(p.subSpec, m, f)) { fSub = f; return; }
    for (uint8_t i = 0; i < p.infoCount; i++) {
        if (specFreq(p.info[i], m, f)) { if (p.infoTo[i]) fSub = f; else fMain = f; return; }
    }
    if (p.txPrefix[0]) {
        size_t pl = strlen(p.txPrefix);
        if (m.length() > pl && strncmp(m.c_str(), p.txPrefix, pl) == 0)
            side = m.substring(pl) == p.txSubVal ? 1 : 0;
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
    const rig::Profile& p = rig::current();
    if (n < 3) return;
    uint8_t from = f[1], cmd = f[2];
    if (from == CIV_OWN_ADDR) return;           // our own echo on the single-wire bus
    String h;
    for (uint8_t i = 0; i < n; i++) { char t[4]; snprintf(t, sizeof(t), "%02X ", f[i]); h += t; }
    h.trim();
    lastMsg = h;
    gotData();
    if ((cmd == p.civMain || cmd == p.civTransceive) && n >= 8) {
        fMain = bcdFreq(f + 3, 5);
    } else if (p.civSub[0] && cmd == p.civSub[0] && n >= 9 && (p.civSub[1] == 0 || f[3] == p.civSub[1])) {
        fSub = bcdFreq(f + 4, 5);
    } else if (p.civSub[0] && cmd == p.civSub[0] && n >= 9 && f[3] == 0x00) {
        fMain = bcdFreq(f + 4, 5);              // 25 00: selected VFO
    } else if (p.civSplit && cmd == p.civSplit && n >= 4) {
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
    const rig::Profile& p = rig::current();
    if (p.family == rig::FAM_ASCII) {
        if (p.poll[0]) Serial0.print(p.poll);
    } else if (p.family == rig::FAM_CIV) {
        for (uint8_t i = 0; i < p.civPollCount; i++) civSend(p.civPoll[i], p.civPollLen[i]);
    }
}

}  // namespace

namespace cat {

void begin() {
    fMain = fSub = 0;
    side = -1;
    lastRx = 0;
    rxBuf = "";
    inFrame = false;
    frameLen = 0;
    lastPoll = 0;
    lastInit = 0;
    portOpen = false;
    if (rig::current().family == rig::FAM_NONE) return;
    Serial0.begin(settings.catBaud, SERIAL_8N1, settings.catRx, settings.catTx, settings.catInvert);
    portOpen = true;
}

void restart() {
    if (portOpen) Serial0.end();
    begin();
}

void send(const String& s) {
    if (!portOpen) return;
    if (rig::current().family == rig::FAM_CIV) {
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
    if (!portOpen) return;
    const rig::Profile& p = rig::current();
    while (Serial0.available()) {
        uint8_t c = Serial0.read();
        if (p.family == rig::FAM_CIV) {
            civByte(c);
        } else if (c == (uint8_t)p.term) {
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
    if (p.family == rig::FAM_ASCII && p.init[0] && p.initEvery && now - lastInit >= (uint32_t)p.initEvery * 1000) {
        lastInit = now;
        Serial0.print(p.init);
    }
}

bool linkOk() { return portOpen && lastRx != 0 && millis() - lastRx < settings.catPollMs * 3 + 1000; }
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
