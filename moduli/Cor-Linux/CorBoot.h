#pragma once

#include <Cor/CorBasis.h>

namespace ESSE
{
	namespace Linux
	{
		static constexpr uint CreateProcessSearchPathOnly = 0x10000;
		void GetCommandLine(int * argc, const char *** argv) noexcept;
		void GetThisProcessPID(pid_t * pid) noexcept;
	}
}