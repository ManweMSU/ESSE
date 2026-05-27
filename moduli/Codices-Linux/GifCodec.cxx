#include <Imagines/Imagines.h>
#include "giflib/gif_lib.h"

using namespace ESSE::Picturae;

namespace ESSE
{
	namespace Linux
	{
		struct GifEsseIO
		{
			Stream * stream;
			ErrorContext last_error;
		};
		int GifLibRead(GifFileType * file, GifByteType * data, int size) noexcept
		{
			auto io = reinterpret_cast<GifEsseIO *>(file->UserData);
			auto read = io->stream->ReadE(data, size, io->last_error);
			if (ErrorTest(io->last_error)) return -1;
			return read;
		}
		int GifLibWrite(GifFileType * file, const GifByteType * data, int size) noexcept
		{
			auto io = reinterpret_cast<GifEsseIO *>(file->UserData);
			auto written = io->stream->WriteE(data, size, io->last_error);
			if (ErrorTest(io->last_error)) return -1;
			return written;
		}
	
		void GifCodecEncode(Stream * dest, Image * src)
		{
			GifEsseIO io;
			io.stream = dest;
			ErrorClear(io.last_error);
			auto gif = EGifOpen(&io, GifLibWrite, 0);
			if (!gif) throw OutOfMemoryException();
			auto & f0 = src->ElementAt(0);
			if (EGifPutScreenDesc(gif, f0.GetDesc().width, f0.GetDesc().height, 8, 0, 0) != GIF_OK) {
				EGifCloseFile(gif, 0);
				throw OutOfMemoryException();
			}
			try {
				array<GifColorType> plt(0x100);
				plt.SetLength(0x100);
				Memory::ZeroMemory(plt.GetBuffer(), plt.GetLength() * sizeof(GifColorType));
				ColorMapObject color_map;
				color_map.SortFlag = false;
				color_map.Colors = plt.GetBuffer();
				GraphicsControlBlock gc_block;
				gc_block.DisposalMode = DISPOSE_BACKGROUND;
				gc_block.UserInputFlag = false;
				GifByteType gc_block_data[0x100];
				for (auto & f : *src) {
					if (f.GetDesc().width != f0.GetDesc().width || f.GetDesc().height != f0.GetDesc().height) continue;
					PictureDesc conv_desc = f.GetDesc();
					conv_desc.stride = conv_desc.width;
					conv_desc.format = PixelFormat::P8;
					conv_desc.alpha_mode = AlphaMode::Straight;
					conv_desc.origin = ScanOrigin::TopLeft;
					conv_desc.palette_size = 0x100;
					auto conv = owrap(new Picture(conv_desc, PictureInit::AllocateUninitialized));
					int color_count = conv->GetDesc().palette_size;
					int transparent = -1;
					if (f.GetDesc().format == PixelFormat::P8) {
						Memory::ZeroMemory(conv->GetDesc().palette, conv->GetDesc().palette_size * sizeof(Color));
						Memory::MemoryCopy(conv->GetDesc().palette, f.GetDesc().palette, f.GetDesc().palette_size * sizeof(Color));
						for (int i = 0; i < color_count; i++) {
							auto pc = conv->GetDesc().palette[i];
							if (pc.a < 128) {
								if (transparent < 0) transparent = i;
								plt[i].Red = plt[i].Green = plt[i].Blue = 0;
							} else { plt[i].Red = pc.r; plt[i].Green = pc.g; plt[i].Blue = pc.b; }
						}
						for (uint y = 0; y < f.GetDesc().height; y++) for (uint x = 0; x < f.GetDesc().width; x++) {
							auto index = f.GetPixel(x, y);
							if (f.GetDesc().palette[index].a < 128) {
								conv->SetPixel(x, y, transparent);
							} else {
								conv->SetPixel(x, y, index);
							}
						}
					} else {
						GeneratePalette(conv->GetDesc(), SystemPaletteType::RGB685T_8bit);
						BlockTransfer(conv->GetDesc(), f.GetDesc());
						for (int i = 0; i < color_count; i++) {
							auto pc = conv->GetDesc().palette[i];
							if (pc.a < 128) {
								if (transparent < 0) transparent = i;
								plt[i].Red = plt[i].Green = plt[i].Blue = 0;
							} else { plt[i].Red = pc.r; plt[i].Green = pc.g; plt[i].Blue = pc.b; }
						}
					}
					color_map.ColorCount = color_count;
					color_map.BitsPerPixel = 0;
					while (color_count > (1 << color_map.BitsPerPixel)) color_map.BitsPerPixel++;
					gc_block.TransparentColor = transparent;
					gc_block.DelayTime = f.GetAttributes().animation_duration / 10;
					int gc_size = EGifGCBToExtension(&gc_block, gc_block_data);
					if (EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE, gc_size, gc_block_data) != GIF_OK) throw OutOfMemoryException();
					if (EGifPutImageDesc(gif, 0, 0, f0.GetDesc().width, f0.GetDesc().height, false, &color_map) != GIF_OK) throw OutOfMemoryException();
					for (uint i = 0; i < conv->GetDesc().height; i++) {
						auto data = reinterpret_cast<uint8 *>(conv->GetDesc().data);
						if (EGifPutLine(gif, data + i * conv->GetDesc().stride, conv->GetDesc().width) != GIF_OK) throw OutOfMemoryException();
					}
				}
			} catch (...) { EGifCloseFile(gif, 0); throw; }
			if (EGifCloseFile(gif, 0) != GIF_OK) throw OutOfMemoryException();
		}
		oref<Image> GifCodecDecode(Stream * stream)
		{
			if (!stream) throw InvalidArgumentException();
			stream->Seek(0, SeekOrigin::Begin);
			GifEsseIO io;
			io.stream = stream;
			ErrorClear(io.last_error);
			auto result = owrap(new Image);
			auto gif = DGifOpen(&io, GifLibRead, 0);
			if (!gif) throw InvalidFormatException();
			try {
				if (DGifSlurp(gif) != GIF_OK) throw InvalidFormatException();
				array<uint32> plt_global(0x100), plt_local(0x100);
				plt_global.SetLength(0x100);
				plt_local.SetLength(0x100);
				Memory::ZeroMemory(plt_global.GetBuffer(), 0x400);
				Memory::ZeroMemory(plt_local.GetBuffer(), 0x400);
				if (gif->SColorMap) {
					for (int i = 0; i < gif->SColorMap->ColorCount; i++) {
						uint32 clr = 0xFF000000;
						clr |= int(gif->SColorMap->Colors[i].Blue) << 16;
						clr |= int(gif->SColorMap->Colors[i].Green) << 8;
						clr |= int(gif->SColorMap->Colors[i].Red);
						plt_global[i] = clr;
					}
				}
				oref<Picture> prev_frame;
				int image_blt_mode = 0;
				for (int i = 0; i < gif->ImageCount; i++) {
					auto & img = gif->SavedImages[i];
					int local_transparent = -1;
					int image_next_blt_mode = 0;
					int animation_time = 0;
					bool use_local_plt = false;
					uint32 local_transparent_restore;
					for (int j = 0; j < img.ExtensionBlockCount; j++) {
						auto & ext = img.ExtensionBlocks[j];
						if (ext.Function == GRAPHICS_EXT_FUNC_CODE) {
							GraphicsControlBlock control;
							DGifExtensionToGCB(ext.ByteCount, ext.Bytes, &control);
							local_transparent = control.TransparentColor;
							image_next_blt_mode = (control.DisposalMode < 2) ? 1 : 0;
							animation_time = control.DelayTime * 10;
							break;
						}
					}
					if (img.ImageDesc.ColorMap) {
						for (int j = 0; j < img.ImageDesc.ColorMap->ColorCount; j++) {
							uint32 clr = 0xFF000000;
							clr |= int(img.ImageDesc.ColorMap->Colors[j].Blue) << 16;
							clr |= int(img.ImageDesc.ColorMap->Colors[j].Green) << 8;
							clr |= int(img.ImageDesc.ColorMap->Colors[j].Red);
							plt_local[j] = clr;
						}
						if (local_transparent >= 0 && local_transparent < 0x100) plt_local[local_transparent] = 0;
						use_local_plt = true;
					} else {
						use_local_plt = false;
						if (local_transparent >= 0 && local_transparent < 0x100) { local_transparent_restore = plt_global[local_transparent]; plt_global[local_transparent] = 0; }
					}
					PictureDesc desc;
					desc.width = img.ImageDesc.Width;
					desc.height = img.ImageDesc.Height;
					desc.stride = desc.width;
					desc.format = PixelFormat::P8;
					desc.alpha_mode = AlphaMode::Straight;
					desc.origin = ScanOrigin::TopLeft;
					desc.palette_size = 0x100;
					auto current = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
					Memory::MemoryCopy(current->GetDesc().data, img.RasterBits, img.ImageDesc.Width * img.ImageDesc.Height);
					Memory::MemoryCopy(current->GetDesc().palette, use_local_plt ? plt_local.GetBuffer() : plt_global.GetBuffer(), 0x400);
					if (image_blt_mode && prev_frame) {
						prev_frame->GetAttributes().animation_duration = animation_time;
						for (uint y = 0; y < prev_frame->GetDesc().height; y++) for (uint x = 0; x < prev_frame->GetDesc().width; x++) {
							if (x >= img.ImageDesc.Left && y >= img.ImageDesc.Top && x < img.ImageDesc.Left + img.ImageDesc.Width && y < img.ImageDesc.Top + img.ImageDesc.Height) {
								auto src = current->ReadPixel(x - img.ImageDesc.Left, y - img.ImageDesc.Top);
								if (src & 0xFF000000) prev_frame->SetPixel(x, y, src);
							}
						}
						result->Append(prev_frame);
					} else if (img.ImageDesc.Width != gif->SWidth || img.ImageDesc.Height != gif->SHeight || img.ImageDesc.Top || img.ImageDesc.Left) {
						PictureDesc desc;
						desc.width = gif->SWidth;
						desc.height = gif->SHeight;
						desc.stride = 4 * desc.width;
						desc.format = PixelFormat::R8G8B8A8;
						desc.alpha_mode = AlphaMode::Straight;
						desc.origin = ScanOrigin::TopLeft;
						desc.palette_size = 0;
						auto sum = owrap(new Picture(desc, PictureInit::AllocateZeroed));
						sum->GetAttributes().animation_duration = animation_time;
						for (uint y = 0; y < sum->GetDesc().height; y++) for (uint x = 0; x < sum->GetDesc().width; x++) {
							if (x >= img.ImageDesc.Left && y >= img.ImageDesc.Top && x < img.ImageDesc.Left + img.ImageDesc.Width && y < img.ImageDesc.Top + img.ImageDesc.Height) {
								sum->SetPixel(x, y, current->ReadPixel(x - img.ImageDesc.Left, y - img.ImageDesc.Top));
							}
						}
						result->Append(sum);
					} else {
						current->GetAttributes().animation_duration = animation_time;
						result->Append(current);
					}
					if (image_next_blt_mode) {
						PictureDesc desc = result->LastElement().GetDesc();
						desc.stride = desc.width;
						desc.format = PixelFormat::R8G8B8A8;
						desc.palette_size = 0;
						prev_frame = result->LastElement().Convert(desc);
					} else prev_frame.Clear();
					if (!use_local_plt && local_transparent >= 0 && local_transparent < 0x100) plt_global[local_transparent] = local_transparent_restore;
					image_blt_mode = image_next_blt_mode;
				}
			} catch (...) { DGifCloseFile(gif, 0); throw; }
			if (DGifCloseFile(gif, 0) != GIF_OK) throw OutOfMemoryException();
			if (!result->GetLength()) throw InvalidFormatException();
			return result;
		}
		bool GifCodecProbe(Codices::CodecIOProbe & prob)
		{
			uint64 signature = 0;
			if (prob.file_title_size >= 6) signature = prob.file_title.qwords[0] & 0xFFFFFFFFFFFFULL;
			if (signature == 0x613938464947 || signature == 0x613738464947) return true;
			return false;
		}
		bool GifCodecI(Codices::CodecIO mode, void * io, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				if (mode == Codices::CodecIO::Encode) {
					auto & enc = *reinterpret_cast<Codices::CodecIOEncode *>(io);
					if (Memory::StringCompare(enc.format, ImageFormatGIF)) return false;
					if (!enc.stream || !enc.encode || !enc.encode->GetLength()) throw InvalidArgumentException();
					GifCodecEncode(enc.stream, enc.encode);
					return true;
				} else if (mode == Codices::CodecIO::Decode) {
					auto & dec = *reinterpret_cast<Codices::CodecIODecode *>(io);
					auto result = GifCodecDecode(dec.stream);
					if (result) { dec.decode = result; dec.format = ImageFormatGIF; return true; } else return false;
				} else if (mode == Codices::CodecIO::Probe) {
					auto & prob = *reinterpret_cast<Codices::CodecIOProbe *>(io);
					if (GifCodecProbe(prob)) { prob.format = ImageFormatGIF; return true; }
					else return false;
				} else if (mode == Codices::CodecIO::EncodeFormats) {
					auto & efl = *reinterpret_cast<Codices::CodecIOEncodeFormats *>(io);
					efl.name = "giflib";
					efl.caps.Append(ImageFormatGIF, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode | Codices::CodecIOMode::Multiframe);
					return true;
				} else if (mode == Codices::CodecIO::EncodeModes) {
					auto & eml = *reinterpret_cast<Codices::CodecIOEncodeModes *>(io);
					if (Memory::StringCompare(eml.format, ImageFormatGIF)) return false;
					eml.pixel_formats.AddElement(PixelFormat::P8);
					return true;
				} else ErrorSet(ectx, Errores::ErrorNotImplemented);
				return false;
			ESSE_TRY_OUTRO()
		}
		Codices::CodecIOFunction giflib = GifCodecI;
	}
}