#include "esse_devenv.h"

using namespace Engine;
using namespace Engine::IO;
using namespace Engine::Streaming;

namespace esse {
	namespace constructor {
		bool devenv_create_workspace(build_state & build)
		{
			try {
				FileStream stream(build.project_root_path + L"/" + IO::Path::GetFileNameWithoutExtension(build.project_file_path) + L".code-workspace", AccessReadWrite, CreateAlways);
				TextWriter writer(&stream, Encoding::UTF8);
				writer.WriteLine(L"{");
				writer.WriteLine(L"\t\"folders\": [");
				writer.WriteLine(L"\t\t{");
				writer.WriteLine(L"\t\t\t\"path\": \".\"");
				writer.WriteLine(L"\t\t}");
				writer.WriteLine(L"\t],");
				writer.WriteLine(L"\t\"settings\": {}");
				writer.Write(L"}");
			} catch (...) {
				if (!build.io->silent_mode) {
					build.io->console->SetTextColor(ConsoleColor::Red);
					build.io->console->WriteLine(build.io->localized(229));
					build.io->console->SetTextColor(ConsoleColor::Default);
					return false;
				}
			}
			return true;
		}
		bool devenv_create_c_environment(build_state & build)
		{
			try {
				string part_name;
				#ifdef ENGINE_WINDOWS
				part_name = L"Win32";
				#endif
				#ifdef ENGINE_MACOSX
				part_name = L"Mac";
				#endif
				#ifdef ENGINE_LINUX
				part_name = L"Linux";
				#endif
				try { CreateDirectory(build.project_root_path + L"/.vscode"); } catch (...) {}
				FileStream stream(build.project_root_path + L"/.vscode/c_cpp_properties.json", AccessReadWrite, OpenAlways);
				Configurations configs;
				Reflection::JsonSerializer serializer(&stream);
				serializer.DeserializeObject(configs);
				Configuration * config = 0;
				for (auto & c : configs.configurations) if (c.name == part_name) { config = &c; break; }
				if (!config) {
					configs.version = 4;
					configs.configurations.AppendNew();
					config = &configs.configurations.InnerArray.LastElement();
					config->name = part_name;
				}
				config->includePath.Clear();
				config->browse.path.Clear();
				config->includePath << L"${workspaceFolder}/**";
				for (auto & inc : build.include_list) {
					auto norm = inc.Replace(L'\\', L'/');
					config->includePath << norm;
					config->browse.path << norm + L"/*";
				}
				config->defines.Clear();
				config->defines << L"UNICODE=1";
				config->defines << L"_UNICODE=1";
				for (auto & def : build.defines_list) config->defines << (def.key + L"=" + def.value);
				#ifdef ENGINE_WINDOWS
				auto cc = local_config->GetValueString(L"Compiler/Path");
				auto sdk = config_state.registry->GetValueString(L"SDK");
				config->macFrameworkPath.Clear();
				config->compilerPath = cc;
				config->windowsSdkVersion = sdk;
				config->intelliSenseMode = L"msvc-x64";
				#endif
				#ifdef ENGINE_MACOSX
				config->macFrameworkPath.Clear();
				config->macFrameworkPath << L"/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks";
				config->compilerPath = L"/usr/bin/clang";
				config->intelliSenseMode = L"clang-x64";
				#endif
				#ifdef ENGINE_LINUX
				config->macFrameworkPath.Clear();
				config->includePath << L"/usr/include";
				config->browse.path << L"/usr/include/*";
				config->compilerPath = L"/usr/bin/gcc";
				config->intelliSenseMode = L"linux-gcc-x64";
				#endif
				config->cStandard = L"c11";
				config->cppStandard = L"c++17";
				stream.Seek(0, Begin);
				stream.SetLength(0);
				serializer.SerializeObject(configs);
			} catch (...) {
				if (!build.io->silent_mode) {
					build.io->console->SetTextColor(ConsoleColor::Red);
					build.io->console->WriteLine(build.io->localized(230));
					build.io->console->SetTextColor(ConsoleColor::Default);
					return false;
				}
			}
			return true;
		}
		void devenv_create_build_task(build_state & build, BuildTasks & tasks, const string & arch, const string & os, const string & conf)
		{
			auto arch_info = build.installed_targets[arch.LowerCase()];
			auto os_info = build.installed_targets[os.LowerCase()];
			auto conf_info = build.installed_targets[conf.LowerCase()];
			if (!arch_info || !os_info || !conf_info) return;
			auto name = FormatString(L"%3 (%0, %1, %2)", read_localized_string(os_info->name), read_localized_string(arch_info->name), read_localized_string(conf_info->name), build.io->localized(323));
			BuildTask * task = 0;
			for (auto & t : tasks.tasks) if (t.label == name) { task = &t; break; }
			if (!task) {
				tasks.version = L"2.0.0";
				tasks.tasks.AppendNew();
				task = &tasks.tasks.InnerArray.LastElement();
				task->label = name;
			}
			task->command = L"esse";
			task->args.Clear();
			task->args << L"${workspaceFolder}/" + IO::Path::GetFileName(build.project_file_path);
			task->args << L"-Naco";
			task->args << arch_info->identifier.LowerCase();
			task->args << conf_info->identifier.LowerCase();
			task->args << os_info->identifier.LowerCase();
			task->type = L"shell";
			task->group.kind = L"build";
			task->group.isDefault = true;
		}
		bool devenv_create_build_tasks(build_state & build)
		{
			try {
				FileStream stream(build.project_root_path + L"/.vscode/tasks.json", AccessReadWrite, OpenAlways);
				BuildTasks tasks;
				Reflection::JsonSerializer serializer(&stream);
				serializer.DeserializeObject(tasks);
				for (auto & t : build.installed_targets) if (t.value.type == build_target_class::architecture) {
					devenv_create_build_task(build, tasks, t.key, build.project_system->identifier, L"Release");
					devenv_create_build_task(build, tasks, t.key, build.project_system->identifier, L"Debug");
				}
				stream.Seek(0, Begin);
				stream.SetLength(0);
				serializer.SerializeObject(tasks);
			} catch (...) {
				if (!build.io->silent_mode) {
					build.io->console->SetTextColor(ConsoleColor::Red);
					build.io->console->WriteLine(build.io->localized(231));
					build.io->console->SetTextColor(ConsoleColor::Default);
					return false;
				}
			}
			return true;
		}
		bool devenv_create_launch_task(build_state & build)
		{
			try {
				if (string::CompareIgnoreCase(build.project_subsystem->identifier, L"Library") == 0) return true;
				auto name = FormatString(L"%1 (%0)", read_localized_string(build.project_system->name), build.io->localized(324));
				FileStream stream(build.project_root_path + L"/.vscode/launch.json", AccessReadWrite, OpenAlways);
				LaunchList list;
				Reflection::JsonSerializer serializer(&stream);
				serializer.DeserializeObject(list);
				LaunchTask * task = 0;
				for (auto & t : list.configurations) if (t.name == name) { task = &t; break; }
				if (!task) {
					list.version = L"0.2.0";
					list.configurations.AppendNew();
					task = &list.configurations.InnerArray.LastElement();
					task->name = name;
				}
				task->request = L"launch";
				task->program = build.output_exec_path;
				task->cwd = IO::Path::GetDirectory(build.output_exec_path);
				task->stopAtEntry = false;
				task->externalConsole = (string::CompareIgnoreCase(build.project_subsystem->identifier, L"Console") == 0) && (string::CompareIgnoreCase(build.project_system->identifier, L"Linux") != 0);
				#ifdef ENGINE_WINDOWS
				task->type = L"cppvsdbg";
				task->MIMode = L"";
				#endif
				#ifdef ENGINE_MACOSX
				task->type = L"cppdbg";
				task->MIMode = L"lldb";
				#endif
				#ifdef ENGINE_LINUX
				task->type = L"cppdbg";
				task->MIMode = L"gdb";
				#endif
				stream.Seek(0, Begin);
				stream.SetLength(0);
				serializer.SerializeObject(list);
			} catch (...) {
				if (!build.io->silent_mode) {
					build.io->console->SetTextColor(ConsoleColor::Red);
					build.io->console->WriteLine(build.io->localized(232));
					build.io->console->SetTextColor(ConsoleColor::Default);
					return false;
				}
			}
			return true;
		}
		bool make_development_environment(build_state & build, bool make_environment, bool make_workspace)
		{
			if (make_workspace) if (!devenv_create_workspace(build)) return false;
			if (make_environment) {
				if (!devenv_create_c_environment(build)) return false;
				if (!devenv_create_build_tasks(build)) return false;
				if (!devenv_create_launch_task(build)) return false;
			}
			return true;
		}
	}
}