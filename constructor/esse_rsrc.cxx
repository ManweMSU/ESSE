#include "esse_rsrc.h"

using namespace Engine;

namespace esse {
	namespace constructor {
		resource_tool::resource_tool(Engine::Storage::RegistryNode * node)
		{
			auto mode_string = node->GetValueString(L"Modus");
			if (string::CompareIgnoreCase(mode_string, L"incorporaliter") == 0) mode = resource_tool_mode::embed;
			else if (string::CompareIgnoreCase(mode_string, L"native") == 0) mode = resource_tool_mode::native;
			else if (string::CompareIgnoreCase(mode_string, L"sarcina") == 0) mode = resource_tool_mode::bundle;
			else mode = resource_tool_mode::none;
			resource_tool_command = node->GetValueString(L"Imperatum");
			output_argument = node->GetValueString(L"ArgumentumDestinationis");
			resource_object_extension = node->GetValueString(L"ExtensioDestinationis");
			SafePointer<Storage::RegistryNode> extra = node->OpenNode(L"ArgumentaExtra");
			if (extra) for (auto & e : extra->GetValues()) extra_command_line.InsertLast(extra->GetValueString(e));
			SafePointer<Storage::RegistryNode> icon = node->OpenNode(L"Icones");
			if (icon) {
				icon_codec = icon->GetValueString(L"Codificator");
				icon_extension = icon->GetValueString(L"Extensio");
				SafePointer<Storage::RegistryNode> sizes = icon->OpenNode(L"Magnitudines");
				if (sizes) for (auto & s : sizes->GetValues()) icon_sizes.InsertLast(sizes->GetValueInteger(s));
			}
		}
		void resource_tool::process_file(const string & input, const string & option, build_state * state, process_context & context)
		{
			// TODO: IMPLEMENT
			// Engine::Volumes::List<string> extra_command_line;
			// resource_tool_mode mode;
			// string resource_tool_command;
			// string output_argument;
			// string resource_object_extension;
			// string icon_codec, icon_extension;
			// Engine::Volumes::List<int> icon_sizes;

			// TODO: CORRECT: raw-exec-name -- bundle folder (if bundled) -- new-exec-root
			// string output_exec_path, output_bundle_path, project_build_path;

			// TODO: REMOVE
			context.enter_state_critical_section();
			state->output_exec_path = state->app.name;
			context.leave_state_critical_section();
			context.build_status_notify(input, build_tool_status::skipped, 0, 0, L"");
		}
		void resource_tool::enumerate_extensions(Engine::Array<string> & list) {}
		string resource_tool::ToString(void) const
		{
			Engine::DynamicString result;
			if (mode == resource_tool_mode::embed) result << L"Adhaesor: incorporaliter";
			else if (mode == resource_tool_mode::native) result << L"Adhaesor: native";
			else if (mode == resource_tool_mode::bundle) result << L"Adhaesor: in sarcinam";
			else result << L"Adhaesor: error";
			result << L", coprocessor: " << resource_tool_command << L" " << output_argument;
			for (auto & e : extra_command_line) result << L" " << e;
			result << L" --> ." << resource_object_extension;
			result << L", icones: ." << icon_extension << L"@" << icon_codec;
			for (auto & s : icon_sizes) result << L", x" << string(s);
			return result.ToString();
		}
	}
}