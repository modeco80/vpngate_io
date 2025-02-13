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
namespace vg = vpngate_io;

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

	return vg::EasyDecrypt(&rc4_key[0], &encryptedBuffer[0], dataSize);
}

void help(char* progname) {
	// clang-format off
	printf(
	"VPNGate .dat to JSON utility\n"
			"Usage: %s [path to VPNGate .dat file]\n",
			progname
	);
	// clang-format on
}

int main(int argc, char** argv) {
	if(argc != 2) {
		help(argv[0]);
		return 0;
	}

	if(std::string_view(argv[1]) == "--help") {
		help(argv[0]);
		return 0;
	}

	auto dat = File::Open(argv[1], O_RDONLY);

	auto decryptedSize = dat.Size() - 0x104;
	auto decryptedData = GetDecryptedFileData(dat);
	vg::PackReader innerPackReader(decryptedData.get(), decryptedSize);

	std::size_t dataSize {};

	auto datPackedDataBuffer = vg::GetDATPackData(innerPackReader, dataSize);

	vg::PackReader packReader(datPackedDataBuffer.get(), dataSize);

	// We can pick any table here to query the length of the keys we want to associatively map, 
	// but in this case, we just pick the ID table. Why not.
	auto length = packReader.Get<vg::ValueType::Int64>("ID").size();

	// These tables are mapped back to an associcative ordering in the following code

	auto idTable = packReader.Get<vg::ValueType::Int64>("ID");
	auto nameTable = packReader.Get<vg::ValueType::String>("Name");
	auto ownerTable = packReader.Get<vg::ValueType::WString>("Owner");
	auto messageTable = packReader.Get<vg::ValueType::WString>("Message");

	auto ipTable = packReader.Get<vg::ValueType::String>("IP");
	auto hostnameTable = packReader.Get<vg::ValueType::String>("HostName");
	auto fqdnTable = packReader.Get<vg::ValueType::String>("Fqdn");
	auto countryTable = packReader.Get<vg::ValueType::String>("CountryShort");

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
