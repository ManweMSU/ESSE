#include "DeviceCairo.h"

#include <math.h>

namespace ESSE
{
	namespace Cairo
	{
		CairoBitmap::CairoBitmap(CairoAPI * api, Picturae::Picture * source) : _api(api), _surface(0) { if (!Update(source)) throw OutOfMemoryException(); }
		CairoBitmap::CairoBitmap(CairoAPI * api, uint width, uint height, const Color & clear_color) : _api(api)
		{
			if (!width || !height) throw InvalidArgumentException();
			Picturae::PictureDesc desc;
			desc.width = width;
			desc.height = height;
			desc.stride = _api->cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, desc.width);
			desc.format = Picturae::PixelFormat::B8G8R8A8;
			desc.alpha_mode = Picturae::AlphaMode::Premultiplied;
			desc.origin = Picturae::ScanOrigin::TopLeft;
			desc.palette_size = 0;
			_data = owrap(new Picturae::Picture(desc, Picturae::PictureInit::AllocateUninitialized));
			Color color_prem;
			color_prem.r = int(clear_color.b) * int(clear_color.a) / 255;
			color_prem.g = int(clear_color.g) * int(clear_color.a) / 255;
			color_prem.b = int(clear_color.r) * int(clear_color.a) / 255;
			color_prem.a = clear_color.a;
			for (uint y = 0; y < height; y++) for (uint x = 0; x < width; x++) _data->SetPixel(x, y, color_prem.value);
			_surface = _api->cairo_image_surface_create_for_data(reinterpret_cast<uint8 *>(_data->GetDesc().data), CAIRO_FORMAT_ARGB32, desc.width, desc.height, desc.stride);
			if (_api->cairo_surface_status(_surface) != CAIRO_STATUS_SUCCESS) {
				_api->cairo_surface_destroy(_surface);
				throw OutOfMemoryException();
			}
		}
		CairoBitmap::~CairoBitmap(void) { if (_surface) _api->cairo_surface_destroy(_surface); }
		string CairoBitmap::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Cairo bitmap"; ESSE_TRY_OUTRO(string()) }
		uint CairoBitmap::GetWidth(void) noexcept { return _data->GetDesc().width; }
		uint CairoBitmap::GetHeight(void) noexcept { return _data->GetDesc().height; }
		bool CairoBitmap::Update(Picturae::Picture * source) noexcept
		{
			try {
				if (!source) return false;
				Picturae::PictureDesc desc;
				desc.width = source->GetDesc().width;
				desc.height = source->GetDesc().height;
				desc.stride = _api->cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, desc.width);
				desc.format = Picturae::PixelFormat::B8G8R8A8;
				desc.alpha_mode = Picturae::AlphaMode::Premultiplied;
				desc.origin = Picturae::ScanOrigin::TopLeft;
				desc.palette_size = 0;
				auto fnov = owrap(new Picturae::Picture(desc, Picturae::PictureInit::AllocateUninitialized));
				Picturae::BlockTransfer(fnov->GetDesc(), source->GetDesc());
				auto surface = _api->cairo_image_surface_create_for_data(reinterpret_cast<uint8 *>(fnov->GetDesc().data), CAIRO_FORMAT_ARGB32, desc.width, desc.height, desc.stride);
				if (_api->cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
					_api->cairo_surface_destroy(surface);
					return false;
				}
				if (_surface) _api->cairo_surface_destroy(_surface);
				_surface = surface;
				_data = fnov;
				return true;
			} catch (...) { return false; }
		}
		oref<Picturae::Picture> CairoBitmap::QueryContents(void) noexcept { try { return owrap(new Picturae::Picture(_data)); } catch (...) { return 0; } }
		Picturae::Picture * CairoBitmap::GetData(void) const noexcept { return _data; }
		cairo_surface_t CairoBitmap::GetSurface(void) const noexcept { return _surface; }

		CairoFont::CairoFont(Graphica::IDeviceContextFactory2D * parent, CairoAPI * capi, FreeTypeAPI * tapi, FT_Face ff, uint height) : _parent_factory(parent), _capi(capi), _tapi(tapi), _font_face(ff), _height(height)
		{
			if (_tapi->FT_Select_Charmap(_font_face, FT_ENCODING_UNICODE)) throw InvalidStateException();
			if (_font_face->face_flags & FT_FACE_FLAG_SCALABLE) {
				_scale_factor = double(_font_face->units_per_EM) / double(_font_face->ascender - _font_face->descender);
			} else {
				int match = -1;
				uint dh = 0;
				for (int i = 0; i < _font_face->num_fixed_sizes; i++) {
					auto & size = _font_face->available_sizes[i];
					auto local_dh = uint(abs(int(size.height) - int(_height)));
					if (local_dh < dh || match < 0) { dh = local_dh; match = i; }
				}
				if (match < 0) throw InvalidFormatException();
				if (_tapi->FT_Select_Size(_font_face, match)) throw InvalidStateException();
				auto & size = _font_face->size->metrics;
				_scale_factor = double(size.height) / double(size.ascender - size.descender);
			}
			_font = _capi->cairo_ft_font_face_create_for_ft_face(_font_face, 0);
			if (_capi->cairo_font_face_status(_font) != CAIRO_STATUS_SUCCESS) {
				_capi->cairo_font_face_destroy(_font);
				throw OutOfMemoryException();
			}
			_emulated_style = 0;
		}
		CairoFont::~CairoFont(void) { _capi->cairo_font_face_destroy(_font); _tapi->FT_Done_Face(_font_face); }
		string CairoFont::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Cairo font"; ESSE_TRY_OUTRO(string()) }
		string CairoFont::GetFontFace(void) noexcept { try { return _font_face->family_name; } catch (...) { return string(); } }
		uint CairoFont::GetFontStyle(void) noexcept
		{
			uint style = 0;
			if (_font_face->style_flags & FT_STYLE_FLAG_BOLD) style |= Graphica::CreateFontWeight700;
			else style |= Graphica::CreateFontWeight400;
			if (_font_face->style_flags & FT_STYLE_FLAG_ITALIC) style |= Graphica::CreateFontItalic;
			return style | _emulated_style;
		}
		uint CairoFont::GetHeight(void) noexcept { return _height; }
		void CairoFont::GetFontMetrics(Graphica::FontMetrics & metrics) noexcept
		{
			if (_font_face->face_flags & FT_FACE_FLAG_SCALABLE) {
				auto scale = double(_height) * _scale_factor / _font_face->units_per_EM;
				metrics.Ascent = _font_face->ascender * scale;
				metrics.Descent = _font_face->descender * scale;
				metrics.LineSpacing = (_font_face->height - _font_face->ascender + _font_face->descender) * scale;
				metrics.UnderlinePosition = _font_face->underline_position * scale;
				metrics.UnderlineWidth = metrics.StrikeoutWidth = _font_face->underline_thickness * scale;
				metrics.StrikeoutPosition = ((_font_face->ascender - _font_face->descender) / 2.0) * scale;
			} else {
				auto & m = _font_face->size->metrics;
				auto scale = double(_height) * _scale_factor / m.height;
				metrics.Ascent = m.ascender * scale;
				metrics.Descent = m.descender * scale;
				metrics.LineSpacing = (m.height - m.ascender + m.descender) * scale;
				metrics.UnderlinePosition = metrics.Descent / 2.0;
				metrics.UnderlineWidth = metrics.StrikeoutWidth = max(double(_height) / 20.0, 1.0);
				metrics.StrikeoutPosition = (metrics.Ascent - metrics.Descent) / 2.0;
			}
		}
		void CairoFont::GetGlyphMetrics(const uint * glyph, Graphica::FontGlyphMetrics * metrics, uintptr length) noexcept
		{
			int32 mode = FT_LOAD_NO_SCALE;
			double scale;
			if (_font_face->face_flags & FT_FACE_FLAG_SCALABLE) scale = double(_height) * _scale_factor / _font_face->units_per_EM;
			else { mode = 0; scale = double(_height) * _scale_factor / double(_font_face->size->metrics.height); }
			for (uintptr i = 0; i < length; i++) {
				if (glyph[i] != Graphica::InvalidGlyph && !_tapi->FT_Load_Glyph(_font_face, glyph[i], mode)) {
					auto & gm = _font_face->glyph->metrics;
					metrics[i].HorizontalAdvance = gm.horiAdvance * scale;
					metrics[i].HorizontalLeftBearing = gm.horiBearingX * scale;
					metrics[i].HorizontalRightBearing = (gm.horiAdvance - gm.horiBearingX - gm.width) * scale;
				} else Memory::ZeroMemory(&metrics[i], sizeof(Graphica::FontGlyphMetrics));
			}
		}
		void CairoFont::GetGlyphsForCharacters(const unichar32 * chr, uint * glyph, uintptr length) noexcept
		{
			for (uintptr i = 0; i < length; i++) {
				auto gi = _tapi->FT_Get_Char_Index(_font_face, chr[i]);
				if (gi) glyph[i] = gi; else glyph[i] = Graphica::InvalidGlyph;
			}
		}
		void CairoFont::SimulateOblique(void) noexcept { _emulated_style = Graphica::CreateFontOblique; _capi->cairo_ft_font_face_set_synthesize(_font, CAIRO_FT_SYNTHESIZE_OBLIQUE); }
		uint CairoFont::GetCairoHeight(void) const noexcept { return _height; }
		cairo_font_face_t CairoFont::GetCairoFont(void) const noexcept { return _font; }
		double CairoFont::GetCairoFontScale(void) const noexcept { return _scale_factor; }

		class CairoColorBrush : public Graphica::IColorBrush
		{
			friend class CairoDevice;
		private:
			oref<CairoAPI> _api;
			Graphica::IDeviceContext2D * _parent;
			cairo_pattern_t _gradient;
			double _r, _g, _b, _a;
			Index2 _from, _to;
		public:
			CairoColorBrush(Graphica::IDeviceContext2D * parent, const Color & color) : _parent(parent), _gradient(0)
			{
				_r = double(color.r) / 255.0;
				_g = double(color.g) / 255.0;
				_b = double(color.b) / 255.0;
				_a = double(color.a) / 255.0;
				_from = _to = Index2(0, 0);
			}
			CairoColorBrush(Graphica::IDeviceContext2D * parent, CairoAPI * api, const Index2 & from, const Index2 & to, const Color * colors, const double * positions, uint count) : _parent(parent), _api(api)
			{
				_r = _g = _b = _a = 0.0;
				_gradient = _api->cairo_pattern_create_linear(0.0, 0.0, 1.0, 0.0);
				if (_api->cairo_pattern_status(_gradient) != CAIRO_STATUS_SUCCESS) {
					_api->cairo_pattern_destroy(_gradient);
					throw OutOfMemoryException();
				}
				for (uint i = 0; i < count; i++) {
					_api->cairo_pattern_add_color_stop_rgba(_gradient, positions[i], double(colors[i].r) / 255.0, double(colors[i].g) / 255.0, double(colors[i].b) / 255.0, double(colors[i].a) / 255.0);
				}
				if (_api->cairo_pattern_status(_gradient) != CAIRO_STATUS_SUCCESS) {
					_api->cairo_pattern_destroy(_gradient);
					throw OutOfMemoryException();
				}
				OverrideGradientPoints(from, to);
			}
			virtual ~CairoColorBrush(void) override { if (_gradient && _api) _api->cairo_pattern_destroy(_gradient); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Cairo color brush"; ESSE_TRY_OUTRO(string()) }
			virtual Graphica::IDevice * GetParentDevice(void) noexcept override { return 0; }
			virtual Graphica::IDeviceContext2D * GetParentContext(void) noexcept override { return _parent; }
			virtual Graphica::BrushType GetBrushType(void) noexcept override { return Graphica::BrushType::Color; }
			virtual void OverrideGradientPoints(const Index2 & from, const Index2 & to) noexcept override
			{
				_from = from; _to = to;
				double dx = to.x - from.x, dy = to.y - from.y;
				double a = atan2(dy, dx);
				double ir = 1.0 / sqrt(dx * dx + dy * dy);
				cairo_matrix_t matrix;
				_api->cairo_matrix_init_scale(matrix, ir, ir);
				_api->cairo_matrix_rotate(matrix, -a);
				_api->cairo_matrix_translate(matrix, -_from.x, -_from.y);
				_api->cairo_pattern_set_matrix(_gradient, matrix);
			}
		};
		class CairoBitmapBrush : public Graphica::IBitmapBrush
		{
			friend class CairoDevice;
		private:
			oref<CairoAPI> _api;
			Graphica::IDeviceContext2D * _parent;
			oref<CairoBitmap> _bitmap;
			cairo_surface_t _subsurface;
			cairo_pattern_t _pattern;
			double _fxw, _fxh;
			Rectangle _tile_ref;
			bool _tile_mode;
		private:
			void _init(const Rectangle & area)
			{
				auto entire = Rectangle(0, 0, _bitmap->GetWidth(), _bitmap->GetHeight());
				auto fx = Rectangle::Intersect(area, entire);
				if (fx.left != 0 || fx.top != 0 || fx.right != _bitmap->GetWidth() || fx.bottom != _bitmap->GetHeight()) {
					_subsurface = _api->cairo_surface_create_for_rectangle(_bitmap->GetSurface(), fx.left, fx.top, fx.right - fx.left, fx.bottom - fx.top);
					if (_api->cairo_surface_status(_subsurface) != CAIRO_STATUS_SUCCESS) {
						_api->cairo_surface_destroy(_subsurface);
						throw OutOfMemoryException();
					}
				} else _subsurface = 0;
				_fxw = fx.right - fx.left;
				_fxh = fx.bottom - fx.top;
				_pattern = _api->cairo_pattern_create_for_surface(_subsurface ? _subsurface : _bitmap->GetSurface());
				if (_api->cairo_pattern_status(_pattern) != CAIRO_STATUS_SUCCESS) {
					_api->cairo_surface_destroy(_subsurface);
					_api->cairo_pattern_destroy(_pattern);
					throw OutOfMemoryException();
				}
				_api->cairo_pattern_set_filter(_pattern, 3);
				if (_tile_mode) {
					_api->cairo_pattern_set_extend(_pattern, 1);
					OverrideTileReferenceRectangle(entire);
				}
			}
		public:
			CairoBitmapBrush(Graphica::IDeviceContext2D * parent, CairoAPI * api, Graphica::IBitmap * bitmap, const Rectangle & area, bool tile) : _parent(parent), _api(api), _tile_mode(tile) { _bitmap = static_cast<CairoBitmap *>(bitmap); _init(area); }
			CairoBitmapBrush(Graphica::IDeviceContext2D * parent, CairoAPI * api, Graphica::IBitmapBrush * bitmap, const Rectangle & area, bool tile) : _parent(parent), _api(api), _tile_mode(tile) { _bitmap = static_cast<CairoBitmapBrush *>(bitmap)->_bitmap; _init(area); }
			virtual ~CairoBitmapBrush(void) override { if (_subsurface) _api->cairo_surface_destroy(_subsurface); if (_pattern) _api->cairo_pattern_destroy(_pattern); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Cairo bitmap brush"; ESSE_TRY_OUTRO(string()) }
			virtual Graphica::IDevice * GetParentDevice(void) noexcept override { return 0; }
			virtual Graphica::IDeviceContext2D * GetParentContext(void) noexcept override { return _parent; }
			virtual Graphica::BrushType GetBrushType(void) noexcept override { return Graphica::BrushType::Bitmap; }
			virtual void OverrideTileReferenceRectangle(const Rectangle & rect) noexcept override
			{
				_tile_ref = rect;
				cairo_matrix_t matrix;
				_api->cairo_matrix_init_scale(matrix, _fxw / double(_tile_ref.right - _tile_ref.left), _fxh / double(_tile_ref.bottom - _tile_ref.top));
				_api->cairo_matrix_translate(matrix, -_tile_ref.left, -_tile_ref.top);
				_api->cairo_pattern_set_matrix(_pattern, matrix);
			}
		};
		class CairoInversionEffectBrush : public Graphica::IInversionEffectBrush
		{
			friend class CairoDevice;
		private:
			Graphica::IDeviceContext2D * _parent;
		public:
			CairoInversionEffectBrush(Graphica::IDeviceContext2D * parent) : _parent(parent) {}
			virtual ~CairoInversionEffectBrush(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Cairo inversion effect brush"; ESSE_TRY_OUTRO(string()) }
			virtual Graphica::IDevice * GetParentDevice(void) noexcept override { return 0; }
			virtual Graphica::IDeviceContext2D * GetParentContext(void) noexcept override { return _parent; }
			virtual Graphica::BrushType GetBrushType(void) noexcept override { return Graphica::BrushType::Inversion; }
		};
		class CairoLayerBacking : public Graphica::ILayerBacking
		{
			friend class CairoDevice;
		private:
			oref<CairoAPI> _api;
			Graphica::IDeviceContext2D * _parent;
			cairo_pattern_t _alpha_pattern;
			Rectangle _rect;
			double _alpha;
			bool _alpha_mode;
		public:
			CairoLayerBacking(Graphica::IDeviceContext2D * parent, CairoAPI * api) : _parent(parent), _api(api), _alpha_pattern(0), _alpha(0.0), _alpha_mode(false) {}
			virtual ~CairoLayerBacking(void) override { if (_alpha_pattern) _api->cairo_pattern_destroy(_alpha_pattern); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Cairo layer backing"; ESSE_TRY_OUTRO(string()) }
			virtual Graphica::IDevice * GetParentDevice(void) noexcept override { return 0; }
			virtual Graphica::IDeviceContext2D * GetParentContext(void) noexcept override { return _parent; }
		};
		class CairoGlyphRun : public Graphica::IGlyphRun
		{
			friend class CairoDevice;
		private:
			struct glyph_factor { oref<Graphica::IFont> font; Color color; };
			struct glyph_run { array<cairo_glyph_t> data; double r, g, b, a; };
			friend bool operator == (const glyph_factor & a, const glyph_factor & b) noexcept { return a.font.Inner() == b.font.Inner() && a.color.value == b.color.value; }
			friend bool operator != (const glyph_factor & a, const glyph_factor & b) noexcept { return a.font.Inner() != b.font.Inner() || a.color.value != b.color.value; }
			friend bool operator < (const glyph_factor & a, const glyph_factor & b) noexcept { return a.font.Inner() < b.font.Inner() || (a.font.Inner() == b.font.Inner() && a.color.value < b.color.value); }
			friend bool operator > (const glyph_factor & a, const glyph_factor & b) noexcept { return a.font.Inner() > b.font.Inner() || (a.font.Inner() == b.font.Inner() && a.color.value > b.color.value); }
			friend bool operator <= (const glyph_factor & a, const glyph_factor & b) noexcept { return a.font.Inner() < b.font.Inner() || (a.font.Inner() == b.font.Inner() && a.color.value <= b.color.value); }
			friend bool operator >= (const glyph_factor & a, const glyph_factor & b) noexcept { return a.font.Inner() > b.font.Inner() || (a.font.Inner() == b.font.Inner() && a.color.value >= b.color.value); }
		private:
			oref<CairoAPI> _api;
			Graphica::IDeviceContext2D * _parent;
			cairo_matrix_t _transform;
			Dictionary<glyph_factor, glyph_run> _glyphs;
		public:
			CairoGlyphRun(Graphica::IDeviceContext2D * parent, CairoAPI * api, Graphica::IFont ** fonts, const uint * glyphs, const double * px, const double * py, const Color * colors, uint count, const double * transform) : _parent(parent), _api(api)
			{
				if (!fonts || !glyphs || !px || !py || !colors) throw InvalidArgumentException();
				if (transform) _api->cairo_matrix_init(_transform, transform[0], transform[3], transform[1], transform[4], transform[2], transform[5]);
				else _api->cairo_matrix_init_identity(_transform);
				for (uint i = 0; i < count; i++) {
					if (glyphs[i] == Graphica::InvalidGlyph) continue;
					if (!fonts[i]) throw InvalidArgumentException();
					glyph_factor gf = { .font = fonts[i], .color = colors[i] };
					if (!_glyphs.ElementExists(gf)) {
						glyph_run gr = { .data = array<cairo_glyph_t>(count), .r = double(gf.color.r) / 255.0, .g = double(gf.color.g) / 255.0, .b = double(gf.color.b) / 255.0, .a = double(gf.color.a) / 255.0 };
						_glyphs.Append(gf, gr);
					}
					auto gr = _glyphs[gf];
					if (!gr) throw InvalidStateException();
					gr->data.Append(cairo_glyph_t { .index = glyphs[i], .x = px[i], .y = py[i] });
				}
			}
			virtual ~CairoGlyphRun(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Cairo glyph run"; ESSE_TRY_OUTRO(string()) }
			virtual Graphica::IDevice * GetParentDevice(void) noexcept override { return 0; }
			virtual Graphica::IDeviceContext2D * GetParentContext(void) noexcept override { return _parent; }
		};

		void CairoDevice::_perform_brush_setup(cairo_t ctx, Graphica::IBrush * brush, const Rectangle & at) noexcept
		{
			if (brush->GetBrushType() == Graphica::BrushType::Color) {
				auto b = static_cast<CairoColorBrush *>(brush);
				if (b->_gradient) _api->cairo_set_source(ctx, b->_gradient);
				else _api->cairo_set_source_rgba(ctx, b->_r, b->_g, b->_b, b->_a);
			} else if (brush->GetBrushType() == Graphica::BrushType::Bitmap) {
				auto b = static_cast<CairoBitmapBrush *>(brush);
				if (!b->_tile_mode) {
					cairo_matrix_t matrix;
					_api->cairo_matrix_init_scale(matrix, b->_fxw / double(at.right - at.left), b->_fxh / double(at.bottom - at.top));
					_api->cairo_matrix_translate(matrix, -at.left, -at.top);
					_api->cairo_pattern_set_matrix(b->_pattern, matrix);
				}
				_api->cairo_set_source(ctx, b->_pattern);
			} else if (brush->GetBrushType() == Graphica::BrushType::Inversion) {
				_api->cairo_set_source_rgb(ctx, 1.0, 1.0, 1.0);
				_api->cairo_set_operator(ctx, 23);
			}
		}
		void CairoDevice::_perform_brush_finalization(cairo_t ctx, Graphica::IBrush * brush) noexcept
		{
			if (brush->GetBrushType() == Graphica::BrushType::Inversion) {
				_api->cairo_set_operator(_context, 2);
			}
		}
		CairoDevice::CairoDevice(CairoAPI * api, Graphica::IDeviceContextFactory2D * parent, Graphica::IBitmap * dest) : _api(api), _parent(parent), _bitmap(dest), _state(false)
		{
			if (!dest) throw InvalidArgumentException();
			_viewport.x = dest->GetWidth();
			_viewport.y = dest->GetHeight();
			_context = _api->cairo_create(static_cast<CairoBitmap *>(dest)->GetSurface());
			if (_api->cairo_status(_context) != CAIRO_STATUS_SUCCESS) { _api->cairo_destroy(_context); throw OutOfMemoryException(); }
		}
		CairoDevice::~CairoDevice(void) { if (_context) _api->cairo_destroy(_context); }
		string CairoDevice::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Cairo device context"; ESSE_TRY_OUTRO(string()) }
		void * CairoDevice::DynamicCast(const void * cls, ErrorContext & ectx) noexcept
		{
			if (cls == Classes.Object || cls == Classes.DynamicObject || cls == Classes.IDeviceContext2D) {
				Retain(); return this;
			} else if (cls == Classes.IDeviceContextFactory2D) {
				_parent->Retain(); return _parent;
			} else ErrorSet(ectx, Errores::ErrorNotImplemented);
		}
		const void * CairoDevice::GetType(void) noexcept { return Classes.IDeviceContext2D; }
		void CairoDevice::GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) noexcept
		{
			try {
				tech = U"Cairo";
				uint version = _api->cairo_version();
				version_major = version / 10000;
				version_minor = version / 100 % 100;
			} catch (...) {}
		}
		uint32 CairoDevice::GetImplementationFeatures(void) noexcept
		{
			// TODO: IMPLEMENT DeviceContextPresentationContext
			uint result = Graphica::DeviceContextSupportsInversionEffect | Graphica::DeviceContextSupportsPolygons | Graphica::DeviceContextSupportsLayers;
			if (_bitmap) result |= Graphica::DeviceContextBitmapContext;
			return result;
		}
		oref<Graphica::IColorBrush> CairoDevice::CreateSolidColorBrush(const Color & color) noexcept
		{
			try { return oref<Graphica::IColorBrush>::CreateOwned(new CairoColorBrush(this, color)); } catch (...) { return 0; }
		}
		oref<Graphica::IColorBrush> CairoDevice::CreateGradientBrush(const Index2 & from, const Index2 & to, const Color * colors, const double * positions, uint count) noexcept
		{
			if (!count) return 0;
			if (count == 1) return CreateSolidColorBrush(colors[0]);
			try { return oref<Graphica::IColorBrush>::CreateOwned(new CairoColorBrush(this, _api, from, to, colors, positions, count)); } catch (...) { return 0; }
		}
		oref<Graphica::IBitmapBrush> CairoDevice::CreateBitmapBrush(Graphica::IBitmap * bitmap, const Rectangle & area) noexcept
		{
			if (!bitmap) return 0;
			try { return oref<Graphica::IBitmapBrush>::CreateOwned(new CairoBitmapBrush(this, _api, bitmap, area, false)); } catch (...) { return 0; }
		}
		oref<Graphica::IBitmapBrush> CairoDevice::CreateBitmapBrushCopy(Graphica::IBitmapBrush * bitmap, const Rectangle & area) noexcept
		{
			if (!bitmap) return 0;
			try { return oref<Graphica::IBitmapBrush>::CreateOwned(new CairoBitmapBrush(this, _api, bitmap, area, false)); } catch (...) { return 0; }
		}
		oref<Graphica::IBitmapBrush> CairoDevice::CreateTileBrush(Graphica::IBitmap * bitmap, const Rectangle & area) noexcept
		{
			if (!bitmap) return 0;
			try { return oref<Graphica::IBitmapBrush>::CreateOwned(new CairoBitmapBrush(this, _api, bitmap, area, true)); } catch (...) { return 0; }
		}
		oref<Graphica::IBitmapBrush> CairoDevice::CreateTileBrushCopy(Graphica::IBitmapBrush * bitmap, const Rectangle & area) noexcept
		{
			if (!bitmap) return 0;
			try { return oref<Graphica::IBitmapBrush>::CreateOwned(new CairoBitmapBrush(this, _api, bitmap, area, true)); } catch (...) { return 0; }
		}
		oref<Graphica::IBitmapBrush> CairoDevice::CreateTextureBrush(Graphica::ITexture * texture, Graphica::TextureAlphaMode mode) noexcept { return 0; }
		oref<Graphica::IBlurEffectBrush> CairoDevice::CreateBlurEffectBrush(double sigma) noexcept { return 0; }
		oref<Graphica::IInversionEffectBrush> CairoDevice::CreateInversionEffectBrush(void) noexcept
		{
			try { return oref<Graphica::IInversionEffectBrush>::CreateOwned(new CairoInversionEffectBrush(this)); } catch (...) { return 0; }
		}
		oref<Graphica::ILayerBacking> CairoDevice::CreateLayerBackingStorage(void) noexcept
		{
			try { return oref<Graphica::ILayerBacking>::CreateOwned(new CairoLayerBacking(this, _api)); } catch (...) { return 0; }
		}
		oref<Graphica::IGlyphRun> CairoDevice::CreateGlyphRun(Graphica::IFont ** fonts, const uint * glyphs, const double * px, const double * py, const Color * colors, uint count, const double * transform) noexcept
		{
			try { return oref<Graphica::IGlyphRun>::CreateOwned(new CairoGlyphRun(this, _api, fonts, glyphs, px, py, colors, count, transform)); } catch (...) { return 0; }
		}
		void CairoDevice::PushClip(const Rectangle & rect) noexcept
		{
			_api->cairo_save(_context);
			if (rect.left < rect.right && rect.top < rect.bottom) _api->cairo_rectangle(_context, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
			_api->cairo_clip(_context);
		}
		void CairoDevice::PopClip(void) noexcept { _api->cairo_restore(_context); }
		bool CairoDevice::BeginLayerAlpha(Graphica::ILayerBacking * layer, const Rectangle & rect) noexcept
		{
			if (!layer) return false;
			auto l = static_cast<CairoLayerBacking *>(layer);
			if (l->_alpha_pattern || l->_alpha_mode) return false;
			l->_rect = rect;
			_api->cairo_save(_context);
			if (rect.left < rect.right && rect.top < rect.bottom) _api->cairo_rectangle(_context, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
			_api->cairo_clip(_context);
			_api->cairo_push_group_with_content(_context, CAIRO_CONTENT_ALPHA);
			l->_alpha_mode = true;
			return true;
		}
		bool CairoDevice::BeginLayer(Graphica::ILayerBacking * layer, const Rectangle & rect, double opacity) noexcept
		{
			if (!layer) return false;
			auto l = static_cast<CairoLayerBacking *>(layer);
			if (l->_alpha_mode) {
				l->_alpha_pattern = _api->cairo_pop_group(_context);
				l->_alpha_mode = false;
				l->_alpha = 1.0;
				_api->cairo_push_group_with_content(_context, CAIRO_CONTENT_COLOR_ALPHA);
				return true;
			} else {
				if (l->_alpha_pattern) return false;
				l->_rect = rect;
				l->_alpha = opacity;
				_api->cairo_save(_context);
				if (rect.left < rect.right && rect.top < rect.bottom) _api->cairo_rectangle(_context, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
				_api->cairo_clip(_context);
				_api->cairo_push_group_with_content(_context, CAIRO_CONTENT_COLOR_ALPHA);
				return true;
			}
		}
		void CairoDevice::EndLayer(Graphica::ILayerBacking * layer) noexcept
		{
			if (!layer) return;
			auto l = static_cast<CairoLayerBacking *>(layer);
			if (l->_alpha_mode) return;
			if (l->_alpha_pattern) {
				_api->cairo_pop_group_to_source(_context);
				_api->cairo_mask(_context, l->_alpha_pattern);
				_api->cairo_restore(_context);
				_api->cairo_pattern_destroy(l->_alpha_pattern);
				l->_alpha_pattern = 0;
			} else {
				_api->cairo_pop_group_to_source(_context);
				if (l->_alpha != 1.0) _api->cairo_paint_with_alpha(_context, l->_alpha);
				else _api->cairo_paint(_context);
				_api->cairo_restore(_context);
			}
		}
		void CairoDevice::Render(Graphica::IBrush * brush, const Rectangle & at) noexcept
		{
			if (!brush || at.left >= at.right || at.top >= at.bottom) return;
			_perform_brush_setup(_context, brush, at);
			_api->cairo_rectangle(_context, at.left, at.top, at.right - at.left, at.bottom - at.top);
			_api->cairo_fill(_context);
			_perform_brush_finalization(_context, brush);
		}
		void CairoDevice::RenderPolyline(const double * px, const double * py, uint count, bool closed, Graphica::IBrush * brush, double width) noexcept
		{
			if (count < 2) return;
			Rectangle aabb(floor(px[0]), floor(py[0]), ceil(px[0]) + 1, ceil(py[0]) + 1);
			_api->cairo_move_to(_context, px[0], py[0]);
			for (uint i = 1; i < count; i++) {
				_api->cairo_line_to(_context, px[i], py[i]);
				if (floor(px[i]) < aabb.left) aabb.left = floor(px[i]);
				if (ceil(px[i]) > aabb.right - 1) aabb.right = ceil(px[i]) + 1;
				if (floor(py[i]) < aabb.top) aabb.top = floor(py[i]);
				if (ceil(py[i]) > aabb.bottom - 1) aabb.bottom = ceil(py[i]) + 1;
			}
			auto ext = int(ceil(width / 2.0));
			aabb.left -= ext; aabb.top -= ext; aabb.right += ext; aabb.bottom += ext;
			if (closed) _api->cairo_close_path(_context);
			_api->cairo_set_line_width(_context, width);
			_api->cairo_set_line_join(_context, 1);
			_api->cairo_set_line_cap(_context, 1);
			_perform_brush_setup(_context, brush, aabb);
			_api->cairo_stroke(_context);
			_perform_brush_finalization(_context, brush);
		}
		void CairoDevice::RenderPolygon(const double * px, const double * py, uint count, Graphica::IBrush * brush) noexcept
		{
			if (count < 2) return;
			Rectangle aabb(floor(px[0]), floor(py[0]), ceil(px[0]) + 1, ceil(py[0]) + 1);
			_api->cairo_move_to(_context, px[0], py[0]);
			for (uint i = 1; i < count; i++) {
				_api->cairo_line_to(_context, px[i], py[i]);
				if (floor(px[i]) < aabb.left) aabb.left = floor(px[i]);
				if (ceil(px[i]) > aabb.right - 1) aabb.right = ceil(px[i]) + 1;
				if (floor(py[i]) < aabb.top) aabb.top = floor(py[i]);
				if (ceil(py[i]) > aabb.bottom - 1) aabb.bottom = ceil(py[i]) + 1;
			}
			_api->cairo_close_path(_context);
			_perform_brush_setup(_context, brush, aabb);
			_api->cairo_fill(_context);
			_perform_brush_finalization(_context, brush);
		}
		void CairoDevice::RenderGlyphRun(Graphica::IGlyphRun * run, const Index2 & at) noexcept
		{
			if (!run) return;
			auto r = static_cast<CairoGlyphRun *>(run);
			cairo_matrix_t base, translation;
			_api->cairo_get_matrix(_context, base);
			_api->cairo_matrix_init_translate(translation, at.x, at.y);
			_api->cairo_matrix_multiply(translation, r->_transform, translation);
			_api->cairo_set_matrix(_context, translation);
			for (auto & sr : r->_glyphs) {
				auto font = static_cast<CairoFont *>(sr.key.font.Inner());
				_api->cairo_set_font_face(_context, font->GetCairoFont());
				_api->cairo_set_font_size(_context, font->GetCairoHeight() * font->GetCairoFontScale());
				_api->cairo_set_source_rgba(_context, sr.value.r, sr.value.g, sr.value.b, sr.value.a);
				_api->cairo_show_glyphs(_context, sr.value.data, sr.value.data.GetLength());
			}
			_api->cairo_set_font_face(_context, 0);
			_api->cairo_set_matrix(_context, base);
		}
		bool CairoDevice::BeginRendering(Graphica::TextureLoadAction load, const Color & clear_color) noexcept
		{
			if (_state) return false;
			if (load == Graphica::TextureLoadAction::Clear) {
				auto dest = static_cast<CairoBitmap *>(_bitmap.Inner());
				auto & data = dest->GetData()->GetDesc();
				Color color_prem;
				color_prem.r = uint(clear_color.b) * uint(clear_color.a) / 255;
				color_prem.g = uint(clear_color.g) * uint(clear_color.a) / 255;
				color_prem.b = uint(clear_color.r) * uint(clear_color.a) / 255;
				color_prem.a = clear_color.a;
				for (uint y = 0; y < data.height; y++) for (uint x = 0; x < data.width; x++) Picturae::SetPixel(data, x, y, color_prem.value);
				_api->cairo_surface_mark_dirty(dest->GetSurface());
			}
			_state = true;
			return true;
		}
		bool CairoDevice::EndRendering(void) noexcept
		{
			if (!_state) return false;
			_api->cairo_surface_flush(static_cast<CairoBitmap *>(_bitmap.Inner())->GetSurface());
			_state = false;
			return true;
		}
		cairo_t CairoDevice::GetCairo(void) const noexcept { return _context; }
		CairoAPI * CairoDevice::GetAPI(void) const noexcept { return _api; }
	}
}