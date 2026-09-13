/**
 * Copyright (c) 2021-2026 - Pierre Quélin <pierre.quelin.1972@gmail.com>
 *
 * All rights reserved
 *
  * For the full license text, see:
 * https://opensource.org/license/lgpl-3-0
 *
 * @file esplora_hid_smoke.hpp
 * @brief Smoke: event-driven interrupt IN only (no control polling).
 * writeRgb kicks one firmware IN; then block until switch1.
 */

#include <esplora/hid/esplora_hid.hpp>

#include <cstdio>

int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    using namespace esplora::hid;

    Device dev;
    std::printf("count=%d\n", dev.countDevices());
    if (!dev.openByIndex())
    {
        std::printf("open fail %s\n", dev.lastLibusbErrorName());
        return 3;
    }
    std::printf("open OK epIn=0x%02X epOut=0x%02X\n", dev.epIn(), dev.epOut());

    if (!dev.writeRgb(0, 40, 0))
    {
        std::printf("writeRgb fail %s\n", dev.lastLibusbErrorName());
        return 4;
    }
    std::printf("LED green — blocking on interrupt IN (press switch1/DOWN)...\n");

    for (;;)
    {
        InputSnapshot snap{};
        if (!dev.readInput(snap, 0))
        {
            std::printf("IN fail err=%d %s\n",
                        static_cast<int>(dev.lastError()),
                        dev.lastLibusbErrorName());
            std::_Exit(2);
        }
        std::printf("IN sw=%d%d%d%d light=%u\n",
                    snap.switch1 ? 1 : 0,
                    snap.switch2 ? 1 : 0,
                    snap.switch3 ? 1 : 0,
                    snap.switch4 ? 1 : 0,
                    static_cast<unsigned>(snap.lightSensor));
        if (snap.switch1)
        {
            break;
        }
    }

    (void)dev.writeRgb(40, 0, 0);
    (void)dev.writeRgb(0, 0, 0);
    std::printf("PASS\n");
    std::_Exit(0);
}
