#include "BitStream.h"

namespace ESSE
{
	namespace Compression
	{
		uint GetBit(const void * data, uint index) noexcept { return (reinterpret_cast<const uint8 *>(data)[index >> 3U] >> (index & 7U)) & 1U; }
		void SetBit(void * data, uint index, uint value) noexcept { auto & byte = reinterpret_cast<uint8 *>(data)[index >> 3U]; byte &= ~(1U << (index & 7U)); byte |= value << (index & 7U); }
		
		void BitStream::InitializeForReading(const uint8 * in, uintptr len) noexcept { stream.Clear(); input = in; position = 0; length = len << 3U; }
		void BitStream::InitializeForWriting(uint block_size) { stream = owrap(new DataBlock(block_size)); input = 0; position = length = 0; }
		void BitStream::WriteBit(uint value) { if (!(length & 7)) stream->Append(0); SetBit(stream->GetBuffer(), length++, value); }
		void BitStream::WriteBits(const void * data, uint bitlength) { for (uint i = 0; i < bitlength; i++) WriteBit(GetBit(data, i)); }
		uint BitStream::ReadBit(void) noexcept { if (EndOfStream()) return 0; return GetBit(input, position++); }
		uint BitStream::ReadByte(void) noexcept { uint8 r; ReadBits(&r, sizeof(r) << 3U); return r; }
		uint BitStream::ReadShort(void) noexcept { uint16 r; ReadBits(&r, sizeof(r) << 3U); return r; }
		uint BitStream::ReadInteger(void) noexcept { uint32 r; ReadBits(&r, sizeof(r) << 3U); return r; }
		void BitStream::ReadBits(void * dest, uint bitlength) noexcept { for (uint i = 0; i < bitlength; i++) SetBit(dest, i, ReadBit()); }
		bool BitStream::EndOfStream(void) const noexcept { return position >= length; }
	}
}