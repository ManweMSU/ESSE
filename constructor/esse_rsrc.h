#pragma once

#include "kernel.h"

namespace esse {
	namespace constructor {
		enum class resource_tool_mode { none = 0, embed = 1, native = 2, bundle = 3 };
		struct resource_tool : build_tool
		{
			resource_tool_mode mode;
			string resource_tool_command;
			string output_argument;
			string resource_object_extension;
			string icon_codec, icon_extension;
			Engine::Volumes::List<int> icon_sizes;

			resource_tool(Engine::Storage::RegistryNode * node);
			virtual void process_file(const string & input, const string & option, build_state * state, process_context & context) override;
			virtual void enumerate_extensions(Engine::Array<string> & list) override;
			virtual string ToString(void) const override;
		};
	}
}