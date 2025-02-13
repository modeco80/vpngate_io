#pragma once
#include <cstdint>
#include <memory>
#include <vpngate_io/pack_reader.hpp>

namespace vpngate_io {

    /// Gets the data, packed in a Pack, serialized in a vpngate .dat file.
	std::unique_ptr<std::uint8_t[]> GetDATPackData(vpngate_io::PackReader& pack, std::size_t& outSize);

} // namespace vpngate_io