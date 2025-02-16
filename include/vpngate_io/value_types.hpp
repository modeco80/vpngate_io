//! value_types.hpp: Types for Pack values
#pragma once

#include <cstdint>
#include <span>
#include <exception>
#include <string_view>

namespace vpngate_io {

	enum class ValueType : std::uint32_t {
		Int = 0,
		Data = 1,
		String = 2,
		WString = 3,
		Int64 = 4
	};

	std::string_view ValueTypeToString(ValueType t);

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

	struct InvalidValueCast : std::exception {
		const char* what() const noexcept override { return "invalid cast of pack value to invalid type"; }
	};

	/// A Pack "value". Holds a single element of varying type.
	/// Not very safe to use really :(
	struct Value {
		ValueType type;

		union {
			std::uint32_t intValue;
			std::span<std::uint8_t> dataValue;
			std::string_view stringValue;
			std::string_view wstringValue;
			std::uint64_t int64Value;
		};

		/// Casts this value to a native C++ type.
		template <ValueType Expected>
		auto Cast() -> typename ValueTypeToNaturalType<Expected>::Type {
			if(type != Expected) [[unlikely]]
				throw InvalidValueCast();

			if constexpr(Expected == ValueType::Int) {
				return this->intValue;
			}

			if constexpr(Expected == ValueType::Data) {
				return this->dataValue;
			}

			if constexpr(Expected == ValueType::String) {
				return this->stringValue;
			}

			if constexpr(Expected == ValueType::WString) {
				return this->wstringValue;
			}

			if constexpr(Expected == ValueType::Int64) {
				return this->int64Value;
			}
		}
	};

} // namespace vpngate_io
