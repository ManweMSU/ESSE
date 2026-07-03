#include "ImprimitorPDF.h"

namespace ESSE
{
	namespace Graphica
	{
		namespace PDF
		{
			class EncoderContext : public IEncoderContext
			{
				class Serializer
				{
					oref<Stream> _stream;
					uintptr _local_offset;
					uintptr _global_offset;
					DataBlock _buffer;
				public:
					Serializer(Stream * stream) : _buffer(0x10000), _local_offset(0), _global_offset(0), _stream(stream) { _buffer.SetLength(0x10000); }
					int GetPosition(void) const noexcept { return _global_offset; }
					void Flush(void)
					{
						if (_local_offset) _stream->Write(_buffer.GetBuffer(), _local_offset);
						_local_offset = 0;
					}
					void WriteRaw(const void * data, uintptr length)
					{
						uintptr pending = length;
						uintptr written = 0;
						while (pending) {
							uintptr write_now = min(_buffer.GetLength() - _local_offset, pending);
							if (write_now == 0) { Flush(); continue; }
							Memory::MemoryCopy(_buffer.GetBuffer() + _local_offset, reinterpret_cast<const uint8 *>(data) + written, write_now);
							_local_offset += write_now;
							_global_offset += write_now;
							written += write_now;
							pending -= write_now;
						}
					}
					void WriteInteger(int value)
					{
						ucs1_string data = string(value);
						WriteRaw(data.GetData(), data.GetLength());
					}
					void WriteLF(void) { WriteRaw("\xd\xa", 2); }
					void WriteSignature(void) { WriteRaw("%PDF-1.3", 8); WriteLF(); }
					void WriteTrailer(int numobj, const uintptr * objoffs, int root, int meta)
					{
						uintptr xref_offs = GetPosition();
						WriteRaw("xref", 4); WriteLF();
						WriteInteger(0); WriteRaw(" ", 1); WriteInteger(numobj + 1); WriteLF();
						WriteRaw("0000000000 65535 f", 18); WriteLF();
						for (int i = 0; i < numobj; i++) {
							ucs1_string data = string(uint(objoffs[i]), DecimalBase, 10);
							WriteRaw(data.GetData(), data.GetLength());
							WriteRaw(" 00000 n", 8); WriteLF();
						}
						WriteRaw("trailer", 7); WriteLF();
						WriteRaw("<< /Size ", 9); WriteInteger(numobj + 1);
						WriteRaw(" /Root ", 7); WriteInteger(root); WriteRaw(" 0 R ", 5);
						if (meta) { WriteRaw("/Info ", 6); WriteInteger(meta); WriteRaw(" 0 R ", 5); }
						WriteRaw(">>", 2); WriteLF();
						WriteRaw("startxref", 9); WriteLF();
						WriteInteger(xref_offs); WriteLF();
						WriteRaw("%%EOF", 5); WriteLF();
					}
					void WriteString(const string & text)
					{
						ucs2_string data = text;
						WriteRaw("(\xFE\xFF", 3);
						for (uintptr i = 0; i < data.GetLength(); i++) {
							char oct[8];
							oct[0] = oct[4] = '\\';
							uint char_hi = (data[i] >> 8U) & 0xFFU;
							uint char_lo = (data[i]) & 0xFFU;
							oct[1] = OctalBase[(char_hi >> 6) & 3];
							oct[2] = OctalBase[(char_hi >> 3) & 7];
							oct[3] = OctalBase[(char_hi) & 7];
							oct[5] = OctalBase[(char_lo >> 6) & 3];
							oct[6] = OctalBase[(char_lo >> 3) & 7];
							oct[7] = OctalBase[(char_lo) & 7];
							WriteRaw(oct, 8);
						}
						WriteRaw(")", 1);
					}
					void WriteWord(const string & text)
					{
						ucs1_string data = text;
						WriteRaw(data.GetData(), data.GetLength());
					}
				};
				class BitWriter
				{
					DataBlock & _data;
					uint8 _buffer;
					int _used;
				public:
					BitWriter(DataBlock & data) noexcept : _data(data), _buffer(0), _used(0) {}
					void Flush(void) { if (_used) _data << _buffer; _buffer = _used = 0; }
					void Write(uint data, int bits)
					{
						int pending = bits;
						while (pending) {
							if (_used == 8) { Flush(); continue; }
							int wrc = min(pending, 8 - _used);
							_buffer |= (data >> (pending - wrc)) << (8 - wrc - _used);
							data &= (1 << (pending - wrc)) - 1;
							_used += wrc;
							pending -= wrc;
						}
					}
				};
				class LZWTable
				{
					struct LZWCode
					{
						uint32 emit;
						uint32 next_index;
					};
				private:
					BitWriter & _wri;
					const uint8 * _data;
					uintptr _length;
					uint _next_alloc;
					uint _bitlength;
					uint _index_alloc;
					LZWCode _root;
					array<LZWCode> _lzw_alloc;
				public:
					LZWTable(BitWriter & wri, const uint8 * data, uintptr length) : _wri(wri), _data(data), _length(length)
					{
						_lzw_alloc.SetLength(0x100000);
						_root.emit = 0x100;
						_root.next_index = 0;
						Reset();
						_wri.Write(0x100, _bitlength);
					}
					void Reset(void) noexcept
					{
						_bitlength = 9;
						_next_alloc = 258;
						Memory::ZeroMemory(_lzw_alloc.GetBuffer(), sizeof(LZWCode) * _lzw_alloc.GetLength());
						_index_alloc = 0x100;
						for (int i = 0; i < 0x100; i++) {
							_lzw_alloc[i].emit = i;
							_lzw_alloc[i].next_index = _index_alloc;
							_index_alloc += 0x100;
						}
					}
					bool Step(void)
					{
						if (_length) {
							uintptr i = 0;
							LZWCode * current = &_root;
							while (i < _length && _lzw_alloc[current->next_index + _data[i]].next_index) {
								current = &_lzw_alloc[current->next_index + _data[i]];
								i++;
							}
							_wri.Write(current->emit, _bitlength);
							if (_index_alloc >= _lzw_alloc.GetLength()) throw OutOfMemoryException();
							_lzw_alloc[current->next_index + _data[i]].emit = _next_alloc;
							_lzw_alloc[current->next_index + _data[i]].next_index = _index_alloc;
							_index_alloc += 0x100;
							_next_alloc++;
							if ((1 << _bitlength) & _next_alloc) _bitlength++;
							if (_next_alloc == 0xFFF) {
								_wri.Write(0x100, _bitlength);
								Reset();
							}
							_data += i;
							_length -= i;
							return true;
						} else {
							_wri.Write(257, _bitlength);
							return false;
						}
					}
				};
			private:
				Serializer _serializer;
				array<uintptr> _objtable;
				array<int> _pagetable;
				int _rootobj, _metaobj;
				Dictionary<string, string> _metadata;
			private:
				void ReserveIndex(void) { _objtable << 0; }
				int BeginObject(int enf_index = -1)
				{
					int index;
					if (enf_index > 0) {
						_objtable[enf_index - 1] = _serializer.GetPosition();
						index = enf_index;
					} else {
						_objtable << _serializer.GetPosition();
						index = _objtable.GetLength();
					}
					_serializer.WriteInteger(index); _serializer.WriteRaw(" 0 obj", 6); _serializer.WriteLF();
					return index;
				}
				void EndObject(void) { _serializer.WriteRaw("endobj", 6); _serializer.WriteLF(); }
				void Encode85(const uint8 * data, uintptr quant, uint8 * output, uintptr * output_length)
				{
					uint num;
					if (quant == 4) num = (uint(data[0]) << 24) | (uint(data[1]) << 16) | (uint(data[2]) << 8) | uint(data[3]);
					else if (quant == 3) num = (uint(data[0]) << 24) | (uint(data[1]) << 16) | (uint(data[2]) << 8);
					else if (quant == 2) num = (uint(data[0]) << 24) | (uint(data[1]) << 16);
					else num = (uint(data[0]) << 24);
					if (num == 0 && quant == 4) {
						*output_length = 1;
						output[0] = 'z';
					} else {
						*output_length = quant + 1;
						output[4] = 33 + num % 85; num /= 85;
						output[3] = 33 + num % 85; num /= 85;
						output[2] = 33 + num % 85; num /= 85;
						output[1] = 33 + num % 85; num /= 85;
						output[0] = 33 + num % 85;
					}
				}
				oref<DataBlock> TranscodeASCII85(const uint8 * data, uintptr length)
				{
					auto result = owrap(new DataBlock(0x40000));
					for (uintptr i = 0; i < length; i += 4) {
						uintptr rem = length - i;
						uintptr quant = min<uintptr>(rem, 4);
						uint8 enc[5];
						Encode85(data + i, quant, enc, &rem);
						result->Append(enc, rem);
					}
					result->Append('~');
					result->Append('>');
					return result;
				}
				oref<DataBlock> TranscodeRLE8(const uint8 * data, uintptr length)
				{
					auto result = owrap(new DataBlock(0x40000));
					uintptr i = 0;
					while (i < length) {
						if (i < length - 1 && data[i] == data[i + 1]) {
							uintptr s = i;
							while (i < length && i - s < 128 && data[i] == data[s]) i++;
							uintptr cnt = i - s;
							result->Append(257 - cnt);
							result->Append(data[s]);
						} else {
							uintptr s = i; i++;
							while (i < length && i - s < 128 && data[i] != data[i - 1]) i++;
							result->Append(i - s - 1);
							for (uintptr j = s; j < i; j++) result->Append(data[j]);
						}
					}
					result->Append(0x80);
					return result;
				}
				oref<DataBlock> TranscodeLZW(const uint8 * data, int length)
				{
					auto result = owrap(new DataBlock(0x40000));
					BitWriter wri(*result);
					LZWTable lzw(wri, data, length);
					while (lzw.Step());
					wri.Flush();
					return result;
				}
				oref<DataBlock> EncodeImage(Picturae::Picture * data, uint flags, string & filter)
				{
					Picturae::PictureDesc desc = data->GetDesc();
					if (flags & PageFlagColorFull) desc.format = Picturae::PixelFormat::R8G8B8;
					else if (flags & PageFlagColorGrayscale) desc.format = Picturae::PixelFormat::R8;
					else desc.format = Picturae::PixelFormat::R1;
					desc.alpha_mode = Picturae::AlphaMode::Undefined;
					desc.origin = Picturae::ScanOrigin::TopLeft;
					desc.stride = (desc.width * Picturae::GetBitsPerPixel(desc.format) + 7U) >> 3U;
					desc.palette_size = 0;
					auto bitmap = owrap(new Picturae::Picture(desc, Picturae::PictureInit::AllocateZeroed));
					Picturae::BlockTransfer(bitmap->GetDesc(), data->GetDesc());
					auto bitmap_data = reinterpret_cast<uint8 *>(bitmap->GetDesc().data);
					auto bitmap_length = uintptr(bitmap->GetDesc().height) * uintptr(bitmap->GetDesc().stride);
					oref<DataBlock> result;
					if (flags & PageFlagCompress) {
						if (flags & PageFlagColorFull) {
							filter = FormatString(U"[ /ASCII85Decode /LZWDecode ] /DecodeParms [ null << /Predictor 2 /Colors 3 /Columns %0 >> ]", data->GetDesc().width);
							for (uint j = 0; j < bitmap->GetDesc().height; j++) {
								auto base = bitmap_data + j * bitmap->GetDesc().stride;
								for (uint i = 3 * bitmap->GetDesc().width - 1; i > 2; i--) base[i] -= base[i - 3];
							}
							result = TranscodeLZW(bitmap_data, bitmap_length);
							bitmap.Clear();
							result = TranscodeASCII85(result->GetBuffer(), result->GetLength());
						} else if (flags & PageFlagColorGrayscale) {
							filter = FormatString(U"[ /ASCII85Decode /LZWDecode ] /DecodeParms [ null << /Predictor 2 /Columns %0 >> ]", data->GetDesc().width);
							for (uint j = 0; j < bitmap->GetDesc().height; j++) {
								auto base = bitmap_data + j * bitmap->GetDesc().stride;
								for (uint i = bitmap->GetDesc().width - 1; i > 0; i--) base[i] -= base[i - 1];
							}
							result = TranscodeLZW(bitmap_data, bitmap_length);
							bitmap.Clear();
							result = TranscodeASCII85(result->GetBuffer(), result->GetLength());
						} else {
							filter = U"[ /ASCII85Decode /RunLengthDecode ]";
							result = TranscodeRLE8(bitmap_data, bitmap_length);
							bitmap.Clear();
							result = TranscodeASCII85(result->GetBuffer(), result->GetLength());
						}
					} else {
						filter = U"/ASCII85Decode";
						result = TranscodeASCII85(bitmap_data, bitmap_length);
					}
					return result;
				}
			public:
				EncoderContext(Stream * stream) : _serializer(stream), _objtable(0x100), _pagetable(0x40), _rootobj(0), _metaobj(0)
				{
					_serializer.WriteSignature();
					ReserveIndex(); ReserveIndex();
				}
				virtual ~EncoderContext(void) override {}
				virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"PDF Encoder Context"; ESSE_TRY_OUTRO(string()) }
				virtual bool AddPage(uint physical_width, uint physical_height, Picturae::Picture * data, uint flags) noexcept override
				{
					try {
						uint uw = physical_width * 72 / 254;
						uint uh = physical_height * 72 / 254;
						int resource, contents;
						if (data) {
							resource = BeginObject();
							_serializer.WriteRaw("<< /Type /XObject /Subtype /Image /Width ", 41);
							_serializer.WriteInteger(data->GetDesc().width);
							_serializer.WriteRaw(" /Height ", 9);
							_serializer.WriteInteger(data->GetDesc().height);
							if (flags & PageFlagColorFull) _serializer.WriteRaw(" /ColorSpace /DeviceRGB ", 24);
							else _serializer.WriteRaw(" /ColorSpace /DeviceGray ", 25);
							if (flags & PageFlagColorFull) _serializer.WriteRaw("/BitsPerComponent 8 ", 20);
							else if (flags & PageFlagColorGrayscale) _serializer.WriteRaw("/BitsPerComponent 8 ", 20);
							else _serializer.WriteRaw("/BitsPerComponent 1 ", 20);
							if (flags & PageFlagInterpolate) _serializer.WriteRaw("/Interpolate true ", 18);
							else _serializer.WriteRaw("/Interpolate false ", 19);
							string filter;
							auto bitmap = EncodeImage(data, flags, filter);
							_serializer.WriteRaw("/Length ", 8);
							_serializer.WriteInteger(bitmap->GetLength());
							_serializer.WriteRaw(" /Filter ", 9);
							_serializer.WriteWord(filter);
							_serializer.WriteRaw(" >>", 3);
							_serializer.WriteLF();
							_serializer.WriteRaw("stream", 6);
							_serializer.WriteLF();
							_serializer.WriteRaw(bitmap->GetBuffer(), bitmap->GetLength());
							_serializer.WriteLF();
							_serializer.WriteRaw("endstream", 9);
							_serializer.WriteLF();
							EndObject();
							contents = BeginObject();
							dynamic_string_ucs4 cmd;
							cmd << U"q " << string(uw) << U" 0 0 " << string(uh) << U" 0 0 cm /IM0 Do Q";
							ucs1_string cmd_acsii = cmd.ToString();
							_serializer.WriteRaw("<< /Length ", 11);
							_serializer.WriteInteger(cmd_acsii.GetLength());
							_serializer.WriteRaw(" >>", 3);
							_serializer.WriteLF();
							_serializer.WriteRaw("stream", 6);
							_serializer.WriteLF();
							_serializer.WriteRaw(cmd_acsii.GetData(), cmd_acsii.GetLength());
							_serializer.WriteLF();
							_serializer.WriteRaw("endstream", 9);
							_serializer.WriteLF();
							EndObject();
						}
						_pagetable << BeginObject();
						_serializer.WriteRaw("<< /Type /Page /Parent 2 0 R ", 29);
						if (data) {
							_serializer.WriteRaw("/Resources << /ProcSet [ /PDF /ImageB /ImageC /ImageI ] /XObject << /IM0 ", 73);
							_serializer.WriteInteger(resource);
							_serializer.WriteRaw(" 0 R >> >> /Contents ", 21);
							_serializer.WriteInteger(contents);
							_serializer.WriteRaw(" 0 R ", 5);
						}
						_serializer.WriteRaw("/MediaBox [0 0 ", 15);
						_serializer.WriteInteger(uw);
						_serializer.WriteRaw(" ", 1);
						_serializer.WriteInteger(uh);
						_serializer.WriteRaw("] >>", 4);
						_serializer.WriteLF();
						EndObject();
						return true;
					} catch (...) { return false; }
				}
				virtual bool SetMetadata(MetadataKey key, const string & value) noexcept override
				{
					try {
						if (key == MetadataKey::CreatorSoftware) _metadata.Update(U"Creator", value);
						else if (key == MetadataKey::EncoderSoftware) _metadata.Update(U"Producer", value);
						else if (key == MetadataKey::Author) _metadata.Update(U"Author", value);
						else if (key == MetadataKey::Title) _metadata.Update(U"Title", value);
						else if (key == MetadataKey::Subject) _metadata.Update(U"Subject", value);
						else if (key == MetadataKey::Keywords) _metadata.Update(U"Keywords", value);
						else throw InvalidArgumentException();
						return true;
					} catch (...) { return false; }
				}
				virtual bool SetMetadata(MetadataKey key, const Time & value) noexcept override
				{
					try {
						uint y, m, d;
						uint H, M, S;
						value.GetDate(y, m, d);
						if (y > 9999) throw InvalidArgumentException();
						H = value.GetHour();
						M = value.GetMinute();
						S = value.GetSecond();
						string enc = FormatString(U"D:%0%1%2%3%4%5Z00\'00\'", string(y, DecimalBase, 4), string(m, DecimalBase, 2),
							string(d, DecimalBase, 2), string(H, DecimalBase, 2), string(M, DecimalBase, 2), string(S, DecimalBase, 2));
						if (key == MetadataKey::CreationDate) _metadata.Update(U"CreationDate", enc);
						else if (key == MetadataKey::AlternationDate) _metadata.Update(U"ModDate", enc);
						else throw InvalidArgumentException();
						return true;
					} catch (...) { return false; }
				}
				virtual bool FinalizeDocument(void) noexcept override
				{
					try {
						BeginObject(2);
						_serializer.WriteRaw("<< /Type /Pages /Count ", 23);
						_serializer.WriteInteger(_pagetable.GetLength());
						_serializer.WriteRaw(" /Kids [ ", 9);
						for (auto & p : _pagetable) {
							_serializer.WriteInteger(p);
							_serializer.WriteRaw(" 0 R ", 5);
						}
						_serializer.WriteRaw("] >>", 4);
						_serializer.WriteLF();
						EndObject();
						_rootobj = BeginObject(1);
						_serializer.WriteRaw("<< /Type /Catalog /Pages 2 0 R >>", 33); _serializer.WriteLF();
						EndObject();
						if (!_metadata.IsEmpty()) {
							_metaobj = BeginObject();
							_serializer.WriteRaw("<< ", 3);
							for (auto & m : _metadata) {
								_serializer.WriteRaw("/", 1);
								_serializer.WriteWord(m.key);
								_serializer.WriteRaw(" ", 1);
								_serializer.WriteString(m.value);
								_serializer.WriteRaw(" ", 1);
							}
							_serializer.WriteRaw(">>", 2);
							_serializer.WriteLF();
							EndObject();
						}
						_serializer.WriteTrailer(_objtable.GetLength(), _objtable.GetBuffer(), _rootobj, _metaobj);
						_serializer.Flush();
						return true;
					} catch (...) { return false; }
				}
			};
			oref<IEncoderContext> CreateEncoder(Stream * stream, ErrorContext & ectx) noexcept { ESSE_TRY_INTRO return CreateEncoder(stream); ESSE_TRY_OUTRO(0) }
			oref<IEncoderContext> CreateEncoder(Stream * stream) { return oref<IEncoderContext>::CreateOwned(new EncoderContext(stream)); }
		}
	}
}