#pragma once

#include "CorBasis.h"

namespace ESSE
{
	namespace Errores
	{
		constexpr uintptr ErrorSuccess			= 0;
		constexpr uintptr ErrorNotImplemented	= 1;
		constexpr uintptr ErrorOutOfMemory		= 2;
		constexpr uintptr ErrorInvalidArgument	= 3;
		constexpr uintptr ErrorInvalidFormat	= 4;
		constexpr uintptr ErrorInvalidState		= 5;
		constexpr uintptr ErrorIO				= 6;
		constexpr uintptr ErrorDynamicLinkage	= 7;
		constexpr uintptr ErrorNetwork			= 8;
		constexpr uintptr ErrorRPC				= 9;

		namespace SuberrorIO
		{
			constexpr uintptr Success				= 0x000;
			constexpr uintptr Unknown				= 0x001;
			constexpr uintptr FileNotFound			= 0x002;
			constexpr uintptr PathNotFound			= 0x003;
			constexpr uintptr TooManyOpenFiles		= 0x004;
			constexpr uintptr AccessDenied			= 0x005;
			constexpr uintptr InvalidHandle			= 0x006;
			constexpr uintptr NotEnoughMemory		= 0x007;
			constexpr uintptr InvalidDevice			= 0x008;
			constexpr uintptr IsReadOnly			= 0x009;
			constexpr uintptr NoDiskSpace			= 0x00A;
			constexpr uintptr FileExists			= 0x00B;
			constexpr uintptr NotImplemented		= 0x00C;
			constexpr uintptr DirectoryNotEmpty		= 0x00D;
			constexpr uintptr DirectoryIsCurrent	= 0x00E;
			constexpr uintptr NotSameDevice			= 0x00F;
			constexpr uintptr BadPathName			= 0x010;
			constexpr uintptr FileNameTooLong		= 0x011;
			constexpr uintptr FileTooLarge			= 0x012;
			constexpr uintptr ReadFailure			= 0x013;
			constexpr uintptr WriteFailure			= 0x014;
			constexpr uintptr CreateFailure			= 0x015;
			constexpr uintptr OpenFailure			= 0x016;
		};
	}
}