// antenna-bridge — CAT to antenna control bridge.
// ESP32-C3 Super Mini at the rig (Yaesu FTX-1, CAT-3 on the TUNER/LINEAR
// jack). Reads the operating frequency and drives antenna hardware through
// rules: IK6BAK BR1 Bluetooth relays, magloop-tune over UDP or Bluetooth,
// local GPIO pins.
//
// Console: USB CDC (115200), line-based commands, see "help".
// Network: web UI on port 80, same commands over UDP (default port 4220).
// Without a known WiFi the bridge opens an access point for setup.

#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "cat.h"
#include "rig.h"
#include "engine.h"
#include "ble.h"
#include "commands.h"
#include "net.h"
#include "web.h"

namespace {

String serialLine;

void serialLoop() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            handleCommand(serialLine, Serial);
            serialLine = "";
        } else if (serialLine.length() < 4200) {   // rig import carries a JSON document
            serialLine += c;
        }
    }
}

// LED: short blink every 2 s with CAT link, every 0.5 s without; solid while
// a Bluetooth operation runs.
void ledLoop() {
    static uint32_t last = 0;
    static bool on = false;
    uint32_t now = millis();
    if (ble::busy()) {
        digitalWrite(PIN_LED, LOW);
        on = true;
        return;
    }
    uint32_t period = cat::linkOk() ? 2000 : 500;
    if (now - last >= (on ? 50 : period)) {
        on = !on;
        last = now;
        digitalWrite(PIN_LED, on ? LOW : HIGH);
    }
}

void catToEngine() {
    static uint32_t lastReported = 0;
    uint32_t f = cat::txFreq();
    if (f && f != lastReported) {
        lastReported = f;
        engine::setFreq(f, "cat");
    }
}

}  // namespace

void setup() {
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    settings.load();

    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 1500) delay(10);
    Serial.printf("\n%s %s ready, outs=%u rules=%u wifi=%u catbaud=%lu\n", FW_NAME, FW_VERSION,
                  settings.outCount, settings.ruleCount, settings.wifiCount, (unsigned long)settings.catBaud);

    rig::begin();
    cat::begin();
    ble::begin();
    net::begin();
    engine::begin();
    web::begin();
}

void loop() {
    cat::loop();
    catToEngine();
    engine::loop();
    serialLoop();
    net::loop();
    web::loop();
    ledLoop();
    delay(1);
}
