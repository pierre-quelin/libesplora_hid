/**
 * Copyright (c) 2021-2026 - Pierre Quélin <pierre.quelin.1972@gmail.com>
 *
 * All rights reserved
 *
  * For the full license text, see:
 * https://opensource.org/license/lgpl-3-0
 *
 * @file esplora_hid_types.hpp
 * @brief Esplora USB HID host types (report layout TBD with LUFA firmware).
 */
#pragma once

#include <array>
#include <cstdint>

namespace esplora
{
namespace hid
{

/**
 * USB IDs — pid.codes VID (open-source), project PID (register on https://pid.codes
 * before public release so the pair is reserved). Do not reuse Atmel/LUFA demo
 * 03EB:204F (conflicts with every GenericHID demo device).
 */
inline constexpr std::uint16_t DefaultVendorId  = 0x1209U; /* pid.codes */
inline constexpr std::uint16_t DefaultProductId = 0xE5F1U; /* Esplora HID PoC — register */

/** Interrupt report size (negotiate with firmware; start with 64 like MCP2221 Generic HID). */
inline constexpr std::size_t ReportSize = 64U;

using RawReport = std::array<std::uint8_t, ReportSize>;

enum class Error
{
    None,
    NotConnected,
    InvalidArgument,
    UsbTransfer,
    Timeout,
    ProtocolMismatch,
    NotImplemented,
};

/**
 * Decoded input snapshot for Foundation EsploraBoard stubs.
 * Wire layout is provisional — adjust when firmware HID descriptor is fixed.
 */
struct InputSnapshot
{
    bool          switch1{false};
    bool          switch2{false};
    bool          switch3{false};
    bool          switch4{false};
    std::uint16_t lightSensor{0};

    friend bool operator==(const InputSnapshot& a, const InputSnapshot& b) noexcept
    {
        return a.switch1 == b.switch1 && a.switch2 == b.switch2 && a.switch3 == b.switch3 &&
               a.switch4 == b.switch4 && a.lightSensor == b.lightSensor;
    }

    friend bool operator!=(const InputSnapshot& a, const InputSnapshot& b) noexcept
    {
        return !(a == b);
    }
};

} // namespace hid
} // namespace esplora
