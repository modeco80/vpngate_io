#include <openssl/core_names.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/provider.h>

#include <cstdio>
#include <vpngate_io/easycrypt.hpp>

// TODO: Rewrite this to use OpenSSL EVP
// so that we don't hit warnings here.

namespace vpngate_io {

	/// Openssl EVP_MD_CTX wrapper
	struct DigestCtx {
		DigestCtx() {
			context = EVP_MD_CTX_new();
		}

		DigestCtx(const DigestCtx&) = delete;
		DigestCtx(DigestCtx&&) = delete;

		int Init(const EVP_MD* algo) {
			return EVP_DigestInit_ex(context, algo, nullptr);
		}

		int Update(const void* buffer, std::size_t length) {
			return EVP_DigestUpdate(context, buffer, length);
		}

		int Final(void* buffer) {
			auto l = std::uint32_t {};
			return EVP_DigestFinal_ex(context, static_cast<std::uint8_t*>(buffer), &l);
		}

		~DigestCtx() {
			if(context) {
				EVP_MD_CTX_free(context);
			}
		}

	   private:
		EVP_MD_CTX* context;
	};

	struct RC4Ctx {
		RC4Ctx() {
			context = EVP_CIPHER_CTX_new();
			legacy = nullptr;
			cipher = nullptr;
		}

		RC4Ctx(const RC4Ctx&) = delete;
		RC4Ctx(RC4Ctx&&) = delete;

		~RC4Ctx() {
			if(context) {
				EVP_CIPHER_CTX_free(context);
			}

			if(cipher) {
				EVP_CIPHER_free(cipher);
				cipher = nullptr;
			}

			if(legacy) {
				OSSL_PROVIDER_unload(legacy);
			}
		}

		int Init(const std::uint8_t* key, std::uint32_t keylen) {
			OSSL_PARAM params[2];
			params[0] = OSSL_PARAM_construct_uint(OSSL_CIPHER_PARAM_KEYLEN, &keylen);
			params[1] = OSSL_PARAM_construct_end();

			// Attempt to load the legacy OpenSSL provider.
			// We need this since OpenSSL (probably rightfully) considers
			// RC4 to be insecure. It is, but hey.
			legacy = OSSL_PROVIDER_load(NULL, "legacy");
			if(legacy == nullptr) {
				return 0;
			}

			// Grab the RC4 cipher from the legacy provider.
			cipher = EVP_CIPHER_fetch(nullptr, "RC4", "provider=legacy");

			// The OpenSSL EVP API is.. Something to behold, let's put it like that.
			// It's extremely poorly documented how to deal with variable key length with ciphers
			// which support it, and there's even multiple differing methods for it.
			//
			// This is the method I found which actually seems to work and isn't *entirely* offputting.
			// Unlike most of the EVP API to be brutally honest.
			//
			// Essentially, we have to initalize the context once with just the EVP_CIPHER* and params (no key),
			// and then reinitalize with just the key. Truly mind boggling.

			if(auto r = EVP_EncryptInit_ex2(context, cipher, nullptr, nullptr, &params[0]); r != 1)
				return r;

			return EVP_EncryptInit_ex2(context, nullptr, key, nullptr, nullptr);
		}

		int Crypt(const std::uint8_t* buffer, std::size_t length, std::uint8_t* outBuffer) {
			int outlen = length;

			if(auto r = EVP_EncryptUpdate(context, outBuffer, &outlen, &buffer[0], length); r != 1)
				return r;

			int outlen2 = length - outlen;
			if(auto r = EVP_EncryptFinal_ex(context, &outBuffer[outlen], &outlen2); r != 1)
				return r;

			return 1;
		}

	   private:
		EVP_CIPHER_CTX* context;
		OSSL_PROVIDER* legacy;
		EVP_CIPHER* cipher;
	};

	void OpenSSLPrintErrors() {
		// clang-format off
		ERR_print_errors_cb([](const char* p, std::size_t len, void*) {
			fprintf(stderr, "OpenSSL error: %.*s\n", static_cast<int>(len), p);
			return 0;
		}, nullptr);
		// clang-format on
	}

	bool KeySha1(std::uint8_t const* pKeySrc, std::uint8_t* pKeyDst) {
		DigestCtx ctx;
		if(auto init = ctx.Init(EVP_sha1()); init != 1) {
			OpenSSLPrintErrors();
			return false;
		}

		if(auto res = ctx.Update(&pKeySrc[0], 0x14); res != 1) {
			OpenSSLPrintErrors();
			return false;
		}

		if(auto res = ctx.Final(&pKeyDst[0]); res != 1) {
			OpenSSLPrintErrors();
			return false;
		}

		return true;
	}

	std::unique_ptr<std::uint8_t[]> EasyDecrypt(std::uint8_t* key, std::uint8_t* buffer, std::size_t bufferSize) {
		auto pBuffer = std::make_unique<std::uint8_t[]>(bufferSize);
		std::uint8_t hashedRc4Key[0x14] {};
		RC4Ctx rc4;

		// SHA1 the key from the file to get the key we should use
		// to schedule RC4
		if(!KeySha1(&key[0], &hashedRc4Key[0])) {
			return nullptr;
		}

		// Now do RC4 (de|en)cryption with the key.

		if(auto init = rc4.Init(&hashedRc4Key[0], 0x14); init != 1) {
			OpenSSLPrintErrors();
			return nullptr;
		}

		if(auto res = rc4.Crypt(&buffer[0], bufferSize, &pBuffer[0]); res != 1) {
			OpenSSLPrintErrors();
			return nullptr;
		}

		return pBuffer;
	}

} // namespace vpngate_io