#pragma once

#include <Interfaces/SystemGraphics.h>

namespace Engine
{
	namespace Graphics
	{
		enum class PaperOrientation { Portrait, Landscape };
		enum class PrinterDuplexMode { Simplex, Duplex, DuplexTumble };
		struct PrinterModeDesc
		{
			PaperOrientation Orientation;
			uint PaperWidth, PaperLength; // 1/10 of millimeter
			double ScaleFactor;
			uint Copies, DPI;
			PrinterDuplexMode DuplexMode;
			bool Collate;
		};

		class IPrintingContext : public Object
		{
		public:
			virtual bool BeginPage(void) noexcept = 0;
			virtual bool EndPage(void) noexcept = 0;
			virtual bool FinalizeDocument(void) noexcept = 0;
			virtual I2DDeviceContext * GetPageDeviceContext(void) noexcept = 0;
			virtual Point GetEffectiveResolution(void) noexcept = 0;
		};
		class IPrinter : public Object
		{
		public:
			virtual string GetName(void) = 0;
			virtual void GetDefaultMode(PrinterModeDesc & desc) noexcept = 0;
			virtual bool SetDefaultMode(const PrinterModeDesc & desc) noexcept = 0;
			virtual bool CheckMode(const PrinterModeDesc & desc) noexcept = 0;
			virtual IPrintingContext * StartPrinting(const string & document_name) noexcept = 0;
			virtual IPrintingContext * StartPrinting(const string & document_name, const PrinterModeDesc & desc) noexcept = 0;
		};
		class IPrinterFactory : public Object
		{
		public:
			virtual string GetDefaultPrinter(void) = 0;
			virtual Array<string> * EnumeratePrinters(void) noexcept = 0;
			virtual IPrinter * OpenPrinter(const string & name) noexcept = 0;
		};

		IPrinterFactory * CreatePrinterFactory(void);
	}
}