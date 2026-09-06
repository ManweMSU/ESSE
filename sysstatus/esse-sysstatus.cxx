#include <Consolatorium/Consolatorium.h>
#include <Formationes/Formationes.h>
#include <Imprimitio/Imprimitio.h>
#include <Fenestrae/Fenestrae.h>
#include <Graphica/Graphica.h>
#include <Energia/Energia.h>
#include <Cor/Cor.h>

using namespace ESSE;
using namespace ESSE::IO;
using namespace ESSE::Graphica;
using namespace ESSE::Formationes;

constexpr uint OutputLevelNone  = 0;
constexpr uint OutputLevelBasic = 1;
constexpr uint OutputLevelFull  = 2;

struct {
	bool nologo = false;
	uint cpu = OutputLevelNone, gpu = OutputLevelNone, printers = OutputLevelNone, power = OutputLevelNone, windows = OutputLevelNone;
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
		auto localizations = ioconf->GetValueString(U"LocaleStatiSystemae");
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
void ProcessCommandLine(void)
{
	auto & console = *state.console;
	auto args = GetCommandLine();
	for (uintptr i = 1; i < args->GetLength(); i++) {
		auto & arg = args->ElementAt(i);
		if (arg[0] == L':' || arg[0] == L'-') {
			for (uintptr j = 1; j < arg.GetLength(); j++) {
				if (arg[j] == L'N') {
					state.nologo = true;
				} else if (arg[j] == L'U') {
					state.cpu = state.gpu = state.printers = state.power = state.windows = OutputLevelFull;
				} else if (arg[j] == L'e') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(201));
						throw InvalidArgumentException();
					}
					uint level;
					try {
						level = args->ElementAt(i).ToUInt32();
						if (level > 2) throw InvalidArgumentException();
					} catch (...) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					state.power = level;
				} else if (arg[j] == L'f') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(201));
						throw InvalidArgumentException();
					}
					uint level;
					try {
						level = args->ElementAt(i).ToUInt32();
						if (level > 2) throw InvalidArgumentException();
					} catch (...) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					state.windows = level;
				} else if (arg[j] == L'g') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(201));
						throw InvalidArgumentException();
					}
					uint level;
					try {
						level = args->ElementAt(i).ToUInt32();
						if (level > 2) throw InvalidArgumentException();
					} catch (...) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					state.gpu = level;
				} else if (arg[j] == L'i') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(201));
						throw InvalidArgumentException();
					}
					uint level;
					try {
						level = args->ElementAt(i).ToUInt32();
						if (level > 2) throw InvalidArgumentException();
					} catch (...) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					state.printers = level;
				} else if (arg[j] == L'p') {
					i++; if (i >= args->GetLength()) {
						console.WriteLineFormatted(Localized(201));
						throw InvalidArgumentException();
					}
					uint level;
					try {
						level = args->ElementAt(i).ToUInt32();
						if (level > 2) throw InvalidArgumentException();
					} catch (...) {
						console.WriteLineFormatted(Localized(203));
						throw InvalidArgumentException();
					}
					state.cpu = level;
				} else if (arg[j] == L'u') {
					state.cpu = state.gpu = state.printers = state.power = state.windows = OutputLevelBasic;
				} else {
					console.WriteLineFormatted(Localized(202));
					throw Exception();
				}
			}
		} else {
			console.WriteLineFormatted(Localized(202));
			throw Exception();
		}
	}
}
void TableView(const string * table, uint columns, uint rows, uint space = 0)
{
	array<uint> colwidth(columns);
	for (uint i = 0; i < columns; i++) {
		uint width = 0;
		for (uint j = 0; j < rows; j++) {
			auto & cell = table[columns * j + i];
			uint ne = 0;
			for (uintptr k = 0; k < cell.GetLength(); k++) if (cell[k] == U'\033') ne++;
			uint cw = cell.GetLength() - 3 * ne;
			if (cw > width) width = cw;
		}
		colwidth.Append(width);
	}
	for (uint j = 0; j < rows; j++) {
		if (space) state.console->Write(string(U' ', space));
		for (uint i = 0; i < columns; i++) {
			auto & cell = table[columns * j + i];
			uint ne = 0;
			for (uintptr k = 0; k < cell.GetLength(); k++) if (cell[k] == U'\033') ne++;
			uint cw = cell.GetLength() - 3 * ne, xw = colwidth[i];
			state.console->WriteFormatted(cell + string(U' ', xw + 1 - cw));
		}
		state.console->LineFeed();
	}
}
string ArchToString(System::Architecture arch)
{
	if (arch == System::Architecture::X86_32) return Localized(309);
	else if (arch == System::Architecture::X86_64) return Localized(310);
	else if (arch == System::Architecture::ARMv7_T32) return Localized(311);
	else if (arch == System::Architecture::ARMv8_A64) return Localized(312);
	else return Localized(308);
}
string NumericString(uint64 value, uint64 divisor, uint prec)
{
	uint64 res = value % divisor;
	for (uint i = 0; i < prec; i++) res *= 10;
	res /= divisor;
	return string(value / divisor) + Localized(300) + string(res, DecimalBase, prec);
}
string ProcessorFeatureState(System::ProcessorFeature f0, System::ProcessorFeature f1)
{
	auto s0 = System::GetProcessorFeatureStatus(f0);
	auto s1 = System::GetProcessorFeatureStatus(f1);
	if (s0 == System::ProcessorFeatureStatus::Present) return Localized(329);
	else if (s1 == System::ProcessorFeatureStatus::Present) return Localized(330);
	else if (s0 == System::ProcessorFeatureStatus::Unknown || s1 == System::ProcessorFeatureStatus::Unknown) return Localized(331);
	else return Localized(328);
}

ESSE_MAIN_ROUTINE {
	try {
		if (!Bootstrap()) return -1;
		ProcessCommandLine();
		if (!state.nologo) {
			state.console->WriteLineFormatted(Localized(1));
			state.console->WriteLineFormatted(Localized(2));
			#ifdef ESSE_META_VERSIO_APPLICATIONIS
			state.console->WriteLineFormatted(FormatString(Localized(3), ESSE_META_VERSIO_APPLICATIONIS));
			#endif
			state.console->LineFeed();
		}
		auto & console = *state.console;
		if (state.cpu || state.gpu || state.printers || state.power || state.windows) {
			if (state.cpu) {
				uint v1, v2;
				array<unichar32> name(0x100);
				name.SetLength(0x200);
				array<string> main_table(0x100);
				state.console->WriteLineFormatted(Localized(301));
				System::GetSystemName(name, name.GetLength());
				System::GetSystemVersion(&v1, &v2);
				main_table << (U"  \033E*" + Localized(302) + U"\033-*:") << FormatString(Localized(303), string(name.GetBuffer()), v1, v2);
				System::GetProcessorName(name, name.GetLength());
				main_table << (U"  \033E*" + Localized(304) + U"\033-*:");
				if (name[0]) main_table << (U"\033F*" + string(name.GetBuffer()) + U"\033-*");
				else main_table << (U"\033B*" + Localized(305) + U"\033-*");
				main_table << (U"  \033E*" + Localized(306) + U"\033-*:") << (U"\033F*" + ArchToString(System::GetSystemArchitecture()) + U"\033-*");
				string emu;
				if (System::IsArchitectureEmulationEnabled(System::Architecture::X86_32)) {
					if (emu[0]) emu += U", ";
					emu += U"\033A*" + ArchToString(System::Architecture::X86_32) + U"\033-*";
				}
				if (System::IsArchitectureEmulationEnabled(System::Architecture::X86_64)) {
					if (emu[0]) emu += U", ";
					emu += U"\033A*" + ArchToString(System::Architecture::X86_64) + U"\033-*";
				}
				if (System::IsArchitectureEmulationEnabled(System::Architecture::ARMv7_T32)) {
					if (emu[0]) emu += U", ";
					emu += U"\033A*" + ArchToString(System::Architecture::ARMv7_T32) + U"\033-*";
				}
				if (System::IsArchitectureEmulationEnabled(System::Architecture::ARMv8_A64)) {
					if (emu[0]) emu += U", ";
					emu += U"\033A*" + ArchToString(System::Architecture::ARMv8_A64) + U"\033-*";
				}
				main_table << (U"  \033E*" + Localized(307) + U"\033-*:") << emu;
				auto freq = System::GetProcessorFrequency();
				main_table << (U"  \033E*" + Localized(313) + U"\033-*:");
				if (freq) main_table << (U"\033F*" + NumericString(freq, 1000000, 3) + U"\033-* " + Localized(315));
				else main_table << (U"\033B*" + Localized(314) + U"\033-*");
				main_table << (U"  \033E*" + Localized(316) + U"\033-*:") << (U"\033F*" + string(System::GetProcessorCores(true)) + U"\033-*");
				main_table << (U"  \033E*" + Localized(317) + U"\033-*:") << (U"\033F*" + string(System::GetProcessorCores(false)) + U"\033-*");
				main_table << (U"  \033E*" + Localized(318) + U"\033-*:") << (U"\033F*" + NumericString(System::GetPhysicalMemory(), 1024 * 1024, 3) + U"\033-* " + Localized(320));
				main_table << (U"  \033E*" + Localized(319) + U"\033-*:") << (U"\033F*" + NumericString(System::GetVirtualMemoryPageSize(), 1024, 3) + U"\033-* " + Localized(321));
				TableView(main_table, 2, main_table.GetLength() / 2);
				if (state.cpu >= 2) {
					state.console->WriteLineFormatted(U"  \033E*" + Localized(322) + U"\033-*:");
					string ttable[12];
					ttable[0] = U"   ";
					ttable[1] = Localized(323);
					ttable[2] = Localized(324);
					ttable[3] = Localized(325);
					ttable[4] = Localized(326);
					ttable[5] = Localized(327);
					ttable[6] = U"   ";
					ttable[7] = ProcessorFeatureState(System::ProcessorFeature::CPUID, System::ProcessorFeature::CPUID);
					ttable[8] = ProcessorFeatureState(System::ProcessorFeature::RNG, System::ProcessorFeature::RNG);
					ttable[9] = ProcessorFeatureState(System::ProcessorFeature::AES, System::ProcessorFeature::AES);
					ttable[10] = ProcessorFeatureState(System::ProcessorFeature::SHA256, System::ProcessorFeature::SHA256SW);
					ttable[11] = ProcessorFeatureState(System::ProcessorFeature::SHA512, System::ProcessorFeature::SHA512SW);
					TableView(ttable, 6, 2);
				}
			}
			if (state.power) {
				state.console->WriteLineFormatted(Localized(401));
				Power::PowerStatusDesc desc;
				Power::GetPowerStatus(desc);
				string ps;
				if (desc.status == Power::PowerStatus::LinePower) ps = Localized(402);
				else if (desc.status == Power::PowerStatus::BatteryDischarging) ps = FormatString(Localized(403), uint(desc.battery_charge_level));
				else if (desc.status == Power::PowerStatus::BatteryCharging) ps = FormatString(Localized(404), uint(desc.battery_charge_level));
				else ps = Localized(405);
				state.console->WriteLineFormatted(U"  \033E*" + Localized(401) + U"\033-*: " + ps);
			}
			if (state.windows) {
				state.console->WriteLineFormatted(Localized(501));
				ErrorContext ectx; ErrorClear(ectx);
				auto ws = Windows::CreateWindowSystem(ectx);
				if (ErrorTest(ectx)) { state.console->WriteLineFormatted(U"  \033E*" + Localized(501) + U"\033-*: " + Localized(508)); ws = 0; }
				else state.console->WriteLineFormatted(U"  \033E*" + Localized(501) + U"\033-*: \033F*" + ws->ToString() + U"\033-*");
				ErrorClear(ectx);
				auto fact = Graphica::CreateDeviceContextFactory2D(ectx);
				oref<Graphica::IDeviceContext2D> dc;
				if (!ErrorTest(ectx)) {
					auto bitmap = fact->CreateBitmap(256, 256, 0);
					if (bitmap) dc = fact->CreateBitmapContext(bitmap);
				}
				if (dc) {
					uint v1, v2;
					string tech;
					dc->GetImplementationInfo(tech, v1, v2);
					state.console->WriteLineFormatted(U"  \033E*" + Localized(502) + U"\033-*: " + FormatString(Localized(503), tech, v1, v2));
					if (state.windows >= 2) {
						uint caps = dc->GetImplementationFeatures();
						if (caps & Graphica::DeviceContextSupportsPolygons) state.console->WriteLineFormatted(U"    \033A*" + Localized(504) + U"\033-*");
						if (caps & Graphica::DeviceContextSupportsLayers) state.console->WriteLineFormatted(U"    \033A*" + Localized(505) + U"\033-*");
						if (caps & Graphica::DeviceContextSupportsInversionEffect) state.console->WriteLineFormatted(U"    \033A*" + Localized(506) + U"\033-*");
						if (caps & Graphica::DeviceContextSupportsBlurEffect) state.console->WriteLineFormatted(U"    \033A*" + Localized(507) + U"\033-*");
					}
				} else state.console->WriteLineFormatted(U"  \033E*" + Localized(502) + U"\033-*: " + Localized(508));
				if (ws) {
					auto screens = ws->EnumerateScreens();
					auto default_screen = ws->GetDefaultScreen();
					if (screens) for (auto & s : *screens) {
						auto rect = s.GetScreenRectangle();
						auto user = s.GetUserRectangle();
						auto base = Localized(default_screen && default_screen->GetScreenRectangle() == s.GetScreenRectangle() ? 510 : 509);
						state.console->WriteLineFormatted(U"  " + FormatString(base, s.GetName()));
						array<string> main_table(0x100);
						main_table << (U"    \033E*" + Localized(511) + U"\033-*:") << (U"\033F*" + string(s.GetResolution().x) + U"\033-* x \033F*" + string(s.GetResolution().y) + U"\033-*");
						main_table << (U"    \033E*" + Localized(512) + U"\033-*:") << FormatString(Localized(514), rect.left, rect.top, rect.right, rect.bottom, user.left, user.top, user.right, user.bottom);
						main_table << (U"    \033E*" + Localized(513) + U"\033-*:") << (U"\033F*" + NumericString(s.GetScaleFactor() * 100.0, 100, 2) + U"\033-*");
						TableView(main_table, 2, main_table.GetLength() / 2);
					}
				}
			}
			if (state.gpu) {
				state.console->WriteLineFormatted(Localized(601));
				ErrorContext ectx; ErrorClear(ectx);
				auto fact = Graphica::CreateDeviceFactory(ectx);
				if (!ErrorTest(ectx)) {
					auto devlist = fact->EnumerateDevices();
					if (devlist && !devlist->IsEmpty()) {
						auto devdef = fact->CreateDefaultDevice();
						for (auto & dev : *devlist) {
							auto base = Localized(devdef && devdef->GetDeviceIdentifier() == dev.key ? 605 : 604);
							state.console->WriteLineFormatted(U"  " + FormatString(base, dev.value, string(dev.key, HexadecimalBase, 16)));
							auto device = fact->CreateDevice(dev.key);
							if (device) {
								array<string> main_table(0x100);
								uint v1, v2;
								string tech;
								auto devcls = device->GetDeviceClass();
								device->GetImplementationInfo(tech, v1, v2);
								main_table << (U"    \033E*" + Localized(606) + U"\033-*:") << FormatString(Localized(607), tech, v1, v2);
								main_table << (U"    \033E*" + Localized(608) + U"\033-*:");
								if (devcls == Graphica::DeviceClass::Integrated) main_table << Localized(609);
								else if (devcls == Graphica::DeviceClass::Discrete) main_table << Localized(610);
								else if (devcls == Graphica::DeviceClass::Software) main_table << Localized(611);
								else main_table << Localized(612);
								main_table << (U"    \033E*" + Localized(613) + U"\033-*:") << (U"\033F*" + NumericString(device->GetDeviceMemory(), 1024 * 1024, 3) + U"\033-* " + Localized(614));
								if (state.gpu >= 2) main_table << (U"    \033E*" + Localized(615) + U"\033-*:") << U"";
								TableView(main_table, 2, main_table.GetLength() / 2);
								if (state.gpu >= 2) {
									Graphica::PixelFormat pxf;
									array<Graphica::PixelFormatUsage> usage(10);
									array<string> format_table(0x100);
									for (uint i = 616; i <= 624; i++) state.console->WriteLineFormatted(U"      \e8*" + Localized(i) + U"\e-*");
									format_table << U"";
									for (uint i = 625; i <= 633; i++) format_table << Localized(i);
									usage << Graphica::PixelFormatUsage::ShaderRead << Graphica::PixelFormatUsage::BitmapSource << Graphica::PixelFormatUsage::ShaderSample;
									usage << Graphica::PixelFormatUsage::RenderTarget << Graphica::PixelFormatUsage::BlendRenderTarget << Graphica::PixelFormatUsage::RenderTarget2D;
									usage << Graphica::PixelFormatUsage::DepthStencil << Graphica::PixelFormatUsage::WindowSurface << Graphica::PixelFormatUsage::VideoIO;
									#define PIXEL_FORMAT_EXAMINE(NAME) format_table << (U"    \033E*" + string(# NAME) + U"\033-*:"); \
										pxf = Graphica::PixelFormat::NAME; \
										for (auto & u : usage) format_table << Localized(device->GetDevicePixelFormatSupport(pxf, u) ? 634 : 635);
									PIXEL_FORMAT_EXAMINE(A8_unorm)
									PIXEL_FORMAT_EXAMINE(R8_unorm)
									PIXEL_FORMAT_EXAMINE(R8_snorm)
									PIXEL_FORMAT_EXAMINE(R8_uint)
									PIXEL_FORMAT_EXAMINE(R8_sint)
									PIXEL_FORMAT_EXAMINE(R16_unorm)
									PIXEL_FORMAT_EXAMINE(R16_snorm)
									PIXEL_FORMAT_EXAMINE(R16_uint)
									PIXEL_FORMAT_EXAMINE(R16_sint)
									PIXEL_FORMAT_EXAMINE(R16_float)
									PIXEL_FORMAT_EXAMINE(R8G8_unorm)
									PIXEL_FORMAT_EXAMINE(R8G8_snorm)
									PIXEL_FORMAT_EXAMINE(R8G8_uint)
									PIXEL_FORMAT_EXAMINE(R8G8_sint)
									PIXEL_FORMAT_EXAMINE(B5G6R5_unorm)
									PIXEL_FORMAT_EXAMINE(R5G6B5_unorm)
									PIXEL_FORMAT_EXAMINE(B5G5R5A1_unorm)
									PIXEL_FORMAT_EXAMINE(R5G5B5A1_unorm)
									PIXEL_FORMAT_EXAMINE(A1B5G5R5_unorm)
									PIXEL_FORMAT_EXAMINE(A1R5G5B5_unorm)
									PIXEL_FORMAT_EXAMINE(B4G4R4A4_unorm)
									PIXEL_FORMAT_EXAMINE(R4G4B4A4_unorm)
									PIXEL_FORMAT_EXAMINE(A4B4G4R4_unorm)
									PIXEL_FORMAT_EXAMINE(A4R4G4B4_unorm)
									PIXEL_FORMAT_EXAMINE(R32_uint)
									PIXEL_FORMAT_EXAMINE(R32_sint)
									PIXEL_FORMAT_EXAMINE(R32_float)
									PIXEL_FORMAT_EXAMINE(R16G16_unorm)
									PIXEL_FORMAT_EXAMINE(R16G16_snorm)
									PIXEL_FORMAT_EXAMINE(R16G16_uint)
									PIXEL_FORMAT_EXAMINE(R16G16_sint)
									PIXEL_FORMAT_EXAMINE(R16G16_float)
									PIXEL_FORMAT_EXAMINE(B8G8R8A8_unorm)
									PIXEL_FORMAT_EXAMINE(R8G8B8A8_unorm)
									PIXEL_FORMAT_EXAMINE(R8G8B8A8_snorm)
									PIXEL_FORMAT_EXAMINE(R8G8B8A8_uint)
									PIXEL_FORMAT_EXAMINE(R8G8B8A8_sint)
									PIXEL_FORMAT_EXAMINE(R10G10B10A2_unorm)
									PIXEL_FORMAT_EXAMINE(R10G10B10A2_uint)
									PIXEL_FORMAT_EXAMINE(A2R10G10B10_unorm)
									PIXEL_FORMAT_EXAMINE(A2R10G10B10_uint)
									PIXEL_FORMAT_EXAMINE(R11G11B10_float)
									PIXEL_FORMAT_EXAMINE(B10G11R11_float)
									PIXEL_FORMAT_EXAMINE(R9G9B9E5_float)
									PIXEL_FORMAT_EXAMINE(E5B9G9R9_float)
									PIXEL_FORMAT_EXAMINE(R32G32_uint)
									PIXEL_FORMAT_EXAMINE(R32G32_sint)
									PIXEL_FORMAT_EXAMINE(R32G32_float)
									PIXEL_FORMAT_EXAMINE(R16G16B16A16_unorm)
									PIXEL_FORMAT_EXAMINE(R16G16B16A16_snorm)
									PIXEL_FORMAT_EXAMINE(R16G16B16A16_uint)
									PIXEL_FORMAT_EXAMINE(R16G16B16A16_sint)
									PIXEL_FORMAT_EXAMINE(R16G16B16A16_float)
									PIXEL_FORMAT_EXAMINE(R32G32B32A32_uint)
									PIXEL_FORMAT_EXAMINE(R32G32B32A32_sint)
									PIXEL_FORMAT_EXAMINE(R32G32B32A32_float)
									PIXEL_FORMAT_EXAMINE(D16_unorm)
									PIXEL_FORMAT_EXAMINE(D24_unorm)
									PIXEL_FORMAT_EXAMINE(D32_float)
									PIXEL_FORMAT_EXAMINE(D16S8_unorm)
									PIXEL_FORMAT_EXAMINE(D24S8_unorm)
									PIXEL_FORMAT_EXAMINE(D32S8_float)
									TableView(format_table, usage.GetLength() + 1, format_table.GetLength() / (usage.GetLength() + 1));
								}
							}
						}
					} else state.console->WriteLineFormatted(U"  " + Localized(603));
				} else state.console->WriteLineFormatted(U"  " + Localized(602));
			}
			if (state.printers) {
				state.console->WriteLineFormatted(Localized(701));
				ErrorContext ectx; ErrorClear(ectx);
				auto fact = Graphica::CreatePrinterFactory(ectx);
				if (!ErrorTest(ectx)) {
					auto devlist = fact->EnumeratePrinters();
					if (devlist && devlist->GetLength()) {
						auto devdef = fact->GetDefaultPrinter();
						for (auto & dev : *devlist) {
							auto base = Localized(devdef == dev ? 705 : 704);
							state.console->WriteLineFormatted(U"  " + FormatString(base, dev));
							auto device = fact->OpenPrinter(dev);
							if (device) {
								Graphica::PrinterModeDesc mode;
								device->GetDefaultMode(mode);
								auto po = device->EnumerateOrientations();
								auto dm = device->EnumerateDuplexModes();
								auto pf = device->EnumeratePaperFormats();
								state.console->WriteLineFormatted(U"    \033E*" + Localized(706) + U"\033-*:");
								for (auto & v : *po) {
									string color = v == mode.Orientation ? U"A*" : U"F*";
									if (v == Graphica::PaperOrientation::Portrait) state.console->WriteLineFormatted(U"      \033" + color + Localized(710) + U"\033-*");
									else if (v == Graphica::PaperOrientation::Landscape) state.console->WriteLineFormatted(U"      \033" + color + Localized(711) + U"\033-*");
								}
								state.console->WriteLineFormatted(U"    \033E*" + Localized(707) + U"\033-*:");
								for (auto & v : *dm) {
									string color = v == mode.DuplexMode ? U"A*" : U"F*";
									if (v == Graphica::PrinterDuplexMode::Simplex) state.console->WriteLineFormatted(U"      \033" + color + Localized(712) + U"\033-*");
									else if (v == Graphica::PrinterDuplexMode::Duplex) state.console->WriteLineFormatted(U"      \033" + color + Localized(713) + U"\033-*");
									else if (v == Graphica::PrinterDuplexMode::DuplexTumble) state.console->WriteLineFormatted(U"      \033" + color + Localized(714) + U"\033-*");
								}
								state.console->WriteLineFormatted(U"    \033E*" + Localized(708) + U"\033-*:");
								array<string> table(0x100);
								for (auto & f : *pf) table << FormatString(Localized(f.x == mode.PaperWidth && f.y == mode.PaperLength ? 718 : 715), NumericString(f.x, 10, 1), NumericString(f.y, 10, 1));
								while (table.GetLength() % 6) table << U"";
								TableView(table, 6, table.GetLength() / 6, 6);
								state.console->WriteLineFormatted(U"    \033E*" + Localized(709) + U"\033-*: " + (device->CanCollate() ? Localized(716) : Localized(717)));
							}
						}
					} else state.console->WriteLineFormatted(U"  " + Localized(703));
				} else state.console->WriteLineFormatted(U"  " + Localized(702));
			}
		} else {
			try {
				auto length = Localized(100).ToInt32();
				for (int i = 0; i < length; i++) state.console->WriteLineFormatted(Localized(101 + i));
			} catch (...) {}
		}
	} catch (...) { return -1; }
	return 0;
}