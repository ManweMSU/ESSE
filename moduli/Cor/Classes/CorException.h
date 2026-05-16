#pragma once

#include "../CorBasis.h"
#include "../CorErrores.h"

#define ESSE_TRY_INTRO try {
#define ESSE_TRY_OUTRO(DRV) } \
catch (ESSE::Exception & e) { ectx = e.GetError(); return DRV; } \
catch (...) { ectx = ESSE::ErrorMake(ESSE::Errores::ErrorOutOfMemory); return DRV; }

namespace ESSE
{
	class Exception
	{
	public:
		Exception(void) noexcept;
		virtual ErrorContext GetError(void) const noexcept;
	};
	class OutOfMemoryException : public Exception
	{
	public:
		OutOfMemoryException(void) noexcept;
		virtual ErrorContext GetError(void) const noexcept override;
	};
	class NotImplementedException : public Exception
	{
	public:
		NotImplementedException(void) noexcept;
		virtual ErrorContext GetError(void) const noexcept override;
	};
	class InvalidArgumentException : public Exception
	{
	public:
		InvalidArgumentException(void) noexcept;
		virtual ErrorContext GetError(void) const noexcept override;
	};
	class InvalidFormatException : public Exception
	{
	public:
		InvalidFormatException(void) noexcept;
		virtual ErrorContext GetError(void) const noexcept override;
	};
	class InvalidStateException : public Exception
	{
	public:
		InvalidStateException(void) noexcept;
		virtual ErrorContext GetError(void) const noexcept override;
	};
	class InputOutputException : public Exception
	{
		uintptr _subcode;
	public:
		InputOutputException(uintptr subcode) noexcept;
		virtual ErrorContext GetError(void) const noexcept override;
	};
	class DynamicLinkageException : public Exception
	{
		uintptr _subcode;
	public:
		DynamicLinkageException(uintptr subcode) noexcept;
		virtual ErrorContext GetError(void) const noexcept override;
	};
	class NetworkException : public Exception
	{
		uintptr _subcode;
	public:
		NetworkException(uintptr subcode) noexcept;
		virtual ErrorContext GetError(void) const noexcept override;
	};
	class RemoteProcedureCallException : public Exception
	{
		uintptr _subcode;
	public:
		RemoteProcedureCallException(uintptr subcode) noexcept;
		virtual ErrorContext GetError(void) const noexcept override;
	};
	class CustomException : public Exception
	{
		ErrorContext _ectx;
	public:
		CustomException(const ErrorContext & ectx) noexcept;
		virtual ErrorContext GetError(void) const noexcept override;
	};
	void ErrorThrow(const ErrorContext & ctx);
}