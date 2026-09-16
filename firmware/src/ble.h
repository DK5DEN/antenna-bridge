#pragma once
#include <Arduino.h>

// BLE central. Runs in its own task so that connects (up to several seconds)
// never stall the console, UDP or the web server.
//
//   relay outputs (IK6BAK BR1): by default the link is kept open (settings
//     bleHold) with slave latency, so a switch is a single write and the
//     unreliable connection setup of the BR1 happens once. With hold off the
//     bridge connects per operation and releases the link, which keeps the
//     phone app usable but makes every switch a connect lottery.
//   line outputs (Nordic UART Service, e.g. magloop-tune): the connection is
//     kept open, lines are written to the RX characteristic, replies arrive
//     as notifications on TX.
namespace ble {

struct DevState {
    int8_t   desired;    // relay: -1 none, 0 off, 1 on
    int8_t   actual;     // relay: last state written/read, -1 unknown; line: 1 connected, 0 not
    int8_t   sense;      // relay: value read back from the device, -1 unknown
    int16_t  battCv;     // relay: battery in 1/100 V, 0 unknown
    int8_t   rssi;       // 0 unknown
    uint32_t lastOk;     // millis of the last successful operation, 0 never
    uint32_t lastTry;
    uint8_t  fails;      // consecutive failures, drives the retry backoff
    uint16_t tries;      // connect attempts since boot
    uint16_t connFails;  // failed connect attempts since boot
    uint8_t  link;       // 1 while a connection to the device is open
    char     err[32];
    char     reply[64];  // line: last reply line received
};

struct ScanHit {
    char    addr[18];
    uint8_t type;        // 0 public, 1 random
    char    name[24];
    int8_t  rssi;
    uint8_t kind;        // 0 other, 1 BR1 relay, 2 uart line target
};

void begin();
void outputsChanged();               // settings.outs changed: drop connections, forget states
void setDesired(uint8_t idx, bool on);
void refresh(uint8_t idx);           // read state/battery (relay) or reconnect (line)
bool sendLine(uint8_t idx, const String& line);
DevState state(uint8_t idx);
bool busy();                         // an operation is running right now

void scanStart();
bool scanning();
int  scanResults(ScanHit* buf, int max);

}  // namespace ble
