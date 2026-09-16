#include "engine.h"
#include "config.h"
#include "settings.h"
#include "ble.h"
#include "net.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>

namespace {

uint32_t curFreq = 0;
char     src[8] = "none";
uint32_t pendingFreq = 0, pendingSince = 0, appliedFreq = 0;
bool     forceApply = false;
bool     retryNeeded = false;   // a udp send failed, try again later
uint32_t lastRetry = 0;

struct UdpOut {
    IPAddress ip;
    uint32_t  resolvedAt;   // 0 = not resolved
    uint32_t  lastTry;
    String    reply;
    String    err;
};
UdpOut   udpOut[OUT_MAX];
uint32_t sent[OUT_MAX];     // last hz forwarded, 0 none
WiFiUDP  txUdp;
bool     txUdpUp = false;

constexpr uint32_t DNS_TTL_MS   = 600000;
constexpr uint32_t DNS_RETRY_MS = 30000;
constexpr uint32_t RETRY_MS     = 5000;

bool resolve(uint8_t i, const Output& o) {
    UdpOut& u = udpOut[i];
    uint32_t now = millis();
    if (u.resolvedAt && now - u.resolvedAt < DNS_TTL_MS) return true;
    if (u.ip.fromString(o.host)) { u.resolvedAt = now; return true; }
    if (!net::staConnected()) { u.err = "no wifi"; return false; }
    if (u.lastTry && now - u.lastTry < DNS_RETRY_MS) return false;
    u.lastTry = now;
    String h(o.host);
    IPAddress ip;
    if (h.endsWith(".local")) {
        ip = MDNS.queryHost(h.substring(0, h.length() - 6), 2000);
    } else {
        if (!WiFi.hostByName(o.host, ip)) ip = IPAddress(0, 0, 0, 0);
    }
    if (ip == IPAddress(0, 0, 0, 0)) { u.err = "name not resolved"; return false; }
    u.ip = ip;
    u.resolvedAt = now;
    u.err = "";
    return true;
}

bool udpSend(uint8_t i, const Output& o, const String& line) {
    if (!resolve(i, o)) return false;
    if (!txUdpUp) return false;
    UdpOut& u = udpOut[i];
    String msg = line + "\n";
    txUdp.beginPacket(u.ip, o.port);
    txUdp.write((const uint8_t*)msg.c_str(), msg.length());
    if (!txUdp.endPacket()) { u.err = "send failed"; return false; }
    u.err = "";
    Serial.printf("udp %s -> %s:%u %s\n", o.name, u.ip.toString().c_str(), o.port, line.c_str());
    return true;
}

void udpReplies() {
    if (!txUdpUp) return;
    int len = txUdp.parsePacket();
    if (len <= 0) return;
    char buf[200];
    int n = txUdp.read(buf, sizeof(buf) - 1);
    if (n <= 0) return;
    buf[n] = 0;
    IPAddress rip = txUdp.remoteIP();
    String r(buf);
    r.trim();
    int nl = r.indexOf('\n');
    if (nl >= 0) r = r.substring(0, nl);
    for (uint8_t i = 0; i < settings.outCount; i++) {
        if (settings.outs[i].type == OUT_UDP && udpOut[i].resolvedAt && udpOut[i].ip == rip) {
            udpOut[i].reply = r;
            Serial.printf("udp %s <- %s\n", settings.outs[i].name, r.c_str());
        }
    }
}

void setupGpio() {
    for (uint8_t i = 0; i < settings.outCount; i++) {
        const Output& o = settings.outs[i];
        if (o.type != OUT_GPIO) continue;
        pinMode(o.pin, OUTPUT);
        digitalWrite(o.pin, o.invert ? HIGH : LOW);
    }
}

void apply(uint32_t hz) {
    retryNeeded = false;
    for (uint8_t i = 0; i < settings.outCount; i++) {
        const Output& o = settings.outs[i];
        bool match = settings.outActive(o) && settings.ruleMatch(o.name, hz);
        switch (o.type) {
            case OUT_RELAY:
                ble::setDesired(i, match);
                break;
            case OUT_GPIO:
                digitalWrite(o.pin, (match != (o.invert != 0)) ? HIGH : LOW);
                break;
            case OUT_UDP:
                // unsent frequencies are retried on the next loop pass (name resolution, WiFi)
                if (match && (forceApply || sent[i] != hz)) { if (udpSend(i, o, "freq " + String(hz))) sent[i] = hz; else retryNeeded = true; }
                else if (!match) sent[i] = 0;
                break;
            case OUT_LINE:
                if (match && (forceApply || sent[i] != hz)) { ble::sendLine(i, "freq " + String(hz)); sent[i] = hz; }
                else if (!match) sent[i] = 0;
                break;
        }
    }
    appliedFreq = hz;
    forceApply = false;
}

}  // namespace

namespace engine {

void begin() {
    setupGpio();
    if (!settings.wifiOn) return;   // no TCP/IP stack without the radio
    txUdp.begin(settings.udpPort + 1);
    txUdpUp = true;
}

void setFreq(uint32_t hz, const char* source) {
    if (hz == 0) return;
    if (hz != pendingFreq) {
        pendingFreq = hz;
        pendingSince = millis();
    }
    if (hz != curFreq || strcmp(src, source) != 0) {
        curFreq = hz;
        strlcpy(src, source, sizeof(src));
    }
}

uint32_t freq() { return curFreq; }
const char* source() { return src; }

void outputsChanged() {
    setupGpio();
    for (uint8_t i = 0; i < OUT_MAX; i++) {
        sent[i] = 0;
        udpOut[i].resolvedAt = 0;
        udpOut[i].lastTry = 0;
        udpOut[i].reply = "";
        udpOut[i].err = "";
    }
    ble::outputsChanged();
    forceApply = true;
}

void applyNow() { forceApply = true; }

uint32_t lastSent(uint8_t idx) { return idx < OUT_MAX ? sent[idx] : 0; }
String lastUdpReply(uint8_t idx) { return idx < OUT_MAX ? udpOut[idx].reply : String(); }
String udpError(uint8_t idx) { return idx < OUT_MAX ? udpOut[idx].err : String(); }

void loop() {
    udpReplies();
    if (pendingFreq == 0) return;
    uint32_t now = millis();
    if (forceApply || (pendingFreq != appliedFreq && now - pendingSince >= settings.settleMs)) apply(pendingFreq);
    else if (retryNeeded && now - lastRetry >= RETRY_MS) { lastRetry = now; apply(appliedFreq); }
}

}  // namespace engine
