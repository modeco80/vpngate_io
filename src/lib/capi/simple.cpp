#include <memory>
#include <vpngate_io/simple.hpp>

// C API
#include <fcntl.h>
#include <vpngate_io/capi/error.h>
#include <vpngate_io/capi/simple.h>

#include "vpngate_io/capi/pack_reader.h"

vpngate_io_Simple* vpngate_io_simple_new(const char* filename) {
	auto ptr = new(std::nothrow) vpngate_io::Simple(filename);

	return reinterpret_cast<vpngate_io_Simple*>(ptr);
}

int vpngate_io_simple_init(vpngate_io_Simple* simple) {
	if(simple == nullptr) [[unlikely]]
		return VPNGATE_IO_ERRC_INVALID_ARGUMENT;

	try {
		auto rc = reinterpret_cast<vpngate_io::Simple*>(simple)->Init();

		using enum vpngate_io::SimpleErrc;
		switch(rc) {
			case Ok: return VPNGATE_IO_ERRC_OK;
			case InvalidDat: return VPNGATE_IO_ERRC_INVALID_FILE;
		}

		return VPNGATE_IO_ERRC_OK;
	} catch(std::exception& ex) {
		// TODO: Catch exceptions and add errc's for them
		return VPNGATE_IO_ERRC_INVALID_ARGUMENT;
	}
}

vpngate_io_PackReader* vpngate_io_simple_get_pack_reader(vpngate_io_Simple* simple) {
	if(simple == nullptr)
		return nullptr;

	auto& packReader = reinterpret_cast<vpngate_io::Simple*>(simple)->PackReader();
	return reinterpret_cast<vpngate_io_PackReader*>(&packReader);
}

int vpngate_io_simple_get_identifier(vpngate_io_Simple* simple, char const** identifier) {
	if(simple == nullptr)
		return VPNGATE_IO_ERRC_INVALID_ARGUMENT;

	if(identifier == nullptr)
		return VPNGATE_IO_ERRC_INVALID_ARGUMENT;

	auto pSimple = reinterpret_cast<vpngate_io::Simple*>(simple);

	const auto& id = pSimple->GetIdentifier();

	*identifier = id.data();
	return VPNGATE_IO_ERRC_OK;
}

void vpngate_io_simple_free(vpngate_io_Simple* simple) {
	if(simple != nullptr) {
		delete reinterpret_cast<vpngate_io::Simple*>(simple);
	}
}