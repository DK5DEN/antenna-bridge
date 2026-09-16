#pragma once
#include <Arduino.h>

// HTTP server on port 80: single-page UI plus a small JSON/text API.
//   GET  /            UI
//   GET  /api/status  JSON with frequency, CAT link, outputs, rules, scan result, settings, wifi
//   POST /api/cmd     form field "line": one console command, text reply
//   POST /api/wifi    form fields action=add|del, ssid, pass
//   GET  /api/scan    start / poll a WiFi network scan
namespace web {
void begin();
void loop();
}
