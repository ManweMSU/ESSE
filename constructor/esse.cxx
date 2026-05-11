#include "kernel.h"
#include "ioss.h"

using namespace Engine;
using namespace Engine::IO;
using namespace Engine::IO::ConsoleControl;

struct {
	string input;
	string os, arch, mode;
	bool nologo = false, clean = false, informative = false, idle = false;
	bool print_output_executable = false;
	bool print_output_bundle = false;
	bool reconfigure = false, create_vscode_environment = false, create_vscode_workspace = false;
	Engine::Volumes::List<string> module_search_list, include_list, data_files_list;
} state;

void ProcessCommandLine(esse::constructor::io_context & io)
{
	SafePointer< Array<string> > args = GetCommandLine();
	for (int i = 1; i < args->Length(); i++) {
		auto & arg = args->ElementAt(i);
		if (arg == L"--reconfigura") {
			if (state.reconfigure || state.input.Length()) {
				(*io.console) << TextColor(12) << io.localized(202) << TextColorDefault() << LineFeed();
				throw Exception();
			}
			state.reconfigure = true;
		} else if (arg[0] == L':' || arg[0] == L'-') {
			for (int j = 1; j < arg.Length(); j++) {
				if (arg[j] == L'B') {
					state.print_output_bundle = true;
				} else if (arg[j] == L'C') {
					state.clean = true;
				} else if (arg[j] == L'D') {
					i++; if (i >= args->Length()) {
						(*io.console) << TextColor(12) << io.localized(203) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					auto & arg2 = args->ElementAt(i);
					state.data_files_list.InsertLast(ExpandPath(arg2));
				} else if (arg[j] == L'E') {
					state.idle = true;
				} else if (arg[j] == L'F') {
					state.informative = true;
				} else if (arg[j] == L'I') {
					i++; if (i >= args->Length()) {
						(*io.console) << TextColor(12) << io.localized(203) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					auto & arg2 = args->ElementAt(i);
					state.include_list.InsertLast(ExpandPath(arg2));
				} else if (arg[j] == L'M') {
					i++; if (i >= args->Length()) {
						(*io.console) << TextColor(12) << io.localized(203) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					auto & arg2 = args->ElementAt(i);
					state.module_search_list.InsertLast(ExpandPath(arg2));
				} else if (arg[j] == L'N') {
					state.nologo = true;
				} else if (arg[j] == L'O') {
					state.print_output_executable = true;
				} else if (arg[j] == L'S') {
					io.silent_mode = true;
				} else if (arg[j] == L'a') {
					i++; if (i >= args->Length()) {
						(*io.console) << TextColor(12) << io.localized(203) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					auto & arg2 = args->ElementAt(i);
					if (state.arch.Length()) {
						(*io.console) << TextColor(12) << io.localized(205) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					state.arch = arg2;
				} else if (arg[j] == L'c') {
					i++; if (i >= args->Length()) {
						(*io.console) << TextColor(12) << io.localized(203) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					auto & arg2 = args->ElementAt(i);
					if (state.mode.Length()) {
						(*io.console) << TextColor(12) << io.localized(205) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					state.mode = arg2;
				} else if (arg[j] == L'd') {
					if (state.mode.Length()) {
						(*io.console) << TextColor(12) << io.localized(205) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					state.mode = L"debug";
				} else if (arg[j] == L'e') {
					state.create_vscode_environment = true;
				} else if (arg[j] == L'o') {
					i++; if (i >= args->Length()) {
						(*io.console) << TextColor(12) << io.localized(203) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					auto & arg2 = args->ElementAt(i);
					if (state.os.Length()) {
						(*io.console) << TextColor(12) << io.localized(205) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					state.os = arg2;
				} else if (arg[j] == L'r') {
					if (state.mode.Length()) {
						(*io.console) << TextColor(12) << io.localized(205) << TextColorDefault() << LineFeed();
						throw Exception();
					}
					state.mode = L"release";
				} else if (arg[j] == L'v') {
					io.verbose_level++;
				} else if (arg[j] == L'w') {
					state.create_vscode_workspace = true;
				} else {
					(*io.console) << TextColor(12) << io.localized(204) << TextColorDefault() << LineFeed();
					throw Exception();
				}
			}
		} else {
			if (state.reconfigure) {
				(*io.console) << TextColor(12) << io.localized(202) << TextColorDefault() << LineFeed();
				throw Exception();
			}
			if (state.input.Length()) {
				(*io.console) << TextColor(12) << io.localized(201) << TextColorDefault() << LineFeed();
				throw Exception();
			}
			state.input = ExpandPath(arg);
		}
	}
}

int Main(void)
{
	try {
		esse::constructor::io_context io;
		ProcessCommandLine(io);
		if (!state.nologo && !io.silent_mode) {
			io.console->WriteLine(io.localized(1));
			io.console->WriteLine(io.localized(2));
			io.console->WriteLine(FormatString(io.localized(3), ENGINE_VI_APPVERSION));
			io.console->LineFeed();
		}
		if (!io.silent_mode && io.verbose_level >= 3) {
			io.console->SetTextColor(ConsoleColor::Cyan);
			io.console->WriteLine(FormatString(L"Lingua constructoris %0 est, tabulatio localizationis %1.", Assembly::CurrentLocale.UpperCase(), io.localization ? L"onerata" : L"afuit"));
			io.console->WriteLine(L"Semitae reperiendi modulorum:");
			for (auto & s : io.module_search_list) io.console->WriteLine(L"  " + s + L" (de configuratione)");
			for (auto & s : state.module_search_list) io.console->WriteLine(L"  " + s + L" (de argumentis)");
			io.console->WriteLine(L"Semitae titulorum:");
			for (auto & s : io.include_list) io.console->WriteLine(L"  " + s + L" (de configuratione)");
			for (auto & s : state.include_list) io.console->WriteLine(L"  " + s + L" (de argumentis)");
			io.console->WriteLine(L"Semitae reperiendi auxiliorum:");
			for (auto & s : io.data_files_list) io.console->WriteLine(L"  " + s + L" (de configuratione)");
			for (auto & s : state.data_files_list) io.console->WriteLine(L"  " + s + L" (de argumentis)");
			io.console->SetTextColor(ConsoleColor::Default);
		}
		if (state.input.Length() || state.reconfigure || state.informative) {
			if (state.reconfigure) {

				// TODO: IMPLEMENT AUTOCONFIGURATION
				// bool reconfigure = false;

			} else {
				esse::constructor::build_state build;
				if (!esse::constructor::build_state_initialize(build, io.esse_root + L"/esse.ini", io)) return -1;
				build.idle_mode = state.idle;
				build.clean_mode = state.clean;
				for (auto & p : state.module_search_list) build.module_search_list.InsertLast(p);
				for (auto & p : io.module_search_list) build.module_search_list.InsertLast(p);
				for (auto & p : state.include_list) build.include_list.InsertLast(p);
				for (auto & p : io.include_list) build.include_list.InsertLast(p);
				for (auto & p : state.data_files_list) build.data_files_list.InsertLast(p);
				for (auto & p : io.data_files_list) build.data_files_list.InsertLast(p);
				if (state.informative && !io.silent_mode) {
					auto length = 0;
					for (auto & t : build.installed_targets) { auto l = t.value.identifier.Length(); if (l > length) length = l; }
					io.console->WriteLine(io.localized(302));
					bool empty = true;
					for (auto & t : build.installed_targets) if (t.value.type == esse::constructor::build_target_class::architecture) {
						io.console->SetTextColor(ConsoleColor::Magenta);
						io.console->Write(L"  " + t.value.identifier + string(L' ', length + 1 - t.value.identifier.Length()));
						io.console->SetTextColor(ConsoleColor::Default);
						io.console->Write(L"- ");
						if (t.value.is_default) io.console->Write(io.localized(305) + L" ");
						io.console->WriteLine(esse::constructor::read_localized_string(t.value.name));
						empty = false;
					}
					if (empty) io.console->WriteLine(L"  " + io.localized(306));
					io.console->WriteLine(io.localized(301));
					empty = true;
					for (auto & t : build.installed_targets) if (t.value.type == esse::constructor::build_target_class::system) {
						io.console->SetTextColor(ConsoleColor::Blue);
						io.console->Write(L"  " + t.value.identifier + string(L' ', length + 1 - t.value.identifier.Length()));
						io.console->SetTextColor(ConsoleColor::Default);
						io.console->Write(L"- ");
						if (t.value.is_default) io.console->Write(io.localized(305) + L" ");
						io.console->WriteLine(esse::constructor::read_localized_string(t.value.name));
						empty = false;
					}
					if (empty) io.console->WriteLine(L"  " + io.localized(306));
					io.console->WriteLine(io.localized(303));
					empty = true;
					for (auto & t : build.installed_targets) if (t.value.type == esse::constructor::build_target_class::configuration) {
						io.console->SetTextColor(ConsoleColor::Cyan);
						io.console->Write(L"  " + t.value.identifier + string(L' ', length + 1 - t.value.identifier.Length()));
						io.console->SetTextColor(ConsoleColor::Default);
						io.console->Write(L"- ");
						if (t.value.is_default) io.console->Write(io.localized(305) + L" ");
						io.console->WriteLine(esse::constructor::read_localized_string(t.value.name));
						empty = false;
					}
					if (empty) io.console->WriteLine(L"  " + io.localized(306));
					io.console->WriteLine(io.localized(304));
					empty = true;
					for (auto & t : build.installed_targets) if (t.value.type == esse::constructor::build_target_class::subsystem) {
						io.console->SetTextColor(ConsoleColor::Green);
						io.console->Write(L"  " + t.value.identifier + string(L' ', length + 1 - t.value.identifier.Length()));
						io.console->SetTextColor(ConsoleColor::Default);
						io.console->Write(L"- ");
						if (t.value.is_default) io.console->Write(io.localized(305) + L" ");
						io.console->WriteLine(esse::constructor::read_localized_string(t.value.name));
						empty = false;
					}
					if (empty) io.console->WriteLine(L"  " + io.localized(306));
				}
				if (state.input.Length()) {
					auto extension = Path::GetExtension(state.input);
					if (string::CompareIgnoreCase(extension, L"esse") == 0) {
						if (!esse::constructor::load_esse_project(build, state.input, state.os, state.arch, state.mode)) return 1;
					} else if (string::CompareIgnoreCase(extension, L"cpp") == 0 || string::CompareIgnoreCase(extension, L"cxx") == 0 || string::CompareIgnoreCase(extension, L"cc") == 0 || string::CompareIgnoreCase(extension, L"c") == 0) {
						if (!esse::constructor::load_adhoc_project(build, state.input, state.os, state.arch, state.mode)) return 1;
					} else {
						if (!esse::constructor::load_engine_runtime_project(build, state.input, state.os, state.arch, state.mode)) return 1;
					}
					if (!esse::constructor::make_project_dependencies(build)) return 2;
					if (!esse::constructor::construct(build)) return 3;

					// TODO: IMPLEMENT

					// bool print_output_executable = false;
					// bool print_output_bundle = false;
					// bool create_vscode_environment = false, create_vscode_workspace = false;

				}
			}
		} else {
			if (!io.silent_mode) try {
				auto length = io.localized(100).ToInt32();
				for (int i = 0; i < length; i++) io.console->WriteLine(io.localized(101 + i));
			} catch (...) {}
		}
	} catch (...) { return -1; }
	return 0;
}