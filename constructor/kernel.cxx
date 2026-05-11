#include "kernel.h"

using namespace Engine;
using namespace Engine::IO;
using namespace Engine::IO::ConsoleControl;
using namespace Engine::Storage;

namespace esse {
	namespace constructor {
		string read_localized_string(const localized_string & str)
		{
			auto var = str[Assembly::CurrentLocale];
			if (var) return *var;
			var = str[L""];
			if (var) return *var; else return L"";
		}
		localized_string load_localized_string(Engine::Storage::RegistryNode * node, const string & path)
		{
			localized_string result;
			auto src = node->GetValueString(path).Split(L'\33');
			if (src.Length()) result.Append(L"", src[0]);
			for (int i = 1; i < src.Length(); i++) result.Append(src[i].Fragment(0, 2), src[i].Fragment(3, -1));
			return result;
		}
		bool build_state_initialize(build_state & state, const string & tsc, io_context & io)
		{
			state.io = &io;
			state.idle_mode = state.clean_mode = false;
			state.project_system = state.project_architecture = state.project_configuration = state.project_subsystem = 0;
			try { state.build_set_common_configuration = load_configuration(tsc); } catch (...) {
				if (!io.silent_mode) (*io.console) << TextColor(12) << io.localized(206) << TextColorDefault() << LineFeed();
				return false;
			}
			SafePointer<RegistryNode> targets = state.build_set_common_configuration->OpenNode(L"Destinationes");
			if (targets) for (auto & n : targets->GetSubnodes()) {
				SafePointer<RegistryNode> target = targets->OpenNode(n);
				if (target) {
					auto ss = target->GetValueString(L"Classis");
					build_target tg;
					tg.identifier = n;
					tg.name = load_localized_string(target, L"Nomen");
					tg.is_default = target->GetValueBoolean(L"Defalta");
					if (string::CompareIgnoreCase(ss, L"systema") == 0) tg.type = build_target_class::system;
					else if (string::CompareIgnoreCase(ss, L"architectura") == 0) tg.type = build_target_class::architecture;
					else if (string::CompareIgnoreCase(ss, L"configuratio") == 0) tg.type = build_target_class::configuration;
					else if (string::CompareIgnoreCase(ss, L"subsystema") == 0) tg.type = build_target_class::subsystem;
					else continue;
					state.installed_targets.Append(tg.identifier.LowerCase(), tg);
					if (tg.is_default) {
						if (tg.type == build_target_class::system && !state.project_system) state.project_system = state.installed_targets[tg.identifier.LowerCase()];
						else if (tg.type == build_target_class::architecture && !state.project_architecture) state.project_architecture = state.installed_targets[tg.identifier.LowerCase()];
						else if (tg.type == build_target_class::configuration && !state.project_configuration) state.project_configuration = state.installed_targets[tg.identifier.LowerCase()];
						else if (tg.type == build_target_class::subsystem && !state.project_subsystem) state.project_subsystem = state.installed_targets[tg.identifier.LowerCase()];
					}
				}
			}
			return true;
		}
		bool build_state_initialize_2(build_state & state, const string & sys, const string & arch, const string & mode, const string & subsys)
		{
			auto & io = *state.io;
			if (sys.Length()) {
				auto target = state.installed_targets[sys.LowerCase()];
				if (!target || target->type != build_target_class::system) {
					if (!io.silent_mode) (*io.console) << TextColor(12) << FormatString(io.localized(207), sys) << TextColorDefault() << LineFeed();
					return false;
				}
				state.project_system = target;
			}
			if (arch.Length()) {
				auto target = state.installed_targets[arch.LowerCase()];
				if (!target || target->type != build_target_class::architecture) {
					if (!io.silent_mode) (*io.console) << TextColor(12) << FormatString(io.localized(208), arch) << TextColorDefault() << LineFeed();
					return false;
				}
				state.project_architecture = target;
			}
			if (mode.Length()) {
				auto target = state.installed_targets[mode.LowerCase()];
				if (!target || target->type != build_target_class::configuration) {
					if (!io.silent_mode) (*io.console) << TextColor(12) << FormatString(io.localized(209), mode) << TextColorDefault() << LineFeed();
					return false;
				}
				state.project_configuration = target;
			}
			if (subsys.Length()) {
				auto target = state.installed_targets[subsys.LowerCase()];
				if (!target || target->type != build_target_class::subsystem) {
					if (!io.silent_mode) (*io.console) << TextColor(12) << FormatString(io.localized(210), subsys) << TextColorDefault() << LineFeed();
					return false;
				}
				state.project_subsystem = target;
			}
			if (!state.project_system) {
				if (!io.silent_mode) (*io.console) << TextColor(12) << io.localized(211) << TextColorDefault() << LineFeed();
				return false;
			}
			if (!state.project_architecture) {
				if (!io.silent_mode) (*io.console) << TextColor(12) << io.localized(212) << TextColorDefault() << LineFeed();
				return false;
			}
			if (!state.project_configuration) {
				if (!io.silent_mode) (*io.console) << TextColor(12) << io.localized(213) << TextColorDefault() << LineFeed();
				return false;
			}
			if (!state.project_subsystem) {
				if (!io.silent_mode) (*io.console) << TextColor(12) << io.localized(214) << TextColorDefault() << LineFeed();
				return false;
			}
			if (!io.silent_mode && io.verbose_level >= 2) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				state.io->console->WriteLine(FormatString(L"Systema operans -> %0, architectura processoris -> %1, configuratio -> %2, subsystema -> %3.", state.project_system->identifier, state.project_architecture->identifier, state.project_configuration->identifier, state.project_subsystem->identifier));
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			try {
				ObjectArray<RegistryNode> confnodes(0x10);
				for (auto & n : state.build_set_common_configuration->GetSubnodes()) {
					auto names = n.Split(L'-');
					bool active = true;
					for (auto & sn : names) {
						if (string::CompareIgnoreCase(sn, state.project_system->identifier) == 0) continue;
						else if (string::CompareIgnoreCase(sn, state.project_architecture->identifier) == 0) continue;
						else if (string::CompareIgnoreCase(sn, state.project_configuration->identifier) == 0) continue;
						else if (string::CompareIgnoreCase(sn, state.project_subsystem->identifier) == 0) continue;
						else { active = false; break; }
					}
					if (active) {
						if (!io.silent_mode && io.verbose_level >= 3) {
							state.io->console->SetTextColor(ConsoleColor::Cyan);
							state.io->console->WriteLine(FormatString(L"Subconfiguratio nova: %0.", n));
							state.io->console->SetTextColor(ConsoleColor::Default);
						}
						SafePointer<RegistryNode> subconf = state.build_set_common_configuration->OpenNode(n);
						if (subconf) confnodes.Append(subconf);
					}
				}
				SafePointer<RegistryNode> superconf = CreateMergedNode(confnodes);
				state.build_set_common_configuration = CreateRegistryFromNode(superconf);
				if (!io.silent_mode && io.verbose_level >= 4) {
					state.io->console->SetTextColor(ConsoleColor::Cyan);
					state.io->console->WriteLine(L" === SUPERCONFIGURATIO ===");
					RegistryToText(state.build_set_common_configuration, io.console);
					state.io->console->WriteLine(L" === FINIS SUPERCONFIGURATIONIS ===");
					state.io->console->SetTextColor(ConsoleColor::Default);
				}
			} catch (...) { return false; }
			try {
				ObjectArray<RegistryNode> confnodes(0x10);
				Array<string> confnodes_delete(0x20);
				confnodes.Append(state.project_common_configuration);
				for (auto & n : state.project_common_configuration->GetSubnodes()) if (n[0] == L'-') {
					auto names = n.Split(L'-');
					bool active = true;
					for (auto & sn : names) if (sn.Length()) {
						if (string::CompareIgnoreCase(sn, state.project_system->identifier) == 0) continue;
						else if (string::CompareIgnoreCase(sn, state.project_architecture->identifier) == 0) continue;
						else if (string::CompareIgnoreCase(sn, state.project_configuration->identifier) == 0) continue;
						else if (string::CompareIgnoreCase(sn, state.project_subsystem->identifier) == 0) continue;
						else { active = false; break; }
					}
					if (active) {
						if (!io.silent_mode && io.verbose_level >= 3) {
							state.io->console->SetTextColor(ConsoleColor::Cyan);
							state.io->console->WriteLine(FormatString(L"Subconfiguratio projecti nova: %0.", n));
							state.io->console->SetTextColor(ConsoleColor::Default);
						}
						SafePointer<RegistryNode> subconf = state.project_common_configuration->OpenNode(n);
						if (subconf) confnodes.Append(subconf);
					}
					confnodes_delete.Append(n);
				}
				for (auto & n : confnodes_delete) state.project_common_configuration->RemoveNode(n);
				SafePointer<RegistryNode> superconf = CreateMergedNode(confnodes);
				state.project_common_configuration = CreateRegistryFromNode(superconf);
				if (!io.silent_mode && io.verbose_level >= 4) {
					state.io->console->SetTextColor(ConsoleColor::Cyan);
					state.io->console->WriteLine(L" === PROJECTUM ===");
					RegistryToText(state.project_common_configuration, io.console);
					state.io->console->WriteLine(L" === FINIS PROJECTI ===");
					state.io->console->SetTextColor(ConsoleColor::Default);
				}
			} catch (...) { return false; }
			
			// // toolset state
			// Engine::Volumes::ObjectDictionary<string, build_tool> compilers; // extension - handler
			// Engine::SafePointer<build_tool> cc_tool, resource_tool, link_tool;

			// TODO: IMPLEMENT

			return true;
		}
		bool load_esse_project(build_state & state, const string & prc, const string & sys, const string & arch, const string & mode)
		{
			if (!state.io->silent_mode && state.io->verbose_level >= 1) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				state.io->console->WriteLine(L"Onero ESSE projectum '" + prc + L"'...");
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			try { state.project_common_configuration = load_configuration(prc, &state.project_alternation_date); } catch (...) {
				if (!state.io->silent_mode) (*state.io->console) << TextColor(12) << state.io->localized(215) << TextColorDefault() << LineFeed();
				return false;
			}
			if (!build_state_initialize_2(state, sys, arch, mode, state.project_common_configuration->GetValueString(L"Subsystema"))) return false;
			state.project_file_path = prc;
			state.project_root_path = Path::GetDirectory(prc);
			SafePointer<module> root = new module;

			// TODO: IMPLEMENT

			// // application build set
			// application app;
			// module * root_module;

			state.modules.Append(root->identifier, root);
			state.root_module = root;
			return true;
		}
		bool load_engine_runtime_project(build_state & state, const string & prc, const string & sys, const string & arch, const string & mode)
		{
			if (!state.io->silent_mode && state.io->verbose_level >= 1) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				state.io->console->WriteLine(L"Onero Engine Runtime projectum '" + prc + L"'...");
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			try { state.project_common_configuration = load_configuration(prc, &state.project_alternation_date); } catch (...) {
				if (!state.io->silent_mode) (*state.io->console) << TextColor(12) << state.io->localized(215) << TextColorDefault() << LineFeed();
				return false;
			}
			if (!build_state_initialize_2(state, sys, arch, mode, state.project_common_configuration->GetValueString(L"Subsystem"))) return false;
			state.project_file_path = prc;
			state.project_root_path = Path::GetDirectory(prc);
			SafePointer<module> root = new module;

			// TODO: IMPLEMENT

			// // application build set
			// application app;
			// module * root_module;
			
			state.modules.Append(root->identifier, root);
			state.root_module = root;
			return true;
		}
		bool load_adhoc_project(build_state & state, const string & prc, const string & sys, const string & arch, const string & mode)
		{
			if (!state.io->silent_mode && state.io->verbose_level >= 1) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				state.io->console->WriteLine(L"Creo ad-hoc projectum '" + prc + L"'...");
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			state.project_common_configuration = CreateRegistry();
			if (!build_state_initialize_2(state, sys, arch, mode, L"")) return false;
			state.project_file_path = prc;
			state.project_root_path = Path::GetDirectory(prc);
			SafePointer<module> root = new module;

			// TODO: IMPLEMENT

			// // application build set
			// application app;
			// module * root_module;
			
			state.modules.Append(root->identifier, root);
			state.root_module = root;
			return true;
		}
		bool make_project_dependencies(build_state & state)
		{

			// TODO: IMPLEMENT

			// // common paths
			// Engine::Volumes::List<string> include_list, data_files_list;
			// // application build set
			// Engine::Volumes::ObjectDictionary<string, module> modules;
			// Engine::Volumes::ObjectDictionary<string, module> modules_to_build;
			// // current build state
			// string project_build_path, project_object_path, project_build_name;
			// string output_exec_path, output_bundle_path;
			// Engine::Volumes::List<compile_task> compile_list;
			// Engine::Volumes::List<resource_task> resource_list;
			// Engine::Volumes::List<attach_task> attach_list;
			// Engine::Volumes::Dictionary<string, string> defines_list;

			return true;
		}
		bool construct(build_state & state)
		{
			// TODO: IMPLEMENT
			return false;
		}
	}
}