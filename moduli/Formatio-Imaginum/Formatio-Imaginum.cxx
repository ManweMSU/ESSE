#include <Imagines/Imagines.h>
#include <Compressio/ChainCompression.h>
#include <math.h>

using namespace ESSE::Picturae;

namespace ESSE
{
	namespace Formationes
	{
		namespace Format
		{
			ESSE_PACKED_STRUCTURE(ImageVolumeHeader)
				uint8 signature[8];		// ecs.1.0
				uint32 signature_ex;	// 0x80000006
				uint32 version;			// 0 or 1
				uint32 frame_count;
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(ImageVolumeFrameHeaderV0)
				uint32 width;
				uint32 height;
				uint32 plane;
				int32 cursor_position_x;
				int32 cursor_position_y;
				int32 duration;
				double scale_factor;
				// bits 0...3: pixel format: 0 - B8G8R8A8, 1 - B8G8R8, 2 - P8, palette in B8G8R8A8
				// bits 4...7: compression method: 0 - no compression, 16 - chain compression
				uint32 data_compression;
				uint32 data_offset;
				uint32 data_size;
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(ImageVolumeFrameHeaderV1)
				uint32 width;
				uint32 height;
				uint32 plane;
				int32 cursor_position_x;
				int32 cursor_position_y;
				int32 duration;
				double scale_factor;
				// bits 0...3: compressed pixel format: 0 - B8G8R8A8, 1 - B8G8R8, 2 - P8, palette in B8G8R8A8, 3 - R8*, 4 - A8*, 5 - R8A8*
				// bits 4...7: compression method: 0 - no compression, 16 - chain compression, 32 - premultiplied alpha*, 64 - top-to-bottom scan*
				uint32 data_compression;
				uint32 data_pixel_format;	// for the uncompressed data only
				uint32 data_stride;			// for the uncompressed data only
				uint32 data_offset;
				uint32 data_size;
			ESSE_END_PACKED_STRUCTURE
		}
		void VolumeCodecEncode(Stream * dest, Image * image, const uint * optn, const uint * optv, uint nopt)
		{
			if (!image->GetLength()) throw InvalidArgumentException();
			uint compress = 1, compression_level = 50, bit_depth_override = 0, needs_v1 = 0;
			for (uint i = 0; i < nopt; i++) {
				if (optn[i] == EncoderOptions::OverrideBitDepth) bit_depth_override = optv[i];
				if (optn[i] == EncoderOptions::CompressionMode) compress = optv[i];
				if (optn[i] == EncoderOptions::CompressionQuality) compression_level = optv[i];
			}
			Format::ImageVolumeHeader hdr;
			array<Format::ImageVolumeFrameHeaderV1> fhdr(1);
			object_array<DataBlock> fdata(1);
			fhdr.SetLength(image->GetLength()); fdata.SetLength(image->GetLength());
			for (uintptr i = 0; i < image->GetLength(); i++) {
				oref<Picture> frame = image->ReferenceAt(i);
				fhdr[i].width = frame->GetDesc().width;
				fhdr[i].height = frame->GetDesc().height;
				fhdr[i].plane = frame->GetAttributes().plane;
				fhdr[i].cursor_position_x = frame->GetAttributes().pointer_offset_x;
				fhdr[i].cursor_position_y = frame->GetAttributes().pointer_offset_y;
				fhdr[i].duration = frame->GetAttributes().animation_duration;
				fhdr[i].scale_factor = frame->GetAttributes().scale_factor;
				if (compress) {
					frame = frame->Convert(PixelFormat::B8G8R8A8, AlphaMode::Straight, ScanOrigin::BottomLeft);
					auto & desc = frame->GetDesc();
					fhdr[i].data_stride = 0;
					Set<uint32> colors;
					uint number_of_colors = 0;
					auto spxf = image->ReferenceAt(i)->GetDesc().format;
					bool alpha_only = PixelFormatHasAlpha(spxf) && !PixelFormatHasRed(spxf) && !PixelFormatHasGreen(spxf) && !PixelFormatHasBlue(spxf);
					bool needs_alpha = false;
					bool needs_color = false;
					for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
						Color color = GetPixel(desc, x, y);
						if (color.a != 0xFF) needs_alpha = true;
						if (color.r != color.g || color.r != color.b) needs_color = true;
						if (number_of_colors <= 0x100 && !colors[color.value]) { colors.AddElement(color); number_of_colors++; }
					}
					auto linear_data = MemoryStream::Create(0x10000);
					if (number_of_colors <= 0x100 && !(compression_level > 50 && !needs_color) && !alpha_only) {
						fhdr[i].data_compression = 18;
						fhdr[i].data_pixel_format = uint(PixelFormat::P8);
						uint pcnt = 0;
						array<uint32> palette(0x100);
						Dictionary<uint32, uint> colormap;
						for (auto & c : colors) { colormap.Append(c, pcnt++); palette.Append(c); }
						linear_data->Write(&pcnt, 1);
						linear_data->Write(palette.GetBuffer(), palette.GetLength() * 4);
						for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
							auto color = colormap[GetPixel(desc, x, desc.height - 1 - y)];
							uint8 index = color ? *color : 0;
							linear_data->Write(&index, 1);
						}
					} else {
						if (compression_level > 50 && alpha_only) {
							needs_v1 = 1;
							fhdr[i].data_compression = 20;
							fhdr[i].data_pixel_format = uint(PixelFormat::A8);
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.a, 1);
							}
						} else if (compression_level > 50 && !needs_alpha && !needs_color) {
							needs_v1 = 1;
							fhdr[i].data_compression = 19;
							fhdr[i].data_pixel_format = uint(PixelFormat::R8);
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.r, 1);
							}
						} else if (compression_level > 50 && needs_alpha && !needs_color) {
							needs_v1 = 1;
							fhdr[i].data_compression = 21;
							fhdr[i].data_pixel_format = uint(PixelFormat::R8A8);
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.r, 1);
							}
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.a, 1);
							}
						} else if (!needs_alpha) {
							fhdr[i].data_compression = 17;
							fhdr[i].data_pixel_format = uint(PixelFormat::B8G8R8);
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.r, 1);
							}
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.g, 1);
							}
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.b, 1);
							}
						} else {
							fhdr[i].data_compression = 16;
							fhdr[i].data_pixel_format = uint(PixelFormat::B8G8R8A8);
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.r, 1);
							}
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.g, 1);
							}
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.b, 1);
							}
							for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
								Color color = GetPixel(desc, x, desc.height - 1 - y);
								linear_data->Write(&color.a, 1);
							}
						}
					}
					linear_data->Seek(0, SeekOrigin::Begin);
					auto linear_data_array = linear_data->GetStorage();
					for (uintptr j = linear_data_array->GetLength() - 1; j > 0; j--) linear_data_array->ElementAt(j) -= linear_data_array->ElementAt(j - 1);
					auto compressed_data = MemoryStream::Create(0x10000);
					if (compression_level >= 100) {
						Compression::MethodChain chains[2];
						chains[0] = Compression::MethodChain(Compression::Method::RunLengthEncoding8bit, Compression::Method::Huffman);
						chains[1] = Compression::MethodChain(Compression::Method::RunLengthEncoding8bit, Compression::Method::FusedLempelZivWelchHuffman12bit);
						Compression::ChainCompress(compressed_data, linear_data, chains, 2, Compression::Quality::ExtraVariative, 0, 0x10000);
					} else if (compression_level > 75) {
						Compression::MethodChain chains[2];
						chains[0] = Compression::MethodChain(Compression::Method::Huffman);
						chains[1] = Compression::MethodChain(Compression::Method::FusedLempelZivWelchHuffman12bit);
						Compression::ChainCompress(compressed_data, linear_data, chains, 2, Compression::Quality::Sequential, 0, 0x10000);
					} else {
						auto chain = Compression::MethodChain(Compression::Method::RunLengthEncoding8bit, Compression::Method::Huffman);
						Compression::ChainCompress(compressed_data, linear_data, &chain, 1, Compression::Quality::Sequential, 0, 0x10000);
					}
					fdata.SetElement(compressed_data->GetStorage(), i);
				} else {
					if (bit_depth_override) {
						if (bit_depth_override <= 8) {
							if (PixelFormatHasAlpha(frame->GetDesc().format) && !PixelFormatHasRed(frame->GetDesc().format)) frame = frame->Convert(PixelFormat::A8);
							else frame = frame->Convert(PixelFormat::R8);
						} else if (bit_depth_override <= 16) frame = frame->Convert(PixelFormat::R8A8);
						else if (bit_depth_override <= 24) frame = frame->Convert(PixelFormat::B8G8R8);
						else frame = frame->Convert(PixelFormat::B8G8R8A8);
					}
					auto & desc = frame->GetDesc();
					needs_v1 = 1;
					fhdr[i].data_compression = 0;
					fhdr[i].data_pixel_format = uint(desc.format);
					fhdr[i].data_stride = desc.stride;
					if (desc.alpha_mode == AlphaMode::Premultiplied) fhdr[i].data_compression |= 32;
					if (desc.origin == ScanOrigin::TopLeft) fhdr[i].data_compression |= 64;
					auto data = owrap(new DataBlock(1));
					if (NeedsPalette(desc.format)) {
						data->SetLength(4 * (desc.palette_size + 1));
						*reinterpret_cast<uint32 *>(data->GetBuffer()) = desc.palette_size;
						Memory::MemoryCopy(data->GetBuffer() + 4, desc.palette, 4 * desc.palette_size);
					}
					auto base = data->GetLength();
					data->SetLength(base + desc.stride * desc.height);
					Memory::ZeroMemory(data->GetBuffer() + base, desc.stride * desc.height);
					for (uint y = 0; y < desc.height; y++) {
						Memory::MemoryCopy(data->GetBuffer() + base + y * desc.stride, reinterpret_cast<const uint8 *>(desc.data) + y * desc.stride, desc.stride);
					}
					fdata.SetElement(data, i);
				}
			}
			Memory::MemoryCopy(&hdr.signature, "ecs.1.0", 8);
			hdr.signature_ex = 0x80000006;
			hdr.frame_count = fhdr.GetLength();
			if (needs_v1) {
				hdr.version = 1;
				uint64 offset = sizeof(hdr) + fhdr.GetLength() * sizeof(*fhdr);
				for (uintptr i = 0; i < fhdr.GetLength(); i++) {
					fhdr[i].data_offset = offset;
					fhdr[i].data_size = fdata[i].GetLength();
					offset += fdata[i].GetLength();
				}
				dest->Write(&hdr, sizeof(hdr));
				dest->Write(fhdr.GetBuffer(), fhdr.GetLength() * sizeof(*fhdr));
				for (auto & d : fdata) dest->WriteBlock(&d);
			} else {
				hdr.version = 0;
				array<Format::ImageVolumeFrameHeaderV0> fhdr0(1);
				fhdr0.SetLength(fhdr.GetLength());
				for (uintptr i = 0; i < fhdr.GetLength(); i++) {
					fhdr0[i].width = fhdr[i].width;
					fhdr0[i].height = fhdr[i].height;
					fhdr0[i].plane = fhdr[i].plane;
					fhdr0[i].cursor_position_x = fhdr[i].cursor_position_x;
					fhdr0[i].cursor_position_y = fhdr[i].cursor_position_y;
					fhdr0[i].duration = fhdr[i].duration;
					fhdr0[i].scale_factor = fhdr[i].scale_factor;
					fhdr0[i].data_compression = fhdr[i].data_compression;
				}
				uint64 offset = sizeof(hdr) + fhdr0.GetLength() * sizeof(*fhdr0);
				for (uintptr i = 0; i < fhdr0.GetLength(); i++) {
					fhdr0[i].data_offset = offset;
					fhdr0[i].data_size = fdata[i].GetLength();
					offset += fdata[i].GetLength();
				}
				dest->Write(&hdr, sizeof(hdr));
				dest->Write(fhdr0.GetBuffer(), fhdr0.GetLength() * sizeof(*fhdr0));
				for (auto & d : fdata) dest->WriteBlock(&d);
			}
		}
		oref<Image> VolumeCodecDecode(Stream * source, const uint * optn, const uint * optv, uint nopt)
		{
			Format::ImageVolumeHeader hdr;
			array<Format::ImageVolumeFrameHeaderV1> fhdr(1);
			source->Seek(0, SeekOrigin::Begin);
			if (source->Read(&hdr, sizeof(hdr)) != sizeof(hdr)) throw InvalidFormatException();
			if (hdr.version == 0) {
				array<Format::ImageVolumeFrameHeaderV0> fhdr0(1);
				fhdr.SetLength(hdr.frame_count); fhdr0.SetLength(hdr.frame_count);
				if (source->Read(fhdr0.GetBuffer(), fhdr0.GetLength() * sizeof(*fhdr0)) != fhdr0.GetLength() * sizeof(*fhdr0)) throw InvalidFormatException();
				for (uintptr i = 0; i < fhdr.GetLength(); i++) {
					fhdr[i].width = fhdr0[i].width;
					fhdr[i].height = fhdr0[i].height;
					fhdr[i].plane = fhdr0[i].plane;
					fhdr[i].cursor_position_x = fhdr0[i].cursor_position_x;
					fhdr[i].cursor_position_y = fhdr0[i].cursor_position_y;
					fhdr[i].duration = fhdr0[i].duration;
					fhdr[i].scale_factor = fhdr0[i].scale_factor;
					fhdr[i].data_compression = fhdr0[i].data_compression;
					fhdr[i].data_pixel_format = 0;
					fhdr[i].data_stride = 0;
					fhdr[i].data_offset = fhdr0[i].data_offset;
					fhdr[i].data_size = fhdr0[i].data_size;
				}
			} else if (hdr.version == 1) {
				fhdr.SetLength(hdr.frame_count);
				if (source->Read(fhdr.GetBuffer(), fhdr.GetLength() * sizeof(*fhdr)) != fhdr.GetLength() * sizeof(*fhdr)) throw InvalidFormatException();
			}
			double min_scale = 0.0;
			double max_scale = INFINITY;
			for (uint i = 0; i < nopt; i++) {
				if (optn[i] == DecoderOptions::MinimalDecodeScaleFactor) min_scale = double(optv[i]) / 65536.0;
				if (optn[i] == DecoderOptions::MaximalDecodeScaleFactor) max_scale = double(optv[i]) / 65536.0;
			}
			auto result = owrap(new Image());
			for (uintptr i = 0; i < fhdr.GetLength(); i++) {
				auto & fdesc = fhdr[i];
				if (fdesc.scale_factor < min_scale || fdesc.scale_factor > max_scale) continue;
				oref<Picture> frame;
				PictureDesc desc;
				desc.width = fdesc.width;
				desc.height = fdesc.height;
				if (fdesc.data_compression & 16) {
					uint mode = fdesc.data_compression & 15;
					auto frame_source = Substream::Create(source, fdesc.data_offset, fdesc.data_size);
					auto uncompressed = MemoryStream::Create(0x10000);
					Compression::ChainDecompress(uncompressed, frame_source);
					auto & data = *uncompressed->GetStorage();
					for (uintptr j = 1; j < data.GetLength(); j++) data[j] += data[j - 1];
					if (mode == 0) {
						desc.stride = 4 * desc.width;
						desc.palette_size = 0;
						desc.format = PixelFormat::B8G8R8A8;
						desc.alpha_mode = AlphaMode::Straight;
						desc.origin = ScanOrigin::BottomLeft;
						frame = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
						desc = frame->GetDesc();
						uintptr channel_stride = desc.width * desc.height;
						if (data.GetLength() < 4 * channel_stride) throw InvalidFormatException();
						for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
							Color color;
							color.r = data[x + y * desc.width];
							color.g = data[x + y * desc.width + channel_stride];
							color.b = data[x + y * desc.width + 2 * channel_stride];
							color.a = data[x + y * desc.width + 3 * channel_stride];
							SetPixel(desc, x, desc.height - 1 - y, color.value);
						}
					} else if (mode == 1) {
						desc.stride = (3 * desc.width + 3) & ~3;
						desc.palette_size = 0;
						desc.format = PixelFormat::B8G8R8;
						desc.alpha_mode = AlphaMode::Undefined;
						desc.origin = ScanOrigin::BottomLeft;
						frame = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
						desc = frame->GetDesc();
						uintptr channel_stride = desc.width * desc.height;
						if (data.GetLength() < 3 * channel_stride) throw InvalidFormatException();
						for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
							Color color;
							color.r = data[x + y * desc.width];
							color.g = data[x + y * desc.width + channel_stride];
							color.b = data[x + y * desc.width + 2 * channel_stride];
							SetPixel(desc, x, desc.height - 1 - y, color.value);
						}
					} else if (mode == 2) {
						if (data.GetLength() < 1) throw InvalidFormatException();
						desc.palette_size = data[0];
						if (!desc.palette_size) desc.palette_size = 0x100;
						if (data.GetLength() < 1 + 4 * desc.palette_size) throw InvalidFormatException();
						array<Color> palette(1);
						palette.SetLength(desc.palette_size);
						Memory::MemoryCopy(palette.GetBuffer(), data.GetBuffer() + 1, 4 * desc.palette_size);
						for (auto & c : palette) { swap(c.r, c.b); }
						desc.stride = (desc.width + 3) & ~3;
						desc.format = PixelFormat::P8;
						desc.alpha_mode = AlphaMode::Undefined;
						desc.origin = ScanOrigin::BottomLeft;
						frame = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
						desc = frame->GetDesc();
						Memory::MemoryCopy(desc.palette, palette.GetBuffer(), 4 * desc.palette_size);
						uintptr channel_base = 1 + 4 * desc.palette_size;
						if (data.GetLength() < channel_base + desc.width * desc.height) throw InvalidFormatException();
						for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
							uint index = data[channel_base + x + y * desc.width];
							if (index >= desc.palette_size) throw InvalidFormatException();
							SetPixel(desc, x, desc.height - 1 - y, index);
						}
					} else if (mode == 3) {
						desc.stride = (desc.width + 3) & ~3;
						desc.palette_size = 0;
						desc.format = PixelFormat::R8;
						desc.alpha_mode = AlphaMode::Undefined;
						desc.origin = ScanOrigin::BottomLeft;
						frame = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
						desc = frame->GetDesc();
						uintptr channel_stride = desc.width * desc.height;
						if (data.GetLength() < channel_stride) throw InvalidFormatException();
						for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
							SetPixel(desc, x, desc.height - 1 - y, data[x + y * desc.width]);
						}
					} else if (mode == 4) {
						desc.stride = (desc.width + 3) & ~3;
						desc.palette_size = 0;
						desc.format = PixelFormat::A8;
						desc.alpha_mode = AlphaMode::Straight;
						desc.origin = ScanOrigin::BottomLeft;
						frame = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
						desc = frame->GetDesc();
						uintptr channel_stride = desc.width * desc.height;
						if (data.GetLength() < channel_stride) throw InvalidFormatException();
						for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
							SetPixel(desc, x, desc.height - 1 - y, data[x + y * desc.width]);
						}
					} else if (mode == 5) {
						desc.stride = (2 * desc.width + 3) & ~3;
						desc.palette_size = 0;
						desc.format = PixelFormat::R8A8;
						desc.alpha_mode = AlphaMode::Straight;
						desc.origin = ScanOrigin::BottomLeft;
						frame = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
						desc = frame->GetDesc();
						uintptr channel_stride = desc.width * desc.height;
						if (data.GetLength() < 2 * channel_stride) throw InvalidFormatException();
						for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
							Color color;
							color.r = data[x + y * desc.width];
							color.g = data[x + y * desc.width + channel_stride];
							SetPixel(desc, x, desc.height - 1 - y, color.value);
						}
					} else throw InvalidFormatException();
				} else {
					if (!fdesc.data_pixel_format || !fdesc.data_stride) throw InvalidFormatException();
					source->Seek(fdesc.data_offset, SeekOrigin::Begin);
					auto data = source->ReadBlock(fdesc.data_size);
					desc.stride = fdesc.data_stride;
					desc.format = static_cast<PixelFormat>(fdesc.data_pixel_format);
					auto bpp = GetBitsPerPixel(desc.format);
					if (!bpp) throw InvalidFormatException();
					if (bpp * desc.width > 8 * desc.stride) throw InvalidFormatException();
					if (PixelFormatHasAlpha(desc.format)) {
						if (fdesc.data_compression & 32) desc.alpha_mode = AlphaMode::Premultiplied;
						else desc.alpha_mode = AlphaMode::Straight;
					} else desc.alpha_mode = AlphaMode::Undefined;
					if (fdesc.data_compression & 64) desc.origin = ScanOrigin::TopLeft;
					else desc.origin = ScanOrigin::BottomLeft;
					if (NeedsPalette(desc.format)) {
						if (data->GetLength() < 4) throw InvalidFormatException();
						desc.palette_size = *reinterpret_cast<const uint32 *>(data->GetBuffer());
						if (desc.palette_size > 0x100) throw InvalidFormatException();
						if (data->GetLength() < 4 * (desc.palette_size + 1)) throw InvalidFormatException();
						desc.palette = reinterpret_cast<Color *>(data->GetBuffer() + 4);
						desc.data = data->GetBuffer() + 4 * (desc.palette_size + 1);
						if (data->GetLength() < 4 * (desc.palette_size + 1) + desc.stride * desc.height) throw InvalidFormatException();
					} else {
						desc.palette_size = 0;
						desc.palette = 0;
						desc.data = data->GetBuffer();
						if (data->GetLength() < desc.stride * desc.height) throw InvalidFormatException();
					}
					frame = owrap(new Picture(desc, PictureInit::AllocateCopy));
					desc = frame->GetDesc();
					if (NeedsPalette(desc.format)) for (uint y = 0; y < desc.height; y++) for (uint x = 0; x < desc.width; x++) {
						auto index = GetPixel(desc, x, y);
						if (index >= desc.palette_size) throw InvalidFormatException();
					}
				}
				auto & attr = frame->GetAttributes();
				attr.plane = fdesc.plane;
				attr.pointer_offset_x = fdesc.cursor_position_x;
				attr.pointer_offset_y = fdesc.cursor_position_y;
				attr.animation_duration = fdesc.duration;
				attr.scale_factor = fdesc.scale_factor;
				result->Append(frame);
			}
			if (!result->GetLength()) throw InvalidArgumentException();
			return result;
		}
		bool VolumeCodecProbe(Codices::CodecIOProbe & prob)
		{
			if (prob.file_title_size >= sizeof(Format::ImageVolumeHeader)) {
				auto & hdr = *reinterpret_cast<Format::ImageVolumeHeader *>(&prob.file_title);
				if (Memory::MemoryCompare(&hdr.signature, "ecs.1.0", 8) || hdr.signature_ex != 0x80000006) return false;
				if (hdr.version > 1 || !hdr.frame_count) return false;
				prob.format = ImageFormatESSE;
				return true;
			} else return false;
		}
		bool VolumeCodecI(Codices::CodecIO mode, void * io, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				if (mode == Codices::CodecIO::Encode) {
					auto & enc = *reinterpret_cast<Codices::CodecIOEncode *>(io);
					if (!enc.stream || !enc.encode || !enc.encode->GetLength()) throw InvalidArgumentException();
					if (Memory::StringCompare(enc.format, ImageFormatESSE) == 0) {
						VolumeCodecEncode(enc.stream, enc.encode, enc.option_names, enc.option_values, enc.option_number);
					} else return false;
					return true;
				} else if (mode == Codices::CodecIO::Decode) {
					auto & dec = *reinterpret_cast<Codices::CodecIODecode *>(io);
					if (dec.format == ImageFormatESSE) {
						dec.decode = VolumeCodecDecode(dec.stream, dec.option_names, dec.option_values, dec.option_number);
						return true;
					} else return false;
				} else if (mode == Codices::CodecIO::Probe) {
					auto & prob = *reinterpret_cast<Codices::CodecIOProbe *>(io);
					return VolumeCodecProbe(prob);
				} else if (mode == Codices::CodecIO::EncodeFormats) {
					auto & efl = *reinterpret_cast<Codices::CodecIOEncodeFormats *>(io);
					efl.name = "ESSE Codificator Extensus";
					efl.caps.Append(ImageFormatESSE, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode | Codices::CodecIOMode::Multiframe);
					return true;
				} else if (mode == Codices::CodecIO::EncodeModes) {
					auto & eml = *reinterpret_cast<Codices::CodecIOEncodeModes *>(io);
					if (Memory::StringCompare(eml.format, ImageFormatESSE) == 0) {
						eml.pixel_formats.AddElement(PixelFormat::P8);
						eml.pixel_formats.AddElement(PixelFormat::A8);
						eml.pixel_formats.AddElement(PixelFormat::R8);
						eml.pixel_formats.AddElement(PixelFormat::R8A8);
						eml.pixel_formats.AddElement(PixelFormat::B8G8R8);
						eml.pixel_formats.AddElement(PixelFormat::B8G8R8A8);
						eml.options.Append(EncoderOptions::OverrideBitDepth, KeyValuePair<uint, uint>(8, 32));
						eml.options.Append(EncoderOptions::CompressionMode, KeyValuePair<uint, uint>(0, 1));
						eml.options.Append(EncoderOptions::CompressionQuality, KeyValuePair<uint, uint>(1, 100));
						eml.options.Append(DecoderOptions::MinimalDecodeScaleFactor, KeyValuePair<uint, uint>(0x10000, 0x100000));
						eml.options.Append(DecoderOptions::MaximalDecodeScaleFactor, KeyValuePair<uint, uint>(0x10000, 0x100000));
						return true;
					} else return false;
				} else ErrorSet(ectx, Errores::ErrorNotImplemented);
				return false;
			ESSE_TRY_OUTRO()
		}
		Codices::CodecIOFunction VolumeCodec = VolumeCodecI;
	}
}