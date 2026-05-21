#include "CorObject.h"
#include "../CorVirtualMemory.h"

namespace ESSE
{
	Object::Object(void) noexcept : _reference_count(1) {}
	uintptr Object::Retain(void) noexcept { return InterlockedIncrement(_reference_count); }
	uintptr Object::Release(void) noexcept { auto rc = InterlockedDecrement(_reference_count); if (!rc) delete this; return rc; }
	Object::~Object(void) {}
	#ifdef ESSE_SYSTEMA_WINDOWS
	void Object::_delete(void) noexcept { delete this; }
	#endif
	string Object::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"objectum"; ESSE_TRY_OUTRO(string()) }
	string Object::ToString(void) const { ErrorContext ectx; ErrorClear(ectx); auto result = ToStringE(ectx); ErrorThrow(ectx); return result; }
	uintptr Object::GetReferenceCount(void) const noexcept { return _reference_count; }

	DynamicCastStandardClasses Classes = {
		.Object			= handle(uintptr(1)),
		.DynamicObject	= handle(uintptr(2)),
		.Console		= handle(uintptr(3)),
		.ITextEncoder	= handle(uintptr(4)),
		.ITextDecoder	= handle(uintptr(5)),
	};
}