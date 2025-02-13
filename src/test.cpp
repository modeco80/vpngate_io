// Code for decompressing VPNGate.dat
// from VPNGate to its final data component.
//
// Compile: g++ -std=c++23 -O3 test.cpp -lcrypto -lz -o vpngate_decompress
// Usage: ./vpngate_decompress /path/to/VPNGate.dat > decompressed.bin
//
// (C) 2025 Lily Tsuru <lily.modeco80@protonmail.ch>
// SPDX-License-Identifier: MIT

#include <zlib.h>

// io shit
#include <fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <system_error>


#include <vpngate_io/pack_reader.hpp>
#include <vpngate_io/easycrypt.hpp>

// custom rolled file thing because I hate iostreams dearly
// (seriously, it sucks)
struct File {
	static File Open(const char* path, int mode) {
		if(auto fd = open(path, mode | O_CLOEXEC); fd != -1) {
			return File(fd);
		} else {
			// errno is mappable to system_category
			throw std::system_error{errno, std::generic_category()};
		}
	}

	// use clone() to clone
	File(const File&) = delete;

	File(File&& m) {
		// move ownership of fd to us
		fd = m.fd;
		m.fd = -1;
	}

	~File() {
		Close();
	}

	std::uint64_t Read(void* buffer, std::size_t length) {
		return read(fd, buffer, length);
	}

	std::uint64_t Write(const void* buffer, std::size_t length) {
		return write(fd, buffer, length);	
	}

	std::uint64_t Seek(std::uint64_t offset, int whence) {
		return lseek64(fd, offset, whence);
	}

	std::uint64_t Tell() {
		return Seek(0, SEEK_CUR);
	}

	/// Close this file, if it hasn't been closed
	/// or otherwise invalidated already
	void Close() {
		if(fd != -1) {
			close(fd);
			fd = -1;
		}
	}

	std::uint64_t Size() {
		return size;
	}	

private:
	File(int fd) 
		: fd(fd) {

		// Cache size.
		Seek(0, SEEK_END);
		size = Tell();
		Seek(0, SEEK_SET);
	}

	int fd;
	std::uint64_t size;
};

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
