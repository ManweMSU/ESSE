#pragma once

#include <Cor/CorBasis.h>

namespace ESSE
{
	bool QueryResource(const void ** pmem, uintptr * plength, const char * resname, const char * locale) noexcept;
}