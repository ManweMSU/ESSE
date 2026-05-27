#pragma once

#include "../Classes/CorArray.hxx"

namespace ESSE
{
	struct Color
	{
		union {
			struct { uint8 r, g, b, a; };
			uint32 value;
		};

		Color(void) noexcept;
		Color(uint8 sr, uint8 sg, uint8 sb, uint8 sa = 0xFF) noexcept;
		Color(int sr, int sg, int sb, int sa = 0xFF) noexcept;
		Color(float sr, float sg, float sb, float sa = 1.0) noexcept;
		Color(double sr, double sg, double sb, double sa = 1.0) noexcept;
		Color(uint32 sv) noexcept;

		operator uint32 (void) const noexcept;
		operator string (void) const;

		bool friend operator == (const Color & a, const Color & b) noexcept;
		bool friend operator != (const Color & a, const Color & b) noexcept;
		bool friend operator < (const Color & a, const Color & b) noexcept;
		bool friend operator > (const Color & a, const Color & b) noexcept;
		bool friend operator <= (const Color & a, const Color & b) noexcept;
		bool friend operator >= (const Color & a, const Color & b) noexcept;
	};

	namespace Picturae
	{
		enum class PixelFormat : uint {
			// 32 bpp
			B8G8R8A8 = 0x04888800, R8G8B8A8 = 0x14888800, B8G8R8X8 = 0x04888008, R8G8B8X8 = 0x14888008,
			// 24 bpp
			B8G8R8 = 0x03888000, R8G8B8 = 0x13888000,
			// 16 bpp
			B5G5R5A1 = 0x04555100, B5G5R5X1 = 0x04555001, B5G6R5 = 0x03565000,
			R5G5B5A1 = 0x14555100, R5G5B5X1 = 0x14555001, R5G6B5 = 0x13565000,
			B4G4R4A4 = 0x04444400, B4G4R4X4 = 0x04444004, R4G4B4A4 = 0x14444400, R4G4B4X4 = 0x14444004,
			R8A8 = 0x02800800,
			// 8 bpp
			B2G3R2A1 = 0x04232100, B2G3R2X1 = 0x04232001, B2G3R3 = 0x03332000,
			R2G3B2A1 = 0x14232100, R2G3B2X1 = 0x14232001, R3G3B2 = 0x13332000,
			B2G2R2A2 = 0x04222200, B2G2R2X2 = 0x04222002, R2G2B2A2 = 0x14222200, R2G2B2X2 = 0x14222002,
			R4A4 = 0x02400400, A8 = 0x01000800, R8 = 0x01800000, P8 = 0x01000080,
			// 4 bpp
			R2A2 = 0x02200200, A4 = 0x01000400, R4 = 0x01400000, P4 = 0x01000040,
			// 2 bpp
			R1A1 = 0x02100100, A2 = 0x01000200, R2 = 0x01200000, P2 = 0x01000020,
			// 1 bpp
			A1 = 0x01000100, R1 = 0x01100000, P1 = 0x01000010
		};
		enum class AlphaMode : uint { Undefined = 0, Straight = 1, Premultiplied = 2 };
		enum class ScanOrigin : uint { TopLeft = 0, BottomLeft = 1 };
		enum class SystemPaletteType : uint { Unknown = 0, Grayscale_1bit = 1, Grayscale_2bit = 2, Grayscale_4bit = 3, Windows_4bit = 4, CGA_4bit = 5, Macintosh_4bit = 6, RGB685_8bit = 7, RGB685T_8bit = 8 };
		enum class PictureInit : uint { AllocateUninitialized = 0, AllocateZeroed = 1, AllocateCopy = 2, Refer = 3 };

		string GetPixelFormatName(PixelFormat format);
		bool PixelFormatHasRed(PixelFormat format) noexcept;
		bool PixelFormatHasGreen(PixelFormat format) noexcept;
		bool PixelFormatHasBlue(PixelFormat format) noexcept;
		bool PixelFormatHasAlpha(PixelFormat format) noexcept;
		bool NeedsPalette(PixelFormat format) noexcept;
		uint GetPaletteVolume(PixelFormat format) noexcept;
		uint GetBitsPerPixel(PixelFormat format) noexcept;
		uint ReadRedChannel(uint data, PixelFormat source_format, AlphaMode source_alpha, uint alpha) noexcept;
		uint ReadGreenChannel(uint data, PixelFormat source_format, AlphaMode source_alpha, uint alpha) noexcept;
		uint ReadBlueChannel(uint data, PixelFormat source_format, AlphaMode source_alpha, uint alpha) noexcept;
		uint ReadAlphaChannel(uint data, PixelFormat source_format) noexcept;
		uint MakePixelValue(uint r, uint g, uint b, uint a, PixelFormat format, AlphaMode alpha) noexcept;
		uint ConvertPixelValue(uint data, PixelFormat source_format, AlphaMode source_alpha, PixelFormat format, AlphaMode alpha) noexcept;

		struct PictureDesc
		{
			void * data;
			Color * palette;
			uint width, height, stride, palette_size;
			PixelFormat format;
			AlphaMode alpha_mode;
			ScanOrigin origin;
		};
		struct PictureExtendedAttributes
		{
			uint plane;
			uint animation_duration;
			uint pointer_offset_x;
			uint pointer_offset_y;
			double scale_factor;
		};

		void GeneratePalette(const PictureDesc & dest, SystemPaletteType type = SystemPaletteType::Unknown) noexcept;
		void BlockTransfer(const PictureDesc & dest, uint dest_x, uint dest_y, const PictureDesc & src, uint src_x, uint src_y, uint width, uint height) noexcept;
		void BlockTransfer(const PictureDesc & dest, const PictureDesc & src) noexcept;
		uint LookupPalette(const PictureDesc & desc, Color color) noexcept;
		uint GetPixel(const PictureDesc & desc, uint x, uint y) noexcept;
		void SetPixel(const PictureDesc & desc, uint x, uint y, uint v) noexcept;

		class Picture : public Object
		{
			bool _free;
			PictureDesc _desc;
			PictureExtendedAttributes _attr;
		public:
			Picture(const PictureDesc & desc, PictureInit init);
			Picture(const Picture * src);
			Picture(const Picture & src);
			Picture(Picture && src) noexcept;
			Picture(uint w, uint h, PixelFormat fmt);
			Picture(uint w, uint h, PixelFormat fmt, AlphaMode alpha);
			Picture(uint w, uint h, PixelFormat fmt, ScanOrigin origin);
			Picture(uint w, uint h, PixelFormat fmt, AlphaMode alpha, ScanOrigin origin);
			virtual ~Picture(void) override;
			virtual string ToStringE(ErrorContext & ectx) const noexcept override;

			void Dither(const PictureDesc & dest) const;
			oref<Picture> Convert(const PictureDesc & desc) const;
			oref<Picture> Convert(PixelFormat fmt) const;
			oref<Picture> Convert(PixelFormat fmt, AlphaMode alpha) const;
			oref<Picture> Convert(PixelFormat fmt, ScanOrigin origin) const;
			oref<Picture> Convert(PixelFormat fmt, AlphaMode alpha, ScanOrigin origin) const;

			const PictureDesc & GetDesc(void) const noexcept;
			PictureExtendedAttributes & GetAttributes(void) noexcept;
			const PictureExtendedAttributes & GetAttributes(void) const noexcept;

			uint GetPixel(uint x, uint y) const noexcept;
			void SetPixel(uint x, uint y, uint v) const noexcept;
			uint LookupPalette(Color color) const noexcept;
			Color ReadPixel(uint x, uint y) const noexcept;
			void WritePixel(uint x, uint y, Color color) const noexcept;
		};
		class Image : public object_array<Picture>
		{
		public:
			Image(void) noexcept;
			Picture * FindBestSizeMatch(uint width, uint height) noexcept;
			Picture * FindExactSizeMatch(uint width, uint height) noexcept;
			Picture * FindBestScaleMatch(double scale) noexcept;
			Picture * FindPlaneMatch(uint plane) noexcept;
		};

		oref<Picture> AlphaColorFuse(Picture * color, Picture * alpha);
		void AlphaColorFuse(Image * image);
	}
}