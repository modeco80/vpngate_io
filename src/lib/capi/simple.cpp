#include <memory>
#include <vpngate_io/dat_file.hpp>
#include <vpngate_io/easycrypt.hpp>
#include <vpngate_io/pack_reader.hpp>

// C API
#include <fcntl.h>
#include <vpngate_io/capi/error.h>
#include <vpngate_io/capi/simple.h>

// Should probably put this in a less annoying place :p
#include "../../file.hpp"
#include "vpngate_io/capi/pack_reader.h"

// FIXME: This struct is implemented here mainly because there was no analog for it in the C++ API.
// However, implementing it in CAPI has kind of made me realise it might be better TO implement it
// in the C++ API and then just bind it, since it should help code reuse; most of the code here
// came from dat2json.

struct SimpleT {
	SimpleT(const char* filename)
		: filename(filename) {
	}

	int Init() {
		std::uint8_t rc4_key[0x14] {};

		auto file = File::Open(filename.c_str(), O_RDONLY);

		dataSize = file.Size() - 0x104;

		auto encryptedBuffer = std::make_unique<std::uint8_t[]>(dataSize);

		if(file.ReadLine() != "[VPNGate Data File]")
			return VPNGATE_IO_ERRC_INVALID_FILE;

		// We skip the weird header thing and go straight to the
		// RC4 key.
		file.Seek(0xf0, 0);

		// Read key and buffer
		file.Read(&rc4_key[0], sizeof(rc4_key));
		file.Read(&encryptedBuffer[0], dataSize);

		std::unique_ptr<std::uint8_t[]> decryptedData = vpngate_io::EasyDecrypt(&rc4_key[0], &encryptedBuffer[0], dataSize);
		vpngate_io::PackReader innerPackReader(decryptedData.get(), dataSize);

		// Get the inner pack data and then set up the pack reader.
		data = vpngate_io::GetDATPackData(innerPackReader, dataSize);
		if(data.get() == nullptr)
			return VPNGATE_IO_ERRC_INVALID_FILE;

		reader.emplace(data.get(), dataSize);

		// No error
		return VPNGATE_IO_ERRC_OK;
	}

	vpngate_io_PackReader* PackReader() {
        // User has probably not called vpngate_io_simple_init() yet. Bad user.
        // The user has clearly decided to not follow the documentation.
        // Maybe a null pointer will clear their head up a bit.
		if(reader.has_value() == false)
			return nullptr;

		return reinterpret_cast<vpngate_io_PackReader*>(&reader.value());
	}

   private:
	std::string filename;
	std::unique_ptr<std::uint8_t[]> data;
	std::size_t dataSize;
	std::optional<vpngate_io::PackReader> reader;
};

vpngate_io_Simple* vpngate_io_simple_new(const char* filename) {
	auto ptr = new(std::nothrow) SimpleT(filename);

	return reinterpret_cast<vpngate_io_Simple*>(ptr);
}

int vpngate_io_simple_init(vpngate_io_Simple* simple) {
	if(simple == nullptr) [[unlikely]]
		return VPNGATE_IO_ERRC_INVALID_ARGUMENT;

    try {
	    return reinterpret_cast<SimpleT*>(simple)->Init();
    } catch(std::exception& ex) {
        // TODO: Catch exceptions and add errc's for them
        return VPNGATE_IO_ERRC_INVALID_ARGUMENT;
    }
}

vpngate_io_PackReader* vpngate_io_simple_get_pack_reader(vpngate_io_Simple* simple) {
	if(simple == nullptr)
		return nullptr;

	return reinterpret_cast<SimpleT*>(simple)->PackReader();
}

void vpngate_io_simple_free(vpngate_io_Simple* simple) {
	if(simple != nullptr) {
		delete reinterpret_cast<SimpleT*>(simple);
	}
}