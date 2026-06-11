#include "ChainCompression.h"

namespace ESSE
{
	namespace Compression
	{
		MethodChain::MethodChain(void) noexcept : value(0) {}
		MethodChain::MethodChain(uint32 sv) noexcept : value(sv) {}
		MethodChain::MethodChain(Method a) noexcept : value(uint(a)) {}
		MethodChain::MethodChain(Method a, Method b) noexcept : value(uint(a) | (uint(b) << 4)) {}
		MethodChain::MethodChain(Method a, Method b, Method c) noexcept : value(uint(a) | (uint(b) << 4) | (uint(c) << 8)) {}
		MethodChain::MethodChain(Method a, Method b, Method c, Method d) noexcept : value(uint(a) | (uint(b) << 4) | (uint(c) << 8) | (uint(d) << 12)) {}
		MethodChain::MethodChain(Method a, Method b, Method c, Method d, Method e) noexcept : value(uint(a) | (uint(b) << 4) | (uint(c) << 8) | (uint(d) << 12) | (uint(e) << 16)) {}
		MethodChain::MethodChain(Method a, Method b, Method c, Method d, Method e, Method f) noexcept : value(uint(a) | (uint(b) << 4) | (uint(c) << 8) | (uint(d) << 12) | (uint(e) << 16) | (uint(f) << 20)) {}
		MethodChain::MethodChain(Method a, Method b, Method c, Method d, Method e, Method f, Method g) noexcept : value(uint(a) | (uint(b) << 4) | (uint(c) << 8) | (uint(d) << 12) | (uint(e) << 16) | (uint(f) << 20) | (uint(g) << 24)) {}
		MethodChain::MethodChain(Method a, Method b, Method c, Method d, Method e, Method f, Method g, Method h) noexcept : value(uint(a) | (uint(b) << 4) | (uint(c) << 8) | (uint(d) << 12) | (uint(e) << 16) | (uint(f) << 20) | (uint(g) << 24) | (uint(h) << 28)) {}
		bool operator == (MethodChain a, MethodChain b) noexcept { return a.value == b.value; }
		bool operator != (MethodChain a, MethodChain b) noexcept { return a.value != b.value; }
		uint MethodChain::GetLength(void) const noexcept { uint length = 0; while ((value & (0xFU << (length << 2))) && (length < 7)) length++; return length; }
		Method MethodChain::operator [] (uintptr index) const noexcept { return static_cast<Method>((value >> (index << 2)) & 0xF); }
		Method MethodChain::MethodAt(uintptr index) const noexcept { return static_cast<Method>((value >> (index << 2)) & 0xF); }
		MethodChain MethodChain::Subchain(uint mask) const noexcept
		{
			uint result = 0, shift = 0;
			for (uint i = 0; i < 8; i++) if ((mask >> i) & 1U) {
				result |= uint(MethodAt(i)) << shift;
				shift += 4;
			}
			return result;
		}
		MethodChain MethodChain::Append(Method method) const noexcept { return value | (uint(method) << (GetLength() << 2)); }

		oref<DataBlock> Compress(const void * data, uintptr length, MethodChain chain)
		{
			oref<DataBlock> result;
			for (uint i = 0; i < chain.GetLength(); i++) {
				result = Compress(data, length, chain[i]);
				data = result->GetBuffer();
				length = result->GetLength();
			}
			if (result) return result; else {
				auto result = owrap(new DataBlock(1));
				result->Append(reinterpret_cast<const uint8 *>(data), length);
				return result;
			}
		}
		oref<DataBlock> Compress(const void * data, uintptr length, MethodChain & chain, Quality quality)
		{
			if (quality == Quality::Force) {
				return Compress(data, length, chain);
			} else if (quality == Quality::Optional) {
				auto com = Compress(data, length, chain);
				if (com->GetLength() < length) return com; else {
					chain = 0;
					auto result = owrap(new DataBlock(1));
					result->Append(reinterpret_cast<const uint8 *>(data), length);
					return result;
				}
			} else if (quality == Quality::Sequential) {
				MethodChain result_chain;
				oref<DataBlock> result;
				for (uint i = 0; i < chain.GetLength(); i++) {
					auto com = Compress(data, length, chain[i]);
					if (com->GetLength() >= length) continue;
					result = com;
					data = result->GetBuffer();
					length = result->GetLength();
					result_chain = result_chain.Append(chain[i]);
				}
				chain = result_chain;
				if (result) return result; else {
					auto result = owrap(new DataBlock(1));
					result->Append(reinterpret_cast<const uint8 *>(data), length);
					return result;
				}
			} else if (quality == Quality::Variative) {
				MethodChain best_chain;
				oref<DataBlock> best_com;
				for (uint k = 0; k <= chain.GetLength(); k++) {
					MethodChain local;
					for (uint i = k; i < chain.GetLength(); i++) local = local.Append(chain[i]);
					auto com = Compress(data, length, local);
					if (!best_com || best_com->GetLength() > com->GetLength()) { best_com = com; best_chain = local; }
				}
				if (best_com && best_com->GetLength() < length) {
					chain = best_chain;
					return best_com;
				} else {
					chain = 0;
					auto best_com = owrap(new DataBlock(1));
					best_com->Append(reinterpret_cast<const uint8 *>(data), length);
					return best_com;
				}
			} else if (quality == Quality::ExtraVariative) {
				MethodChain best_chain;
				oref<DataBlock> best_com;
				for (uint8 k = 0; k < (1U << chain.GetLength()); k++) {
					auto local = chain.Subchain(k);
					auto com = Compress(data, length, local);
					if (!best_com || best_com->GetLength() > com->GetLength()) { best_com = com; best_chain = local; }
				}
				if (best_com && best_com->GetLength() < length) {
					chain = best_chain;
					return best_com;
				} else {
					chain = 0;
					auto best_com = owrap(new DataBlock(1));
					best_com->Append(reinterpret_cast<const uint8 *>(data), length);
					return best_com;
				}
			} else throw InvalidArgumentException();
		}
		oref<DataBlock> Compress(const void * data, uintptr length, const MethodChain * chains, uint number, Quality quality, MethodChain & final)
		{
			if (!number) throw InvalidArgumentException();
			oref<DataBlock> result;
			for (uint i = 0; i < number; i++) {
				auto chain = chains[i];
				auto com = Compress(data, length, chain);
				if (!result || result->GetLength() > com->GetLength()) { result = com; final = chain; }
			}
			return result;
		}
		oref<DataBlock> Decompress(const void * data, uintptr length, MethodChain chain)
		{
			oref<DataBlock> result;
			for (uint i = chain.GetLength(); i > 0; i--) {
				result = Decompress(data, length, chain[i - 1]);
				data = result->GetBuffer();
				length = result->GetLength();
			}
			if (result) return result; else {
				auto result = owrap(new DataBlock(1));
				result->Append(reinterpret_cast<const uint8 *>(data), length);
				return result;
			}
		}

		void ChainCompress(Stream * dest, Stream * source, const MethodChain * chains, uint number, Quality quality, ThreadPool * pool, uint32 block_size)
		{
			if (!block_size || !dest || !source) throw InvalidArgumentException();
			auto length = source->GetLength();
			auto num_blocks = length / block_size + (length % block_size ? 1 : 0);
			if (num_blocks > 0xFFFFFFFFU) throw InvalidArgumentException();
			dest->Write(&num_blocks, 4);
			if (pool) {
				pool->Wait();
				object_array<Semaphore> sync(1);
				object_array<IDispatchTask> tasks(1);
				sync.SetLength(pool->GetThreadCount());
				tasks.SetLength(sync.GetLength());
				ErrorContext ectx; ErrorClear(ectx);
				for (uint i = 0; i < sync.GetLength(); i++) {
					sync.SetElement(CreateSemaphore(0), i);
					tasks.SetElement(CreateFunctionalTask([&, index = i]() {
						sync[index].Wait();
						while (true) {
							if (ErrorTest(ectx)) { for (auto & s : sync) s.Open(); return; }
							if (!length) { sync[(index + 1) % sync.GetLength()].Open(); return; }
							try {
								auto subblock_length = (length > block_size) ? block_size : length;
								auto subblock_data = source->ReadBlock(subblock_length);
								length -= subblock_length;
								sync[(index + 1) % sync.GetLength()].Open();
								MethodChain final_chain;
								auto com = Compress(subblock_data->GetBuffer(), subblock_data->GetLength(), chains, number, quality, final_chain);
								uint32 header[3];
								header[0] = com->GetLength();
								header[1] = subblock_data->GetLength();
								header[2] = final_chain.value;
								sync[index].Wait();
								if (ErrorTest(ectx)) { for (auto & s : sync) s.Open(); return; }
								dest->Write(&header, sizeof(header));
								dest->WriteBlock(com);
							} catch (Exception & e) {
								Memory::AcquireRootLock();
								ectx = e.GetError();
								Memory::ReleaseRootLock();
								for (auto & s : sync) s.Open();
								return;
							} catch (...) {
								Memory::AcquireRootLock();
								ErrorSet(ectx, Errores::ErrorOutOfMemory);
								Memory::ReleaseRootLock();
								for (auto & s : sync) s.Open();
								return;
							}
						}
					}), i);
				}
				pool->SubmitTasksE(tasks.GetBuffer(), tasks.GetLength(), ectx);
				if (ErrorTest(ectx)) {
					for (auto & s : sync) s.Open();
					pool->Wait();
					ErrorThrow(ectx);
				}
				sync[0].Open();
				pool->Wait();
				ErrorThrow(ectx);
			} else while (length) {
				auto subblock_length = (length > block_size) ? block_size : length;
				auto subblock_data = source->ReadBlock(subblock_length);
				MethodChain final_chain;
				auto com = Compress(subblock_data->GetBuffer(), subblock_data->GetLength(), chains, number, quality, final_chain);
				uint32 header[3];
				header[0] = com->GetLength();
				header[1] = subblock_data->GetLength();
				header[2] = final_chain.value;
				dest->Write(&header, sizeof(header));
				dest->WriteBlock(com);
				length -= subblock_length;
			}
		}
		void ChainDecompress(Stream * dest, Stream * source, ThreadPool * pool)
		{
			if (!dest || !source) throw InvalidArgumentException();
			uint32 num_blocks;
			source->Read(&num_blocks, 4);
			if (pool) {
				pool->Wait();
				object_array<Semaphore> sync(1);
				object_array<IDispatchTask> tasks(1);
				sync.SetLength(pool->GetThreadCount());
				tasks.SetLength(sync.GetLength());
				ErrorContext ectx; ErrorClear(ectx);
				for (uint i = 0; i < sync.GetLength(); i++) {
					sync.SetElement(CreateSemaphore(0), i);
					tasks.SetElement(CreateFunctionalTask([&, index = i]() {
						sync[index].Wait();
						while (true) {
							if (ErrorTest(ectx)) { for (auto & s : sync) s.Open(); return; }
							if (!num_blocks) { sync[(index + 1) % sync.GetLength()].Open(); return; }
							try {
								uint32 header[3];
								if (source->Read(&header, sizeof(header)) != sizeof(header)) throw InvalidFormatException();
								auto com = source->ReadBlock(header[0]);
								num_blocks--;
								sync[(index + 1) % sync.GetLength()].Open();
								auto data = Decompress(com->GetBuffer(), com->GetLength(), header[2]);
								if (data->GetLength() != header[1]) throw InvalidFormatException();
								sync[index].Wait();
								if (ErrorTest(ectx)) { for (auto & s : sync) s.Open(); return; }
								dest->WriteBlock(data);
							} catch (Exception & e) {
								Memory::AcquireRootLock();
								ectx = e.GetError();
								Memory::ReleaseRootLock();
								for (auto & s : sync) s.Open();
								return;
							} catch (...) {
								Memory::AcquireRootLock();
								ErrorSet(ectx, Errores::ErrorOutOfMemory);
								Memory::ReleaseRootLock();
								for (auto & s : sync) s.Open();
								return;
							}
						}
					}), i);
				}
				pool->SubmitTasksE(tasks.GetBuffer(), tasks.GetLength(), ectx);
				if (ErrorTest(ectx)) {
					for (auto & s : sync) s.Open();
					pool->Wait();
					ErrorThrow(ectx);
				}
				sync[0].Open();
				pool->Wait();
				ErrorThrow(ectx);
			} else while (num_blocks) {
				uint32 header[3];
				if (source->Read(&header, sizeof(header)) != sizeof(header)) throw InvalidFormatException();
				auto com = source->ReadBlock(header[0]);
				auto data = Decompress(com->GetBuffer(), com->GetLength(), header[2]);
				if (data->GetLength() != header[1]) throw InvalidFormatException();
				dest->WriteBlock(data);
				num_blocks--;
			}
		}
		
		class DecompressionStream : public Stream
		{
			struct _segment
			{
				uint64 compressed_offset, real_offset;
				uint32 compressed_length, real_length;
				MethodChain compression;
			};
		private:
			oref<Stream> _inner;
			oref<DataBlock> _segment_view;
			array<_segment> _segments;
			uint64 _pointer, _size;
			uint32 _current_segment;
		private:
			uint32 _get_segment_for_offset(uint64 offs) const noexcept
			{
				if (!_segments.GetLength()) return 0;
				for (uint i = 0; i < _segments.GetLength() - 1; i++) if (_segments[i + 1].real_offset > offs) return i;
				return _segments.GetLength() - 1;
			}
		public:
			DecompressionStream(Stream * inner) : _inner(inner), _pointer(0), _size(0), _current_segment(0)
			{
				if (!inner) throw InvalidArgumentException();
				uint32 block_count;
				_inner->Seek(0, SeekOrigin::Begin);
				if (_inner->Read(&block_count, 4) != 4) throw InvalidFormatException();
				_segments.SetLength(block_count);
				for (uint i = 0; i < _segments.GetLength(); i++) {
					auto & s = _segments[i];
					uint32 header[3];
					if (_inner->Read(&header, sizeof(header)) != sizeof(header)) throw InvalidFormatException();
					s.compressed_offset = _inner->Seek(0, SeekOrigin::Current);
					s.compressed_length = header[0];
					s.real_offset = _size;
					s.real_length = header[1];
					s.compression = header[2];
					_size += s.real_length;
					_inner->Seek(s.compressed_length, SeekOrigin::Current);
				}
			}
			virtual ~DecompressionStream(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Flumen decompressionis"; ESSE_TRY_OUTRO(string()) }
			virtual uintptr ReadE(void * data, uintptr size, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					auto dest = reinterpret_cast<uint8 *>(data);
					uintptr read = 0;
					Seek(_pointer, SeekOrigin::Begin);
					while (size) {
						uintptr available = _segments[_current_segment].real_offset + _segments[_current_segment].real_length - _pointer;
						if (!available) return read;
						if (available > size) available = size;
						uintptr segment_position = _pointer - _segments[_current_segment].real_offset;
						Memory::MemoryCopy(dest, _segment_view->GetBuffer() + segment_position, available);
						size -= available; read += available; dest += available;
						Seek(available, SeekOrigin::Current);
					}
					return read;
				ESSE_TRY_OUTRO(0)
			}
			virtual uintptr WriteE(const void * data, uintptr size, ErrorContext & ectx) noexcept override { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			virtual uint64 SeekE(int64 position, SeekOrigin org, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					uint64 newpos = position;
					if (org == SeekOrigin::Current) newpos += _pointer;
					else if (org == SeekOrigin::End) newpos += _size;
					else if (org != SeekOrigin::Begin) throw InvalidArgumentException();
					if (newpos > _size) throw InvalidArgumentException();
					auto cs = _current_segment;
					auto ns = _get_segment_for_offset(newpos);
					if (cs != ns || !_segment_view) {
						_inner->Seek(_segments[ns].compressed_offset, SeekOrigin::Begin);
						auto data = _inner->ReadBlock(_segments[ns].compressed_length);
						_segment_view = Decompress(data->GetBuffer(), data->GetLength(), _segments[ns].compression);
						_current_segment = ns;
					}
					_pointer = newpos;
					return _pointer;
				ESSE_TRY_OUTRO(0)
			}
			virtual uint64 GetLengthE(ErrorContext & ectx) noexcept override { return _size; }
			virtual void SetLengthE(const uint64 & length, ErrorContext & ectx) noexcept override { ErrorSet(ectx, Errores::ErrorNotImplemented); }
			virtual void FlushE(ErrorContext & ectx) noexcept override {}
		};
		oref<Stream> CreateDecompressionStream(Stream * compressed) { return oref<Stream>::CreateOwned(new DecompressionStream(compressed)); }
	}
}