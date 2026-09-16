#pragma once
#include <Arduino.h>

// Rule engine: takes the current frequency, waits until it has settled and
// drives every output according to the rules.
//   relay / gpio : on while a rule of that output covers the frequency
//   udp / line   : "freq <hz>" is forwarded while a rule covers the frequency,
//                  once per frequency change
namespace engine {

void begin();
void loop();

// New frequency from CAT ("cat") or the console ("manual").
void setFreq(uint32_t hz, const char* source);
uint32_t freq();
const char* source();

// Outputs or rules changed: re-initialise pins, forget what was sent, re-apply.
void outputsChanged();
// Re-apply the current frequency to all outputs now.
void applyNow();

// Last frequency forwarded to a udp/line output, 0 if none.
uint32_t lastSent(uint8_t idx);
// Last reply text from a udp output.
String lastUdpReply(uint8_t idx);
// Last error text for a udp output ("" if fine).
String udpError(uint8_t idx);

}  // namespace engine
