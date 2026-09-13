/**
 * Copyright (c) 2021-2026 - Pierre Quélin <pierre.quelin.1972@gmail.com>
 *
 * All rights reserved
 *
  * For the full license text, see:
 * https://opensource.org/license/lgpl-3-0
 *
 * @file esplora_hid.hpp
 * @brief Esplora USB HID host API using libusb-1.0 (interrupt transfers).
 * Paired with Arduino Esplora LUFA firmware (see firmware/README.md).
 */
 #pragma once

#include <cstdint>
#include <libusb-1.0/libusb.h>
#include <memory>
#include <string>

#include "esplora_hid_types.hpp"

namespace esplora
{
namespace hid
{

/**
 * Host wrapper: open Esplora HID, read IN reports, write OUT (RGB LED).
 */
class Device
{
public:
    Device();
    ~Device();

    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) noexcept;
    Device& operator=(Device&&) noexcept;

    int countDevices(std::uint16_t vid = DefaultVendorId,
                     std::uint16_t pid = DefaultProductId) const;

    bool openByIndex(std::uint16_t vid = DefaultVendorId,
                     std::uint16_t pid = DefaultProductId,
                     int           deviceIndex = 0);
    void close();
    [[nodiscard]] bool isOpen() const { return _handle != nullptr; }

    [[nodiscard]] Error lastError() const { return _lastError; }
    void                clearLastError() { _lastError = Error::None; }

    [[nodiscard]] int         lastLibusbError() const { return _lastLibusbErr; }
    [[nodiscard]] const char* lastLibusbErrorName() const;

    [[nodiscard]] std::uint8_t epIn() const { return _epIn; }
    [[nodiscard]] std::uint8_t epOut() const { return _epOut; }

    /** Blocking interrupt IN; returns false on USB/protocol error / timeout. */
    bool readRawReport(RawReport& out, unsigned int timeoutMs = 1000);
    /** HID GET_REPORT (Input) on the control pipe — sync without interrupt IN. */
    bool readInputReportControl(RawReport& out, unsigned int timeoutMs = 1000);

    bool decodeInput(const RawReport& report, InputSnapshot& out) const;
    bool readInput(InputSnapshot& out, unsigned int timeoutMs = 1000);
    bool readInputControl(InputSnapshot& out, unsigned int timeoutMs = 1000);

    /** RGB LED OUT report (WHITE unused on Esplora hardware). */
    bool writeRgb(std::uint8_t red, std::uint8_t green, std::uint8_t blue,
                  unsigned int timeoutMs = 1000);

private:
    bool _claimInterfaces();
    void _setError(Error e, int libusbErr = 0);

    libusb_context*       _ctx{nullptr};
    libusb_device_handle* _handle{nullptr};
    std::uint8_t          _epIn{0};
    std::uint8_t          _epOut{0};
    Error                 _lastError{Error::None};
    int                   _lastLibusbErr{0};
};

} // namespace hid
} // namespace esplora
