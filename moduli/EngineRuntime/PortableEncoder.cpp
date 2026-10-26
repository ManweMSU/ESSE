#include "PortableEncoder.h"

#include <Interfaces/SystemGraphics.h>

using namespace Engine::Streaming;
using namespace Engine::Codec;

namespace Engine
{
	namespace Portable
	{
		class EncoderContext : public IEncoderContext
		{
			class Serializer
			{
				SafePointer<Stream> _stream;
				int _local_offset;
				int _global_offset;
				DataBlock _buffer;
			public:
				Serializer(Stream * stream) : _buffer(0x10000), _local_offset(0), _global_offset(0) { _stream.SetRetain(stream); _buffer.SetLength(0x10000); }
				int GetPosition(void) const noexcept { return _global_offset; }
				void Flush(void)
				{
					if (_local_offset) _stream->Write(_buffer.GetBuffer(), _local_offset);
					_local_offset = 0;
				}
				void WriteRaw(const void * data, int length)
				{
					int pending = length;
					int written = 0;
					while (pending) {
						int write_now = min(_buffer.Length() - _local_offset, pending);
						if (write_now == 0) { Flush(); continue; }
						MemoryCopy(_buffer.GetBuffer() + _local_offset, reinterpret_cast<const uint8 *>(data) + written, write_now);
						_local_offset += write_now;
						_global_offset += write_now;
						written += write_now;
						pending -= write_now;
					}
				}
				void WriteInteger(int value)
				{
					SafePointer<DataBlock> data = string(value).EncodeSequence(Encoding::UTF8, false);
					WriteRaw(data->GetBuffer(), data->Length());
				}
				void WriteLF(void) { WriteRaw("\xd\xa", 2); }
				void WriteSignature(void) { WriteRaw("%PDF-1.3", 8); WriteLF(); }
				void WriteTrailer(int numobj, const int * objoffs, int root, int meta)
				{
					int xref_offs = GetPosition();
					WriteRaw("xref", 4); WriteLF();
					WriteInteger(0); WriteRaw(" ", 1); WriteInteger(numobj + 1); WriteLF();
					WriteRaw("0000000000 65535 f", 18); WriteLF();
					for (int i = 0; i < numobj; i++) {
						SafePointer<DataBlock> data = string(uint(objoffs[i]), DecimalBase, 10).EncodeSequence(Encoding::UTF8, false);
						WriteRaw(data->GetBuffer(), data->Length());
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
					SafePointer<DataBlock> data = text.EncodeSequence(Encoding::UTF16, false);
					WriteRaw("(\xFE\xFF", 3);
					for (int i = 0; i < data->Length(); i += 2) {
						char oct[8];
						oct[0] = oct[4] = '\\';
						string(uint(data->ElementAt(i + 1)), OctalBase, 3).Encode(oct + 1, Encoding::ANSI, false);
						string(uint(data->ElementAt(i + 0)), OctalBase, 3).Encode(oct + 5, Encoding::ANSI, false);
						WriteRaw(oct, 8);
					}
					WriteRaw(")", 1);
				}
				void WriteWord(const string & text)
				{
					SafePointer<DataBlock> data = text.EncodeSequence(Encoding::UTF8, false);
					WriteRaw(data->GetBuffer(), data->Length());
				}
			};
			class BitWriter
			{
				DataBlock & _data;
				uint8 _buffer;
				int _used;
			public:
				BitWriter(DataBlock & data) : _data(data), _buffer(0), _used(0) {}
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
					Array<LZWCode> next = Array<LZWCode>(0x100);
				};
			private:
				BitWriter & _wri;
				const uint8 * _data;
				int _length;
				uint _next_alloc;
				uint _bitlength;
				LZWCode _table;
			public:
				LZWTable(BitWriter & wri, const uint8 * data, int length) : _wri(wri), _data(data), _length(length)
				{
					Reset();
					_wri.Write(0x100, _bitlength);
				}
				void Reset(void)
				{
					_bitlength = 9;
					_next_alloc = 258;
					_table.emit = 0x100;
					_table.next.SetLength(0x100);
					for (int i = 0; i < 0x100; i++) {
						_table.next[i].emit = i;
						_table.next[i].next.Clear();
					}
				}
				bool Step(void)
				{
					if (_length) {
						int i = 0;
						LZWCode * emit = &_table;
						while (i < _length && emit->next.Length() == 0x100 && emit->next[_data[i]].emit != 0x100) {
							emit = &emit->next[_data[i]];
							i++;
						}
						_wri.Write(emit->emit, _bitlength);
						if (emit->next.Length() != 0x100) {
							emit->next.SetLength(0x100);
							for (int j = 0; j < 0x100; j++) {
								emit->next[j].emit = 0x100;
								emit->next[j].next.Clear();
							}
						}
						emit->next[_data[i]].emit = _next_alloc;
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
			Array<int> _objtable;
			Array<int> _pagetable;
			int _rootobj, _metaobj;
			Volumes::Dictionary<string, string> _metadata;
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
					index = _objtable.Length();
				}
				_serializer.WriteInteger(index); _serializer.WriteRaw(" 0 obj", 6); _serializer.WriteLF();
				return index;
			}
			void EndObject(void) { _serializer.WriteRaw("endobj", 6); _serializer.WriteLF(); }
			void Encode85(const uint8 * data, int quant, uint8 * output, int * output_length)
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
			DataBlock * TranscodeASCII85(const uint8 * data, int length)
			{
				SafePointer<DataBlock> result = new DataBlock(0x40000);
				for (int i = 0; i < length; i += 4) {
					int rem = length - i;
					int quant = min(rem, 4);
					uint8 enc[5];
					Encode85(data + i, quant, enc, &rem);
					result->Append(enc, rem);
				}
				result->Append('~');
				result->Append('>');
				result->Retain();
				return result;
			}
			DataBlock * TranscodeRLE8(const uint8 * data, int length)
			{
				SafePointer<DataBlock> result = new DataBlock(0x40000);
				int i = 0;
				while (i < length) {
					if (i < length - 1 && data[i] == data[i + 1]) {
						int s = i;
						while (i < length && i - s < 128 && data[i] == data[s]) i++;
						int cnt = i - s;
						result->Append(257 - cnt);
						result->Append(data[s]);
					} else {
						int s = i; i++;
						while (i < length && i - s < 128 && data[i] != data[i - 1]) i++;
						result->Append(i - s - 1);
						for (int j = s; j < i; j++) result->Append(data[j]);
					}
				}
				result->Append(0x80);
				result->Retain();
				return result;
			}
			DataBlock * TranscodeLZW(const uint8 * data, int length)
			{
				SafePointer<DataBlock> result = new DataBlock(0x40000);
				BitWriter wri(*result);
				LZWTable lzw(wri, data, length);
				while (lzw.Step());
				wri.Flush();
				result->Retain();
				return result;
			}
			DataBlock * EncodeImage(Codec::Frame * data, uint flags, string & filter)
			{
				SafePointer<Codec::Frame> bitmap;
				uint8 * bitmap_data;
				int bitmap_length;
				if (flags & PageFlagColorFull) bitmap = data->ConvertFormat(Codec::PixelFormat::R8G8B8, Codec::ScanOrigin::TopDown, 3 * data->GetWidth());
				else if (flags & PageFlagColorGrayscale) bitmap = data->ConvertFormat(Codec::PixelFormat::R8, Codec::ScanOrigin::TopDown, data->GetWidth());
				else bitmap = data->ConvertFormat(Codec::PixelFormat::R1, Codec::ScanOrigin::TopDown, (data->GetWidth() + 7) >> 3);
				bitmap_data = bitmap->GetData();
				bitmap_length = bitmap->GetHeight() * bitmap->GetScanLineLength();
				SafePointer<DataBlock> result;
				if (flags & PageFlagCompress) {
					if (flags & PageFlagColorFull) {
						filter = FormatString(L"[ /ASCII85Decode /LZWDecode ] /DecodeParms [ null << /Predictor 2 /Colors 3 /Columns %0 >> ]", data->GetWidth());
						for (int j = 0; j < bitmap->GetHeight(); j++) {
							auto base = bitmap_data + j * bitmap->GetScanLineLength();
							for (int i = 3 * bitmap->GetWidth() - 1; i > 2; i--) base[i] -= base[i - 3];
						}
						result = TranscodeLZW(bitmap_data, bitmap_length);
						bitmap.SetReference(0);
						result = TranscodeASCII85(result->GetBuffer(), result->Length());
					} else if (flags & PageFlagColorGrayscale) {
						filter = FormatString(L"[ /ASCII85Decode /LZWDecode ] /DecodeParms [ null << /Predictor 2 /Columns %0 >> ]", data->GetWidth());
						for (int j = 0; j < bitmap->GetHeight(); j++) {
							auto base = bitmap_data + j * bitmap->GetScanLineLength();
							for (int i = bitmap->GetWidth() - 1; i > 0; i--) base[i] -= base[i - 1];
						}
						result = TranscodeLZW(bitmap_data, bitmap_length);
						bitmap.SetReference(0);
						result = TranscodeASCII85(result->GetBuffer(), result->Length());
					} else {
						filter = L"[ /ASCII85Decode /RunLengthDecode ]";
						result = TranscodeRLE8(bitmap_data, bitmap_length);
						bitmap.SetReference(0);
						result = TranscodeASCII85(result->GetBuffer(), result->Length());
					}
				} else {
					filter = L"/ASCII85Decode";
					result = TranscodeASCII85(bitmap_data, bitmap_length);
				}
				result->Retain();
				return result;
			}
		public:
			EncoderContext(Stream * stream) : _serializer(stream), _objtable(0x100), _pagetable(0x40), _rootobj(0), _metaobj(0)
			{
				_serializer.WriteSignature();
				ReserveIndex(); ReserveIndex();
			}
			virtual ~EncoderContext(void) override {}
			virtual bool AddPage(uint physical_width, uint physical_height, Codec::Frame * data, uint flags) noexcept override
			{
				try {
					uint uw = physical_width * 72 / 254;
					uint uh = physical_height * 72 / 254;
					int resource, contents;
					if (data) {
						resource = BeginObject();
						_serializer.WriteRaw("<< /Type /XObject /Subtype /Image /Width ", 41);
						_serializer.WriteInteger(data->GetWidth());
						_serializer.WriteRaw(" /Height ", 9);
						_serializer.WriteInteger(data->GetHeight());
						if (flags & PageFlagColorFull) _serializer.WriteRaw(" /ColorSpace /DeviceRGB ", 24);
						else _serializer.WriteRaw(" /ColorSpace /DeviceGray ", 25);
						if (flags & PageFlagColorFull) _serializer.WriteRaw("/BitsPerComponent 8 ", 20);
						else if (flags & PageFlagColorGrayscale) _serializer.WriteRaw("/BitsPerComponent 8 ", 20);
						else _serializer.WriteRaw("/BitsPerComponent 1 ", 20);
						if (flags & PageFlagInterpolate) _serializer.WriteRaw("/Interpolate true ", 18);
						else _serializer.WriteRaw("/Interpolate false ", 19);
						string filter;
						SafePointer<DataBlock> bitmap = EncodeImage(data, flags, filter);
						_serializer.WriteRaw("/Length ", 8);
						_serializer.WriteInteger(bitmap->Length());
						_serializer.WriteRaw(" /Filter ", 9);
						_serializer.WriteWord(filter);
						_serializer.WriteRaw(" >>", 3);
						_serializer.WriteLF();
						_serializer.WriteRaw("stream", 6);
						_serializer.WriteLF();
						_serializer.WriteRaw(bitmap->GetBuffer(), bitmap->Length());
						_serializer.WriteLF();
						_serializer.WriteRaw("endstream", 9);
						_serializer.WriteLF();
						EndObject();
						contents = BeginObject();
						DynamicString cmd;
						cmd << L"q " << string(uw) << L" 0 0 " << string(uh) << L" 0 0 cm /IM0 Do Q";
						SafePointer<DataBlock> cmd_acsii = cmd.ToString().EncodeSequence(Encoding::ANSI, false);
						_serializer.WriteRaw("<< /Length ", 11);
						_serializer.WriteInteger(cmd_acsii->Length());
						_serializer.WriteRaw(" >>", 3);
						_serializer.WriteLF();
						_serializer.WriteRaw("stream", 6);
						_serializer.WriteLF();
						_serializer.WriteRaw(cmd_acsii->GetBuffer(), cmd_acsii->Length());
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
					if (key == MetadataKey::CreatorSoftware) _metadata.Update(L"Creator", value);
					else if (key == MetadataKey::EncoderSoftware) _metadata.Update(L"Producer", value);
					else if (key == MetadataKey::Author) _metadata.Update(L"Author", value);
					else if (key == MetadataKey::Title) _metadata.Update(L"Title", value);
					else if (key == MetadataKey::Subject) _metadata.Update(L"Subject", value);
					else if (key == MetadataKey::Keywords) _metadata.Update(L"Keywords", value);
					else throw InvalidArgumentException();
					return true;
				} catch (...) { return false; }
			}
			virtual bool SetMetadata(MetadataKey key, Time value) noexcept override
			{
				try {
					uint y, m, d;
					uint H, M, S;
					value.GetDate(y, m, d);
					if (y > 9999) throw InvalidArgumentException();
					H = value.GetHour();
					M = value.GetMinute();
					S = value.GetSecond();
					string enc = FormatString(L"D:%0%1%2%3%4%5Z00\'00\'", string(y, DecimalBase, 4), string(m, DecimalBase, 2),
						string(d, DecimalBase, 2), string(H, DecimalBase, 2), string(M, DecimalBase, 2), string(S, DecimalBase, 2));
					if (key == MetadataKey::CreationDate) _metadata.Update(L"CreationDate", enc);
					else if (key == MetadataKey::AlternationDate) _metadata.Update(L"ModDate", enc);
					else throw InvalidArgumentException();
					return true;
				} catch (...) { return false; }
			}
			virtual bool FinalizeDocument(void) noexcept override
			{
				try {
					BeginObject(2);
					_serializer.WriteRaw("<< /Type /Pages /Count ", 23);
					_serializer.WriteInteger(_pagetable.Length());
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
					_serializer.WriteTrailer(_objtable.Length(), _objtable.GetBuffer(), _rootobj, _metaobj);
					_serializer.Flush();
					return true;
				} catch (...) { return false; }
			}
			virtual string ToString(void) const override { return L"PortableEncoderContext"; }
		};

		IEncoderContext * CreateEncoder(Streaming::Stream * stream)
		{
			try {
				InitializeDefaultCodecs();
				return new EncoderContext(stream);
			} catch (...) { return 0; }
		}
	}
}