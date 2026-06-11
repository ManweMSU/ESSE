#include "BlockCompression.h"
#include "BitStream.h"
#include <Cor/Classes/CorVolume.hxx>

#include <stdio.h>

namespace ESSE
{
	namespace Compression
	{
		namespace RLE
		{
			ESSE_PACKED_STRUCTURE(RleWord8)
				static constexpr uint64 MaxRepeats = 0x80;
				union {
					struct { uint8 repeat_count : 7; uint8 compressed : 1; };
					uint8 data;
				};
				bool friend operator == (const RleWord8 & a, const RleWord8 & b) { return a.data == b.data; }
				bool friend operator != (const RleWord8 & a, const RleWord8 & b) { return a.data != b.data; }
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(RleWord16)
				static constexpr uint64 MaxRepeats = 0x8000;
				union {
					struct { uint16 repeat_count : 15; uint16 compressed : 1; };
					uint16 data;
				};
				bool friend operator == (const RleWord16 & a, const RleWord16 & b) { return a.data == b.data; }
				bool friend operator != (const RleWord16 & a, const RleWord16 & b) { return a.data != b.data; }
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(RleWord32)
				static constexpr uint64 MaxRepeats = 0x80000000;
				union {
					struct { uint32 repeat_count : 31; uint32 compressed : 1; };
					uint32 data;
				};
				bool friend operator == (const RleWord32 & a, const RleWord32 & b) { return a.data == b.data; }
				bool friend operator != (const RleWord32 & a, const RleWord32 & b) { return a.data != b.data; }
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(RleWord64)
				static constexpr uint64 MaxRepeats = 0x8000000000000000;
				union {
					struct { uint64 repeat_count : 63; uint64 compressed : 1; };
					uint64 data;
				};
				bool friend operator == (const RleWord64 & a, const RleWord64 & b) { return a.data == b.data; }
				bool friend operator != (const RleWord64 & a, const RleWord64 & b) { return a.data != b.data; }
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(RleWord128)
				static constexpr uint64 MaxRepeats = 0xFFFFFFFFFFFFFFFF;
				union {
					struct { uint64 repeat_count : 64; uint64 unused : 63; uint64 compressed : 1; };
					struct { uint64 lo; uint64 hi; } data;
				};
				bool friend operator == (const RleWord128 & a, const RleWord128 & b) { return a.data.lo == b.data.lo && a.data.hi == b.data.hi; }
				bool friend operator != (const RleWord128 & a, const RleWord128 & b) { return a.data.lo != b.data.lo || a.data.hi != b.data.hi; }
			ESSE_END_PACKED_STRUCTURE
		}
		namespace Huffman
		{
			struct EncodeWord
			{
				uint value;
				uint repeats;
				uint code_offset;
				uint code_bitlength;
			};
			struct EncodeNode
			{
				uint weight;
				EncodeNode * on0, * on1;
				EncodeWord * word;
			};
			struct DecodeNode
			{
				uint value;
				DecodeNode * on0, * on1;
			};
			struct EncodeState
			{
				uint bitmode, words_used;
				array<uint8> code_heap;
				array<EncodeWord> words;
				array<EncodeNode> nodes;
				EncodeState(void) noexcept : bitmode(0), words_used(0), code_heap(0x1000), words(1), nodes(1) {}
				void Initialize(uint num_bits)
				{
					bitmode = num_bits;
					words.SetLength(1U << num_bits);
					nodes.SetLength(1U << (num_bits + 1));
					Memory::ZeroMemory(words.GetBuffer(), words.GetLength() * sizeof(EncodeWord));
					Memory::ZeroMemory(nodes.GetBuffer(), nodes.GetLength() * sizeof(EncodeNode));
				}
				void AddStatistic(uint word) noexcept { words[word].repeats++; }
				EncodeNode * BuildTree(void)
				{
					uint nodes_used = 0, nodes_inproc = 0;
					Set<uint> cstate;
					for (uint i = 0; i < words.GetLength(); i++) if (words[i].repeats) {
						words[i].value = i;
						nodes[nodes_used].weight = words[i].repeats;
						nodes[nodes_used].word = &words[i];
						cstate.AddElement(nodes_used);
						nodes_used++; nodes_inproc++;
					}
					words_used = nodes_used;
					while (nodes_inproc > 1) {
						int min1 = -1, min2 = -1;
						uint w1 = 0, w2 = 0;
						for (auto i : cstate) {
							if (min1 < 0) { min1 = i; w1 = nodes[i].weight; }
							else if (min2 < 0) { min2 = i; w2 = nodes[i].weight; }
							else if (nodes[i].weight < w1) {
								if (w1 < w2) { min2 = min1; w2 = w1; }
								min1 = i; w1 = nodes[i].weight;
							} else if (nodes[i].weight < w2) {
								if (w2 < w1) { min1 = min2; w1 = w2; }
								min2 = i; w2 = nodes[i].weight;
							}
						}
						nodes[nodes_used].on0 = &nodes[min1];
						nodes[nodes_used].on1 = &nodes[min2];
						nodes[nodes_used].weight = w1 + w2;
						cstate.RemoveElement(min1);
						cstate.RemoveElement(min2);
						cstate.AddElement(nodes_used);
						nodes_inproc--; nodes_used++;
					}
					return &nodes[cstate.GetRoot()->GetValue()];
				}
				void BuildCodes(EncodeNode * node, uint8 * code, uint bitlength)
				{
					if (node->on0) { SetBit(code, bitlength, 0); BuildCodes(node->on0, code, bitlength + 1); }
					if (node->on1) { SetBit(code, bitlength, 1); BuildCodes(node->on1, code, bitlength + 1); }
					SetBit(code, bitlength, 0);
					if (node->word) {
						node->word->code_offset = code_heap.GetLength();
						node->word->code_bitlength = bitlength;
						code_heap.Append(code, (bitlength + 7U) >> 3U);
					}
				}
				void BuildCodes(EncodeNode * root)
				{
					uint8 buffer[512];
					Memory::ZeroMemory(&buffer, sizeof(buffer));
					BuildCodes(root, buffer, 0);
				}
				void EncodeTreeNode(BitStream & stream, EncodeNode * node)
				{
					if (node->on0) {
						EncodeTreeNode(stream, node->on0);
						EncodeTreeNode(stream, node->on1);
					} else if (node->word) {
						uint depth = node->word->code_bitlength ? node->word->code_bitlength - 1 : 0;
						stream.WriteBits(&node->word->value, bitmode);
						stream.WriteBits(&depth, bitmode);
					}
				}
				void EncodeTree(BitStream & stream, EncodeNode * root)
				{
					stream.WriteBits(&words_used, bitmode);
					EncodeTreeNode(stream, root);
				}
			};
			struct DecodeState
			{
				uint bitmode, nodes_used;
				DecodeNode * root;
				array<DecodeNode> nodes;
				DecodeState(void) noexcept : bitmode(0), nodes_used(0), root(0), nodes(1) {}
				void Initialize(uint num_bits)
				{
					bitmode = num_bits;
					nodes.SetLength(1U << (num_bits + 1));
					Memory::ZeroMemory(nodes.GetBuffer(), nodes.GetLength() * sizeof(DecodeNode));
				}
				static uint GetIndeterminationLevel(const DecodeNode * node) noexcept
				{
					if (!node->on0 && !node->on1) return 0;
					else if (node->on0 && node->on1) return GetIndeterminationLevel(node->on0) + GetIndeterminationLevel(node->on1);
					else return 1;
				}
				void AddNode(DecodeNode ** root, uint value, uint depth, uint cdepth) noexcept
				{
					if (!*root) *root = &nodes[nodes_used++];
					if (depth == cdepth) (*root)->value = value; else {
						if (!(*root)->on0 || GetIndeterminationLevel((*root)->on0)) AddNode(&(*root)->on0, value, depth, cdepth + 1);
						else AddNode(&(*root)->on1, value, depth, cdepth + 1);
					}
				}
				void DecodeTree(BitStream & stream) noexcept
				{
					uint codes_used = 0;
					stream.ReadBits(&codes_used, bitmode);
					if (!codes_used) codes_used = 1U << bitmode;
					if (codes_used > 1) for (uint i = 0; i < codes_used; i++) {
						uint value = 0, depth = 0;
						stream.ReadBits(&value, bitmode);
						stream.ReadBits(&depth, bitmode);
						AddNode(&root, value, depth, uint(-1));
					} else {
						uint value = 0, depth = 0;
						stream.ReadBits(&value, bitmode);
						stream.ReadBits(&depth, bitmode);
						root = &nodes[nodes_used++];
						root->value = value;
					}
				}
				bool TreeValidate(DecodeNode * node) noexcept
				{
					if (node->on0 && node->on1) return TreeValidate(node->on0) && TreeValidate(node->on1);
					else if (node->on0 || node->on1) return false;
					else return true;
				}
				bool TreeValidate(void) noexcept { return TreeValidate(root); }
			};
		}
		namespace LZW
		{
			struct Word
			{
				uint next_byte;
				int previous_word_index;
				int next_hash_index;
			};
			struct State
			{
				uint bitmode, words_used;
				uint hash_bit_size, hash_mask;
				array<Word> dictionary;
				array<int> hash_table;
				uint8 previous_byte;
				int previous_index;
				uint previous_hash;
				State(void) noexcept : bitmode(0), words_used(0), hash_bit_size(0), hash_mask(0), dictionary(1), hash_table(1) {}
				uint EvaluateHash(const void * data, uintptr length) noexcept
				{
					uint result = 0;
					for (uintptr i = 0; i < length; i++) {
						uint byte = (reinterpret_cast<const uint8 *>(data)[i] + 137 * i) & 0xFF;
						uint N = i * 8 / hash_bit_size;
						byte <<= i * 8 - N * hash_bit_size;
						result ^= (byte & hash_mask) | (byte >> hash_bit_size);
					}
					return result;
				}
				bool WordCompare(const uint8 * data, uintptr length, int wi) noexcept
				{
					int w = wi, p = length - 1;
					while (true) {
						if (p < 0 && w < 0) return true;
						if (p < 0 || w < 0) return false;
						if (data[p] != dictionary[w].next_byte) return false;
						w = dictionary[w].previous_word_index; p--;
					}
				}
				void DictionaryReset(void) noexcept
				{
					previous_byte = 0;
					previous_index = -1;
					previous_hash = 0;
					for (uint i = 0; i < 256; i++) {
						dictionary[i].next_byte = i;
						dictionary[i].previous_word_index = -1;
						dictionary[i].next_hash_index = -1;
					}
					words_used = 256;
					if (hash_bit_size) {
						for (auto & h : hash_table) h = -1;
						for (uint i = 0; i < words_used; i++) {
							auto h = EvaluateHash(&dictionary[i].next_byte, 1);
							hash_table[h] = i;
						}
					}
				}
				void Initialize(uint bits, uint hash_size)
				{
					dictionary.SetLength(1U << bits);
					bitmode = bits;
					hash_bit_size = hash_size;
					if (hash_bit_size) {
						hash_mask = (1U << hash_bit_size) - 1U;
						hash_table.SetLength(1U << hash_bit_size);
					} else hash_mask = 0;
					DictionaryReset();
				}
				void AddWord(uint8 byte, int prefix, uint hash, bool grow)
				{
					auto new_index = words_used++;
					dictionary[new_index].next_byte = byte;
					dictionary[new_index].previous_word_index = prefix;
					dictionary[new_index].next_hash_index = -1;
					if (hash_bit_size) {
						if (hash_table[hash] < 0) hash_table[hash] = new_index; else {
							uint h = hash_table[hash];
							while (dictionary[h].next_hash_index >= 0) h = dictionary[h].next_hash_index;
							dictionary[h].next_hash_index = new_index;
						}
					}
					if (words_used == dictionary.GetLength()) {
						if (grow) {
							bitmode++;
							dictionary.SetLength(1U << bitmode);
						} else DictionaryReset();
					}
				}
				uintptr Encode(const uint8 * data, uintptr length, uint & write_data, uint & write_bits, bool grow)
				{
					uint hash, word;
					uint runlength = 1;
					while (true) {
						if (runlength > length) break;
						auto local_hash = EvaluateHash(data, runlength);
						auto local_word = hash_table[local_hash];
						while (local_word >= 0 && !WordCompare(data, runlength, local_word)) local_word = dictionary[local_word].next_hash_index;
						if (local_word >= 0 && runlength <= length) { word = local_word; runlength++; } else { hash = local_hash; break; }
					}
					write_data = word;
					write_bits = bitmode;
					auto add_word_byte = previous_byte;
					auto add_word_index = previous_index;
					auto add_word_hash = previous_hash;
					previous_byte = data[runlength - 1];
					previous_index = word;
					previous_hash = hash;
					if (add_word_index >= 0) AddWord(add_word_byte, add_word_index, add_word_hash, grow);
					return runlength - 1;
				}
				uint SerializeWord(DataBlock & dest, int word)
				{
					if (word < 0 || word >= words_used) throw InvalidFormatException();
					auto & w = dictionary[word];
					if (w.previous_word_index >= 0) {
						auto result = SerializeWord(dest, w.previous_word_index);
						dest.Append(w.next_byte);
						return result;
					} else {
						dest.Append(w.next_byte);
						return w.next_byte;
					}
				}
				void Decode(DataBlock & dest, uint word, bool grow)
				{
					auto byte = SerializeWord(dest, word);
					auto add_word_index = previous_index;
					previous_index = word;
					if (add_word_index >= 0) AddWord(byte, add_word_index, 0, grow);
				}
			};
		}
		oref<DataBlock> HuffmanCompress(const void * data, uintptr length)
		{
			if (!length) return owrap(new DataBlock(1));
			Huffman::EncodeState state;
			state.Initialize(8);
			for (uintptr i = 0; i < length; i++) state.AddStatistic(reinterpret_cast<const uint8 *>(data)[i]);
			auto tree = state.BuildTree();
			state.BuildCodes(tree);
			BitStream stream;
			stream.InitializeForWriting(length >> 1U);
			stream.WriteBits(&length, 32);
			state.EncodeTree(stream, tree);
			for (uintptr i = 0; i < length; i++) {
				auto & word = state.words[reinterpret_cast<const uint8 *>(data)[i]];
				stream.WriteBits(state.code_heap.GetBuffer() + word.code_offset, word.code_bitlength);
			}
			return stream.stream;
		}
		oref<DataBlock> HuffmanDecompress(const void * data, uintptr length)
		{
			auto result = owrap(new DataBlock(1));
			if (!length) return result;
			Huffman::DecodeState state;
			BitStream stream;
			state.Initialize(8);
			stream.InitializeForReading(reinterpret_cast<const uint8 *>(data), length);
			uint32 block_length;
			stream.ReadBits(&block_length, 32);
			result->SetLength(block_length);
			state.DecodeTree(stream);
			if (!state.TreeValidate()) throw InvalidFormatException();
			if (!state.root->on0 && !state.root->on1) for (auto & v : *result) v = state.root->value; else {
				uint position = 0;
				auto current = state.root;
				while (position < result->GetLength()) {
					if (!current->on0) { (*result)[position++] = current->value; current = state.root; }
					else current = stream.ReadBit() ? current->on1 : current->on0;
				}
			}
			return result;
		}
		oref<DataBlock> LempelZivWelchCompress(const void * data, uintptr length)
		{
			LZW::State state;
			state.Initialize(9, 16);
			BitStream stream;
			stream.InitializeForWriting(length);
			uintptr position = 0;
			while (position < length) {
				uint write_data, write_bits;
				position += state.Encode(reinterpret_cast<const uint8 *>(data) + position, length - position, write_data, write_bits, true);
				stream.WriteBits(&write_data, write_bits);
			}
			return stream.stream;
		}
		oref<DataBlock> LempelZivWelchDecompress(const void * data, uintptr length)
		{
			auto result = owrap(new DataBlock(0x10000));
			LZW::State state;
			state.Initialize(9, 0);
			BitStream stream;
			stream.InitializeForReading(reinterpret_cast<const uint8 *>(data), length);
			while (stream.length - stream.position >= state.bitmode) {
				uint word = 0;
				stream.ReadBits(&word, state.bitmode);
				state.Decode(*result, word, true);
			}
			return result;
		}
		oref<DataBlock> FusedLempelZivWelchHuffmanCompress(const void * data, uintptr length, uint bl)
		{
			if (!length) return owrap(new DataBlock(1));
			Huffman::EncodeState state_huffman;
			state_huffman.Initialize(bl);
			LZW::State state_lzw;
			state_lzw.Initialize(bl, bl + 4);
			uintptr position = 0;
			while (position < length) {
				uint write_data = 0, write_bits;
				position += state_lzw.Encode(reinterpret_cast<const uint8 *>(data) + position, length - position, write_data, write_bits, false);
				state_huffman.AddStatistic(write_data);
			}
			auto tree = state_huffman.BuildTree();
			state_huffman.BuildCodes(tree);
			BitStream stream;
			stream.InitializeForWriting(length >> 1U);
			state_lzw.Initialize(bl, bl + 4);
			stream.WriteBits(&length, 32);
			state_huffman.EncodeTree(stream, tree);
			position = 0;
			while (position < length) {
				uint write_data = 0, write_bits;
				position += state_lzw.Encode(reinterpret_cast<const uint8 *>(data) + position, length - position, write_data, write_bits, false);
				auto & word = state_huffman.words[write_data];
				stream.WriteBits(state_huffman.code_heap.GetBuffer() + word.code_offset, word.code_bitlength);
			}
			return stream.stream;
		}
		oref<DataBlock> FusedLempelZivWelchHuffmanDecompress(const void * data, uintptr length, uint bl)
		{
			if (!length) return owrap(new DataBlock(1));
			Huffman::DecodeState state_huffman;
			LZW::State state_lzw;
			BitStream stream;
			state_huffman.Initialize(bl);
			state_lzw.Initialize(bl, 0);
			stream.InitializeForReading(reinterpret_cast<const uint8 *>(data), length);
			uint32 block_length;
			stream.ReadBits(&block_length, 32);
			auto result = owrap(new DataBlock(block_length));
			state_huffman.DecodeTree(stream);
			if (!state_huffman.TreeValidate()) throw InvalidFormatException();
			auto current = state_huffman.root;
			while (result->GetLength() < block_length) {
				if (!current->on0) {
					state_lzw.Decode(*result, current->value, false);
					current = state_huffman.root;
				} else current = stream.ReadBit() ? current->on1 : current->on0;
			}
			return result;
		}
		template<class RleWord> oref<DataBlock> RunLengthCompress(const void * data, uintptr length)
		{
			if (length % sizeof(RleWord)) throw InvalidArgumentException();
			uintptr position = 0;
			uintptr size = length / sizeof(RleWord);
			auto source = reinterpret_cast<const RleWord *>(data);
			auto result = owrap(new DataBlock(length));
			while (position < size) {
				if (size - position < 2 || source[position] != source[position + 1]) {
					uintptr count = 1;
					while (position + count < size && count < RleWord::MaxRepeats && (size - position - count <= 2 || !(source[position + count] == source[position + count + 1] && source[position + count + 1] == source[position + count + 2]))) count++;
					RleWord header;
					Memory::ZeroMemory(&header, sizeof(header));
					header.repeat_count = count;
					result->Append(reinterpret_cast<const uint8 *>(&header), sizeof(header));
					for (uintptr i = 0; i < count; i++) result->Append(reinterpret_cast<const uint8 *>(source + position++), sizeof(header));
				} else {
					uintptr count = 2;
					while (position + count < size && source[position + count] == source[position] && count < RleWord::MaxRepeats) count++;
					RleWord header;
					Memory::ZeroMemory(&header, sizeof(header));
					header.repeat_count = count;
					header.compressed = 1;
					result->Append(reinterpret_cast<const uint8 *>(&header), sizeof(header));
					result->Append(reinterpret_cast<const uint8 *>(source + position), sizeof(header));
					position += count;
				}
			}
			return result;
		}
		template<class RleWord> oref<DataBlock> RunLengthDecompress(const void * data, uintptr length)
		{
			if (length % sizeof(RleWord)) throw InvalidFormatException();
			uintptr position = 0;
			uintptr size = length / sizeof(RleWord);
			auto source = reinterpret_cast<const RleWord *>(data);
			auto result = owrap(new DataBlock(length));
			while (position < size) {
				auto nrep = source[position].repeat_count;
				if (!nrep) nrep = RleWord::MaxRepeats;
				if (source[position].compressed) {
					for (uintptr i = 0; i < nrep; i++) result->Append(reinterpret_cast<const uint8 *>(source + position + 1), sizeof(RleWord));
					position += 2;
				} else {
					position++;
					for (uintptr i = 0; i < nrep; i++) result->Append(reinterpret_cast<const uint8 *>(source + position++), sizeof(RleWord));
				}
			}
			return result;
		}
		oref<DataBlock> Compress(const void * data, uintptr length, Method method)
		{
			if (method == Method::Huffman) return HuffmanCompress(data, length);
			else if (method == Method::LempelZivWelch) return LempelZivWelchCompress(data, length);
			else if (method == Method::FusedLempelZivWelchHuffman9bit) return FusedLempelZivWelchHuffmanCompress(data, length, 9);
			else if (method == Method::FusedLempelZivWelchHuffman10bit) return FusedLempelZivWelchHuffmanCompress(data, length, 10);
			else if (method == Method::FusedLempelZivWelchHuffman11bit) return FusedLempelZivWelchHuffmanCompress(data, length, 11);
			else if (method == Method::FusedLempelZivWelchHuffman12bit) return FusedLempelZivWelchHuffmanCompress(data, length, 12);
			else if (method == Method::RunLengthEncoding8bit) return RunLengthCompress<RLE::RleWord8>(data, length);
			else if (method == Method::RunLengthEncoding16bit) return RunLengthCompress<RLE::RleWord16>(data, length);
			else if (method == Method::RunLengthEncoding32bit) return RunLengthCompress<RLE::RleWord32>(data, length);
			else if (method == Method::RunLengthEncoding64bit) return RunLengthCompress<RLE::RleWord64>(data, length);
			else if (method == Method::RunLengthEncoding128bit) return RunLengthCompress<RLE::RleWord128>(data, length);
			else throw InvalidArgumentException();
		}
		oref<DataBlock> Decompress(const void * data, uintptr length, Method method)
		{
			if (method == Method::Huffman) return HuffmanDecompress(data, length);
			else if (method == Method::LempelZivWelch) return LempelZivWelchDecompress(data, length);
			else if (method == Method::FusedLempelZivWelchHuffman9bit) return FusedLempelZivWelchHuffmanDecompress(data, length, 9);
			else if (method == Method::FusedLempelZivWelchHuffman10bit) return FusedLempelZivWelchHuffmanDecompress(data, length, 10);
			else if (method == Method::FusedLempelZivWelchHuffman11bit) return FusedLempelZivWelchHuffmanDecompress(data, length, 11);
			else if (method == Method::FusedLempelZivWelchHuffman12bit) return FusedLempelZivWelchHuffmanDecompress(data, length, 12);
			else if (method == Method::RunLengthEncoding8bit) return RunLengthDecompress<RLE::RleWord8>(data, length);
			else if (method == Method::RunLengthEncoding16bit) return RunLengthDecompress<RLE::RleWord16>(data, length);
			else if (method == Method::RunLengthEncoding32bit) return RunLengthDecompress<RLE::RleWord32>(data, length);
			else if (method == Method::RunLengthEncoding64bit) return RunLengthDecompress<RLE::RleWord64>(data, length);
			else if (method == Method::RunLengthEncoding128bit) return RunLengthDecompress<RLE::RleWord128>(data, length);
			else throw InvalidArgumentException();
		}
	}
}