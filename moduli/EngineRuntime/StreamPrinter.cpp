#include "StreamPrinter.h"

namespace Engine
{
	namespace Portable
	{
		class PDFPrintingContext : public Graphics::IPrintingContext
		{
			SafePointer<IEncoderContext> _ctx;
			SafePointer<Graphics::I2DDeviceContextFactory> _factory;
			SafePointer<Graphics::IBitmap> _surface;
			SafePointer<Graphics::IBitmapContext> _device;
			uint paper_width, paper_height;
			uint device_width, device_height;
			uint flags;
		public:
			PDFPrintingContext(IEncoderContext * ctx, const Graphics::PrinterModeDesc & desc, uint pdf_flags)
			{
				_factory = Graphics::CreateDeviceContextFactory();
				_device = _factory->CreateBitmapContext();
				_ctx.SetRetain(ctx);
				flags = pdf_flags;
				paper_width = desc.PaperWidth;
				paper_height = desc.PaperLength;
				if (desc.Orientation == Graphics::PaperOrientation::Landscape) swap(paper_width, paper_height);
				device_width = paper_width * desc.DPI / 254;
				device_height = paper_height * desc.DPI / 254;
			}
			virtual ~PDFPrintingContext(void) override {}
			virtual bool BeginPage(void) noexcept override
			{
				if (_surface) return false;
				_surface = _factory->CreateBitmap(device_width, device_height, Color(255, 255, 255));
				if (!_surface) return false;
				if (!_device->BeginRendering(_surface)) { _surface.SetReference(0); return false; }
				return true;
			}
			virtual bool EndPage(void) noexcept override
			{
				if (!_surface) return false;
				if (!_device->EndRendering()) return false;
				SafePointer<Codec::Frame> frame = _surface->QueryFrame();
				_surface.SetReference(0);
				if (frame) return _ctx->AddPage(paper_width, paper_height, frame, flags);
				else return false;
			}
			virtual bool FinalizeDocument(void) noexcept override
			{
				if (flags & StreamPrinterCloseContext) {
					flags &= ~StreamPrinterCloseContext;
					EndPage();
					return _ctx->FinalizeDocument();
				} else {
					if (_surface) {
						EndPage();
						return false;
					} else return true;
				}
			}
			virtual Graphics::I2DDeviceContext * GetPageDeviceContext(void) noexcept override { return _device; }
			virtual Point GetEffectiveResolution(void) noexcept override { return Point(device_width, device_height); }
			virtual string ToString(void) const override { return L"PDFPrintingContext"; }
		};
		
		Graphics::IPrintingContext * CreateStreamPrinter(IEncoderContext * ctx, const Graphics::PrinterModeDesc & desc, uint pdf_flags) { try { return new PDFPrintingContext(ctx, desc, pdf_flags); } catch (...) { return 0; } }
	}
}