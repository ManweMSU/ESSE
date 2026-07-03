#include "Imprimitor.h"
#include <Graphica/Graphica.h>

namespace ESSE
{
	namespace Graphica
	{
		struct PDFSessionStructure : Object
		{
			oref<PDF::IEncoderContext> _ctx;
			uint _paper_width, _paper_height;
			uint _device_width, _device_height;
			uint _page_flags;
			bool _close_context;
		};
		class PDFDeviceContext : public Graphica::IDeviceContext2D
		{
			oref<PDFSessionStructure> _session;
			oref<IBitmap> _surface;
			oref<IDeviceContext2D> _bitmap_context;
		public:
			PDFDeviceContext(PDFSessionStructure * session) : _session(session)
			{
				auto factory = CreateDeviceContextFactory2D();
				_surface = factory->CreateBitmap(_session->_device_width, _session->_device_height, Color(0xFFFFFFFF));
				if (!_surface) throw OutOfMemoryException();
				_bitmap_context = factory->CreateBitmapContext(_surface);
				if (!_bitmap_context) throw OutOfMemoryException();
			}
			virtual ~PDFDeviceContext(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"PDF Device Context"; ESSE_TRY_OUTRO(string()) }
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept override
			{
				if (cls == Classes.Object || cls == Classes.DynamicObject || cls == Classes.IDeviceContext2D) {
					Retain(); return this;
				} else if (cls == Classes.IDeviceContextFactory2D) {
					return _bitmap_context->DynamicCast(Classes.IDeviceContextFactory2D, ectx);
				} else { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			}
			virtual const void * GetType(void) noexcept override { return Classes.IDeviceContext2D; }
			virtual void GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) noexcept override { _bitmap_context->GetImplementationInfo(tech, version_major, version_minor); }
			virtual uint32 GetImplementationFeatures(void) noexcept override
			{
				auto mask = _bitmap_context->GetImplementationFeatures();
				return mask & ~(DeviceContextPresentationContext | DeviceContextHasControllingDevice | DeviceContextBitmapContext);
			}
			virtual oref<IColorBrush> CreateSolidColorBrush(const Color & color) noexcept override { return _bitmap_context->CreateSolidColorBrush(color); }
			virtual oref<IColorBrush> CreateGradientBrush(const Index2 & from, const Index2 & to, const Color * colors, const double * positions, uint count) noexcept override { return _bitmap_context->CreateGradientBrush(from, to, colors, positions, count); }
			virtual oref<IBitmapBrush> CreateBitmapBrush(IBitmap * bitmap, const Rectangle & area) noexcept override { return _bitmap_context->CreateBitmapBrush(bitmap, area); }
			virtual oref<IBitmapBrush> CreateBitmapBrushCopy(IBitmapBrush * bitmap, const Rectangle & area) noexcept override { return _bitmap_context->CreateBitmapBrushCopy(bitmap, area); }
			virtual oref<IBitmapBrush> CreateTileBrush(IBitmap * bitmap, const Rectangle & area) noexcept override { return _bitmap_context->CreateTileBrush(bitmap, area); }
			virtual oref<IBitmapBrush> CreateTileBrushCopy(IBitmapBrush * bitmap, const Rectangle & area) noexcept override { return _bitmap_context->CreateTileBrushCopy(bitmap, area); }
			virtual oref<IBitmapBrush> CreateTextureBrush(ITexture * texture, TextureAlphaMode mode) noexcept override { return _bitmap_context->CreateTextureBrush(texture, mode); }
			virtual oref<IBlurEffectBrush> CreateBlurEffectBrush(double sigma) noexcept override { return _bitmap_context->CreateBlurEffectBrush(sigma); }
			virtual oref<IInversionEffectBrush> CreateInversionEffectBrush(void) noexcept override { return _bitmap_context->CreateInversionEffectBrush(); }
			virtual oref<ILayerBacking> CreateLayerBackingStorage(void) noexcept override { return _bitmap_context->CreateLayerBackingStorage(); }
			virtual oref<IGlyphRun> CreateGlyphRun(IFont ** fonts, const uint * glyphs, const double * px, const double * py, const Color * colors, uint count, const double * transform) noexcept override { return _bitmap_context->CreateGlyphRun(fonts, glyphs, px, py, colors, count, transform); }
			virtual void PushClip(const Rectangle & rect) noexcept override { _bitmap_context->PushClip(rect); }
			virtual void PopClip(void) noexcept override { _bitmap_context->PopClip(); }
			virtual bool BeginLayerAlpha(ILayerBacking * layer, const Rectangle & rect) noexcept override { return _bitmap_context->BeginLayerAlpha(layer, rect); }
			virtual bool BeginLayer(ILayerBacking * layer, const Rectangle & rect, double opacity) noexcept override { return _bitmap_context->BeginLayer(layer, rect, opacity); }
			virtual void EndLayer(ILayerBacking * layer) noexcept override { _bitmap_context->EndLayer(layer); }
			virtual void Render(IBrush * brush, const Rectangle & at) noexcept override { _bitmap_context->Render(brush, at); }
			virtual void RenderPolyline(const double * px, const double * py, uint count, bool closed, IBrush * brush, double width) noexcept override { _bitmap_context->RenderPolyline(px, py, count, closed, brush, width); }
			virtual void RenderPolygon(const double * px, const double * py, uint count, IBrush * brush) noexcept override { _bitmap_context->RenderPolygon(px, py, count, brush); }
			virtual void RenderGlyphRun(IGlyphRun * run, const Index2 & at) noexcept override { _bitmap_context->RenderGlyphRun(run, at); }
			virtual bool BeginRendering(TextureLoadAction load, const Color & clear_color) noexcept override
			{
				if (!_session->_ctx) return false;
				if (load == TextureLoadAction::Clear) return _bitmap_context->BeginRendering(TextureLoadAction::Clear, Color(clear_color.r, clear_color.g, clear_color.b));
				else return _bitmap_context->BeginRendering(TextureLoadAction::Clear, Color(0xFFFFFFFF));
			}
			virtual bool EndRendering(void) noexcept override
			{
				if (!_session->_ctx) return false;
				auto status = _bitmap_context->EndRendering();
				if (status) {
					auto frame = _surface->QueryContents();
					if (!_session->_ctx->AddPage(_session->_paper_width, _session->_paper_height, frame, _session->_page_flags)) return false;
				}
				return status;
			}
		};
		class PDFPrintingContext : public IPrintingContext
		{
			oref<PDFSessionStructure> _session;
			oref<PDFDeviceContext> _context;
		public:
			PDFPrintingContext(PDF::IEncoderContext * ctx, const PrinterModeDesc & mode, uint pdf_flags, bool close_ctx)
			{
				_session = owrap(new PDFSessionStructure);
				_session->_ctx = ctx;
				_session->_paper_width = mode.PaperWidth;
				_session->_paper_height = mode.PaperLength;
				if (mode.Orientation == PaperOrientation::Landscape) swap(_session->_paper_width, _session->_paper_height);
				_session->_device_width = _session->_paper_width * mode.DPI / 254;
				_session->_device_height = _session->_paper_height * mode.DPI / 254;
				_session->_page_flags = pdf_flags;
				_session->_close_context = close_ctx;
				_context = owrap(new PDFDeviceContext(_session));
			}
			virtual ~PDFPrintingContext(void) override { _session->_ctx.Clear(); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"PDF Printing Context"; ESSE_TRY_OUTRO(string()) }
			virtual bool FinalizeDocument(void) noexcept override
			{
				if (!_session->_ctx) return false;
				if (_session->_close_context) {
					_session->_close_context = false;
					_context->EndRendering();
					auto status = _session->_ctx->FinalizeDocument();
					_session->_ctx.Clear();
					return status;
				} else {
					_context->EndRendering();
					_session->_ctx.Clear();
					return true;
				}
			}
			virtual oref<IDeviceContext2D> GetDocumentDeviceContext(void) noexcept override { return _context.Inner(); }
			virtual Index2 GetEffectiveResolution(void) noexcept override { return Index2(_session->_device_width, _session->_device_height); }
		};

		oref<IPrinterFactory> CreatePrinterFactory(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreatePrinterFactory(ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<IPrintingContext> CreateContextPDF(PDF::IEncoderContext * ctx, const PrinterModeDesc & mode, uint pdf_flags, bool close_ctx, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			return CreateContextPDF(ctx, mode, pdf_flags, close_ctx);
			ESSE_TRY_OUTRO(0)
		}
		oref<IPrintingContext> CreateContextPDF(PDF::IEncoderContext * ctx, const PrinterModeDesc & mode, uint pdf_flags, bool close_ctx) { return oref<IPrintingContext>::CreateOwned(new PDFPrintingContext(ctx, mode, pdf_flags, close_ctx)); }
	}
}