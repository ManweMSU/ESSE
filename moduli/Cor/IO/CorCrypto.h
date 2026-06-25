#pragma once

#include "CorStreams.h"

namespace ESSE
{
	namespace Cryptography
	{
		enum class EncryptionAlgorithm { AES128 = 128, AES192 = 192, AES256 = 256 };
		enum class EncryptionMode { ECB = 0, CBC = 1 };
		enum class HashAlgorithm { MD5 = 1, SHA1 = 2, SHA224 = 224, SHA256 = 256, SHA384 = 384, SHA512 = 512, SHA224_512 = 225, SHA256_512 = 257 };

		class Key : public Object
		{
		public:
			virtual EncryptionAlgorithm GetAlgorithm(void) const noexcept = 0;
			virtual EncryptionMode GetMode(void) const noexcept = 0;
			virtual uint GetBlockSize(void) const noexcept = 0;
			virtual uint GetKeySize(void) const noexcept = 0;
			virtual const void * GetKeyData(void) const noexcept = 0;

			virtual void EncryptStream(Stream * dest, Stream * src, const void * iv, ErrorContext & ectx) noexcept = 0;
			virtual oref<DataBlock> EncryptData(const void * data, uintptr length, const void * iv, ErrorContext & ectx) noexcept = 0;
			virtual void DecryptStream(Stream * dest, Stream * src, const void * iv, ErrorContext & ectx) noexcept = 0;
			virtual oref<DataBlock> DecryptData(const void * data, uintptr length, const void * iv, ErrorContext & ectx) noexcept = 0;

			void EncryptStream(Stream * dest, Stream * src, const void * iv);
			oref<DataBlock> EncryptData(const void * data, uintptr length, const void * iv);
			void DecryptStream(Stream * dest, Stream * src, const void * iv);
			oref<DataBlock> DecryptData(const void * data, uintptr length, const void * iv);
		};

		oref<Key> CreateKey(EncryptionAlgorithm alg, EncryptionMode mode, const void * key_data, uintptr key_size, ErrorContext & ectx) noexcept;
		oref<DataBlock> CreateHashOfStream(HashAlgorithm hash, Stream * src, ErrorContext & ectx) noexcept;
		oref<DataBlock> CreateHashOfData(HashAlgorithm hash, const void * data, uintptr length, ErrorContext & ectx) noexcept;
		oref<DataBlock> CreateRandom(uintptr length, ErrorContext & ectx) noexcept;

		oref<Key> CreateKey(EncryptionAlgorithm alg, EncryptionMode mode, const void * key_data, uintptr key_size);
		oref<DataBlock> CreateHashOfStream(HashAlgorithm hash, Stream * src);
		oref<DataBlock> CreateHashOfData(HashAlgorithm hash, const void * data, uintptr length);
		oref<DataBlock> CreateRandom(uintptr length);
	}
}