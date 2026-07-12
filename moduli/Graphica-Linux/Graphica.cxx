#include <Graphica/Graphica.h>
#include <Cor/IO/CorWindows.h>
#include <Cor-Linux/CorLinuxClasses.h>
#include "DeviceCairo.h"
#include "DeviceWindowedCairo.h"
#include "Vulkan.h"

namespace ESSE
{
	namespace Linux
	{
		class ProxyDeviceContext2D : public Graphica::IDeviceContext2D
		{
			Windows::IWindow * _window;
			oref<Graphica::IDeviceContextFactory2D> _factory;
			oref<Graphica::IDevice> _device;
			oref<Graphica::IDeviceContext> _deferred;
			oref<Graphica::IDeviceContext2D> _inner;
			oref<Graphica::IPresentationLayer> _layer;
			Index2 _size;
		public:
			ProxyDeviceContext2D(Graphica::IDeviceContextFactory2D * parent, Windows::IWindow * window, Graphica::IDevice * device) : _window(window), _factory(parent), _device(device)
			{
				_deferred = _device->CreateDeferredDeviceContext();
				if (!_deferred) throw OutOfMemoryException();
				_inner = Vulkan::PrecreateContext2D(_deferred);
				if (!_inner) throw OutOfMemoryException();
				auto _size = window->GetClientSize();
				Graphica::PresentationLayerDesc desc;
				desc.Format = Graphica::PixelFormat::B8G8R8A8_unorm;
				desc.Usage = Graphica::ResourceUsageRenderTarget | Graphica::ResourceUsageShaderRead;
				desc.Width = _size.x;
				desc.Height = _size.y;
				_layer = _device->CreatePresentationLayer(window, desc);
				if (!_layer) throw Exception();
			}
			virtual ~ProxyDeviceContext2D(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { return _inner->ToStringE(ectx); }
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept override
			{
				if (cls == ESSE::Classes.Object || cls == ESSE::Classes.DynamicObject || cls == ESSE::Classes.IDeviceContext2D) {
					Retain(); return this;
				} else if (cls == ESSE::Classes.IDevice) {
					_device->Retain(); return _device;
				} else if (cls == ESSE::Classes.IDeviceContext) {
					auto immctx = _device->GetPrimaryDeviceContext();
					immctx->Retain(); return immctx;
				} else if (cls == ESSE::Classes.IDeviceContextFactory2D) {
					_factory->Retain(); return _factory;
				} else { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			}
			virtual const void * GetType(void) noexcept override { return ESSE::Classes.IDeviceContext2D; }
			virtual void GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) noexcept override { _inner->GetImplementationInfo(tech, version_major, version_minor); }
			virtual uint32 GetImplementationFeatures(void) noexcept override { return _inner->GetImplementationFeatures() | Graphica::DeviceContextPresentationContext; }
			virtual oref<Graphica::IColorBrush> CreateSolidColorBrush(const Color & color) noexcept override { return _inner->CreateSolidColorBrush(color); }
			virtual oref<Graphica::IColorBrush> CreateGradientBrush(const Index2 & from, const Index2 & to, const Color * colors, const double * positions, uint count) noexcept override { return _inner->CreateGradientBrush(from, to, colors, positions, count); }
			virtual oref<Graphica::IBitmapBrush> CreateBitmapBrush(Graphica::IBitmap * bitmap, const Rectangle & area) noexcept override { return _inner->CreateBitmapBrush(bitmap, area); }
			virtual oref<Graphica::IBitmapBrush> CreateBitmapBrushCopy(Graphica::IBitmapBrush * bitmap, const Rectangle & area) noexcept override { return _inner->CreateBitmapBrushCopy(bitmap, area); }
			virtual oref<Graphica::IBitmapBrush> CreateTileBrush(Graphica::IBitmap * bitmap, const Rectangle & area) noexcept override { return _inner->CreateTileBrush(bitmap, area); }
			virtual oref<Graphica::IBitmapBrush> CreateTileBrushCopy(Graphica::IBitmapBrush * bitmap, const Rectangle & area) noexcept override { return _inner->CreateTileBrushCopy(bitmap, area); }
			virtual oref<Graphica::IBitmapBrush> CreateTextureBrush(Graphica::ITexture * texture, Graphica::TextureAlphaMode mode) noexcept override { return _inner->CreateTextureBrush(texture, mode); }
			virtual oref<Graphica::IBlurEffectBrush> CreateBlurEffectBrush(double sigma) noexcept override { return _inner->CreateBlurEffectBrush(sigma); }
			virtual oref<Graphica::IInversionEffectBrush> CreateInversionEffectBrush(void) noexcept override { return _inner->CreateInversionEffectBrush(); }
			virtual oref<Graphica::ILayerBacking> CreateLayerBackingStorage(void) noexcept override { return _inner->CreateLayerBackingStorage(); }
			virtual oref<Graphica::IGlyphRun> CreateGlyphRun(Graphica::IFont ** fonts, const uint * glyphs, const double * px, const double * py, const Color * colors, uint count, const double * transform) noexcept override { return _inner->CreateGlyphRun(fonts, glyphs, px, py, colors, count, transform); }
			virtual void PushClip(const Rectangle & rect) noexcept override { _inner->PushClip(rect); }
			virtual void PopClip(void) noexcept override { _inner->PopClip(); }
			virtual bool BeginLayerAlpha(Graphica::ILayerBacking * layer, const Rectangle & rect) noexcept override { return _inner->BeginLayerAlpha(layer, rect); }
			virtual bool BeginLayer(Graphica::ILayerBacking * layer, const Rectangle & rect, double opacity) noexcept override { return _inner->BeginLayer(layer, rect, opacity); }
			virtual void EndLayer(Graphica::ILayerBacking * layer) noexcept override { _inner->EndLayer(layer); }
			virtual void Render(Graphica::IBrush * brush, const Rectangle & at) noexcept override { _inner->Render(brush, at); }
			virtual void RenderPolyline(const double * px, const double * py, uint count, bool closed, Graphica::IBrush * brush, double width) noexcept override { _inner->RenderPolyline(px, py, count, closed, brush, width); }
			virtual void RenderPolygon(const double * px, const double * py, uint count, Graphica::IBrush * brush) noexcept override { _inner->RenderPolygon(px, py, count, brush); }
			virtual void RenderGlyphRun(Graphica::IGlyphRun * run, const Index2 & at) noexcept override { _inner->RenderGlyphRun(run, at); }
			virtual bool BeginRendering(Graphica::TextureLoadAction load, const Color & clear_color) noexcept override
			{
				if (!_window) return false;
				auto size = _window->GetClientSize();
				if (_size != size) {
					if (!_layer->ResizeSurface(size.x, size.y)) return false;
					_size = size;
				}
				auto surface = _layer->QuerySurface();
				if (!surface) return false;
				Graphica::RenderTargetViewDesc rtvd;
				rtvd.Texture = surface;
				rtvd.LoadAction = load;
				rtvd.ClearValue[3] = float(clear_color.a) / 255.0f;
				rtvd.ClearValue[0] = float(clear_color.r) * rtvd.ClearValue[3] / 255.0f;
				rtvd.ClearValue[1] = float(clear_color.g) * rtvd.ClearValue[3] / 255.0f;
				rtvd.ClearValue[2] = float(clear_color.b) * rtvd.ClearValue[3] / 255.0f;
				return _deferred->BeginRenderingPass2D(rtvd);
			}
			virtual bool EndRendering(void) noexcept override
			{
				if (!_deferred->EndCurrentPass()) return false;
				_device->GetPrimaryDeviceContext()->Flush();
				if (!_device->GetPrimaryDeviceContext()->SubmitDeferredContext(_deferred)) return false;
				return _layer->Present();
			}
			void Invalidate(void) noexcept { _window = 0; }
		};
		class ProxyContextClass : public Windows::IWindowExtensionClass
		{
		public:
			virtual bool ExtensionAttached(Windows::IWindow * window, Object * extension) noexcept override { return true; }
			virtual void ExtensionDetached(Windows::IWindow * window, Object * extension) noexcept override { static_cast<ProxyDeviceContext2D *>(extension)->Invalidate(); }
		};
		#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
		class X11CairoContextClass : public Windows::IWindowExtensionClass
		{
		public:
			virtual bool ExtensionAttached(Windows::IWindow * window, Object * extension) noexcept override { return true; }
			virtual void ExtensionDetached(Windows::IWindow * window, Object * extension) noexcept override { static_cast<Cairo::CairoDeviceX11 *>(extension)->Invalidate(); }
		};
		#endif
		class LinuxDeviceContextFactory2D : public Graphica::IDeviceContextFactory2D
		{
			Cairo::FT_Library _font_library;
			oref<Cairo::CairoAPI> _cairo_api;
			oref<Cairo::FontConfigAPI> _fc_api;
			oref<Cairo::FreeTypeAPI> _ft_api;
			oref<ProxyContextClass> _proxy_class;
			#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
			oref<X11CairoContextClass> _x11_cairo_class;
			#endif
		public:
			LinuxDeviceContextFactory2D(void) : _cairo_api(owrap(new Cairo::CairoAPI))
			{
				try { _fc_api = owrap(new Cairo::FontConfigAPI); } catch (...) {}
				try { _ft_api = owrap(new Cairo::FreeTypeAPI); } catch (...) {}
				if (_fc_api && !_fc_api->FcInit()) throw OutOfMemoryException();
				if (_ft_api && _ft_api->FT_Init_FreeType(&_font_library)) {
					if (_fc_api) _fc_api->FcFini();
					throw OutOfMemoryException();
				}
			}
			virtual ~LinuxDeviceContextFactory2D(void) override { if (_ft_api) _ft_api->FT_Done_FreeType(_font_library); if (_fc_api) _fc_api->FcFini(); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Linux 2D device context factory"; ESSE_TRY_OUTRO(string()) }
			virtual oref<Graphica::IBitmap> CreateBitmap(uint width, uint height, const Color & clear_color) noexcept override { try { return oref<Graphica::IBitmap>::CreateOwned(new Cairo::CairoBitmap(_cairo_api, width, height, clear_color)); } catch (...) { return 0; } }
			virtual oref<Graphica::IBitmap> LoadBitmap(Picturae::Picture * source) noexcept override { try { return oref<Graphica::IBitmap>::CreateOwned(new Cairo::CairoBitmap(_cairo_api, source)); } catch (...) { return 0; } }
			virtual oref<Graphica::IDeviceContext2D> CreateBitmapContext(Graphica::IBitmap * bitmap) noexcept override { try { return oref<Graphica::IDeviceContext2D>::CreateOwned(new Cairo::CairoDevice(_cairo_api, this, bitmap)); } catch (...) { return 0; } }
			virtual oref<Graphica::IDeviceContext2D> CreatePresentationContext(DynamicObject * presentor, Graphica::IDevice * device) noexcept override
			{
				if (!presentor) return 0;
				ErrorContext ectx; ErrorClear(ectx);
				auto window = owrap(reinterpret_cast<Windows::IWindow *>(presentor->DynamicCast(ESSE::Classes.IWindow, ectx)));
				if (ErrorTest(ectx)) return 0;
				if (_proxy_class) window->RemoveExtension(_proxy_class);
				#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
				if (_x11_cairo_class) window->RemoveExtension(_x11_cairo_class);
				#endif
				if (device) {
					try {
						if (!_proxy_class) _proxy_class = owrap(new ProxyContextClass);
						auto context = oref<Graphica::IDeviceContext2D>::CreateOwned(new ProxyDeviceContext2D(this, window, device));
						if (!window->AddExtension(context, _proxy_class)) throw InvalidStateException();
						return context;
					} catch (...) { return 0; }
				} else {
					// TODO: IMPLEMENT WITH CAIRO-WAYLAND
					#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
					ErrorClear(ectx);
					auto wx11 = reinterpret_cast<X11::IX11Window *>(window->DynamicCast(Classes::X11_Window, ectx));
					if (!ErrorTest(ectx)) try {
						if (!_x11_cairo_class) _x11_cairo_class = owrap(new X11CairoContextClass);
						auto context = oref<Graphica::IDeviceContext2D>::CreateOwned(new Cairo::CairoDeviceX11(_cairo_api, this, window, wx11));
						if (!window->AddExtension(context, _x11_cairo_class)) throw InvalidStateException();
						return context;
					} catch (...) { return 0; }
					#endif
					return 0;
				}
			}
			virtual oref<Graphica::IFont> CreateFont(const string & font_face, uint style, uint height, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!_fc_api || !_ft_api) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
					auto config = _fc_api->FcInitLoadConfigAndFonts();
					if (!config) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
					auto font_set = _fc_api->FcConfigGetFonts(config, 0);
					if (!font_set) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
					auto pattern = _fc_api->FcPatternCreate();
					if (!pattern) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
					try {
						if (style & Graphica::CreateFontSystemDefault) {
							ucs1_string fname;
							if (style & Graphica::CreateFontMonospace) fname = "monospace";
							else if (style & Graphica::CreateFontSansSerif) fname = "sans-serif";
							else fname = "serif";
							if (!_fc_api->FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const Cairo::FcChar8 *>(fname.GetData()))) throw OutOfMemoryException();
						} else {
							ucs1_string fname = font_face;
							if (!_fc_api->FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const Cairo::FcChar8 *>(fname.GetData()))) throw OutOfMemoryException();
						}
						int slant, weight;
						if (style & Graphica::CreateFontItalic) slant = FC_SLANT_ITALIC;
						else if (style & Graphica::CreateFontOblique) slant = FC_SLANT_OBLIQUE;
						else slant = FC_SLANT_ROMAN;
						if (!_fc_api->FcPatternAddDouble(pattern, FC_PIXEL_SIZE, double(height))) throw OutOfMemoryException();
						if (!_fc_api->FcPatternAddInteger(pattern, FC_SLANT, slant)) throw OutOfMemoryException();
						if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight100) weight = FC_WEIGHT_ULTRALIGHT;
						else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight200) weight = FC_WEIGHT_LIGHT;
						else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight300) weight = FC_WEIGHT_SEMILIGHT;
						else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight400) weight = FC_WEIGHT_REGULAR;
						else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight500) weight = FC_WEIGHT_MEDIUM;
						else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight600) weight = FC_WEIGHT_SEMIBOLD;
						else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight700) weight = FC_WEIGHT_BOLD;
						else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight800) weight = FC_WEIGHT_ULTRABOLD;
						else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight900) weight = FC_WEIGHT_BLACK;
						else weight = FC_WEIGHT_REGULAR;
						if (!_fc_api->FcPatternAddInteger(pattern, FC_WEIGHT, weight)) throw OutOfMemoryException();
						if (!_fc_api->FcConfigSubstitute(config, pattern, FcMatchPattern)) throw OutOfMemoryException();
						_fc_api->FcDefaultSubstitute(pattern);
					} catch (...) { _fc_api->FcPatternDestroy(pattern); throw; }
					Cairo::FcResult status;
					auto match = _fc_api->FcFontSetMatch(config, &font_set, 1, pattern, &status);
					_fc_api->FcPatternDestroy(pattern);
					if (!match) { ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::FileNotFound); return 0; }
					Cairo::FcChar8 * font_path; int font_index;
					if (_fc_api->FcPatternGetString(match, FC_FILE, 0, &font_path) != FcResultMatch) { _fc_api->FcPatternDestroy(match); return 0; }
					if (_fc_api->FcPatternGetInteger(match, FC_INDEX, 0, &font_index) != FcResultMatch) { _fc_api->FcPatternDestroy(match); return 0; }
					Cairo::FT_Face ff;
					if (_ft_api->FT_New_Face(_font_library, reinterpret_cast<const char *>(font_path), font_index, &ff)) {
						_fc_api->FcPatternDestroy(match);
						ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::Unknown);
						return 0;
					}
					_fc_api->FcPatternDestroy(match);
					oref<Graphica::IFont> result;
					try { result = oref<Graphica::IFont>::CreateOwned(new Cairo::CairoFont(this, _cairo_api, _ft_api, ff, height)); }
					catch (...) { _ft_api->FT_Done_Face(ff); throw; }
					if ((style & Graphica::CreateFontItalic) && !(ff->style_flags & FT_STYLE_FLAG_ITALIC)) static_cast<Cairo::CairoFont *>(result.Inner())->SimulateOblique();
					if ((style & Graphica::CreateFontOblique) && !(style & Graphica::CreateFontItalic)) static_cast<Cairo::CairoFont *>(result.Inner())->SimulateOblique();
					return result;
				ESSE_TRY_OUTRO(0)
			}
			virtual oref<Graphica::IFont> LoadFont(Stream * stream, uint height, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!_ft_api) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
					if (!stream) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
					auto data = stream->ReadAll();
					Cairo::FT_Face ff;
					if (_ft_api->FT_New_Memory_Face(_font_library, data->GetBuffer(), data->GetLength(), 0, &ff)) {
						ErrorSet(ectx, Errores::ErrorInvalidFormat);
						return 0;
					}
					oref<Graphica::IFont> result;
					try { result = oref<Graphica::IFont>::CreateOwned(new Cairo::CairoFont(this, _cairo_api, _ft_api, ff, height)); }
					catch (...) { _ft_api->FT_Done_Face(ff); throw; }
					return result;

				ESSE_TRY_OUTRO(0)
			}
			virtual oref<Graphica::IFont> SearchFont(Graphica::IFont * base_font, const unichar32 * chars, uint count, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!_fc_api || !_ft_api) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
					if (!base_font || !count || !chars) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
					auto style = base_font->GetFontStyle();
					int ref_slant, ref_weight;
					if (style & Graphica::CreateFontItalic) ref_slant = FC_SLANT_ITALIC;
					else if (style & Graphica::CreateFontOblique) ref_slant = FC_SLANT_OBLIQUE;
					else ref_slant = FC_SLANT_ROMAN;
					if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight100) ref_weight = FC_WEIGHT_ULTRALIGHT;
					else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight200) ref_weight = FC_WEIGHT_LIGHT;
					else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight300) ref_weight = FC_WEIGHT_SEMILIGHT;
					else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight400) ref_weight = FC_WEIGHT_REGULAR;
					else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight500) ref_weight = FC_WEIGHT_MEDIUM;
					else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight600) ref_weight = FC_WEIGHT_SEMIBOLD;
					else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight700) ref_weight = FC_WEIGHT_BOLD;
					else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight800) ref_weight = FC_WEIGHT_ULTRABOLD;
					else if ((style & Graphica::CreateFontWeightMask) == Graphica::CreateFontWeight900) ref_weight = FC_WEIGHT_BLACK;
					else ref_weight = FC_WEIGHT_REGULAR;
					auto config = _fc_api->FcInitLoadConfigAndFonts();
					if (!config) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
					auto charset = _fc_api->FcCharSetCreate();
					if (!charset) throw OutOfMemoryException();
					for (uint i = 0; i < count; i++) if (!_fc_api->FcCharSetAddChar(charset, chars[i])) { _fc_api->FcCharSetDestroy(charset); throw OutOfMemoryException(); }
					auto pattern = _fc_api->FcPatternCreate();
					if (!pattern) { _fc_api->FcCharSetDestroy(charset); throw OutOfMemoryException(); }
					if (!_fc_api->FcPatternAddCharSet(pattern, FC_CHARSET, charset)) { _fc_api->FcCharSetDestroy(charset); _fc_api->FcPatternDestroy(pattern); throw OutOfMemoryException(); }
					_fc_api->FcCharSetDestroy(charset);
					auto oset = _fc_api->FcObjectSetBuild(FC_SLANT, FC_WEIGHT, FC_FILE, FC_INDEX, 0);
					if (!oset) { _fc_api->FcPatternDestroy(pattern); throw OutOfMemoryException(); }
					auto list = _fc_api->FcFontList(config, pattern, oset);
					_fc_api->FcPatternDestroy(pattern);
					_fc_api->FcObjectSetDestroy(oset);
					if (!list) { _fc_api->FcFontSetDestroy(list); throw OutOfMemoryException(); }
					if (!list->nfont) { _fc_api->FcFontSetDestroy(list); throw InputOutputException(Errores::SuberrorIO::FileNotFound); }
					int match = -1;
					bool prev_slant_match = false;
					bool prev_weight_match = false;
					for (int i = 0; i < list->nfont; i++) {
						auto fp = list->fonts[i];
						int slant, weight;
						_fc_api->FcPatternGetInteger(fp, FC_SLANT, 0, &slant);
						_fc_api->FcPatternGetInteger(fp, FC_WEIGHT, 0, &weight);
						bool slant_match = slant == ref_slant;
						bool weight_match = weight == ref_weight;
						if (!prev_slant_match && slant_match) {
							match = i;
							prev_slant_match = slant_match;
							prev_weight_match = weight_match;
						} else if (!prev_weight_match && weight_match && prev_slant_match == slant_match) {
							match = i;
							prev_slant_match = slant_match;
							prev_weight_match = weight_match;
						} else if (match < 0) {
							match = i;
							prev_slant_match = slant_match;
							prev_weight_match = weight_match;
						}
					}
					Cairo::FcChar8 * font_path; int font_index;
					if (_fc_api->FcPatternGetString(list->fonts[match], FC_FILE, 0, &font_path) != FcResultMatch) { _fc_api->FcFontSetDestroy(list); return 0; }
					if (_fc_api->FcPatternGetInteger(list->fonts[match], FC_INDEX, 0, &font_index) != FcResultMatch) { _fc_api->FcFontSetDestroy(list); return 0; }
					Cairo::FT_Face ff;
					if (_ft_api->FT_New_Face(_font_library, reinterpret_cast<const char *>(font_path), font_index, &ff)) {
						_fc_api->FcFontSetDestroy(list);
						ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::Unknown);
						return 0;
					}
					_fc_api->FcFontSetDestroy(list);
					oref<Graphica::IFont> result;
					try { result = oref<Graphica::IFont>::CreateOwned(new Cairo::CairoFont(this, _cairo_api, _ft_api, ff, base_font->GetHeight())); }
					catch (...) { _ft_api->FT_Done_Face(ff); throw; }
					if ((style & Graphica::CreateFontItalic) && !(ff->style_flags & FT_STYLE_FLAG_ITALIC)) static_cast<Cairo::CairoFont *>(result.Inner())->SimulateOblique();
					if ((style & Graphica::CreateFontOblique) && !(style & Graphica::CreateFontItalic)) static_cast<Cairo::CairoFont *>(result.Inner())->SimulateOblique();
					return result;
				ESSE_TRY_OUTRO(0)
			}
			virtual oref<array<string>> EnumerateFontFamilies(void) noexcept override
			{
				if (!_fc_api) return 0;
				try {
					auto result = owrap(new array<string>(0x100));
					Set<string> unique;
					auto config = _fc_api->FcInitLoadConfigAndFonts();
					if (!config) return 0;
					auto set = _fc_api->FcConfigGetFonts(config, 0);
					if (!set) return 0;
					for (int i = 0; i < set->nfont; i++) {
						if (!set->fonts[i]) continue;
						Cairo::FcChar8 * fname;
						if (_fc_api->FcPatternGetString(set->fonts[i], FC_FAMILY, 0, &fname) == FcResultMatch) unique.AddElement(string(reinterpret_cast<const unichar8 *>(fname)));
					}
					for (auto & n : unique) result->Append(n);
					return result;
				} catch (...) { return 0; }
			}
		};
	}
	namespace Graphica
	{
		oref<IDeviceContextFactory2D> _common_factory_2d;
		oref<IDeviceFactory> CreateDeviceFactory(ErrorContext & ectx) noexcept { return Vulkan::CreateDeviceFactory(ectx); }
		oref<IDeviceContextFactory2D> CreateDeviceContextFactory2D(ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				Memory::AcquireRootLock();
				auto result = _common_factory_2d;
				if (!result) try { result = _common_factory_2d = oref<IDeviceContextFactory2D>::CreateOwned(new Linux::LinuxDeviceContextFactory2D); }
				catch (...) { Memory::ReleaseRootLock(); throw; }
				Memory::ReleaseRootLock();
				return result;
			ESSE_TRY_OUTRO(0)
		}
	}
}