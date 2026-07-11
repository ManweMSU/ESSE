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
		.Object						= handle(uintptr(0x01)),
		.DynamicObject				= handle(uintptr(0x02)),
		.Console					= handle(uintptr(0x03)),
		.ITextEncoder				= handle(uintptr(0x04)),
		.ITextDecoder				= handle(uintptr(0x05)),
		.IDevice					= handle(uintptr(0x06)),
		.IDeviceContext				= handle(uintptr(0x07)),
		.IDeviceContext2D			= handle(uintptr(0x08)),
		.IDeviceContextFactory2D	= handle(uintptr(0x09)),
		.IScreen					= handle(uintptr(0x0A)),
		.IWindow					= handle(uintptr(0x0B)),
		.IWindowSystem				= handle(uintptr(0x0C)),
	};
}