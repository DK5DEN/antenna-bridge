# IK6BAK BR1 remote switch — Bluetooth protocol

Source: the switch firmware is open source (GPL-3.0),
[eliachiarucci/EFHW-Switch](https://github.com/eliachiarucci/EFHW-Switch),
nRF52832 + Zephyr / nRF Connect SDK 2.9. The Android app
`eliachiarucci.android.ble_app` ("EFHW Remote Switch", Flutter +
flutter_blue_plus) only uses the characteristics below; firmware updates go
through the standard mcumgr SMP service. Firmware images are published at
`https://raw.github.com/eliachiarucci/builds/main/BR1-12/` (`latest-version.txt`).

## Hardware

- nRF52832, CR2032 coin cell, latching relay driven by SET/RESET pulses of
  10 ms (P0.08 set, P0.09 reset), relay sense input P0.10, status LED P0.07.
- Advertising interval 1.5 s (2400 × 0.625 ms), TX power +4 dBm. A scan has
  to run at least 2–3 s to see the device.
- Preferred connection interval 250 ms, supervision timeout 2 s. Every GATT
  round trip takes about one interval, a switch operation therefore takes
  1–3 s including connect.
- `CONFIG_BT_MAX_CONN=1`: one central at a time. While the phone app is
  connected nobody else can connect, and vice versa. Keep connections short.
- SMP pairing is enabled and auto-confirmed, but no characteristic requires
  encryption. Plain connections work, no bonding needed.
- Address: random static (Zephyr default with `CONFIG_BT_SETTINGS`), stable
  across reboots. Use address type "random" when connecting.

## Advertising

| Field | Content |
| --- | --- |
| Flags | general discoverable, BR/EDR not supported |
| Complete local name | `BR1` (default), changeable, up to 31 characters |
| Scan response | 128-bit service UUID `67d03ad6-34ba-450a-a39c-00805f9b34fb` |

The custom UUID only marks the device, it is not a GATT service with
characteristics. Active scanning is needed to receive it.

## GATT

| Service | Characteristic | UUID | Properties | Value |
| --- | --- | --- | --- | --- |
| Automation IO `0x1815` | Digital | `0x2A56` | read, write | write 1 byte: `0x01` relay ON (SET pulse), anything else OFF (RESET pulse). Read: 4-byte little-endian int, relay sense pin (1 = on). No encryption required. |
| Battery `0x180F` | Battery Level | `0x2A19` | read | `int16` little-endian, cell voltage × 100 (e.g. `0x012A` = 2.98 V). Not a percentage. |
| Device Information `0x180A` | Firmware Revision | `0x2A26` | read | 3 raw bytes, e.g. `00 01 08` = 0.1.8 |
| | Hardware Revision | `0x2A27` | read | `uint32` little-endian |
| | Model Number | `0x2A24` | read | ASCII `BR1` |
| Custom `397bfacd-f6c0-470e-8c72-5b17d34e1259` | same UUID | write | new advertising name, ≤ 31 bytes ASCII. Device stores it and reboots (takes up to a minute per the app). |
| Bond Management `0x181E` | | | | delete bonds; "delete all" / "delete rest" need the authorization code `ABCD` |
| SMP (mcumgr) `8D53DC1D-1DB7-4CD3-868B-8A527460AA84` | `DA2E7828-FBCE-4E01-AE9E-261174997C48` | write, notify | firmware update, MCUboot image slot |

The Characteristic Presentation Format descriptor `0x2904` attached at the
end of the service table (format `uint16`, unit volts) belongs, by position,
to the Model Number characteristic — a bug in the firmware; ignore it.

## Switching sequence (as used by antenna-bridge)

1. Connect to the stored random address, timeout 8 s, up to four attempts.
2. Discover services, write `01` or `00` to `0x2A56` with response.
3. Wait 50 ms, read `0x2A56` (sense), read `0x2A19` (battery).
4. Keep the link (default) or disconnect (`blehold 0`).

## Connection reliability (measured 2026-09-16)

The BR1 accepts a connection only about every second attempt: 40–48 % of
connect requests from an ESP32-C3 (NimBLE) end with HCI error 0x3E
"connection failed to be established", a Windows PC (bleak) connected 3 of
10 times and often saw no advertising for 10 s. Independent of WiFi
coexistence, TX power and initial connection interval on the central side,
so the cause is in the switch (Zephyr controller on the RC low-frequency
clock, `CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC`, 500 ppm declared). The phone
app hides it behind automatic retries.

antenna-bridge therefore keeps the link open by default: one connection
(up to four attempts), then 250 ms interval with slave latency 4 and 6 s
supervision timeout (the BR1's own parameter request — 250 ms, latency 0,
2 s — is declined). A switch is then a single write and takes 1–2 s, the
switch wakes every 1.25 s like it does for advertising. The LED pattern
while connected has the same duty cycle as while advertising.

## nRF Connect quick test

Scan → `BR1` → Connect → Automation IO → Digital → write `01` (on) or `00`
(off). Battery Service → read → interpret the two bytes as little-endian
hundredths of a volt.
