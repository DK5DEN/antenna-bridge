#pragma once
#include <Arduino.h>

// Parse and execute one command line. Replies are written to out.
// The same command set is used on USB serial, UDP and the web console.
void handleCommand(String line, Print& out);

// Status line, key=value pairs.
void printStatus(Print& out);

// One line per output with live state.
void printOutputs(Print& out);

// Accept "7100000", "7100.5k", "7.1M" style frequencies.
bool parseFreq(const String& s, uint32_t& hz);
