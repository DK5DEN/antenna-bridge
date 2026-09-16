#pragma once
#include <Arduino.h>
#include <IPAddress.h>

// WiFi station with several stored networks, access point fallback for
// configuration, UDP command port and mDNS.
namespace net {

void begin();
void loop();

bool staConnected();
bool apActive();
const char* apSsid();
const char* apPass();
IPAddress ip();          // STA address when connected, otherwise AP address
String stateText();      // "sta", "ap", "connecting", "off"

// Credentials changed: rebuild the network list and try again soon.
void reconnect();

// Asynchronous network scan. start returns immediately; results returns the
// number of networks found, -1 while running, -2 when no scan was started.
void scanStart();
int  scanResults();

}  // namespace net
