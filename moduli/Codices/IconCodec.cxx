#include <Imagines/Imagines.h>
#include <math.h>

namespace ESSE
{
	namespace Picturae
	{
		namespace ICO
		{
			namespace Format
			{
				ESSE_PACKED_STRUCTURE(WindowsIconHeader)
					uint16 reserved;
					uint16 type;
					uint16 count;
				ESSE_END_PACKED_STRUCTURE
				ESSE_PACKED_STRUCTURE(WindowsIconFrameHeader)
					uint8 width;
					uint8 height;
					uint8 colors;
					uint8 reserved;
					union {
						struct { uint16 planes; uint16 bpp; };
						struct { uint16 x_hot_point; uint16 y_hot_point; };
					};
					uint32 size;
					uint32 offset;
				ESSE_END_PACKED_STRUCTURE
				ESSE_PACKED_STRUCTURE(WindowsBitmapInfoHeader)
					uint32 struct_size;
					int32 width;
					int32 height;
					uint16 planes;
					uint16 bpp;
					uint32 compression;
					uint32 data_size;
					int32 dpm_x;
					int32 dpm_y;
					uint32 color_count;
					uint32 color_used;
				ESSE_END_PACKED_STRUCTURE
			}
			oref<DataBlock> AppleRleCompress(const uint8 * data, uintptr size)
			{
				auto dest = owrap(new DataBlock(0x800));
				uintptr pos = 0;
				while (pos < size) {
					if (pos + 2 < size && data[pos + 1] == data[pos] && data[pos + 2] == data[pos]) {
						uintptr ep = pos + 3;
						while (ep - pos < 130 && ep < size && data[ep] == data[pos]) ep++;
						uint8 command = uint8(ep - pos - 3) | 0x80;
						dest->Append(command);
						dest->Append(data[pos]);
						pos = ep;
					} else {
						uintptr ep = pos + 1;
						while (ep - pos < 128 && ep < size && (ep + 2 >= size || data[ep] != data[ep + 1] || data[ep] != data[ep + 2])) ep++;
						uint8 command = uint8(ep - pos - 1);
						dest->Append(command);
						for (uintptr i = pos; i < ep; i++) dest->Append(data[i]);
						pos = ep;
					}
				}
				return dest;
			}
			oref<DataBlock> AppleRleDecompress(const uint8 * data, uintptr size)
			{
				auto dest = owrap(new DataBlock(0x800));
				uintptr pos = 0;
				while (pos < size) {
					uint8 command = data[pos];
					if ((command & 0x80) == 0) {
						pos++;
						int rep = 1 + int(command);
						for (int i = 0; i < rep; i++) {
							if (pos >= size) break;
							dest->Append(data[pos]);
							pos++;
						}
					} else {
						pos++;
						if (pos < size) {
							uint8 word = data[pos];
							pos++;
							int rep = 3 + int(command & 0x7F);
							for (int i = 0; i < rep; i++) dest->Append(word);
						}
					}
				}
				return dest;
			}
			uintptr AppleIconOpenSection(Stream * at, uint type)
			{
				uint write[2];
				write[0] = Memory::ReverseByteOrder(type);
				write[1] = 0;
				auto seek = at->Seek(0, SeekOrigin::Current);
				auto written = at->Write(&write, sizeof(write));
				if (written != sizeof(write)) throw InputOutputException(Errores::SuberrorIO::WriteFailure);
				return seek;
			}
			void AppleIconCloseSection(Stream * at, uint64 seek)
			{
				auto position = at->Seek(0, SeekOrigin::Current);
				auto size = Memory::ReverseByteOrder(uint32(position - seek));
				at->Seek(seek + 4, SeekOrigin::Begin);
				if (at->Write(&size, 4) != 4) throw InputOutputException(Errores::SuberrorIO::WriteFailure);
				at->Seek(position, SeekOrigin::Begin);
			}
			void AppleIconWriteFrame(Stream * stream, Picture * frame, uint type)
			{
				if (type == 'ic11' || type == 'ic12' || type == 'ic13' || type == 'ic14' || type == 'ic07' || type == 'ic08' || type == 'ic09' || type == 'ic10') {
					auto encoded = MemoryStream::Create(0x1000);
					Picturae::Encode(encoded, frame, ImageFormatPNG);
					auto sect = AppleIconOpenSection(stream, type);
					encoded->Seek(0, SeekOrigin::Begin);
					encoded->CopyToUntilEof(stream);
					AppleIconCloseSection(stream, sect);
				} else if (type == 'is32' || type == 'il32') {
					array<uint8> out_r(0x200);
					array<uint8> out_g(0x200);
					array<uint8> out_b(0x200);
					for (uint y = 0; y < frame->GetDesc().height; y++) for (uint x = 0; x < frame->GetDesc().width; x++) {
						auto clr = Color(frame->GetPixel(x, y));
						out_r << clr.r; out_g << clr.g; out_b << clr.b;
					}
					auto com_r = AppleRleCompress(out_r.GetBuffer(), out_r.GetLength());
					auto com_g = AppleRleCompress(out_g.GetBuffer(), out_g.GetLength());
					auto com_b = AppleRleCompress(out_b.GetBuffer(), out_b.GetLength());
					auto sect = AppleIconOpenSection(stream, type);
					stream->WriteBlock(com_r);
					stream->WriteBlock(com_g);
					stream->WriteBlock(com_b);
					AppleIconCloseSection(stream, sect);
				} else if (type == 's8mk' || type == 'l8mk') {
					array<uint8> out(0x200);
					for (uint y = 0; y < frame->GetDesc().height; y++) for (uint x = 0; x < frame->GetDesc().width; x++) {
						auto clr = Color(frame->GetPixel(x, y));
						out << clr.a;
					}
					auto sect = AppleIconOpenSection(stream, type);
					stream->WriteBlock(&out);
					AppleIconCloseSection(stream, sect);
				}
			}
			Picture * AppleIconFindFrame(Image * image, uint size, uint plane)
			{
				for (auto & i : *image) if (i.GetDesc().width == size && i.GetDesc().height == size && i.GetAttributes().plane == plane) return &i;
				return 0;
			}
			void IconCodecEncodeICO(Stream * dest, Image * image, bool cursor_mode, const uint * arg_names, const uint * arg_values, uint argc)
			{
				struct encode_entry {
					uint compress;
					PixelFormat format;
					oref<Picture> color, alpha;
				};
				uint compression_mode = !cursor_mode;
				for (uint i = 0; i < argc; i++) if (arg_names[i] == EncoderOptions::CompressionMode) { compression_mode = arg_values[i]; break; }
				array<encode_entry> entries(image->GetLength());
				for (auto & i : *image) entries.Append(encode_entry{ .compress = 0, .format = i.GetDesc().format, .color = &i });
				for (uintptr i = 0; i < entries.GetLength(); i++) {
					auto & e = entries[i];
					if (e.color->GetDesc().width > 0x100 || e.color->GetDesc().height > 0x100) { entries.Remove(i--); continue; }
					if ((PixelFormatHasRed(e.format) || PixelFormatHasGreen(e.format) || PixelFormatHasBlue(e.format)) && PixelFormatHasAlpha(e.format)) {
						e.format = PixelFormat::B8G8R8A8;
						e.compress = e.color->GetDesc().width == 0x100 && e.color->GetDesc().height == 0x100 && compression_mode;
					} else {
						uintptr pair = i;
						for (uintptr j = i + 1; j < entries.GetLength(); j++) {
							auto & ej = entries[j];
							if (ej.color->GetDesc().width == e.color->GetDesc().width && ej.color->GetDesc().height == e.color->GetDesc().height && ej.color->GetAttributes().plane == e.color->GetAttributes().plane) { pair = j; break; }
						}
						e.compress = 0;
						if (pair != i) {
							if (PixelFormatHasAlpha(e.format)) {
								e.alpha = e.color;
								e.color = entries[pair].color;
							} else e.alpha = entries[pair].color;
							entries.Remove(pair);
						}
						if (!NeedsPalette(e.format)) e.format = PixelFormat::B8G8R8;
						else if (e.format == PixelFormat::P2) e.format = PixelFormat::P4;
					}
				}
				if (!entries.GetLength()) throw InvalidArgumentException();
				Format::WindowsIconHeader hdr;
				array<Format::WindowsIconFrameHeader> fhdr(1);
				object_array<DataBlock> fdata(1);
				fhdr.SetLength(entries.GetLength());
				fdata.SetLength(entries.GetLength());
				hdr.reserved = 0;
				hdr.type = cursor_mode ? 2 : 1;
				hdr.count = fhdr.GetLength();
				Memory::ZeroMemory(fhdr.GetBuffer(), fhdr.GetLength() * sizeof(Format::WindowsIconFrameHeader));
				for (uintptr i = 0; i < entries.GetLength(); i++) {
					auto & e = entries[i];
					auto & ihdr = fhdr[i];
					ihdr.width = e.color->GetDesc().width;
					ihdr.height = e.color->GetDesc().height;
					ihdr.colors = ihdr.reserved = 0;
					if (cursor_mode) {
						ihdr.x_hot_point = e.color->GetAttributes().pointer_offset_x;
						ihdr.y_hot_point = e.color->GetAttributes().pointer_offset_y;
					} else {
						ihdr.planes = 1;
						ihdr.bpp = GetBitsPerPixel(e.format);
					}
					auto data = MemoryStream::Create(0x1000);
					if (e.compress) Encode(data, e.color, ImageFormatPNG); else {
						PictureDesc desc_c, desc_m;
						desc_c.width = desc_m.width = e.color->GetDesc().width;
						desc_c.height = desc_m.height = e.color->GetDesc().height;
						desc_c.format = e.format;
						desc_m.format = PixelFormat::A1;
						desc_c.stride = ((desc_c.width * GetBitsPerPixel(desc_c.format) + 31) & ~31) >> 3;
						desc_m.stride = ((desc_m.width * GetBitsPerPixel(desc_m.format) + 31) & ~31) >> 3;
						desc_c.alpha_mode = desc_m.alpha_mode = AlphaMode::Straight;
						desc_c.origin = desc_m.origin = ScanOrigin::BottomLeft;
						desc_c.palette_size = NeedsPalette(desc_c.format) ? 1U << GetBitsPerPixel(desc_c.format) : 0;
						desc_m.palette_size = 0;
						auto color_map = owrap(new Picture(desc_c, PictureInit::AllocateZeroed));
						auto mask_map = owrap(new Picture(desc_m, PictureInit::AllocateZeroed));
						desc_c = color_map->GetDesc();
						desc_m = mask_map->GetDesc();
						Format::WindowsBitmapInfoHeader bhdr;
						bhdr.struct_size = sizeof(bhdr);
						bhdr.width = desc_c.width;
						bhdr.height = desc_c.height << 1;
						bhdr.planes = 1;
						bhdr.bpp = GetBitsPerPixel(e.format);
						bhdr.compression = bhdr.dpm_x = bhdr.dpm_y = bhdr.color_count = bhdr.color_used = 0;
						if (e.format == PixelFormat::B8G8R8A8) {
							BlockTransfer(desc_c, e.color->GetDesc());
							for (uint y = 0; y < desc_c.height; y++) for (uint x = 0; x < desc_c.width; x++) {
								if (GetPixel(desc_c, x, y) & 0xFF000000) SetPixel(desc_m, x, y, 0); else SetPixel(desc_m, x, y, 1);
							}
						} else if (e.format == PixelFormat::B8G8R8) {
							for (uint y = 0; y < desc_c.height; y++) for (uint x = 0; x < desc_c.width; x++) {
								auto alpha = e.alpha ? ReadAlphaChannel(GetPixel(e.alpha->GetDesc(), x, y), e.alpha->GetDesc().format) : 0xFF;
								if (alpha) {
									auto color = e.color->ReadPixel(x, y);
									SetPixel(desc_c, x, y, (uint(color.r) << 16) | (uint(color.g) << 8) | color.b);
									SetPixel(desc_m, x, y, 0);
								} else {
									SetPixel(desc_c, x, y, 0);
									SetPixel(desc_m, x, y, 1);
								}
							}
						} else {
							int palette_black = -1, palette_transparent = -1;
							if (desc_c.palette_size > e.color->GetDesc().palette_size) {
								palette_transparent = e.color->GetDesc().palette_size;
							} else for (uint j = 0; j < e.color->GetDesc().palette_size; j++) {
								auto color = e.color->GetDesc().palette[j];
								if (!(color.value & 0xFFFFFF)) { if (color.a) palette_black = j; else palette_transparent = j; }
							}
							if (e.alpha && palette_black < 0 && palette_transparent < 0) {
								GeneratePalette(desc_c);
								for (uint j = 0; j < desc_c.palette_size; j++) {
									auto color = desc_c.palette[j];
									if (!(color.value & 0xFFFFFF)) { if (color.a) palette_black = j; else palette_transparent = j; }
								}
								for (uint y = 0; y < desc_c.height; y++) for (uint x = 0; x < desc_c.width; x++) {
									auto color = e.color->ReadPixel(x, y);
									auto alpha = ReadAlphaChannel(GetPixel(e.alpha->GetDesc(), x, y), e.alpha->GetDesc().format);
									if (alpha) {
										color_map->WritePixel(x, y, color);
										SetPixel(desc_m, x, y, 0);
									} else {
										SetPixel(desc_c, x, y, max(palette_black, palette_transparent));
										SetPixel(desc_m, x, y, 1);
									}
								}
							} else {
								Memory::MemoryCopy(desc_c.palette, e.color->GetDesc().palette, e.color->GetDesc().palette_size * sizeof(Color));
								for (uint y = 0; y < desc_c.height; y++) for (uint x = 0; x < desc_c.width; x++) {
									auto index = e.color->GetPixel(x, y);
									auto color = e.color->GetDesc().palette[index];
									auto alpha = e.alpha ? ReadAlphaChannel(GetPixel(e.alpha->GetDesc(), x, y), e.alpha->GetDesc().format) : color.a;
									if (alpha) {
										SetPixel(desc_c, x, y, index);
										SetPixel(desc_m, x, y, 0);
									} else {
										SetPixel(desc_c, x, y, max(palette_black, palette_transparent));
										SetPixel(desc_m, x, y, 1);
									}
								}
							}
							for (uint j = 0; j < desc_c.palette_size; j++) {
								swap(desc_c.palette[j].r, desc_c.palette[j].b);
								desc_c.palette[j].a = 0;
							}
						}
						bhdr.data_size = desc_c.stride * desc_c.height + desc_m.stride * desc_m.height;
						data->Write(&bhdr, sizeof(bhdr));
						if (desc_c.palette_size) data->Write(desc_c.palette, desc_c.palette_size * sizeof(Color));
						data->Write(desc_c.data, desc_c.stride * desc_c.height);
						data->Write(desc_m.data, desc_m.stride * desc_m.height);
					}
					fdata.SetElement(data->GetStorage(), i);
					ihdr.size = fdata[i].GetLength();
				}
				uintptr offset = sizeof(hdr) + sizeof(Format::WindowsIconFrameHeader) * fhdr.GetLength();
				for (auto & ihdr : fhdr) { ihdr.offset = offset; offset += ihdr.size; }
				dest->Write(&hdr, sizeof(hdr));
				dest->Write(fhdr.GetBuffer(), sizeof(Format::WindowsIconFrameHeader) * fhdr.GetLength());
				for (auto & data : fdata) dest->WriteBlock(&data);
			}
			void IconCodecEncodeICNSPlane(Stream * dest, Image * image, uint plane)
			{
				oref<Picture> encode;
				encode = AppleIconFindFrame(image, 16, plane);
				if (encode) {
					encode = encode->Convert(PixelFormat::R8G8B8A8, AlphaMode::Straight, ScanOrigin::TopLeft);
					AppleIconWriteFrame(dest, encode, 'is32');
					AppleIconWriteFrame(dest, encode, 's8mk');
				}
				encode = AppleIconFindFrame(image, 32, plane);
				if (encode) {
					encode = encode->Convert(PixelFormat::R8G8B8A8, AlphaMode::Straight, ScanOrigin::TopLeft);
					AppleIconWriteFrame(dest, encode, 'ic11');
					AppleIconWriteFrame(dest, encode, 'il32');
					AppleIconWriteFrame(dest, encode, 'l8mk');
				}
				encode = AppleIconFindFrame(image, 64, plane);
				if (encode) {
					AppleIconWriteFrame(dest, encode, 'ic12');
				}
				encode = AppleIconFindFrame(image, 128, plane);
				if (encode) {
					AppleIconWriteFrame(dest, encode, 'ic07');
				}
				encode = AppleIconFindFrame(image, 256, plane);
				if (encode) {
					AppleIconWriteFrame(dest, encode, 'ic13');
					AppleIconWriteFrame(dest, encode, 'ic08');
				}
				encode = AppleIconFindFrame(image, 512, plane);
				if (encode) {
					AppleIconWriteFrame(dest, encode, 'ic14');
					AppleIconWriteFrame(dest, encode, 'ic09');
				}
				encode = AppleIconFindFrame(image, 1024, plane);
				if (encode) {
					AppleIconWriteFrame(dest, encode, 'ic10');
				}
			}
			void IconCodecEncodeICNS(Stream * dest, Image * image)
			{
				bool plane1_present = false;
				for (auto & i : *image) if (i.GetAttributes().plane == 1) plane1_present = true;
				auto pos = dest->Seek(0, SeekOrigin::Current);
				uint32 signature = 0x736E6369;
				dest->Write(&signature, 4);
				signature = 0;
				dest->Write(&signature, 4);
				IconCodecEncodeICNSPlane(dest, image, 0);
				if (plane1_present) {
					auto sect = AppleIconOpenSection(dest, 0xFDD92FA8);
					IconCodecEncodeICNSPlane(dest, image, 1);
					AppleIconCloseSection(dest, sect);
				}
				auto end = dest->Seek(0, SeekOrigin::Current);
				uint32 size = Memory::ReverseByteOrder(uint32(end - pos));
				dest->Seek(pos + 4, SeekOrigin::Begin);
				dest->Write(&size, 4);
				dest->Seek(end, SeekOrigin::Begin);
			}
			oref<Image> IconCodecDecodeICO(Stream * stream, bool cursor_mode)
			{
				auto result = owrap(new Image);
				Format::WindowsIconHeader hdr;
				stream->Seek(0, SeekOrigin::Begin);
				auto read = stream->Read(&hdr, sizeof(hdr));
				if (read != sizeof(hdr)) throw InvalidFormatException();
				array<Format::WindowsIconFrameHeader> fhdr(1);
				fhdr.SetLength(hdr.count);
				read = stream->Read(fhdr.GetBuffer(), sizeof(Format::WindowsIconFrameHeader) * fhdr.GetLength());
				if (read != sizeof(Format::WindowsIconFrameHeader) * fhdr.GetLength()) throw InvalidFormatException();
				uint plane_number = 0;
				for (auto & ihdr : fhdr) {
					uint64 signature;
					stream->Seek(ihdr.offset, SeekOrigin::Begin);
					if (ihdr.size >= 8 && stream->Read(&signature, 8) == 8 && signature == 0x0A1A0A0D474E5089) {
						auto substream = Substream::Create(stream, ihdr.offset, ihdr.size);
						ErrorContext ectx; ErrorClear(ectx);
						auto frame = DecodePicture(substream, ectx);
						if (ErrorTest(ectx)) continue;
						auto & attr = frame->GetAttributes();
						if (cursor_mode) {
							attr.pointer_offset_x = ihdr.x_hot_point;
							attr.pointer_offset_y = ihdr.y_hot_point;
						} else {
							attr.pointer_offset_x = 0;
							attr.pointer_offset_y = 0;
						}
						attr.plane = plane_number++;
						attr.animation_duration = 0;
						attr.scale_factor = 0.0;
						result->Append(frame);
					} else {
						Format::WindowsBitmapInfoHeader bhdr;
						stream->Seek(ihdr.offset, SeekOrigin::Begin);
						read = stream->Read(&bhdr, sizeof(bhdr));
						if (read < 24) continue;
						if (bhdr.bpp == 32) {
							PictureDesc desc;
							desc.width = ihdr.width ? ihdr.width : 0x100;
							desc.height = ihdr.height ? ihdr.height : 0x100;
							desc.stride = 4 * desc.width;
							desc.palette_size = 0;
							desc.format = PixelFormat::B8G8R8A8;
							desc.alpha_mode = AlphaMode::Straight;
							desc.origin = bhdr.height > 0 ? ScanOrigin::BottomLeft : ScanOrigin::TopLeft;
							auto frame = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
							stream->Seek(ihdr.offset + bhdr.struct_size, SeekOrigin::Begin);
							auto read = stream->Read(frame->GetDesc().data, desc.stride * desc.height);
							if (read != desc.stride * desc.height) throw InvalidFormatException();
							auto & attr = frame->GetAttributes();
							if (cursor_mode) {
								attr.pointer_offset_x = ihdr.x_hot_point;
								attr.pointer_offset_y = ihdr.y_hot_point;
							} else {
								attr.pointer_offset_x = 0;
								attr.pointer_offset_y = 0;
							}
							attr.plane = plane_number++;
							attr.animation_duration = 0;
							attr.scale_factor = 0.0;
							result->Append(frame);
						} else {
							PictureDesc desc_c, desc_a;
							desc_c.width = desc_a.width = ihdr.width ? ihdr.width : 0x100;
							desc_c.height = desc_a.height = ihdr.height ? ihdr.height : 0x100;
							desc_c.origin = desc_a.origin = bhdr.height > 0 ? ScanOrigin::BottomLeft : ScanOrigin::TopLeft;
							if (bhdr.bpp == 24) desc_c.format = PixelFormat::B8G8R8;
							else if (bhdr.bpp == 8) desc_c.format = PixelFormat::P8;
							else if (bhdr.bpp == 4) desc_c.format = PixelFormat::P4;
							else if (bhdr.bpp == 2) desc_c.format = PixelFormat::P2;
							else if (bhdr.bpp == 1) desc_c.format = PixelFormat::P1;
							else continue;
							desc_a.format = PixelFormat::A1;
							desc_c.stride = ((desc_c.width * GetBitsPerPixel(desc_c.format) + 31) & ~31) >> 3;
							desc_a.stride = ((desc_a.width * GetBitsPerPixel(desc_a.format) + 31) & ~31) >> 3;
							desc_c.alpha_mode = AlphaMode::Undefined;
							desc_a.alpha_mode = AlphaMode::Straight;
							if (bhdr.bpp <= 8) desc_c.palette_size = bhdr.color_used ? bhdr.color_used : 1U << bhdr.bpp;
							else desc_c.palette_size = 0;
							desc_a.palette_size = 0;
							auto frame_c = owrap(new Picture(desc_c, PictureInit::AllocateUninitialized));
							auto frame_a = owrap(new Picture(desc_a, PictureInit::AllocateUninitialized));
							stream->Seek(ihdr.offset + bhdr.struct_size, SeekOrigin::Begin);
							if (frame_c->GetDesc().palette_size) {
								read = stream->Read(frame_c->GetDesc().palette, frame_c->GetDesc().palette_size * sizeof(Color));
								if (read != frame_c->GetDesc().palette_size * sizeof(Color)) continue;
								for (uint i = 0; i < frame_c->GetDesc().palette_size; i++) {
									frame_c->GetDesc().palette[i].a = 0xFF;
									swap(frame_c->GetDesc().palette[i].r, frame_c->GetDesc().palette[i].b);
								}
							}
							read = stream->Read(frame_c->GetDesc().data, desc_c.stride * desc_c.height);
							if (read != desc_c.stride * desc_c.height) throw InvalidFormatException();
							read = stream->Read(frame_a->GetDesc().data, desc_a.stride * desc_a.height);
							if (read != desc_a.stride * desc_a.height) throw InvalidFormatException();
							uint num_dwords = desc_a.stride * desc_a.height / 4;
							for (uint i = 0; i < num_dwords; i++) reinterpret_cast<uint32 *>(frame_a->GetDesc().data)[i] ^= 0xFFFFFFFF;
							auto & attr = frame_c->GetAttributes();
							if (cursor_mode) {
								attr.pointer_offset_x = ihdr.x_hot_point;
								attr.pointer_offset_y = ihdr.y_hot_point;
							} else {
								attr.pointer_offset_x = 0;
								attr.pointer_offset_y = 0;
							}
							attr.plane = plane_number++;
							attr.animation_duration = 0;
							attr.scale_factor = 0.0;
							frame_a->GetAttributes() = attr;
							result->Append(frame_c);
							result->Append(frame_a);
						}
					}
				}
				if (!result->GetLength()) throw InvalidFormatException();
				return result;
			}
			void IconCodecDecodeICNS(Image * dest, const uint8 * src_data, uintptr src_length, uint domain)
			{
				uintptr pos = 0;
				Dictionary<uint32, KeyValuePair<const uint8 *, uintptr>> frames;
				while (pos + 8 <= src_length) {
					uint32 frame_type, frame_size;
					frame_type = Memory::ReverseByteOrder(*reinterpret_cast<const uint32 *>(src_data + pos));
					frame_size = Memory::ReverseByteOrder(*reinterpret_cast<const uint32 *>(src_data + pos + 4));
					if (frame_size < 8) throw InvalidFormatException();
					frames.Append(frame_type, KeyValuePair<const uint8 *, uintptr>(src_data + pos + 8, frame_size - 8));
					pos += frame_size;
				}
				for (auto & fd : frames) {
					if (fd.key == 'icp4' || fd.key == 'icp5' || fd.key == 'icp6' || fd.key == 'ic07' || fd.key == 'ic08' ||
						fd.key == 'ic09' || fd.key == 'ic10' || fd.key == 'ic11' || fd.key == 'ic12' || fd.key == 'ic13' ||
						fd.key == 'ic14' || fd.key == 'icsB' || fd.key == 'sb24' || fd.key == 'SB24') {
						auto substream = StaticMemoryStream::Create(fd.value.key, fd.value.value);
						ErrorContext ectx; ErrorClear(ectx);
						auto frame = DecodePicture(substream, ectx);
						if (!ErrorTest(ectx)) { frame->GetAttributes().plane = domain; dest->Append(frame); }
					} else if (fd.key == 'is32' || fd.key == 'il32' || fd.key == 'ih32' || fd.key == 'it32') {
						uint32 mask_name, side, skip;
						if (fd.key == 'is32') { side = 16; mask_name = 's8mk'; skip = 0; }
						else if (fd.key == 'il32') { side = 32; mask_name = 'l8mk'; skip = 0; }
						else if (fd.key == 'ih32') { side = 48; mask_name = 'h8mk'; skip = 0; }
						else if (fd.key == 'it32') { side = 128; mask_name = 't8mk'; skip = 4; }
						if (fd.value.value < skip) continue;
						auto mask = frames[mask_name];
						auto clr_map = AppleRleDecompress(fd.value.key + skip, fd.value.value - skip);
						auto frame = owrap(new Picture(side, side, PixelFormat::R8G8B8A8, AlphaMode::Straight, ScanOrigin::TopLeft));
						auto desc = frame->GetDesc();
						auto qd = side * side;
						if (mask && mask->value < qd) mask = 0;
						if (clr_map->GetLength() < 3 * qd) continue;
						for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
							uint8 r = clr_map->ElementAt(x + side * y);
							uint8 g = clr_map->ElementAt(qd + x + side * y);
							uint8 b = clr_map->ElementAt(2 * qd + x + side * y);
							uint8 a = mask ? mask->key[x + side * y] : 0xFF;
							frame->SetPixel(x, y, Color(r, g, b, a).value);
						}
						frame->GetAttributes().plane = domain;
						dest->Append(frame);
					} else if (fd.key == 'ic04' || fd.key == 'ic05' || fd.key == 'icsb') {
						if (fd.value.value < 4) continue;
						uint32 subtype = Memory::ReverseByteOrder(*reinterpret_cast<const uint32*>(fd.value.key));
						if (subtype == 'ARGB') {
							uint32 side;
							if (fd.key == 'ic04') side = 16;
							else if (fd.key == 'ic05') side = 32;
							else if (fd.key == 'icsb') side = 18;
							auto clr_map = AppleRleDecompress(fd.value.key + 4, fd.value.value - 4);
							auto frame = owrap(new Picture(side, side, PixelFormat::R8G8B8A8, AlphaMode::Straight, ScanOrigin::TopLeft));
							auto desc = frame->GetDesc();
							auto qd = side * side;
							if (clr_map->GetLength() < 4 * qd) continue;
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								uint8 a = clr_map->ElementAt(x + side * y);
								uint8 r = clr_map->ElementAt(qd + x + side * y);
								uint8 g = clr_map->ElementAt(2 * qd + x + side * y);
								uint8 b = clr_map->ElementAt(3 * qd + x + side * y);
								frame->SetPixel(x, y, Color(r, g, b, a).value);
							}
							frame->GetAttributes().plane = domain;
							dest->Append(frame);
						} else {
							auto substream = StaticMemoryStream::Create(fd.value.key, fd.value.value);
							ErrorContext ectx; ErrorClear(ectx);
							auto frame = DecodePicture(substream, ectx);
							if (!ErrorTest(ectx)) { frame->GetAttributes().plane = domain; dest->Append(frame); }
						}
					} else if (fd.key == 's8mk' || fd.key == 'l8mk' || fd.key == 'h8mk' || fd.key == 't8mk') {
					} else if (fd.key == 0xFDD92FA8) {
						IconCodecDecodeICNS(dest, fd.value.key, fd.value.value, 1);
					} else {}
				}
			}
			oref<Image> IconCodecDecodeICNS(Stream * stream)
			{
				auto result = owrap(new Image);
				uint32 root_length;
				stream->Seek(4, SeekOrigin::Begin);
				auto read = stream->Read(&root_length, 4);
				if (read != 4) throw InvalidFormatException();
				root_length = Memory::ReverseByteOrder(root_length);
				if (root_length <= 8) throw InvalidFormatException();
				auto root_data = owrap(new DataBlock(1));
				root_data->SetLength(root_length - 8);
				read = stream->Read(root_data->GetBuffer(), root_data->GetLength());
				if (read != root_data->GetLength()) throw InvalidFormatException();
				IconCodecDecodeICNS(result, root_data->GetBuffer(), root_data->GetLength(), 0);
				if (!result->GetLength()) throw InvalidFormatException();
				return result;
			}
			bool IconCodecDecode(Codices::CodecIODecode & io)
			{
				if (io.format == ImageFormatWindowsIcon) {
					io.decode = IconCodecDecodeICO(io.stream, false);
					uint fuse = 1;
					for (uint i = 0; i < io.option_number; i++) if (io.option_names[i] == DecoderOptions::TransparentcyMaskFusionMode) fuse = io.option_values[i];
					if (fuse) AlphaColorFuse(io.decode);
				} else if (io.format == ImageFormatWindowsCursor) {
					io.decode = IconCodecDecodeICO(io.stream, true);
					uint fuse = 1;
					for (uint i = 0; i < io.option_number; i++) if (io.option_names[i] == DecoderOptions::TransparentcyMaskFusionMode) fuse = io.option_values[i];
					if (fuse) AlphaColorFuse(io.decode);
				} else if (io.format == ImageFormatAppleIcon) {
					io.decode = IconCodecDecodeICNS(io.stream);
				} else return false;
				if (io.decode) {
					uint64 min_quad = int64(-1);
					for (auto & f : *io.decode) {
						uint64 quad = uint64(f.GetDesc().width) * uint64(f.GetDesc().height);
						if (quad < min_quad) min_quad = quad;
					}
					for (auto & f : *io.decode) {
						uint64 quad = uint64(f.GetDesc().width) * uint64(f.GetDesc().height);
						f.GetAttributes().scale_factor = sqrt(double(quad) / double(min_quad));
					}
					return true;
				} else return false;
			}
			bool IconCodecProbe(Codices::CodecIOProbe & prob)
			{
				if (prob.file_title_size < 4) return false;
				if (prob.file_title_size >= 12 && Memory::MemoryCompare(&prob.file_title.dwords[1], "ftyp", 4) == 0) return false;
				if (prob.file_title.dwords[0] == 0x00010000) {
					prob.format = ImageFormatWindowsIcon;
					return true;
				} else if (prob.file_title.dwords[0] == 0x00020000) {
					prob.format = ImageFormatWindowsCursor;
					return true;
				} else if (prob.file_title.dwords[0] == 0x736E6369) {
					prob.format = ImageFormatAppleIcon;
					return true;
				} else return false;
			}
			bool IconCodecI(Codices::CodecIO mode, void * io, ErrorContext & ectx) noexcept
			{
				ESSE_TRY_INTRO
					if (mode == Codices::CodecIO::Encode) {
						auto & enc = *reinterpret_cast<Codices::CodecIOEncode *>(io);
						if (!enc.stream || !enc.encode || !enc.encode->GetLength()) throw InvalidArgumentException();
						if (Memory::StringCompare(enc.format, ImageFormatWindowsIcon) == 0) {
							IconCodecEncodeICO(enc.stream, enc.encode, false, enc.option_names, enc.option_values, enc.option_number);
							return true;
						} else if (Memory::StringCompare(enc.format, ImageFormatWindowsCursor) == 0) {
							IconCodecEncodeICO(enc.stream, enc.encode, true, enc.option_names, enc.option_values, enc.option_number);
							return true;
						} else if (Memory::StringCompare(enc.format, ImageFormatAppleIcon) == 0) {
							IconCodecEncodeICNS(enc.stream, enc.encode);
							return true;
						} else return false;
					} else if (mode == Codices::CodecIO::Decode) {
						auto & dec = *reinterpret_cast<Codices::CodecIODecode *>(io);
						return IconCodecDecode(dec);
					} else if (mode == Codices::CodecIO::Probe) {
						auto & prob = *reinterpret_cast<Codices::CodecIOProbe *>(io);
						return IconCodecProbe(prob);
					} else if (mode == Codices::CodecIO::EncodeFormats) {
						auto & efl = *reinterpret_cast<Codices::CodecIOEncodeFormats *>(io);
						efl.name = "ESSE Codificator Iconum";
						efl.caps.Append(ImageFormatWindowsIcon, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode | Codices::CodecIOMode::Multiframe);
						efl.caps.Append(ImageFormatWindowsCursor, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode | Codices::CodecIOMode::Multiframe);
						efl.caps.Append(ImageFormatAppleIcon, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode | Codices::CodecIOMode::Multiframe);
						return true;
					} else if (mode == Codices::CodecIO::EncodeModes) {
						auto & eml = *reinterpret_cast<Codices::CodecIOEncodeModes *>(io);
						if (Memory::StringCompare(eml.format, ImageFormatWindowsIcon) == 0 || Memory::StringCompare(eml.format, ImageFormatWindowsCursor) == 0) {
							eml.pixel_formats.AddElement(PixelFormat::A1);
							eml.pixel_formats.AddElement(PixelFormat::P1);
							eml.pixel_formats.AddElement(PixelFormat::P4);
							eml.pixel_formats.AddElement(PixelFormat::P8);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8A8);
							eml.options.Append(EncoderOptions::CompressionMode, KeyValuePair<uint, uint>(0, 1));
							eml.options.Append(DecoderOptions::TransparentcyMaskFusionMode, KeyValuePair<uint, uint>(0, 1));
							return true;
						} else if (Memory::StringCompare(eml.format, ImageFormatAppleIcon) == 0) {
							eml.pixel_formats.AddElement(PixelFormat::R8G8B8A8);
							return true;
						} else return false;
					} else ErrorSet(ectx, Errores::ErrorNotImplemented);
					return false;
				ESSE_TRY_OUTRO()
			}
			Codices::CodecIOFunction IconCodec = IconCodecI;
		}
	}
}