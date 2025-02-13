//! value_types.hpp: Types for Pack values
#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace vpngate_io {

	enum class ValueType : std::uint32_t {
		Int = 0,
		Data = 1,
		String = 2,
		WString = 3,
		Int64 = 4
	};

	constexpr std::string_view ValueTypeToString(ValueType t) {
		constexpr static std::string_view table[] = {
			"ValueType::Int",
			"ValueType::Data",
			"ValueType::String",
			"ValueType::WString",
			"ValueType::Int64"
		};

		return table[static_cast<std::uint64_t>(t)];
	}

	/// A simple metafunction which returns a "natural"
	/// C++ type to repressent a Pack value.
	///
	/// Well, that's a lie; metafunctions are never simple.
	/// But this one should be *understandable* at least.
	template <ValueType t>
	struct ValueTypeToNaturalType {};

	template <>
	struct ValueTypeToNaturalType<ValueType::Int> {
		using Type = std::uint32_t;
	};

	template <>
	struct ValueTypeToNaturalType<ValueType::Data> {
		using Type = std::span<std::uint8_t>;
	};

	template <>
	struct ValueTypeToNaturalType<ValueType::String> {
		using Type = std::string_view;
	};

	template <>
	struct ValueTypeToNaturalType<ValueType::WString> {
		using Type = std::string_view;
	};

	template <>
	struct ValueTypeToNaturalType<ValueType::Int64> {
		using Type = std::uint64_t;
	};

} // namespace vpngate_io
