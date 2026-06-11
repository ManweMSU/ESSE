#pragma once

#include <Cor/Images/CorImages.h>
#include <Cor/IO/CorStreams.h>
#include <Cor/Classes/CorVolume.hxx>

namespace ESSE
{
	namespace Picturae
	{
		constexpr const unichar8 * ImageFormatDIB				= "BMP";
		constexpr const unichar8 * ImageFormatPNG				= "PNG";
		constexpr const unichar8 * ImageFormatJPEG				= "JPEG";
		constexpr const unichar8 * ImageFormatGIF				= "GIF";
		constexpr const unichar8 * ImageFormatTIFF				= "TIFF";
		constexpr const unichar8 * ImageFormatHEIC				= "HEIC";
		constexpr const unichar8 * ImageFormatWEBP				= "WEBP";
		constexpr const unichar8 * ImageFormatDDS				= "DDS";
		constexpr const unichar8 * ImageFormatTGA				= "TGA";
		constexpr const unichar8 * ImageFormatWindowsIcon		= "ICO";
		constexpr const unichar8 * ImageFormatWindowsCursor		= "CUR";
		constexpr const unichar8 * ImageFormatAppleIcon			= "ICNS";
		constexpr const unichar8 * ImageFormatESSE				= "EFIE";

		namespace EncoderOptions
		{
			constexpr uint OverrideBitDepth					= 1;
			constexpr uint CompressionMode					= 2;
			constexpr uint CompressionQuality				= 3;
			constexpr uint CompressionChrominanceSubsample	= 4;

			string GetOptionName(uint optn);
		}
		namespace DecoderOptions
		{
			constexpr uint TransparentcyMaskFusionMode		= 0x10001;
			constexpr uint MinimalDecodeScaleFactor			= 0x10002;
			constexpr uint MaximalDecodeScaleFactor			= 0x10003;

			string GetOptionName(uint optn);
		}
		namespace Codices
		{
			enum class CodecIO : uint { Encode = 0, Decode = 1, Probe = 2, EncodeFormats = 3, EncodeModes = 4 };
			namespace CodecIOMode { constexpr uint Encode = 0x01; constexpr uint Decode = 0x02; constexpr uint Multiframe = 0x04; }

			struct CodecIOEncode
			{
				Stream * stream;
				const unichar8 * format;
				oref<Image> encode;
				const uint * option_names;
				const uint * option_values;
				uint option_number;
			};
			struct CodecIODecode
			{
				Stream * stream;
				ucs1_string format;
				oref<Image> decode;
				const uint * option_names;
				const uint * option_values;
				uint option_number;
			};
			struct CodecIOProbe
			{
				uintptr file_title_size;
				union {
					uint8 bytes[32];
					uint16 words[16];
					uint32 dwords[8];
					uint64 qwords[4];
				} file_title;
				ucs1_string format;
			};
			struct CodecIOEncodeFormats
			{
				ucs1_string name;
				Dictionary<ucs1_string, uint> caps;
			};
			struct CodecIOEncodeModes
			{
				ucs1_string format;
				Set<PixelFormat> pixel_formats;
				Dictionary<uint, KeyValuePair<uint, uint>> options;
			};

			typedef bool (* CodecIOFunction) (CodecIO mode, void * io, ErrorContext & ectx) noexcept;
		}

		void Encode(Stream * dest, Image * image, const unichar8 * format, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept;
		void Encode(Stream * dest, Image * image, const unichar8 * format, ErrorContext & ectx) noexcept;
		void Encode(Stream * dest, Picture * picture, const unichar8 * format, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept;
		void Encode(Stream * dest, Picture * picture, const unichar8 * format, ErrorContext & ectx) noexcept;
		void Encode(Stream * dest, Image * image, const unichar8 * format, uint * optn, uint * optv, uint nopt);
		void Encode(Stream * dest, Image * image, const unichar8 * format);
		void Encode(Stream * dest, Picture * picture, const unichar8 * format, uint * optn, uint * optv, uint nopt);
		void Encode(Stream * dest, Picture * picture, const unichar8 * format);

		oref<Image> DecodeImage(Stream * source, ucs1_string * format, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept;
		oref<Image> DecodeImage(Stream * source, ucs1_string * format, ErrorContext & ectx) noexcept;
		oref<Image> DecodeImage(Stream * source, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept;
		oref<Image> DecodeImage(Stream * source, ErrorContext & ectx) noexcept;
		oref<Picture> DecodePicture(Stream * source, ucs1_string * format, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept;
		oref<Picture> DecodePicture(Stream * source, ucs1_string * format, ErrorContext & ectx) noexcept;
		oref<Picture> DecodePicture(Stream * source, uint * optn, uint * optv, uint nopt, ErrorContext & ectx) noexcept;
		oref<Picture> DecodePicture(Stream * source, ErrorContext & ectx) noexcept;
		oref<Image> DecodeImage(Stream * source, ucs1_string * format, uint * optn, uint * optv, uint nopt);
		oref<Image> DecodeImage(Stream * source, ucs1_string * format);
		oref<Image> DecodeImage(Stream * source, uint * optn, uint * optv, uint nopt);
		oref<Image> DecodeImage(Stream * source);
		oref<Picture> DecodePicture(Stream * source, ucs1_string * format, uint * optn, uint * optv, uint nopt);
		oref<Picture> DecodePicture(Stream * source, ucs1_string * format);
		oref<Picture> DecodePicture(Stream * source, uint * optn, uint * optv, uint nopt);
		oref<Picture> DecodePicture(Stream * source);

		ucs1_string ProbeImageFileFormat(Stream * source, ErrorContext & ectx) noexcept;
		ucs1_string ProbeImageFileFormat(Stream * source);
		oref<array<Codices::CodecIOEncodeFormats>> GetEncodeFormats(ErrorContext & ectx) noexcept;
		oref<array<Codices::CodecIOEncodeFormats>> GetEncodeFormats(void);
		bool GetEncodeMode(Codices::CodecIOEncodeModes * dest, const unichar8 * format, ErrorContext & ectx) noexcept;
		bool GetEncodeMode(Codices::CodecIOEncodeModes * dest, const unichar8 * format);
	}
}