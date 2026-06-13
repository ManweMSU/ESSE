#include <Imagines/Imagines.h>

#ifndef ESSE_VERSIO_CORDIS_MAJOR
#define ESSE_VERSIO_CORDIS_MAJOR 0
#endif
#ifndef ESSE_VERSIO_CORDIS_MINOR
#define ESSE_VERSIO_CORDIS_MINOR 0
#endif

namespace ESSE
{
	namespace Picturae
	{
		namespace BMP
		{
			namespace Format
			{
				ESSE_PACKED_STRUCTURE(REFCOLOR_XYZ)
					uint32 x, y, z;
				ESSE_END_PACKED_STRUCTURE
				ESSE_PACKED_STRUCTURE(BITMAPFILEHEADER)
					uint16 sign;
					uint32 file_size;
					uint16 reserved0, reserved1;
					uint32 pixels_offset;
				ESSE_END_PACKED_STRUCTURE
				ESSE_PACKED_STRUCTURE(BITMAPINFOHEADER)
					uint32 struct_size;
					int32 width;
					int32 height;
					uint16 planes;
					uint16 bpp;
					uint32 compression;
					uint32 pixels_size;
					uint32 xppm;
					uint32 yppm;
					uint32 plt_colors_used;
					uint32 plt_colors_imp;
				ESSE_END_PACKED_STRUCTURE
				ESSE_PACKED_STRUCTURE(BITMAPINFOHEADER_V4)
					BITMAPINFOHEADER base_header;
					uint32 mask_red;
					uint32 mask_green;
					uint32 mask_blue;
					uint32 mask_alpha;
					uint32 cs_type;
					REFCOLOR_XYZ ref_red;
					REFCOLOR_XYZ ref_green;
					REFCOLOR_XYZ ref_blue;
					uint32 gamma_red;
					uint32 gamma_green;
					uint32 gamma_blue;
				ESSE_END_PACKED_STRUCTURE
				ESSE_PACKED_STRUCTURE(DirectDrawSurfacePixelFormat)
					uint32 size;
					uint32 flags;
					uint32 four_cc;
					uint32 rgb_bit_count;
					uint32 mask_red;
					uint32 mask_green;
					uint32 mask_blue;
					uint32 mask_alpha;
				ESSE_END_PACKED_STRUCTURE
				ESSE_PACKED_STRUCTURE(DirectDrawSurfaceHeader)
					uint32 size;
					uint32 flags;
					uint32 height;
					uint32 width;
					uint32 stride;
					uint32 depth;
					uint32 mip_map_count;
					uint32 reserved_1[11];
					DirectDrawSurfacePixelFormat pixel_format;
					uint32 caps;
					uint32 caps2;
					uint32 caps3;
					uint32 caps4;
					uint32 reserved_2;
				ESSE_END_PACKED_STRUCTURE
				ESSE_PACKED_STRUCTURE(TruevisionFileHeader)
					uint8 id_length; // set this to zero, when read - ignore 'id_length' bytes after the header
					uint8 palette_usage; // 0 - no palette, 1 - palette present
					uint8 data_format; // bits 0..1: 1 - indexed, 2 - true color, 3 - grayscale; bit 3: use RLE compression
					uint16 palette_first_index;
					uint16 palette_color_number;
					uint8 palette_color_bpp; // 15, 16, 24 or 32 in formats: R5G5B5, R5G5B5A1, R8G8B8, R8G8B8A8
					uint16 x_screen_origin; // ignore, set to zero
					uint16 y_screen_origin; // ignore, set to zero
					uint16 width;
					uint16 height;
					uint8 bpp;
					uint8 image_descr; // bits 0..3: alpha bits number per pixel; bit 4: 0/1 ~ left/right origin; bit 5: 0/1 ~ bottom/top origin
				ESSE_END_PACKED_STRUCTURE
				ESSE_PACKED_STRUCTURE(TruevisionFileExtendedHeader)
					uint16 ex_hdr_size;
					char author[41];
					char comments[324];
					uint16 date_month;
					uint16 date_day;
					uint16 date_year;
					uint16 date_hour;
					uint16 date_minute;
					uint16 date_second;
					char job_name[41];
					uint16 job_hour;
					uint16 job_minute;
					uint16 job_second;
					char encoder_software[41];
					uint16 encoder_version_major;
					uint8 encoder_version_minor;
					uint32 key_color;
					uint16 pixel_aspect_numerator;
					uint16 pixel_aspect_denominator;
					uint16 gamma_numerator;
					uint16 gamma_denominator;
					uint32 color_correction_offset;
					uint32 postage_stamp_offset;
					uint32 scan_line_table_offset;
					uint8 attribute_usage; // 3 - straight, 4 - premultiplied, else ignore
				ESSE_END_PACKED_STRUCTURE
				ESSE_PACKED_STRUCTURE(TruevisionFileFooter)
					uint32 ex_header_offset;
					uint32 dev_header_offset;
					char signature[18]; // 'TRUEVISION-XFILE.\0'
				ESSE_END_PACKED_STRUCTURE
			}
			uint BitmapCodecExtractChannel(uint32 dword, uint32 mask) noexcept
			{
				if (!mask) return 0;
				dword &= mask;
				while (mask & 0xFFFFFF00) { dword >>= 1U; mask >>= 1U; }
				while (!(mask & 0x80)) { dword <<= 1U; mask <<= 1U; }
				while (mask != 0xFF) {
					uint32 test = 0x80;
					int repl = 0;
					while (mask & test) { test >>= 1U; repl++; }
					dword |= dword >> repl;
					mask |= mask >> repl;
				}
				return dword;
			}
			void BitmapCodecEncodeDDS(Stream * dest, Picture * src, const uint * arg_names, const uint * arg_values, uint argc)
			{
				Format::DirectDrawSurfaceHeader hdr;
				uint override_bpp = 0;
				for (uint i = 0; i < argc; i++) if (arg_names[i] == EncoderOptions::OverrideBitDepth) { override_bpp = arg_values[i]; break; }
				PixelFormat pxf = src->GetDesc().format;
				if (pxf != PixelFormat::R8G8B8 && pxf != PixelFormat::R8G8B8X8 && pxf != PixelFormat::R8G8B8A8 &&
					pxf != PixelFormat::B8G8R8 && pxf != PixelFormat::B8G8R8X8 && pxf != PixelFormat::B8G8R8A8) {
					if (!override_bpp) override_bpp = GetBitsPerPixel(pxf);
				}
				if (override_bpp) {
					if (override_bpp <= 24) pxf = PixelFormat::B8G8R8;
					else if (PixelFormatHasAlpha(pxf)) pxf = PixelFormat::B8G8R8A8;
					else pxf = PixelFormat::B8G8R8X8;
				}
				oref<Picture> encode;
				if (src->GetDesc().format != pxf || src->GetDesc().alpha_mode != AlphaMode::Straight || src->GetDesc().origin != ScanOrigin::TopLeft) {
					encode = src->Convert(pxf, AlphaMode::Straight, ScanOrigin::TopLeft);
				} else encode = src;
				auto & desc = encode->GetDesc();
				Memory::ZeroMemory(&hdr, sizeof(hdr));
				hdr.size = sizeof(hdr);
				hdr.flags = 0x100F;
				hdr.height = desc.height;
				hdr.width = desc.width;
				hdr.stride = ((hdr.width * GetBitsPerPixel(pxf) + 7) & ~7) >> 3;
				hdr.pixel_format.size = 32;
				hdr.pixel_format.flags = PixelFormatHasAlpha(pxf) ? 0x41 : 0x40;
				hdr.pixel_format.rgb_bit_count = GetBitsPerPixel(pxf);
				if (pxf == PixelFormat::R8G8B8 || pxf == PixelFormat::R8G8B8X8 || pxf == PixelFormat::R8G8B8A8) {
					hdr.pixel_format.mask_red = 0x000000FF;
					hdr.pixel_format.mask_blue = 0x00FF0000;
				} else if (pxf == PixelFormat::B8G8R8 || pxf == PixelFormat::B8G8R8X8 || pxf == PixelFormat::B8G8R8A8) {
					hdr.pixel_format.mask_red = 0x00FF0000;
					hdr.pixel_format.mask_blue = 0x000000FF;
				}
				hdr.pixel_format.mask_green = 0x0000FF00;
				if (PixelFormatHasAlpha(pxf)) hdr.pixel_format.mask_alpha = 0xFF000000;
				hdr.caps = 0x1000;
				dest->Write("DDS ", 4);
				dest->Write(&hdr, sizeof(hdr));
				for (uint y = 0; y < desc.height; y++) {
					auto base = reinterpret_cast<const uint8 *>(desc.data) + y * desc.stride;
					dest->Write(base, hdr.stride);
				}
			}
			void BitmapCodecEncodeTGA(Stream * dest, Picture * src, const uint * arg_names, const uint * arg_values, uint argc)
			{
				Format::TruevisionFileHeader hdr;
				Format::TruevisionFileExtendedHeader xhdr;
				Format::TruevisionFileFooter footer;
				uint override_bpp = 0;
				for (uint i = 0; i < argc; i++) if (arg_names[i] == EncoderOptions::OverrideBitDepth) { override_bpp = arg_values[i]; break; }
				PixelFormat pxf = src->GetDesc().format;
				if (pxf != PixelFormat::P8 && pxf != PixelFormat::R8 && pxf != PixelFormat::B5G5R5X1 &&
					pxf != PixelFormat::B5G5R5A1 && pxf != PixelFormat::B8G8R8 && pxf != PixelFormat::B8G8R8X8 && pxf != PixelFormat::B8G8R8A8) {
					if (!override_bpp) override_bpp = GetBitsPerPixel(pxf);
				}
				if (override_bpp) {
					if (override_bpp <= 8) {
						if (NeedsPalette(pxf)) pxf = PixelFormat::P8;
						else pxf = PixelFormat::R8;
					} else if (override_bpp <= 15) pxf = PixelFormat::B5G5R5X1;
					else if (override_bpp <= 16 && PixelFormatHasAlpha(pxf)) pxf = PixelFormat::B5G5R5A1;
					else if (override_bpp <= 16) pxf = PixelFormat::B5G5R5X1;
					else if (override_bpp <= 24) pxf = PixelFormat::B8G8R8;
					else if (PixelFormatHasAlpha(pxf)) pxf = PixelFormat::B8G8R8A8;
					else pxf = PixelFormat::B8G8R8X8;
				}
				oref<Picture> encode;
				if (src->GetDesc().format != pxf) {
					auto desc = src->GetDesc();
					desc.format = pxf;
					desc.stride = ((desc.width * GetBitsPerPixel(desc.format) + 31) & ~31) >> 3;
					desc.palette_size = NeedsPalette(desc.format) ? src->GetDesc().palette_size : 0;
					encode = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
					if (NeedsPalette(desc.format)) {
						for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
							encode->SetPixel(x, y, src->GetPixel(x, y));
						}
					} else BlockTransfer(encode->GetDesc(), src->GetDesc());
				} else encode = src;
				hdr.id_length = 0;
				if (NeedsPalette(pxf)) {
					hdr.palette_usage = 1;
					hdr.data_format = 1;
					hdr.palette_color_number = encode->GetDesc().palette_size;
					hdr.palette_color_bpp = 32;
				} else if (pxf == PixelFormat::R8) {
					hdr.palette_usage = 0;
					hdr.data_format = 3;
					hdr.palette_color_number = 0;
					hdr.palette_color_bpp = 0;
				} else {
					hdr.palette_usage = 0;
					hdr.data_format = 2;
					hdr.palette_color_number = 0;
					hdr.palette_color_bpp = 0;
				}
				hdr.palette_first_index = 0;
				hdr.x_screen_origin = 0;
				hdr.y_screen_origin = 0;
				hdr.width = encode->GetDesc().width;
				hdr.height = encode->GetDesc().height;
				hdr.bpp = GetBitsPerPixel(pxf);
				Memory::ZeroMemory(&xhdr, sizeof(xhdr));
				Memory::ZeroMemory(&footer, sizeof(footer));
				Memory::MemoryCopy(&xhdr.encoder_software, "ESSE", 24);
				xhdr.ex_hdr_size = sizeof(xhdr);
				xhdr.encoder_version_major = ESSE_VERSIO_CORDIS_MAJOR;
				xhdr.encoder_version_minor = ESSE_VERSIO_CORDIS_MINOR;
				xhdr.pixel_aspect_numerator = xhdr.pixel_aspect_denominator = 1;
				if (pxf == PixelFormat::B8G8R8A8 || pxf == PixelFormat::B8G8R8X8) hdr.image_descr = 8;
				else if (pxf == PixelFormat::B5G5R5A1 || pxf == PixelFormat::B5G5R5X1) hdr.image_descr = 1;
				else hdr.image_descr = 0;
				if (encode->GetDesc().origin == ScanOrigin::TopLeft) hdr.image_descr |= 0x20;
				if (NeedsPalette(pxf)) xhdr.attribute_usage = 3;
				else if (!PixelFormatHasAlpha(pxf)) xhdr.attribute_usage = 0;
				else if (encode->GetDesc().alpha_mode == AlphaMode::Premultiplied) xhdr.attribute_usage = 4;
				else if (encode->GetDesc().alpha_mode == AlphaMode::Straight) xhdr.attribute_usage = 3;
				else xhdr.attribute_usage = 0;
				array<Color> palette(1);
				if (NeedsPalette(pxf)) palette.Append(encode->GetDesc().palette, encode->GetDesc().palette_size);
				for (auto & c : palette) swap(c.r, c.b);
				dest->Write(&hdr, sizeof(hdr));
				if (palette.GetLength()) dest->Write(palette.GetBuffer(), palette.GetLength() * sizeof(Color));
				auto & desc = encode->GetDesc();
				uint stride = ((desc.width * GetBitsPerPixel(pxf) + 7) & ~7) >> 3;
				array<uint> scanlines(1);
				scanlines.SetLength(desc.height);
				for (uint y = 0; y < desc.height; y++) {
					scanlines[y] = dest->Seek(0, SeekOrigin::Current);
					auto base = reinterpret_cast<const uint8 *>(desc.data) + y * desc.stride;
					dest->Write(base, stride);
				}
				footer.ex_header_offset = dest->Seek(0, SeekOrigin::Current);
				xhdr.scan_line_table_offset = footer.ex_header_offset + sizeof(xhdr);
				Memory::MemoryCopy(&footer.signature, "TRUEVISION-XFILE.", 18);
				dest->Write(&xhdr, sizeof(xhdr));
				dest->Write(scanlines.GetBuffer(), scanlines.GetLength() * 4);
				dest->Write(&footer, sizeof(footer));
			}
			oref<Picture> BitmapCodecDecodeDDS(Stream * stream)
			{
				stream->Seek(4, SeekOrigin::Begin);
				Format::DirectDrawSurfaceHeader hdr;
				if (stream->Read(&hdr, sizeof(hdr)) != sizeof(hdr)) throw InvalidFormatException();
				if ((hdr.flags & 0x1007) != 0x1007) throw InvalidFormatException();
				if ((hdr.flags & 0x800000) && hdr.depth > 1) throw InvalidFormatException();
				if ((hdr.pixel_format.flags & 0x40) == 0) throw InvalidFormatException();
				if (hdr.pixel_format.flags & 0x20206) throw InvalidFormatException();
				if (hdr.pixel_format.size > 32) throw InvalidFormatException();
				if (hdr.pixel_format.rgb_bit_count != 32 && hdr.pixel_format.rgb_bit_count != 24 && hdr.pixel_format.rgb_bit_count != 16 && hdr.pixel_format.rgb_bit_count != 8) throw InvalidFormatException();
				uint stride;
				if (hdr.flags & 0x8) stride = hdr.stride;
				else stride = hdr.pixel_format.rgb_bit_count * hdr.width / 8;
				if (!hdr.width || !hdr.height) throw InvalidFormatException();
				uint32 mask_r, mask_g, mask_b, mask_a;
				uint32 pixel_size = hdr.pixel_format.rgb_bit_count / 8;
				mask_r = hdr.pixel_format.mask_red;
				mask_g = hdr.pixel_format.mask_green;
				mask_b = hdr.pixel_format.mask_blue;
				mask_a = (hdr.pixel_format.flags & 0x01) ? hdr.pixel_format.mask_alpha : 0;
				bool known_pxf = false;
				PixelFormat pxf;
				if (hdr.pixel_format.rgb_bit_count == 16) {
					if (mask_r == 0x1F) {
						if (mask_g == 0x7E0 && mask_b == 0xF800) { pxf = PixelFormat::R5G6B5; known_pxf = true; }
						else if (mask_g == 0x3E0 && mask_b == 0x7C00) {
							if (mask_a == 0x8000) { pxf = PixelFormat::R5G5B5A1; known_pxf = true; }
							else { pxf = PixelFormat::R5G5B5X1; known_pxf = true; }
						}
					} else if (mask_r == 0x0F && mask_g == 0xF0 && mask_b == 0xF00) {
						if (mask_a == 0xF000) { pxf = PixelFormat::R4G4B4A4; known_pxf = true; }
						else { pxf = PixelFormat::R4G4B4X4; known_pxf = true; }
					} else if (mask_b == 0x1F) {
						if (mask_g == 0x7E0 && mask_r == 0xF800) { pxf = PixelFormat::B5G6R5; known_pxf = true; }
						else if (mask_g == 0x3E0 && mask_r == 0x7C00) {
							if (mask_a == 0x8000) { pxf = PixelFormat::B5G5R5A1; known_pxf = true; }
							else { pxf = PixelFormat::B5G5R5X1; known_pxf = true; }
						}
					} else if (mask_b == 0x0F && mask_g == 0xF0 && mask_r == 0xF00) {
						if (mask_a == 0xF000) { pxf = PixelFormat::B4G4R4A4; known_pxf = true; }
						else { pxf = PixelFormat::B4G4R4X4; known_pxf = true; }
					}
				} else if (hdr.pixel_format.rgb_bit_count == 24) {
					if (mask_r == 0x0000FF && mask_g == 0x00FF00 && mask_b == 0xFF0000) { pxf = PixelFormat::R8G8B8; known_pxf = true; }
					else if (mask_b == 0x0000FF && mask_g == 0x00FF00 && mask_r == 0xFF0000) { pxf = PixelFormat::B8G8R8; known_pxf = true; }
				} else if (hdr.pixel_format.rgb_bit_count == 32) {
					if (mask_r == 0x0000FF && mask_g == 0x00FF00 && mask_b == 0xFF0000) {
						if (mask_a == 0xFF000000) { pxf = PixelFormat::R8G8B8A8; known_pxf = true; }
						else { pxf = PixelFormat::R8G8B8X8; known_pxf = true; }
					} else if (mask_b == 0x0000FF && mask_g == 0x00FF00 && mask_r == 0xFF0000) {
						if (mask_a == 0xFF000000) { pxf = PixelFormat::B8G8R8A8; known_pxf = true; }
						else { pxf = PixelFormat::B8G8R8X8; known_pxf = true; }
					}
				}
				PictureInit init;
				PictureDesc desc;
				desc.width = hdr.width;
				desc.height = hdr.height;
				if (known_pxf) {
					desc.format = pxf;
					desc.stride = stride;
					init = PictureInit::AllocateCopy;
				} else if (mask_a) {
					desc.format = PixelFormat::R8G8B8A8;
					desc.stride = 4 * desc.width;
					init = PictureInit::AllocateUninitialized;
				} else {
					desc.format = PixelFormat::R8G8B8X8;
					desc.stride = 4 * desc.width;
					init = PictureInit::AllocateUninitialized;
				}
				desc.alpha_mode = mask_a ? AlphaMode::Straight : AlphaMode::Undefined;
				desc.origin = ScanOrigin::TopLeft;
				desc.palette_size = 0;
				desc.palette = 0;
				DataBlock pixels(1);
				pixels.SetLength(hdr.height * stride);
				stream->Seek(4 + hdr.size, SeekOrigin::Begin);
				if (stream->Read(pixels.GetBuffer(), pixels.GetLength()) != pixels.GetLength()) throw InvalidFormatException();
				desc.data = pixels.GetBuffer();
				auto result = owrap(new Picture(desc, init));
				if (!known_pxf) {
					auto & desc = result->GetDesc();
					for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
						Color pixel = 0;
						uintptr base = (x * hdr.pixel_format.rgb_bit_count / 8) + y * stride;
						uint32 data = pixels[base];
						if (hdr.pixel_format.rgb_bit_count >= 16) data |= uint32(pixels[base + 1]) << 8;
						if (hdr.pixel_format.rgb_bit_count >= 24) data |= uint32(pixels[base + 2]) << 16;
						if (hdr.pixel_format.rgb_bit_count >= 32) data |= uint32(pixels[base + 3]) << 24;
						pixel.r = BitmapCodecExtractChannel(data, mask_r);
						pixel.g = BitmapCodecExtractChannel(data, mask_g);
						pixel.b = BitmapCodecExtractChannel(data, mask_b);
						pixel.a = BitmapCodecExtractChannel(data, mask_a);
						result->SetPixel(x, y, pixel);
					}
				}
				return result;
			}
			oref<Picture> BitmapCodecDecodeTGA(Stream * stream)
			{
				Format::TruevisionFileHeader hdr;
				Format::TruevisionFileExtendedHeader ex_hdr;
				Format::TruevisionFileFooter footer;
				try {
					stream->Seek(stream->GetLength() - sizeof(footer), SeekOrigin::Begin);
					if (stream->Read(&footer, sizeof(footer)) != sizeof(footer)) throw Exception();
					if (Memory::MemoryCompare(&footer.signature, "TRUEVISION-XFILE.", 18) != 0) throw Exception();
					stream->Seek(footer.ex_header_offset, SeekOrigin::Begin);
					if (stream->Read(&ex_hdr, sizeof(ex_hdr)) != sizeof(ex_hdr)) throw Exception();
				} catch (...) {
					Memory::ZeroMemory(&ex_hdr, sizeof(ex_hdr));
					Memory::ZeroMemory(&footer, sizeof(footer));
				}
				stream->Seek(0, SeekOrigin::Begin);
				if (stream->Read(&hdr, sizeof(hdr)) != sizeof(hdr)) throw InvalidFormatException();
				if (!hdr.width || !hdr.height) throw InvalidFormatException();
				AlphaMode alpha_mode = AlphaMode::Undefined;
				if (ex_hdr.attribute_usage == 3) alpha_mode = AlphaMode::Straight;
				else if (ex_hdr.attribute_usage == 4) alpha_mode = AlphaMode::Premultiplied;
				array<Color> palette(1);
				if (hdr.id_length) stream->Seek(sizeof(hdr) + hdr.id_length, SeekOrigin::Begin);
				if (hdr.palette_usage) {
					palette.SetLength(max(uintptr(hdr.palette_first_index) + uintptr(hdr.palette_color_number), uintptr(256)));
					Memory::ZeroMemory(palette.GetBuffer(), sizeof(Color) * palette.GetLength());
					uintptr palette_color_size;
					if (hdr.palette_color_bpp == 15) palette_color_size = 2;
					else if (hdr.palette_color_bpp == 16) palette_color_size = 2;
					else if (hdr.palette_color_bpp == 24) palette_color_size = 3;
					else if (hdr.palette_color_bpp == 32) palette_color_size = 4;
					else throw InvalidFormatException();
					array<uint8> palette_data(1);
					palette_data.SetLength(palette_color_size * hdr.palette_color_number);
					if (stream->Read(palette_data.GetBuffer(), palette_data.GetLength()) != palette_data.GetLength()) throw InvalidFormatException();
					for (uintptr i = 0; i < uintptr(hdr.palette_color_number); i++) {
						Color color;
						if (hdr.palette_color_bpp == 15) {
							uint16 data = reinterpret_cast<uint16 *>(palette_data.GetBuffer())[i];
							color = ConvertPixelValue(data, PixelFormat::B5G5R5X1, AlphaMode::Undefined, PixelFormat::R8G8B8A8, AlphaMode::Straight);
						} else if (hdr.palette_color_bpp == 16) {
							uint16 data = reinterpret_cast<uint16 *>(palette_data.GetBuffer())[i];
							color = ConvertPixelValue(data, PixelFormat::B5G5R5A1, alpha_mode, PixelFormat::R8G8B8A8, AlphaMode::Straight);
							if (alpha_mode == AlphaMode::Undefined) color.a = 0xFF;
						} else if (hdr.palette_color_bpp == 24) {
							uint32 data = *reinterpret_cast<uint32 *>(palette_data.GetBuffer() + 3 * i);
							color = ConvertPixelValue(data, PixelFormat::B8G8R8X8, AlphaMode::Undefined, PixelFormat::R8G8B8A8, AlphaMode::Straight);
						} else if (hdr.palette_color_bpp == 32) {
							uint32 data = *reinterpret_cast<uint32 *>(palette_data.GetBuffer() + 4 * i);
							color = ConvertPixelValue(data, PixelFormat::B8G8R8A8, alpha_mode, PixelFormat::R8G8B8A8, AlphaMode::Straight);
							if (alpha_mode == AlphaMode::Undefined) color.a = 0xFF;
						}
						palette[i + hdr.palette_first_index] = color;
					}
				}
				auto residual = stream->ReadAll();
				uint px_size = hdr.bpp / 8;
				uint image_size = px_size * hdr.width * hdr.height;
				if (hdr.data_format & 0x8) {
					array<uint8> decompress(0x1000);
					uintptr pos = 0;
					while (decompress.GetLength() < image_size && pos < residual->GetLength()) {
						uint8 packet_header = residual->ElementAt(pos);
						uint repeats = (packet_header & 0x7F) + 1;
						pos++;
						if (packet_header & 0x80) {
							for (uint i = 0; i < repeats; i++) for (uint j = 0; j < px_size; j++) decompress << residual->ElementAt(pos + j);
							pos += px_size;
						} else {
							for (uint i = 0; i < repeats; i++) for (uint j = 0; j < px_size; j++) { decompress << residual->ElementAt(pos); pos++; }
						}
					}
					*residual = decompress;
				}
				if (residual->GetLength() < image_size) throw InvalidFormatException();
				residual->Append(0);
				oref<Picture> result;
				if ((hdr.data_format & 0x3) == 1 && hdr.bpp == 16) {
					PictureDesc desc;
					desc.width = hdr.width;
					desc.height = hdr.height;
					desc.stride = 4 * desc.width;
					desc.format = PixelFormat::R8G8B8A8;
					desc.alpha_mode = AlphaMode::Straight;
					desc.origin = ScanOrigin::BottomLeft;
					desc.palette_size = 0;
					result = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
					for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
						uint16 index = reinterpret_cast<uint16 *>(residual->GetBuffer())[x + y * desc.width];
						if (index >= palette.GetLength()) index = 0;
						result->SetPixel(x, y, palette[index]);
					}
				} else {
					PictureDesc desc;
					desc.width = hdr.width;
					desc.height = hdr.height;
					if ((hdr.data_format & 0x3) == 1) {
						if (hdr.bpp != 8) throw InvalidFormatException();
						desc.format = PixelFormat::P8;
						desc.palette = palette;
						desc.palette_size = palette.GetLength();
					} else if ((hdr.data_format & 0x3) == 2) {
						if (hdr.bpp == 15) desc.format = PixelFormat::B5G5R5X1;
						else if (hdr.bpp == 16 && alpha_mode == AlphaMode::Undefined) desc.format = PixelFormat::B5G5R5X1;
						else if (hdr.bpp == 16 && alpha_mode != AlphaMode::Undefined) desc.format = PixelFormat::B5G5R5A1;
						else if (hdr.bpp == 24) desc.format = PixelFormat::B8G8R8;
						else if (hdr.bpp == 32 && alpha_mode == AlphaMode::Undefined) desc.format = PixelFormat::B8G8R8X8;
						else if (hdr.bpp == 32 && alpha_mode != AlphaMode::Undefined) desc.format = PixelFormat::B8G8R8A8;
						else throw InvalidFormatException();
						desc.palette = 0;
						desc.palette_size = 0;
					} else if ((hdr.data_format & 0x3) == 3) {
						if (hdr.bpp != 8) throw InvalidFormatException();
						desc.format = PixelFormat::R8;
						desc.palette = 0;
						desc.palette_size = 0;
					} else throw InvalidFormatException();
					desc.alpha_mode = alpha_mode;
					desc.origin = ScanOrigin::BottomLeft;
					desc.stride = desc.width * GetBitsPerPixel(desc.format) / 8;
					desc.data = residual->GetBuffer();
					result = owrap(new Picture(desc, PictureInit::AllocateCopy));
				}
				auto & desc = result->GetDesc();
				if (hdr.image_descr & 0x10) for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width / 2; x++) {
					auto px = result->GetPixel(x, y);
					result->SetPixel(x, y, result->GetPixel(desc.width - 1 - x, y));
					result->SetPixel(desc.width - 1 - x, y, px);
				}
				if (hdr.image_descr & 0x20) for (uint y = 0; y < desc.height / 2; y++) for (uint x = 0; x < desc.width; x++) {
					auto px = result->GetPixel(x, y);
					result->SetPixel(x, y, result->GetPixel(x, desc.height - 1 - y));
					result->SetPixel(x, desc.height - 1 - y, px);
				}
				return result;
			}
			void BitmapCodecEncode(Stream * dest, Picture * src, const uint * arg_names, const uint * arg_values, uint argc)
			{
				auto pxf = src->GetDesc().format;
				uint enforce_bpp = 0;
				for (uint i = 0; i < argc; i++) if (arg_names[i] == EncoderOptions::OverrideBitDepth) { enforce_bpp = arg_values[i]; break; }
				if (enforce_bpp == 1) pxf = PixelFormat::P1;
				else if (enforce_bpp == 2) pxf = PixelFormat::P2;
				else if (enforce_bpp == 4) pxf = PixelFormat::P4;
				else if (enforce_bpp == 8) pxf = PixelFormat::P8;
				else if (enforce_bpp == 16) pxf = PixelFormat::B5G5R5X1;
				else if (enforce_bpp == 24) pxf = PixelFormat::B8G8R8;
				else if (enforce_bpp == 32) { if (uint(pxf) & 0x00000F00) pxf = PixelFormat::B8G8R8A8; else pxf = PixelFormat::B8G8R8X8; }
				if (pxf != PixelFormat::P1 && pxf != PixelFormat::P2 && pxf != PixelFormat::P4 && pxf != PixelFormat::P8 && pxf != PixelFormat::B8G8R8 && pxf != PixelFormat::B8G8R8X8 && pxf != PixelFormat::B8G8R8A8) {
					auto bpp = GetBitsPerPixel(pxf);
					if (bpp > 25 || (uint(pxf) & 0x00000F00)) { if (uint(pxf) & 0x00000F00) pxf = PixelFormat::B8G8R8A8; else pxf = PixelFormat::B8G8R8X8; }
					else if (bpp > 16) pxf = PixelFormat::B8G8R8;
					else if (bpp > 8) pxf = PixelFormat::B5G5R5X1;
					else if (bpp > 4) pxf = PixelFormat::P8;
					else if (bpp > 2) pxf = PixelFormat::P4;
					else if (bpp > 1) pxf = PixelFormat::P2;
					else pxf = PixelFormat::P1;
				}
				oref<Picture> encode = src->Convert(pxf, AlphaMode::Straight, ScanOrigin::BottomLeft);
				auto & desc = encode->GetDesc();
				if (pxf == PixelFormat::P1 || pxf == PixelFormat::P2 || pxf == PixelFormat::P4 || pxf == PixelFormat::P8) {
					for (uint i = 0; i < desc.palette_size; i++) { swap(desc.palette[i].r, desc.palette[i].b); desc.palette[i].value &= 0xFFFFFF; }
					uint plt_size = 4 * desc.palette_size;
					uint plt_max = 1U << GetBitsPerPixel(pxf);
					Format::BITMAPFILEHEADER fhdr;
					Format::BITMAPINFOHEADER hdr;
					Memory::MemoryCopy(&fhdr.sign, "BM", 2);
					fhdr.reserved0 = fhdr.reserved1 = 0;
					fhdr.pixels_offset = sizeof(fhdr) + sizeof(hdr) + plt_size;
					fhdr.file_size = fhdr.pixels_offset + desc.stride * desc.height;
					hdr.struct_size = sizeof(hdr);
					hdr.width = desc.width;
					hdr.height = desc.height;
					hdr.planes = 1;
					hdr.bpp = GetBitsPerPixel(pxf);
					hdr.compression = 0;
					hdr.pixels_size = desc.stride * desc.height;
					hdr.xppm = hdr.yppm = hdr.plt_colors_imp = 0;
					hdr.plt_colors_used = (plt_size / 4 < plt_max) ? (plt_size / 4) : 0;
					dest->Write(&fhdr, sizeof(fhdr));
					dest->Write(&hdr, sizeof(hdr));
					dest->Write(desc.palette, plt_size);
					dest->Write(desc.data, hdr.pixels_size);
				} else if (pxf == PixelFormat::B5G5R5X1 || pxf == PixelFormat::B8G8R8 || pxf == PixelFormat::B8G8R8X8) {
					Format::BITMAPFILEHEADER fhdr;
					Format::BITMAPINFOHEADER hdr;
					Memory::MemoryCopy(&fhdr.sign, "BM", 2);
					fhdr.reserved0 = fhdr.reserved1 = 0;
					fhdr.pixels_offset = sizeof(fhdr) + sizeof(hdr);
					fhdr.file_size = fhdr.pixels_offset + desc.stride * desc.height;
					hdr.struct_size = sizeof(hdr);
					hdr.width = desc.width;
					hdr.height = desc.height;
					hdr.planes = 1;
					hdr.bpp = GetBitsPerPixel(pxf);
					hdr.compression = 0;
					hdr.pixels_size = desc.stride * desc.height;
					hdr.xppm = hdr.yppm = hdr.plt_colors_used = hdr.plt_colors_imp = 0;
					dest->Write(&fhdr, sizeof(fhdr));
					dest->Write(&hdr, sizeof(hdr));
					dest->Write(desc.data, hdr.pixels_size);
				} else if (pxf == PixelFormat::B8G8R8A8) {
					Format::BITMAPFILEHEADER fhdr;
					Format::BITMAPINFOHEADER_V4 hdr;
					Memory::ZeroMemory(&hdr, sizeof(hdr));
					Memory::MemoryCopy(&fhdr.sign, "BM", 2);
					fhdr.reserved0 = fhdr.reserved1 = 0;
					fhdr.pixels_offset = sizeof(fhdr) + sizeof(hdr) + 12;
					fhdr.file_size = fhdr.pixels_offset + desc.stride * desc.height;
					hdr.base_header.struct_size = sizeof(hdr);
					hdr.base_header.width = desc.width;
					hdr.base_header.height = desc.height;
					hdr.base_header.planes = 1;
					hdr.base_header.bpp = GetBitsPerPixel(pxf);
					hdr.base_header.compression = 3;
					hdr.base_header.pixels_size = desc.stride * desc.height;
					hdr.base_header.plt_colors_used = 3;
					hdr.mask_alpha = 0xFF000000;
					hdr.mask_red = 0x00FF0000;
					hdr.mask_green = 0x0000FF00;
					hdr.mask_blue = 0x000000FF;
					dest->Write(&fhdr, sizeof(fhdr));
					dest->Write(&hdr, sizeof(hdr));
					dest->Write(&hdr.mask_red, 12);
					dest->Write(desc.data, hdr.base_header.pixels_size);
				} else throw InvalidFormatException();
			}
			oref<Picture> BitmapCodecDecode(Stream * stream)
			{
				Format::BITMAPFILEHEADER fhdr;
				stream->Seek(0, SeekOrigin::Begin);
				auto read = stream->Read(&fhdr, sizeof(fhdr));
				if (read != sizeof(fhdr) || fhdr.file_size < sizeof(fhdr) || fhdr.pixels_offset < sizeof(fhdr)) return 0;
				array<uint8> data(1);
				data.SetLength(fhdr.file_size - sizeof(fhdr));
				read = stream->Read(data.GetBuffer(), data.GetLength());
				if (read != data.GetLength() || data.GetLength() < sizeof(Format::BITMAPINFOHEADER)) throw InvalidFormatException();
				auto hdr = reinterpret_cast<Format::BITMAPINFOHEADER *>(data.GetBuffer());
				uint32 pixels_offs = fhdr.pixels_offset - sizeof(fhdr);
				uint32 palette_offs = hdr->struct_size;
				if (hdr->struct_size < sizeof(Format::BITMAPINFOHEADER)) throw InvalidFormatException();
				if (hdr->compression == 3) {
					uint32 r_mask = 0, g_mask = 0, b_mask = 0, a_mask = 0;
					if (hdr->struct_size >= sizeof(Format::BITMAPINFOHEADER_V4)) {
						if (hdr->struct_size > data.GetLength()) throw InvalidFormatException();
						auto hdr_v4 = reinterpret_cast<Format::BITMAPINFOHEADER_V4 *>(data.GetBuffer());
						r_mask = hdr_v4->mask_red;
						g_mask = hdr_v4->mask_green;
						b_mask = hdr_v4->mask_blue;
						a_mask = hdr_v4->mask_alpha;
					} else {
						if (palette_offs + 12 > data.GetLength()) throw InvalidFormatException();
						r_mask = *reinterpret_cast<uint32 *>(data.GetBuffer() + palette_offs + 0);
						g_mask = *reinterpret_cast<uint32 *>(data.GetBuffer() + palette_offs + 4);
						b_mask = *reinterpret_cast<uint32 *>(data.GetBuffer() + palette_offs + 8);
					}
					if (hdr->bpp != 32 && hdr->bpp != 16) throw InvalidFormatException();
					uint32 width = hdr->width;
					uint32 height = hdr->height > 0 ? hdr->height : -hdr->height;
					ScanOrigin org = hdr->height > 0 ? ScanOrigin::BottomLeft : ScanOrigin::TopLeft;
					PixelFormat pxf;
					uint32 src_stride = width * hdr->bpp / 8;
					if (src_stride & 3) src_stride += 2;
					if (a_mask) pxf = PixelFormat::B8G8R8A8;
					else if (hdr->bpp == 32) pxf = PixelFormat::B8G8R8X8;
					else pxf = PixelFormat::B5G5R5X1;
					if (!width || !height) throw InvalidFormatException();
					if (hdr->planes != 1) throw InvalidFormatException();
					if (!hdr->pixels_size) hdr->pixels_size = src_stride * height;
					if (hdr->pixels_size != src_stride * height) throw InvalidFormatException();
					if (hdr->pixels_size + pixels_offs > data.GetLength()) throw InvalidFormatException();
					PictureDesc desc;
					desc.format = pxf;
					desc.alpha_mode = AlphaMode::Straight;
					desc.origin = org;
					desc.width = width;
					desc.height = height;
					desc.stride = src_stride;
					desc.palette_size = 0;
					auto surface = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
					for (uint y = 0; y < height; y++) for (uint x = 0; x < width; x++) {
						uint32 pixel;
						if (hdr->bpp == 32) pixel = *reinterpret_cast<uint32 *>(data.GetBuffer() + pixels_offs + x * 4 + y * src_stride);
						else pixel = *reinterpret_cast<uint16 *>(data.GetBuffer() + pixels_offs + x * 2 + y * src_stride);
						auto r = BitmapCodecExtractChannel(pixel, r_mask);
						auto g = BitmapCodecExtractChannel(pixel, g_mask);
						auto b = BitmapCodecExtractChannel(pixel, b_mask);
						auto a = BitmapCodecExtractChannel(pixel, a_mask);
						auto surface_data = reinterpret_cast<uint8 *>(surface->GetDesc().data);
						if (pxf == PixelFormat::B5G5R5X1) {
							auto & pixel = *reinterpret_cast<uint16 *>(surface_data + x * 2 + y * desc.stride);
							pixel = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
						} else {
							auto & pixel = *reinterpret_cast<uint32 *>(surface_data + x * 4 + y * desc.stride);
							pixel = (a << 24) | (r << 16) | (g << 8) | b;
						}
					}
					return surface;
				} else if (hdr->compression == 0 || hdr->compression == 1 || hdr->compression == 2) {
					uint32 width = hdr->width;
					uint32 height = hdr->height > 0 ? hdr->height : -hdr->height;
					ScanOrigin org = hdr->height > 0 ? ScanOrigin::BottomLeft : ScanOrigin::TopLeft;
					PixelFormat pxf;
					if (!width || !height) throw InvalidFormatException();
					if (hdr->bpp == 32) pxf = PixelFormat::B8G8R8X8;
					else if (hdr->bpp == 24) pxf = PixelFormat::B8G8R8;
					else if (hdr->bpp == 16) pxf = PixelFormat::B5G5R5X1;
					else if (hdr->bpp == 8) pxf = PixelFormat::P8;
					else if (hdr->bpp == 4) pxf = PixelFormat::P4;
					else if (hdr->bpp == 2) pxf = PixelFormat::P2;
					else if (hdr->bpp == 1) pxf = PixelFormat::P1;
					else throw InvalidFormatException();
					if (hdr->planes != 1) throw InvalidFormatException();
					PictureDesc desc;
					desc.format = pxf;
					desc.alpha_mode = AlphaMode::Undefined;
					desc.origin = org;
					desc.width = width;
					desc.height = height;
					desc.stride = ((width * hdr->bpp + 31) & ~31) / 8;
					desc.palette_size = NeedsPalette(pxf) ? 1U << GetBitsPerPixel(pxf) : 0;
					auto surface = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
					if (NeedsPalette(pxf)) {
						uint plt_vol = 1 << hdr->bpp;
						if (hdr->plt_colors_used && hdr->plt_colors_used < plt_vol) {
							Memory::ZeroMemory(surface->GetDesc().palette, plt_vol * sizeof(Color));
							plt_vol = hdr->plt_colors_used;
						}
						if (palette_offs + plt_vol * 4 > data.GetLength()) throw InvalidFormatException();
						for (uint i = 0; i < plt_vol; i++) surface->GetDesc().palette[i].value = *reinterpret_cast<uint32 *>(data.GetBuffer() + palette_offs + 4 * i);
						for (uint i = 0; i < surface->GetDesc().palette_size; i++) {
							swap(surface->GetDesc().palette[i].r, surface->GetDesc().palette[i].b);
							surface->GetDesc().palette[i].a = 0xFF;
						}
						if (hdr->compression == 2) {
							if (hdr->bpp != 4) throw InvalidFormatException();
							if (hdr->pixels_size + pixels_offs > data.GetLength()) throw InvalidFormatException();
							int x = 0, y = 0, rp = 0;
							auto bytes = data.GetBuffer() + pixels_offs;
							while (rp + 2 <= hdr->pixels_size) {
								if (y >= height) break;
								auto repcnt = bytes[rp];
								auto word = bytes[rp + 1];
								rp += 2;
								if (repcnt) {
									for (int i = 0; i < int(repcnt); i++) {
										if (x >= width) { x = 0; y++; }
										if (y < height) {
											if (i & 1) surface->SetPixel(x, height - 1 - y, word & 0xF);
											else surface->SetPixel(x, height - 1 - y, word >> 4);
										}
										x++;
									}
								} else {
									if (word == 0) {
										x = 0;
										y++;
									} else if (word == 1) {
										break;
									} else if (word == 2) {
										if (rp + 2 <= hdr->pixels_size) {
											int dx = bytes[rp];
											int dy = bytes[rp + 1];
											rp += 2;
											x += dx;
											y += dy;
										} else break;
									} else {
										for (int i = 0; i < int(word); i++) {
											if (rp < hdr->pixels_size) {
												uint8 px = bytes[rp];
												if (i & 1) px &= 0xF; else px >>= 4;
												if (x >= width) { x = 0; y++; }
												if (y < height) surface->SetPixel(x, height - 1 - y, px);
												x++;
												if (i & 1) rp++;
											} else break;
										}
										if (word & 1) rp++;
										if ((word & 3) > 0 && (word & 3) < 3) rp++;
									}
								}
							}
						} else if (hdr->compression == 1) {
							if (hdr->bpp != 8) throw InvalidFormatException();
							if (hdr->pixels_size + pixels_offs > data.GetLength()) throw InvalidFormatException();
							int x = 0, y = 0, rp = 0;
							auto bytes = data.GetBuffer() + pixels_offs;
							while (rp + 2 <= hdr->pixels_size) {
								if (y >= height) break;
								auto repcnt = bytes[rp];
								auto word = bytes[rp + 1];
								rp += 2;
								if (repcnt) {
									for (int i = 0; i < int(repcnt); i++) {
										if (x >= width) { x = 0; y++; }
										if (y < height) surface->SetPixel(x, height - 1 - y, word);
										x++;
									}
								} else {
									if (word == 0) {
										x = 0;
										y++;
									} else if (word == 1) {
										break;
									} else if (word == 2) {
										if (rp + 2 <= hdr->pixels_size) {
											int dx = bytes[rp];
											int dy = bytes[rp + 1];
											rp += 2;
											x += dx;
											y += dy;
										} else break;
									} else {
										for (int i = 0; i < int(word); i++) {
											if (rp < hdr->pixels_size) {
												if (x >= width) { x = 0; y++; }
												if (y < height) surface->SetPixel(x, height - 1 - y, bytes[rp]);
												x++;
												rp++;
											} else break;
										}
										if (word & 1) rp++;
									}
								}
							}
						} else {
							if (!hdr->pixels_size) hdr->pixels_size = desc.stride * desc.height;
							if (hdr->pixels_size != desc.stride * desc.height) throw InvalidFormatException();
							if (hdr->pixels_size + pixels_offs > data.GetLength()) throw InvalidFormatException();
							Memory::MemoryCopy(surface->GetDesc().data, data.GetBuffer() + pixels_offs, desc.stride * desc.height);
						}
					} else {
						if (!hdr->pixels_size) hdr->pixels_size = desc.stride * desc.height;
						if (hdr->pixels_size != desc.stride * desc.height) throw InvalidFormatException();
						if (hdr->pixels_size + pixels_offs > data.GetLength()) throw InvalidFormatException();
						Memory::MemoryCopy(surface->GetDesc().data, data.GetBuffer() + pixels_offs, desc.stride * desc.height);
					}
					return surface;
				} else throw InvalidFormatException();
			}
			bool BitmapCodecProbe(Codices::CodecIOProbe & prob)
			{
				if (prob.file_title_size >= 2 && prob.file_title.words[0] == 0x4D42) {
					prob.format = ImageFormatDIB;
					return true;
				} else if (prob.file_title_size >= 4 && Memory::MemoryCompare(&prob.file_title.dwords[0], "DDS ", 4) == 0) {
					prob.format = ImageFormatDDS;
					return true;
				} else if (prob.file_title_size >= sizeof(Format::TruevisionFileHeader)) {
					if (Memory::MemoryCompare(&prob.file_title.dwords[1], "ftyp", 4) == 0) return false;
					auto & hdr = *reinterpret_cast<Format::TruevisionFileHeader *>(&prob.file_title);
					if (hdr.palette_usage == 1) {
						if (hdr.data_format != 1 && hdr.data_format != 9) return false;
					} else if (hdr.palette_usage == 0) {
						if (hdr.data_format != 2 && hdr.data_format != 3 && hdr.data_format != 10 && hdr.data_format != 11) return false;
					} else return false;
					if (hdr.bpp != 8 && hdr.bpp != 16 && hdr.bpp != 24 && hdr.bpp != 32) return false;
					if (hdr.image_descr & 0xC0) return false;
					if (!hdr.width || !hdr.height) return false;
					prob.format = ImageFormatTGA;
					return true;
				} else return false;
			}
			bool BitmapCodecI(Codices::CodecIO mode, void * io, ErrorContext & ectx) noexcept
			{
				ESSE_TRY_INTRO
					if (mode == Codices::CodecIO::Encode) {
						auto & enc = *reinterpret_cast<Codices::CodecIOEncode *>(io);
						if (!enc.stream || !enc.encode || !enc.encode->GetLength()) throw InvalidArgumentException();
						if (Memory::StringCompare(enc.format, ImageFormatDIB) == 0) {
							BitmapCodecEncode(enc.stream, enc.encode->ReferenceAt(0), enc.option_names, enc.option_values, enc.option_number);
						} else if (Memory::StringCompare(enc.format, ImageFormatDDS) == 0) {
							BitmapCodecEncodeDDS(enc.stream, enc.encode->ReferenceAt(0), enc.option_names, enc.option_values, enc.option_number);
						} else if (Memory::StringCompare(enc.format, ImageFormatTGA) == 0) {
							BitmapCodecEncodeTGA(enc.stream, enc.encode->ReferenceAt(0), enc.option_names, enc.option_values, enc.option_number);
						} else return false;
						return true;
					} else if (mode == Codices::CodecIO::Decode) {
						auto & dec = *reinterpret_cast<Codices::CodecIODecode *>(io);
						oref<Picture> result;
						if (dec.format == ImageFormatDIB) result = BitmapCodecDecode(dec.stream);
						else if (dec.format == ImageFormatDDS) result = BitmapCodecDecodeDDS(dec.stream);
						else if (dec.format == ImageFormatTGA) result = BitmapCodecDecodeTGA(dec.stream);
						if (result) {
							dec.decode = owrap(new Image);
							dec.decode->Append(result);
							return true;
						} else return false;
					} else if (mode == Codices::CodecIO::Probe) {
						auto & prob = *reinterpret_cast<Codices::CodecIOProbe *>(io);
						return BitmapCodecProbe(prob);
					} else if (mode == Codices::CodecIO::EncodeFormats) {
						auto & efl = *reinterpret_cast<Codices::CodecIOEncodeFormats *>(io);
						efl.name = "ESSE Codificator Picturarum";
						efl.caps.Append(ImageFormatDIB, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode);
						efl.caps.Append(ImageFormatDDS, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode);
						efl.caps.Append(ImageFormatTGA, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode);
						return true;
					} else if (mode == Codices::CodecIO::EncodeModes) {
						auto & eml = *reinterpret_cast<Codices::CodecIOEncodeModes *>(io);
						if (Memory::StringCompare(eml.format, ImageFormatDIB) == 0) {
							eml.pixel_formats.AddElement(PixelFormat::P1);
							eml.pixel_formats.AddElement(PixelFormat::P2);
							eml.pixel_formats.AddElement(PixelFormat::P4);
							eml.pixel_formats.AddElement(PixelFormat::P8);
							eml.pixel_formats.AddElement(PixelFormat::B5G5R5X1);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8X8);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8A8);
							eml.options.Append(EncoderOptions::OverrideBitDepth, KeyValuePair<uint, uint>(1, 32));
							return true;
						} else if (Memory::StringCompare(eml.format, ImageFormatDDS) == 0) {
							eml.pixel_formats.AddElement(PixelFormat::R8G8B8);
							eml.pixel_formats.AddElement(PixelFormat::R8G8B8X8);
							eml.pixel_formats.AddElement(PixelFormat::R8G8B8A8);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8X8);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8A8);
							eml.options.Append(EncoderOptions::OverrideBitDepth, KeyValuePair<uint, uint>(24, 32));
							return true;
						} else if (Memory::StringCompare(eml.format, ImageFormatTGA) == 0) {
							eml.pixel_formats.AddElement(PixelFormat::P8);
							eml.pixel_formats.AddElement(PixelFormat::R8);
							eml.pixel_formats.AddElement(PixelFormat::B5G5R5X1);
							eml.pixel_formats.AddElement(PixelFormat::B5G5R5A1);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8X8);
							eml.pixel_formats.AddElement(PixelFormat::B8G8R8A8);
							eml.options.Append(EncoderOptions::OverrideBitDepth, KeyValuePair<uint, uint>(8, 32));
							return true;
						} else return false;
					} else ErrorSet(ectx, Errores::ErrorNotImplemented);
					return false;
				ESSE_TRY_OUTRO(false)
			}
			Codices::CodecIOFunction BitmapCodec = BitmapCodecI;
		}
	}
}