#pragma once

#include <Cor/Images/CorGraphics.h>
#include "ImprimitorPDF.h"

namespace ESSE
{
	namespace Graphica
	{
		enum class PaperOrientation : uint { Portrait = 0, Landscape = 1 };
		enum class PrinterDuplexMode : uint { Simplex = 0, Duplex = 1, DuplexTumble = 2 };
		struct PrinterModeDesc
		{
			PaperOrientation Orientation;
			PrinterDuplexMode DuplexMode;
			uint PaperWidth, PaperLength; // In 1/10th of millimeter
			uint Copies, DPI;
			bool Collate;
		};

		class IPrintingContext : public Object
		{
		public:
			virtual bool FinalizeDocument(void) noexcept = 0;
			virtual oref<IDeviceContext2D> GetDocumentDeviceContext(void) noexcept = 0;
			virtual Index2 GetEffectiveResolution(void) noexcept = 0;
		};
		class IPrinter : public Object
		{
		public:
			virtual const string & GetName(void) noexcept = 0;
			virtual oref<array<PaperOrientation>> EnumerateOrientations(void) noexcept = 0;
			virtual oref<array<PrinterDuplexMode>> EnumerateDuplexModes(void) noexcept = 0;
			virtual oref<array<Index2>> EnumeratePaperFormats(void) noexcept = 0;
			virtual bool CanCollate(void) noexcept = 0;
			virtual void GetDefaultMode(PrinterModeDesc & desc) noexcept = 0;
			virtual bool SetDefaultMode(const PrinterModeDesc & desc) noexcept = 0;
			virtual bool CheckMode(const PrinterModeDesc & desc) noexcept = 0;
			virtual oref<IPrintingContext> StartPrinting(const string & document_name) noexcept = 0;
			virtual oref<IPrintingContext> StartPrintingWithMode(const string & document_name, const PrinterModeDesc & desc) noexcept = 0;
		};
		class IPrinterFactory : public Object
		{
		public:
			virtual const string & GetDefaultPrinter(void) noexcept = 0;
			virtual oref<array<string>> EnumeratePrinters(void) noexcept = 0;
			virtual oref<IPrinter> OpenPrinter(const string & name) noexcept = 0;
		};

		oref<IPrinterFactory> CreatePrinterFactory(ErrorContext & ectx) noexcept;
		oref<IPrinterFactory> CreatePrinterFactory(void);
		oref<IPrintingContext> CreateContextPDF(PDF::IEncoderContext * ctx, const PrinterModeDesc & mode, uint pdf_flags, bool close_ctx, ErrorContext & ectx) noexcept;
		oref<IPrintingContext> CreateContextPDF(PDF::IEncoderContext * ctx, const PrinterModeDesc & mode, uint pdf_flags, bool close_ctx);
	}
}