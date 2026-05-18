#pragma once

#include <Cor/IO/CorIO.h>

namespace ESSE
{
	namespace Linux
	{
		void ErrorSetPosix(ErrorContext & ectx) noexcept;
		void ErrorSetPosix(ErrorContext & ectx, int error_number) noexcept;
	}
}