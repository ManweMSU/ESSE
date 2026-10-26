#include "SystemPrinting.h"
#ifdef ENGINE_RUNTIME
#include <Imprimitio/Imprimitio.h>
#include "PlatformDependent/SystemGraphicsEx.h"

namespace Engine
{
	namespace Graphics
	{
		class SystemPrintingContext : public IPrintingContext
		{
			ESSE::oref<ESSE::Graphica::IPrintingContext> _pc;
			ESSE::oref<ESSE::Graphica::IDeviceContext2D> _dc;
			ESSE::oref<ESSE::Graphica::DeviceCache> _dc_cache;
			SafePointer<I2DDeviceContext> _public_context;
			bool _page;
		public:
			SystemPrintingContext(ESSE::Graphica::IPrinter * device, const string & task, const ESSE::Graphica::PrinterModeDesc & mode) : _page(false)
			{
				_pc = device->StartPrintingWithMode(static_cast<const widechar *>(task), mode);
				if (!_pc) throw InvalidStateException();
				_dc = _pc->GetDocumentDeviceContext();
				if (!_dc) throw InvalidStateException();
				_dc_cache = ESSE::owrap(new ESSE::Graphica::DeviceCache(_dc));
				_public_context = ESSEIO::WrapContext(_dc, _dc_cache);
				if (!_public_context) throw OutOfMemoryException();
			}
			virtual ~SystemPrintingContext(void) override { FinalizeDocument(); }
			virtual bool BeginPage(void) noexcept override
			{
				if (_page) return false;
				if (!_dc->BeginRendering(ESSE::Graphica::TextureLoadAction::Clear, ESSE::Color(0xFF, 0xFF, 0xFF))) return false;
				_page = true;
				_dc_cache->ResetCache();
				return true;
			}
			virtual bool EndPage(void) noexcept override
			{
				if (!_page) return false;
				_page = false;
				if (!_dc->EndRendering()) return false;
				return true;
			}
			virtual bool FinalizeDocument(void) noexcept override
			{
				bool status = true;
				if (_page) status = EndPage();
				return status && _pc->FinalizeDocument();
			}
			virtual I2DDeviceContext * GetPageDeviceContext(void) noexcept override { return _public_context; }
			virtual Point GetEffectiveResolution(void) noexcept override { auto res = _pc->GetEffectiveResolution(); return Point(res.x, res.y); }
			virtual string ToString(void) const override { return L"SystemPrintingContext"; }
		};
		class SystemPrinter : public IPrinter
		{
			ESSE::oref<ESSE::Graphica::IPrinter> _device;
			PrinterModeDesc _desc;

			static void _desc_convert(ESSE::Graphica::PrinterModeDesc & dest, const PrinterModeDesc & src) noexcept
			{
				if (src.Orientation == PaperOrientation::Portrait) dest.Orientation = ESSE::Graphica::PaperOrientation::Portrait;
				else dest.Orientation = ESSE::Graphica::PaperOrientation::Landscape;
				if (src.DuplexMode == PrinterDuplexMode::Duplex) dest.DuplexMode = ESSE::Graphica::PrinterDuplexMode::Duplex;
				else if (src.DuplexMode == PrinterDuplexMode::DuplexTumble) dest.DuplexMode = ESSE::Graphica::PrinterDuplexMode::DuplexTumble;
				else dest.DuplexMode = ESSE::Graphica::PrinterDuplexMode::Simplex;
				dest.PaperWidth = src.PaperWidth;
				dest.PaperLength = src.PaperLength;
				dest.DPI = src.DPI;
				dest.Copies = src.Copies;
				dest.Collate = src.Collate;
			}
			static void _desc_convert(PrinterModeDesc & dest, const ESSE::Graphica::PrinterModeDesc & src) noexcept
			{
				if (src.Orientation == ESSE::Graphica::PaperOrientation::Portrait) dest.Orientation = PaperOrientation::Portrait;
				else dest.Orientation = PaperOrientation::Landscape;
				if (src.DuplexMode == ESSE::Graphica::PrinterDuplexMode::Duplex) dest.DuplexMode = PrinterDuplexMode::Duplex;
				else if (src.DuplexMode == ESSE::Graphica::PrinterDuplexMode::DuplexTumble) dest.DuplexMode = PrinterDuplexMode::DuplexTumble;
				else dest.DuplexMode = PrinterDuplexMode::Simplex;
				dest.PaperWidth = src.PaperWidth;
				dest.PaperLength = src.PaperLength;
				dest.DPI = src.DPI;
				dest.Copies = src.Copies;
				dest.Collate = src.Collate;
				dest.ScaleFactor = 1.0;
			}
		public:
			SystemPrinter(ESSE::Graphica::IPrinterFactory * factory, const string & name)
			{
				_device = factory->OpenPrinter(static_cast<const widechar *>(name));
				if (!_device) throw InvalidStateException();
				ESSE::Graphica::PrinterModeDesc desc;
				_device->GetDefaultMode(desc);
				_desc_convert(_desc, desc);
			}
			virtual ~SystemPrinter(void) override {}
			virtual string GetName(void) override { return _device->GetName().GetData(); }
			virtual void GetDefaultMode(PrinterModeDesc & desc) noexcept override { desc = _desc; }
			virtual bool SetDefaultMode(const PrinterModeDesc & desc) noexcept override
			{
				if (!CheckMode(desc)) return false;
				_desc = desc;
				return true;
			}
			virtual bool CheckMode(const PrinterModeDesc & desc) noexcept override
			{
				if (desc.ScaleFactor != 1.0) return false;
				ESSE::Graphica::PrinterModeDesc mdesc;
				_desc_convert(mdesc, desc);
				return _device->CheckMode(mdesc);
			}
			virtual IPrintingContext * StartPrinting(const string & document_name) noexcept override
			{
				ESSE::Graphica::PrinterModeDesc mode;
				_desc_convert(mode, _desc);
				try { return new SystemPrintingContext(_device, document_name, mode); }
				catch (...) { return 0; }
			}
			virtual IPrintingContext * StartPrinting(const string & document_name, const PrinterModeDesc & desc) noexcept override
			{
				try {
					if (!CheckMode(desc)) return 0;
					ESSE::Graphica::PrinterModeDesc mode;
					_desc_convert(mode, desc);
					return new SystemPrintingContext(_device, document_name, mode);
				} catch (...) { return 0; }
			}
			virtual string ToString(void) const override { return L"SystemPrinter"; }
		};
		class SystemPrinterFactory : public IPrinterFactory
		{
			ESSE::oref<ESSE::Graphica::IPrinterFactory> _fact;
		public:
			SystemPrinterFactory(void) { _fact = ESSE::Graphica::CreatePrinterFactory(); }
			virtual ~SystemPrinterFactory(void) override {}
			virtual string GetDefaultPrinter(void) override { return _fact->GetDefaultPrinter().GetData(); }
			virtual Array<string> * EnumeratePrinters(void) noexcept override
			{
				try {
					SafePointer< Array<string> > result = new Array<string>(0x10);
					auto printers = _fact->EnumeratePrinters();
					for (auto & p : *printers) result->Append(p.GetData());
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual IPrinter * OpenPrinter(const string & name) noexcept override { try { return new SystemPrinter(_fact, name); } catch (...) { return 0; } }
			virtual string ToString(void) const override { return L"SystemPrinterFactory"; }
		};

		IPrinterFactory * CreatePrinterFactory(void) { try { return new SystemPrinterFactory; } catch (...) { return 0; } }
	}
}
#else
namespace Engine { namespace Graphics { IPrinterFactory * CreatePrinterFactory(void) { return 0; } } }
#endif