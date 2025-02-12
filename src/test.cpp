// Code for decompressing VPNGate.dat
// from VPNGate to its final data component.
//
// Compile: g++ -std=c++23 -O3 test.cpp -lcrypto -lz -o vpngate_decompress
// Usage: ./vpngate_decompress /path/to/VPNGate.dat > decompressed.bin
//
// (C) 2025 Lily Tsuru <lily.modeco80@protonmail.ch>
// SPDX-License-Identifier: MIT

#include <openssl/rc4.h>
#include <openssl/sha.h>

#include <zlib.h>

// io shit
#include <fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <system_error>
#include <memory>
#include <span>
#include <optional>
#include <vector>

// needs c++23!!!
#include <bit>

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


enum class ValueType : std::uint32_t {
	Int = 0,
	Data = 1,
	String = 2,
	WString = 3,
	Int64 = 4
};

constexpr std::string_view ValueTypeToString(ValueType t) {
	constexpr static std::string_view table[] = {
		"ValueType::Int",
		"ValueType::Data",
		"ValueType::String",
		"ValueType::WString",
		"ValueType::Int64"
	};

	return table[static_cast<std::uint64_t>(t)];
}

template<class T>
constexpr T Swap(const T value) {
	if constexpr(std::endian::native == std::endian::little)
		return std::byteswap(value);
	return value;
}

template<ValueType t>
struct ValueTypeToNaturalType {};

template<>
struct ValueTypeToNaturalType<ValueType::Int> {
	using Type = std::uint32_t;
};

template<>
struct ValueTypeToNaturalType<ValueType::Data> {
	using Type = std::span<std::uint8_t>;
};

template<>
struct ValueTypeToNaturalType<ValueType::String> {
	using Type = std::string_view;
};

template<>
struct ValueTypeToNaturalType<ValueType::WString> {
	using Type = std::string_view;
};

template<>
struct ValueTypeToNaturalType<ValueType::Int64> {
	using Type = std::uint64_t;
};



/// Reader for SoftEther Mayaqua "Pack" files
///
/// # Notes
/// This currently only handles enough required to work with
/// the VPNGate.dat file. It should also be refactored to maybe
/// use a byte stream, but for now, this works.
struct PackReader {
	PackReader(std::uint8_t* buffer, std::size_t size)
		: buffer(buffer), size(size) {
	}

	template<class F>
	void WalkAll(F&& f) {
		auto* bufptr = buffer;

		// Swap elements
		auto nrElements = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
		bufptr += 4;

		for(auto i = 0; i < nrElements; ++i) {
			auto elementNameLength = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
			bufptr += 4;

			auto elementName = std::string_view(reinterpret_cast<const char*>(bufptr), elementNameLength - 1);
			bufptr += elementNameLength - 1;

			auto elementType = static_cast<ValueType>(Swap(*reinterpret_cast<std::uint32_t*>(bufptr)));
			bufptr += 4;

			auto elementNumValues = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
			bufptr += 4;

			// Walk all elements
			for(auto j = 0; i < elementNumValues; ++j) {
				switch(elementType) {
					case ValueType::Int: {
						f(elementType, elementName, 4, bufptr);
						bufptr += 4;
					} break;

					case ValueType::Data: {
						auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
						f(elementType, elementName, dataSize, bufptr + 4); 

						// Walk next value
						bufptr += 4 + dataSize;
					} break;

					case ValueType::String: {
						auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
						f(elementType, elementName, dataSize - 1, bufptr + 4); 

						// Walk next value
						bufptr += 4 + (dataSize - 1);

					} break;
				 
					default:
						break;
				}
			}
	
		}
	}

	/// Returns `true` if the specified key exists
	template<ValueType Type>
	bool KeyExists(std::string_view key) {
		bool ret = false;

		// Walk the keys; if the key exists AND has the type the user has specified, update ret to true
		WalkAll([&](ValueType type, std::string_view name, std::size_t size, std::uint8_t* buffer) {
			// Skip values
			if(ret)
				return; 

			if(name == key && type == Type) {
				ret = true;
			}
		});
		return ret;
	}


	template<ValueType Type>
	auto Get(std::string_view key) -> std::vector<typename ValueTypeToNaturalType<Type>::Type> {
		using T = typename ValueTypeToNaturalType<Type>::Type;
		std::vector<T> ret;

		WalkAll([&](ValueType type, std::string_view name, std::size_t size, std::uint8_t* buffer) {
			if(name == key && type == Type) {
				// Can't really do this in a better way I don't think
				// These will compile out anyways depending on the type so
				// I don't think it's worth bothering

				if constexpr(Type == ValueType::Int) {
					auto swapped = Swap(*reinterpret_cast<std::uint32_t*>(buffer));
					ret.push_back(swapped);
				}

				if constexpr(Type == ValueType::Data) {
					ret.push_back({ buffer, size });
				}

				if constexpr(Type == ValueType::String) {
					if(size == 0) {
						ret.push_back("");
					} else {
						ret.push_back({ buffer, size });
					}
				}

			}
		});
	
		return ret;
	}

	template<ValueType Type>
	auto GetFirst(std::string_view key) -> std::optional<typename ValueTypeToNaturalType<Type>::Type> {
		auto values = Get<Type>(key);
		if(values.empty())
			return std::nullopt;
		return values[0];
	}
	

private:
	std::uint8_t* buffer;
	std::size_t size;
};

int main(int argc, char** argv) {

	auto dat = File::Open(argv[1], O_RDONLY);

	// Key shit
	std::uint8_t rc4_key[0x14]{};
	std::uint8_t rc4_key_sha1[0x14]{};


	auto size = dat.Size();
	auto encrypted_size = size - 0x104;

	auto pBuffer = std::make_unique<std::uint8_t[]>(encrypted_size);

	RC4_KEY rc4Key{};

	// We skip the weird header thing and go straight to the
	// RC4 key.
	dat.Seek(0xf0, 0);

	// Read key and buffer
	dat.Read(&rc4_key[0], sizeof(rc4_key));
	dat.Read(&pBuffer[0], encrypted_size);

	// SHA1 the key from the file to get the key we should use
	// to schedule RC4
	SHA1(&rc4_key[0], sizeof(rc4_key), &rc4_key_sha1[0]);

	// Init/schedule the RC4 key
	RC4_set_key(&rc4Key, sizeof(rc4_key), &rc4_key_sha1[0]);

	// Decrypt the data
	RC4(&rc4Key, encrypted_size, &pBuffer[0], &pBuffer[0]);

	PackReader pack(pBuffer.get(), encrypted_size);

	// Handle the data being packed; this seems to be the default
	// now, but we also can safely handle the data NOT being packed

	std::size_t dataSize = 0;
	bool dataCompressed = false;

	if(auto c = pack.GetFirst<ValueType::Int>("compressed"); c.has_value()) {
		auto compressed = c.value();

		if(compressed == 1) {
			dataCompressed = true;
			dataSize = pack.GetFirst<ValueType::Int>("data_size").value(); 
		} else {
			dataSize = pack.GetFirst<ValueType::Data>("data").value().size();
		}
	}

	auto dataUnpackBuffer = std::make_unique<std::uint8_t[]>(dataSize);
	auto dataSource = pack.GetFirst<ValueType::Data>("data").value();

	if(dataCompressed) {
		std::size_t size = dataSize;
		auto res = uncompress(dataUnpackBuffer.get(), &size, &dataSource[0], dataSource.size());

		if(res != Z_OK) {
			printf("Error decompressing!!!\n");
			return 1;
		}

		//printf("Decompressed les fucking go\n");
	} else {
		// Just memcpy() it
		std::fprintf(stderr, "not compressed\n");
		memcpy(&dataUnpackBuffer[0], &dataSource[0], dataSource.size());
	}

	// Write data to stdout.
	write(1, &dataUnpackBuffer[0], dataSize);
	return 0;
}
