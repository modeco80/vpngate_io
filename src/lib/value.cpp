#include <vpngate_io/value_types.hpp>

#include "bytemuck.hpp"

namespace vpngate_io {
	std::string_view ValueTypeToString(ValueType t) {
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

	Value Value::FromRaw(ValueType type, std::uint8_t* pBuffer, std::size_t size) {
		using enum ValueType;

		Value valueCreate {
			.type = type
		};

		switch(type) {
			case Int:
				valueCreate.intValue = impl::BESwap(*reinterpret_cast<std::uint32_t*>(pBuffer));
				break;
			case Data:
				valueCreate.dataValue = { pBuffer, size };
				break;
			case String: {
				if(size == 0) {
					valueCreate.stringValue = "";
				} else {
					valueCreate.stringValue = std::string_view { reinterpret_cast<const char*>(pBuffer), size };
				}
			} break;
			case WString: {
				if(size == 0) {
					valueCreate.wstringValue = "";
				} else {
					valueCreate.wstringValue = std::string_view { reinterpret_cast<const char*>(pBuffer), size };
				}
			} break;
			case Int64:
				valueCreate.int64Value = impl::BESwap(*reinterpret_cast<std::uint64_t*>(pBuffer));
				break;
		}

        return valueCreate;
	}
} // namespace vpngate_io