#include "net.h"
#include "config.h"
#include "settings.h"
#include "commands.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <esp_wifi.h>

namespace {

constexpr const char* AP_SSID = "antenna-bridge";
constexpr const char* AP_PASS = "antenna-bridge";
constexpr uint32_t STA_TIMEOUT_MS   = 8000;    // per connection attempt
constexpr uint32_t AP_AFTER_MS      = 20000;   // no STA after boot -> AP
constexpr uint32_t RETRY_STA_MS     = 60000;   // retry STA while in AP mode
constexpr uint32_t RETRY_LOST_MS    = 15000;   // retry after a lost connection
constexpr uint32_t AP_LINGER_MS     = 30000;   // keep AP this long after STA came back

WiFiMulti* multi = nullptr;
WiFiUDP udp;
DNSServer dns;
bool udpUp = false;
bool mdnsUp = false;
bool apUp = false;
bool wasConnected = false;
uint32_t lastAttempt = 0;
uint32_t bootMs = 0;
uint32_t staSince = 0;
bool scanRunning = false;
bool forceAttempt = false;

class StringPrint : public Print {
public:
    String buf;
    size_t write(uint8_t c) override { buf += (char)c; return 1; }
    size_t write(const uint8_t* b, size_t n) override { buf.concat((const char*)b, n); return n; }
};

void rebuildMulti() {
    if (multi) delete multi;
    multi = new WiFiMulti();
    for (uint8_t i = 0; i < settings.wifiCount; i++)
        multi->addAP(settings.wifi[i].ssid.c_str(), settings.wifi[i].pass.c_str());
}

void startAp() {
    if (apUp) return;
    WiFi.mode(WIFI_AP_STA);
    bool ok = WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 4);
    dns.start(53, "*", WiFi.softAPIP());
    apUp = true;
    wifi_config_t cfg;
    esp_wifi_get_config(WIFI_IF_AP, &cfg);
    Serial.printf("wifi ap started ok=%d ssid=%s pass=%s authmode=%d ip=%s\n", ok, AP_SSID, AP_PASS,
                  cfg.ap.authmode, WiFi.softAPIP().toString().c_str());
}

void stopAp() {
    if (!apUp) return;
    dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    apUp = false;
    Serial.println("wifi ap stopped");
}

// Blocking scan + connect to the strongest known network.
void tryConnect() {
    lastAttempt = millis();
    if (settings.wifiCount == 0 || !multi) return;
    if (scanRunning) return;
    Serial.println("wifi connecting...");
    multi->run(STA_TIMEOUT_MS);
}

void handleUdp() {
    int len = udp.parsePacket();
    if (len <= 0) return;
    char buf[256];
    int n = udp.read(buf, sizeof(buf) - 1);
    if (n <= 0) return;
    buf[n] = '\0';
    IPAddress rip = udp.remoteIP();
    uint16_t rport = udp.remotePort();

    StringPrint reply;
    String packet(buf);
    int start = 0;
    while (start < (int)packet.length()) {
        int nl = packet.indexOf('\n', start);
        if (nl < 0) nl = packet.length();
        handleCommand(packet.substring(start, nl), reply);
        start = nl + 1;
    }
    if (reply.buf.length()) {
        udp.beginPacket(rip, rport);
        udp.write((const uint8_t*)reply.buf.c_str(), reply.buf.length());
        udp.endPacket();
    }
}

}  // namespace

namespace net {

void begin() {
    bootMs = millis();
    if (!settings.wifiOn) { WiFi.mode(WIFI_OFF); return; }
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    // Modem sleep must stay on while Bluetooth is active (coexistence), the
    // WiFi driver aborts otherwise.
    WiFi.setSleep(true);
    WiFi.setAutoReconnect(false);
    WiFi.setHostname(MDNS_NAME);
    // Super Mini boards have a poorly matched antenna; full power tends to
    // fail on many units, reduced power connects more reliably.
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    rebuildMulti();
    udp.begin(settings.udpPort);
    udpUp = true;
    if (settings.wifiCount == 0) {
        startAp();
    } else {
        tryConnect();
    }
}

void reconnect() {
    rebuildMulti();
    lastAttempt = 0;
    forceAttempt = true;
    if (WiFi.status() == WL_CONNECTED) WiFi.disconnect();
}

bool staConnected() { return WiFi.status() == WL_CONNECTED; }
bool apActive() { return apUp; }
const char* apSsid() { return AP_SSID; }
const char* apPass() { return AP_PASS; }

IPAddress ip() {
    if (staConnected()) return WiFi.localIP();
    if (apUp) return WiFi.softAPIP();
    return IPAddress(0, 0, 0, 0);
}

String stateText() {
    if (!settings.wifiOn) return "off";
    if (staConnected()) return "sta";
    if (apUp) return "ap";
    if (settings.wifiCount) return "connecting";
    return "off";
}

void scanStart() {
    if (scanRunning) return;
    WiFi.scanNetworks(true, false);
    scanRunning = true;
}

int scanResults() {
    int r = WiFi.scanComplete();
    if (r == WIFI_SCAN_RUNNING) return -1;
    if (r == WIFI_SCAN_FAILED) { scanRunning = false; return -2; }
    scanRunning = false;
    return r;
}

void loop() {
    if (!settings.wifiOn) return;
    uint32_t now = millis();
    bool up = staConnected();

    if (up && !wasConnected) {
        wasConnected = true;
        staSince = now;
        if (!mdnsUp && MDNS.begin(MDNS_NAME)) {
            MDNS.addService("http", "tcp", 80);
            MDNS.addService("antbridge", "udp", settings.udpPort);
            mdnsUp = true;
        }
        Serial.printf("wifi connected ssid=%s ip=%s rssi=%d\n", WiFi.SSID().c_str(),
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }
    if (!up && wasConnected) {
        wasConnected = false;
        if (mdnsUp) { MDNS.end(); mdnsUp = false; }
        lastAttempt = now;
        Serial.println("wifi lost");
    }

    if (up) {
        // Drop the fallback AP once the station link has been stable for a while
        // and nobody is using the AP.
        if (apUp && now - staSince > AP_LINGER_MS && WiFi.softAPgetStationNum() == 0) stopAp();
    } else {
        bool idle = true;
        bool apBusy = apUp && WiFi.softAPgetStationNum() > 0;
        uint32_t interval = apUp ? RETRY_STA_MS : RETRY_LOST_MS;
        if (idle && settings.wifiCount > 0 && (forceAttempt || (!apBusy && now - lastAttempt > interval))) {
            forceAttempt = false;
            tryConnect();
        }
        if (!apUp && now - bootMs > AP_AFTER_MS && !staConnected()) startAp();
    }

    if (apUp) dns.processNextRequest();
    if (udpUp) handleUdp();
}

}  // namespace net
