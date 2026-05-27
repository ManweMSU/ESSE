#include "CorImages.h"

#include <math.h>

namespace ESSE
{
	Color::Color(void) noexcept {}
	Color::Color(uint8 sr, uint8 sg, uint8 sb, uint8 sa) noexcept : r(sr), g(sg), b(sb), a(sa) {}
	Color::Color(int sr, int sg, int sb, int sa) noexcept : r(sr), g(sg), b(sb), a(sa) {}
	Color::Color(float sr, float sg, float sb, float sa) noexcept : r(max(min(int(sr * 255.0f), 255), 0)), g(max(min(int(sg * 255.0f), 255), 0)), b(max(min(int(sb * 255.0f), 255), 0)), a(max(min(int(sa * 255.0f), 255), 0)) {}
	Color::Color(double sr, double sg, double sb, double sa) noexcept : r(max(min(int(sr * 255.0), 255), 0)), g(max(min(int(sg * 255.0), 255), 0)), b(max(min(int(sb * 255.0), 255), 0)), a(max(min(int(sa * 255.0), 255), 0)) {}
	Color::Color(uint32 sv) noexcept : value(sv) {}
	Color::operator uint32 (void) const noexcept { return value; }
	Color::operator string (void) const { return string(value, HexadecimalBase, 8); }

	bool operator == (const Color & a, const Color & b) noexcept { return a.value == b.value; }
	bool operator != (const Color & a, const Color & b) noexcept { return a.value != b.value; }
	bool operator < (const Color & a, const Color & b) noexcept { return a.value < b.value; }
	bool operator > (const Color & a, const Color & b) noexcept { return a.value > b.value; }
	bool operator <= (const Color & a, const Color & b) noexcept { return a.value <= b.value; }
	bool operator >= (const Color & a, const Color & b) noexcept { return a.value >= b.value; }

	namespace Picturae
	{
		constexpr uint grayscale_weight_r = 54;
		constexpr uint grayscale_weight_g = 182;
		constexpr uint grayscale_weight_b = 19;

		string GetPixelFormatName(PixelFormat format)
		{
			uint pxf = uint(format);
			dynamic_string_ucs4 result;
			if (pxf & 0xF0000000) {
				if (pxf & 0xF00000) { result << U'R' << (U'0' + ((pxf & 0xF00000) >> 20)); }
				if (pxf & 0x0F0000) { result << U'G' << (U'0' + ((pxf & 0x0F0000) >> 16)); }
				if (pxf & 0x00F000) { result << U'B' << (U'0' + ((pxf & 0x00F000) >> 12)); }
			} else {
				if (pxf & 0x00F000) { result << U'B' << (U'0' + ((pxf & 0x00F000) >> 12)); }
				if (pxf & 0x0F0000) { result << U'G' << (U'0' + ((pxf & 0x0F0000) >> 16)); }
				if (pxf & 0xF00000) { result << U'R' << (U'0' + ((pxf & 0xF00000) >> 20)); }
			}
			if (pxf & 0xF00) { result << U'A' << (U'0' + ((pxf & 0xF00) >> 8)); }
			if (pxf & 0x0F0) { result << U'P' << (U'0' + ((pxf & 0x0F0) >> 4)); }
			if (pxf & 0x00F) { result << U'X' << (U'0' + ((pxf & 0x00F) >> 0)); }
			return result;
		}
		bool PixelFormatHasRed(PixelFormat format) noexcept { return (uint(format) & 0xF00000) != 0; }
		bool PixelFormatHasGreen(PixelFormat format) noexcept { return (uint(format) & 0x0F0000) != 0; }
		bool PixelFormatHasBlue(PixelFormat format) noexcept { return (uint(format) & 0x00F000) != 0; }
		bool PixelFormatHasAlpha(PixelFormat format) noexcept { return (uint(format) & 0xF00) != 0; }
		bool NeedsPalette(PixelFormat format) noexcept { return (uint(format) & 0x0F0) != 0; }
		uint GetPaletteVolume(PixelFormat format) noexcept { return uint(1) << GetBitsPerPixel(format); }
		uint GetBitsPerPixel(PixelFormat format) noexcept
		{
			if (format == PixelFormat::B8G8R8A8) return 32;
			else if (format == PixelFormat::B8G8R8X8) return 32;
			else if (format == PixelFormat::R8G8B8A8) return 32;
			else if (format == PixelFormat::R8G8B8X8) return 32;
			else if (format == PixelFormat::B8G8R8) return 24;
			else if (format == PixelFormat::R8G8B8) return 24;
			else if (format == PixelFormat::B5G5R5A1) return 16;
			else if (format == PixelFormat::B5G5R5X1) return 16;
			else if (format == PixelFormat::B5G6R5) return 16;
			else if (format == PixelFormat::R5G5B5A1) return 16;
			else if (format == PixelFormat::R5G5B5X1) return 16;
			else if (format == PixelFormat::R5G6B5) return 16;
			else if (format == PixelFormat::B4G4R4A4) return 16;
			else if (format == PixelFormat::B4G4R4X4) return 16;
			else if (format == PixelFormat::R4G4B4A4) return 16;
			else if (format == PixelFormat::R4G4B4X4) return 16;
			else if (format == PixelFormat::R8A8) return 16;
			else if (format == PixelFormat::B2G3R2A1) return 8;
			else if (format == PixelFormat::B2G3R2X1) return 8;
			else if (format == PixelFormat::B2G3R3) return 8;
			else if (format == PixelFormat::R2G3B2A1) return 8;
			else if (format == PixelFormat::R2G3B2X1) return 8;
			else if (format == PixelFormat::R3G3B2) return 8;
			else if (format == PixelFormat::B2G2R2A2) return 8;
			else if (format == PixelFormat::B2G2R2X2) return 8;
			else if (format == PixelFormat::R2G2B2A2) return 8;
			else if (format == PixelFormat::R2G2B2X2) return 8;
			else if (format == PixelFormat::R4A4) return 8;
			else if (format == PixelFormat::A8) return 8;
			else if (format == PixelFormat::R8) return 8;
			else if (format == PixelFormat::P8) return 8;
			else if (format == PixelFormat::R2A2) return 4;
			else if (format == PixelFormat::A4) return 4;
			else if (format == PixelFormat::R4) return 4;
			else if (format == PixelFormat::P4) return 4;
			else if (format == PixelFormat::R1A1) return 2;
			else if (format == PixelFormat::A2) return 2;
			else if (format == PixelFormat::R2) return 2;
			else if (format == PixelFormat::P2) return 2;
			else if (format == PixelFormat::A1) return 1;
			else if (format == PixelFormat::R1) return 1;
			else if (format == PixelFormat::P1) return 1;
			else return 0;
		}
		uint ReadRedChannel(uint data, PixelFormat source_format, AlphaMode source_alpha, uint alpha) noexcept
		{
			uint ev = 0;
			if (source_format == PixelFormat::A8 || source_format == PixelFormat::A4) return 255;
			else if (source_format == PixelFormat::A2 || source_format == PixelFormat::A1) return 255;
			if (source_format == PixelFormat::B8G8R8A8 || source_format == PixelFormat::B8G8R8) ev = (data & 0x00FF0000) >> 16;
			else if (source_format == PixelFormat::R8G8B8A8 || source_format == PixelFormat::R8G8B8) ev = (data & 0x000000FF);
			else if (source_format == PixelFormat::B8G8R8X8) ev = (data & 0x00FF0000) >> 16;
			else if (source_format == PixelFormat::R8G8B8X8) ev = (data & 0x000000FF);
			else if (source_format == PixelFormat::B5G5R5A1 || source_format == PixelFormat::B5G5R5X1) ev = ((data & 0x7C00) >> 10) * 255 / 31;
			else if (source_format == PixelFormat::B5G6R5) ev = ((data & 0xF800) >> 11) * 255 / 31;
			else if (source_format == PixelFormat::R5G5B5A1 || source_format == PixelFormat::R5G5B5X1 || source_format == PixelFormat::R5G6B5) ev = (data & 0x001F) * 255 / 31;
			else if (source_format == PixelFormat::B4G4R4A4 || source_format == PixelFormat::B4G4R4X4) ev = ((data & 0x0F00) >> 8) * 255 / 15;
			else if (source_format == PixelFormat::R4G4B4A4 || source_format == PixelFormat::R4G4B4X4) ev = (data & 0x000F) * 255 / 15;
			else if (source_format == PixelFormat::R8 || source_format == PixelFormat::R8A8) ev = data & 0xFF;
			else if (source_format == PixelFormat::R4 || source_format == PixelFormat::R4A4) ev = (data & 0x0F) * 255 / 15;
			else if (source_format == PixelFormat::R2 || source_format == PixelFormat::R2A2) ev = (data & 0x3) * 255 / 3;
			else if (source_format == PixelFormat::R1 || source_format == PixelFormat::R1A1) ev = (data & 0x1) * 255;
			else if (source_format == PixelFormat::B2G3R2A1 || source_format == PixelFormat::B2G3R2X1) ev = ((data & 0x60) >> 5) * 255 / 3;
			else if (source_format == PixelFormat::R2G3B2A1 || source_format == PixelFormat::R2G3B2X1) ev = (data & 0x03) * 255 / 3;
			else if (source_format == PixelFormat::R2G2B2A2 || source_format == PixelFormat::R2G2B2X2) ev = (data & 0x03) * 255 / 3;
			else if (source_format == PixelFormat::B2G2R2A2 || source_format == PixelFormat::B2G2R2X2) ev = ((data & 0x30) >> 4) * 255 / 3;
			else if (source_format == PixelFormat::B2G3R3) ev = ((data & 0xE0) >> 5) * 255 / 7;
			else if (source_format == PixelFormat::R3G3B2) ev = (data & 0x07) * 255 / 7;
			if (source_alpha == AlphaMode::Premultiplied && alpha) { ev *= 255; ev /= alpha; }
			return ev;
		}
		uint ReadGreenChannel(uint data, PixelFormat source_format, AlphaMode source_alpha, uint alpha) noexcept
		{
			uint ev = 0;
			if (source_format == PixelFormat::A8 || source_format == PixelFormat::A4) return 255;
			else if (source_format == PixelFormat::A2 || source_format == PixelFormat::A1) return 255;
			if (source_format == PixelFormat::B8G8R8A8 || source_format == PixelFormat::B8G8R8) ev = (data & 0x0000FF00) >> 8;
			else if (source_format == PixelFormat::R8G8B8A8 || source_format == PixelFormat::R8G8B8) ev = (data & 0x0000FF00) >> 8;
			else if (source_format == PixelFormat::B8G8R8X8) ev = (data & 0x0000FF00) >> 8;
			else if (source_format == PixelFormat::R8G8B8X8) ev = (data & 0x0000FF00) >> 8;
			else if (source_format == PixelFormat::B5G5R5A1 || source_format == PixelFormat::B5G5R5X1) ev = ((data & 0x03E0) >> 5) * 255 / 31;
			else if (source_format == PixelFormat::R5G5B5A1 || source_format == PixelFormat::R5G5B5X1) ev = ((data & 0x03E0) >> 5) * 255 / 31;
			else if (source_format == PixelFormat::R5G6B5 || source_format == PixelFormat::B5G6R5) ev = ((data & 0x07E0) >> 5) * 255 / 63;
			else if (source_format == PixelFormat::B4G4R4A4 || source_format == PixelFormat::B4G4R4X4) ev = ((data & 0x00F0) >> 4) * 255 / 15;
			else if (source_format == PixelFormat::R4G4B4A4 || source_format == PixelFormat::R4G4B4X4) ev = ((data & 0x00F0) >> 4) * 255 / 15;
			else if (source_format == PixelFormat::R8 || source_format == PixelFormat::R8A8) ev = data & 0xFF;
			else if (source_format == PixelFormat::R4 || source_format == PixelFormat::R4A4) ev = (data & 0x0F) * 255 / 15;
			else if (source_format == PixelFormat::R2 || source_format == PixelFormat::R2A2) ev = (data & 0x3) * 255 / 3;
			else if (source_format == PixelFormat::R1 || source_format == PixelFormat::R1A1) ev = (data & 0x1) * 255;
			else if (source_format == PixelFormat::B2G3R2A1 || source_format == PixelFormat::R2G3B2A1) ev = ((data & 0x1C) >> 2) * 255 / 7;
			else if (source_format == PixelFormat::B2G3R2X1 || source_format == PixelFormat::R2G3B2X1) ev = ((data & 0x1C) >> 2) * 255 / 7;
			else if (source_format == PixelFormat::B2G3R3) ev = ((data & 0x1C) >> 2) * 255 / 7;
			else if (source_format == PixelFormat::R3G3B2) ev = ((data & 0x38) >> 3) * 255 / 7;
			else if (source_format == PixelFormat::B2G2R2A2 || source_format == PixelFormat::B2G2R2X2) ev = ((data & 0x0C) >> 2) * 255 / 3;
			else if (source_format == PixelFormat::R2G2B2A2 || source_format == PixelFormat::R2G2B2X2) ev = ((data & 0x0C) >> 2) * 255 / 3;
			if (source_alpha == AlphaMode::Premultiplied && alpha) { ev *= 255; ev /= alpha; }
			return ev;
		}
		uint ReadBlueChannel(uint data, PixelFormat source_format, AlphaMode source_alpha, uint alpha) noexcept
		{
			uint ev = 0;
			if (source_format == PixelFormat::A8 || source_format == PixelFormat::A4) return 255;
			else if (source_format == PixelFormat::A2 || source_format == PixelFormat::A1) return 255;
			if (source_format == PixelFormat::B8G8R8A8 || source_format == PixelFormat::B8G8R8) ev = (data & 0x000000FF);
			else if (source_format == PixelFormat::R8G8B8A8 || source_format == PixelFormat::R8G8B8) ev = (data & 0x00FF0000) >> 16;
			else if (source_format == PixelFormat::B8G8R8X8) ev = (data & 0x000000FF);
			else if (source_format == PixelFormat::R8G8B8X8) ev = (data & 0x00FF0000) >> 16;
			else if (source_format == PixelFormat::R5G5B5A1 || source_format == PixelFormat::R5G5B5X1) ev = ((data & 0x7C00) >> 10) * 255 / 31;
			else if (source_format == PixelFormat::R5G6B5) ev = ((data & 0xF800) >> 11) * 255 / 31;
			else if (source_format == PixelFormat::B5G5R5A1 || source_format == PixelFormat::B5G5R5X1 || source_format == PixelFormat::B5G6R5) ev = (data & 0x001F) * 255 / 31;
			else if (source_format == PixelFormat::R4G4B4A4 || source_format == PixelFormat::R4G4B4X4) ev = ((data & 0x0F00) >> 8) * 255 / 15;
			else if (source_format == PixelFormat::B4G4R4A4 || source_format == PixelFormat::B4G4R4X4) ev = (data & 0x000F) * 255 / 15;
			else if (source_format == PixelFormat::R8 || source_format == PixelFormat::R8A8) ev = data & 0xFF;
			else if (source_format == PixelFormat::R4 || source_format == PixelFormat::R4A4) ev = (data & 0x0F) * 255 / 15;
			else if (source_format == PixelFormat::R2 || source_format == PixelFormat::R2A2) ev = (data & 0x3) * 255 / 3;
			else if (source_format == PixelFormat::R1 || source_format == PixelFormat::R1A1) ev = (data & 0x1) * 255;
			else if (source_format == PixelFormat::R2G3B2A1 || source_format == PixelFormat::R2G3B2X1) ev = ((data & 0x60) >> 5) * 255 / 3;
			else if (source_format == PixelFormat::B2G3R2A1 || source_format == PixelFormat::B2G3R2X1) ev = (data & 0x03) * 255 / 3;
			else if (source_format == PixelFormat::B2G2R2A2 || source_format == PixelFormat::B2G2R2X2) ev = (data & 0x03) * 255 / 3;
			else if (source_format == PixelFormat::B2G3R3) ev = (data & 0x03) * 255 / 3;
			else if (source_format == PixelFormat::R2G2B2A2 || source_format == PixelFormat::R2G2B2X2) ev = ((data & 0x30) >> 4) * 255 / 3;
			else if (source_format == PixelFormat::R3G3B2) ev = ((data & 0xC0) >> 6) * 255 / 3;
			if (source_alpha == AlphaMode::Premultiplied && alpha) { ev *= 255; ev /= alpha; }
			return ev;
		}
		uint ReadAlphaChannel(uint data, PixelFormat source_format) noexcept
		{
			uint ev = 0;
			if (source_format == PixelFormat::B8G8R8A8) ev = (data & 0xFF000000) >> 24;
			else if (source_format == PixelFormat::R8G8B8A8) ev = (data & 0xFF000000) >> 24;
			else if (source_format == PixelFormat::B8G8R8X8 || source_format == PixelFormat::B8G8R8) ev = 0xFF;
			else if (source_format == PixelFormat::R8G8B8X8 || source_format == PixelFormat::R8G8B8) ev = 0xFF;
			else if (source_format == PixelFormat::R5G5B5A1 || source_format == PixelFormat::B5G5R5A1) ev = ((data & 0x8000) >> 15) * 255;
			else if (source_format == PixelFormat::R4G4B4A4 || source_format == PixelFormat::B4G4R4A4) ev = ((data & 0xF000) >> 12) * 255 / 15;
			else if (source_format == PixelFormat::R5G5B5X1 || source_format == PixelFormat::B5G5R5X1) ev = 0xFF;
			else if (source_format == PixelFormat::R4G4B4X4 || source_format == PixelFormat::B4G4R4X4) ev = 0xFF;
			else if (source_format == PixelFormat::R5G6B5 || source_format == PixelFormat::B5G6R5) ev = 0xFF;
			else if (source_format == PixelFormat::B2G3R2A1 || source_format == PixelFormat::R2G3B2A1) ev = ((data & 0x80) >> 7) * 255;
			else if (source_format == PixelFormat::B2G3R2X1 || source_format == PixelFormat::R2G3B2X1) ev = 0xFF;
			else if (source_format == PixelFormat::B2G3R3 || source_format == PixelFormat::R3G3B2) ev = 0xFF;
			else if (source_format == PixelFormat::B2G2R2A2 || source_format == PixelFormat::R2G2B2A2) ev = ((data & 0xC0) >> 6) * 255 / 3;
			else if (source_format == PixelFormat::B2G2R2X2 || source_format == PixelFormat::R2G2B2X2) ev = 0xFF;
			else if (source_format == PixelFormat::A8) ev = (data & 0xFF);
			else if (source_format == PixelFormat::R8) ev = 0xFF;
			else if (source_format == PixelFormat::R8A8) ev = (data & 0xFF00) >> 8;
			else if (source_format == PixelFormat::A4) ev = (data & 0x0F) * 255 / 15;
			else if (source_format == PixelFormat::R4) ev = 0xFF;
			else if (source_format == PixelFormat::R4A4) ev = ((data & 0xF0) >> 4) * 255 / 15;
			else if (source_format == PixelFormat::A2) ev = (data & 0x3) * 255 / 3;
			else if (source_format == PixelFormat::R2) ev = 0xFF;
			else if (source_format == PixelFormat::R2A2) ev = ((data & 0xC) >> 2) * 255 / 3;
			else if (source_format == PixelFormat::A1) ev = (data & 0x1) * 255;
			else if (source_format == PixelFormat::R1) ev = 0xFF;
			else if (source_format == PixelFormat::R1A1) ev = ((data & 0x2) >> 1) * 255;
			return ev;
		}
		uint MakePixelValue(uint r, uint g, uint b, uint a, PixelFormat format, AlphaMode alpha) noexcept
		{
			if (alpha == AlphaMode::Premultiplied) { r *= a; r /= 255; g *= a; g /= 255; b *= a; b /= 255; }
			if (format == PixelFormat::B8G8R8A8) {
				return b | (g << 8) | (r << 16) | (a << 24);
			} else if (format == PixelFormat::R8G8B8A8) {
				return r | (g << 8) | (b << 16) | (a << 24);
			} else if (format == PixelFormat::B8G8R8X8 || format == PixelFormat::B8G8R8) {
				return b | (g << 8) | (r << 16);
			} else if (format == PixelFormat::R8G8B8X8 || format == PixelFormat::R8G8B8) {
				return r | (g << 8) | (b << 16);
			} else if (format == PixelFormat::B5G5R5A1) {
				return ((b + 4) * 31 / 255) | (((g + 4) * 31 / 255) << 5) | (((r + 4) * 31 / 255) << 10) | (((a + 128) / 255) << 15);
			} else if (format == PixelFormat::B5G5R5X1) {
				return ((b + 4) * 31 / 255) | (((g + 4) * 31 / 255) << 5) | (((r + 4) * 31 / 255) << 10);
			} else if (format == PixelFormat::B5G6R5) {
				return ((b + 4) * 31 / 255) | (((g + 2) * 63 / 255) << 5) | (((r + 4) * 31 / 255) << 11);
			} else if (format == PixelFormat::R5G5B5A1) {
				return ((r + 4) * 31 / 255) | (((g + 4) * 31 / 255) << 5) | (((b + 4) * 31 / 255) << 10) | (((a + 128) / 255) << 15);
			} else if (format == PixelFormat::R5G5B5X1) {
				return ((r + 4) * 31 / 255) | (((g + 4) * 31 / 255) << 5) | (((b + 4) * 31 / 255) << 10);
			} else if (format == PixelFormat::R5G6B5) {
				return ((r + 4) * 31 / 255) | (((g + 2) * 63 / 255) << 5) | (((b + 4) * 31 / 255) << 11);
			} else if (format == PixelFormat::B4G4R4A4) {
				return ((b + 8) * 15 / 255) | (((g + 8) * 15 / 255) << 4) | (((r + 8) * 15 / 255) << 8) | (((a + 8) * 15 / 255) << 12);
			} else if (format == PixelFormat::B4G4R4X4) {
				return ((b + 8) * 15 / 255) | (((g + 8) * 15 / 255) << 4) | (((r + 8) * 15 / 255) << 8);
			} else if (format == PixelFormat::R4G4B4A4) {
				return ((r + 8) * 15 / 255) | (((g + 8) * 15 / 255) << 4) | (((b + 8) * 15 / 255) << 8) | (((a + 8) * 15 / 255) << 12);
			} else if (format == PixelFormat::R4G4B4X4) {
				return ((r + 8) * 15 / 255) | (((g + 8) * 15 / 255) << 4) | (((b + 8) * 15 / 255) << 8);
			} else if (format == PixelFormat::R8A8) {
				return ((grayscale_weight_r * r + grayscale_weight_g * g + grayscale_weight_b * b + 128) / 255) | (a << 8);
			} else if (format == PixelFormat::R4A4) {
				return ((grayscale_weight_r * r + grayscale_weight_g * g + grayscale_weight_b * b + 128) / 4335) | (((a + 8) * 15 / 255) << 4);
			} else if (format == PixelFormat::R2A2) {
				return ((grayscale_weight_r * r + grayscale_weight_g * g + grayscale_weight_b * b + 128) / 21675) | (((a + 42) * 3 / 255) << 2);
			} else if (format == PixelFormat::R1A1) {
				return ((grayscale_weight_r * r + grayscale_weight_g * g + grayscale_weight_b * b + 128) / 65025) | (((a + 128) / 255) << 1);
			} else if (format == PixelFormat::B2G3R2A1) {
				return ((b + 42) * 3 / 255) | (((g + 18) * 7 / 255) << 2) | (((r + 42) * 3 / 255) << 5) | (((a + 128) / 255) << 7);
			} else if (format == PixelFormat::B2G3R2X1) {
				return ((b + 42) * 3 / 255) | (((g + 18) * 7 / 255) << 2) | (((r + 42) * 3 / 255) << 5);
			} else if (format == PixelFormat::B2G3R3) {
				return ((b + 42) * 3 / 255) | (((g + 18) * 7 / 255) << 2) | (((r + 18) * 7 / 255) << 5);
			} else if (format == PixelFormat::R2G3B2A1) {
				return ((r + 42) * 3 / 255) | (((g + 18) * 7 / 255) << 2) | (((b + 42) * 3 / 255) << 5) | (((a + 128) / 255) << 7);
			} else if (format == PixelFormat::R2G3B2X1) {
				return ((r + 42) * 3 / 255) | (((g + 18) * 7 / 255) << 2) | (((b + 42) * 3 / 255) << 5);
			} else if (format == PixelFormat::R3G3B2) {
				return ((r + 18) * 7 / 255) | (((g + 18) * 7 / 255) << 3) | (((b + 42) * 3 / 255) << 6);
			} else if (format == PixelFormat::B2G2R2A2) {
				return ((b + 42) * 3 / 255) | (((g + 42) * 3 / 255) << 2) | (((r + 42) * 3 / 255) << 4) | (((a + 42) * 3 / 255) << 6);
			} else if (format == PixelFormat::B2G2R2X2) {
				return ((b + 42) * 3 / 255) | (((g + 42) * 3 / 255) << 2) | (((r + 42) * 3 / 255) << 4);
			} else if (format == PixelFormat::R2G2B2A2) {
				return ((r + 42) * 3 / 255) | (((g + 42) * 3 / 255) << 2) | (((b + 42) * 3 / 255) << 4) | (((a + 42) * 3 / 255) << 6);
			} else if (format == PixelFormat::R2G2B2X2) {
				return ((r + 42) * 3 / 255) | (((g + 42) * 3 / 255) << 2) | (((b + 42) * 3 / 255) << 4);
			} else if (format == PixelFormat::A8) {
				return a;
			} else if (format == PixelFormat::R8) {
				return (grayscale_weight_r * r + grayscale_weight_g * g + grayscale_weight_b * b + 128) / 255;
			} else if (format == PixelFormat::A4) {
				return (a + 8) * 15 / 255;
			} else if (format == PixelFormat::R4) {
				return (grayscale_weight_r * r + grayscale_weight_g * g + grayscale_weight_b * b + 128) / 4335;
			} else if (format == PixelFormat::A2) {
				return ((a + 42) * 3 / 255);
			} else if (format == PixelFormat::R2) {
				return (grayscale_weight_r * r + grayscale_weight_g * g + grayscale_weight_b * b + 128) / 21675;
			} else if (format == PixelFormat::A1) {
				return ((a + 128) / 255);
			} else if (format == PixelFormat::R1) {
				return (grayscale_weight_r * r + grayscale_weight_g * g + grayscale_weight_b * b + 128) / 65025;
			} else return 0;
		}
		uint ConvertPixelValue(uint data, PixelFormat source_format, AlphaMode source_alpha, PixelFormat format, AlphaMode alpha) noexcept
		{
			auto a = ReadAlphaChannel(data, source_format);
			return MakePixelValue(
				ReadRedChannel(data, source_format, source_alpha, a),
				ReadGreenChannel(data, source_format, source_alpha, a),
				ReadBlueChannel(data, source_format, source_alpha, a),
				a, format, alpha);
		}

		void GeneratePalette(const PictureDesc & dest, SystemPaletteType type) noexcept
		{
			if (type == SystemPaletteType::Unknown) {
				auto bpp = GetBitsPerPixel(dest.format);
				if (bpp == 8) type = SystemPaletteType::RGB685_8bit;
				else if (bpp == 4) type = SystemPaletteType::Windows_4bit;
				else if (bpp == 2) type = SystemPaletteType::Grayscale_2bit;
				else if (bpp == 1) type = SystemPaletteType::Grayscale_1bit;
			}
			if (type == SystemPaletteType::Grayscale_1bit) {
				if (dest.palette_size < 2) return;
				dest.palette[0] = Color(0x00, 0x00, 0x00);
				dest.palette[1] = Color(0xFF, 0xFF, 0xFF);
			} else if (type == SystemPaletteType::Grayscale_2bit) {
				if (dest.palette_size < 4) return;
				dest.palette[0] = Color(0x00, 0x00, 0x00);
				dest.palette[1] = Color(0x80, 0x80, 0x80);
				dest.palette[2] = Color(0xC0, 0xC0, 0xC0);
				dest.palette[3] = Color(0xFF, 0xFF, 0xFF);
			} else if (type == SystemPaletteType::Grayscale_4bit) {
				if (dest.palette_size < 16) return;
				for (uint i = 0; i < 16; i++) { uint8 v = i * 0xFF / 15; dest.palette[i] = Color(v, v, v); }
			} else if (type == SystemPaletteType::Windows_4bit) {
				if (dest.palette_size < 16) return;
				dest.palette[0] = Color(0x00, 0x00, 0x00);
				dest.palette[1] = Color(0x00, 0x00, 0x80);
				dest.palette[2] = Color(0x00, 0x80, 0x00);
				dest.palette[3] = Color(0x00, 0x80, 0x80);
				dest.palette[4] = Color(0x80, 0x00, 0x00);
				dest.palette[5] = Color(0x80, 0x00, 0x80);
				dest.palette[6] = Color(0x80, 0x80, 0x00);
				dest.palette[7] = Color(0xC0, 0xC0, 0xC0);
				dest.palette[8] = Color(0x80, 0x80, 0x80);
				dest.palette[9] = Color(0x00, 0x00, 0xFF);
				dest.palette[10] = Color(0x00, 0xFF, 0x00);
				dest.palette[11] = Color(0x00, 0xFF, 0xFF);
				dest.palette[12] = Color(0xFF, 0x00, 0x00);
				dest.palette[13] = Color(0xFF, 0x00, 0xFF);
				dest.palette[14] = Color(0xFF, 0xFF, 0x00);
				dest.palette[15] = Color(0xFF, 0xFF, 0xFF);
			} else if (type == SystemPaletteType::CGA_4bit) {
				if (dest.palette_size < 16) return;
				dest.palette[0] = Color(0x00, 0x00, 0x00);
				dest.palette[1] = Color(0x00, 0x00, 0xAA);
				dest.palette[2] = Color(0x00, 0xAA, 0x00);
				dest.palette[3] = Color(0x00, 0xAA, 0xAA);
				dest.palette[4] = Color(0xAA, 0x00, 0x00);
				dest.palette[5] = Color(0xAA, 0x00, 0xAA);
				dest.palette[6] = Color(0xAA, 0x55, 0x00);
				dest.palette[7] = Color(0xAA, 0xAA, 0xAA);
				dest.palette[8] = Color(0x55, 0x55, 0x55);
				dest.palette[9] = Color(0x55, 0x55, 0xFF);
				dest.palette[10] = Color(0x55, 0xFF, 0x55);
				dest.palette[11] = Color(0x55, 0xFF, 0xFF);
				dest.palette[12] = Color(0xFF, 0x55, 0x55);
				dest.palette[13] = Color(0xFF, 0x55, 0xFF);
				dest.palette[14] = Color(0xFF, 0xFF, 0x55);
				dest.palette[15] = Color(0xFF, 0xFF, 0xFF);
			} else if (type == SystemPaletteType::Macintosh_4bit) {
				if (dest.palette_size < 16) return;
				dest.palette[0] = Color(0xFF, 0xFF, 0xFF);
				dest.palette[1] = Color(0xFC, 0xF3, 0x05);
				dest.palette[2] = Color(0xFF, 0x64, 0x02);
				dest.palette[3] = Color(0xDD, 0x08, 0x06);
				dest.palette[4] = Color(0xF2, 0x08, 0x84);
				dest.palette[5] = Color(0x46, 0x00, 0xA5);
				dest.palette[6] = Color(0x00, 0x00, 0xD4);
				dest.palette[7] = Color(0x02, 0xAB, 0xEA);
				dest.palette[8] = Color(0x1F, 0xB7, 0x14);
				dest.palette[9] = Color(0x00, 0x64, 0x11);
				dest.palette[10] = Color(0x56, 0x2C, 0x05);
				dest.palette[11] = Color(0x90, 0x71, 0x3A);
				dest.palette[12] = Color(0xC0, 0xC0, 0xC0);
				dest.palette[13] = Color(0x80, 0x80, 0x80);
				dest.palette[14] = Color(0x40, 0x40, 0x40);
				dest.palette[15] = Color(0x00, 0x00, 0x00);
			} else if (type == SystemPaletteType::RGB685_8bit) {
				if (dest.palette_size < 256) return;
				for (uint r = 0; r < 6; r++) for (uint g = 0; g < 8; g++) for (uint b = 0; b < 5; b++) {
					dest.palette[40 * r + 5 * g + b] = Color(uint8(r * 255 / 5), uint8(g * 255 / 7), uint8(b * 255 / 4));
				}
				for (uint i = 0; i < 16; i++) {
					uint8 v = (1 + i) * 255 / 17;
					dest.palette[240 + i] = Color(v, v, v);
				}
			} else if (type == SystemPaletteType::RGB685T_8bit) {
				if (dest.palette_size < 256) return;
				for (uint r = 0; r < 6; r++) for (uint g = 0; g < 8; g++) for (uint b = 0; b < 5; b++) {
					dest.palette[40 * r + 5 * g + b] = Color(uint8(r * 255 / 5), uint8(g * 255 / 7), uint8(b * 255 / 4));
				}
				for (uint i = 0; i < 15; i++) {
					uint8 v = (1 + i) * 255 / 16;
					dest.palette[240 + i] = Color(v, v, v);
				}
				dest.palette[255].value = 0;
			}
		}
		void BlockTransfer(const PictureDesc & dest, uint dest_x, uint dest_y, const PictureDesc & src, uint src_x, uint src_y, uint width, uint height) noexcept
		{
			if (!NeedsPalette(dest.format) && !NeedsPalette(src.format)) {
				if (dest.format == src.format && dest.alpha_mode == src.alpha_mode) {
					for (uint y = 0; y < height; y++) for (uint x = 0; x < width; x++) {
						SetPixel(dest, dest_x + x, dest_y + y, GetPixel(src, src_x + x, src_y + y));
					}
				} else {
					for (uint y = 0; y < height; y++) for (uint x = 0; x < width; x++) {
						SetPixel(dest, dest_x + x, dest_y + y, ConvertPixelValue(GetPixel(src, src_x + x, src_y + y), src.format, src.alpha_mode, dest.format, dest.alpha_mode));
					}
				}
			} else if (!NeedsPalette(dest.format)) {
				for (uint y = 0; y < height; y++) for (uint x = 0; x < width; x++) {
					SetPixel(dest, dest_x + x, dest_y + y, ConvertPixelValue(src.palette[GetPixel(src, src_x + x, src_y + y)].value, PixelFormat::R8G8B8A8, AlphaMode::Straight, dest.format, dest.alpha_mode));
				}
			} else {
				for (uint y = 0; y < height; y++) for (uint x = 0; x < width; x++) {
					uint srcc = NeedsPalette(src.format) ?
						src.palette[GetPixel(src, src_x + x, src_y + y)].value :
						ConvertPixelValue(GetPixel(src, src_x + x, src_y + y), src.format, src.alpha_mode, PixelFormat::R8G8B8A8, AlphaMode::Straight);
					SetPixel(dest, dest_x + x, dest_y + y, LookupPalette(dest, srcc));
				}
			}
		}
		void BlockTransfer(const PictureDesc & dest, const PictureDesc & src) noexcept { BlockTransfer(dest, 0, 0, src, 0, 0, src.width, src.height); }
		uint LookupPalette(const PictureDesc & desc, Color color) noexcept
		{
			auto a = float(color.a) / 255.0f, r = float(color.r) / 255.0f * a, g = float(color.g) / 255.0f * a, b = float(color.b) / 255.0f * a;
			int min_dist = -1, min_index = -1;
			int index = -1;
			float error = -1.0f;
			for (uint i = 0; i < desc.palette_size; i++) {
				auto pa = float(desc.palette[i].a) / 255.0f;
				auto pr = float(desc.palette[i].r) / 255.0f * pa;
				auto pg = float(desc.palette[i].g) / 255.0f * pa;
				auto pb = float(desc.palette[i].b) / 255.0f * pa;
				auto dr = r - pr, dg = g - pg, db = b - pb, da = a - pa;
				auto e = sqrt(dr * dr + dg * dg + db * db + da * da);
				if (e < error || index < 0) { index = i; error = e; }
			}
			return index;
		}
		uint GetPixel(const PictureDesc & desc, uint x, uint y) noexcept
		{
			if (desc.origin == ScanOrigin::BottomLeft) y = desc.height - y - 1;
			auto bpp = GetBitsPerPixel(desc.format);
			auto data = reinterpret_cast<const uint8 *>(desc.data);
			if (bpp == 32) {
				return *reinterpret_cast<const uint32 *>(data + desc.stride * y + 4 * x);
			} else if (bpp == 24) {
				auto base = desc.stride * y + 3 * x;
				return data[base] | (uint32(data[base + 1]) << 8) | (uint32(data[base + 2]) << 16);
			} else if (bpp == 16) {
				auto base = desc.stride * y + 2 * x;
				return data[base] | (uint32(data[base + 1]) << 8);
			} else if (bpp <= 8) {
				auto line_bit = x * bpp;
				auto byte = y * desc.stride + (line_bit >> 3U);
				auto bit = 8U - bpp - (line_bit & 0x7);
				uint8 b = data[byte];
				if (bpp == 8) return b;
				else if (bpp == 4) return (b >> bit) & 0xF;
				else if (bpp == 2) return (b >> bit) & 0x3;
				else if (bpp == 1) return (b >> bit) & 0x1;
				else return 0;
			} else return 0;
		}
		void SetPixel(const PictureDesc & desc, uint x, uint y, uint v) noexcept
		{
			if (desc.origin == ScanOrigin::BottomLeft) y = desc.height - y - 1;
			auto bpp = GetBitsPerPixel(desc.format);
			auto data = reinterpret_cast<uint8 *>(desc.data);
			if (bpp == 32) {
				*reinterpret_cast<uint32 *>(data + desc.stride * y + 4 * x) = v;
			} else if (bpp == 24) {
				auto base = desc.stride * y + 3 * x;
				data[base] = v & 0xFF;
				data[base + 1] = (v & 0xFF00) >> 8;
				data[base + 2] = (v & 0xFF0000) >> 16;
			} else if (bpp == 16) {
				auto base = desc.stride * y + 2 * x;
				data[base] = v & 0xFF;
				data[base + 1] = (v & 0xFF00) >> 8;
			} else if (bpp <= 8) {
				auto line_bit = x * bpp;
				auto byte = y * desc.stride + (line_bit >> 3U);
				auto bit = 8U - bpp - (line_bit & 0x7);
				if (bpp == 8) data[byte] = v;
				else if (bpp == 4) {
					data[byte] &= 0xFF ^ (0xF << bit);
					data[byte] |= v << bit;
				} else if (bpp == 2) {
					data[byte] &= 0xFF ^ (0x3 << bit);
					data[byte] |= v << bit;
				} else if (bpp == 1) {
					data[byte] &= 0xFF ^ (0x1 << bit);
					data[byte] |= v << bit;
				}
			}
		}

		Picture::Picture(const PictureDesc & desc, PictureInit init) : _desc(desc)
		{
			if (!desc.width || !desc.height) throw InvalidArgumentException();
			_attr.plane = _attr.animation_duration = _attr.pointer_offset_x = _attr.pointer_offset_y = 0;
			_attr.scale_factor = 1.0;
			if (init != PictureInit::Refer) {
				_free = true;
				_desc.data = malloc(uintptr(_desc.stride) * uintptr(_desc.height));
				if (!_desc.data) throw OutOfMemoryException();
				if (_desc.palette_size) {
					_desc.palette = reinterpret_cast<Color *>(malloc(uintptr(_desc.palette_size) * sizeof(Color)));
					if (!_desc.palette) { free(_desc.data); throw OutOfMemoryException(); }
				} else _desc.palette = 0;
				if (init == PictureInit::AllocateZeroed) {
					Memory::ZeroMemory(_desc.data, uintptr(_desc.stride) * uintptr(_desc.height));
					if (_desc.palette) Memory::ZeroMemory(_desc.palette, uintptr(_desc.palette_size) * sizeof(Color));
				} else if (init == PictureInit::AllocateCopy) {
					Memory::MemoryCopy(_desc.data, desc.data, uintptr(_desc.stride) * uintptr(_desc.height));
					if (_desc.palette) Memory::MemoryCopy(_desc.palette, desc.palette, uintptr(_desc.palette_size) * sizeof(Color));
				}
			} else _free = false;
		}
		Picture::Picture(const Picture * src) : Picture(src->_desc, PictureInit::AllocateCopy) { _attr = src->_attr; }
		Picture::Picture(const Picture & src) : Picture(src._desc, PictureInit::AllocateCopy) { _attr = src._attr; }
		Picture::Picture(Picture && src) noexcept { _free = src._free; _desc = src._desc; _attr = src._attr; src._free = false; }
		Picture::Picture(uint w, uint h, PixelFormat fmt) : Picture(PictureDesc {
			.data = 0, .palette = 0,
			.width = w, .height = h, .stride = (w * GetBitsPerPixel(fmt) / 8 + 3) & ~3,
			.palette_size = NeedsPalette(fmt) ? uint(1) << GetBitsPerPixel(fmt) : 0,
			.format = fmt, .alpha_mode = AlphaMode::Straight, .origin = ScanOrigin::TopLeft
		}, PictureInit::AllocateZeroed) {}
		Picture::Picture(uint w, uint h, PixelFormat fmt, AlphaMode alpha) : Picture(PictureDesc {
			.data = 0, .palette = 0,
			.width = w, .height = h, .stride = (w * GetBitsPerPixel(fmt) / 8 + 3) & ~3,
			.palette_size = NeedsPalette(fmt) ? uint(1) << GetBitsPerPixel(fmt) : 0,
			.format = fmt, .alpha_mode = alpha, .origin = ScanOrigin::TopLeft
		}, PictureInit::AllocateZeroed) {}
		Picture::Picture(uint w, uint h, PixelFormat fmt, ScanOrigin origin) : Picture(PictureDesc {
			.data = 0, .palette = 0,
			.width = w, .height = h, .stride = (w * GetBitsPerPixel(fmt) / 8 + 3) & ~3,
			.palette_size = NeedsPalette(fmt) ? uint(1) << GetBitsPerPixel(fmt) : 0,
			.format = fmt, .alpha_mode = AlphaMode::Straight, .origin = origin
		}, PictureInit::AllocateZeroed) {}
		Picture::Picture(uint w, uint h, PixelFormat fmt, AlphaMode alpha, ScanOrigin origin) : Picture(PictureDesc {
			.data = 0, .palette = 0,
			.width = w, .height = h, .stride = (w * GetBitsPerPixel(fmt) / 8 + 3) & ~3,
			.palette_size = NeedsPalette(fmt) ? uint(1) << GetBitsPerPixel(fmt) : 0,
			.format = fmt, .alpha_mode = alpha, .origin = origin
		}, PictureInit::AllocateZeroed) {}
		Picture::~Picture(void) { if (_free) { free(_desc.data); free(_desc.palette); } }
		string Picture::ToStringE(ErrorContext & ectx) const noexcept
		{
			ESSE_TRY_INTRO
			return U"Pictura " + string(_desc.width) + U" x " + string(_desc.height) + U", " + string(GetBitsPerPixel(_desc.format)) + U"bpp";
			ESSE_TRY_OUTRO(string())
		}
		void Picture::Dither(const PictureDesc & dest) const
		{
			if (!NeedsPalette(dest.format) || dest.width != _desc.width || dest.height != _desc.height || !dest.palette_size) throw InvalidArgumentException();
			auto size = 8 * _desc.width * sizeof(float);
			auto edata = reinterpret_cast<float *>(malloc(size));
			if (!edata) throw OutOfMemoryException();
			Memory::ZeroMemory(edata, size);
			auto scan0_r = edata, scan0_g = edata + _desc.width, scan0_b = edata + 2 * _desc.width, scan0_a = edata + 3 * _desc.width;
			auto scan1_r = edata + 4 * _desc.width, scan1_g = edata + 5 * _desc.width, scan1_b = edata + 6 * _desc.width, scan1_a = edata + 7 * _desc.width;
			for (uintptr y = 0; y < _desc.height; y++) {
				for (uintptr x = 0; x < _desc.width; x++) {
					auto v = ReadPixel(x, y);
					auto a = float(v.a) / 255.0f, r = float(v.r) / 255.0f * a, g = float(v.g) / 255.0f * a, b = float(v.b) / 255.0f * a;
					a += scan0_a[x]; r += scan0_r[x]; g += scan0_g[x]; b += scan0_b[x];
					int index = -1;
					float error = -1.0f, error_r, error_g, error_b, error_a;
					for (uint i = 0; i < dest.palette_size; i++) {
						auto pa = float(dest.palette[i].a) / 255.0f;
						auto pr = float(dest.palette[i].r) / 255.0f * pa;
						auto pg = float(dest.palette[i].g) / 255.0f * pa;
						auto pb = float(dest.palette[i].b) / 255.0f * pa;
						auto dr = r - pr, dg = g - pg, db = b - pb, da = a - pa;
						auto e = sqrt(dr * dr + dg * dg + db * db + da * da);
						if (e < error || index < 0) { index = i; error = e; error_r = dr; error_g = dg; error_b = db; error_a = da; }
					}
					Picturae::SetPixel(dest, x, y, index);
					if (x > 0) {
						scan1_r[x - 1] += error_r * (3.0f / 16.0f);
						scan1_g[x - 1] += error_g * (3.0f / 16.0f);
						scan1_b[x - 1] += error_b * (3.0f / 16.0f);
						scan1_a[x - 1] += error_a * (3.0f / 16.0f);
					}
					scan1_r[x] += error_r * (5.0f / 16.0f);
					scan1_g[x] += error_g * (5.0f / 16.0f);
					scan1_b[x] += error_b * (5.0f / 16.0f);
					scan1_a[x] += error_a * (5.0f / 16.0f);
					if (x + 1 < _desc.width) {
						scan0_r[x + 1] += error_r * (7.0f / 16.0f);
						scan0_g[x + 1] += error_g * (7.0f / 16.0f);
						scan0_b[x + 1] += error_b * (7.0f / 16.0f);
						scan0_a[x + 1] += error_a * (7.0f / 16.0f);
						scan1_r[x + 1] += error_r * (1.0f / 16.0f);
						scan1_g[x + 1] += error_g * (1.0f / 16.0f);
						scan1_b[x + 1] += error_b * (1.0f / 16.0f);
						scan1_a[x + 1] += error_a * (1.0f / 16.0f);
					}
				}
				swap(scan0_r, scan1_r); swap(scan0_g, scan1_g); swap(scan0_b, scan1_b); swap(scan0_a, scan1_a);
				Memory::ZeroMemory(scan1_r, _desc.width * sizeof(float));
				Memory::ZeroMemory(scan1_g, _desc.width * sizeof(float));
				Memory::ZeroMemory(scan1_b, _desc.width * sizeof(float));
				Memory::ZeroMemory(scan1_a, _desc.width * sizeof(float));
			}
			free(edata);
		}
		oref<Picture> Picture::Convert(const PictureDesc & desc) const
		{
			if (desc.width != _desc.width || desc.height != _desc.height) throw InvalidArgumentException();
			auto result = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
			if (desc.palette_size && desc.palette_size == _desc.palette_size) Memory::MemoryCopy(result->GetDesc().palette, _desc.palette, _desc.palette_size * sizeof(Color));
			else if (desc.palette_size) GeneratePalette(result->GetDesc());
			BlockTransfer(result->GetDesc(), _desc);
			result->_attr = _attr;
			return result;
		}
		oref<Picture> Picture::Convert(PixelFormat fmt) const
		{
			auto desc = _desc;
			desc.format = fmt;
			desc.stride = ((_desc.width * GetBitsPerPixel(fmt) + 31) & ~31) / 8;
			desc.palette_size = NeedsPalette(fmt) ? 1U << GetBitsPerPixel(fmt) : 0;
			auto result = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
			if (desc.palette_size && desc.palette_size == _desc.palette_size) Memory::MemoryCopy(result->GetDesc().palette, _desc.palette, _desc.palette_size * sizeof(Color));
			else if (desc.palette_size) GeneratePalette(result->GetDesc());
			BlockTransfer(result->GetDesc(), _desc);
			result->_attr = _attr;
			return result;
		}
		oref<Picture> Picture::Convert(PixelFormat fmt, AlphaMode alpha) const
		{
			auto desc = _desc;
			desc.format = fmt;
			desc.alpha_mode = alpha;
			desc.stride = ((_desc.width * GetBitsPerPixel(fmt) + 31) & ~31) / 8;
			desc.palette_size = NeedsPalette(fmt) ? 1U << GetBitsPerPixel(fmt) : 0;
			auto result = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
			if (desc.palette_size && desc.palette_size == _desc.palette_size) Memory::MemoryCopy(result->GetDesc().palette, _desc.palette, _desc.palette_size * sizeof(Color));
			else if (desc.palette_size) GeneratePalette(result->GetDesc());
			BlockTransfer(result->GetDesc(), _desc);
			result->_attr = _attr;
			return result;
		}
		oref<Picture> Picture::Convert(PixelFormat fmt, ScanOrigin origin) const
		{
			auto desc = _desc;
			desc.format = fmt;
			desc.origin = origin;
			desc.stride = ((_desc.width * GetBitsPerPixel(fmt) + 31) & ~31) / 8;
			desc.palette_size = NeedsPalette(fmt) ? 1U << GetBitsPerPixel(fmt) : 0;
			auto result = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
			if (desc.palette_size && desc.palette_size == _desc.palette_size) Memory::MemoryCopy(result->GetDesc().palette, _desc.palette, _desc.palette_size * sizeof(Color));
			else if (desc.palette_size) GeneratePalette(result->GetDesc());
			BlockTransfer(result->GetDesc(), _desc);
			result->_attr = _attr;
			return result;
		}
		oref<Picture> Picture::Convert(PixelFormat fmt, AlphaMode alpha, ScanOrigin origin) const
		{
			auto desc = _desc;
			desc.format = fmt;
			desc.alpha_mode = alpha;
			desc.origin = origin;
			desc.stride = ((_desc.width * GetBitsPerPixel(fmt) + 31) & ~31) / 8;
			desc.palette_size = NeedsPalette(fmt) ? 1U << GetBitsPerPixel(fmt) : 0;
			auto result = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
			if (desc.palette_size && desc.palette_size == _desc.palette_size) Memory::MemoryCopy(result->GetDesc().palette, _desc.palette, _desc.palette_size * sizeof(Color));
			else if (desc.palette_size) GeneratePalette(result->GetDesc());
			BlockTransfer(result->GetDesc(), _desc);
			result->_attr = _attr;
			return result;
		}
		const PictureDesc & Picture::GetDesc(void) const noexcept { return _desc; }
		PictureExtendedAttributes & Picture::GetAttributes(void) noexcept { return _attr; }
		const PictureExtendedAttributes & Picture::GetAttributes(void) const noexcept { return _attr; }
		uint Picture::GetPixel(uint x, uint y) const noexcept { return Picturae::GetPixel(_desc, x, y); }
		void Picture::SetPixel(uint x, uint y, uint v) const noexcept { Picturae::SetPixel(_desc, x, y, v); }
		uint Picture::LookupPalette(Color color) const noexcept { return Picturae::LookupPalette(_desc, color); }
		Color Picture::ReadPixel(uint x, uint y) const noexcept
		{
			if (NeedsPalette(_desc.format)) return _desc.palette[GetPixel(x, y)];
			else return ConvertPixelValue(GetPixel(x, y), _desc.format, _desc.alpha_mode, PixelFormat::R8G8B8A8, AlphaMode::Straight);
		}
		void Picture::WritePixel(uint x, uint y, Color color) const noexcept
		{
			if (NeedsPalette(_desc.format)) SetPixel(x, y, LookupPalette(color));
			else SetPixel(x, y, ConvertPixelValue(color.value, PixelFormat::R8G8B8A8, AlphaMode::Straight, _desc.format, _desc.alpha_mode));
		}

		Image::Image(void) noexcept : object_array<Picture>(0x20) {}
		Picture * Image::FindBestSizeMatch(uint width, uint height) noexcept
		{
			if (!GetLength()) return 0;
			intptr best_i = -1;
			double min_dist = 0.0;
			for (uintptr i = 0; i < GetLength(); i++) {
				auto & f = ElementAt(i);
				double dist = sqrt(pow(double(width - f.GetDesc().width), 2.0) + pow(double(height - f.GetDesc().height), 2.0));
				if (dist < min_dist || best_i == -1) { best_i = i; min_dist = dist; }
			}
			return (best_i >= 0) ? ReferenceAt(best_i) : 0;
		}
		Picture * Image::FindExactSizeMatch(uint width, uint height) noexcept
		{
			for (auto & f : Elements()) if (f.GetDesc().width == width && f.GetDesc().height == height) return &f;
			return 0;
		}
		Picture * Image::FindBestScaleMatch(double scale) noexcept
		{
			if (!GetLength()) return 0;
			intptr best_i = -1;
			double min_dist = 0.0;
			for (uintptr i = 0; i < GetLength(); i++) {
				auto & f = ElementAt(i);
				double dist = fabs(scale - f.GetAttributes().scale_factor);
				if (dist < min_dist || best_i == -1) { best_i = i; min_dist = dist; }
			}
			return (best_i >= 0) ? ReferenceAt(best_i) : 0;
		}
		Picture * Image::FindPlaneMatch(uint plane) noexcept
		{
			for (auto & f : Elements()) if (f.GetAttributes().plane == plane) return &f;
			return 0;
		}
	
		oref<Picture> AlphaColorFuse(Picture * color, Picture * alpha)
		{
			if (color->GetDesc().width != alpha->GetDesc().width || color->GetDesc().height != alpha->GetDesc().height) throw InvalidArgumentException();
			auto result = color->Convert(PixelFormat::B8G8R8A8, AlphaMode::Straight);
			for (uint y = 0; y < result->GetDesc().height; y++) for (uint x = 0; x < result->GetDesc().width; x++) {
				auto pixel = result->GetPixel(x, y);
				pixel = (pixel & 0xFFFFFFU) | (ReadAlphaChannel(alpha->GetPixel(x, y), alpha->GetDesc().format) << 24);
				result->SetPixel(x, y, pixel);
			}
			return result;
		}
		void AlphaColorFuse(Image * image)
		{
			for (uintptr i = 0; i < image->GetLength(); i++) {
				if (!PixelFormatHasAlpha(image->ElementAt(i).GetDesc().format)) {
					auto plane = image->ElementAt(i).GetAttributes().plane;
					uintptr mask_index = i;
					for (uintptr j = 0; j < image->GetLength(); j++) if (image->ElementAt(j).GetAttributes().plane == plane) {
						auto pxf = image->ElementAt(j).GetDesc().format;
						if (pxf == PixelFormat::A1 || pxf == PixelFormat::A2 || pxf == PixelFormat::A4 || pxf == PixelFormat::A8) { mask_index = j; break; }
					}
					if (mask_index != i) {
						auto fused = AlphaColorFuse(image->ReferenceAt(i), image->ReferenceAt(mask_index));
						image->SetElement(fused, i);
						image->Remove(mask_index);
						if (mask_index < i) i--;
					}
				}
			}
		}
	}
}