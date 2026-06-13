#include "Imagines.h"

#define ESSE_CODICES_IMPORT_TYPE ::ESSE::Picturae::Codices::CodecIOFunction
#ifndef ESSE_CODICES_IMAGINES_IMPORT
#define ESSE_CODICES_IMAGINES_IMPORT
#endif
#ifndef ESSE_CODICES_IMAGINES_LIST
#define ESSE_CODICES_IMAGINES_LIST
#endif

ESSE_CODICES_IMAGINES_IMPORT

namespace ESSE
{
	namespace Picturae
	{
		Codices::CodecIOFunction * _codices[] = { ESSE_CODICES_IMAGINES_LIST };

		string __GetOptionName(uint optn)
		{
			if (optn == EncoderOptions::OverrideBitDepth) return U"Redefini Altum Puncti";
			else if (optn == EncoderOptions::CompressionMode) return U"Modus Compressionis";
			else if (optn == EncoderOptions::CompressionQuality) return U"Qualitas Compressionis";
			else if (optn == EncoderOptions::CompressionChrominanceSubsample) return U"Subtilitas Chromae";
			else if (optn == DecoderOptions::TransparentcyMaskFusionMode) return U"Modus Fusionis";
			else if (optn == DecoderOptions::MinimalDecodeScaleFactor) return U"Scala Minima";
			else if (optn == DecoderOptions::MaximalDecodeScaleFactor) return U"Scala Maxima";
			else return U"Ignota";
		}
		string EncoderOptions::GetOptionName(uint optn) { return __GetOptionName(optn); }
		string DecoderOptions::GetOptionName(uint optn) { return __GetOptionName(optn); }

		void Encode(Stream * dest, Image * image, const unichar8 * format, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept
		{
			Codices::CodecIOEncode io;
			io.stream = dest;
			io.format = format;
			io.encode = image;
			io.option_names = optn;
			io.option_values = optv;
			io.option_number = nopt;
			for (auto & cdx : _codices) {
				if ((*cdx)(Codices::CodecIO::Encode, &io, ectx)) return;
				if (ErrorTest(ectx)) return;
			}
			ErrorSet(ectx, Errores::ErrorNotImplemented);
		}
		void Encode(Stream * dest, Image * image, const unichar8 * format, ErrorContext & ectx) noexcept { Encode(dest, image, format, 0, 0, 0, ectx); }
		void Encode(Stream * dest, Picture * picture, const unichar8 * format, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			Image im;
			im.Append(picture);
			Encode(dest, &im, format, optn, optv, nopt, ectx);
			ESSE_TRY_OUTRO();
		}
		void Encode(Stream * dest, Picture * picture, const unichar8 * format, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			Image im;
			im.Append(picture);
			Encode(dest, &im, format, 0, 0, 0, ectx);
			ESSE_TRY_OUTRO();
		}
		void Encode(Stream * dest, Image * image, const unichar8 * format, uint * optn, uint * optv, uint nopt)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Encode(dest, image, format, optn, optv, nopt, ectx);
			ErrorThrow(ectx);
		}
		void Encode(Stream * dest, Image * image, const unichar8 * format)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Encode(dest, image, format, 0, 0, 0, ectx);
			ErrorThrow(ectx);
		}
		void Encode(Stream * dest, Picture * picture, const unichar8 * format, uint * optn, uint * optv, uint nopt)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Encode(dest, picture, format, optn, optv, nopt, ectx);
			ErrorThrow(ectx);
		}
		void Encode(Stream * dest, Picture * picture, const unichar8 * format)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Encode(dest, picture, format, 0, 0, 0, ectx);
			ErrorThrow(ectx);
		}

		Codices::CodecIOFunction ProbeImageFileFormatF(Stream * source, ucs1_string & ff, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				if (!source) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
				Codices::CodecIOProbe io;
				source->SeekE(0, SeekOrigin::Begin, ectx);
				if (ErrorTest(ectx)) return 0;
				io.file_title_size = source->ReadE(&io.file_title, sizeof(io.file_title), ectx);
				if (ErrorTest(ectx)) return 0;
				for (auto & cdx : _codices) {
					bool probed = (*cdx)(Codices::CodecIO::Probe, &io, ectx);
					if (ErrorTest(ectx)) return 0;
					if (probed) { ff = io.format; return (*cdx); }
				}
				return 0;
			ESSE_TRY_OUTRO(0)
		}
		oref<Image> DecodeImage(Stream * source, ucs1_string * format, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				Codices::CodecIODecode io;
				io.stream = source;
				io.option_names = optn;
				io.option_values = optv;
				io.option_number = nopt;
				auto decoder = ProbeImageFileFormatF(source, io.format, ectx);
				if (ErrorTest(ectx)) return 0;
				if (!decoder) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
				bool decoded = decoder(Codices::CodecIO::Decode, &io, ectx);
				if (ErrorTest(ectx)) return 0;
				if (!decoded || !io.decode) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
				if (format) *format = io.format;
				return io.decode;
			ESSE_TRY_OUTRO(0)
		}
		oref<Image> DecodeImage(Stream * source, ucs1_string * format, ErrorContext & ectx) noexcept { return DecodeImage(source, format, 0, 0, 0, ectx); }
		oref<Image> DecodeImage(Stream * source, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept { return DecodeImage(source, 0, optn, optv, nopt, ectx); }
		oref<Image> DecodeImage(Stream * source, ErrorContext & ectx) noexcept { return DecodeImage(source, 0, ectx); }
		oref<Picture> DecodePicture(Stream * source, ucs1_string * format, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept
		{
			auto result = DecodeImage(source, format, optn, optv, nopt, ectx);
			if (ErrorTest(ectx)) return 0;
			uintptr index = 0;
			double scale = 0.0;
			uint bpp = 0;
			uint plane = int(-1);
			for (uintptr i = 0; i < result->GetLength(); i++) {
				auto & f = result->ElementAt(i);
				auto s = f.GetAttributes().scale_factor;
				auto b = GetBitsPerPixel(f.GetDesc().format);
				auto p = f.GetAttributes().plane;
				if (p < plane || (p == plane && s > scale) || (p == plane && s == scale && b > bpp)) {
					scale = s; bpp = b; plane = p;
					index = i;
				}
			}
			return result->ReferenceAt(index);
		}
		oref<Picture> DecodePicture(Stream * source, ucs1_string * format, ErrorContext & ectx) noexcept { return DecodePicture(source, format, 0, 0, 0, ectx); }
		oref<Picture> DecodePicture(Stream * source, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept { return DecodePicture(source, 0, optn, optv, nopt, ectx); }
		oref<Picture> DecodePicture(Stream * source, ErrorContext & ectx) noexcept { return DecodePicture(source, 0, ectx); }
		oref<Image> DecodeImage(Stream * source, ucs1_string * format, uint * optn, uint * optv, uint nopt)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = DecodeImage(source, format, optn, optv, nopt, ectx);
			ErrorThrow(ectx); return result;
		}
		oref<Image> DecodeImage(Stream * source, ucs1_string * format)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = DecodeImage(source, format, ectx);
			ErrorThrow(ectx); return result;
		}
		oref<Image> DecodeImage(Stream * source, uint * optn, uint * optv, uint nopt)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = DecodeImage(source, 0, optn, optv, nopt, ectx);
			ErrorThrow(ectx); return result;
		}
		oref<Image> DecodeImage(Stream * source)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = DecodeImage(source, 0, ectx);
			ErrorThrow(ectx); return result;
		}
		oref<Picture> DecodePicture(Stream * source, ucs1_string * format, uint * optn, uint * optv, uint nopt)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = DecodePicture(source, format, optn, optv, nopt, ectx);
			ErrorThrow(ectx); return result;
		}
		oref<Picture> DecodePicture(Stream * source, ucs1_string * format)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = DecodePicture(source, format, ectx);
			ErrorThrow(ectx); return result;
		}
		oref<Picture> DecodePicture(Stream * source, uint * optn, uint * optv, uint nopt)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = DecodePicture(source, 0, optn, optv, nopt, ectx);
			ErrorThrow(ectx); return result;
		}
		oref<Picture> DecodePicture(Stream * source)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = DecodePicture(source, 0, ectx);
			ErrorThrow(ectx); return result;
		}

		ucs1_string ProbeImageFileFormat(Stream * source, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				if (!source) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return ucs1_string(); }
				Codices::CodecIOProbe io;
				source->SeekE(0, SeekOrigin::Begin, ectx);
				if (ErrorTest(ectx)) return ucs1_string();
				io.file_title_size = source->ReadE(&io.file_title, sizeof(io.file_title), ectx);
				if (ErrorTest(ectx)) return ucs1_string();
				for (auto & cdx : _codices) {
					bool probed = (*cdx)(Codices::CodecIO::Probe, &io, ectx);
					if (ErrorTest(ectx)) return ucs1_string();
					if (probed) break;
				}
				return io.format;
			ESSE_TRY_OUTRO(ucs1_string())
		}
		ucs1_string ProbeImageFileFormat(Stream * source)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = ProbeImageFileFormat(source, ectx);
			ErrorThrow(ectx); return result;
		}
		oref<array<Codices::CodecIOEncodeFormats>> GetEncodeFormats(ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			auto result = owrap(new array<Codices::CodecIOEncodeFormats>(0x20));
			for (auto & cdx : _codices) {
				Codices::CodecIOEncodeFormats caps;
				auto status = (*cdx)(Codices::CodecIO::EncodeFormats, &caps, ectx);
				if (ErrorTest(ectx)) return 0;
				if (status) result->Append(caps);
			}
			return result;
			ESSE_TRY_OUTRO(0)
		}
		oref<array<Codices::CodecIOEncodeFormats>> GetEncodeFormats(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = GetEncodeFormats(ectx);
			ErrorThrow(ectx); return result;
		}
		bool GetEncodeMode(Codices::CodecIOEncodeModes * dest, const unichar8 * format, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			Codices::CodecIOEncodeModes local;
			local.format = format;
			bool result = false;
			for (auto & cdx : _codices) {
				auto status = (*cdx)(Codices::CodecIO::EncodeModes, &local, ectx);
				if (ErrorTest(ectx)) return false;
				if (status) { result = true; break; }
			}
			if (dest && result) *dest = local;
			return result;
			ESSE_TRY_OUTRO(false)
		}
		bool GetEncodeMode(Codices::CodecIOEncodeModes * dest, const unichar8 * format)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = GetEncodeMode(dest, format, ectx);
			ErrorThrow(ectx); return result;
		}
	}
}