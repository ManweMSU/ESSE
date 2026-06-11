#pragma once

#include "../EngineBase.h"
#include <Compressio/BlockCompression.h>

namespace Engine
{
	namespace Storage
	{
		typedef ESSE::Compression::Method CompressionMethod;
		Array<uint8> * Compress(const void * data, int length, CompressionMethod method);
		Array<uint8> * Compress(const Array<uint8> & data, CompressionMethod method);
		Array<uint8> * Decompress(const void * data, int length, CompressionMethod method);
		Array<uint8> * Decompress(const Array<uint8> & data, CompressionMethod method);
	}
}