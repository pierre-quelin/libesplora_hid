/**
 * Copyright (c) 2021-2026 - Pierre Quélin <pierre.quelin.1972@gmail.com>
 *
 * All rights reserved
 *
 * For the full license text, see:
 * https://opensource.org/license/lgpl-3-0
 */

#include "esplora/hid/esplora_hid.hpp"

#include <cstring>
#include <utility>

namespace esplora
{
namespace hid
{
namespace
{

constexpr std::uint8_t HidGetReport       = 0x01;
constexpr std::uint8_t HidReportTypeInput = 0x01;
/** Windows HID/WinUSB may write a Report-ID byte past the requested length (stack smash). */
constexpr int HidOverrunSlop = 16;

bool discoverInterruptEndpoints(libusb_device_handle* handle, std::uint8_t& epIn, std::uint8_t& epOut)
{
    epIn  = 0;
    epOut = 0;
    libusb_device*                          dev = libusb_get_device(handle);
    struct libusb_config_descriptor* cfg = nullptr;
    if (libusb_get_active_config_descriptor(dev, &cfg) != 0 || cfg == nullptr)
    {
        return false;
    }
    for (int i = 0; i < cfg->bNumInterfaces; ++i)
    {
        const auto& iface = cfg->interface[i];
        for (int a = 0; a < iface.num_altsetting; ++a)
        {
            const auto& alt = iface.altsetting[a];
            for (int e = 0; e < alt.bNumEndpoints; ++e)
            {
                const auto& ep = alt.endpoint[e];
                if ((ep.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_INTERRUPT)
                {
                    continue;
                }
                if ((ep.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
                {
                    epIn = ep.bEndpointAddress;
                }
                else
                {
                    epOut = ep.bEndpointAddress;
                }
            }
        }
    }
    libusb_free_config_descriptor(cfg);
    return epIn != 0;
}

void setConfigurationIfNeeded(libusb_device_handle* handle)
{
    if (handle == nullptr)
    {
        return;
    }
    int cfg = 0;
    if (libusb_get_configuration(handle, &cfg) == 0 && cfg == 1)
    {
        return;
    }
    (void)libusb_set_configuration(handle, 1);
}

} // namespace

Device::Device()
{
    (void)libusb_init(&_ctx);
}

Device::~Device()
{
    close();
    if (_ctx != nullptr)
    {
        libusb_exit(_ctx);
        _ctx = nullptr;
    }
}

Device::Device(Device&& other) noexcept
{
    *this = std::move(other);
}

Device& Device::operator=(Device&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }
    close();
    if (_ctx != nullptr)
    {
        libusb_exit(_ctx);
    }
    _ctx           = other._ctx;
    _handle        = other._handle;
    _epIn          = other._epIn;
    _epOut         = other._epOut;
    _lastError     = other._lastError;
    _lastLibusbErr = other._lastLibusbErr;
    other._ctx     = nullptr;
    other._handle  = nullptr;
    other._epIn    = 0;
    other._epOut   = 0;
    return *this;
}

void Device::_setError(Error e, int libusbErr)
{
    _lastError     = e;
    _lastLibusbErr = libusbErr;
}

const char* Device::lastLibusbErrorName() const
{
    if (_lastLibusbErr == 0)
    {
        return "";
    }
    return libusb_error_name(_lastLibusbErr);
}

int Device::countDevices(std::uint16_t vid, std::uint16_t pid) const
{
    if (_ctx == nullptr)
    {
        return -1;
    }
    libusb_device** list = nullptr;
    const ssize_t   n    = libusb_get_device_list(_ctx, &list);
    if (n < 0)
    {
        return -1;
    }
    int count = 0;
    for (ssize_t i = 0; i < n; ++i)
    {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(list[i], &desc) != 0)
        {
            continue;
        }
        if (desc.idVendor == vid && desc.idProduct == pid)
        {
            ++count;
        }
    }
    libusb_free_device_list(list, 1);
    return count;
}

bool Device::_claimInterfaces()
{
    if (_handle == nullptr)
    {
        return false;
    }
    setConfigurationIfNeeded(_handle);
    if (libusb_kernel_driver_active(_handle, 0) == 1)
    {
        (void)libusb_detach_kernel_driver(_handle, 0);
    }
    const int rc = libusb_claim_interface(_handle, 0);
    if (rc != 0)
    {
        _setError(Error::UsbTransfer, rc);
        return false;
    }
    if (!discoverInterruptEndpoints(_handle, _epIn, _epOut))
    {
        (void)libusb_release_interface(_handle, 0);
        _setError(Error::UsbTransfer);
        return false;
    }
    // Sticky HALT after (re)claim — same as mcp2221a_hid (Windows/WinUSB).
    if (_epIn != 0)
    {
        (void)libusb_clear_halt(_handle, _epIn);
    }
    if (_epOut != 0)
    {
        (void)libusb_clear_halt(_handle, _epOut);
    }
    return true;
}

bool Device::openByIndex(std::uint16_t vid, std::uint16_t pid, int deviceIndex)
{
    close();
    clearLastError();
    if (_ctx == nullptr || deviceIndex < 0)
    {
        _setError(Error::InvalidArgument);
        return false;
    }
    libusb_device** list = nullptr;
    const ssize_t   n    = libusb_get_device_list(_ctx, &list);
    if (n < 0)
    {
        _setError(Error::UsbTransfer, static_cast<int>(n));
        return false;
    }
    int match = -1;
    for (ssize_t i = 0; i < n; ++i)
    {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(list[i], &desc) != 0)
        {
            continue;
        }
        if (desc.idVendor != vid || desc.idProduct != pid)
        {
            continue;
        }
        ++match;
        if (match != deviceIndex)
        {
            continue;
        }
        const int openRc = libusb_open(list[i], &_handle);
        libusb_free_device_list(list, 1);
        if (openRc != 0 || _handle == nullptr)
        {
            _handle = nullptr;
            _setError(Error::UsbTransfer, openRc);
            return false;
        }
        if (!_claimInterfaces())
        {
            close();
            return false;
        }
        return true;
    }
    libusb_free_device_list(list, 1);
    _setError(Error::NotConnected);
    return false;
}

void Device::close()
{
    if (_handle != nullptr)
    {
        (void)libusb_release_interface(_handle, 0);
        libusb_close(_handle);
        _handle = nullptr;
    }
    _epIn  = 0;
    _epOut = 0;
}

bool Device::readRawReport(RawReport& out, unsigned int timeoutMs)
{
    if (!isOpen() || _epIn == 0)
    {
        _setError(Error::NotConnected);
        return false;
    }
    // Extra slop: some Windows backends overrun by a Report-ID byte (see mcp2221a_hid).
    unsigned char buf[ReportSize + HidOverrunSlop] = {};
    int           transferred                       = 0;
    const int     rc = libusb_interrupt_transfer(_handle, _epIn, buf, static_cast<int>(ReportSize),
                                                 &transferred, timeoutMs);
    if (rc != 0)
    {
        if (rc == LIBUSB_ERROR_TIMEOUT)
        {
            _setError(Error::Timeout, rc);
        }
        else
        {
            _setError(Error::UsbTransfer, rc);
        }
        return false;
    }
    if (transferred <= 0)
    {
        _setError(Error::UsbTransfer);
        return false;
    }
    if (transferred > static_cast<int>(ReportSize) + HidOverrunSlop)
    {
        _setError(Error::UsbTransfer, LIBUSB_ERROR_OVERFLOW);
        return false;
    }
    const int copyN = (transferred > static_cast<int>(ReportSize)) ? static_cast<int>(ReportSize)
                                                                   : transferred;
    std::memcpy(out.data(), buf, static_cast<std::size_t>(copyN));
    if (copyN < static_cast<int>(ReportSize))
    {
        std::memset(out.data() + copyN, 0, ReportSize - static_cast<std::size_t>(copyN));
    }
    clearLastError();
    return true;
}

bool Device::readInputReportControl(RawReport& out, unsigned int timeoutMs)
{
    if (!isOpen())
    {
        _setError(Error::NotConnected);
        return false;
    }
    out.fill(0);
    const std::uint16_t wValue =
        static_cast<std::uint16_t>((static_cast<std::uint16_t>(HidReportTypeInput) << 8) | 0U);
    const int st = libusb_control_transfer(
        _handle,
        static_cast<std::uint8_t>(LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS |
                                  LIBUSB_RECIPIENT_INTERFACE),
        HidGetReport,
        wValue,
        0,
        out.data(),
        static_cast<std::uint16_t>(out.size()),
        timeoutMs);
    if (st < 0)
    {
        if (st == LIBUSB_ERROR_TIMEOUT)
        {
            _setError(Error::Timeout, st);
        }
        else
        {
            _setError(Error::UsbTransfer, st);
        }
        return false;
    }
    if (st == 0)
    {
        _setError(Error::UsbTransfer, LIBUSB_ERROR_IO);
        return false;
    }
    clearLastError();
    return true;
}

bool Device::decodeInput(const RawReport& report, InputSnapshot& out) const
{
    out.switch1     = (report[0] & 0x01U) != 0;
    out.switch2     = (report[0] & 0x02U) != 0;
    out.switch3     = (report[0] & 0x04U) != 0;
    out.switch4     = (report[0] & 0x08U) != 0;
    out.lightSensor = static_cast<std::uint16_t>(report[1]) |
                      (static_cast<std::uint16_t>(report[2]) << 8);
    return true;
}

bool Device::readInput(InputSnapshot& out, unsigned int timeoutMs)
{
    RawReport raw{};
    if (!readRawReport(raw, timeoutMs))
    {
        return false;
    }
    return decodeInput(raw, out);
}

bool Device::readInputControl(InputSnapshot& out, unsigned int timeoutMs)
{
    RawReport raw{};
    if (!readInputReportControl(raw, timeoutMs))
    {
        return false;
    }
    return decodeInput(raw, out);
}

bool Device::writeRgb(std::uint8_t red, std::uint8_t green, std::uint8_t blue, unsigned int timeoutMs)
{
    if (!isOpen() || _epOut == 0)
    {
        _setError(Error::NotConnected);
        return false;
    }
    unsigned char buf[ReportSize + HidOverrunSlop] = {};
    buf[0]                                          = red;
    buf[1]                                          = green;
    buf[2]                                          = blue;
    int       transferred = 0;
    const int rc = libusb_interrupt_transfer(_handle, _epOut, buf, static_cast<int>(ReportSize),
                                             &transferred, timeoutMs);
    if (rc != 0)
    {
        if (rc == LIBUSB_ERROR_TIMEOUT)
        {
            _setError(Error::Timeout, rc);
        }
        else
        {
            _setError(Error::UsbTransfer, rc);
        }
        return false;
    }
    clearLastError();
    return true;
}

} // namespace hid
} // namespace esplora
