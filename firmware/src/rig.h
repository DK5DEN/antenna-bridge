#pragma once
#include <Arduino.h>

// Rig profiles: how to talk CAT to a radio, as a small JSON document.
//
//   {
//     "id": "ftx1", "name": "Yaesu FTX-1 (TUNER/LINEAR, CAT-3)",
//     "author": "DK5DEN", "version": 1,
//     "family": "ascii",            ascii | civ | none
//     "baud": 38400, "invert": false,
//     "wiring": "free text, how the jack is connected",
//     "ascii": {
//       "term": ";",                 answer terminator
//       "poll": "FA;FB;FT;",         sent every catpoll ms
//       "init": "AI1;", "initEvery": 10,   auto information, repeated every n seconds
//       "main": {"prefix": "FA", "skip": 0, "digits": 9},   frequency answers
//       "sub":  {"prefix": "FB", "skip": 0, "digits": 9},   digits 0 = all remaining
//       "info": [{"prefix": "IF", "skip": 5, "digits": 9, "to": "main"}],
//       "tx":   {"prefix": "FT", "sub": "1"}   answer value that means "sub transmits"
//     },
//     "civ": {"addr": "a4", "poll": ["03", "2501", "0F"],
//             "main": "03", "sub": "2501", "split": "0F", "transceive": "00"}
//   }
//
// Built-in profiles live in flash, user profiles as files in LittleFS
// (/rigs/<id>.json). The same documents are shared through afu.tools.
namespace rig {

enum Family : uint8_t { FAM_NONE = 0, FAM_ASCII = 1, FAM_CIV = 2 };

struct Spec {
    char    prefix[4];
    uint8_t skip;
    uint8_t digits;     // 0: all remaining digits
};

struct Profile {
    char     id[24];
    char     name[48];
    uint8_t  family;
    uint32_t baud;
    bool     invert;
    uint8_t  civAddr;
    // ascii
    char     term;
    char     poll[40];
    char     init[24];
    uint16_t initEvery;
    Spec     mainSpec, subSpec;
    Spec     info[4];
    uint8_t  infoTo[4];     // 0 main, 1 sub
    uint8_t  infoCount;
    char     txPrefix[4];
    char     txSubVal[4];
    // civ
    uint8_t  civPoll[4][4];
    uint8_t  civPollLen[4];
    uint8_t  civPollCount;
    uint8_t  civMain;       // command carrying the operating frequency (03), 00 = transceive
    uint8_t  civSub[2];     // command + sub for the other VFO (25 01), 0 = none
    uint8_t  civSplit;      // split state command (0F), 0 = none
    uint8_t  civTransceive; // broadcast command (00)
};

struct Entry {
    char id[24];
    char name[48];
    uint8_t family;
    uint32_t baud;
    bool stored;            // user profile in LittleFS (false: built in)
};

void begin();                                    // mount LittleFS, load the profile named in settings.rig
const Profile& current();
bool parse(const String& json, Profile& p, String& err);   // validate a document
bool select(const String& id, String& err);      // load by id (built in or stored), apply baud/invert/civaddr to settings, restart CAT
bool import(const String& json, String& err);    // validate and store as /rigs/<id>.json
bool remove(const String& id);                   // stored profiles only
String document(const String& id);               // the JSON text, "" if unknown
int  list(Entry* buf, int max);                  // built in first, then stored
const char* familyName(uint8_t f);

}  // namespace rig
