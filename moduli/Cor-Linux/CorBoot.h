#pragma once

#include <Cor/CorBasis.h>

namespace ESSE
{
	namespace Linux
	{
		void GetCommandLine(int * argc, const char *** argv) noexcept;
		void GetThisProcessPID(pid_t * pid) noexcept;
	}
}