#pragma once
#include <Arduino.h>

// CAT reader on a UART (default UART0, GPIO20 RX / GPIO21 TX), driven by the
// rig profile (rig.h): line based ASCII families (Yaesu, Kenwood, Elecraft
// ...) or Icom CI-V binary frames.
namespace cat {

void begin();
void loop();
void restart();          // reopen the UART after a settings or profile change

bool     linkOk();       // an answer arrived recently
uint32_t lastRxAgeMs();
uint32_t freqMain();     // VFO A / main, 0 if unknown
uint32_t freqSub();      // VFO B / sub, 0 if unknown
int8_t   txSide();       // 0 main, 1 sub, -1 unknown
uint32_t txFreq();       // frequency of the side selected by settings.catVfo, 0 if unknown
uint32_t rxCount();
String   lastMessage();  // last decoded message, hex for CI-V

// Send a raw command to the rig (ASCII with terminator, or hex bytes for CI-V).
void send(const String& s);

}  // namespace cat
