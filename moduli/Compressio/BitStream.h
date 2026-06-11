#pragma once

#include <Cor/Classes/CorArray.hxx>

namespace ESSE
{
	namespace Compression
	{
		uint GetBit(const void * data, uint index) noexcept;
		void SetBit(void * data, uint index, uint value) noexcept;

		struct BitStream
		{
			oref<DataBlock> stream;
			const uint8 * input;
			uint length, position;

			void InitializeForReading(const uint8 * in, uintptr len) noexcept;
			void InitializeForWriting(uint block_size);
		
			void WriteBit(uint value);
			void WriteBits(const void * data, uint bitlength);

			uint ReadBit(void) noexcept;
			uint ReadByte(void) noexcept;
			uint ReadShort(void) noexcept;
			uint ReadInteger(void) noexcept;
			void ReadBits(void * dest, uint bitlength) noexcept;
			bool EndOfStream(void) const noexcept;
		};
	}
}