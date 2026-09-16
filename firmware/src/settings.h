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
    char     legacyAnt[16]; // former per-output antenna, only read for migration
    uint8_t  reserved[5];
};

// An antenna is a set of rules. Only the rules of the active antenna and
// the global rules (antenna "-") drive the outputs; an output may appear in
// the rules of several antennas.
struct Band {
    uint32_t fmin;
    uint32_t fmax;
};

struct Antenna {
    char name[16];
    char type[12];      // efhw, dipole, vertical, loop, beam, wire, other ... informational
    uint8_t bandCount;  // ranges the antenna works on without a tuner (informational, shown in the UI)
    uint8_t reserved[3];
    Band bands[8];
};

struct Rule {
    char     out[16];       // output name
    uint32_t fmin;          // Hz, inclusive
    uint32_t fmax;          // Hz, inclusive
    char     ant[16];       // antenna this rule belongs to, empty = global (every antenna)
    uint8_t  state;         // 1 = output on inside the range, 0 = forced off (off wins over on)
    uint8_t  reserved[3];
};

struct Settings {
    char     rig[24]   = "ftx1";        // id of the rig profile (rig.h)
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
    bool ruleAdd(const String& ant, const String& out, uint32_t fmin, uint32_t fmax, bool on = true);   // ant "-" = global
    bool ruleDel(uint8_t i);
    void ruleClear() { ruleCount = 0; }
    bool ruleMatch(const char* out, uint32_t hz) const;     // on: an "on" rule of the active antenna or a global one covers hz and no "off" rule does
    bool ruleActive(const Rule& r) const;                    // rule belongs to the active antenna or is global
    bool outUsedBy(const char* out, const char* ant) const;  // any rule of that antenna names the output

    int  antIndex(const String& name) const;
    bool antAdd(const String& name, const String& type);
    bool antType(const String& name, const String& type);
    bool antDel(const String& name);            // its rules go with it
    bool antRename(const String& name, const String& newName);   // rules and the active selection follow
    bool antBandAdd(const String& name, uint32_t fmin, uint32_t fmax);
    bool antBandDel(const String& name, uint8_t i);
    int  antDirect(const Antenna& a, uint32_t hz) const;   // 1 inside a direct range, 0 outside, -1 no ranges given
    bool antSelect(const String& name);         // empty string deselects

    bool wifiAdd(const String& ssid, const String& pass);
    bool wifiDel(const String& ssid);
};

extern Settings settings;
