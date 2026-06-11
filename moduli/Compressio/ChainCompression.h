#pragma once

#include "BlockCompression.h"
#include <Cor/IO/CorStreams.h>
#include <Cor/Tasks/CorTasks.h>

namespace ESSE
{
	namespace Compression
	{
		enum class Quality {
			// Always apply full compression chain, efficiency is not checked.
			Force,
			// Apply full compression chain or do not compress if efficiency is negative.
			Optional,
			// The compression step is skipped, if the local efficiency of this step is negative.
			Sequential,
			// Variates chaines by excluding some first methods from source chain. The most effective chain result is written.
			Variative,
			// Variates all subchaines of the source chain. The most effective chain result is written. Very expensive.
			ExtraVariative
		};
		class MethodChain
		{
		public:
			uint32 value;
		public:
			MethodChain(void) noexcept;
			MethodChain(uint32 sv) noexcept;
			MethodChain(Method a) noexcept;
			MethodChain(Method a, Method b) noexcept;
			MethodChain(Method a, Method b, Method c) noexcept;
			MethodChain(Method a, Method b, Method c, Method d) noexcept;
			MethodChain(Method a, Method b, Method c, Method d, Method e) noexcept;
			MethodChain(Method a, Method b, Method c, Method d, Method e, Method f) noexcept;
			MethodChain(Method a, Method b, Method c, Method d, Method e, Method f, Method g) noexcept;
			MethodChain(Method a, Method b, Method c, Method d, Method e, Method f, Method g, Method h) noexcept;

			bool friend operator == (MethodChain a, MethodChain b) noexcept;
			bool friend operator != (MethodChain a, MethodChain b) noexcept;

			uint GetLength(void) const noexcept;
			Method operator [] (uintptr index) const noexcept;
			Method MethodAt(uintptr index) const noexcept;
			MethodChain Subchain(uint mask) const noexcept;
			MethodChain Append(Method method) const noexcept;
		};

		oref<DataBlock> Compress(const void * data, uintptr length, MethodChain chain);
		oref<DataBlock> Compress(const void * data, uintptr length, MethodChain & chain, Quality quality);
		oref<DataBlock> Compress(const void * data, uintptr length, const MethodChain * chains, uint number, Quality quality, MethodChain & final);
		oref<DataBlock> Decompress(const void * data, uintptr length, MethodChain chain);

		void ChainCompress(Stream * dest, Stream * source, const MethodChain * chains, uint number, Quality quality, ThreadPool * pool = 0, uint32 block_size = 0x20000);
		void ChainDecompress(Stream * dest, Stream * source, ThreadPool * pool = 0);
		
		oref<Stream> CreateDecompressionStream(Stream * compressed);
	}
}