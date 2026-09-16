#pragma once
#include <Arduino.h>
#include "config.h"

// Output types. An output is one thing the bridge can drive; rules map
// frequency ranges onto outputs.
//   udp   : sends "freq <hz>" as a UDP datagram (magloop-tune protocol)
//   relay : IK6BAK BR1 Bluetooth latching relay, on inside a rule, off outside
//   line  : BLE Nordic UART Service target, receives "freq <hz>" lines
//   gpio  : local pin, high inside a rule (or low with invert)
enum OutType : uint8_t { OUT_UDP = 0, OUT_RELAY = 1, OUT_LINE = 2, OUT_GPIO = 3 };
const char* outTypeName(uint8_t t);
bool outTypeParse(const String& s, uint8_t& t);

struct Output {
    char     name[16];
    uint8_t  type;
    char     host[40];      // udp: host name or ip; relay/line: BLE address aa:bb:cc:dd:ee:ff
    uint16_t port;          // udp
    uint8_t  addrType;      // ble: 0 public, 1 random
    uint8_t  pin;           // gpio
    uint8_t  invert;        // gpio: 1 = active low
    char     antenna[16];   // antenna this output belongs to, empty = always active
    uint8_t  reserved[5];
};

// An antenna groups outputs. Only the outputs of the active antenna (and
// unassigned outputs) follow their rules; the others are treated as off.
struct Antenna {
    char name[16];
    uint8_t reserved[8];
};

struct Rule {
    char     out[16];       // output name
    uint32_t fmin;          // Hz, inclusive
    uint32_t fmax;          // Hz, inclusive
};

// CAT protocol family. Presets in cat.cpp map rig names onto these.
enum RigProto : uint8_t { PROTO_NONE = 0, PROTO_YAESU = 1, PROTO_KENWOOD = 2, PROTO_ICOM = 3 };
const char* protoName(uint8_t p);
bool protoParse(const String& s, uint8_t& p);

struct Settings {
    char     rig[16]   = "ftx1";        // preset name, informational
    uint8_t  proto     = PROTO_YAESU;
    int8_t   catRx     = PIN_CAT_RX;
    int8_t   catTx     = PIN_CAT_TX;
    bool     catInvert = false;         // invert UART levels (inverted TTL from a bare RS-232 line driver)
    uint8_t  civAddr   = DEF_CIV_ADDR;  // Icom CI-V address of the rig
    uint32_t catBaud   = DEF_CAT_BAUD;
    uint32_t catPollMs = DEF_CAT_POLL_MS;
    uint8_t  catVfo    = 0;             // 0 follow FT (transmitting side), 1 main, 2 sub
    uint32_t settleMs  = DEF_SETTLE_MS;
    uint16_t udpPort   = DEF_UDP_PORT;
    bool     wifiOn    = true;          // false: radio stays off (portable, Bluetooth only), reboot
    bool     bleHold   = true;          // keep BR1 links open

    struct WifiNet { String ssid; String pass; };
    static constexpr uint8_t WIFI_MAX = 5;
    WifiNet  wifi[WIFI_MAX];
    uint8_t  wifiCount = 0;

    Output   outs[OUT_MAX];
    uint8_t  outCount = 0;
    Rule     rules[RULE_MAX];
    uint8_t  ruleCount = 0;
    Antenna  ants[ANT_MAX];
    uint8_t  antCount = 0;
    char     activeAnt[16] = "";     // name of the selected antenna, empty = none

    void load();
    void save();
    void saveOuts();
    void saveRules();
    void saveAnts();
    void saveWifi();

    int  outIndex(const String& name) const;
    bool outAdd(const Output& o);           // add, or replace an output with the same name
    bool outDel(const String& name);        // removes the output and its rules
    bool ruleAdd(const String& out, uint32_t fmin, uint32_t fmax);
    bool ruleDel(uint8_t i);
    void ruleClear() { ruleCount = 0; }
    bool ruleMatch(const char* out, uint32_t hz) const;

    int  antIndex(const String& name) const;
    bool antAdd(const String& name);
    bool antDel(const String& name);            // outputs of it become unassigned
    bool antSelect(const String& name);         // empty string deselects
    bool outAssign(const String& out, const String& ant);   // ant "-" or empty clears
    bool outActive(const Output& o) const;      // unassigned or belongs to the active antenna

    bool wifiAdd(const String& ssid, const String& pass);
    bool wifiDel(const String& ssid);
};

extern Settings settings;
