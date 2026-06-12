#pragma once

#include "kernel.h"
#include "ioss.h"

namespace esse {
	namespace constructor {
		bool reconfigure(const string & tsc, io_context & io);
	}
}