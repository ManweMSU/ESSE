#include "kernel.h"

#include "esse_cc.h"
#include "esse_rsrc.h"
#include "esse_link.h"

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
		string read_localized_string(const localized_string & str, const string & locale)
		{
			auto var = str[locale];
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
		string escape_string_c(const string & input)
		{
			int ucslen = input.GetEncodedLength(Encoding::UTF32);
			Array<uint32> ucs(0x100);
			ucs.SetLength(ucslen);
			input.Encode(ucs, Encoding::UTF32, false);
			DynamicString result;
			for (auto & c : ucs) {
				if (c < 0x20 || c > 0x7F || c == L'\\' || c == L'\"') {
					if (c >= 0x10000) {
						result += L"\\U" + string(c, L"0123456789ABCDEF", 8);
					} else if (c > 0x7F) {
						result += L"\\u" + string(c, L"0123456789ABCDEF", 4);
					} else if (c == L'\\') {
						result += L"\\\\";
					} else if (c == L'\"') {
						result += L"\\\"";
					} else if (c == L'\n') {
						result += L"\\n";
					} else if (c == L'\r') {
						result += L"\\r";
					} else if (c == L'\t') {
						result += L"\\t";
					} else {
						result += L"\\" + string(c, L"01234567", 3);
					}
				} else {
					result += widechar(c);
				}
			}
			return result.ToString();
		}
		void command_line_append(Engine::Array<string> & cmd, const string & pattern, const string & value)
		{
			if (pattern.FindFirst(L'$') >= 0) cmd.Append(pattern.Replace(L'$', value)); else {
				cmd.Append(pattern);
				cmd.Append(value);
			}
		}
		void make_define_from_value(Volumes::Dictionary<string, string> & dest, RegistryNode * node, const string & value)
		{
			auto type = node->GetValueType(value);
			if (type == RegistryValueType::Boolean) dest.Append(value, node->GetValueBoolean(value) ? L"1" : L"0");
			else if (type == RegistryValueType::Integer) dest.Append(value, node->GetValueInteger(value));
			else if (type == RegistryValueType::String) dest.Append(value, L"U\"" + escape_string_c(node->GetValueString(value)) + L"\"");
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
			SafePointer<RegistryNode> node = state.build_set_common_configuration->OpenNode(L"Definitiones");
			if (node) for (auto & v : node->GetValues()) make_define_from_value(state.defines_list, node, v);
			node = state.build_set_common_configuration->OpenNode(L"Instrumenta");
			if (node) for (auto & n : node->GetSubnodes()) {
				SafePointer<RegistryNode> tool = node->OpenNode(n);
				if (tool) {
					auto type = tool->GetValueString(L"Stadium").LowerCase();
					if (type.Fragment(0, 10) == L"compilator") {
						SafePointer<build_tool> compiler;
						if (type == L"compilator/cc") {
							compiler = new ccxx_compiler(tool);
							if (!state.cc_tool) state.cc_tool = compiler;
						}
						if (compiler) {
							Array<string> formats(0x10);
							compiler->enumerate_extensions(formats);
							for (auto & f : formats) {
								state.compilers.Append(f, compiler);
								if (!state.io->silent_mode && state.io->verbose_level >= 3) {
									state.io->console->SetTextColor(ConsoleColor::Cyan);
									state.io->console->WriteLine(L"Compilator: *." + f + L" --> " + compiler->ToString());
									state.io->console->SetTextColor(ConsoleColor::Default);
								}
							}
						}
					} else if (type == L"auxilia") {
						SafePointer<build_tool> restool = new resource_tool(tool);
						if (!state.resource_tool) state.resource_tool = restool;
					} else if (type == L"adhaesor") {
						SafePointer<build_tool> linktool = new linker(tool);
						if (!state.link_tool) state.link_tool = linktool;
					}
				}
			}
			if (!state.cc_tool) {
				if (!io.silent_mode) (*io.console) << TextColor(12) << io.localized(216) << TextColorDefault() << LineFeed();
				return false;
			}
			if (!state.resource_tool) {
				if (!io.silent_mode) (*io.console) << TextColor(12) << io.localized(217) << TextColorDefault() << LineFeed();
				return false;
			}
			if (!state.link_tool) {
				if (!io.silent_mode) (*io.console) << TextColor(12) << io.localized(218) << TextColorDefault() << LineFeed();
				return false;
			}
			if (!state.io->silent_mode && state.io->verbose_level >= 3) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				state.io->console->WriteLine(L"Compilator primus: " + state.cc_tool->ToString());
				state.io->console->WriteLine(L"Processor auxiliorum: " + state.resource_tool->ToString());
				state.io->console->WriteLine(L"Adhaesor: " + state.link_tool->ToString());
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			return true;
		}
		bool project_post_configure(build_state & state)
		{
			state.app.application_version_major = state.app.application_version_minor = 0;
			state.app.application_version_micro = state.app.application_build_number = 0;
			state.root_module->version = state.app.application_version;
			auto vparts = state.app.application_version.Split(L'.');
			if (vparts.Length() > 0) try { state.app.application_version_major = vparts[0].ToUInt32(); } catch (...) {}
			if (vparts.Length() > 1) try { state.app.application_version_minor = vparts[1].ToUInt32(); } catch (...) {}
			if (vparts.Length() > 2) try { state.app.application_version_micro = vparts[2].ToUInt32(); } catch (...) {}
			if (vparts.Length() > 3) try { state.app.application_build_number = vparts[3].ToUInt32(); } catch (...) {}
			if (!state.app.application_identifier.Length()) {
				state.app.application_identifier = Path::GetFileNameWithoutExtension(state.project_file_path).LowerCase();
			}
			state.root_module->name = state.app.application_identifier;
			if (!state.app.author_identifier.Length()) {
				state.app.author_identifier = L"anonymous";
			}
			state.root_module->author = state.app.author_identifier;
			if (!state.app.name.Length()) {
				state.app.name = Path::GetFileNameWithoutExtension(state.project_file_path);
			}
			if (!state.app.application_internal_name.Length()) {
				state.app.application_internal_name = Path::GetFileNameWithoutExtension(state.project_file_path).LowerCase();
			}
			if (!state.app.build_path.Length()) {
				state.app.build_path = FormatString(L"_build/%0_%1_%2", state.project_system->identifier.LowerCase(),
					state.project_architecture->identifier.LowerCase(), state.project_configuration->identifier.LowerCase());
			}
			state.app.subsystem = state.project_subsystem->identifier;
			if (state.app.application_name.IsEmpty()) {
				state.app.application_name.Append(L"", state.app.name);
			}
			if (state.app.needs_elevation) {
				state.defines_list.Append(L"ESSE_ELEVATIO_REQUISITA", 1);
			}
			if (state.app.make_version_defines) {
				state.defines_list.Append(L"ESSE_META_NOMEN_APPLICATIONIS", L"U\"" + escape_string_c(read_localized_string(state.app.application_name, L"")) + L"\"");
				state.defines_list.Append(L"ESSE_META_AUTHOR_APPLICATIONIS", L"U\"" + escape_string_c(read_localized_string(state.app.application_author, L"")) + L"\"");
				state.defines_list.Append(L"ESSE_META_JURA_EXEMPLI", L"U\"" + escape_string_c(read_localized_string(state.app.application_copyright, L"")) + L"\"");
				state.defines_list.Append(L"ESSE_META_DESCRIPTIO", L"U\"" + escape_string_c(read_localized_string(state.app.application_description, L"")) + L"\"");
				state.defines_list.Append(L"ESSE_META_VERSIO_APPLICATIONIS", L"U\"" + escape_string_c(state.app.application_version) + L"\"");
				state.defines_list.Append(L"ESSE_META_VERSIO_MAJOR", state.app.application_version_major);
				state.defines_list.Append(L"ESSE_META_VERSIO_MINOR", state.app.application_version_minor);
				state.defines_list.Append(L"ESSE_META_VERSIO_MICRO", state.app.application_version_micro);
				state.defines_list.Append(L"ESSE_META_VERSIO_ITERATIO", state.app.application_build_number);
				state.defines_list.Append(L"ESSE_META_VERSIO_BREVIS", L"U\"" + string(state.app.application_version_major) + L"." + string(state.app.application_version_minor) + L"\"");
				state.defines_list.Append(L"ESSE_META_NOMEN_INTERNUM", L"U\"" + escape_string_c(state.app.application_internal_name) + L"\"");
				state.defines_list.Append(L"ESSE_META_INDENTITAS_APPLICATIONIS", L"U\"" + escape_string_c(state.app.application_identifier) + L"\"");
				state.defines_list.Append(L"ESSE_META_INDENTITAS_AUTHORIS", L"U\"" + escape_string_c(state.app.author_identifier) + L"\"");
			}
			return true;
		}
		string find_resource_file(const build_state & state, const string & current_root, const string & name)
		{
			if (FileExists(current_root + L"/" + name)) return ExpandPath(current_root + L"/" + name);
			for (auto & d : state.data_files_list) if (FileExists(d + L"/" + name)) return ExpandPath(d + L"/" + name);
			return ExpandPath(current_root + L"/" + name);
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
			root->type = module_class::common_root;
			root->manifest_alternation_date = state.project_alternation_date;
			root->is_default_abstraction_layer_implementation = false;
			SafePointer<RegistryNode> node = state.project_common_configuration->OpenNode(L"Compila");
			if (node) for (auto & v : node->GetValues()) {
				auto value = node->GetValueString(v);
				auto del = value.FindFirst(L':');
				compile_task task;
				if (del >= 0) {
					task.source_path = ExpandPath(state.project_root_path + L"/" + value.Fragment(0, del));
					task.option = value.Fragment(del + 1, -1);
				} else task.source_path = ExpandPath(state.project_root_path + L"/" + value);
				root->compile_list.InsertLast(task);
			}
			node = state.project_common_configuration->OpenNode(L"Moduli");
			if (node) for (auto & v : node->GetValues()) {
				auto value = node->GetValueString(v);
				root->needs_modules.AddElement(value.LowerCase());
			}
			node = state.project_common_configuration->OpenNode(L"Auxilia");
			if (node) for (auto & v : node->GetSubnodes()) {
				auto rsrc = node->OpenNode(v);
				if (rsrc) {
					resource_task task;
					task.source_path = find_resource_file(state, state.project_root_path, rsrc->GetValueString(L"Lima"));
					task.resource_locale = rsrc->GetValueString(L"Lingua").LowerCase();
					task.resource_name = rsrc->GetValueString(L"Nomen");
					root->resource_list.InsertLast(task);
				}
			}
			node = state.project_common_configuration->OpenNode(L"Addenda");
			if (node) for (auto & v : node->GetSubnodes()) {
				auto attach = node->OpenNode(v);
				if (attach) {
					attach_task task;
					task.source_path = find_resource_file(state, state.project_root_path, attach->GetValueString(L"Lima"));
					task.destination_path = attach->GetValueString(L"Destinatio");
					root->attach_list.InsertLast(task);
				}
			}
			auto locales = state.project_common_configuration->GetValueString(L"Meta/Linguae").Split(L',');
			for (auto & l : locales) state.app.localizations.InsertLast(l);
			state.app.name = state.project_common_configuration->GetValueString(L"Nomen");
			state.app.build_path = state.project_common_configuration->GetValueString(L"Destinatio");
			state.app.application_identifier = state.project_common_configuration->GetValueString(L"IndentitasApplicationis");
			state.app.author_identifier = state.project_common_configuration->GetValueString(L"IndentitasCreatoris");
			state.app.application_icon = find_resource_file(state, state.project_root_path, state.project_common_configuration->GetValueString(L"Meta/Icon"));
			state.app.application_internal_name = state.project_common_configuration->GetValueString(L"Meta/NomenInternum");
			state.app.application_version = state.project_common_configuration->GetValueString(L"Meta/VersioApplicationis");
			state.app.application_name = load_localized_string(state.project_common_configuration, L"Meta/NomenApplicationis");
			state.app.application_author = load_localized_string(state.project_common_configuration, L"Meta/CreatorApplicationis");
			state.app.application_copyright = load_localized_string(state.project_common_configuration, L"Meta/JuraExempli");
			state.app.application_description = load_localized_string(state.project_common_configuration, L"Meta/Descriptio");
			state.app.needs_camera_reason = load_localized_string(state.project_common_configuration, L"Permissiones/Camera");
			state.app.needs_microphone_reson = load_localized_string(state.project_common_configuration, L"Permissiones/Microphonus");
			state.app.no_high_dpi_scale = state.project_common_configuration->GetValueBoolean(L"ConscendeNulle");
			state.app.no_dock_icon = state.project_common_configuration->GetValueBoolean(L"DockNullum");
			state.app.make_version_defines = state.project_common_configuration->GetValueBoolean(L"CreaMetaDefinitiones");
			state.app.needs_elevation = state.project_common_configuration->GetValueBoolean(L"Permissiones/Eleva");
			node = state.project_common_configuration->OpenNode(L"Formati");
			if (node) for (auto & v : node->GetSubnodes()) {
				auto shell = node->OpenNode(v);
				if (shell) {
					auto role = shell->GetValueString(L"Functio");
					file_format ff;
					ff.file_format_extension = shell->GetValueString(L"Extensio");
					ff.file_format_description = load_localized_string(shell, L"Descriptio");
					ff.file_format_icon = find_resource_file(state, state.project_root_path, shell->GetValueString(L"Icon"));
					if (string::CompareIgnoreCase(role, L"creator") == 0) ff.role = file_format_role::editor;
					else ff.role = file_format_role::viewer;
					state.app.file_formats.InsertLast(ff);
				}
			}
			node = state.project_common_configuration->OpenNode(L"Schemae");
			if (node) for (auto & v : node->GetSubnodes()) {
				auto shell = node->OpenNode(v);
				if (shell) {
					uri_scheme uri;
					uri.uri_prefix = shell->GetValueString(L"Protocol");
					uri.uri_scheme_description = load_localized_string(shell, L"Descriptio");
					state.app.uri_schemes.InsertLast(uri);
				}
			}
			node = state.project_common_configuration->OpenNode(L"Imperata");
			if (node) for (auto & v : node->GetValues()) {
				auto value = node->GetValueString(v);
				state.app.command_line_tools.InsertLast(value);
			}
			state.app.store_database_integration_file = state.project_common_configuration->GetValueString(L"Entheca/Configuratio");
			state.app.store_persistent_files = state.project_common_configuration->GetValueString(L"Entheca/Perstata");
			node = state.project_common_configuration->OpenNode(L"Entheca/Extensiones");
			if (node) for (auto & v : node->GetSubnodes()) {
				auto ext = node->OpenNode(v);
				if (ext) {
					store_extension e;
					e.name = load_localized_string(ext, L"Nomen");
					e.target_product_identifier = ext->GetValueString(L"Ad");
					e.extension_file = ext->GetValueString(L"Lima");
					state.app.store_extensions.InsertLast(e);
				}
			}
			state.modules.Append(root->identifier, root);
			state.root_module = root;
			if (!project_post_configure(state)) return false;
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
			root->type = module_class::common_root;
			root->manifest_alternation_date = state.project_alternation_date;
			root->needs_modules.AddElement(L"engineruntime");
			root->is_default_abstraction_layer_implementation = false;
			if (state.project_common_configuration->GetValueBoolean(L"CompileAll")) {
				SafePointer< Array<string> > files = Search::GetFiles(state.project_root_path + L"/*.c;*.cpp;*.cxx");
				for (auto & f : *files) {
					compile_task task;
					task.source_path = ExpandPath(state.project_root_path + L"/" + f);
					root->compile_list.InsertLast(task);
				}
			} else {
				SafePointer<RegistryNode> comlist = state.project_common_configuration->OpenNode(L"CompileList");
				if (comlist) for (auto & v : comlist->GetValues()) {
					auto value = comlist->GetValueString(v);
					auto del = value.FindFirst(L':');
					compile_task task;
					if (del >= 0) {
						task.source_path = ExpandPath(state.project_root_path + L"/" + value.Fragment(0, del));
						task.option = value.Fragment(del + 1, -1);
					} else task.source_path = ExpandPath(state.project_root_path + L"/" + value);
					root->compile_list.InsertLast(task);
				}
			}
			SafePointer<RegistryNode> node = state.project_common_configuration->OpenNode(L"Resources");
			if (node) {
				for (auto & v : node->GetValues()) {
					resource_task task;
					task.source_path = find_resource_file(state, state.project_root_path, node->GetValueString(v));
					task.resource_name = v;
					root->resource_list.InsertLast(task);
				}
				for (auto & l : node->GetSubnodes()) {
					auto locale = node->OpenNode(l);
					if (locale) for (auto & v : locale->GetValues()) {
						resource_task task;
						task.source_path = find_resource_file(state, state.project_root_path, locale->GetValueString(v));
						task.resource_locale = l.LowerCase();
						task.resource_name = v;
						root->resource_list.InsertLast(task);
					}
				}
			}
			node = state.project_common_configuration->OpenNode(L"Attachments");
			if (node) for (auto & v : node->GetSubnodes()) {
				auto attach = node->OpenNode(v);
				if (attach) {
					attach_task task;
					task.source_path = find_resource_file(state, state.project_root_path, attach->GetValueString(L"From"));
					task.destination_path = attach->GetValueString(L"To");
					root->attach_list.InsertLast(task);
				}
			}
			auto locales = state.project_common_configuration->GetValueString(L"Languages").Split(L',');
			for (auto & l : locales) state.app.localizations.InsertLast(l);
			state.app.name = state.project_common_configuration->GetValueString(L"OutputName");
			state.app.build_path = state.project_common_configuration->GetValueString(L"OutputLocation");
			state.app.application_identifier = state.project_common_configuration->GetValueString(L"VersionInformation/ApplicationIdentifier");
			state.app.author_identifier = state.project_common_configuration->GetValueString(L"VersionInformation/CompanyIdentifier");
			state.app.application_icon = find_resource_file(state, state.project_root_path, state.project_common_configuration->GetValueString(L"ApplicationIcon"));
			state.app.application_internal_name = state.project_common_configuration->GetValueString(L"VersionInformation/InternalName");
			state.app.application_version = state.project_common_configuration->GetValueString(L"VersionInformation/Version");
			state.app.application_name = load_localized_string(state.project_common_configuration, L"VersionInformation/ApplicationName");
			state.app.application_author = load_localized_string(state.project_common_configuration, L"VersionInformation/CompanyName");
			state.app.application_copyright = load_localized_string(state.project_common_configuration, L"VersionInformation/Copyright");
			state.app.application_description = load_localized_string(state.project_common_configuration, L"VersionInformation/Description");
			state.app.needs_camera_reason = load_localized_string(state.project_common_configuration, L"AccessRequirements/Camera");
			state.app.needs_microphone_reson = load_localized_string(state.project_common_configuration, L"AccessRequirements/Microphone");
			state.app.no_high_dpi_scale = state.project_common_configuration->GetValueBoolean(L"NoHiDPI");
			state.app.no_dock_icon = state.project_common_configuration->GetValueBoolean(L"NoDockIcon");
			state.app.make_version_defines = state.project_common_configuration->GetValueBoolean(L"UseVersionDefines");
			state.app.needs_elevation = state.project_common_configuration->GetValueBoolean(L"NeedsElevation");
			node = state.project_common_configuration->OpenNode(L"FileFormats");
			if (node) for (auto & v : node->GetSubnodes()) {
				auto shell = node->OpenNode(v);
				if (shell) {
					if (shell->GetValueString(L"Extension").Length()) {
						file_format ff;
						ff.file_format_extension = shell->GetValueString(L"Extension");
						ff.file_format_description = load_localized_string(shell, L"Description");
						ff.file_format_icon = find_resource_file(state, state.project_root_path, shell->GetValueString(L"Icon"));
						ff.role = shell->GetValueString(L"CanCreate") ? file_format_role::editor : file_format_role::viewer;
						state.app.file_formats.InsertLast(ff);
					} else if (shell->GetValueString(L"Protocol").Length()) {
						uri_scheme uri;
						uri.uri_prefix = shell->GetValueString(L"Protocol");
						uri.uri_scheme_description = load_localized_string(shell, L"Description");
						state.app.uri_schemes.InsertLast(uri);
					}
				}
			}
			state.modules.Append(root->identifier, root);
			state.root_module = root;
			if (!project_post_configure(state)) return false;
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
			compile_task compile_main;
			compile_main.source_path = prc;
			compile_main.option = L"";
			SafePointer<module> root = new module;
			root->type = module_class::common_root;
			root->manifest_alternation_date = state.project_alternation_date = 0;
			root->needs_modules.AddElement(L"engineruntime");
			root->is_default_abstraction_layer_implementation = false;
			root->compile_list.InsertLast(compile_main);
			root->name = state.app.name = Path::GetFileNameWithoutExtension(prc);
			state.app.subsystem = state.project_subsystem->identifier;
			state.app.no_high_dpi_scale = false;
			state.app.no_dock_icon = false;
			state.app.make_version_defines = false;
			state.app.needs_elevation = false;
			state.modules.Append(root->identifier, root);
			state.root_module = root;
			if (!project_post_configure(state)) return false;
			return true;
		}
		bool load_module(build_state & state, module * mdl, const string & from)
		{
			if (!state.io->silent_mode && state.io->verbose_level >= 2) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				if (from.Length()) state.io->console->WriteLine(FormatString(L"Modulus '%0' ab '%1' requisitus est.", mdl->identifier, from));
				else if (mdl->identifier.Length()) state.io->console->WriteLine(FormatString(L"Modulus '%0' ab projecto requisitus est.", mdl->identifier));
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			state.modules_to_build.Append(mdl->identifier.LowerCase(), mdl);
			for (auto & m : mdl->needs_modules) {
				if (state.modules_to_build[m]) continue;
				auto mdlreq = state.modules[m];
				if (!mdlreq) {
					if (!state.io->silent_mode) (*state.io->console) << TextColor(12) << FormatString(state.io->localized(219), m) << TextColorDefault() << LineFeed();
					return false;
				}
				if (mdlreq->type == module_class::abstraction_layer_implementation) if (!load_module(state, mdlreq, mdl->identifier)) return false;
			}
			for (auto & m : mdl->needs_modules) {
				if (state.modules_to_build[m]) continue;
				auto mdlreq = state.modules[m];
				if (!mdlreq) {
					if (!state.io->silent_mode) (*state.io->console) << TextColor(12) << FormatString(state.io->localized(219), m) << TextColorDefault() << LineFeed();
					return false;
				}
				if (mdlreq->type != module_class::abstraction_layer_implementation) if (!load_module(state, mdlreq, mdl->identifier)) return false;
			}
			if (mdl->implements_abstraction_layer.Length()) state.abstraction_layers.AddElement(mdl->implements_abstraction_layer.LowerCase());
			if (mdl->needs_abstraction_layer_implementation.Length()) {
				if (!state.io->silent_mode && state.io->verbose_level >= 3) {
					state.io->console->SetTextColor(ConsoleColor::Cyan);
					state.io->console->WriteLine(FormatString(L"Planus abstractionis '%0' requisitus est.", mdl->needs_abstraction_layer_implementation));
					state.io->console->SetTextColor(ConsoleColor::Default);
				}
				if (!state.abstraction_layers[mdl->needs_abstraction_layer_implementation.LowerCase()]) {
					ObjectArray<module> variants(0x10);
					for (auto & m : state.modules) if (string::CompareIgnoreCase(m.value->implements_abstraction_layer, mdl->needs_abstraction_layer_implementation) == 0) {
						auto mt = m.value;
						if (!mt->systems.IsEmpty() && !mt->systems[state.project_system->identifier.LowerCase()]) {
							if (!state.io->silent_mode && state.io->verbose_level >= 3) {
								state.io->console->SetTextColor(ConsoleColor::Cyan);
								state.io->console->WriteLine(FormatString(L"  '%0': systema requisita de '%1' falsa est.", mdl->needs_abstraction_layer_implementation, mt->identifier));
								state.io->console->SetTextColor(ConsoleColor::Default);
							}
							continue;
						}
						if (!mt->subsystems.IsEmpty() && !mt->subsystems[state.project_subsystem->identifier.LowerCase()]) {
							if (!state.io->silent_mode && state.io->verbose_level >= 3) {
								state.io->console->SetTextColor(ConsoleColor::Cyan);
								state.io->console->WriteLine(FormatString(L"  '%0': subsystema requisita de '%1' falsa est.", mdl->needs_abstraction_layer_implementation, mt->identifier));
								state.io->console->SetTextColor(ConsoleColor::Default);
							}
							continue;
						}
						if (!state.io->silent_mode && state.io->verbose_level >= 3) {
							state.io->console->SetTextColor(ConsoleColor::Cyan);
							state.io->console->WriteLine(FormatString(L"  '%0': variatus '%1'.", mdl->needs_abstraction_layer_implementation, mt->identifier));
							state.io->console->SetTextColor(ConsoleColor::Default);
						}
						variants.Append(m.value);
					}
					if (!state.io->silent_mode && state.io->verbose_level >= 3) {
						state.io->console->SetTextColor(ConsoleColor::Cyan);
						state.io->console->WriteLine(FormatString(L"Variati pro '%0':", mdl->needs_abstraction_layer_implementation));
						for (auto & v : variants) state.io->console->WriteLine(L"  " + v.identifier);
						state.io->console->SetTextColor(ConsoleColor::Default);
					}
					module * load = 0;
					for (auto & v : variants) if (v.is_default_abstraction_layer_implementation) load = &v;
					if (!load && variants.Length()) load = &variants[0];
					if (!load) {
						if (!state.io->silent_mode) (*state.io->console) << TextColor(12) << FormatString(state.io->localized(220), mdl->needs_abstraction_layer_implementation) << TextColorDefault() << LineFeed();
						return false;
					}
					if (!state.modules_to_build[load->identifier.LowerCase()]) if (!load_module(state, load, mdl->identifier)) return false;
				}
			}
			if (mdl->needs_codecs.Length()) {
				if (!state.io->silent_mode && state.io->verbose_level >= 3) {
					state.io->console->SetTextColor(ConsoleColor::Cyan);
					state.io->console->WriteLine(FormatString(L"Systema codificatorum '%0' requisitus est.", mdl->needs_codecs));
					state.io->console->SetTextColor(ConsoleColor::Default);
				}
				for (auto & m : state.modules) if (m.value->implements_codecs[mdl->needs_codecs]) {
					auto mt = m.value;
					if (!mt->systems.IsEmpty() && !mt->systems[state.project_system->identifier.LowerCase()]) {
						if (!state.io->silent_mode && state.io->verbose_level >= 3) {
							state.io->console->SetTextColor(ConsoleColor::Cyan);
							state.io->console->WriteLine(FormatString(L"  '%0': systema requisita de '%1' falsa est.", mdl->needs_abstraction_layer_implementation, mt->identifier));
							state.io->console->SetTextColor(ConsoleColor::Default);
						}
						continue;
					}
					if (!mt->subsystems.IsEmpty() && !mt->subsystems[state.project_subsystem->identifier.LowerCase()]) {
						if (!state.io->silent_mode && state.io->verbose_level >= 3) {
							state.io->console->SetTextColor(ConsoleColor::Cyan);
							state.io->console->WriteLine(FormatString(L"  '%0': subsystema requisita de '%1' falsa est.", mdl->needs_abstraction_layer_implementation, mt->identifier));
							state.io->console->SetTextColor(ConsoleColor::Default);
						}
						continue;
					}
					if (!state.io->silent_mode && state.io->verbose_level >= 3) {
						state.io->console->SetTextColor(ConsoleColor::Cyan);
						state.io->console->WriteLine(FormatString(L"  '%0': codificator '%1'.", mdl->needs_abstraction_layer_implementation, mt->identifier));
						state.io->console->SetTextColor(ConsoleColor::Default);
					}
					if (!state.modules_to_build[mt->identifier.LowerCase()]) if (!load_module(state, mt, mdl->identifier)) return false;
				}
			}
			for (auto & cdx : mdl->implements_codecs) {
				if (state.codec_domains.ElementExists(cdx.key)) {
					state.codec_domains[cdx.key]->AddElement(cdx.value);
				} else {
					SafePointer< Volumes::Set<string> > domain = new Volumes::Set<string>;
					domain->AddElement(cdx.value);
					state.codec_domains.Append(cdx.key, domain);
				}
			}
			return true;
		}
		bool make_project_dependencies(build_state & state)
		{
			for (auto & mp : state.module_search_list) {
				SafePointer< Array<string> > subpaths = Search::GetDirectories(mp + L"/*");
				for (auto & sp : *subpaths) if (sp[0] != L'.') {
					auto prob = ExpandPath(mp + L"/" + sp + L"/" + sp + L".esse");
					SafePointer<Registry> mconf;
					Time mtime;
					try { mconf = load_configuration(prob, &mtime); } catch (...) {
						if (!state.io->silent_mode && state.io->verbose_level >= 4) {
							state.io->console->SetTextColor(ConsoleColor::Cyan);
							state.io->console->WriteLine(L"Lima \"" + prob + L"\" tabulatio non est.");
							state.io->console->SetTextColor(ConsoleColor::Default);
						}
						continue;
					}
					auto mclass_value = mconf->GetValueString(L"CoClassis");
					module_class mclass;
					if (string::CompareIgnoreCase(mclass_value, L"Communalis") == 0) mclass = module_class::common;
					else if (string::CompareIgnoreCase(mclass_value, L"ImplantatioAbstractionis") == 0) mclass = module_class::abstraction_layer;
					else if (string::CompareIgnoreCase(mclass_value, L"Specialis") == 0) mclass = module_class::abstraction_layer_implementation;
					else if (string::CompareIgnoreCase(mclass_value, L"Codex") == 0) mclass = module_class::codec;
					else {
						if (!state.io->silent_mode && state.io->verbose_level >= 4) {
							state.io->console->SetTextColor(ConsoleColor::Cyan);
							state.io->console->WriteLine(L"Tabulatio \"" + prob + L"\" manifestum moduli non est.");
							state.io->console->SetTextColor(ConsoleColor::Default);
						}
						continue;
					}
					if (!state.io->silent_mode && state.io->verbose_level >= 4) {
						state.io->console->SetTextColor(ConsoleColor::Cyan);
						state.io->console->WriteLine(L"Manifestum moduli in \"" + prob + L"\".");
						state.io->console->SetTextColor(ConsoleColor::Default);
					}
					auto module_root = Path::GetDirectory(prob);
					SafePointer<module> mdl = new module;
					mdl->type = mclass;
					mdl->manifest_alternation_date = mtime;
					mdl->name = mconf->GetValueString(L"CoNomen");
					mdl->author = mconf->GetValueString(L"CoAuthor");
					mdl->identifier = mconf->GetValueString(L"CoIndentitas");
					mdl->version = mconf->GetValueString(L"CoVersio");
					SafePointer<RegistryNode> node = mconf->OpenNode(L"CoSystema");
					if (node) for (auto & v : node->GetValues()) mdl->systems.AddElement(node->GetValueString(v).LowerCase());
					node = mconf->OpenNode(L"CoSubsystema");
					if (node) for (auto & v : node->GetValues()) mdl->subsystems.AddElement(node->GetValueString(v).LowerCase());
					node = mconf->OpenNode(L"CoModuliRequisiti");
					if (node) for (auto & v : node->GetValues()) mdl->needs_modules.AddElement(node->GetValueString(v).LowerCase());
					mdl->needs_abstraction_layer_implementation = mconf->GetValueString(L"CoPlanusAbstractionis");
					mdl->implements_abstraction_layer = mconf->GetValueString(L"CoImplantatioAbstractionis");
					mdl->needs_codecs = mconf->GetValueString(L"CoCodicesUtitur").LowerCase();
					mdl->is_default_abstraction_layer_implementation = mconf->GetValueBoolean(L"CoImplantatioDefalta");
					node = mconf->OpenNode(L"CoCodicemDefinit");
					if (node) for (auto & v : node->GetValues()) {
						auto p = node->GetValueString(v).Split(L':');
						if (p.Length() == 2) mdl->implements_codecs.Append(p[0].LowerCase(), p[1]);
					}
					node = mconf->OpenNode(L"CoCompila");
					if (node) for (auto & v : node->GetValues()) {
						auto value = node->GetValueString(v);
						auto del = value.FindFirst(L':');
						compile_task task;
						if (del >= 0) {
							task.from_module = mdl->identifier;
							task.source_path = ExpandPath(module_root + L"/" + value.Fragment(0, del));
							task.option = value.Fragment(del + 1, -1);
						} else {
							task.from_module = mdl->identifier;
							task.source_path = ExpandPath(module_root + L"/" + value);
						}
						mdl->compile_list.InsertLast(task);
					}
					node = mconf->OpenNode(L"CoAuxilia");
					if (node) for (auto & v : node->GetSubnodes()) {
						auto rsrc = node->OpenNode(v);
						if (rsrc) {
							resource_task task;
							task.source_path = find_resource_file(state, module_root, rsrc->GetValueString(L"Lima"));
							task.resource_locale = rsrc->GetValueString(L"Lingua").LowerCase();
							task.resource_name = rsrc->GetValueString(L"Nomen");
							mdl->resource_list.InsertLast(task);
						}
					}
					node = mconf->OpenNode(L"CoAddenda");
					if (node) for (auto & v : node->GetSubnodes()) {
						auto attach = node->OpenNode(v);
						if (attach) {
							attach_task task;
							task.source_path = find_resource_file(state, module_root, attach->GetValueString(L"Lima"));
							task.destination_path = attach->GetValueString(L"Destinatio");
							mdl->attach_list.InsertLast(task);
						}
					}
					node = mconf->OpenNode(L"CoTituli");
					if (node) for (auto & v : node->GetValues()) {
						auto value = node->GetValueString(v);
						mdl->include_list.InsertLast(ExpandPath(module_root + L"/" + value));
					}
					node = mconf->OpenNode(L"CoDefini");
					if (node) for (auto & v : node->GetValues()) make_define_from_value(mdl->defines_list, node, v);
					state.modules.Append(mdl->identifier.LowerCase(), mdl);
					if (!state.io->silent_mode && state.io->verbose_level >= 3) {
						state.io->console->SetTextColor(ConsoleColor::Cyan);
						state.io->console->WriteLine(FormatString(L"Modulus novus %0 '%3' ab %1, versio %2.", mdl->name, mdl->author, mdl->version, mdl->identifier));
						state.io->console->SetTextColor(ConsoleColor::Default);
					}
				}
			}
			for (auto & p : state.module_search_list) state.include_list.InsertLast(p);
			if (!load_module(state, state.root_module, L"")) return false;
			if (!state.io->silent_mode && state.io->verbose_level >= 1) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				state.io->console->WriteLine(L"MODULI REQUISITI AB PROJECTO:");
				for (auto & m : state.modules_to_build) if (m.value->identifier.Length()) {
					auto md = m.value.Inner();
					state.io->console->WriteLine(FormatString(L"  %0 '%3' ab %1, versio %2.", md->name, md->author, md->version, md->identifier));
				}
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			for (auto & m : state.modules_to_build) {
				auto & mdl = *m.value;
				for (auto & t : mdl.compile_list) state.compile_list.InsertLast(t);
				for (auto & t : mdl.resource_list) state.resource_list.InsertLast(t);
				for (auto & t : mdl.attach_list) state.attach_list.InsertLast(t);
				for (auto & t : mdl.include_list) state.include_list.InsertLast(t);
				for (auto & t : mdl.defines_list) state.defines_list.Append(t.key, t.value);
				if (mdl.manifest_alternation_date > state.project_alternation_date) state.project_alternation_date = mdl.manifest_alternation_date;
			}
			state.project_build_path = ExpandPath(state.project_root_path + L"/" + state.app.build_path);
			state.project_object_path = ExpandPath(state.project_build_path + L"/_obj");
			state.project_build_name = state.app.name;
			if (!state.io->silent_mode && state.io->verbose_level >= 3) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				state.io->console->WriteLine(L"COLLECTORIUM DESTINATIONIS: " + state.project_build_path);
				state.io->console->WriteLine(L"COLLECTORIUM INTERSTADII: " + state.project_object_path);
				state.io->console->WriteLine(L"NOMEN DESTINATIONIS: " + state.project_build_name);
				state.io->console->WriteLine(L"AD COMPILATIONE:");
				for (auto & c : state.compile_list) {
					if (c.option.Length()) state.io->console->WriteLine(L"  " + c.source_path + L":" + c.option);
					else state.io->console->WriteLine(L"  " + c.source_path);
				}
				state.io->console->WriteLine(L"SEMITAE REPERIENDI TITULORUM:");
				for (auto & i : state.include_list) state.io->console->WriteLine(L"  " + i);
				for (auto & d : state.defines_list) state.io->console->WriteLine(L"#define " + d.key + L" " + d.value);
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			return true;
		}
		
		struct process_context_iml : process_context
		{
			Semaphore * sync;
			build_tool_status build_status;
			Engine::Time time1, time2;
			string data;

			process_context_iml(Semaphore * sem) : sync(sem), time1(0), time2(0), build_status(build_tool_status::failed) {}
			virtual void build_status_notify(const string & input, build_tool_status status, Engine::Time t1, Engine::Time t2, const string & output) override
			{
				build_status = status;
				time1 = t1;
				time2 = t2;
				data = output;
			}
			virtual void enter_state_critical_section(void) noexcept override { sync->Wait(); }
			virtual void leave_state_critical_section(void) noexcept override { sync->Open(); }
		};
		void print_tool_status(build_state * pstate, process_context_iml & context, const string & desc, const string & file)
		{
			if (!pstate->io->silent_mode && !pstate->idle_mode) {
				if (context.build_status != build_tool_status::skipped) {
					context.sync->Wait();
					auto & io = *pstate->io;
					io.console->Write(desc);
					if (file.Length()) {
						io.console->SetTextColor(ConsoleColor::Cyan);
						io.console->Write(L" " + Path::GetFileName(file));
						io.console->SetTextColor(ConsoleColor::Default);
					}
					io.console->Write(L"...");
					if (context.build_status == build_tool_status::failed) {
						io.console->SetTextColor(ConsoleColor::Red);
						io.console->WriteLine(io.localized(316));
						io.console->SetTextColor(ConsoleColor::Default);
						if (context.data.Length()) io.console->WriteLine(context.data);
					} else if (context.build_status == build_tool_status::built_new) {
						io.console->SetTextColor(ConsoleColor::Green);
						io.console->WriteLine(io.localized(314));
						io.console->SetTextColor(ConsoleColor::Default);
					} else if (context.build_status == build_tool_status::built_renew) {
						io.console->SetTextColor(ConsoleColor::Green);
						io.console->Write(io.localized(314));
						io.console->SetTextColor(ConsoleColor::Default);
						io.console->Write(io.localized(315));
						io.console->SetTextColor(ConsoleColor::Yellow);
						io.console->Write(context.time1.ToLocal().ToString());
						io.console->SetTextColor(ConsoleColor::Default);
						io.console->Write(io.localized(321));
						io.console->SetTextColor(ConsoleColor::Green);
						io.console->Write(context.time2.ToLocal().ToString());
						io.console->SetTextColor(ConsoleColor::Default);
						io.console->WriteLine(io.localized(322));
					}
					context.sync->Open();
				} else if (pstate->io->verbose_level >= 1) {
					context.sync->Wait();
					auto & io = *pstate->io;
					io.console->Write(desc);
					if (file.Length()) {
						io.console->SetTextColor(ConsoleColor::Cyan);
						io.console->Write(L" " + Path::GetFileName(file));
						io.console->SetTextColor(ConsoleColor::Default);
					}
					io.console->Write(L"...");
					io.console->SetTextColor(ConsoleColor::Yellow);
					io.console->WriteLine(io.localized(318));
					io.console->SetTextColor(ConsoleColor::Default);
					context.sync->Open();
				}
			}
		}
		void commit_compile_tasks(build_state & state, ThreadPool * pool, Semaphore * sync, volatile bool * cstat)
		{
			sync->Wait();
			while (!state.compile_list.IsEmpty()) try {
				auto task = state.compile_list.GetFirst()->GetValue();
				auto pstate = &state;
				state.compile_list.RemoveFirst();
				pool->SubmitTask(CreateFunctionalTask([pstate, pool, sync, cstat, task]() {
					process_context_iml context(sync);
					auto tool = pstate->compilers[Path::GetExtension(task.source_path).LowerCase()];
					if (tool) tool->process_file(task.source_path, task.from_module, task.option, pstate, context); else context.data = pstate->io->localized(317);
					print_tool_status(pstate, context, pstate->io->localized(313), task.source_path);
					if (context.build_status == build_tool_status::failed) *cstat = false;
					commit_compile_tasks(*pstate, pool, sync, cstat);
				}));
			} catch (...) { sync->Open(); throw; }
			sync->Open();
		}
		void perform_resource_task(build_state & state, Semaphore * sync, volatile bool * cstat)
		{
			process_context_iml context(sync);
			state.resource_tool->process_file(L"", L"", L"", &state, context);
			print_tool_status(&state, context, state.io->localized(319), L"");
			if (context.build_status == build_tool_status::failed) *cstat = false;
		}
		void perform_link_task(build_state & state, Semaphore * sync, volatile bool * cstat)
		{
			process_context_iml context(sync);
			state.link_tool->process_file(L"", L"", L"", &state, context);
			print_tool_status(&state, context, state.io->localized(320), state.output_exec_path);
			if (context.build_status == build_tool_status::failed) *cstat = false;
		}
		bool construct(build_state & state)
		{
			auto & io = *state.io;
			auto time_started = GetTimerValue();
			if (!io.silent_mode && !state.idle_mode) {
				io.console->Write(io.localized(307));
				io.console->SetTextColor(ConsoleColor::Yellow);
				io.console->Write(state.app.name);
				io.console->SetTextColor(ConsoleColor::Default);
				io.console->Write(io.localized(308));
				io.console->SetTextColor(ConsoleColor::Magenta);
				io.console->Write(read_localized_string(state.project_architecture->name));
				io.console->SetTextColor(ConsoleColor::Default);
				io.console->Write(io.localized(309));
				io.console->SetTextColor(ConsoleColor::Blue);
				io.console->Write(read_localized_string(state.project_system->name));
				io.console->SetTextColor(ConsoleColor::Default);
				io.console->Write(io.localized(310));
				io.console->SetTextColor(ConsoleColor::Cyan);
				io.console->Write(read_localized_string(state.project_configuration->name));
				io.console->SetTextColor(ConsoleColor::Default);
				io.console->WriteLine(io.localized(311));
			}
			try { CreateDirectoryTree(state.project_object_path); } catch (...) {
				if (!state.io->silent_mode) (*state.io->console) << TextColor(12) << state.io->localized(222) << TextColorDefault() << LineFeed();
				return false;
			}
			SafePointer<ThreadPool> pool = new ThreadPool;
			SafePointer<Semaphore> sync = CreateSemaphore(1);
			volatile bool alive = true;
			commit_compile_tasks(state, pool, sync, &alive);
			pool->Wait();
			if (!alive) return false;
			if (!state.io->silent_mode && state.io->verbose_level >= 3) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				state.io->console->WriteLine(L"AD AUXILIS:");
				for (auto & r : state.resource_list) {
					if (r.resource_locale.Length()) state.io->console->WriteLine(L"  " + r.source_path + L" --> " + r.resource_name + L"@" + r.resource_locale);
					else state.io->console->WriteLine(L"  " + r.source_path + L" --> " + r.resource_name);
				}
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			perform_resource_task(state, sync, &alive);
			if (!alive) return false;
			if (!state.io->silent_mode && state.io->verbose_level >= 3) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				state.io->console->WriteLine(L"NOMEN SARCINAE: " + state.output_bundle_path);
				state.io->console->WriteLine(L"NOMEN EXECUTI: " + state.output_exec_path);
				state.io->console->WriteLine(L"AD ADHAESIONE:");
				for (auto & l : state.link_list) {
					state.io->console->WriteLine(L"  " + l);
				}
				state.io->console->WriteLine(L"AD ADDENDIS:");
				for (auto & a : state.attach_list) state.io->console->WriteLine(L"  " + a.source_path + L" --> " + a.destination_path);
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			perform_link_task(state, sync, &alive);
			if (!alive) return false;
			if (!state.io->silent_mode && state.io->verbose_level >= 3) {
				state.io->console->SetTextColor(ConsoleColor::Cyan);
				state.io->console->WriteLine(L"NOMEN EXECUTI ULTIMUM: " + state.output_exec_path);
				state.io->console->SetTextColor(ConsoleColor::Default);
			}
			auto time_finished = GetTimerValue();
			if (!io.silent_mode && !state.idle_mode) {
				io.console->SetTextColor(ConsoleColor::Green);
				io.console->WriteLine(FormatString(io.localized(312), time_finished - time_started));
				io.console->SetTextColor(ConsoleColor::Default);
			}
			return true;
		}
	}
}