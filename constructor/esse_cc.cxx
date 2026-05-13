#include "esse_cc.h"

using namespace Engine;
using namespace Engine::IO;
using namespace Engine::Streaming;

namespace esse {
	namespace constructor {
		ccxx_compiler::ccxx_compiler(Engine::Storage::RegistryNode * node)
		{
			compiler_command = node->GetValueString(L"Imperatum");
			define_argument = node->GetValueString(L"ArgumentumDefinitionis");
			include_argument = node->GetValueString(L"ArgumentumTituli");
			output_argument = node->GetValueString(L"ArgumentumDestinationis");
			object_extension = node->GetValueString(L"ExtensioDestinationis");
			SafePointer<Storage::RegistryNode> extra = node->OpenNode(L"ArgumentaExtra");
			if (extra) for (auto & e : extra->GetValues()) extra_command_line.InsertLast(extra->GetValueString(e));
		}
		void ccxx_compiler::process_file(const string & input, const string & option, build_state * state, process_context & context)
		{
			if (state->idle_mode) {
				context.build_status_notify(input, build_tool_status::skipped, 0, 0, L"");
				return;
			}
			auto output = ExpandPath(state->project_object_path + L"/" + Path::GetFileNameWithoutExtension(input) + L"." + object_extension);
			Time src_date = 0, out_date = 0;
			build_tool_status status_success = build_tool_status::built_new;
			if (!state->clean_mode) try {
				FileStream src(input, AccessRead, OpenExisting);
				FileStream out(output, AccessRead, OpenExisting);
				src_date = DateTime::GetFileAlterTime(src.Handle());
				out_date = DateTime::GetFileAlterTime(out.Handle());
				if (out_date > src_date && out_date > state->project_alternation_date) {
					context.build_status_notify(input, build_tool_status::skipped, 0, 0, L"");
					context.enter_state_critical_section();
					state->link_list.InsertLast(output);
					context.leave_state_critical_section();
					return;
				}
				status_success = build_tool_status::built_renew;
			} catch (...) {}
			Array<string> cc_args(0x80);
			cc_args << input;
			for (auto & i : state->include_list) command_line_append(cc_args, include_argument, i);
			for (auto & d : state->defines_list) command_line_append(cc_args, define_argument, d.key + L"=" + d.value);
			for (auto & x : extra_command_line) cc_args.Append(x);
			command_line_append(cc_args, output_argument, output);
			SafePointer<Process> compiler;
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
				compiler = CreateCommandProcess(compiler_command, &cc_args);
				if (!compiler) throw Exception();
				IO::SetStandardOutput(InvalidHandle);
				IO::SetStandardError(InvalidHandle);
			} catch (...) {
				IO::SetStandardOutput(InvalidHandle);
				IO::SetStandardError(InvalidHandle);
				context.leave_state_critical_section();
				context.build_status_notify(input, build_tool_status::failed, 0, 0, state->io->localized(221));
				return;
			}
			context.leave_state_critical_section();
			SafePointer<TextReader> reader = new TextReader(pipe_read, Encoding::UTF8);
			auto log = reader->ReadAll();
			compiler->Wait();
			if (compiler->GetExitCode()) {
				context.build_status_notify(input, build_tool_status::failed, 0, 0, log);
				return;
			}
			context.build_status_notify(input, status_success, out_date, src_date, L"");
			context.enter_state_critical_section();
			state->link_list.InsertLast(output);
			context.leave_state_critical_section();
		}
		void ccxx_compiler::enumerate_extensions(Engine::Array<string> & list)
		{
			list.Append(L"c");
			list.Append(L"cc");
			list.Append(L"cpp");
			list.Append(L"cxx");
			list.Append(L"c++");
			list.Append(L"m");
			list.Append(L"mm");
		}
		string ccxx_compiler::ToString(void) const
		{
			Engine::DynamicString result;
			result << L"Compilator C/C++: " << compiler_command;
			result << L" " << define_argument;
			result << L" " << include_argument;
			result << L" " << output_argument;
			for (auto & e : extra_command_line) result << L" " << e;
			result << L" --> ." << object_extension;
			return result.ToString();
		}
	}
}