#include "CorException.h"

namespace ESSE
{
	Exception::Exception(void) noexcept {}
	ErrorContext Exception::GetError(void) const noexcept { return ErrorMake(0xFFFFFFFF); }
	NotImplementedException::NotImplementedException(void) noexcept {}
	ErrorContext NotImplementedException::GetError(void) const noexcept { return ErrorMake(Errores::ErrorNotImplemented); }
	OutOfMemoryException::OutOfMemoryException(void) noexcept {}
	ErrorContext OutOfMemoryException::GetError(void) const noexcept { return ErrorMake(Errores::ErrorOutOfMemory); }
	InvalidArgumentException::InvalidArgumentException(void) noexcept {}
	ErrorContext InvalidArgumentException::GetError(void) const noexcept { return ErrorMake(Errores::ErrorInvalidArgument); }
	InvalidFormatException::InvalidFormatException(void) noexcept {}
	ErrorContext InvalidFormatException::GetError(void) const noexcept { return ErrorMake(Errores::ErrorInvalidFormat); }
	InvalidStateException::InvalidStateException(void) noexcept {}
	ErrorContext InvalidStateException::GetError(void) const noexcept { return ErrorMake(Errores::ErrorInvalidState); }
	InputOutputException::InputOutputException(uintptr subcode) noexcept : _subcode(subcode) {}
	ErrorContext InputOutputException::GetError(void) const noexcept { return ErrorMake(Errores::ErrorIO, _subcode); }
	DynamicLinkageException::DynamicLinkageException(uintptr subcode) noexcept : _subcode(subcode) {}
	ErrorContext DynamicLinkageException::GetError(void) const noexcept { return ErrorMake(Errores::ErrorDynamicLinkage, _subcode); }
	NetworkException::NetworkException(uintptr subcode) noexcept : _subcode(subcode) {}
	ErrorContext NetworkException::GetError(void) const noexcept { return ErrorMake(Errores::ErrorNetwork, _subcode); }
	RemoteProcedureCallException::RemoteProcedureCallException(uintptr subcode) noexcept : _subcode(subcode) {}
	ErrorContext RemoteProcedureCallException::GetError(void) const noexcept { return ErrorMake(Errores::ErrorRPC, _subcode); }
	CustomException::CustomException(const ErrorContext & ectx) noexcept : _ectx(ectx) {}
	ErrorContext CustomException::GetError(void) const noexcept { return _ectx; }
	void ErrorThrow(const ErrorContext & ctx) { if (ctx.error_code) throw CustomException(ctx); }
}