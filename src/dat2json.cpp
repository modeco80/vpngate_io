// Tool for converting VPNGate.dat to JSON.
//
// (C) 2025 Lily Tsuru <lily.modeco80@protonmail.ch>
// SPDX-License-Identifier: MIT

#include <boost/json.hpp>
#include <boost/json/serialize.hpp>
#include <vpngate_io/dat_file.hpp>
#include <vpngate_io/easycrypt.hpp>
#include <vpngate_io/pack_reader.hpp>

#include "file.hpp"

namespace json = boost::json;

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

	auto length = packReader.Get<vpngate_io::ValueType::Int64>("ID").size();

	// These tables are mapped back to an associcative ordering in the following code

	auto idTable = packReader.Get<vpngate_io::ValueType::Int64>("ID");
	auto nameTable = packReader.Get<vpngate_io::ValueType::String>("Name");
	auto ownerTable = packReader.Get<vpngate_io::ValueType::WString>("Owner");
	auto messageTable = packReader.Get<vpngate_io::ValueType::WString>("Message");

	auto ipTable = packReader.Get<vpngate_io::ValueType::String>("IP");
	auto hostnameTable = packReader.Get<vpngate_io::ValueType::String>("HostName");
	auto fqdnTable = packReader.Get<vpngate_io::ValueType::String>("Fqdn");
	auto countryTable = packReader.Get<vpngate_io::ValueType::String>("CountryShort");

	json::object root = {
		{ "version", 1 }
	};

	auto& array = root["entries"].emplace_array();

	for(auto i = 0; i < length; ++i) {
		auto& id = idTable[i];
		auto& name = nameTable[i];
		auto& owner = ownerTable[i];
		auto& message = messageTable[i];
		auto& ip = ipTable[i];
		auto& hostname = hostnameTable[i];
		auto& fqdn = fqdnTable[i];
		auto& country = countryTable[i];

		json::object object = {
			{ "id", id },
			{ "name", name },
			{ "owner", owner },
			{ "message", message },
			{ "ip", ip },
			{ "hostname", hostname },
			{ "fqdn", fqdn },
			{ "country", country }
		};

		array.push_back(object);
	}

	auto out = json::serialize(json::value(root));

	write(1, &out[0], out.length());
	return 0;
}
