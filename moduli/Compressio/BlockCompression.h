#pragma once

#include <Cor/Classes/CorArray.hxx>

namespace ESSE
{
	namespace Compression
	{
		enum class Method : uint {
			Huffman							= 1,
			LempelZivWelch					= 3,
			FusedLempelZivWelchHuffman9bit	= 4,
			FusedLempelZivWelchHuffman10bit	= 5,
			FusedLempelZivWelchHuffman11bit	= 6,
			FusedLempelZivWelchHuffman12bit	= 7,
			RunLengthEncoding8bit			= 9,
			RunLengthEncoding16bit			= 10,
			RunLengthEncoding32bit			= 11,
			RunLengthEncoding64bit			= 12,
			RunLengthEncoding128bit			= 13
		};
		oref<DataBlock> Compress(const void * data, uintptr length, Method method);
		oref<DataBlock> Decompress(const void * data, uintptr length, Method method);
	}
}