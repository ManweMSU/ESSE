#include "../Interfaces/Cryptography.h"

namespace Engine
{
	namespace Cryptography
	{
		class SystemEncryptionKey : public Key
		{
			ESSE::oref<ESSE::Cryptography::Key> _key;
			SafePointer<Algorithm> _alg;
		public:
			SystemEncryptionKey(Algorithm * alg, const void * key_data, ESSE::Cryptography::Key * key) : _key(key) { _alg.SetRetain(alg); }
			virtual ~SystemEncryptionKey(void) override {}
			virtual DataBlock * ExportKey(void) override
			{
				try {
					SafePointer<DataBlock> result = new DataBlock(32);
					result->SetLength(_key->GetKeySize());
					MemoryCopy(result->GetBuffer(), _key->GetKeyData(), _key->GetKeySize());
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual Algorithm * GetAlgorithm(void) const override { return _alg; }
			virtual DataBlock * EncryptData(const void * data, int length, const void * iv) override
			{
				try {
					auto transformed = _key->EncryptData(data, length, iv);
					SafePointer<DataBlock> result = new DataBlock(1);
					result->SetLength(transformed->GetLength());
					MemoryCopy(result->GetBuffer(), transformed->GetBuffer(), transformed->GetLength());
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual DataBlock * DecryptData(const void * data, int length, const void * iv) override
			{
				try {
					auto transformed = _key->DecryptData(data, length, iv);
					SafePointer<DataBlock> result = new DataBlock(1);
					result->SetLength(transformed->GetLength());
					MemoryCopy(result->GetBuffer(), transformed->GetBuffer(), transformed->GetLength());
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
		};
		class SystemEncryptionAlgorithm : public Algorithm
		{
			EncryptionAlgorithm _algorithm;
			EncryptionMode _mode;
		public:
			SystemEncryptionAlgorithm(EncryptionAlgorithm algorithm) : _algorithm(algorithm), _mode(EncryptionMode::CBC) { if (algorithm != EncryptionAlgorithm::AES) throw Exception(); }
			virtual ~SystemEncryptionAlgorithm(void) override {}
			virtual EncryptionAlgorithm Identifier(void) const override { return _algorithm; }
			virtual int EncryptionBlockSize(void) const override
			{
				if (_algorithm == EncryptionAlgorithm::AES) return 16;
				else return 0;
			}
			virtual EncryptionMode GetEncryptionMode(void) const override { return _mode; }
			virtual void SetEncryptionMode(EncryptionMode mode) override { _mode = mode; }
			virtual Key * ImportKey(const void * data, int length) override
			{
				if (_algorithm == EncryptionAlgorithm::AES) {
					if (length != 32) return 0;
					try {
						ESSE::oref<ESSE::Cryptography::Key> key;
						if (_mode == EncryptionMode::ECB) key = ESSE::Cryptography::CreateKey(ESSE::Cryptography::EncryptionAlgorithm::AES256, ESSE::Cryptography::EncryptionMode::ECB, data, length);
						else if (_mode == EncryptionMode::CBC) key = ESSE::Cryptography::CreateKey(ESSE::Cryptography::EncryptionAlgorithm::AES256, ESSE::Cryptography::EncryptionMode::CBC, data, length);
						else return 0;
						return new SystemEncryptionKey(this, data, key);
					} catch (...) { return 0; }
				} else return 0;
			}
			virtual Key * GenerateKey(const void * secret, int length) override
			{
				if (length < 0) return 0;
				if (_algorithm == EncryptionAlgorithm::AES) {
					uint8 data[32];
					ZeroMemory(&data, 32);
					MemoryCopy(&data, secret, min(length, 32));
					return ImportKey(&data, 32);
				} else return 0;
			}
		};

		Key * Algorithm::ImportKey(const DataBlock * data) { return ImportKey(data->GetBuffer(), data->Length()); }
		Key * Algorithm::GenerateKey(const DataBlock * data) { return GenerateKey(data->GetBuffer(), data->Length()); }
		DataBlock * Key::EncryptData(const DataBlock * data, const void * iv) { return EncryptData(data->GetBuffer(), data->Length(), iv); }
		DataBlock * Key::DecryptData(const DataBlock * data, const void * iv) { return DecryptData(data->GetBuffer(), data->Length(), iv); }

		Algorithm * OpenEncryptionAlgorithm(EncryptionAlgorithm algorithm) { try { return new SystemEncryptionAlgorithm(algorithm); } catch (...) { return 0; } }
		DataBlock * CreateHash(HashAlgorithm algorithm, const void * data, int length)
		{
			try {
				ESSE::oref<ESSE::DataBlock> hash;
				if (algorithm == HashAlgorithm::MD5) hash = ESSE::Cryptography::CreateHashOfData(ESSE::Cryptography::HashAlgorithm::MD5, data, length);
				else if (algorithm == HashAlgorithm::SHA1) hash = ESSE::Cryptography::CreateHashOfData(ESSE::Cryptography::HashAlgorithm::SHA1, data, length);
				else if (algorithm == HashAlgorithm::SHA256) hash = ESSE::Cryptography::CreateHashOfData(ESSE::Cryptography::HashAlgorithm::SHA256, data, length);
				else if (algorithm == HashAlgorithm::SHA384) hash = ESSE::Cryptography::CreateHashOfData(ESSE::Cryptography::HashAlgorithm::SHA384, data, length);
				else if (algorithm == HashAlgorithm::SHA512) hash = ESSE::Cryptography::CreateHashOfData(ESSE::Cryptography::HashAlgorithm::SHA512, data, length);
				if (!hash) return 0;
				SafePointer<DataBlock> result = new DataBlock(hash->GetLength());
				result->SetLength(hash->GetLength());
				MemoryCopy(result->GetBuffer(), hash->GetBuffer(), hash->GetLength());
				result->Retain();
				return result;
			} catch (...) { return 0; }
		}
		DataBlock * CreateHash(HashAlgorithm algorithm, const DataBlock * data) { return CreateHash(algorithm, data->GetBuffer(), data->Length()); }
		DataBlock * CreateSecureRandom(int length)
		{
			try {
				auto random = ESSE::Cryptography::CreateRandom(length);
				SafePointer<DataBlock> result = new DataBlock(random->GetLength());
				result->SetLength(random->GetLength());
				MemoryCopy(result->GetBuffer(), random->GetBuffer(), random->GetLength());
				result->Retain();
				return result;
			} catch (...) { return 0; }
		}
	}
}