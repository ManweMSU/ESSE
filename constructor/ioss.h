#pragma once

#include <EngineRuntime.h>

namespace esse {
	namespace constructor {
		struct io_context
		{
			bool silent_mode;
			Engine::uint verbose_level;
			Engine::ImmutableString esse_root;
			Engine::SafePointer<Engine::Storage::StringTable> localization;
			Engine::SafePointer<Engine::IO::Console> console;
			Engine::handle standard_input, standard_output;
			Engine::Volumes::List<Engine::ImmutableString> module_search_list, include_list, data_files_list, init_list;

			io_context(void);
			~io_context(void);
			Engine::ImmutableString localized(int id);
		};

		Engine::Storage::Registry * load_configuration(const Engine::ImmutableString & file_path, Engine::Time * date_modified = 0);
	}
}