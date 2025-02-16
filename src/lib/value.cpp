#include <vpngate_io/value_types.hpp>

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
} // namespace vpngate_io