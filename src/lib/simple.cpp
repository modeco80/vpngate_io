#include <vpngate_io/dat_file.hpp>
#include <vpngate_io/easycrypt.hpp>
#include <vpngate_io/simple.hpp>

#include "../file.hpp"

namespace vpngate_io {

	Simple::Simple(std::string_view filename)
		: filename(filename) {
	}

	SimpleErrc Simple::Init() {
		std::uint8_t rc4_key[0x14] {};

		auto file = File::Open(filename.c_str(), O_RDONLY);

		dataSize = file.Size() - 0x104;

		auto encryptedBuffer = std::make_unique<std::uint8_t[]>(dataSize);

		if(file.ReadLine() != "[VPNGate Data File]")
			return SimpleErrc::InvalidDat;

        // Read the identifier.
        identifier = file.ReadLine();

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
			return SimpleErrc::InvalidDat;

		reader.emplace(data.get(), dataSize);

		// No error
		return SimpleErrc::Ok;
	}

	PackReader& Simple::PackReader() {
		return reader.value();
	}

    const std::string& Simple::GetIdentifier() const {
        return identifier;
    }
} // namespace vpngate_io