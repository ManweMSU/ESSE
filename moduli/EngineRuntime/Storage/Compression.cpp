#include "Compression.h"

namespace Engine
{
	namespace Storage
	{
		Array<uint8>* Compress(const void * data, int length, CompressionMethod method)
		{
			SafePointer<DataBlock> result = new DataBlock(1);
			auto com = ESSE::Compression::Compress(data, length, method);
			result->Append(com->GetBuffer(), com->GetLength());
			result->Retain();
			return result;
		}
		Array<uint8>* Compress(const Array<uint8>& data, CompressionMethod method)
		{
			SafePointer<DataBlock> result = new DataBlock(1);
			auto com = ESSE::Compression::Compress(data.GetBuffer(), data.Length(), method);
			result->Append(com->GetBuffer(), com->GetLength());
			result->Retain();
			return result;
		}
		Array<uint8>* Decompress(const void * data, int length, CompressionMethod method)
		{
			SafePointer<DataBlock> result = new DataBlock(1);
			auto com = ESSE::Compression::Decompress(data, length, method);
			result->Append(com->GetBuffer(), com->GetLength());
			result->Retain();
			return result;
		}
		Array<uint8>* Decompress(const Array<uint8>& data, CompressionMethod method)
		{
			SafePointer<DataBlock> result = new DataBlock(1);
			auto com = ESSE::Compression::Decompress(data.GetBuffer(), data.Length(), method);
			result->Append(com->GetBuffer(), com->GetLength());
			result->Retain();
			return result;
		}
	}
}