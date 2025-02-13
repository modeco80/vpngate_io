// Code for decompressing VPNGate.dat
// from VPNGate to its final data component.
//
// Compile: g++ -std=c++23 -O3 test.cpp -lcrypto -lz -o vpngate_decompress
// Usage: ./vpngate_decompress /path/to/VPNGate.dat > decompressed.bin
//
// (C) 2025 Lily Tsuru <lily.modeco80@protonmail.ch>
// SPDX-License-Identifier: MIT

#include <zlib.h>

#include "file.hpp"

#include <vpngate_io/pack_reader.hpp>
#include <vpngate_io/easycrypt.hpp>

std::unique_ptr<std::uint8_t[]> DecryptData(File& file) {
	std::uint8_t rc4_key[0x14]{};
	auto dataSize = file.Size() - 0x104;

	auto encryptedBuffer = std::make_unique<std::uint8_t[]>(dataSize);

	// We skip the weird header thing and go straight to the
	// RC4 key.
	file.Seek(0xf0, 0);

	// Read key and buffer
	file.Read(&rc4_key[0], sizeof(rc4_key));
	file.Read(&encryptedBuffer[0], dataSize);

	return vpngate_io::EasyDecrypt(&rc4_key[0], &encryptedBuffer[0], dataSize);
}

std::unique_ptr<std::uint8_t[]> GetPackData(vpngate_io::PackReader& pack, std::size_t& outSize) {

	// Handle the data being packed; this seems to be the default
	// now, but we also can safely handle the data NOT being packed

	std::size_t dataSize = 0;
	bool dataCompressed = false;

	if(auto c = pack.GetFirst<vpngate_io::ValueType::Int>("compressed"); c.has_value()) {
		auto compressed = c.value();

		if(compressed == 1) {
			dataCompressed = true;
			dataSize = pack.GetFirst<vpngate_io::ValueType::Int>("data_size").value(); 
		} else {
			dataSize = pack.GetFirst<vpngate_io::ValueType::Data>("data").value().size();
		}
	}

	auto dataUnpackBuffer = std::make_unique<std::uint8_t[]>(dataSize);
	auto dataSource = pack.GetFirst<vpngate_io::ValueType::Data>("data").value();

	if(dataCompressed) {
		std::size_t size = dataSize;
		auto res = uncompress(dataUnpackBuffer.get(), &size, &dataSource[0], dataSource.size());

		if(res != Z_OK) {
			// TODO: throw here
			return nullptr;
		}
	} else {
		// Just memcpy() it
		memcpy(&dataUnpackBuffer[0], &dataSource[0], dataSource.size());
	}

	outSize = dataSize;
	return dataUnpackBuffer;
}

int main(int argc, char** argv) {

	auto dat = File::Open(argv[1], O_RDONLY);

	auto decryptedSize = dat.Size() - 0x104;
	auto decryptedData = DecryptData(dat);


	//write(1, &decryptedData[0], decryptedSize);

	std::size_t dataSize{};

	vpngate_io::PackReader pack(decryptedData.get(), decryptedSize);
	auto dataUnpackBuffer = GetPackData(pack, dataSize);


	vpngate_io::PackReader pack2(dataUnpackBuffer.get(), dataSize);

	for(auto& key: pack2.Keys()) {
		std::printf("key \"%s\" type %s\n", key.key.data(), vpngate_io::ValueTypeToString(key.elementType).data());
	}

/*
	for(auto& str: pack2.Get<vpngate_io::ValueType::String>("CountryFull")) {
		std::printf("lets go: %s\n", str.data());
	}
	*/

	// Write data to stdout.
	//write(1, &dataUnpackBuffer[0], dataSize);
	return 0;
}
