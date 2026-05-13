#pragma once

#include "kernel.h"

namespace esse {
	namespace constructor {
		struct ccxx_compiler : build_tool
		{
			string compiler_command;
			string define_argument, include_argument, output_argument;
			string object_extension;

			ccxx_compiler(Engine::Storage::RegistryNode * node);
			virtual void process_file(const string & input, const string & option, build_state * state, process_context & context) override;
			virtual void enumerate_extensions(Engine::Array<string> & list) override;
			virtual string ToString(void) const override;
		};
	}
}