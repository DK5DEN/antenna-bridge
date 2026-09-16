#pragma once
#include <Arduino.h>

// CAT reader on a UART (default UART0, GPIO20 RX / GPIO21 TX). Three
// protocol families:
//   yaesu    ASCII "FA014074000;" style (FTX-1, FT-DX10, FT-891, FT-991 ...),
//            polled with "FA;FB;FT;", auto information with "AI1;"
//   kenwood  same ASCII family with 11 digit frequencies (TS-590/890,
//            Elecraft K3/KX2/KX3), "AI2;" for auto information
//   icom     CI-V binary frames FE FE to from cmd ... FD, frequency polled
//            with command 03, transceive broadcasts (command 00) accepted
// Rig presets (rig list) set protocol, baud rate and the wiring notes.
namespace cat {

struct Preset {
    const char* name;
    uint8_t     proto;
    uint32_t    baud;
    uint8_t     civAddr;
    const char* rigName;
    const char* wiring;
};
const Preset* presets(uint8_t& count);
const Preset* preset(const String& name);
bool applyPreset(const String& name);   // sets settings.proto/catBaud/civAddr/rig, restarts the UART

void begin();
void loop();
void restart();          // reopen the UART after a settings change

bool     linkOk();       // an answer arrived recently
uint32_t lastRxAgeMs();
uint32_t freqMain();     // VFO A / main, 0 if unknown
uint32_t freqSub();      // VFO B / sub, 0 if unknown
int8_t   txSide();       // 0 main, 1 sub, -1 unknown
uint32_t txFreq();       // frequency of the side selected by settings.catVfo, 0 if unknown
uint32_t rxCount();
String   lastMessage();  // last decoded message, hex for CI-V

// Send a raw command to the rig (ASCII ";" terminated, or hex bytes for CI-V).
void send(const String& s);

}  // namespace cat
