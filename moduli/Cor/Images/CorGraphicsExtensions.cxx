#include "CorGraphicsExtensions.h"

#include <math.h>

namespace ESSE
{
	namespace Graphica
	{
		AggregateFont::AggregateFont(IDeviceContextFactory2D * factory, IFont * primary) noexcept : _factory(factory), _primary(primary), _surrogate(0x10) {}
		AggregateFont::AggregateFont(IDeviceContextFactory2D * factory, const string & font_face, uint style, uint height) : _factory(factory), _surrogate(0x10) { _primary = factory->CreateFont(font_face, style, height); }
		AggregateFont::AggregateFont(IDeviceContextFactory2D * factory, Stream * stream, uint height) : _factory(factory), _surrogate(0x10){ _primary = factory->LoadFont(stream, height); }
		AggregateFont::~AggregateFont(void) {}
		string AggregateFont::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Aggregate Font"; ESSE_TRY_OUTRO(string()) }
		string AggregateFont::GetFontFace(void) noexcept { return _primary->GetFontFace(); }
		uint AggregateFont::GetFontStyle(void) noexcept { return _primary->GetFontStyle(); }
		uint AggregateFont::GetHeight(void) noexcept { return _primary->GetHeight(); }
		void AggregateFont::GetFontMetrics(FontMetrics & metrics) noexcept { return _primary->GetFontMetrics(metrics); }
		void AggregateFont::GetGlyphsForCharacters(const unichar32 * chr, IFont ** font, uint * glyph, uintptr length) noexcept
		{
			_primary->GetGlyphsForCharacters(chr, glyph, length);
			for (uintptr i = 0; i < length; i++) {
				if (glyph[i] == InvalidGlyph) {
					for (auto & s : _surrogate) {
						s.GetGlyphsForCharacters(chr + i, glyph + i, 1);
						if (glyph[i] != InvalidGlyph) { font[i] = &s; break; }
					}
					if (glyph[i] == InvalidGlyph) {
						ErrorContext ectx; ErrorClear(ectx);
						auto xfont = _factory->SearchFont(_primary, chr + i, 1, ectx);
						if (ErrorTest(ectx)) { font[i] = _primary; continue; }
						xfont->GetGlyphsForCharacters(chr + i, glyph + i, 1);
						if (glyph[i] != InvalidGlyph) try {
							_surrogate.Append(xfont);
							font[i] = xfont;
						} catch (...) { font[i] = _primary; glyph[i] = InvalidGlyph; }
					}
				} else font[i] = _primary;
			}
		}
		IDeviceContextFactory2D * AggregateFont::GetParentFactory(void) noexcept { return _factory; }
		IFont * AggregateFont::GetPrimaryFont(void) noexcept { return _primary; }

		void DeviceCache::_brush_cache_cleanup(void) noexcept
		{
			auto current1 = _color_brushes.GetFirst();
			while (current1) {
				auto next = current1->GetNext();
				if (current1->GetValue().value->GetReferenceCount() == 1) _color_brushes.BinaryTree::Remove(current1);
				current1 = next;
			}
			auto current2 = _blur_brushes.GetFirst();
			while (current2) {
				auto next = current2->GetNext();
				if (current2->GetValue().value->GetReferenceCount() == 1) _blur_brushes.BinaryTree::Remove(current2);
				current2 = next;
			}
		}
		void DeviceCache::_bitmap_cache_cleanup(void) noexcept
		{
			auto current = _device_bitmaps.GetFirst();
			while (current) {
				auto next = current->GetNext();
				auto & set = *current->GetValue().value;
				auto current_brush = set.GetFirst();
				while (current_brush) {
					auto next_brush = current_brush->GetNext();
					if (current_brush->GetValue()->GetReferenceCount() == 1) set.BinaryTree::Remove(current_brush);
					current_brush = next_brush;
				}
				if (set.IsEmpty()) _device_bitmaps.BinaryTree::Remove(current);
				current = next;
			}
		}
		DeviceCache::DeviceCache(IDeviceContext2D * context) : _context(context), _brush_allocation_counter(0), _bitmap_allocation_counter(0) {}
		DeviceCache::~DeviceCache(void) {}
		string DeviceCache::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Device Context Cache"; ESSE_TRY_OUTRO(string()) }
		void DeviceCache::ResetCache(void) noexcept { _color_brushes.Clear(); _device_bitmaps.Clear(); _blur_brushes.Clear(); _inversion_brush.Clear(); _brush_allocation_counter = _bitmap_allocation_counter = 0; }
		void DeviceCache::ResetBitmapCache(IBitmap * bitmap) noexcept { _device_bitmaps.Remove(bitmap); }
		oref<IColorBrush> DeviceCache::CreateSolidColorBrush(const Color & color) noexcept
		{
			auto cached = _color_brushes[color];
			if (cached) return cached;
			auto brush = _context->CreateSolidColorBrush(color);
			if (!brush) return 0;
			try {
				_color_brushes.Append(color, brush);
				_brush_allocation_counter++;
				if (_brush_allocation_counter == 0x80) { _brush_allocation_counter = 0; _brush_cache_cleanup(); }
			} catch (...) {}
			return brush;
		}
		oref<IBitmapBrush> DeviceCache::CreateBitmapBrush(IBitmap * bitmap, const Rectangle & area) noexcept
		{
			auto devset = _device_bitmaps[bitmap];
			oref<IBitmapBrush> brush;
			if (devset && !devset->IsEmpty()) brush = _context->CreateBitmapBrushCopy(devset->GetFirst()->GetValue(), area);
			else brush = _context->CreateBitmapBrush(bitmap, area);
			if (!brush) return 0;
			try {
				if (!devset) {
					_device_bitmaps.Append(bitmap, owrap(new Set<oref<IBitmapBrush>>));
					_bitmap_allocation_counter++;
					devset = _device_bitmaps[bitmap];
				}
				if (!devset) abort();
				devset->AddElement(brush);
				if (_bitmap_allocation_counter == 0x10) { _bitmap_allocation_counter = 0; _bitmap_cache_cleanup(); }
			} catch (...) {}
			return brush;
		}
		oref<IBitmapBrush> DeviceCache::CreateTileBrush(IBitmap * bitmap, const Rectangle & area) noexcept
		{
			auto devset = _device_bitmaps[bitmap];
			oref<IBitmapBrush> brush;
			if (devset && !devset->IsEmpty()) brush = _context->CreateTileBrushCopy(devset->GetFirst()->GetValue(), area);
			else brush = _context->CreateTileBrush(bitmap, area);
			if (!brush) return 0;
			try {
				if (!devset) {
					_device_bitmaps.Append(bitmap, owrap(new Set<oref<IBitmapBrush>>));
					_bitmap_allocation_counter++;
					devset = _device_bitmaps[bitmap];
				}
				if (!devset) abort();
				devset->AddElement(brush);
				if (_bitmap_allocation_counter == 0x10) { _bitmap_allocation_counter = 0; _bitmap_cache_cleanup(); }
			} catch (...) {}
			return brush;
		}
		oref<IBlurEffectBrush> DeviceCache::CreateBlurEffectBrush(double sigma) noexcept
		{
			auto cached = _blur_brushes[sigma];
			if (cached) return cached;
			auto brush = _context->CreateBlurEffectBrush(sigma);
			if (!brush) return 0;
			try {
				_blur_brushes.Append(sigma, brush);
				_brush_allocation_counter++;
				if (_brush_allocation_counter == 0x80) { _brush_allocation_counter = 0; _brush_cache_cleanup(); }
			} catch (...) {}
			return brush;
		}
		oref<IInversionEffectBrush> DeviceCache::CreateInversionEffectBrush(void) noexcept { if (!_inversion_brush) _inversion_brush = _context->CreateInversionEffectBrush(); return _inversion_brush; }

		void Typesetter::_reset_attributes(void) noexcept
		{
			if (_primary_font) {
				_primary_font->GetGlyphsForCharacters(_text, _glyphs, _text.GetLength());
				for (auto & f : _fonts) f = _primary_font;
			} else _primary_aggregate_font->GetGlyphsForCharacters(_text, _fonts, _glyphs, _text.GetLength());
			for (auto & c : _colors) c = _default_color;
			for (uintptr i = 0; i < _glyphs.GetLength(); i++) {
				if (_glyphs[i] == InvalidGlyph) _advances[i] = 0; else {
					FontGlyphMetrics gm;
					_fonts[i]->GetGlyphMetrics(&_glyphs[i], &gm, 1);
					_advances[i] = gm.HorizontalAdvance;
				}
			}
		}
		void Typesetter::_reset_layout(void) noexcept
		{
			uintptr i = 0;
			double x = 0.0, y = _metrics.Ascent;
			double h = _metrics.Ascent - _metrics.Descent;
			while (i < _glyphs.GetLength()) {
				if (_text[i] < U' ') {
					_position_x[i] = x;
					_position_y[i] = y;
					_glyphs[i] = InvalidGlyph;
					if (_text[i] == U'\t') x = ceil(x / (2.0 * h)) * 2.0 * h;
				} else {
					_position_x[i] = x;
					_position_y[i] = y;
					x += _advances[i];
				}
				i++;
			}
			_extents.x = ceil(x);
			_extents.y = ceil(y - _metrics.Descent);
		}
		Typesetter::Typesetter(IFont * font, const string & text, uint flags, const Color & color) : _primary_font(font), _text(text), _default_color(color), _flags(flags),
			_glyphs(1), _fonts(1), _colors(1), _position_x(1), _position_y(1), _advances(1)
		{
			font->GetFontMetrics(_metrics);
			auto length = _text.GetLength();
			_glyphs.SetLength(length); _fonts.SetLength(length); _colors.SetLength(length); _position_x.SetLength(length); _position_y.SetLength(length); _advances.SetLength(length);
			_reset_attributes();
			_reset_layout();
		}
		Typesetter::Typesetter(AggregateFont * font, const string & text, uint flags, const Color & color) : _primary_aggregate_font(font), _text(text), _default_color(color), _flags(flags),
			_glyphs(1), _fonts(1), _colors(1), _position_x(1), _position_y(1), _advances(1)
		{
			font->GetFontMetrics(_metrics);
			auto length = _text.GetLength();
			_glyphs.SetLength(length); _fonts.SetLength(length); _colors.SetLength(length); _position_x.SetLength(length); _position_y.SetLength(length); _advances.SetLength(length);
			_reset_attributes();
			_reset_layout();
		}
		Typesetter::~Typesetter(void) {}
		string Typesetter::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Typesetter"; ESSE_TRY_OUTRO(string()) }
		void Typesetter::Render(IDeviceContext2D * context, DeviceCache * cache, const Rectangle & at) noexcept
		{
			if (!_run) _run = context->CreateGlyphRun(_fonts, _glyphs, _position_x, _position_y, _colors, _glyphs.GetLength(), 0);
			int dx, dy;
			if (_flags & TypesetterFlags::HorizontalAlignmentCenter) dx = (at.right + at.left - _extents.x) / 2;
			else if (_flags & TypesetterFlags::HorizontalAlignmentRight) dx = at.right - _extents.x;
			else dx = at.left;
			if (_flags & TypesetterFlags::VerticalAlignmentCenter) dy = (at.top + at.bottom - _extents.y) / 2;
			else if (_flags & TypesetterFlags::VerticalAlignmentBottom) dy = at.bottom - _extents.y;
			else dy = at.top;
			if (_flags & TypesetterFlags::EnableClipping) context->PushClip(at);
			context->RenderGlyphRun(_run, Index2(dx, dy));
			if (_flags & (TypesetterFlags::Underlined | TypesetterFlags::Strikedout)) {
				if (!_brush) {
					if (cache) _brush = cache->CreateSolidColorBrush(_default_color);
					else _brush = context->CreateSolidColorBrush(_default_color);
				}
				if (_flags & TypesetterFlags::Underlined) {
					auto w2 = _metrics.UnderlineWidth / 2;
					auto base = int(dy + _metrics.Ascent - _metrics.UnderlinePosition - w2);
					context->Render(_brush, Rectangle(dx, base, dx + _extents.x, base + max<int>(_metrics.UnderlineWidth, 1)));
				}
				if (_flags & TypesetterFlags::Strikedout) {
					auto w2 = _metrics.StrikeoutWidth / 2;
					auto base = int(dy + _metrics.Ascent - _metrics.StrikeoutPosition - w2);
					context->Render(_brush, Rectangle(dx, base, dx + _extents.x, base + max<int>(_metrics.StrikeoutWidth, 1)));
				}
			}
			if (_flags & TypesetterFlags::EnableClipping) context->PopClip();
		}
		void Typesetter::ResetCache(void) noexcept { _run.Clear(); _brush.Clear(); }
		uint Typesetter::GetTextLength(void) noexcept { return _text.GetLength(); }
		const unichar32 * Typesetter::GetText(void) noexcept { return _text.GetData(); }
		Index2 Typesetter::GetExtents(void) noexcept { return _extents; }
		void Typesetter::GetGlyphRectangles(uint from, uint count, double * pleft, double * pright, double * ptop, double * pbottom) noexcept
		{
			for (uintptr i = 0; i < count; i++) {
				if (pleft) pleft[i] = _position_x[from + i];
				if (pright) pright[i] = _position_x[from + i] + _advances[from + i];
				if (ptop) ptop[i] = _position_y[from + i] - _metrics.Ascent;
				if (pbottom) pbottom[i] = _position_y[from + i] + _metrics.Descent;
			}
		}
		void Typesetter::SetColors(const Color * colors, uint from, uint num_clr) noexcept
		{
			for (uintptr i = 0; i < num_clr; i++) _colors[from + i] = colors[i];
			_run.Clear();
		}
		void Typesetter::SetAdvances(const double * advances, uint from, uint num_adv) noexcept
		{
			if (advances) for (uintptr i = 0; i < num_adv; i++) _advances[from + i] = advances[i];
			else _reset_attributes();
			_reset_layout();
			_run.Clear();
		}
	}
}