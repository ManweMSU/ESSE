#pragma once

#include "kernel.h"

namespace esse {
	namespace constructor {
		struct script_compiler : build_tool
		{
			Engine::SafePointer<Engine::Semaphore> sync;

			script_compiler(Engine::Storage::RegistryNode * node);
			virtual void process_file(const string & input, const string & mdl, const string & option, build_state * state, process_context & context) override;
			virtual void enumerate_extensions(Engine::Array<string> & list) override;
			virtual string ToString(void) const override;
		};
	}
}