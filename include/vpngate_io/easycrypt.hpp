#pragma once

#include <memory>
#include <cstdint>

namespace vpngate_io {

    /// Does the decrypt operation.
    std::unique_ptr<std::uint8_t[]> EasyDecrypt(std::uint8_t* key, std::uint8_t* buffer, std::size_t bufferSize);

}