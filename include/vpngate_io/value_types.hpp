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
		using enum ValueType;
		// clang-format off
		switch(t) {
			case Int: return "int"; break;
			case Data: return "data"; break;
			case String: return "string"; break;
			case WString: return "wstring"; break;
			case Int64: return "int64"; break;
			default: return "unknown"; break;
		}
		// clang-format on
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


	struct Value {
		ValueType type;
		union {
			std::uint32_t intValue;
			std::span<std::uint8_t> dataValue;
			std::string_view stringValue;
			std::string_view wstringValue;
			std::uint64_t int64Value;
		};
	};

} // namespace vpngate_io
