#include "CorGraphics.h"

namespace ESSE
{
	Index2::Index2(void) noexcept {}
	Index2::Index2(int sx, int sy) noexcept : x(sx), y(sy) {}
	Index2::~Index2(void) {}
	Index2::operator string (void) const { return FormatString(U"(%0, %1)", x, y); }
	Index2 Index2::operator - (void) const noexcept { return Index2(-x, -y); }
	Index2 & Index2::operator += (const Index2 & a) noexcept { x += a.x; y += a.y; return *this; }
	Index2 & Index2::operator -= (const Index2 & a) noexcept { x -= a.x; y -= a.y; return *this; }
	Index2 & Index2::operator *= (const Index2 & a) noexcept { x *= a.x; y *= a.y; return *this; }
	Index2 & Index2::operator /= (const Index2 & a) noexcept { x /= a.x; y /= a.y; return *this; }
	Index2 & Index2::operator %= (const Index2 & a) noexcept { x %= a.x; y %= a.y; return *this; }
	Index2 operator + (const Index2 & a, const Index2 & b) noexcept { return Index2(a.x + b.x, a.y + b.y); }
	Index2 operator - (const Index2 & a, const Index2 & b) noexcept { return Index2(a.x - b.x, a.y - b.y); }
	Index2 operator * (const Index2 & a, const Index2 & b) noexcept { return Index2(a.x * b.x, a.y * b.y); }
	Index2 operator / (const Index2 & a, const Index2 & b) noexcept { return Index2(a.x / b.x, a.y / b.y); }
	Index2 operator % (const Index2 & a, const Index2 & b) noexcept { return Index2(a.x % b.x, a.y % b.y); }
	bool operator == (const Index2 & a, const Index2 & b) noexcept { return a.x == b.x && a.y == b.y; }
	bool operator != (const Index2 & a, const Index2 & b) noexcept { return a.x != b.x || a.y != b.y; }
	bool operator <= (const Index2 & a, const Index2 & b) noexcept { return a.x < b.x || (a.x == b.x && a.y <= b.y); }
	bool operator >= (const Index2 & a, const Index2 & b) noexcept { return a.x > b.x || (a.x == b.x && a.y >= b.y); }
	bool operator < (const Index2 & a, const Index2 & b) noexcept { return a.x < b.x || (a.x == b.x && a.y < b.y); }
	bool operator > (const Index2 & a, const Index2 & b) noexcept { return a.x > b.x || (a.x == b.x && a.y > b.y); }
	Index3::Index3(void) noexcept {}
	Index3::Index3(int sx, int sy, int sz) noexcept : x(sx), y(sy), z(sz) {}
	Index3::Index3(const Index2 & sxy, int sz) noexcept : x(sxy.x), y(sxy.y), z(sz) {}
	Index3::Index3(int sx, const Index2 & syz) noexcept : x(sx), y(syz.x), z(syz.y) {}
	Index3::~Index3(void) {}
	Index3::operator string (void) const { return FormatString(U"(%0, %1, %2)", x, y, z); }
	Index3 Index3::operator - (void) const noexcept { return Index3(-x, -y, -z); }
	Index3 & Index3::operator += (const Index3 & a) noexcept { x += a.x; y += a.y; z += a.z; return *this; }
	Index3 & Index3::operator -= (const Index3 & a) noexcept { x -= a.x; y -= a.y; z -= a.z; return *this; }
	Index3 & Index3::operator *= (const Index3 & a) noexcept { x *= a.x; y *= a.y; z *= a.z; return *this; }
	Index3 & Index3::operator /= (const Index3 & a) noexcept { x /= a.x; y /= a.y; z /= a.z; return *this; }
	Index3 & Index3::operator %= (const Index3 & a) noexcept { x %= a.x; y %= a.y; z %= a.z; return *this; }
	Index3 operator + (const Index3 & a, const Index3 & b) noexcept { return Index3(a.x + b.x, a.y + b.y, a.z + b.z); }
	Index3 operator - (const Index3 & a, const Index3 & b) noexcept { return Index3(a.x - b.x, a.y - b.y, a.z - b.z); }
	Index3 operator * (const Index3 & a, const Index3 & b) noexcept { return Index3(a.x * b.x, a.y * b.y, a.z * b.z); }
	Index3 operator / (const Index3 & a, const Index3 & b) noexcept { return Index3(a.x / b.x, a.y / b.y, a.z / b.z); }
	Index3 operator % (const Index3 & a, const Index3 & b) noexcept { return Index3(a.x % b.x, a.y % b.y, a.z % b.z); }
	bool operator == (const Index3 & a, const Index3 & b) noexcept { return a.x == b.x && a.y == b.y && a.z == b.z; }
	bool operator != (const Index3 & a, const Index3 & b) noexcept { return a.x != b.x || a.y != b.y || a.z != b.z; }
	bool operator <= (const Index3 & a, const Index3 & b) noexcept { return a.x < b.x || (a.x == b.x && a.y < b.y) || (a.x == b.x && a.y == b.y && a.z <= b.z); }
	bool operator >= (const Index3 & a, const Index3 & b) noexcept { return a.x > b.x || (a.x == b.x && a.y > b.y) || (a.x == b.x && a.y == b.y && a.z >= b.z); }
	bool operator < (const Index3 & a, const Index3 & b) noexcept { return a.x < b.x || (a.x == b.x && a.y < b.y) || (a.x == b.x && a.y == b.y && a.z < b.z); }
	bool operator > (const Index3 & a, const Index3 & b) noexcept { return a.x > b.x || (a.x == b.x && a.y > b.y) || (a.x == b.x && a.y == b.y && a.z > b.z); }
	Rectangle::Rectangle(void) noexcept {}
	Rectangle::Rectangle(int l, int t, int r, int b) noexcept : left(l), top(t), right(r), bottom(b) {}
	Rectangle::Rectangle(const Index2 & lt, const Index2 & rb) noexcept : left(lt.x), top(lt.y), right(rb.x), bottom(rb.y) {}
	Rectangle::~Rectangle(void) {}
	Rectangle::operator string (void) const { return FormatString(U"{%0, %1, %2, %3}", left, top, right, bottom); }
	bool Rectangle::IsInside(const Index2 & i) const noexcept { return i.x >= left && i.y >= top && i.x < right && i.y < bottom; }
	bool operator == (const Rectangle & a, const Rectangle & b) noexcept { return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom; }
	bool operator != (const Rectangle & a, const Rectangle & b) noexcept { return a.left != b.left || a.top != b.top || a.right != b.right || a.bottom != b.bottom; }
	Rectangle Rectangle::Intersect(const Rectangle & a, const Rectangle & b) noexcept { return Rectangle(max(a.left, b.left), max(a.top, b.top), min(a.right, b.right), min(a.bottom, b.bottom)); }
	Rectangle Rectangle::OuterRectangle(const Rectangle & a, const Rectangle & b) noexcept { return Rectangle(min(a.left, b.left), min(a.top, b.top), max(a.right, b.right), max(a.bottom, b.bottom)); }

	namespace Graphica
	{
		bool IsColorFormat(PixelFormat format) noexcept { return (uint32(format) & 0xF0000000) == 0x80000000; }
		bool IsDepthStencilFormat(PixelFormat format) noexcept { return (uint32(format) & 0xF0000000) == 0x40000000; }
		uint GetFormatChannelCount(PixelFormat format) noexcept { return (uint32(format) & 0x00F00000) >> 20; }
		uint GetFormatBitsPerPixel(PixelFormat format) noexcept
		{
			auto bpp = (uint32(format) & 0x0F000000);
			if (bpp == 0x01000000) return 8;
			else if (bpp == 0x02000000) return 16;
			else if (bpp == 0x03000000) return 32;
			else if (bpp == 0x04000000) return 64;
			else if (bpp == 0x05000000) return 128;
			else return 0;
		}
	
		oref<IShaderLibrary> IDevice::LoadShaderLibraryFromData(const void * data, uintptr length) { ErrorContext ectx; ErrorClear(ectx); auto result = LoadShaderLibraryFromData(data, length, ectx); ErrorThrow(ectx); return result; }
		oref<IShaderLibrary> IDevice::LoadShaderLibrary(Stream * stream) { ErrorContext ectx; ErrorClear(ectx); auto result = LoadShaderLibrary(stream, ectx); ErrorThrow(ectx); return result; }
		oref<IShaderLibrary> IDevice::CompileShaderLibraryFromData(const void * data, uintptr length) { ErrorContext ectx; ErrorClear(ectx); auto result = CompileShaderLibraryFromData(data, length, ectx); ErrorThrow(ectx); return result; }
		oref<IShaderLibrary> IDevice::CompileShaderLibrary(Stream * stream) { ErrorContext ectx; ErrorClear(ectx); auto result = CompileShaderLibrary(stream, ectx); ErrorThrow(ectx); return result; }
		void IDeviceResourceHandle::Send(IPC::IConnection * con) { ErrorContext ectx; ErrorClear(ectx); Send(con, ectx); ErrorThrow(ectx); }
		oref<IDeviceResourceHandle> IDeviceFactory::ReceiveResourceHandle(IPC::IConnection * con) { ErrorContext ectx; ErrorClear(ectx); auto result = ReceiveResourceHandle(con, ectx); ErrorThrow(ectx); return result; }
		oref<IFont> IDeviceContextFactory2D::CreateFont(const string & font_face, uint style, uint height) { ErrorContext ectx; ErrorClear(ectx); auto result = CreateFont(font_face, style, height, ectx); ErrorThrow(ectx); return result; }
		oref<IFont> IDeviceContextFactory2D::LoadFont(Stream * stream, uint height) { ErrorContext ectx; ErrorClear(ectx); auto result = LoadFont(stream, height, ectx); ErrorThrow(ectx); return result; }
		oref<IFont> IDeviceContextFactory2D::SearchFont(IFont * base_font, const unichar32 * chars, uint count) { ErrorContext ectx; ErrorClear(ectx); auto result = SearchFont(base_font, chars, count, ectx); ErrorThrow(ectx); return result; }
	}
}