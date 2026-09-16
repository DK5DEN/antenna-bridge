#include "ble.h"
#include "config.h"
#include "settings.h"
#include <NimBLEDevice.h>

namespace {

const char* UUID_BR1_SVC = "67d03ad6-34ba-450a-a39c-00805f9b34fb";
const char* UUID_NUS     = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const char* UUID_NUS_RX  = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const char* UUID_NUS_TX  = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
constexpr uint16_t UUID_AIO        = 0x1815;
constexpr uint16_t UUID_DIGITAL    = 0x2A56;
constexpr uint16_t UUID_BATT       = 0x180F;
constexpr uint16_t UUID_BATT_LEVEL = 0x2A19;

constexpr uint32_t LINE_RECONNECT_MS  = 30000;
constexpr uint32_t RELAY_REFRESH_MS   = 600000;  // battery / sense readback on a held link
constexpr uint8_t  CONNECT_TIMEOUT_S  = 8;
constexpr uint8_t  CONNECT_ATTEMPTS   = 4;
constexpr uint8_t  SCAN_SECONDS       = 6;

// Held BR1 link: 250 ms interval (what the BR1 asks for itself) with slave
// latency 4, so the switch wakes every 1.25 s like it would for advertising.
// Supervision timeout 6 s.
constexpr uint16_t HOLD_ITVL    = 200;
constexpr uint16_t HOLD_LATENCY = 4;
constexpr uint16_t HOLD_TIMEOUT = 600;

struct Dev {
    ble::DevState st;
    bool  refreshReq;
    bool  lineQueued;
    char  line[64];
    NimBLEClient* client;
};

Dev devs[OUT_MAX];
SemaphoreHandle_t mtx;
volatile bool scanReq = false, scanBusy = false, opBusy = false, resetReq = false;
ble::ScanHit hits[16];
int hitCount = 0;

struct Lock {
    Lock() { xSemaphoreTake(mtx, portMAX_DELAY); }
    ~Lock() { xSemaphoreGive(mtx); }
};

// The BR1 requests its own parameters ~5 s after connecting (250 ms, latency
// 0, timeout 2 s). On a held link we keep ours, so the request is declined.
class HoldCallbacks : public NimBLEClientCallbacks {
    bool onConnParamsUpdateRequest(NimBLEClient*, const ble_gap_upd_params*) override { return false; }
};
HoldCallbacks holdCb;

uint32_t backoffMs(uint8_t fails) {
    if (fails == 0) return 0;
    uint8_t e = fails > 3 ? 3 : fails;
    return 1000UL << e;   // 2, 4, 8 s, then every 8 s
}

void setErr(Dev& d, const char* e) { strlcpy(d.st.err, e, sizeof(d.st.err)); }

void resetDev(Dev& d, uint8_t type) {
    memset(&d, 0, sizeof(d));
    d.st.desired = -1;
    d.st.actual = -1;
    d.st.sense = -1;
    d.refreshReq = (type == OUT_RELAY || type == OUT_LINE);
}

void dropClient(Dev& d, const char* why) {
    if (d.client) { NimBLEDevice::deleteClient(d.client); d.client = nullptr; }
    d.st.link = 0;
    if (why) setErr(d, why);
}

// The BR1 accepts a connection only about every second attempt (its
// firmware misses the first connection events now and then, a PC shows the
// same), so one operation tries several times before giving up.
NimBLEClient* connectTo(const Output& o, Dev& d) {
    NimBLEAddress addr(std::string(o.host), o.addrType);
    for (uint8_t attempt = 0; attempt < CONNECT_ATTEMPTS; attempt++) {
        NimBLEClient* c = NimBLEDevice::createClient();
        if (!c) return nullptr;
        c->setConnectTimeout(CONNECT_TIMEOUT_S);
        uint32_t t0 = millis();
        d.st.tries++;
        if (c->connect(addr, true)) {
            Serial.printf("ble %s connected in %lu ms (attempt %u)\n", o.name, (unsigned long)(millis() - t0), attempt + 1);
            return c;
        }
        int rc = c->getLastError();
        Serial.printf("ble %s connect failed rc=%d after %lu ms (attempt %u)\n", o.name, rc, (unsigned long)(millis() - t0), attempt + 1);
        snprintf(d.st.err, sizeof(d.st.err), "connect failed rc=%d", rc);
        d.st.connFails++;
        NimBLEDevice::deleteClient(c);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    return nullptr;
}

// GATT part of a relay operation on an open link. want: 1 on, 0 off, -1 read only.
bool relayIo(NimBLEClient* c, Dev& d, int8_t want) {
    bool ok = false;
    NimBLERemoteService* svc = c->getService(NimBLEUUID(UUID_AIO));
    NimBLERemoteCharacteristic* ch = svc ? svc->getCharacteristic(NimBLEUUID(UUID_DIGITAL)) : nullptr;
    if (!ch) {
        setErr(d, "no relay characteristic");
    } else {
        ok = true;
        if (want >= 0) {
            uint8_t v = want ? 1 : 0;
            if (ch->writeValue(&v, 1, true)) {
                d.st.actual = want;
                delay(50);   // 10 ms coil pulse in the BR1 firmware
            } else {
                ok = false;
                setErr(d, "write failed");
            }
        }
        NimBLEAttValue val = ch->readValue();
        if (val.length() >= 1) {
            d.st.sense = val.data()[0] ? 1 : 0;
            if (want < 0) d.st.actual = d.st.sense;
        }
    }
    NimBLERemoteService* bs = c->getService(NimBLEUUID(UUID_BATT));
    NimBLERemoteCharacteristic* bc = bs ? bs->getCharacteristic(NimBLEUUID(UUID_BATT_LEVEL)) : nullptr;
    if (bc) {
        NimBLEAttValue v = bc->readValue();
        if (v.length() >= 2) d.st.battCv = (int16_t)(v.data()[0] | (v.data()[1] << 8));
    }
    d.st.rssi = c->getRssi();
    if (ok) { d.st.fails = 0; d.st.lastOk = millis(); d.st.err[0] = 0; }
    else d.st.fails++;
    return ok;
}

// Relay without a held link: connect, switch, disconnect (phone app stays usable).
void relayOnce(const Output& o, Dev& d, int8_t want) {
    d.st.lastTry = millis();
    NimBLEClient* c = connectTo(o, d);
    if (!c) { d.st.fails++; return; }
    relayIo(c, d, want);
    NimBLEDevice::deleteClient(c);
}

// Relay with a held link: make sure the link is up, then switch on it.
void relayHeld(const Output& o, Dev& d, bool change, bool refresh) {
    if (!(d.client && d.client->isConnected())) {
        d.st.lastTry = millis();
        dropClient(d, nullptr);
        NimBLEClient* c = connectTo(o, d);
        if (!c) { d.st.fails++; return; }
        c->setClientCallbacks(&holdCb, false);
        c->updateConnParams(HOLD_ITVL, HOLD_ITVL, HOLD_LATENCY, HOLD_TIMEOUT);
        d.client = c;
        d.st.link = 1;
        d.st.fails = 0;
        d.st.err[0] = 0;
        refresh = true;   // fresh link: read state and battery
    }
    if (change || refresh) {
        int8_t want = change ? d.st.desired : -1;
        if (!relayIo(d.client, d, want)) dropClient(d, nullptr);
    }
}

bool lineEnsure(uint8_t idx, const Output& o, Dev& d) {
    if (d.client && d.client->isConnected()) return true;
    if (d.client) dropClient(d, nullptr);
    d.st.lastTry = millis();
    NimBLEClient* c = connectTo(o, d);
    if (!c) { d.st.fails++; d.st.actual = 0; return false; }
    NimBLERemoteService* s = c->getService(NimBLEUUID(UUID_NUS));
    NimBLERemoteCharacteristic* tx = s ? s->getCharacteristic(NimBLEUUID(UUID_NUS_TX)) : nullptr;
    NimBLERemoteCharacteristic* rx = s ? s->getCharacteristic(NimBLEUUID(UUID_NUS_RX)) : nullptr;
    if (!tx || !rx) {
        NimBLEDevice::deleteClient(c);
        d.st.fails++;
        d.st.actual = 0;
        setErr(d, "no uart service");
        return false;
    }
    if (tx->canNotify()) {
        tx->subscribe(true, [idx](NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
            // keep the last complete line per device
            static char acc[OUT_MAX][80];
            static uint8_t pos[OUT_MAX];
            Lock l;
            Dev& dd = devs[idx];
            for (size_t i = 0; i < len; i++) {
                char ch = (char)data[i];
                if (ch == 10 || ch == 13) {
                    if (pos[idx]) {
                        acc[idx][pos[idx]] = 0;
                        strlcpy(dd.st.reply, acc[idx], sizeof(dd.st.reply));
                        pos[idx] = 0;
                    }
                } else if (pos[idx] < sizeof(acc[idx]) - 1) {
                    acc[idx][pos[idx]++] = ch;
                }
            }
        }, true);
    }
    d.client = c;
    d.st.link = 1;
    d.st.actual = 1;
    d.st.fails = 0;
    d.st.err[0] = 0;
    d.st.lastOk = millis();
    d.st.rssi = c->getRssi();
    return true;
}

void lineSend(Dev& d, const char* line) {
    NimBLERemoteService* s = d.client ? d.client->getService(NimBLEUUID(UUID_NUS)) : nullptr;
    NimBLERemoteCharacteristic* rx = s ? s->getCharacteristic(NimBLEUUID(UUID_NUS_RX)) : nullptr;
    if (!rx) { dropClient(d, "no uart service"); d.st.actual = 0; d.st.fails++; return; }
    String msg(line);
    msg += "\n";
    if (!rx->writeValue((const uint8_t*)msg.c_str(), msg.length(), true)) {
        dropClient(d, "write failed");
        d.st.actual = 0;
        d.st.fails++;
        return;
    }
    d.st.lastOk = millis();
    d.st.rssi = d.client->getRssi();
}

void doScan() {
    NimBLEScan* s = NimBLEDevice::getScan();
    s->setActiveScan(true);
    s->setInterval(100);
    s->setWindow(80);
    NimBLEScanResults r = s->start(SCAN_SECONDS, false);
    Lock l;
    hitCount = 0;
    for (int i = 0; i < r.getCount() && hitCount < (int)(sizeof(hits) / sizeof(hits[0])); i++) {
        NimBLEAdvertisedDevice dv = r.getDevice(i);
        uint8_t kind = 0;
        if (dv.isAdvertisingService(NimBLEUUID(UUID_BR1_SVC))) kind = 1;
        else if (dv.isAdvertisingService(NimBLEUUID(UUID_NUS))) kind = 2;
        else if (!dv.haveName()) continue;   // anonymous beacons are noise here
        ble::ScanHit& h = hits[hitCount++];
        strlcpy(h.addr, dv.getAddress().toString().c_str(), sizeof(h.addr));
        h.type = dv.getAddress().getType();
        strlcpy(h.name, dv.haveName() ? dv.getName().c_str() : "", sizeof(h.name));
        h.rssi = dv.getRSSI();
        h.kind = kind;
    }
    s->clearResults();
}

void bleTask(void*) {
    for (;;) {
        if (resetReq) {
            resetReq = false;
            for (uint8_t i = 0; i < OUT_MAX; i++) {
                uint8_t type = i < settings.outCount ? settings.outs[i].type : 0xFF;
                // disconnect outside the lock, deleteClient waits for the link to drop
                if (devs[i].client) { NimBLEDevice::deleteClient(devs[i].client); devs[i].client = nullptr; }
                Lock l;
                resetDev(devs[i], type);
            }
        }
        if (scanReq) {
            scanReq = false;
            scanBusy = true;
            doScan();
            scanBusy = false;
        }
        uint32_t now = millis();
        for (uint8_t i = 0; i < settings.outCount; i++) {
            Output o = settings.outs[i];
            Dev& d = devs[i];
            if (o.type == OUT_RELAY) {
                bool connected = d.client && d.client->isConnected();
                if (!connected && d.st.link) dropClient(d, "link lost");
                bool change = d.st.desired >= 0 && d.st.desired != d.st.actual;
                if (settings.bleHold) {
                    if (connected && now - d.st.lastOk > RELAY_REFRESH_MS) d.refreshReq = true;
                    bool want = change || d.refreshReq || !connected;
                    if (want && now - d.st.lastTry >= backoffMs(d.st.fails)) {
                        bool refresh = d.refreshReq;
                        d.refreshReq = false;
                        opBusy = true;
                        relayHeld(o, d, change, refresh);
                        opBusy = false;
                    }
                } else {
                    if (connected) dropClient(d, nullptr);   // hold switched off
                    if ((change || d.refreshReq) && now - d.st.lastTry >= backoffMs(d.st.fails)) {
                        int8_t want = change ? d.st.desired : -1;
                        d.refreshReq = false;
                        opBusy = true;
                        relayOnce(o, d, want);
                        opBusy = false;
                    }
                }
            } else if (o.type == OUT_LINE) {
                bool connected = d.client && d.client->isConnected();
                if (!connected && d.st.actual == 1) { dropClient(d, "disconnected"); d.st.actual = 0; }
                bool want = d.lineQueued || d.refreshReq || (!connected && now - d.st.lastTry > LINE_RECONNECT_MS);
                if (want && now - d.st.lastTry >= backoffMs(d.st.fails)) {
                    d.refreshReq = false;
                    opBusy = true;
                    if (lineEnsure(i, o, d) && d.lineQueued) {
                        char buf[64];
                        { Lock l; strlcpy(buf, d.line, sizeof(buf)); d.lineQueued = false; }
                        lineSend(d, buf);
                    }
                    opBusy = false;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

}  // namespace

namespace ble {

void begin() {
    mtx = xSemaphoreCreateMutex();
    for (uint8_t i = 0; i < OUT_MAX; i++) { devs[i].client = nullptr; resetDev(devs[i], 0xFF); }
    NimBLEDevice::init(BLE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setMTU(128);
    resetReq = true;
    xTaskCreate(bleTask, "ble", 10240, nullptr, 1, nullptr);
}

void outputsChanged() { resetReq = true; }

void setDesired(uint8_t idx, bool on) {
    if (idx >= OUT_MAX) return;
    Lock l;
    devs[idx].st.desired = on ? 1 : 0;
}

void refresh(uint8_t idx) {
    if (idx >= OUT_MAX) return;
    Lock l;
    devs[idx].refreshReq = true;
    devs[idx].st.fails = 0;
    devs[idx].st.lastTry = 0;
}

bool sendLine(uint8_t idx, const String& line) {
    if (idx >= OUT_MAX) return false;
    Lock l;
    strlcpy(devs[idx].line, line.c_str(), sizeof(devs[idx].line));
    devs[idx].lineQueued = true;
    return true;
}

DevState state(uint8_t idx) {
    DevState s;
    memset(&s, 0, sizeof(s));
    if (idx >= OUT_MAX) return s;
    Lock l;
    s = devs[idx].st;
    return s;
}

bool busy() { return opBusy || scanBusy; }

void scanStart() { if (!scanBusy) scanReq = true; }
bool scanning() { return scanReq || scanBusy; }

int scanResults(ScanHit* buf, int max) {
    Lock l;
    int n = hitCount < max ? hitCount : max;
    for (int i = 0; i < n; i++) buf[i] = hits[i];
    return n;
}

}  // namespace ble
