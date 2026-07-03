#include <Imprimitio/Imprimitor.h>
#include "CUPSDL.h"

namespace ESSE
{
	namespace Graphica
	{
		class CUPSPrintingContext : public IPrintingContext
		{
			oref<CUPS::CUPSAPI> _api;
			oref<IPrinter> _printer;
			CUPS::http_t _http;
			CUPS::cups_dest_t * _dest;
			CUPS::cups_dinfo_t _info;
			int _job;
			bool _finished;
			ucs1_string _name;
			oref<MemoryStream> _pdf_stream;
			oref<IPrintingContext> _pdf_context;
		private:
			void _add_option(int * optc, CUPS::cups_option_t ** optv, const char * name, const char * value)
			{
				auto status = _api->cupsAddOption(name, value, *optc, optv);
				if (status == *optc) throw OutOfMemoryException();
				*optc = status;
			}
		public:
			CUPSPrintingContext(CUPS::CUPSAPI * api, IPrinter * printer, CUPS::http_t http, CUPS::cups_dest_t * dest, CUPS::cups_dinfo_t info, const ucs1_string & name, const PrinterModeDesc & mode) : _api(api), _printer(printer), _http(http), _dest(dest), _info(info), _finished(false), _name(name)
			{
				_pdf_stream = MemoryStream::Create(0x100000);
				_pdf_context = CreateContextPDF(PDF::CreateEncoder(_pdf_stream), mode, PDF::PageFlagColorFull | PDF::PageFlagCompress, true);
				int option_count = 0;
				CUPS::cups_option_t * options = 0;
				try {
					CUPS::cups_size_t selected;
					Memory::ZeroMemory(&selected, sizeof(selected));
					auto num_formats = _api->cupsGetDestMediaCount(_http, _dest, _info, 0);
					for (int i = 0; i < num_formats; i++) {
						CUPS::cups_size_t size;
						if (_api->cupsGetDestMediaByIndex(_http, _dest, _info, i, 0, &size)) {
							if (size.width / 10 == mode.PaperWidth && size.length / 10 == mode.PaperLength) { selected = size; break; }
						}
					}
					if (!selected.width || !selected.length) throw InvalidArgumentException();
					_add_option(&option_count, &options, CUPS_MEDIA, selected.media);
					if (mode.Orientation == PaperOrientation::Portrait) _add_option(&option_count, &options, CUPS_ORIENTATION, CUPS_ORIENTATION_PORTRAIT);
					else if (mode.Orientation == PaperOrientation::Landscape) _add_option(&option_count, &options, CUPS_ORIENTATION, CUPS_ORIENTATION_LANDSCAPE);
					else throw InvalidArgumentException();
					if (mode.DuplexMode == PrinterDuplexMode::Simplex) _add_option(&option_count, &options, CUPS_SIDES, CUPS_SIDES_ONE_SIDED);
					else if (mode.DuplexMode == PrinterDuplexMode::Duplex) _add_option(&option_count, &options, CUPS_SIDES, CUPS_SIDES_TWO_SIDED_PORTRAIT);
					else if (mode.DuplexMode == PrinterDuplexMode::DuplexTumble) _add_option(&option_count, &options, CUPS_SIDES, CUPS_SIDES_TWO_SIDED_LANDSCAPE);
					else throw InvalidArgumentException();
					if (mode.Collate) _add_option(&option_count, &options, "collate", "true");
					_add_option(&option_count, &options, CUPS_COPIES, ucs1_string(string(mode.Copies)));
				} catch (...) {
					_api->cupsFreeOptions(option_count, options);
					throw;
				}
				auto status = _api->cupsCreateDestJob(_http, _dest, _info, &_job, _name, option_count, options);
				_api->cupsFreeOptions(option_count, options);
				if (status != IPP_STATUS_OK) throw InvalidStateException();
			}
			virtual ~CUPSPrintingContext(void) override { if (!_finished) _api->cupsCancelDestJob(_http, _dest, _job); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"CUPS Printing Context"; ESSE_TRY_OUTRO(string()) }
			virtual bool FinalizeDocument(void) noexcept override
			{
				if (_finished) return false;
				if (!_pdf_context->FinalizeDocument()) return false;
				if (_api->cupsStartDestDocument(_http, _dest, _info, _job, _name, CUPS_FORMAT_PDF, 0, 0, 1) != HTTP_STATUS_CONTINUE) return false;
				auto buffer = _pdf_stream->GetStorage();
				auto status = _api->cupsWriteRequestData(_http, reinterpret_cast<const char *>(buffer->GetBuffer()), buffer->GetLength());
				if (_api->cupsFinishDestDocument(_http, _dest, _info) != IPP_STATUS_OK) return false;
				_finished = true;
				if (status != HTTP_STATUS_CONTINUE) return false;
				return true;
			}
			virtual oref<IDeviceContext2D> GetDocumentDeviceContext(void) noexcept override { return _pdf_context->GetDocumentDeviceContext(); }
			virtual Index2 GetEffectiveResolution(void) noexcept override { return _pdf_context->GetEffectiveResolution(); }
		};
		class CUPSPrinter : public IPrinter
		{
			oref<CUPS::CUPSAPI> _api;
			CUPS::http_t _http;
			CUPS::cups_dest_t * _dest;
			CUPS::cups_dinfo_t _info;
			PrinterModeDesc _mode;
			string _name;
		private:
			const char * _get_default_value_string(const char * optn) noexcept
			{
				auto value = _api->cupsGetOption(optn, _dest->num_options, _dest->options);
				if (value) return value;
				auto ipp = _api->cupsFindDestDefault(_http, _dest, _info, optn);
				if (!ipp) return 0;
				return _api->ippGetString(ipp, 0, 0);
			}
			int _get_default_value_integer(const char * optn) noexcept
			{
				auto value = _api->cupsGetOption(optn, _dest->num_options, _dest->options);
				if (value) try { return string(value).ToInt32(); } catch (...) { return 0; }
				auto ipp = _api->cupsFindDestDefault(_http, _dest, _info, optn);
				if (!ipp) return 0;
				return _api->ippGetInteger(ipp, 0);
			}
			bool _get_default_value_boolean(const char * optn) noexcept
			{
				auto value = _api->cupsGetOption(optn, _dest->num_options, _dest->options);
				if (value) try {
					auto s = string(value);
					if (string::CompareCaseInsensitively(s, U"true") == 0 || string::CompareCaseInsensitively(s, U"on") == 0) return true;
					else return false;
				} catch (...) { return 0; }
				auto ipp = _api->cupsFindDestDefault(_http, _dest, _info, optn);
				if (!ipp) return 0;
				return _api->ippGetBoolean(ipp, 0);
			}
		public:
			CUPSPrinter(CUPS::CUPSAPI * api, CUPS::cups_dest_t * dest) : _api(api), _http(0), _name(dest->name)
			{
				if (!_api->cupsCopyDest(dest, 0, &_dest)) throw OutOfMemoryException();
				if (!(_info = _api->cupsCopyDestInfo(_http, _dest))) {
					_api->cupsFreeDests(1, _dest);
					throw OutOfMemoryException();
				}
				auto orientation = _get_default_value_integer(CUPS_ORIENTATION);
				auto duplex = _get_default_value_string(CUPS_SIDES);
				auto copies = _get_default_value_integer(CUPS_COPIES);
				auto collate1 = _get_default_value_string("multiple-document-handling");
				auto collate2 = _get_default_value_boolean("collate");
				if (orientation) {
					if (orientation == 4) _mode.Orientation = PaperOrientation::Landscape;
					else _mode.Orientation = PaperOrientation::Portrait;
				} else _mode.Orientation = PaperOrientation::Portrait;
				if (duplex) {
					if (Memory::StringCompare(duplex, CUPS_SIDES_TWO_SIDED_PORTRAIT) == 0) _mode.DuplexMode = PrinterDuplexMode::Duplex;
					else if (Memory::StringCompare(duplex, CUPS_SIDES_TWO_SIDED_LANDSCAPE) == 0) _mode.DuplexMode = PrinterDuplexMode::DuplexTumble;
					else _mode.DuplexMode = PrinterDuplexMode::Simplex;
				} else _mode.DuplexMode = PrinterDuplexMode::Simplex;
				if (copies) _mode.Copies = string(copies).ToUInt32();
				else _mode.Copies = 1;
				auto ipp_dpi = _api->cupsFindDestDefault(_http, _dest, _info, "printer-resolution");
				if (ipp_dpi && _api->ippGetCount(ipp_dpi)) {
					int dpx, dpy, measure;
					dpx = _api->ippGetResolution(ipp_dpi, 0, &dpy, &measure);
					if (measure == 3) _mode.DPI = dpx;
					else if (measure == 4) _mode.DPI = dpx * 100 / 254;
					else _mode.DPI = 96;
				} else _mode.DPI = 96;
				if (collate1) _mode.Collate = Memory::StringCompare(collate1, "separate-documents-collated-copies") == 0;
				else _mode.Collate = collate2;
				CUPS::cups_size_t size;
				Memory::ZeroMemory(&size, sizeof(size));
				auto media = _get_default_value_string(CUPS_MEDIA);
				if (media) {
					auto num_formats = _api->cupsGetDestMediaCount(_http, _dest, _info, 0);
					for (int i = 0; i < num_formats; i++) {
						CUPS::cups_size_t local_size;
						if (_api->cupsGetDestMediaByIndex(_http, _dest, _info, i, 0, &local_size)) {
							if (Memory::StringCompare(local_size.media, media) == 0) { size = local_size; break; }
						}
					}
				}
				if (!size.width || !size.length) {
					if (!_api->cupsGetDestMediaDefault(_http, _dest, _info, 0, &size)) {
						_api->cupsFreeDestInfo(_info);
						_api->cupsFreeDests(1, _dest);
						throw InvalidStateException();
					}
				}
				_mode.PaperWidth = size.width / 10;
				_mode.PaperLength = size.length / 10;
			}
			virtual ~CUPSPrinter(void) override { _api->cupsFreeDestInfo(_info); _api->cupsFreeDests(1, _dest); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"CUPS Printer"; ESSE_TRY_OUTRO(string()) }
			virtual const string & GetName(void) noexcept override { return _name; }
			virtual oref<array<PaperOrientation>> EnumerateOrientations(void) noexcept override
			{
				try {
					auto result = owrap(new array<PaperOrientation>(0x10));
					if (_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_ORIENTATION, CUPS_ORIENTATION_PORTRAIT)) result->Append(PaperOrientation::Portrait);
					if (_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_ORIENTATION, CUPS_ORIENTATION_LANDSCAPE)) result->Append(PaperOrientation::Landscape);
					return result;
				} catch (...) { return 0; }
			}
			virtual oref<array<PrinterDuplexMode>> EnumerateDuplexModes(void) noexcept override
			{
				try {
					auto result = owrap(new array<PrinterDuplexMode>(0x10));
					if (_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_SIDES, CUPS_SIDES_ONE_SIDED)) result->Append(PrinterDuplexMode::Simplex);
					if (_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_SIDES, CUPS_SIDES_TWO_SIDED_PORTRAIT)) result->Append(PrinterDuplexMode::Duplex);
					if (_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_SIDES, CUPS_SIDES_TWO_SIDED_LANDSCAPE)) result->Append(PrinterDuplexMode::DuplexTumble);
					return result;
				} catch (...) { return 0; }
			}
			virtual oref<array<Index2>> EnumeratePaperFormats(void) noexcept override
			{
				try {
					auto num_formats = _api->cupsGetDestMediaCount(_http, _dest, _info, 0);
					Set<Index2> formats;
					for (int i = 0; i < num_formats; i++) {
						CUPS::cups_size_t size;
						if (_api->cupsGetDestMediaByIndex(_http, _dest, _info, i, 0, &size)) formats.AddElement(Index2(size.width / 10, size.length / 10));
					}
					auto result = owrap(new array<Index2>(num_formats));
					for (auto & f : formats) result->Append(f);
					return result;
				} catch (...) { return 0; }
			}
			virtual bool CanCollate(void) noexcept override { return _api->cupsCheckDestSupported(_http, _dest, _info, "collate", "true") != 0; }
			virtual void GetDefaultMode(PrinterModeDesc & desc) noexcept override { desc = _mode; }
			virtual bool SetDefaultMode(const PrinterModeDesc & desc) noexcept override { if (CheckMode(desc)) { _mode = desc; return true; } else return false; }
			virtual bool CheckMode(const PrinterModeDesc & desc) noexcept override
			{
				if (!desc.DPI || !desc.Copies) return false;
				auto num_formats = _api->cupsGetDestMediaCount(_http, _dest, _info, 0);
				bool size_present = false;
				for (int i = 0; i < num_formats; i++) {
					CUPS::cups_size_t size;
					if (_api->cupsGetDestMediaByIndex(_http, _dest, _info, i, 0, &size)) {
						if (size.width / 10 == desc.PaperWidth && size.length / 10 == desc.PaperLength) { size_present = true; break; }
					}
				}
				if (!size_present) return false;
				if (desc.PaperWidth * desc.DPI / 254 > 0x4000 || desc.PaperLength * desc.DPI / 254 > 0x4000) return false;
				if (desc.Orientation == PaperOrientation::Portrait) {
					if (!_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_ORIENTATION, CUPS_ORIENTATION_PORTRAIT)) return false;
				} else if (desc.Orientation == PaperOrientation::Landscape) {
					if (!_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_ORIENTATION, CUPS_ORIENTATION_LANDSCAPE)) return false;
				} else return false;
				if (desc.DuplexMode == PrinterDuplexMode::Simplex) {
					if (!_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_SIDES, CUPS_SIDES_ONE_SIDED)) return false;
				} else if (desc.DuplexMode == PrinterDuplexMode::Duplex) {
					if (!_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_SIDES, CUPS_SIDES_TWO_SIDED_PORTRAIT)) return false;
				} else if (desc.DuplexMode == PrinterDuplexMode::DuplexTumble) {
					if (!_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_SIDES, CUPS_SIDES_TWO_SIDED_LANDSCAPE)) return false;
				} else return false;
				if (desc.Collate) {
					if (!_api->cupsCheckDestSupported(_http, _dest, _info, "collate", "true")) return false;
				}
				try {
					ucs1_string value = string(desc.Copies);
					if (!_api->cupsCheckDestSupported(_http, _dest, _info, CUPS_COPIES, value)) return false;
				} catch (...) { return false; }
				return true;
			}
			virtual oref<IPrintingContext> StartPrinting(const string & document_name) noexcept override { return StartPrintingWithMode(document_name, _mode); }
			virtual oref<IPrintingContext> StartPrintingWithMode(const string & document_name, const PrinterModeDesc & desc) noexcept override
			{
				if (!CheckMode(desc)) return 0;
				try { return oref<IPrintingContext>::CreateOwned(new CUPSPrintingContext(_api, this, _http, _dest, _info, document_name, desc)); } catch (...) { return 0; }
			}
		};
		class CUPSPrinterFactory : public IPrinterFactory
		{
			oref<CUPS::CUPSAPI> _api;
			string _default;
		public:
			CUPSPrinterFactory(void) { _api = owrap(new CUPS::CUPSAPI); }
			virtual ~CUPSPrinterFactory(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"CUPS Printer Factory"; ESSE_TRY_OUTRO(string()) }
			virtual const string & GetDefaultPrinter(void) noexcept override
			{
				if (!_default.GetLength()) {
					_api->cupsEnumDests(CUPS_DEST_FLAGS_NONE, 5000, 0, 0, 0, [](void * user, unsigned flags, CUPS::cups_dest_t * dest)->int {
						if (flags & CUPS_DEST_FLAGS_REMOVED) return 1;
						if (flags & CUPS_DEST_FLAGS_ERROR) return 0;
						if (dest->is_default) {
							try { *static_cast<string *>(user) = dest->name; } catch (...) {}
							return 0;
						} else return 1;
						return 1;
					}, &_default);
				}
				return _default;
			}
			virtual oref<array<string>> EnumeratePrinters(void) noexcept override
			{
				try {
					Set<string> printers;
					auto result = owrap(new array<string>(0x10));
					_api->cupsEnumDests(CUPS_DEST_FLAGS_NONE, 5000, 0, 0, 0, [](void * user, unsigned flags, CUPS::cups_dest_t * dest)->int {
						if (flags & CUPS_DEST_FLAGS_REMOVED) return 1;
						if (flags & CUPS_DEST_FLAGS_ERROR) return 0;
						try { static_cast<Set<string> *>(user)->AddElement(dest->name); } catch (...) { return 0; }
						return 1;
					}, &printers);
					for (auto & p : printers) result->Append(p);
					return result;
				} catch (...) { return 0; }
			}
			virtual oref<IPrinter> OpenPrinter(const string & name) noexcept override
			{
				try {
					oref<CUPSPrinter> result;
					ucs1_string name1 = name;
					void * user[3] = { &result, &name1, _api.Inner() };
					_api->cupsEnumDests(CUPS_DEST_FLAGS_NONE, 5000, 0, 0, 0, [](void * user, unsigned flags, CUPS::cups_dest_t * dest)->int {
						if (flags & CUPS_DEST_FLAGS_REMOVED) return 1;
						if (flags & CUPS_DEST_FLAGS_ERROR) return 0;
						if (Memory::StringCompare(*reinterpret_cast<const ucs1_string *>(reinterpret_cast<void **>(user)[1]), dest->name) == 0) try {
							auto & result = *reinterpret_cast<oref<CUPSPrinter> *>(reinterpret_cast<void **>(user)[0]);
							auto api = reinterpret_cast<CUPS::CUPSAPI *>(reinterpret_cast<void **>(user)[2]);
							try { result = owrap(new CUPSPrinter(api, dest)); } catch (...) { return 0; }
							return 0;
						} catch (...) { return 0; } else return 1;
					}, &user);
					return result.Inner();
				} catch (...) { return 0; }
			}
		};
		oref<IPrinterFactory> CreatePrinterFactory(ErrorContext & ectx) noexcept { ESSE_TRY_INTRO return oref<IPrinterFactory>::CreateOwned(new CUPSPrinterFactory); ESSE_TRY_OUTRO(0) }
	}
}