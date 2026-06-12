#include "esse_link.h"

using namespace Engine;
using namespace Engine::IO;
using namespace Engine::Streaming;

namespace esse {
	namespace constructor {
		linker::linker(Engine::Storage::RegistryNode * node)
		{
			linker_command = node->GetValueString(L"Imperatum");
			output_argument = node->GetValueString(L"ArgumentumDestinationis");
			executable_extension = node->GetValueString(L"ExtensioDestinationis");
			SafePointer<Storage::RegistryNode> extra = node->OpenNode(L"ArgumentaExtra");
			if (extra) for (auto & e : extra->GetValues()) extra_command_line.InsertLast(extra->GetValueString(e));
		}
		void linker::process_file(const string & input, const string & mdl, const string & option, build_state * state, process_context & context)
		{
			auto postfix = executable_extension.Length() ? L"." + executable_extension : L"";
			auto transient_output = ExpandPath(state->project_object_path + L"/" + state->output_exec_path + postfix);
			state->output_exec_path = ExpandPath(state->project_build_path + L"/" + state->output_exec_path + postfix);
			if (state->idle_mode) {
				context.build_status_notify(input, build_tool_status::skipped, 0, 0, L"");
				return;
			}
			Array<string> link_args(0x80);
			for (auto & l : state->link_list) link_args.Append(l);
			for (auto & x : extra_command_line) link_args.Append(x);
			command_line_append(link_args, output_argument, transient_output);
			SafePointer<Process> linker;
			SafePointer<Stream> pipe_read;
			context.enter_state_critical_section();
			try {
				SafePointer<Stream> pipe_write;
				handle log_read, log_write;
				IO::CreatePipe(&log_write, &log_read);
				try { pipe_read = new FileStream(log_read, true); } catch (...) { CloseHandle(log_read); CloseHandle(log_write); throw; }
				try { pipe_write = new FileStream(log_write, true); } catch (...) { CloseHandle(log_write); throw; }
				IO::SetStandardOutput(log_write);
				IO::SetStandardError(log_write);
				linker = CreateCommandProcess(linker_command, &link_args);
				if (!linker) throw Exception();
				IO::SetStandardOutput(InvalidHandle);
				IO::SetStandardError(InvalidHandle);
			} catch (...) {
				IO::SetStandardOutput(InvalidHandle);
				IO::SetStandardError(InvalidHandle);
				context.leave_state_critical_section();
				context.build_status_notify(input, build_tool_status::failed, 0, 0, state->io->localized(223));
				return;
			}
			context.leave_state_critical_section();
			SafePointer<TextReader> reader = new TextReader(pipe_read, Encoding::UTF8);
			auto log = reader->ReadAll();
			linker->Wait();
			if (linker->GetExitCode()) {
				context.build_status_notify(input, build_tool_status::failed, 0, 0, log);
				return;
			}
			try {
				SafePointer<FileStream> src = new FileStream(transient_output, AccessRead, OpenExisting);
				SafePointer<FileStream> dest = new FileStream(state->output_exec_path, AccessWrite, CreateAlways);
				try { Unix::SetFileAccessRights(dest->Handle(), Unix::AccessRightAll, Unix::AccessRightReadOnly, Unix::AccessRightReadOnly); } catch (...) {}
				src->CopyTo(dest);
			} catch (...) {
				context.build_status_notify(input, build_tool_status::failed, 0, 0, state->io->localized(224));
				return;
			}
			for (auto & a : state->attach_list) try {
				auto dest_path = ExpandPath(state->project_build_path + L"/" + a.destination_path);
				CreateDirectoryTree(Path::GetDirectory(dest_path));
				SafePointer<FileStream> src = new FileStream(a.source_path, AccessRead, OpenExisting);
				SafePointer<FileStream> dest = new FileStream(dest_path, AccessWrite, CreateAlways);
				auto src_perm = Unix::GetFileUserAccessRights(src->Handle());
				try { Unix::SetFileAccessRights(dest->Handle(), src_perm, src_perm & Unix::AccessRightReadOnly, src_perm & Unix::AccessRightReadOnly); } catch (...) {}
				src->CopyTo(dest);
			} catch (...) {
				context.build_status_notify(input, build_tool_status::failed, 0, 0, FormatString(state->io->localized(225), a.source_path));
				return;
			}
			context.build_status_notify(input, build_tool_status::built_new, 0, 0, L"");
		}
		void linker::enumerate_extensions(Engine::Array<string> & list) {}
		string linker::ToString(void) const
		{
			Engine::DynamicString result;
			result << L"Adhaesor: " << linker_command;
			result << L" " << output_argument;
			for (auto & e : extra_command_line) result << L" " << e;
			result << L" --> ." << executable_extension;
			return result.ToString();
		}
	}
}