// Code for decompressing VPNGate.dat
// from VPNGate to its final data component.
//
// Compile: g++ -std=c++23 -O3 test.cpp -lcrypto -lz -o vpngate_decompress
// Usage: ./vpngate_decompress /path/to/VPNGate.dat > decompressed.bin
//
// (C) 2025 Lily Tsuru <lily.modeco80@protonmail.ch>
// SPDX-License-Identifier: MIT

#include <zlib.h>

#include <vpngate_io/dat_file.hpp>
#include <vpngate_io/easycrypt.hpp>
#include <vpngate_io/pack_reader.hpp>

#include "file.hpp"

std::unique_ptr<std::uint8_t[]> GetDecryptedFileData(File& file) {
	std::uint8_t rc4_key[0x14] {};
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

int main(int argc, char** argv) {
	auto dat = File::Open(argv[1], O_RDONLY);

	auto decryptedSize = dat.Size() - 0x104;
	auto decryptedData = GetDecryptedFileData(dat);
	vpngate_io::PackReader innerPackReader(decryptedData.get(), decryptedSize);

	std::size_t dataSize {};

	auto datPackedDataBuffer = vpngate_io::GetDATPackData(innerPackReader, dataSize);

	vpngate_io::PackReader packReader(datPackedDataBuffer.get(), dataSize);

	for(auto& key : packReader.Keys()) {
		std::printf("key \"%s\" type %s\n", key.key.data(), vpngate_io::ValueTypeToString(key.elementType).data());
	}

	/*
		for(auto& str: pack2.Get<vpngate_io::ValueType::String>("CountryFull")) {
			std::printf("lets go: %s\n", str.data());
		}
		*/

	// Write data to stdout.
	// write(1, &dataUnpackBuffer[0], dataSize);
	return 0;
}
