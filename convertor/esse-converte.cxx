#include <Consolatorium/Consolatorium.h>
#include <Formationes/Formationes.h>
#include <Imagines/Imagines.h>
#include <Cor/Cor.h>

using namespace ESSE;
using namespace ESSE::IO;
using namespace ESSE::Picturae;
using namespace ESSE::Formationes;

enum class TypeMode : uint { Undefined, Image, Registry, StringTable };
enum class ConvertMode : uint { ConvertPixelFormat, ConvertPixelFormatAndDither, ConvertAlphaMode, ConvertScanOrder, ConvertPalette };
enum class PaletteType : uint {
	Grayscale_1bpp,	Grayscale_1bppTransparentDecompose,
	Grayscale_2bpp,	Grayscale_2bppTransparentDecompose,
	Grayscale_4bpp,	Grayscale_4bppTransparentDecompose,
	Windows_4bpp,	Windows_4bppTransparentDecompose,
	CGA_4bpp,		CGA_4bppTransparentDecompose,
	Macintosh_4bpp,	Macintosh_4bppTransparentDecompose,
	RGB685_8bpp,	RGB685_8bppTransparentDecompose,
					RGB685_8bppTransparent,
					TransparentDecompose
};

struct input_entity
{
	string input;
	Dictionary<string, string> attributes;
	Dictionary<uint, uint> decoder_options, converts;
};
struct {
	bool silent = false, nologo = false, print_caps = false, decompose = false;
	TypeMode mode = TypeMode::Undefined;
	string output, output_format;
	List<input_entity> inputs;
	Dictionary<string, string> current_attributes;
	Dictionary<uint, uint> current_decoder_options, current_converts, encoder_options;
	oref<Console> console;
	oref<StringTable> localization;
} state;

bool Bootstrap(void) noexcept
{
	try { state.console = CreateConsole(); } catch (...) { return false; }
	try {
		unichar32 user_locale_data[3];
		System::GetUserLocale(user_locale_data, 3);
		auto user_locale = string(user_locale_data);
		auto esse_root = Path::GetDirectory(GetExecutablePath());
		auto ioconf = Registry::LoadGeneric(FileStream::Create(esse_root + U"/esse.loc.ini", FileAccess::AccessRead, FileCreationMode::OpenExisting));
		auto language_override = ioconf->GetValueString(U"Lingua");
		if (language_override.GetLength()) user_locale = language_override;
		auto localizations = ioconf->GetValueString(U"LocaleConvertoris");
		if (localizations.GetLength()) {
			try {
				auto table = FileStream::Create(esse_root + U"/" + localizations + U"/" + user_locale + U".ecst", FileAccess::AccessRead, FileCreationMode::OpenExisting);
				state.localization = StringTable::LoadGeneric(table);
			} catch (...) {}
			if (!state.localization) {
				auto language_default = ioconf->GetValueString(U"LinguaDefalta");
				try {
					auto table = FileStream::Create(esse_root + U"/" + localizations + U"/" + language_default + U".ecst", FileAccess::AccessRead, FileCreationMode::OpenExisting);
					state.localization = StringTable::LoadGeneric(table);
				} catch (...) { state.localization = StringTable::Create(); }
			}
		} else state.localization = StringTable::Create();
		return true;
	} catch (...) {
		state.console->SetTextColor(ConsoleColor::Red);
		state.console->WriteLine(L"Error initializationis subsystemae inponendi/exponendi.");
		state.console->SetTextColor(ConsoleColor::Default);
		state.console.Clear();
		return false;
	}
}
string Localized(int id) { return state.localization->GetString(id); }
oref<array<string>> GetExtensionsForImageFormat(const unichar8 * name)
{
	auto result = owrap(new array<string>(0x10));
	if (Memory::StringCompare(name, ImageFormatDIB) == 0) {
		result->Append(U"bmp"); result->Append(U"dib"); result->Append(U"rle");
	} else if (Memory::StringCompare(name, ImageFormatPNG) == 0) {
		result->Append(U"png");
	} else if (Memory::StringCompare(name, ImageFormatJPEG) == 0) {
		result->Append(U"jpg"); result->Append(U"jpeg"); result->Append(U"jfif"); result->Append(U"jpe");
	} else if (Memory::StringCompare(name, ImageFormatGIF) == 0) {
		result->Append(U"gif");
	} else if (Memory::StringCompare(name, ImageFormatTIFF) == 0) {
		result->Append(U"tif"); result->Append(U"tiff");
	} else if (Memory::StringCompare(name, ImageFormatHEIC) == 0) {
		result->Append(U"heic"); result->Append(U"heif");
	} else if (Memory::StringCompare(name, ImageFormatWEBP) == 0) {
		result->Append(U"webp");
	} else if (Memory::StringCompare(name, ImageFormatDDS) == 0) {
		result->Append(U"dds");
	} else if (Memory::StringCompare(name, ImageFormatTGA) == 0) {
		result->Append(U"tga");
	} else if (Memory::StringCompare(name, ImageFormatWindowsIcon) == 0) {
		result->Append(U"ico");
	} else if (Memory::StringCompare(name, ImageFormatWindowsCursor) == 0) {
		result->Append(U"cur");
	} else if (Memory::StringCompare(name, ImageFormatAppleIcon) == 0) {
		result->Append(U"icns");
	} else if (Memory::StringCompare(name, ImageFormatESSE) == 0) {
		result->Append(U"efie"); result->Append(U"eiwv");
	}
	return result;
}
string GetIdentifierForOption(uint opt)
{
	if (opt == EncoderOptions::OverrideBitDepth) return U"bpp";
	else if (opt == EncoderOptions::CompressionMode) return U"cmd";
	else if (opt == EncoderOptions::CompressionQuality) return U"cql";
	else if (opt == EncoderOptions::CompressionChrominanceSubsample) return U"css";
	else if (opt == DecoderOptions::TransparentcyMaskFusionMode) return U"fmd";
	else if (opt == DecoderOptions::MinimalDecodeScaleFactor) return U"smn";
	else if (opt == DecoderOptions::MaximalDecodeScaleFactor) return U"smx";
	else return string(opt);
}
uint GetOptionForIdentifier(const string & opt)
{
	if (string::CompareCaseInsensitively(opt, U"bpp") == 0) return EncoderOptions::OverrideBitDepth;
	else if (string::CompareCaseInsensitively(opt, U"cmd") == 0) return EncoderOptions::CompressionMode;
	else if (string::CompareCaseInsensitively(opt, U"cql") == 0) return EncoderOptions::CompressionQuality;
	else if (string::CompareCaseInsensitively(opt, U"css") == 0) return EncoderOptions::CompressionChrominanceSubsample;
	else if (string::CompareCaseInsensitively(opt, U"fmd") == 0) return DecoderOptions::TransparentcyMaskFusionMode;
	else if (string::CompareCaseInsensitively(opt, U"smn") == 0) return DecoderOptions::MinimalDecodeScaleFactor;
	else if (string::CompareCaseInsensitively(opt, U"smx") == 0) return DecoderOptions::MaximalDecodeScaleFactor;
	else return opt.ToUInt32();
}
uint GetPixelFormat(const string & name)
{
	if (string::CompareCaseInsensitively(name, U"B8G8R8A8") == 0) return uint(PixelFormat::B8G8R8A8);
	else if (string::CompareCaseInsensitively(name, U"R8G8B8A8") == 0) return uint(PixelFormat::R8G8B8A8);
	else if (string::CompareCaseInsensitively(name, U"B8G8R8X8") == 0) return uint(PixelFormat::B8G8R8X8);
	else if (string::CompareCaseInsensitively(name, U"R8G8B8X8") == 0) return uint(PixelFormat::R8G8B8X8);
	else if (string::CompareCaseInsensitively(name, U"B8G8R8") == 0) return uint(PixelFormat::B8G8R8);
	else if (string::CompareCaseInsensitively(name, U"R8G8B8") == 0) return uint(PixelFormat::R8G8B8);
	else if (string::CompareCaseInsensitively(name, U"B5G5R5A1") == 0) return uint(PixelFormat::B5G5R5A1);
	else if (string::CompareCaseInsensitively(name, U"B5G5R5X1") == 0) return uint(PixelFormat::B5G5R5X1);
	else if (string::CompareCaseInsensitively(name, U"B5G6R5") == 0) return uint(PixelFormat::B5G6R5);
	else if (string::CompareCaseInsensitively(name, U"R5G5B5A1") == 0) return uint(PixelFormat::R5G5B5A1);
	else if (string::CompareCaseInsensitively(name, U"R5G5B5X1") == 0) return uint(PixelFormat::R5G5B5X1);
	else if (string::CompareCaseInsensitively(name, U"R5G6B5") == 0) return uint(PixelFormat::R5G6B5);
	else if (string::CompareCaseInsensitively(name, U"B4G4R4A4") == 0) return uint(PixelFormat::B4G4R4A4);
	else if (string::CompareCaseInsensitively(name, U"B4G4R4X4") == 0) return uint(PixelFormat::B4G4R4X4);
	else if (string::CompareCaseInsensitively(name, U"R4G4B4A4") == 0) return uint(PixelFormat::R4G4B4A4);
	else if (string::CompareCaseInsensitively(name, U"R4G4B4X4") == 0) return uint(PixelFormat::R4G4B4X4);
	else if (string::CompareCaseInsensitively(name, U"R8A8") == 0) return uint(PixelFormat::R8A8);
	else if (string::CompareCaseInsensitively(name, U"B2G3R2A1") == 0) return uint(PixelFormat::B2G3R2A1);
	else if (string::CompareCaseInsensitively(name, U"B2G3R2X1") == 0) return uint(PixelFormat::B2G3R2X1);
	else if (string::CompareCaseInsensitively(name, U"B2G3R3") == 0) return uint(PixelFormat::B2G3R3);
	else if (string::CompareCaseInsensitively(name, U"R2G3B2A1") == 0) return uint(PixelFormat::R2G3B2A1);
	else if (string::CompareCaseInsensitively(name, U"R2G3B2X1") == 0) return uint(PixelFormat::R2G3B2X1);
	else if (string::CompareCaseInsensitively(name, U"R3G3B2") == 0) return uint(PixelFormat::R3G3B2);
	else if (string::CompareCaseInsensitively(name, U"B2G2R2A2") == 0) return uint(PixelFormat::B2G2R2A2);
	else if (string::CompareCaseInsensitively(name, U"B2G2R2X2") == 0) return uint(PixelFormat::B2G2R2X2);
	else if (string::CompareCaseInsensitively(name, U"R2G2B2A2") == 0) return uint(PixelFormat::R2G2B2A2);
	else if (string::CompareCaseInsensitively(name, U"R2G2B2X2") == 0) return uint(PixelFormat::R2G2B2X2);
	else if (string::CompareCaseInsensitively(name, U"R4A4") == 0) return uint(PixelFormat::R4A4);
	else if (string::CompareCaseInsensitively(name, U"A8") == 0) return uint(PixelFormat::A8);
	else if (string::CompareCaseInsensitively(name, U"R8") == 0) return uint(PixelFormat::R8);
	else if (string::CompareCaseInsensitively(name, U"P8") == 0) return uint(PixelFormat::P8);
	else if (string::CompareCaseInsensitively(name, U"R2A2") == 0) return uint(PixelFormat::R2A2);
	else if (string::CompareCaseInsensitively(name, U"A4") == 0) return uint(PixelFormat::A4);
	else if (string::CompareCaseInsensitively(name, U"R4") == 0) return uint(PixelFormat::R4);
	else if (string::CompareCaseInsensitively(name, U"P4") == 0) return uint(PixelFormat::P4);
	else if (string::CompareCaseInsensitively(name, U"R1A1") == 0) return uint(PixelFormat::R1A1);
	else if (string::CompareCaseInsensitively(name, U"A2") == 0) return uint(PixelFormat::A2);
	else if (string::CompareCaseInsensitively(name, U"R2") == 0) return uint(PixelFormat::R2);
	else if (string::CompareCaseInsensitively(name, U"P2") == 0) return uint(PixelFormat::P2);
	else if (string::CompareCaseInsensitively(name, U"A1") == 0) return uint(PixelFormat::A1);
	else if (string::CompareCaseInsensitively(name, U"R1") == 0) return uint(PixelFormat::R1);
	else if (string::CompareCaseInsensitively(name, U"P1") == 0) return uint(PixelFormat::P1);
	else throw InvalidArgumentException();
}
uint GetAlphaMode(const string & name)
{
	if (string::CompareCaseInsensitively(name, U"reg") == 0) return uint(AlphaMode::Straight);
	else if (string::CompareCaseInsensitively(name, U"praem") == 0) return uint(AlphaMode::Premultiplied);
	else throw InvalidArgumentException();
}
uint GetScanOrigin(const string & name)
{
	if (string::CompareCaseInsensitively(name, U"sup") == 0) return uint(ScanOrigin::TopLeft);
	else if (string::CompareCaseInsensitively(name, U"inf") == 0) return uint(ScanOrigin::BottomLeft);
	else throw InvalidArgumentException();
}
uint GetPaletteType(const string & name)
{
	if (string::CompareCaseInsensitively(name, U"grs1") == 0) return uint(PaletteType::Grayscale_1bpp);
	else if (string::CompareCaseInsensitively(name, U"grs1+t") == 0) return uint(PaletteType::Grayscale_1bppTransparentDecompose);
	else if (string::CompareCaseInsensitively(name, U"grs2") == 0) return uint(PaletteType::Grayscale_2bpp);
	else if (string::CompareCaseInsensitively(name, U"grs2+t") == 0) return uint(PaletteType::Grayscale_2bppTransparentDecompose);
	else if (string::CompareCaseInsensitively(name, U"grs4") == 0) return uint(PaletteType::Grayscale_4bpp);
	else if (string::CompareCaseInsensitively(name, U"grs4+t") == 0) return uint(PaletteType::Grayscale_4bppTransparentDecompose);
	else if (string::CompareCaseInsensitively(name, U"win4") == 0) return uint(PaletteType::Windows_4bpp);
	else if (string::CompareCaseInsensitively(name, U"win4+t") == 0) return uint(PaletteType::Windows_4bppTransparentDecompose);
	else if (string::CompareCaseInsensitively(name, U"cga4") == 0) return uint(PaletteType::CGA_4bpp);
	else if (string::CompareCaseInsensitively(name, U"cga4+t") == 0) return uint(PaletteType::CGA_4bppTransparentDecompose);
	else if (string::CompareCaseInsensitively(name, U"mac4") == 0) return uint(PaletteType::Macintosh_4bpp);
	else if (string::CompareCaseInsensitively(name, U"mac4+t") == 0) return uint(PaletteType::Macintosh_4bppTransparentDecompose);
	else if (string::CompareCaseInsensitively(name, U"256") == 0) return uint(PaletteType::RGB685_8bpp);
	else if (string::CompareCaseInsensitively(name, U"256+t") == 0) return uint(PaletteType::RGB685_8bppTransparentDecompose);
	else if (string::CompareCaseInsensitively(name, U"256t") == 0) return uint(PaletteType::RGB685_8bppTransparent);
	else if (string::CompareCaseInsensitively(name, U"256t+t") == 0) return uint(PaletteType::RGB685_8bppTransparentDecompose);
	else if (string::CompareCaseInsensitively(name, U"+t") == 0) return uint(PaletteType::TransparentDecompose);
	else throw InvalidArgumentException();
}
bool DecomposeAssignment(const string & src, string * dec)
{
	auto index = src.FindFirst(U'=');
	if (index < 0) return false;
	dec[0] = src.Substring(0, index);
	dec[1] = src.Substring(index + 1, -1);
	return true;
}
void ProcessCommandLine(void)
{
	auto & console = *state.console;
	auto args = GetCommandLine();
	if (args->GetLength() < 2) return;
	if (string::CompareCaseInsensitively(args->ElementAt(1), U"imago") == 0) state.mode = TypeMode::Image;
	else if (string::CompareCaseInsensitively(args->ElementAt(1), U"tabula") == 0) state.mode = TypeMode::Registry;
	else if (string::CompareCaseInsensitively(args->ElementAt(1), U"lineae") == 0) state.mode = TypeMode::StringTable;
	else { console.WriteLineFormatted(Localized(201)); throw InvalidArgumentException(); }
	for (uintptr i = 2; i < args->GetLength(); i++) {
		auto & arg = args->ElementAt(i);
		if (arg[0] == L':' || arg[0] == L'-') {
			for (uintptr j = 1; j < arg.GetLength(); j++) {
				if (arg[j] == L'C') {
					state.decompose = true;
				} else if (arg[j] == L'F') {
					state.print_caps = true;
				} else if (arg[j] == L'N') {
					state.nologo = true;
				} else if (arg[j] == L'S') {
					state.silent = true;
				} else if (arg[j] == L'a') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					string asgn[2];
					if (!DecomposeAssignment(args->ElementAt(i), asgn)) {
						console.WriteLineFormatted(Localized(205));
						throw InvalidArgumentException();
					}
					if (asgn[1].GetLength()) state.current_attributes.Update(asgn[0], asgn[1]);
					else state.current_attributes.Remove(asgn[0]);
				} else if (arg[j] == L'c') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					string asgn[2];
					if (!DecomposeAssignment(args->ElementAt(i), asgn)) {
						console.WriteLineFormatted(Localized(205));
						throw InvalidArgumentException();
					}
					try {
						uint convid;
						if (string::CompareCaseInsensitively(asgn[0], U"pxf") == 0) convid = uint(ConvertMode::ConvertPixelFormat);
						else if (string::CompareCaseInsensitively(asgn[0], U"pxfd") == 0) convid = uint(ConvertMode::ConvertPixelFormatAndDither);
						else if (string::CompareCaseInsensitively(asgn[0], U"am") == 0) convid = uint(ConvertMode::ConvertAlphaMode);
						else if (string::CompareCaseInsensitively(asgn[0], U"so") == 0) convid = uint(ConvertMode::ConvertScanOrder);
						else if (string::CompareCaseInsensitively(asgn[0], U"plt") == 0) convid = uint(ConvertMode::ConvertPalette);
						else throw InvalidArgumentException();
						if (asgn[1].GetLength()) {
							uint convval = 0;
							if (convid == uint(ConvertMode::ConvertPixelFormat) || convid == uint(ConvertMode::ConvertPixelFormatAndDither)) convval = GetPixelFormat(asgn[1]);
							else if (convid == uint(ConvertMode::ConvertAlphaMode)) convval = GetAlphaMode(asgn[1]);
							else if (convid == uint(ConvertMode::ConvertScanOrder)) convval = GetScanOrigin(asgn[1]);
							else if (convid == uint(ConvertMode::ConvertPalette)) convval = GetPaletteType(asgn[1]);
							state.current_converts.Update(convid, convval);
						} else state.current_converts.Remove(convid);
					} catch (...) {
						console.WriteLineFormatted(Localized(205));
						throw InvalidArgumentException();
					}
				} else if (arg[j] == L'd') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					string asgn[2];
					if (!DecomposeAssignment(args->ElementAt(i), asgn)) {
						console.WriteLineFormatted(Localized(205));
						throw InvalidArgumentException();
					}
					uint optid, optval;
					try {
						optid = GetOptionForIdentifier(asgn[0]);
						if (!(optid & 0x10000)) throw Exception();
					} catch (...) {
						console.WriteLineFormatted(Localized(205));
						throw InvalidArgumentException();
					}
					if (asgn[1].GetLength()) {
						try {
							optval = asgn[1].ToUInt32();
						} catch (...) {
							console.WriteLineFormatted(Localized(205));
							throw InvalidArgumentException();
						}
						state.current_decoder_options.Update(optid, optval);
					} else state.current_decoder_options.Remove(optid);
				} else if (arg[j] == L'e') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					string asgn[2];
					if (!DecomposeAssignment(args->ElementAt(i), asgn)) {
						console.WriteLineFormatted(Localized(205));
						throw InvalidArgumentException();
					}
					uint optid, optval;
					try {
						optid = GetOptionForIdentifier(asgn[0]);
						optval = asgn[1].ToUInt32();
						if (optid & 0x10000) throw Exception();
					} catch (...) {
						console.WriteLineFormatted(Localized(205));
						throw InvalidArgumentException();
					}
					state.encoder_options.Update(optid, optval);
				} else if (arg[j] == L'f') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					if (state.output_format.GetLength()) {
						console.WriteLineFormatted(Localized(202));
						throw InvalidArgumentException();
					}
					state.output_format = args->ElementAt(i).Lowercased();
				} else if (arg[j] == L'o') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					if (state.output.GetLength()) {
						console.WriteLineFormatted(Localized(202));
						throw InvalidArgumentException();
					}
					state.output = args->ElementAt(i) == U"*" ? U"*" : ExpandPath(args->ElementAt(i));
				} else {
					console.WriteLineFormatted(Localized(204));
					throw Exception();
				}
			}
		} else {
			auto file = ExpandPath(arg);
			if (file.FindFirst(U'*') >= 0 || file.FindFirst(U'?') >= 0) {
				auto files = EnumerateFiles(Path::GetDirectory(file), Path::GetFileName(file), FileSearch::FileSearchMainEntries);
				for (auto & f : *files) {
					state.inputs.InsertLast(input_entity {
						.input = ExpandPath(Path::GetDirectory(file) + U"/" + f),
						.attributes = state.current_attributes,
						.decoder_options = state.current_decoder_options,
						.converts = state.current_converts
					});
				}
			} else state.inputs.InsertLast(input_entity {
				.input = file,
				.attributes = state.current_attributes,
				.decoder_options = state.current_decoder_options,
				.converts = state.current_converts
			});
		}
	}
}
void PrintImageModeCaps(Console & console)
{
	auto formats = GetEncodeFormats();
	for (auto & cdx : *formats) {
		console.WriteLineFormatted(FormatString(Localized(301), cdx.name));
		for (auto & f : cdx.caps) {
			dynamic_string_ucs4 caps;
			if (f.value & Codices::CodecIOMode::Encode) caps += Localized(307);
			if (f.value & Codices::CodecIOMode::Decode) {
				if (caps.GetLength()) caps += U", ";
				caps += Localized(308);
			}
			if (f.value & Codices::CodecIOMode::Multiframe) {
				if (caps.GetLength()) caps += U", ";
				caps += Localized(309);
			}
			console.WriteLineFormatted(FormatString(Localized(302), string(f.key).Lowercased(), caps));
			auto ext = GetExtensionsForImageFormat(f.key);
			caps.Clear();
			for (auto & e : *ext) {
				if (caps.GetLength()) caps += U", ";
				caps << U"." << e;
			}
			console.WriteLineFormatted(FormatString(Localized(303), caps));
			Codices::CodecIOEncodeModes mode;
			if (GetEncodeMode(&mode, f.key)) {
				caps.Clear();
				for (auto & e : mode.pixel_formats) {
					if (caps.GetLength()) caps += U", ";
					caps << GetPixelFormatName(e);
				}
				console.WriteLineFormatted(FormatString(Localized(304), caps));
				for (auto & opt : mode.options) {
					auto base = opt.key & 0x10000 ? Localized(306) : Localized(305);
					console.WriteLineFormatted(FormatString(base, GetIdentifierForOption(opt.key), opt.value.key, opt.value.value));
				}
			}
		}
	}
	auto length = Localized(400).ToInt32();
	for (int i = 0; i < length; i++) state.console->WriteLineFormatted(Localized(401 + i));
}
void PrintOtherModeCaps(Console & console)
{
	auto length = Localized(500).ToInt32();
	for (int i = 0; i < length; i++) state.console->WriteLineFormatted(Localized(501 + i));
}
void ImageFormatConvert(Picture & picture, Image * dest, const Dictionary<uint, uint> & converts, Set<uint> & planes)
{
	bool alpha_decompose = false, dither = false;
	PixelFormat pxf = picture.GetDesc().format;
	AlphaMode am = picture.GetDesc().alpha_mode;
	ScanOrigin so = picture.GetDesc().origin;
	SystemPaletteType plt = SystemPaletteType::Unknown;
	if (am == AlphaMode::Undefined) am = AlphaMode::Straight;
	for (auto & c : converts) {
		if (c.key == uint(ConvertMode::ConvertPixelFormat)) {
			pxf = static_cast<PixelFormat>(c.value);
			dither = false;
		} else if (c.key == uint(ConvertMode::ConvertPixelFormatAndDither)) {
			pxf = static_cast<PixelFormat>(c.value);
			dither = true;
		} else if (c.key == uint(ConvertMode::ConvertAlphaMode)) {
			am = static_cast<AlphaMode>(c.value);
		} else if (c.key == uint(ConvertMode::ConvertScanOrder)) {
			so = static_cast<ScanOrigin>(c.value);
		} else if (c.key == uint(ConvertMode::ConvertPalette)) {
			auto pt = static_cast<PaletteType>(c.value);
			if (pt == PaletteType::Grayscale_1bpp) { plt = SystemPaletteType::Grayscale_1bit; alpha_decompose = false; }
			else if (pt == PaletteType::Grayscale_1bppTransparentDecompose) { plt = SystemPaletteType::Grayscale_1bit; alpha_decompose = true; }
			else if (pt == PaletteType::Grayscale_2bpp) { plt = SystemPaletteType::Grayscale_2bit; alpha_decompose = false; }
			else if (pt == PaletteType::Grayscale_2bppTransparentDecompose) { plt = SystemPaletteType::Grayscale_2bit; alpha_decompose = true; }
			else if (pt == PaletteType::Grayscale_4bpp) { plt = SystemPaletteType::Grayscale_4bit; alpha_decompose = false; }
			else if (pt == PaletteType::Grayscale_4bppTransparentDecompose) { plt = SystemPaletteType::Grayscale_4bit; alpha_decompose = true; }
			else if (pt == PaletteType::Windows_4bpp) { plt = SystemPaletteType::Windows_4bit; alpha_decompose = false; }
			else if (pt == PaletteType::Windows_4bppTransparentDecompose) { plt = SystemPaletteType::Windows_4bit; alpha_decompose = true; }
			else if (pt == PaletteType::CGA_4bpp) { plt = SystemPaletteType::CGA_4bit; alpha_decompose = false; }
			else if (pt == PaletteType::CGA_4bppTransparentDecompose) { plt = SystemPaletteType::CGA_4bit; alpha_decompose = true; }
			else if (pt == PaletteType::Macintosh_4bpp) { plt = SystemPaletteType::Macintosh_4bit; alpha_decompose = false; }
			else if (pt == PaletteType::Macintosh_4bppTransparentDecompose) { plt = SystemPaletteType::Macintosh_4bit; alpha_decompose = true; }
			else if (pt == PaletteType::RGB685_8bpp) { plt = SystemPaletteType::RGB685_8bit; alpha_decompose = false; }
			else if (pt == PaletteType::RGB685_8bppTransparentDecompose) { plt = SystemPaletteType::RGB685T_8bit; alpha_decompose = true; }
			else if (pt == PaletteType::RGB685_8bppTransparent) { plt = SystemPaletteType::RGB685T_8bit; alpha_decompose = false; }
			else if (pt == PaletteType::TransparentDecompose) { plt = SystemPaletteType::Unknown; alpha_decompose = true; }
		}
	}
	uint unused_plane = 0;
	auto current_plane = planes.GetFirst();
	if (current_plane->GetValue() > 1) { unused_plane = 1; } else while (current_plane) {
		auto next_plane = current_plane->GetNext();
		if (next_plane && next_plane->GetValue() - current_plane->GetValue() > 1) { unused_plane = current_plane->GetValue() + 1; break; }
		else if (!next_plane && current_plane->GetValue() < 0xFFFFFFFF) unused_plane = current_plane->GetValue() + 1;
		current_plane = next_plane;
	}
	if (!NeedsPalette(pxf) && !PixelFormatHasAlpha(pxf) && alpha_decompose) {
		PictureDesc image = picture.GetDesc(), mask = picture.GetDesc();
		image.format = pxf; mask.format = PixelFormat::A1;
		image.alpha_mode = AlphaMode::Undefined; mask.alpha_mode = AlphaMode::Straight;
		image.origin = mask.origin = so;
		image.stride = ((image.width * GetBitsPerPixel(image.format) + 31U) & ~31U) >> 3U;
		mask.stride = ((mask.width * GetBitsPerPixel(mask.format) + 31U) & ~31U) >> 3U;
		image.palette_size = mask.palette_size = 0;
		auto pcolor = owrap(new Picture(image, PictureInit::AllocateUninitialized));
		auto pmask = owrap(new Picture(mask, PictureInit::AllocateUninitialized));
		for (uint y = 0; y < image.height; y++) for (uint x = 0; x < image.width; x++) {
			auto color = picture.ReadPixel(x, y);
			pcolor->WritePixel(x, y, color);
			if (color.a >= 0x80) pmask->SetPixel(x, y, 1); else pmask->SetPixel(x, y, 0);
		}
		pcolor->GetAttributes() = pmask->GetAttributes() = picture.GetAttributes();
		pcolor->GetAttributes().plane = pmask->GetAttributes().plane = unused_plane;
		dest->Append(pcolor);
		dest->Append(pmask);
	} else {
		PictureDesc desc = picture.GetDesc();
		desc.format = pxf;
		desc.alpha_mode = am;
		desc.origin = so;
		if (NeedsPalette(desc.format)) {
			desc.palette_size = 1U << GetBitsPerPixel(desc.format);
			if (alpha_decompose && desc.palette_size < 0x100) { desc.palette_size++; desc.format = PixelFormat::P8; }
		} else desc.palette_size = 0;
		desc.stride = ((desc.width * GetBitsPerPixel(desc.format) + 31U) & ~31U) >> 3U;
		auto pnew = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
		if (NeedsPalette(desc.format)) {
			Memory::ZeroMemory(pnew->GetDesc().palette, sizeof(Color) * desc.palette_size);
			GeneratePalette(pnew->GetDesc(), plt);
			if (alpha_decompose && desc.palette_size < 0x100) { pnew->GetDesc().palette[desc.palette_size - 1].value = 0; }
		}
		if (dither && NeedsPalette(desc.format)) picture.Dither(pnew->GetDesc()); else BlockTransfer(pnew->GetDesc(), picture.GetDesc());
		if (NeedsPalette(desc.format) && alpha_decompose) {
			PictureDesc image = desc, mask = desc;
			image.format = pxf; mask.format = PixelFormat::A1;
			image.alpha_mode = AlphaMode::Undefined; mask.alpha_mode = AlphaMode::Straight;
			image.origin = mask.origin = so;
			image.stride = ((image.width * GetBitsPerPixel(image.format) + 31U) & ~31U) >> 3U;
			mask.stride = ((mask.width * GetBitsPerPixel(mask.format) + 31U) & ~31U) >> 3U;
			image.palette_size = 1U << GetBitsPerPixel(image.format);
			mask.palette_size = 0;
			auto pcolor = owrap(new Picture(image, PictureInit::AllocateUninitialized));
			auto pmask = owrap(new Picture(mask, PictureInit::AllocateUninitialized));
			Memory::ZeroMemory(pcolor->GetDesc().palette, sizeof(Color) * pcolor->GetDesc().palette_size);
			GeneratePalette(pcolor->GetDesc(), plt);
			int index_black = -1, index_transparent = -1;
			for (int i = 0; i < desc.palette_size; i++) {
				auto color = pnew->GetDesc().palette[i];
				if (color.value == 0) index_transparent = i;
				else if (color.value == 0xFF000000) index_black = i;
			}
			for (uint y = 0; y < image.height; y++) for (uint x = 0; x < image.width; x++) {
				auto index = pnew->GetPixel(x, y);
				if (index == index_transparent) {
					pcolor->SetPixel(x, y, index_black);
					pmask->SetPixel(x, y, 0);
				} else {
					pcolor->SetPixel(x, y, index);
					pmask->SetPixel(x, y, 1);
				}
			}
			pcolor->GetAttributes() = pmask->GetAttributes() = picture.GetAttributes();
			pcolor->GetAttributes().plane = pmask->GetAttributes().plane = unused_plane;
			dest->Append(pcolor);
			dest->Append(pmask);
		} else {
			pnew->GetAttributes() = picture.GetAttributes();
			dest->Append(pnew);
		}
	}
}
oref<Image> ImageFormatConvert(Image * source, const Dictionary<uint, uint> & converts, Set<uint> & planes)
{
	auto result = owrap(new Image);
	for (auto & p : *source) ImageFormatConvert(p, result, converts, planes);
	return result;
}

ESSE_MAIN_ROUTINE {
	try {
		if (!Bootstrap()) return -1;
		ProcessCommandLine();
		if (!state.nologo && !state.silent) {
			state.console->WriteLineFormatted(Localized(1));
			state.console->WriteLineFormatted(Localized(2));
			#ifdef ESSE_META_VERSIO_APPLICATIONIS
			state.console->WriteLineFormatted(FormatString(Localized(3), ESSE_META_VERSIO_APPLICATIONIS));
			#endif
			state.console->LineFeed();
		}
		auto & console = *state.console;
		if (state.mode != TypeMode::Undefined) {
			try {
				if (state.print_caps && !state.silent) {
					if (state.mode == TypeMode::Image) PrintImageModeCaps(console);
					else PrintOtherModeCaps(console);
				}
				if (state.mode == TypeMode::Image) {
					Set<uint> used_planes_set;
					ucs1_string result_fallback_format;
					oref<Image> result;
					for (auto & i : state.inputs) {
						ucs1_string input_format;
						oref<Image> input;
						try {
							if (i.decoder_options.IsEmpty()) {
								input = DecodeImage(FileStream::Create(i.input, FileAccess::AccessRead, FileCreationMode::OpenExisting), &input_format);
							} else {
								array<uint> optn(0x10), optv(0x10);
								for (auto & o : i.decoder_options) { optn << o.key; optv << o.value; }
								input = DecodeImage(FileStream::Create(i.input, FileAccess::AccessRead, FileCreationMode::OpenExisting), &input_format, optn, optv, optn.GetLength());
							}
						} catch (Exception & e) { console.WriteLineFormatted(FormatString(Localized(206), i.input, e.GetError().error_code, e.GetError().error_subcode)); return 1; }
						for (auto & a : i.attributes) {
							if (string::CompareCaseInsensitively(a.key, U"scala") == 0) {
								double scale = a.value.ToDouble();
								for (auto & p : *input) p.GetAttributes().scale_factor = scale;
							} else if (string::CompareCaseInsensitively(a.key, U"cpx") == 0) {
								uint value = a.value.ToUInt32();
								for (auto & p : *input) p.GetAttributes().pointer_offset_x = value;
							} else if (string::CompareCaseInsensitively(a.key, U"cpy") == 0) {
								uint value = a.value.ToUInt32();
								for (auto & p : *input) p.GetAttributes().pointer_offset_y = value;
							} else if (string::CompareCaseInsensitively(a.key, U"tempus") == 0) {
								uint value = a.value.ToUInt32();
								for (auto & p : *input) p.GetAttributes().animation_duration = value;
							} else if (string::CompareCaseInsensitively(a.key, U"planus") == 0) {
								uint value = a.value.ToUInt32();
								for (auto & p : *input) p.GetAttributes().plane = value;
							} else {
								console.WriteLineFormatted(FormatString(Localized(210), a.key));
								return 1;
							}
						}
						for (auto & p : *input) used_planes_set.AddElement(p.GetAttributes().plane);
						if (!i.converts.IsEmpty()) input = ImageFormatConvert(input, i.converts, used_planes_set);
						if (!result) result = input; else result->Append(input->GetBuffer(), input->GetLength());
						if (!result_fallback_format.GetLength()) result_fallback_format = input_format;
					}
					if (!result) return 0;
					if (!state.output_format.GetLength()) state.output = result_fallback_format;
					string postfix;
					auto extensions = GetExtensionsForImageFormat(ucs1_string(state.output_format.Uppercased()));
					if (extensions->GetLength() > 0) postfix = U"." + extensions->ElementAt(0);
					if (!state.output.GetLength()) {
						auto & input0 = state.inputs.GetFirst()->GetValue().input;
						uint counter = 0;
						if (state.decompose) {
							state.output = ExpandPath(Path::GetDirectory(input0) + U"/" + Path::GetPureFileName(input0));
							while (FileExists(state.output)) state.output = ExpandPath(Path::GetDirectory(input0) + U"/" + Path::GetPureFileName(input0) + U"." + string(counter++, DecimalBase, 3));
						} else {
							state.output = ExpandPath(Path::GetDirectory(input0) + U"/" + Path::GetPureFileName(input0) + postfix);
							while (FileExists(state.output)) state.output = ExpandPath(Path::GetDirectory(input0) + U"/" + Path::GetPureFileName(input0) + U"." + string(counter++, DecimalBase, 3) + postfix);
						}
					}
					if (state.decompose) {
						try { CreateDirectoryTree(state.output); }
						catch (Exception & e) { console.WriteLineFormatted(FormatString(Localized(207), state.output, e.GetError().error_code, e.GetError().error_subcode)); return 2; }
						uintptr counter = 0;
						for (auto & p : *result) {
							auto output_path = ExpandPath(state.output + U"/" + string(counter++, DecimalBase, 4) + postfix);
							try {
								auto output_stream = FileStream::Create(output_path, FileAccess::AccessReadWrite, FileCreationMode::CreateAlways);
								if (state.encoder_options.IsEmpty()) {
									Encode(output_stream, &p, ucs1_string(state.output_format.Uppercased()));
								} else {
									array<uint> optn(0x10), optv(0x10);
									for (auto & o : state.encoder_options) { optn << o.key; optv << o.value; }
									Encode(output_stream, &p, ucs1_string(state.output_format.Uppercased()), optn, optv, optn.GetLength());
								}
							} catch (Exception & e) {
								if (e.GetError().error_code == Errores::ErrorNotImplemented) console.WriteLineFormatted(FormatString(Localized(209), state.output_format));
								else console.WriteLineFormatted(FormatString(Localized(207), output_path, e.GetError().error_code, e.GetError().error_subcode)); return 2;
								return 2;
							}
						}	
					} else {
						try {
							auto output_stream = FileStream::Create(state.output, FileAccess::AccessReadWrite, FileCreationMode::CreateAlways);
							if (state.encoder_options.IsEmpty()) {
								Encode(output_stream, result, ucs1_string(state.output_format.Uppercased()));
							} else {
								array<uint> optn(0x10), optv(0x10);
								for (auto & o : state.encoder_options) { optn << o.key; optv << o.value; }
								Encode(output_stream, result, ucs1_string(state.output_format.Uppercased()), optn, optv, optn.GetLength());
							}
						} catch (Exception & e) {
							if (e.GetError().error_code == Errores::ErrorNotImplemented) console.WriteLineFormatted(FormatString(Localized(209), state.output_format));
							else console.WriteLineFormatted(FormatString(Localized(207), state.output, e.GetError().error_code, e.GetError().error_subcode)); return 2;
							return 2;
						}
					}
				} else if (state.mode == TypeMode::Registry) {
					object_array<Registry> inputs(0x10);
					for (auto & i : state.inputs) {
						oref<Registry> input;
						try { input = Registry::LoadGeneric(FileStream::Create(i.input, FileAccess::AccessRead, FileCreationMode::OpenExisting)); }
						catch (Exception & e) {
							console.WriteLineFormatted(FormatString(Localized(206), i.input, e.GetError().error_code, e.GetError().error_subcode));
							return 1;
						}
						for (auto & a : i.attributes) {
							try { input->RemoveValue(a.key); } catch (...) {}
							input->CreateValue(a.key, RegistryValueType::String);
							input->SetValue(a.key, a.value);
						}
						inputs.Append(input);
					}
					oref<Registry> result;
					if (!inputs.GetLength()) return 0;
					else if (inputs.GetLength() == 1) result = inputs(0);
					else result = Registry::Create(RegistryNode::Merge(reinterpret_cast<RegistryNode **>(inputs.GetBuffer()), inputs.GetLength()));
					if (!state.output_format.GetLength()) state.output = U"*";
					string ext;
					if (string::CompareCaseInsensitively(state.output_format, U"bin") == 0) ext = "ecsr"; else ext = "ini";
					if (!state.output.GetLength()) {
						auto & input0 = state.inputs.GetFirst()->GetValue().input;
						uint counter = 0;
						state.output = ExpandPath(Path::GetDirectory(input0) + U"/" + Path::GetPureFileName(input0) + U"." + ext);
						while (FileExists(state.output)) state.output = ExpandPath(Path::GetDirectory(input0) + U"/" + Path::GetPureFileName(input0) + U"." + string(counter++, DecimalBase, 3) + U"." + ext);
					}
					try {
						if (state.output == U"*") {
							ErrorContext ectx; ErrorClear(ectx);
							auto encoder = owrap(reinterpret_cast<ITextEncoder *>(console.DynamicCast(Classes.ITextEncoder, ectx)));
							ErrorThrow(ectx);
							result->SaveToText(encoder, false);
						} else {
							auto output_stream = FileStream::Create(state.output, FileAccess::AccessWrite, FileCreationMode::CreateAlways);
							if (string::CompareCaseInsensitively(state.output_format, U"bin") == 0) {
								result->Save(output_stream);
							} else if (string::CompareCaseInsensitively(state.output_format, U"ascii") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::ASCII);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf8") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF8);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf16") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF16);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf16-le") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF16_LE);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf16-be") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF16_BE);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf32") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF32);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf32-le") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF32_LE);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf32-be") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF32_BE);
							} else {
								console.WriteLineFormatted(FormatString(Localized(209), state.output_format));
								return 4;
							}
						}
					} catch (Exception & e) { console.WriteLineFormatted(FormatString(Localized(207), state.output, e.GetError().error_code, e.GetError().error_subcode)); return 2; }
				} else if (state.mode == TypeMode::StringTable) {
					oref<StringTable> result;
					for (auto & i : state.inputs) {
						oref<StringTable> input;
						try { input = StringTable::LoadGeneric(FileStream::Create(i.input, FileAccess::AccessRead, FileCreationMode::OpenExisting)); }
						catch (Exception & e) {
							console.WriteLineFormatted(FormatString(Localized(206), i.input, e.GetError().error_code, e.GetError().error_subcode));
							return 1;
						}
						for (auto & a : i.attributes) {
							auto id = a.key.ToInt32();
							try { input->RemoveString(id); } catch (...) {}
							input->AddString(a.value, id);
						}
						if (!result) result = input; else {
							auto index = input->GetIndex();
							for (auto & i : *index) result->AddString(input->GetString(i), i);
						}
					}
					if (!result) return 0;
					if (!state.output_format.GetLength()) state.output = U"*";
					string ext;
					if (string::CompareCaseInsensitively(state.output_format, U"bin") == 0) ext = "ecst"; else ext = "txt";
					if (!state.output.GetLength()) {
						auto & input0 = state.inputs.GetFirst()->GetValue().input;
						uint counter = 0;
						state.output = ExpandPath(Path::GetDirectory(input0) + U"/" + Path::GetPureFileName(input0) + U"." + ext);
						while (FileExists(state.output)) state.output = ExpandPath(Path::GetDirectory(input0) + U"/" + Path::GetPureFileName(input0) + U"." + string(counter++, DecimalBase, 3) + U"." + ext);
					}
					try {
						if (state.output == U"*") {
							ErrorContext ectx; ErrorClear(ectx);
							auto encoder = owrap(reinterpret_cast<ITextEncoder *>(console.DynamicCast(Classes.ITextEncoder, ectx)));
							ErrorThrow(ectx);
							result->SaveToText(encoder, false);
						} else {
							auto output_stream = FileStream::Create(state.output, FileAccess::AccessWrite, FileCreationMode::CreateAlways);
							if (string::CompareCaseInsensitively(state.output_format, U"bin") == 0) {
								result->Save(output_stream);
							} else if (string::CompareCaseInsensitively(state.output_format, U"ascii") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::ASCII);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf8") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF8);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf16") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF16);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf16-le") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF16_LE);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf16-be") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF16_BE);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf32") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF32);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf32-le") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF32_LE);
							} else if (string::CompareCaseInsensitively(state.output_format, U"utf32-be") == 0) {
								result->SaveToText(output_stream, Unicode::Encoding::UTF32_BE);
							} else {
								console.WriteLineFormatted(FormatString(Localized(209), state.output_format));
								return 4;
							}
						}
					} catch (Exception & e) { console.WriteLineFormatted(FormatString(Localized(207), state.output, e.GetError().error_code, e.GetError().error_subcode)); return 2; }
				}
			} catch (Exception & e) {
				console.WriteLineFormatted(FormatString(Localized(208), e.GetError().error_code, e.GetError().error_subcode));
				return 3;
			}
		} else {
			if (!state.silent) try {
				auto length = Localized(100).ToInt32();
				for (int i = 0; i < length; i++) state.console->WriteLineFormatted(Localized(101 + i));
			} catch (...) {}
		}
	} catch (...) { return -1; }
	return 0;
}