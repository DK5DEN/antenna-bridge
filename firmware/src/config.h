#pragma once
#include <Arduino.h>

// ---- Pin assignment (ESP32-C3 Super Mini) ----
constexpr uint8_t PIN_LED    = 8;   // on-board LED, active low
constexpr int8_t  PIN_CAT_RX = 20;  // U0RXD <- rig TXD (through level shifter, rig side is 5 V CMOS)
constexpr int8_t  PIN_CAT_TX = 21;  // U0TXD -> rig RXD (through level shifter)

// ---- Defaults (persisted in NVS, changeable with "set") ----
constexpr uint32_t DEF_CAT_BAUD    = 38400;  // FTX-1 CAT-3 factory default
constexpr uint8_t  DEF_CIV_ADDR    = 0xA4;   // IC-705
constexpr uint8_t  CIV_OWN_ADDR    = 0xE0;   // controller address on the CI-V bus
constexpr uint32_t DEF_CAT_POLL_MS = 1000;   // FA;FB;FT; poll interval
constexpr uint32_t DEF_SETTLE_MS   = 300;    // frequency must be stable this long before outputs switch
constexpr uint16_t DEF_UDP_PORT    = 4220;   // own command port (magloop-tune uses 4210)

constexpr uint8_t OUT_MAX  = 8;
constexpr uint8_t RULE_MAX = 32;
constexpr uint8_t ANT_MAX  = 8;

// GPIO pins that may be used for local relay outputs or the CAT UART. 8 is
// the LED, 9 is the boot strap, 18/19 are USB.
constexpr uint8_t GPIO_ALLOWED[] = {0, 1, 2, 3, 4, 5, 6, 7, 10, 20, 21};

constexpr const char* FW_NAME    = "antenna-bridge";
constexpr const char* FW_VERSION = "0.1.0";
constexpr const char* MDNS_NAME  = "antbridge";
constexpr const char* BLE_NAME   = "antenna-bridge";
