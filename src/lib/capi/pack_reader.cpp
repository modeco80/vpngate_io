#include <vpngate_io/capi/error.h>
#include <vpngate_io/capi/pack_reader.h>

#include <vpngate_io/pack_reader.hpp>

namespace {
	/// Helper function used to implement vpngate_io_pack_reader_get()
	int PackReaderGetHelper(vpngate_io::PackReader* pack, std::string_view key, vpngate_io::ValueType expected, vpngate_io_value* value) {
		auto values = pack->GetValues(key, expected);

		if(!values.has_value())
			return VPNGATE_IO_ERRC_KEY_DOES_NOT_EXIST;

		using vpngate_io::ValueType;

		// Convert the C++ value to its capi equlivant value
		for(auto& raw : values.value()) {
			*value = {
				.type = static_cast<vpngate_io_value_type>(expected),
			};

			if(raw.type == ValueType::Int) {
				value->intValue = raw.intValue;
			}

			if(raw.type == ValueType::Data) {
				value->dataValue = {
					.ptr = raw.dataValue.data(),
					.len = raw.dataValue.size()
				};
			}

			if(raw.type == ValueType::String) {
				value->stringValue = {
					.ptr = raw.stringValue.data(),
					.len = raw.stringValue.size()
				};
			}

			if(raw.type == ValueType::WString) {
				value->wstringValue = {
					.ptr = raw.wstringValue.data(),
					.len = raw.wstringValue.size()
				};
			}

			if(raw.type == ValueType::Int64) {
				value->int64Value = raw.int64Value;
			}

			value++;
		}

		return VPNGATE_IO_ERRC_OK;
	}
} // namespace

extern "C" {

vpngate_io_PackReader* vpngate_io_pack_reader_new(uint8_t* pBuffer, size_t dataSize) {
	auto* ptr = new(std::nothrow) vpngate_io::PackReader(pBuffer, dataSize);
	return reinterpret_cast<vpngate_io_PackReader*>(ptr);
}

int vpngate_io_pack_reader_value_type(vpngate_io_PackReader* reader, const char* key, vpngate_io_value_type* pOutType) {
	if(reader) {
		auto* pReader = reinterpret_cast<vpngate_io::PackReader*>(reader);

		if(pOutType == nullptr)
			return VPNGATE_IO_ERRC_INVALID_ARGUMENT;

		if(key == nullptr)
			return VPNGATE_IO_ERRC_INVALID_ARGUMENT;

		auto keyView = std::string_view(key);

		try {
			auto keyView = std::string_view(key);

			if(!pReader->KeyExists(keyView))
				return VPNGATE_IO_ERRC_KEY_DOES_NOT_EXIST;

			auto keys = pReader->Keys();

			for(auto& key : keys) {
				if(key.key == keyView) {
					*pOutType = static_cast<vpngate_io_value_type>(key.elementType);
					return VPNGATE_IO_ERRC_OK;
				}
			}
		} catch(...) {
			return VPNGATE_IO_ERRC_INVALID_ARGUMENT;
		}
	}

	return VPNGATE_IO_ERRC_INVALID_ARGUMENT;
}

int vpngate_io_pack_reader_length(vpngate_io_PackReader* pack, const char* key, size_t* outLen) {
	if(pack) {
		auto* pReader = reinterpret_cast<vpngate_io::PackReader*>(pack);

		// what, did you think it would telepathically show up?
		// Pass a valid pointer!
		if(outLen == nullptr)
			return VPNGATE_IO_ERRC_INVALID_ARGUMENT;

		// Alright come on now.
		if(key == nullptr)
			return VPNGATE_IO_ERRC_INVALID_ARGUMENT;

		try {
			auto keyView = std::string_view(key);

			if(!pReader->KeyExists(keyView))
				return VPNGATE_IO_ERRC_KEY_DOES_NOT_EXIST;

			*outLen = pReader->ValueCount(keyView).value_or(-1);

			return VPNGATE_IO_ERRC_OK;
		} catch(...) {
			return VPNGATE_IO_ERRC_INVALID_ARGUMENT;
		}

	} else {
		return VPNGATE_IO_ERRC_INVALID_ARGUMENT;
	}
}

int vpngate_io_pack_reader_get(vpngate_io_PackReader* reader, const char* key, vpngate_io_value* pValues, vpngate_io_value_type valueType) {
	if(reader) {
		auto* pReader = reinterpret_cast<vpngate_io::PackReader*>(reader);

		if(pValues == nullptr)
			return VPNGATE_IO_ERRC_INVALID_ARGUMENT;

		if(key == nullptr)
			return VPNGATE_IO_ERRC_INVALID_ARGUMENT;

		auto keyView = std::string_view(key);

		try {
			vpngate_io_value_type type {};
			if(auto r = vpngate_io_pack_reader_value_type(reader, key, &type); r != VPNGATE_IO_ERRC_OK)
				return r;

			if(type != valueType)
				return VPNGATE_IO_ERRC_TYPE_MISMATCH;

			return PackReaderGetHelper(pReader, keyView, static_cast<vpngate_io::ValueType>(type), pValues);
		} catch(...) {
			return VPNGATE_IO_ERRC_INVALID_ARGUMENT;
		}
	}

	return VPNGATE_IO_ERRC_INVALID_ARGUMENT;
}

void vpngate_io_pack_reader_free(vpngate_io_PackReader* reader) {
	if(reader) {
		delete reinterpret_cast<vpngate_io::PackReader*>(reader);
	}
}
};