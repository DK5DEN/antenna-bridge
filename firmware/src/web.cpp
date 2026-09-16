#include "web.h"
#include "config.h"
#include "settings.h"
#include "commands.h"
#include "cat.h"
#include "engine.h"
#include "ble.h"
#include "net.h"
#include "page.h"
#include <WebServer.h>
#include <WiFi.h>

namespace {

WebServer server(80);

class StringPrint : public Print {
public:
    String buf;
    size_t write(uint8_t c) override { buf += (char)c; return 1; }
    size_t write(const uint8_t* b, size_t n) override { buf.concat((const char*)b, n); return n; }
};

String jsonEscape(const String& in) {
    String out;
    out.reserve(in.length() + 8);
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': break;
            case '\t': out += "\\t"; break;
            default:
                if ((uint8_t)c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
                else out += c;
        }
    }
    return out;
}

String q(const char* s) { return "\"" + jsonEscape(String(s)) + "\""; }
String q(const String& s) { return "\"" + jsonEscape(s) + "\""; }

String statusJson() {
    const Settings& s = settings;
    uint32_t f = engine::freq();
    String j;
    j.reserve(3000);
    j += "{\"fw\":\"" + String(FW_NAME) + " " + FW_VERSION + "\"";
    j += ",\"freq\":" + String(f);
    j += ",\"src\":" + q(engine::source());
    j += ",\"uptime\":" + String(millis() / 1000);
    j += ",\"blebusy\":" + String(ble::busy() ? "true" : "false");
    j += ",\"active\":" + q(s.activeAnt);
    j += ",\"ants\":[";
    for (uint8_t i = 0; i < s.antCount; i++) { if (i) j += ","; j += q(s.ants[i].name); }
    j += "]";

    j += ",\"cat\":{\"ok\":" + String(cat::linkOk() ? "true" : "false");
    j += ",\"rig\":" + q(s.rig) + ",\"proto\":" + q(protoName(s.proto));
    j += ",\"main\":" + String(cat::freqMain());
    j += ",\"sub\":" + String(cat::freqSub());
    j += ",\"tx\":" + String((int)cat::txSide());
    j += ",\"rxage\":" + String(cat::lastRxAgeMs());
    j += ",\"rxcount\":" + String(cat::rxCount());
    j += ",\"last\":" + q(cat::lastMessage()) + "}";

    j += ",\"outs\":[";
    for (uint8_t i = 0; i < s.outCount; i++) {
        const Output& o = s.outs[i];
        if (i) j += ",";
        bool match = f && s.outActive(o) && s.ruleMatch(o.name, f);
        j += "{\"name\":" + q(o.name) + ",\"type\":" + q(outTypeName(o.type)) + ",\"match\":" + String(match ? "true" : "false");
        j += ",\"antenna\":" + q(o.antenna) + ",\"active\":" + String(s.outActive(o) ? "true" : "false");
        if (o.type == OUT_UDP) {
            j += ",\"host\":" + q(o.host) + ",\"port\":" + String(o.port);
            j += ",\"sent\":" + String(engine::lastSent(i)) + ",\"reply\":" + q(engine::lastUdpReply(i)) + ",\"err\":" + q(engine::udpError(i));
        } else if (o.type == OUT_RELAY || o.type == OUT_LINE) {
            ble::DevState d = ble::state(i);
            j += ",\"addr\":" + q(o.host) + ",\"atype\":" + String(o.addrType);
            j += ",\"want\":" + String((int)d.desired) + ",\"state\":" + String((int)d.actual) + ",\"sense\":" + String((int)d.sense);
            j += ",\"batt\":" + String(d.battCv) + ",\"rssi\":" + String((int)d.rssi);
            j += ",\"lastok\":" + String(d.lastOk ? (millis() - d.lastOk) / 1000 : -1);
            j += ",\"fails\":" + String(d.fails) + ",\"link\":" + String(d.link) + ",\"conn\":\"" + String(d.tries - d.connFails) + "/" + String(d.tries) + "\"";
            j += ",\"err\":" + q(d.err) + ",\"reply\":" + q(d.reply);
            j += ",\"sent\":" + String(engine::lastSent(i));
        } else {
            j += ",\"pin\":" + String(o.pin) + ",\"inv\":" + String(o.invert);
        }
        j += "}";
    }
    j += "]";

    j += ",\"rules\":[";
    for (uint8_t i = 0; i < s.ruleCount; i++) {
        if (i) j += ",";
        j += "{\"out\":" + q(s.rules[i].out) + ",\"fmin\":" + String(s.rules[i].fmin) + ",\"fmax\":" + String(s.rules[i].fmax) + "}";
    }
    j += "]";

    ble::ScanHit hits[16];
    int m = ble::scanResults(hits, 16);
    j += ",\"scan\":{\"running\":" + String(ble::scanning() ? "true" : "false") + ",\"hits\":[";
    for (int i = 0; i < m; i++) {
        if (i) j += ",";
        j += "{\"addr\":" + q(hits[i].addr) + ",\"atype\":" + String(hits[i].type) + ",\"name\":" + q(hits[i].name);
        j += ",\"rssi\":" + String((int)hits[i].rssi) + ",\"kind\":" + String(hits[i].kind) + "}";
    }
    j += "]}";

    uint8_t pc;
    const cat::Preset* pp = cat::presets(pc);
    j += ",\"presets\":[";
    for (uint8_t i = 0; i < pc; i++) {
        if (i) j += ",";
        j += "{\"name\":" + q(pp[i].name) + ",\"proto\":" + q(protoName(pp[i].proto)) + ",\"baud\":" + String(pp[i].baud);
        j += ",\"rig\":" + q(pp[i].rigName) + ",\"wiring\":" + q(pp[i].wiring) + "}";
    }
    j += "]";

    j += ",\"settings\":{";
    j += "\"rig\":" + q(s.rig);
    j += ",\"proto\":" + q(protoName(s.proto));
    j += ",\"catrx\":" + String(s.catRx);
    j += ",\"cattx\":" + String(s.catTx);
    j += ",\"catinv\":" + String(s.catInvert ? 1 : 0);
    char ca[4]; snprintf(ca, sizeof(ca), "%02x", s.civAddr);
    j += ",\"civaddr\":" + q(ca);
    j += ",\"catbaud\":" + String(s.catBaud);
    j += ",\"catpoll\":" + String(s.catPollMs);
    j += ",\"catvfo\":" + String(s.catVfo);
    j += ",\"settle\":" + String(s.settleMs);
    j += ",\"udpport\":" + String(s.udpPort);
    j += ",\"wifion\":" + String(s.wifiOn ? 1 : 0);
    j += ",\"blehold\":" + String(s.bleHold ? 1 : 0);
    j += "}";

    j += ",\"wifi\":{";
    j += "\"mode\":\"" + net::stateText() + "\"";
    j += ",\"ssid\":\"" + jsonEscape(net::staConnected() ? WiFi.SSID() : String("")) + "\"";
    j += ",\"ip\":\"" + net::ip().toString() + "\"";
    j += ",\"rssi\":" + String(net::staConnected() ? WiFi.RSSI() : 0);
    j += ",\"ap\":" + String(net::apActive() ? "true" : "false");
    j += ",\"ap_ssid\":\"" + String(net::apSsid()) + "\"";
    j += ",\"ap_pass\":\"" + String(net::apPass()) + "\"";
    j += ",\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\"";
    j += ",\"hostname\":\"" + String(MDNS_NAME) + ".local\"";
    j += ",\"networks\":[";
    for (uint8_t i = 0; i < s.wifiCount; i++) {
        if (i) j += ",";
        j += "\"" + jsonEscape(s.wifi[i].ssid) + "\"";
    }
    j += "]}";
    j += "}";
    return j;
}

void handleStatus() {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", statusJson());
}

void handleCmd() {
    String line = server.arg("line");
    if (line.length() == 0) { server.send(400, "text/plain", "ERR missing line"); return; }
    StringPrint out;
    handleCommand(line, out);
    server.send(200, "text/plain", out.buf);
}

void handleWifi() {
    String action = server.arg("action");
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    if (action == "add") {
        if (!settings.wifiAdd(ssid, pass)) { server.send(400, "text/plain", "ERR ssid empty or list full"); return; }
        net::reconnect();
        server.send(200, "text/plain", "OK stored " + ssid + ", connecting");
    } else if (action == "del") {
        if (!settings.wifiDel(ssid)) { server.send(404, "text/plain", "ERR not found"); return; }
        net::reconnect();
        server.send(200, "text/plain", "OK removed " + ssid);
    } else {
        server.send(400, "text/plain", "ERR action");
    }
}

void handleScan() {
    int n = net::scanResults();
    if (n == -2) { net::scanStart(); server.send(200, "application/json", "{\"running\":true}"); return; }
    if (n == -1) { server.send(200, "application/json", "{\"running\":true}"); return; }
    String j = "{\"running\":false,\"networks\":[";
    for (int i = 0; i < n; i++) {
        if (i) j += ",";
        j += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",\"rssi\":" + String(WiFi.RSSI(i));
        j += ",\"enc\":" + String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true") + "}";
    }
    j += "]}";
    WiFi.scanDelete();
    server.send(200, "application/json", j);
}

void handleNotFound() {
    if (net::apActive() && !net::staConnected()) {
        server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
        server.send(302, "text/plain", "");
        return;
    }
    server.send(404, "text/plain", "not found");
}

void handleRoot() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html", PAGE);
}

}  // namespace

namespace web {

void begin() {
    if (!settings.wifiOn) return;
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/cmd", HTTP_POST, handleCmd);
    server.on("/api/wifi", HTTP_POST, handleWifi);
    server.on("/api/scan", HTTP_GET, handleScan);
    server.onNotFound(handleNotFound);
    server.begin();
}

void loop() {
    if (!settings.wifiOn) return;
    server.handleClient();
}

}  // namespace web
