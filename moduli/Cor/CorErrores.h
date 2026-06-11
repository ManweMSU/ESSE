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
		namespace SuberrorDL
		{
			constexpr uintptr Success				= 0x000;
			constexpr uintptr ModuleNotFound		= 0x001;
			constexpr uintptr InvalidImageFormat	= 0x002;
			constexpr uintptr InvalidFunctionFormat	= 0x003;
			constexpr uintptr NoDedicatedVersion	= 0x004;
			constexpr uintptr SymbolRedefinition	= 0x005;
			constexpr uintptr LinkageFailure		= 0x006;
			constexpr uintptr InvalidLocalImport	= 0x007;
			constexpr uintptr LibraryNotFound		= 0x008;
			constexpr uintptr InvalidLibraryImport	= 0x009;
			constexpr uintptr AllocationFailure		= 0x00A;
			constexpr uintptr NoEntryPoint			= 0x00B;
			constexpr uintptr InvalidModuleVersion	= 0x00C;
			constexpr uintptr ModuleCorruption		= 0x00D;
			constexpr uintptr UntrustedModule		= 0x00E;
		};
		namespace SuberrorNetwork
		{
			constexpr uintptr Success				= 0x000;
			constexpr uintptr Unknown				= 0x001;
			constexpr uintptr AddressAlreadyIsUse	= 0x002;
			constexpr uintptr AddressIsNotProvided	= 0x003;
			constexpr uintptr DomainUnavailable		= 0x004;
			constexpr uintptr MachineUnreachable	= 0x005;
			constexpr uintptr SubnetUnreachable		= 0x006;
			constexpr uintptr ConnectionRefused		= 0x007;
			constexpr uintptr ConnectionReset		= 0x008;
			constexpr uintptr InvalidProtocol		= 0x009;
			constexpr uintptr RequestTimedOut		= 0x00A;
			constexpr uintptr SecurityLayerError	= 0x00B;
			constexpr uintptr UnresolvedDomainName	= 0x00C;
		};
		namespace SuberrorRPC
		{
			constexpr uintptr Success				= 0x000;
			constexpr uintptr Unknown				= 0x001;
			constexpr uintptr InvalidObjectHandle	= 0x002;
			constexpr uintptr InvalidSelector		= 0x003;
			constexpr uintptr SerializationError	= 0x004;
			constexpr uintptr TransportLayerError	= 0x005;
		}
	}
}