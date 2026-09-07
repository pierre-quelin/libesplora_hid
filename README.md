# libesplora_hid

C++ host API for an **Arduino Esplora** over **USB HID** (libusb-1.0 interrupt transfers), paired with **LUFA** device firmware in [`Esplora-firmware`](https://github.com/pierre-quelin/Esplora-firmware) (host contract notes: [`firmware/README.md`](firmware/README.md)).

Public include:

```cpp
#include <esplora/hid/esplora_hid.hpp>
```

## Status

| Piece | State |
|-------|--------|
| Host open / interrupt IN-OUT | Implemented (libusb) |
| Report decode (switches, light, RGB) | **Provisional** layout — must match firmware |
| LUFA firmware | [`Esplora-firmware`](https://github.com/pierre-quelin/Esplora-firmware) (Generic HID IN/OUT 64) |
| GitHub publish | Local sibling `C:/Projects/libesplora_hid` (Foundation fetches via `fs`; intended remote name `libesplora_hid`) |

## Solo build

```bat
Build.bat fetch
Build.bat gen
```

Produces `dist/<BUILD_TARGET>/lib/libesplora_hid.lib` and headers under `include/esplora/hid/`.

## Live smoke (Windows + board plugged)

```bat
tools\run_esplora_hid_smoke.bat
```

Builds and runs a small host that `writeRgb` (sync IN) then blocks until **switch1** (DOWN). Requires `Build.bat fetch` once (libusb) and event-driven firmware (IN on switch change / OUT sync).

## Foundation

Declared in Foundation `Description.xml` as component `esplora_hid` → path `lib/esplora_hid` (same pattern as `mcp2221a_hid` → `lib/mcp2221a_hid`). CMake target / artifact: **`libesplora_hid`**. Run `Build.bat fetch` from Foundation after this tree exists. Optional `add_subdirectory` / link from `EsploraBoard` comes with the HID pump lot.
