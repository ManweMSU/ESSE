#include "CorCrypto.h"

namespace ESSE
{
	namespace Cryptography
	{
		void Key::EncryptStream(Stream * dest, Stream * src, const void * iv)
		{
			ErrorContext ectx; ErrorClear(ectx);
			EncryptStream(dest, src, iv, ectx);
			ErrorThrow(ectx);
		}
		oref<DataBlock> Key::EncryptData(const void * data, uintptr length, const void * iv)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = EncryptData(data, length, iv, ectx);
			ErrorThrow(ectx);
			return result;
		}
		void Key::DecryptStream(Stream * dest, Stream * src, const void * iv)
		{
			ErrorContext ectx; ErrorClear(ectx);
			DecryptStream(dest, src, iv, ectx);
			ErrorThrow(ectx);
		}
		oref<DataBlock> Key::DecryptData(const void * data, uintptr length, const void * iv)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = DecryptData(data, length, iv, ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<Key> CreateKey(EncryptionAlgorithm alg, EncryptionMode mode, const void * key_data, uintptr key_size)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreateKey(alg, mode, key_data, key_size, ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<DataBlock> CreateHashOfStream(HashAlgorithm hash, Stream * src)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreateHashOfStream(hash, src, ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<DataBlock> CreateHashOfData(HashAlgorithm hash, const void * data, uintptr length)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreateHashOfData(hash, data, length, ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<DataBlock> CreateRandom(uintptr length)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreateRandom(length, ectx);
			ErrorThrow(ectx);
			return result;
		}
	}
}