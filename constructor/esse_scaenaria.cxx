#include "esse_scaenaria.h"

using namespace Engine;
using namespace Engine::IO;
using namespace Engine::Streaming;

#define ERTBT_SCRIPT_COMMAND_MKDIR	L"mkdir"
#define ERTBT_SCRIPT_COMMAND_MV		L"mv"
#define ERTBT_SCRIPT_COMMAND_CP		L"cp"
#define ERTBT_SCRIPT_COMMAND_RM		L"rm"
#define ERTBT_SCRIPT_COMMAND_BUILD	L"compile"
#define ERTBT_SCRIPT_COMMAND_LINK	L"link"
#define ERTBT_SCRIPT_COMMAND_ATTACH	L"attachment"
#define ERTBT_SCRIPT_COMMAND_RSRC	L"resource"
#define ERTBT_SCRIPT_COMMAND_LINKA	L"linkarg"
#define ERTBT_SCRIPT_COMMAND_IFEQ	L"ifeq"
#define ERTBT_SCRIPT_COMMAND_IFNEQ	L"ifneq"
#define ERTBT_SCRIPT_COMMAND_GOTO	L"goto"
#define ERTBT_SCRIPT_COMMAND_EXIT	L"exit"
#define ERTBT_SCRIPT_COMMAND_SET	L"set"
#define ERTBT_SCRIPT_COMMAND_ALERT	L"alert"
#define ERTBT_SCRIPT_COMMAND_FAIL	L"fail"
#define ERTBT_SCRIPT_COMMAND_DEFINE	L"define"
#define ERTBT_SCRIPT_COMMAND_INC	L"include"
#define ERTBT_SCRIPT_VAR_PROJROOT	L"PROJROOT"
#define ERTBT_SCRIPT_VAR_OBJROOT	L"OBJROOT"
#define ERTBT_SCRIPT_VAR_EXROOT		L"EXROOT"
#define ERTBT_SCRIPT_VAR_ERTRSRC	L"ERTRSRC"
#define ERTBT_SCRIPT_VAR_ERTMDL		L"ERTMDL"
#define ERTBT_SCRIPT_VAR_ARCH		L"ARCH"
#define ERTBT_SCRIPT_VAR_OS			L"OS"
#define ERTBT_SCRIPT_VAR_SUBSYS		L"SUBSYS"
#define ERTBT_SCRIPT_VAR_CONF		L"CONFIG"

namespace esse {
	namespace constructor {
		Array<string> * ertbs_decompose_command(const string & command)
		{
			SafePointer< Array<string> > parts = new Array<string>(0x10);
			int sp = 0;
			while (sp < command.Length()) {
				while (sp < command.Length() && (command[sp] == L' ' || command[sp] == L'\t')) sp++;
				if (sp < command.Length()) {
					widechar bc = 0;
					if (command[sp] == L'\'' || command[sp] == L'\"') bc = command[sp];
					auto ep = sp;
					if (bc == 0) {
						while (ep < command.Length() && command[ep] != L' ' && command[ep] != L'\t') ep++;
						parts->Append(command.Fragment(sp, ep - sp));
						sp = ep;
					} else {
						ep++;
						while (ep < command.Length()) {
							if (command[ep] == bc) {
								if (command[ep + 1] == bc) {
									ep += 2;
								} else {
									ep++;
									widechar from[3] = { bc, bc, 0 };
									widechar to[2] = { bc, 0 };
									parts->Append(command.Fragment(sp + 1, ep - sp - 2).Replace(from, to));
									sp = ep;
									break;
								}
							} else ep++;
						}
					}
				}
			}
			parts->Retain();
			return parts;
		}
		string ertbs_substitute_variables(const string & text, const Volumes::Dictionary<string, string> & vars)
		{
			DynamicString result;
			int i = 0, length = text.Length();
			while (i < length) {
				if (text[i] == L'$') {
					i++;
					int j = i;
					while (i < length && text[i] != L'$') i++;
					if (i == j) { result << L'$'; } else {
						auto value = vars[text.Fragment(j, i - j).UpperCase()];
						if (value) result << *value;
					}
					i++;
				} else { result << text[i]; i++; }
			}
			return result.ToString();
		}

		script_compiler::script_compiler(Engine::Storage::RegistryNode * node) { sync = CreateSemaphore(1); if (!sync) throw OutOfMemoryException(); }
		void script_compiler::process_file(const string & input, const string & mdl, const string & option, build_state * state, process_context & context)
		{
			if (state->idle_mode) { context.build_status_notify(input, build_tool_status::skipped, 0, 0, L""); return; }
			Array<string> alerts(0x10);
			try {
				sync->Wait();
				SafePointer<Stream> stream = new FileStream(input, AccessRead, OpenExisting);
				SafePointer<TextReader> reader = new TextReader(stream, Encoding::UTF8);
				Array<string> commands(0x200);
				Volumes::Dictionary<string, string> variables;
				variables.Append(ERTBT_SCRIPT_VAR_PROJROOT, state->project_root_path);
				variables.Append(ERTBT_SCRIPT_VAR_OBJROOT, state->project_object_path);
				variables.Append(ERTBT_SCRIPT_VAR_EXROOT, state->project_build_path);
				variables.Append(ERTBT_SCRIPT_VAR_ARCH, state->project_architecture->identifier);
				variables.Append(ERTBT_SCRIPT_VAR_OS, state->project_system->identifier);
				variables.Append(ERTBT_SCRIPT_VAR_SUBSYS, state->project_subsystem->identifier);
				variables.Append(ERTBT_SCRIPT_VAR_CONF, state->project_configuration->identifier);
				while (!reader->EofReached()) commands << reader->ReadLine();
				int ip = 0;
				auto sroot = Path::GetDirectory(input);
				auto wd = GetCurrentDirectory();
				SetCurrentDirectory(sroot);
				while (ip < commands.Length()) {
					SafePointer< Array<string> > arguments = ertbs_decompose_command(commands[ip]);
					if (!arguments || !arguments->Length()) { ip++; continue; }
					for (auto & a : arguments->Elements()) a = ertbs_substitute_variables(a, variables);
					auto & command = arguments->FirstElement();
					if (!command.Length() || command[command.Length() - 1] == L':') {
						ip++; continue;
					} else if (command.Length() && command[0] == L'#') {
						ip++; continue;
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_MKDIR) == 0) {
						for (int i = 1; i < arguments->Length(); i++) IO::CreateDirectoryTree(arguments->ElementAt(i));
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_MV) == 0) {
						if (arguments->Length() > 2) IO::MoveFile(arguments->ElementAt(1), arguments->ElementAt(2));
						else throw InvalidArgumentException();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_CP) == 0) {
						if (arguments->Length() > 2) {
							SafePointer<Stream> from = new FileStream(arguments->ElementAt(1), AccessRead, OpenExisting);
							SafePointer<Stream> to = new FileStream(arguments->ElementAt(2), AccessWrite, CreateAlways);
							from->CopyTo(to);
						} else throw InvalidArgumentException();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_RM) == 0) {
						for (int i = 1; i < arguments->Length(); i++) {
							auto type = IO::GetFileType(arguments->ElementAt(i));
							if (type == IO::FileType::Directory) IO::RemoveEntireDirectory(arguments->ElementAt(i));
							else IO::RemoveFile(arguments->ElementAt(i));
						}
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_BUILD) == 0) {
						context.enter_state_critical_section();
						try { for (int i = 1; i < arguments->Length(); i++) {
							auto path = ExpandPath(arguments->ElementAt(i));
							bool present = false;
							for (auto & c : state->compile_list) if (string::CompareIgnoreCase(c.source_path, path) == 0) { present = true; break; }
							if (!present) state->compile_list.InsertLast(compile_task { .source_path = path });
						} } catch (...) { context.leave_state_critical_section(); throw; }
						context.leave_state_critical_section();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_LINK) == 0) {
						context.enter_state_critical_section();
						try { for (int i = 1; i < arguments->Length(); i++) {
							state->link_list.InsertLast(ExpandPath(arguments->ElementAt(i)));
						} } catch (...) { context.leave_state_critical_section(); throw; }
						context.leave_state_critical_section();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_ATTACH) == 0) {
						context.enter_state_critical_section();
						try { if (arguments->Length() > 2) {
							state->attach_list.InsertLast(attach_task {
								.source_path = ExpandPath(arguments->ElementAt(1)),
								.destination_path = arguments->ElementAt(2)
							});
						} else throw InvalidArgumentException(); } catch (...) { context.leave_state_critical_section(); throw; }
						context.leave_state_critical_section();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_RSRC) == 0) {
						context.enter_state_critical_section();
						try { if (arguments->Length() > 2) {
							state->resource_list.InsertLast(resource_task {
								.source_path = ExpandPath(arguments->ElementAt(1)),
								.resource_name = arguments->ElementAt(2)
							});
						} else throw InvalidArgumentException(); } catch (...) { context.leave_state_critical_section(); throw; }
						context.leave_state_critical_section();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_LINKA) == 0) {
						context.enter_state_critical_section();
						try { for (int i = 1; i < arguments->Length(); i++) state->link_tool->extra_command_line.InsertLast(arguments->ElementAt(i)); }
						catch (...) { context.leave_state_critical_section(); throw; }
						context.leave_state_critical_section();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_IFEQ) == 0) {
						if (arguments->Length() > 3) {
							if (string::CompareIgnoreCase(arguments->ElementAt(1), arguments->ElementAt(2)) == 0) {
								int pos_jump = -1;
								for (int j = 0; j < commands.Length(); j++) {
									SafePointer< Array<string> > aj = ertbs_decompose_command(commands[j]);
									if (aj && aj->Length() == 1) {
										auto cj = ertbs_substitute_variables(aj->ElementAt(0), variables);
										if (cj.Length() && cj[cj.Length() - 1] == L':' && string::CompareIgnoreCase(cj.Fragment(0, cj.Length() - 1), arguments->ElementAt(3)) == 0) {
											pos_jump = j + 1;
											break;
										}
									}
								}
								if (pos_jump >= 0) { ip = pos_jump; continue; }
							}
						} else throw InvalidArgumentException();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_IFNEQ) == 0) {
						if (arguments->Length() > 3) {
							if (string::CompareIgnoreCase(arguments->ElementAt(1), arguments->ElementAt(2)) != 0) {
								int pos_jump = -1;
								for (int j = 0; j < commands.Length(); j++) {
									SafePointer< Array<string> > aj = ertbs_decompose_command(commands[j]);
									if (aj && aj->Length() == 1) {
										auto cj = ertbs_substitute_variables(aj->ElementAt(0), variables);
										if (cj.Length() && cj[cj.Length() - 1] == L':' && string::CompareIgnoreCase(cj.Fragment(0, cj.Length() - 1), arguments->ElementAt(3)) == 0) {
											pos_jump = j + 1;
											break;
										}
									}
								}
								if (pos_jump >= 0) { ip = pos_jump; continue; }
							}
						} else throw InvalidArgumentException();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_GOTO) == 0) {
						if (arguments->Length() > 1) {
							int pos_jump = -1;
							for (int j = 0; j < commands.Length(); j++) {
								SafePointer< Array<string> > aj = ertbs_decompose_command(commands[j]);
								if (aj && aj->Length() == 1) {
									auto cj = ertbs_substitute_variables(aj->ElementAt(0), variables);
									if (cj.Length() && cj[cj.Length() - 1] == L':' && string::CompareIgnoreCase(cj.Fragment(0, cj.Length() - 1), arguments->ElementAt(1)) == 0) {
										pos_jump = j + 1;
										break;
									}
								}
							}
							if (pos_jump >= 0) { ip = pos_jump; continue; }
						} else throw InvalidArgumentException();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_EXIT) == 0) {
						break;
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_SET) == 0) {
						if (arguments->Length() > 2) {
							variables.Update(arguments->ElementAt(1).UpperCase(), arguments->ElementAt(2));
						} else throw InvalidArgumentException();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_ALERT) == 0) {
						for (int i = 1; i < arguments->Length(); i++) alerts.Append(arguments->ElementAt(i));
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_FAIL) == 0) {
						for (int i = 1; i < arguments->Length(); i++) alerts.Append(arguments->ElementAt(i));
						throw Exception();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_DEFINE) == 0) {
						context.enter_state_critical_section();
						try { for (int i = 1; i < arguments->Length(); i++) state->defines_list.Append(arguments->ElementAt(i), L"1"); }
						catch (...) { context.leave_state_critical_section(); throw; }
						context.leave_state_critical_section();
					} else if (string::CompareIgnoreCase(command, ERTBT_SCRIPT_COMMAND_INC) == 0) {
						context.enter_state_critical_section();
						try { for (int i = 1; i < arguments->Length(); i++) state->include_list.InsertLast(ExpandPath(arguments->ElementAt(i))); }
						catch (...) { context.leave_state_critical_section(); throw; }
						context.leave_state_critical_section();
					} else {
						auto cmd = command;
						arguments->RemoveFirst();
						SafePointer<Process> executor = CreateCommandProcess(cmd, arguments);
						if (!executor) throw InvalidArgumentException();
						executor->Wait();
						if (executor->GetExitCode()) throw Exception();
					}
					ip++;
				}
				SetCurrentDirectory(wd);
				sync->Open();
			} catch (...) {
				sync->Open();
				DynamicString alert;
				for (auto & a : alerts) alert += a += L"\n";
				context.build_status_notify(input, build_tool_status::failed, 0, 0, alert.ToString());
				return;
			}
			context.build_status_notify(input, build_tool_status::built_new, 0, 0, L"");
		}
		void script_compiler::enumerate_extensions(Engine::Array<string> & list) { list.Append(L"ertbs"); }
		string script_compiler::ToString(void) const { return L"Executor scaenariorum"; }
	}
}