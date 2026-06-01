#pragma once

#include <Cor/Images/CorGraphics.h>
#include "CairoDL.h"

namespace ESSE
{
	namespace Cairo
	{
		class CairoBitmap : public Graphica::IBitmap
		{
			oref<CairoAPI> _api;
			oref<Picturae::Picture> _data;
			cairo_surface_t _surface;
		public:
			CairoBitmap(CairoAPI * api, Picturae::Picture * source);
			CairoBitmap(CairoAPI * api, uint width, uint height, const Color & clear_color);
			virtual ~CairoBitmap(void) override;
			virtual string ToStringE(ErrorContext & ectx) const noexcept override;
			virtual uint GetWidth(void) noexcept override;
			virtual uint GetHeight(void) noexcept override;
			virtual bool Update(Picturae::Picture * source) noexcept override;
			virtual oref<Picturae::Picture> QueryContents(void) noexcept override;
			Picturae::Picture * GetData(void) const noexcept;
			cairo_surface_t GetSurface(void) const noexcept;
		};
		class CairoFont : public Graphica::IFont
		{
			oref<Graphica::IDeviceContextFactory2D> _parent_factory;
			oref<CairoAPI> _capi;
			oref<FreeTypeAPI> _tapi;
			FT_Face _font_face;
			cairo_font_face_t _font;
			uint _height, _emulated_style;
			double _scale_factor;
		public:
			CairoFont(Graphica::IDeviceContextFactory2D * parent, CairoAPI * capi, FreeTypeAPI * tapi, FT_Face ff, uint height);
			virtual ~CairoFont(void) override;
			virtual string ToStringE(ErrorContext & ectx) const noexcept override;
			virtual string GetFontFace(void) noexcept override;
			virtual uint GetFontStyle(void) noexcept override;
			virtual uint GetHeight(void) noexcept override;
			virtual void GetFontMetrics(Graphica::FontMetrics & metrics) noexcept override;
			virtual void GetGlyphMetrics(const uint * glyph, Graphica::FontGlyphMetrics * metrics, uintptr length) noexcept override;
			virtual void GetGlyphsForCharacters(const unichar32 * chr, uint * glyph, uintptr length) noexcept override;
			void SimulateOblique(void) noexcept;
			uint GetCairoHeight(void) const noexcept;
			cairo_font_face_t GetCairoFont(void) const noexcept;
			double GetCairoFontScale(void) const noexcept;
		};
		class CairoDevice : public Graphica::IDeviceContext2D
		{
			oref<CairoAPI> _api;
			oref<Graphica::IDeviceContextFactory2D> _parent;
			oref<Graphica::IBitmap> _bitmap;
			cairo_t _context;
			Index2 _viewport;
			bool _state;
		private:
			void _perform_brush_setup(cairo_t ctx, Graphica::IBrush * brush, const Rectangle & at) noexcept;
			void _perform_brush_finalization(cairo_t ctx, Graphica::IBrush * brush) noexcept;
		public:
			CairoDevice(CairoAPI * api, Graphica::IDeviceContextFactory2D * parent, Graphica::IBitmap * dest);
			virtual ~CairoDevice(void) override;
			virtual string ToStringE(ErrorContext & ectx) const noexcept override;
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept override;
			virtual const void * GetType(void) noexcept override;
			virtual void GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) noexcept override;
			virtual uint32 GetImplementationFeatures(void) noexcept override;
			virtual oref<Graphica::IColorBrush> CreateSolidColorBrush(const Color & color) noexcept override;
			virtual oref<Graphica::IColorBrush> CreateGradientBrush(const Index2 & from, const Index2 & to, const Color * colors, const double * positions, uint count) noexcept override;
			virtual oref<Graphica::IBitmapBrush> CreateBitmapBrush(Graphica::IBitmap * bitmap, const Rectangle & area) noexcept override;
			virtual oref<Graphica::IBitmapBrush> CreateBitmapBrushCopy(Graphica::IBitmapBrush * bitmap, const Rectangle & area) noexcept override;
			virtual oref<Graphica::IBitmapBrush> CreateTileBrush(Graphica::IBitmap * bitmap, const Rectangle & area) noexcept override;
			virtual oref<Graphica::IBitmapBrush> CreateTileBrushCopy(Graphica::IBitmapBrush * bitmap, const Rectangle & area) noexcept override;
			virtual oref<Graphica::IBitmapBrush> CreateTextureBrush(Graphica::ITexture * texture, Graphica::TextureAlphaMode mode) noexcept override;
			virtual oref<Graphica::IBlurEffectBrush> CreateBlurEffectBrush(double sigma) noexcept override;
			virtual oref<Graphica::IInversionEffectBrush> CreateInversionEffectBrush(void) noexcept override;
			virtual oref<Graphica::ILayerBacking> CreateLayerBackingStorage(void) noexcept override;
			virtual oref<Graphica::IGlyphRun> CreateGlyphRun(Graphica::IFont ** fonts, const uint * glyphs, const double * px, const double * py, const Color * colors, uint count, const double * transform) noexcept override;
			virtual void PushClip(const Rectangle & rect) noexcept override;
			virtual void PopClip(void) noexcept override;
			virtual bool BeginLayerAlpha(Graphica::ILayerBacking * layer, const Rectangle & rect) noexcept override;
			virtual bool BeginLayer(Graphica::ILayerBacking * layer, const Rectangle & rect, double opacity) noexcept override;
			virtual void EndLayer(Graphica::ILayerBacking * layer) noexcept override;
			virtual void Render(Graphica::IBrush * brush, const Rectangle & at) noexcept override;
			virtual void RenderPolyline(const double * px, const double * py, uint count, bool closed, Graphica::IBrush * brush, double width) noexcept override;
			virtual void RenderPolygon(const double * px, const double * py, uint count, Graphica::IBrush * brush) noexcept override;
			virtual void RenderGlyphRun(Graphica::IGlyphRun * run, const Index2 & at) noexcept override;
			virtual bool BeginRendering(Graphica::TextureLoadAction load, const Color & clear_color) noexcept override;
			virtual bool EndRendering(void) noexcept override;
			cairo_t GetCairo(void) const noexcept;
			CairoAPI * GetAPI(void) const noexcept;
		};
	}
}