#pragma once

// Internal byte mucking goodness.

#include <bit>

namespace vpngate_io::impl {
	/// Swaps a big endian value on little endian machines.
	template <class T>
	constexpr T BESwap(const T value) {
		if constexpr(std::endian::native == std::endian::little)
			return std::byteswap(value);
		return value;
	}
} // namespace vpngate_io::impl