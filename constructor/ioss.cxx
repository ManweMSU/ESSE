#include "ioss.h"

using namespace Engine;
using namespace Engine::IO;
using namespace Engine::Streaming;

namespace esse {
	namespace constructor {
		io_context::io_context(void) : silent_mode(false), verbose_level(0)
		{
			standard_input = CloneHandle(GetStandardInput());
			try { standard_output = CloneHandle(GetStandardOutput()); } catch (...) { CloseHandle(standard_input); throw; }
			try { console = new Console(standard_output, standard_input); } catch (...) { CloseHandle(standard_input); CloseHandle(standard_output); throw; }
			try {
				Assembly::CurrentLocale = Assembly::GetCurrentUserLocale();
				esse_root = Path::GetDirectory(GetExecutablePath());
				SafePointer<Storage::Registry> ioconf = load_configuration(esse_root + L"/esse.loc.ini");
				auto language_override = ioconf->GetValueString(L"Lingua");
				if (language_override.Length()) Assembly::CurrentLocale = language_override;
				auto localizations = ioconf->GetValueString(L"Locale");
				if (localizations.Length()) {
					try {
						SafePointer<Stream> table = new FileStream(esse_root + L"/" + localizations + L"/" + Assembly::CurrentLocale + L".ecst", AccessRead, OpenExisting);
						localization = new Storage::StringTable(table);
					} catch (...) {}
					if (!localization) {
						auto language_default = ioconf->GetValueString(L"LinguaDefalta");
						try {
							SafePointer<Stream> table = new FileStream(esse_root + L"/" + localizations + L"/" + language_default + L".ecst", AccessRead, OpenExisting);
							localization = new Storage::StringTable(table);
							Assembly::CurrentLocale = language_default;
						} catch (...) { localization = new Storage::StringTable; }
					}
				}
				SafePointer<Storage::RegistryNode> paths;
				paths = ioconf->OpenNode(L"Moduli");
				if (paths) for (auto & n : paths->GetValues()) module_search_list.InsertLast(ExpandPath(esse_root + L"/" + paths->GetValueString(n)));
				paths = ioconf->OpenNode(L"Data");
				if (paths) for (auto & n : paths->GetValues()) data_files_list.InsertLast(ExpandPath(esse_root + L"/" + paths->GetValueString(n)));
				paths = ioconf->OpenNode(L"Tituli");
				if (paths) for (auto & n : paths->GetValues()) include_list.InsertLast(ExpandPath(esse_root + L"/" + paths->GetValueString(n)));
				paths = ioconf->OpenNode(L"Inire");
				if (paths) for (auto & n : paths->GetValues()) init_list.InsertLast(ExpandPath(esse_root + L"/" + paths->GetValueString(n)));
			} catch (...) {
				console->SetTextColor(ConsoleColor::Red);
				console->WriteLine(L"Error initializationis subsystemae inponendi/exponendi.");
				console->SetTextColor(ConsoleColor::Default);
				console.SetReference(0);
				CloseHandle(standard_input); CloseHandle(standard_output);
				throw;
			}
		}
		io_context::~io_context(void) { console.SetReference(0); CloseHandle(standard_input); CloseHandle(standard_output); }
		string io_context::localized(int id) { try { return localization->GetString(id); } catch (...) { return string(L"#") + string(id); } }

		Engine::Storage::Registry * load_configuration(const Engine::ImmutableString & file_path, Engine::Time * date_modified)
		{
			SafePointer<FileStream> stream = new FileStream(file_path, AccessRead, OpenExisting);
			if (date_modified) *date_modified = DateTime::GetFileAlterTime(stream->Handle());
			SafePointer<Storage::Registry> reg = Storage::LoadRegistry(stream);
			if (reg) {
				reg->Retain();
				return reg;
			} else {
				stream->Seek(0, Begin);
				reg = Storage::CompileTextRegistry(stream);
				if (!reg) throw InvalidFormatException();
				reg->Retain();
				return reg;
			}
		}
	}
}