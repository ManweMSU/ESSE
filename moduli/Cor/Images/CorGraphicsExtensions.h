#pragma once

#include "CorGraphics.h"

namespace ESSE
{
	namespace Graphica
	{
		namespace TypesetterFlags
		{
			constexpr uint HorizontalAlignmentLeft		= 0x000;
			constexpr uint HorizontalAlignmentCenter	= 0x001;
			constexpr uint HorizontalAlignmentRight		= 0x002;
			constexpr uint HorizontalAlignmentFill		= 0x003;
			constexpr uint VerticalAlignmentTop			= 0x000;
			constexpr uint VerticalAlignmentCenter		= 0x004;
			constexpr uint VerticalAlignmentBottom		= 0x008;
			constexpr uint EnableWordWrap				= 0x010;
			constexpr uint EnableEllipsis				= 0x020;
			constexpr uint EnableClipping				= 0x040;
			constexpr uint Underlined					= 0x100;
			constexpr uint Strikedout					= 0x200;
		}
		class AggregateFont : public Object
		{
			oref<IDeviceContextFactory2D> _factory;
			oref<IFont> _primary;
			object_array<IFont> _surrogate;
		public:
			AggregateFont(IDeviceContextFactory2D * factory, IFont * primary) noexcept;
			AggregateFont(IDeviceContextFactory2D * factory, const string & font_face, uint style, uint height);
			AggregateFont(IDeviceContextFactory2D * factory, Stream * stream, uint height);
			virtual ~AggregateFont(void) override;
			virtual string ToStringE(ErrorContext & ectx) const noexcept override;
			string GetFontFace(void) noexcept;
			uint GetFontStyle(void) noexcept;
			uint GetHeight(void) noexcept;
			void GetFontMetrics(FontMetrics & metrics) noexcept;
			void GetGlyphsForCharacters(const unichar32 * chr, IFont ** font, uint * glyph, uintptr length) noexcept;
			IDeviceContextFactory2D * GetParentFactory(void) noexcept;
			IFont * GetPrimaryFont(void) noexcept;
		};
		class DeviceCache : public Object
		{
			oref<IDeviceContext2D> _context;
			ObjectDictionary<Color, IColorBrush> _color_brushes;
			ObjectDictionary<oref<IBitmap>, Set<oref<IBitmapBrush>>> _device_bitmaps;
			ObjectDictionary<double, IBlurEffectBrush> _blur_brushes;
			oref<IInversionEffectBrush> _inversion_brush;
			uint _allocation_counter;
		public:
			DeviceCache(IDeviceContext2D * context);
			virtual ~DeviceCache(void) override;
			virtual string ToStringE(ErrorContext & ectx) const noexcept override;
			void ResetCache(void) noexcept;
			void ResetBitmapCache(IBitmap * bitmap) noexcept;
			oref<IColorBrush> CreateSolidColorBrush(const Color & color) noexcept;
			oref<IBitmapBrush> CreateBitmapBrush(IBitmap * bitmap, const Rectangle & area) noexcept;
			oref<IBitmapBrush> CreateTileBrush(IBitmap * bitmap, const Rectangle & area) noexcept;
			oref<IBlurEffectBrush> CreateBlurEffectBrush(double sigma) noexcept;
			oref<IInversionEffectBrush> CreateInversionEffectBrush(void) noexcept;
		};
		class Typesetter : public Object
		{
			// TODO: IMPLEMENT FIELDS
		public:
			Typesetter(IFont * font, const string & text, uint flags, const Color & color);
			Typesetter(AggregateFont * font, const string & text, uint flags, const Color & color);
			virtual ~Typesetter(void) override;
			virtual string ToStringE(ErrorContext & ectx) const noexcept override;
			// Rendering and general control
			void Render(IDeviceContext2D * context, DeviceCache * cache, const Rectangle & at) noexcept;
			void ResetCache(void) noexcept;
			void ResetAttribution(void) noexcept;
			// Statistical requests
			uint GetTextLength(void) noexcept;
			const unichar32 * GetText(void) noexcept;
			Index2 GetExtents(void) noexcept;
			Index2 GetExtents(uint for_width) noexcept;
			void GetGlyphRectangles(uint from, uint count, double * pleft, double * pright, double * ptop, double * pbottom) noexcept;
			// Setting the rendering properties
			void SetTabStops(const double * stops, uint num_stops, double generic_stop) noexcept;
			void SetLineSpacingFactor(double factor) noexcept;
			void SetColors(const Color * colors, uint from, uint num_clr) noexcept;
			void SetAdvances(const double * advances, uint from, uint num_adv) noexcept;
			void AddAttributeRange(uint from, uint to, IFont * font_override, uint flags_override, uint flag_mask) noexcept;
			void AddAttributeRange(uint from, uint to, AggregateFont * font_override, uint flags_override, uint flag_mask) noexcept;
		};
	}
}