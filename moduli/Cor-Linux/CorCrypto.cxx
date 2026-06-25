#include <Cor/IO/CorCrypto.h>
#include <dlfcn.h>

namespace ESSE
{
	namespace Cryptography
	{
		namespace libcrypto
		{
			#define DEFINE_CRYPTO_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
			#define DEFINE_CRYPTO_FUNCTION_IMPORT(NAME) libcrypto::NAME = reinterpret_cast<libcrypto::func_##NAME>(dlsym(libcrypto::library, #NAME));

			bool initialized = false;
			handle library;

			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_aes_128_ecb, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_aes_128_cbc, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_aes_192_ecb, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_aes_192_cbc, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_aes_256_ecb, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_aes_256_cbc, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_md5, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_sha1, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_sha224, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_sha256, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_sha384, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_sha512, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_sha512_224, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_sha512_256, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_CIPHER_CTX_new, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_CIPHER_CTX_free, void, (handle))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_CIPHER_CTX_reset, int, (handle))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_EncryptInit_ex, int, (handle, handle, handle, const void *, const void *))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_EncryptUpdate, int, (handle, void *, int *, const void *, int))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_EncryptFinal_ex, int, (handle, void *, int *))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_DecryptInit_ex, int, (handle, handle, handle, const void *, const void *))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_DecryptUpdate, int, (handle, void *, int *, const void *, int))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_DecryptFinal_ex, int, (handle, void *, int *))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_MD_CTX_new, handle, (void))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_MD_CTX_free, void, (handle))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_MD_CTX_get_size_ex, int, (handle))
			DEFINE_CRYPTO_FUNCTION_POINTER(RAND_bytes, int, (unsigned char *, int))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_DigestInit_ex, int, (handle, handle, handle))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_DigestUpdate, int, (handle, const void *, uintptr))
			DEFINE_CRYPTO_FUNCTION_POINTER(EVP_DigestFinal_ex, int, (handle, uint8 *, uint *))
		}
		class CryptoKey : public Key
		{
			uint8 _key_data[32];
			uint _key_length;
			EncryptionAlgorithm _alg;
			EncryptionMode _mode;
			handle _cipher;
		public:
			CryptoKey(EncryptionAlgorithm alg, EncryptionMode mode, const void * key_data, uintptr key_size) : _alg(alg), _mode(mode)
			{
				if (_alg == EncryptionAlgorithm::AES128) _key_length = 16;
				else if (_alg == EncryptionAlgorithm::AES192) _key_length = 24;
				else if (_alg == EncryptionAlgorithm::AES256) _key_length = 32;
				else throw InvalidArgumentException();
				if (_key_length != key_size) throw InvalidArgumentException();
				Memory::MemoryCopy(&_key_data, key_data, _key_length);
				if (_mode == EncryptionMode::ECB) {
					if (_alg == EncryptionAlgorithm::AES128) _cipher = libcrypto::EVP_aes_128_ecb ? libcrypto::EVP_aes_128_ecb() : 0;
					else if (_alg == EncryptionAlgorithm::AES192) _cipher = libcrypto::EVP_aes_192_ecb ? libcrypto::EVP_aes_192_ecb() : 0;
					else if (_alg == EncryptionAlgorithm::AES256) _cipher = libcrypto::EVP_aes_256_ecb ? libcrypto::EVP_aes_256_ecb() : 0;
					else throw InvalidArgumentException();
				} else if (_mode == EncryptionMode::CBC) {
					if (_alg == EncryptionAlgorithm::AES128) _cipher = libcrypto::EVP_aes_128_cbc ? libcrypto::EVP_aes_128_cbc() : 0;
					else if (_alg == EncryptionAlgorithm::AES192) _cipher = libcrypto::EVP_aes_192_cbc ? libcrypto::EVP_aes_192_cbc() : 0;
					else if (_alg == EncryptionAlgorithm::AES256) _cipher = libcrypto::EVP_aes_256_cbc ? libcrypto::EVP_aes_256_cbc() : 0;
					else throw InvalidArgumentException();
				} else throw InvalidArgumentException();
				if (!_cipher) throw NotImplementedException();
			}
			virtual ~CryptoKey(void) override { Memory::ZeroMemory(&_key_data, sizeof(_key_data)); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Clavis"; ESSE_TRY_OUTRO(string()) }
			virtual EncryptionAlgorithm GetAlgorithm(void) const noexcept override { return _alg; }
			virtual EncryptionMode GetMode(void) const noexcept override { return _mode; }
			virtual uint GetBlockSize(void) const noexcept override { return 16; }
			virtual uint GetKeySize(void) const noexcept override { return _key_length; }
			virtual const void * GetKeyData(void) const noexcept override { return _key_data; }
			virtual void EncryptStream(Stream * dest, Stream * src, const void * iv, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!dest || !src) throw InvalidArgumentException();
					if (!libcrypto::EVP_CIPHER_CTX_new || !libcrypto::EVP_CIPHER_CTX_reset || !libcrypto::EVP_CIPHER_CTX_free) throw NotImplementedException();
					if (!libcrypto::EVP_EncryptInit_ex || !libcrypto::EVP_EncryptUpdate || !libcrypto::EVP_EncryptFinal_ex) throw NotImplementedException();
					auto context = libcrypto::EVP_CIPHER_CTX_new();
					if (!context) throw OutOfMemoryException();
					try {
						auto input = owrap(new DataBlock(1));
						auto output = owrap(new DataBlock(1));
						input->SetLength(0x100000);
						output->SetLength(0x100000 + GetBlockSize());
						if (!libcrypto::EVP_CIPHER_CTX_reset(context) || !libcrypto::EVP_EncryptInit_ex(context, _cipher, 0, _key_data, iv)) throw OutOfMemoryException();
						while (true) {
							auto size = src->ReadE(input->GetBuffer(), input->GetLength(), ectx);
							if (ErrorTest(ectx)) { libcrypto::EVP_CIPHER_CTX_free(context); return; }
							if (!size) break;
							int bytes_used = 0;
							if (!libcrypto::EVP_EncryptUpdate(context, output->GetBuffer(), &bytes_used, input->GetBuffer(), size)) throw OutOfMemoryException();
							dest->WriteE(output->GetBuffer(), bytes_used, ectx);
							if (ErrorTest(ectx)) { libcrypto::EVP_CIPHER_CTX_free(context); return; }
						}
						int bytes_final = 0;
						if (!libcrypto::EVP_EncryptFinal_ex(context, output->GetBuffer(), &bytes_final)) throw OutOfMemoryException();
						dest->WriteE(output->GetBuffer(), bytes_final, ectx);
						if (ErrorTest(ectx)) { libcrypto::EVP_CIPHER_CTX_free(context); return; }
					} catch (...) { libcrypto::EVP_CIPHER_CTX_free(context); throw; }
					libcrypto::EVP_CIPHER_CTX_free(context);
				ESSE_TRY_OUTRO()
			}
			virtual oref<DataBlock> EncryptData(const void * data, uintptr length, const void * iv, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!libcrypto::EVP_CIPHER_CTX_new || !libcrypto::EVP_CIPHER_CTX_reset || !libcrypto::EVP_CIPHER_CTX_free) throw NotImplementedException();
					if (!libcrypto::EVP_EncryptInit_ex || !libcrypto::EVP_EncryptUpdate || !libcrypto::EVP_EncryptFinal_ex) throw NotImplementedException();
					auto context = libcrypto::EVP_CIPHER_CTX_new();
					if (!context) throw OutOfMemoryException();
					oref<DataBlock> result;
					try {
						result = owrap(new DataBlock(length + GetBlockSize()));
						result->SetLength(length + GetBlockSize());
						if (!libcrypto::EVP_CIPHER_CTX_reset(context) || !libcrypto::EVP_EncryptInit_ex(context, _cipher, 0, _key_data, iv)) throw OutOfMemoryException();
						uintptr read_position = 0, write_position = 0;
						while (read_position < length) {
							auto size = min<uintptr>(length - read_position, 0x100000);
							int bytes_used = 0;
							if (!libcrypto::EVP_EncryptUpdate(context, result->GetBuffer() + write_position, &bytes_used, reinterpret_cast<const uint8 *>(data) + read_position, size)) throw OutOfMemoryException();
							read_position += size;
							write_position += bytes_used;
						}
						int bytes_final = 0;
						if (!libcrypto::EVP_EncryptFinal_ex(context, result->GetBuffer() + write_position, &bytes_final)) throw OutOfMemoryException();
						write_position += bytes_final;
						result->SetLength(write_position);
					} catch (...) { libcrypto::EVP_CIPHER_CTX_free(context); throw; }
					libcrypto::EVP_CIPHER_CTX_free(context);
					return result;
				ESSE_TRY_OUTRO(0)
			}
			virtual void DecryptStream(Stream * dest, Stream * src, const void * iv, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!dest || !src) throw InvalidArgumentException();
					if (!libcrypto::EVP_CIPHER_CTX_new || !libcrypto::EVP_CIPHER_CTX_reset || !libcrypto::EVP_CIPHER_CTX_free) throw NotImplementedException();
					if (!libcrypto::EVP_DecryptInit_ex || !libcrypto::EVP_DecryptUpdate || !libcrypto::EVP_DecryptFinal_ex) throw NotImplementedException();
					auto context = libcrypto::EVP_CIPHER_CTX_new();
					if (!context) throw OutOfMemoryException();
					try {
						auto input = owrap(new DataBlock(1));
						auto output = owrap(new DataBlock(1));
						input->SetLength(0x100000);
						output->SetLength(0x100000 + GetBlockSize());
						if (!libcrypto::EVP_CIPHER_CTX_reset(context) || !libcrypto::EVP_DecryptInit_ex(context, _cipher, 0, _key_data, iv)) throw OutOfMemoryException();
						while (true) {
							auto size = src->ReadE(input->GetBuffer(), input->GetLength(), ectx);
							if (ErrorTest(ectx)) { libcrypto::EVP_CIPHER_CTX_free(context); return; }
							if (!size) break;
							int bytes_used = 0;
							if (!libcrypto::EVP_DecryptUpdate(context, output->GetBuffer(), &bytes_used, input->GetBuffer(), size)) throw OutOfMemoryException();
							dest->WriteE(output->GetBuffer(), bytes_used, ectx);
							if (ErrorTest(ectx)) { libcrypto::EVP_CIPHER_CTX_free(context); return; }
						}
						int bytes_final = 0;
						if (!libcrypto::EVP_DecryptFinal_ex(context, output->GetBuffer(), &bytes_final)) throw InvalidFormatException();
						dest->WriteE(output->GetBuffer(), bytes_final, ectx);
						if (ErrorTest(ectx)) { libcrypto::EVP_CIPHER_CTX_free(context); return; }
					} catch (...) { libcrypto::EVP_CIPHER_CTX_free(context); throw; }
					libcrypto::EVP_CIPHER_CTX_free(context);
				ESSE_TRY_OUTRO()
			}
			virtual oref<DataBlock> DecryptData(const void * data, uintptr length, const void * iv, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!libcrypto::EVP_CIPHER_CTX_new || !libcrypto::EVP_CIPHER_CTX_reset || !libcrypto::EVP_CIPHER_CTX_free) throw NotImplementedException();
					if (!libcrypto::EVP_DecryptInit_ex || !libcrypto::EVP_DecryptUpdate || !libcrypto::EVP_DecryptFinal_ex) throw NotImplementedException();
					auto context = libcrypto::EVP_CIPHER_CTX_new();
					if (!context) throw OutOfMemoryException();
					oref<DataBlock> result;
					try {
						result = owrap(new DataBlock(length + GetBlockSize()));
						result->SetLength(length + GetBlockSize());
						if (!libcrypto::EVP_CIPHER_CTX_reset(context) || !libcrypto::EVP_DecryptInit_ex(context, _cipher, 0, _key_data, iv)) throw OutOfMemoryException();
						uintptr read_position = 0, write_position = 0;
						while (read_position < length) {
							auto size = min<uintptr>(length - read_position, 0x100000);
							int bytes_used = 0;
							if (!libcrypto::EVP_DecryptUpdate(context, result->GetBuffer() + write_position, &bytes_used, reinterpret_cast<const uint8 *>(data) + read_position, size)) throw OutOfMemoryException();
							read_position += size;
							write_position += bytes_used;
						}
						int bytes_final = 0;
						if (!libcrypto::EVP_DecryptFinal_ex(context, result->GetBuffer() + write_position, &bytes_final)) throw InvalidFormatException();
						write_position += bytes_final;
						result->SetLength(write_position);
					} catch (...) { libcrypto::EVP_CIPHER_CTX_free(context); throw; }
					libcrypto::EVP_CIPHER_CTX_free(context);
					return result;
				ESSE_TRY_OUTRO(0)
			}
		};
		void Linux_CryptoLibraryInitialize(void) noexcept
		{
			if (libcrypto::initialized) return;
			Memory::AcquireRootLock();
			if (libcrypto::initialized) { Memory::ReleaseRootLock(); return; }
			libcrypto::library = dlopen("/usr/lib/libcrypto.so", RTLD_NOW);
			if (libcrypto::library) {
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_aes_128_ecb)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_aes_128_cbc)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_aes_192_ecb)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_aes_192_cbc)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_aes_256_ecb)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_aes_256_cbc)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_md5)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_sha1)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_sha224)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_sha256)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_sha384)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_sha512)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_sha512_224)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_sha512_256)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_CIPHER_CTX_new)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_CIPHER_CTX_free)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_CIPHER_CTX_reset)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_EncryptInit_ex)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_EncryptUpdate)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_EncryptFinal_ex)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_DecryptInit_ex)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_DecryptUpdate)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_DecryptFinal_ex)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_MD_CTX_new)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_MD_CTX_free)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_MD_CTX_get_size_ex)
				DEFINE_CRYPTO_FUNCTION_IMPORT(RAND_bytes)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_DigestInit_ex)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_DigestUpdate)
				DEFINE_CRYPTO_FUNCTION_IMPORT(EVP_DigestFinal_ex)
			}
			libcrypto::initialized = true;
			Memory::ReleaseRootLock();
		}
		oref<Key> CreateKey(EncryptionAlgorithm alg, EncryptionMode mode, const void * key_data, uintptr key_size, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				Linux_CryptoLibraryInitialize();
				return oref<Key>::CreateOwned(new CryptoKey(alg, mode, key_data, key_size));
			ESSE_TRY_OUTRO(0)
		}
		oref<DataBlock> CreateHashOfStream(HashAlgorithm hash, Stream * src, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				if (!src) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
				Linux_CryptoLibraryInitialize();
				if (!libcrypto::EVP_MD_CTX_new || !libcrypto::EVP_MD_CTX_free || !libcrypto::EVP_MD_CTX_get_size_ex) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
				if (!libcrypto::EVP_DigestInit_ex || !libcrypto::EVP_DigestUpdate || !libcrypto::EVP_DigestFinal_ex) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
				handle method = 0;
				if (hash == HashAlgorithm::MD5) method = libcrypto::EVP_md5 ? libcrypto::EVP_md5() : 0;
				else if (hash == HashAlgorithm::SHA1) method = libcrypto::EVP_sha1 ? libcrypto::EVP_sha1() : 0;
				else if (hash == HashAlgorithm::SHA224) method = libcrypto::EVP_sha224 ? libcrypto::EVP_sha224() : 0;
				else if (hash == HashAlgorithm::SHA256) method = libcrypto::EVP_sha256 ? libcrypto::EVP_sha256() : 0;
				else if (hash == HashAlgorithm::SHA384) method = libcrypto::EVP_sha384 ? libcrypto::EVP_sha384() : 0;
				else if (hash == HashAlgorithm::SHA512) method = libcrypto::EVP_sha512 ? libcrypto::EVP_sha512() : 0;
				else if (hash == HashAlgorithm::SHA224_512) method = libcrypto::EVP_sha512_224 ? libcrypto::EVP_sha512_224() : 0;
				else if (hash == HashAlgorithm::SHA256_512) method = libcrypto::EVP_sha512_256 ? libcrypto::EVP_sha512_256() : 0;
				else { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
				if (!method) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
				handle context = libcrypto::EVP_MD_CTX_new();
				if (!context) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
				if (!libcrypto::EVP_DigestInit_ex(context, method, 0)) { libcrypto::EVP_MD_CTX_free(context); ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
				oref<DataBlock> result;
				try {
					oref<DataBlock> data = owrap(new DataBlock(1));
					data->SetLength(0x100000);
					while (true) {
						auto size = src->ReadE(data->GetBuffer(), data->GetLength(), ectx);
						if (ErrorTest(ectx)) { libcrypto::EVP_MD_CTX_free(context); return 0; }
						if (!size) break;
						if (!libcrypto::EVP_DigestUpdate(context, data->GetBuffer(), size)) { libcrypto::EVP_MD_CTX_free(context); ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
					}
					auto size = libcrypto::EVP_MD_CTX_get_size_ex(context);
					if (size <= 0) throw InvalidStateException(); 
					result = owrap(new DataBlock(size));
					result->SetLength(size);
					if (!libcrypto::EVP_DigestFinal_ex(context, result->GetBuffer(), 0)) throw OutOfMemoryException();
				} catch (...) { libcrypto::EVP_MD_CTX_free(context); throw; }
				libcrypto::EVP_MD_CTX_free(context);
				return result;
			ESSE_TRY_OUTRO(0)
		}
		oref<DataBlock> CreateHashOfData(HashAlgorithm hash, const void * data, uintptr length, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				Linux_CryptoLibraryInitialize();
				if (!libcrypto::EVP_MD_CTX_new || !libcrypto::EVP_MD_CTX_free || !libcrypto::EVP_MD_CTX_get_size_ex) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
				if (!libcrypto::EVP_DigestInit_ex || !libcrypto::EVP_DigestUpdate || !libcrypto::EVP_DigestFinal_ex) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
				handle method = 0;
				if (hash == HashAlgorithm::MD5) method = libcrypto::EVP_md5 ? libcrypto::EVP_md5() : 0;
				else if (hash == HashAlgorithm::SHA1) method = libcrypto::EVP_sha1 ? libcrypto::EVP_sha1() : 0;
				else if (hash == HashAlgorithm::SHA224) method = libcrypto::EVP_sha224 ? libcrypto::EVP_sha224() : 0;
				else if (hash == HashAlgorithm::SHA256) method = libcrypto::EVP_sha256 ? libcrypto::EVP_sha256() : 0;
				else if (hash == HashAlgorithm::SHA384) method = libcrypto::EVP_sha384 ? libcrypto::EVP_sha384() : 0;
				else if (hash == HashAlgorithm::SHA512) method = libcrypto::EVP_sha512 ? libcrypto::EVP_sha512() : 0;
				else if (hash == HashAlgorithm::SHA224_512) method = libcrypto::EVP_sha512_224 ? libcrypto::EVP_sha512_224() : 0;
				else if (hash == HashAlgorithm::SHA256_512) method = libcrypto::EVP_sha512_256 ? libcrypto::EVP_sha512_256() : 0;
				else { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
				if (!method) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
				handle context = libcrypto::EVP_MD_CTX_new();
				if (!context) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
				if (!libcrypto::EVP_DigestInit_ex(context, method, 0)) { libcrypto::EVP_MD_CTX_free(context); ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
				uintptr position = 0;
				while (position < length) {
					uintptr process = min<uintptr>(length - position, 0x100000);
					if (!libcrypto::EVP_DigestUpdate(context, reinterpret_cast<const uint8 *>(data) + position, process)) { libcrypto::EVP_MD_CTX_free(context); ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
					position += process;
				}
				auto size = libcrypto::EVP_MD_CTX_get_size_ex(context);
				oref<DataBlock> result;
				try {
					if (size <= 0) throw InvalidStateException(); 
					result = owrap(new DataBlock(size));
					result->SetLength(size);
					if (!libcrypto::EVP_DigestFinal_ex(context, result->GetBuffer(), 0)) throw OutOfMemoryException();
				} catch (...) { libcrypto::EVP_MD_CTX_free(context); throw; }
				libcrypto::EVP_MD_CTX_free(context);
				return result;
			ESSE_TRY_OUTRO(0)
		}
		oref<DataBlock> CreateRandom(uintptr length, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				Linux_CryptoLibraryInitialize();
				if (!libcrypto::RAND_bytes) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
				auto result = owrap(new DataBlock(length));
				result->SetLength(length);
				uintptr position = 0;
				while (position < result->GetLength()) {
					uintptr create = min<uintptr>(result->GetLength() - position, 0x1000000);
					auto status = libcrypto::RAND_bytes(result->GetBuffer() + position, create);
					if (status != 1) { ErrorSet(ectx, Errores::ErrorInvalidState); return 0; }
					position += create;
				}
				return result;
			ESSE_TRY_OUTRO(0)
		}
	}
}