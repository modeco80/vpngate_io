#include <vpngate_io/easycrypt.hpp>
#include <openssl/rc4.h>
#include <openssl/sha.h>

// TODO: Rewrite this to use OpenSSL EVP
// so that we don't hit warnings here.

namespace vpngate_io {

	std::unique_ptr<std::uint8_t[]> EasyDecrypt(std::uint8_t* key, std::uint8_t* buffer, std::size_t bufferSize) {
		auto pBuffer = std::make_unique<std::uint8_t[]>(bufferSize);
        RC4_KEY rc4Key{};

	    std::uint8_t rc4_key_sha1[0x14]{};

		// SHA1 the key from the file to get the key we should use
		// to schedule RC4
		SHA1(&key[0], sizeof(rc4_key_sha1), &rc4_key_sha1[0]);

		// Init/schedule the RC4 key
		RC4_set_key(&rc4Key, sizeof(rc4_key_sha1), &rc4_key_sha1[0]);

		// Decrypt the data
		RC4(&rc4Key, bufferSize, &buffer[0], &pBuffer[0]);

		return pBuffer;
	}

} // namespace vpngate_io