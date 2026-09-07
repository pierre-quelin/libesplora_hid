# Esplora LUFA firmware

Device firmware lives in a **separate** repository:

**[`pierre-quelin/Esplora-firmware`](https://github.com/pierre-quelin/Esplora-firmware)**

This folder documents the **HID report contract** expected by `libesplora_hid` (must stay aligned with that repo).

Host library expects a **Generic HID** device on the Arduino Esplora (ATmega32U4) built with [LUFA](https://github.com/abcminiuser/lufa).

## Contract with `libesplora_hid`

| Direction | Content |
|-----------|---------|
| IN report (64 bytes) | `[0]` bits0–3 = switch1–4 pressed; `[1..2]` light ADC LE `uint16` |
| OUT report (64 bytes) | `[0]=R [1]=G [2]=B` LED intensities 0–255 |

USB IDs: VID `0x1209` ([pid.codes](https://pid.codes)), PID `0xE5F1` (align with firmware + `esplora_hid_types.hpp`; register the PID before public release). Not LUFA demo `03EB:204F`.

Endpoints: interrupt IN + OUT, size 64.

Firmware is **event-driven**: interrupt IN on switch change or after host OUT (sync). Host `readInput(..., 0)` blocks in the kernel until a packet arrives — no PC busy-poll.
