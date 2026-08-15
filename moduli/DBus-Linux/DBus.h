#pragma once

#include <Cor/Images/CorImages.h>

namespace ESSE
{
	namespace DBus
	{
		enum class BusType { SystemBus = 0, SessionBus = 1 };
		struct Variant
		{
			int type;
			union { uint32 ui32; int32 i32; uint64 ui64; int64 i64; double d; };
			string s;
		};
		struct SessionDesc
		{
			ucs1_string sid, user_name, seat_id, session_path;
			uint uid;
		};
		class IConnection : public Object
		{
		public:
			virtual bool BeginInvocation(const char * service, const char * path, const char * interface, const char * method) noexcept = 0;
			virtual bool BeginInvocation(const char * path, const char * interface, const char * method) noexcept = 0;
			virtual bool EndInvocationNoWait(void) noexcept = 0;
			virtual bool EndInvocationVoid(void) noexcept = 0;
			virtual bool EndInvocationHandle(handle & responce) noexcept = 0;
			virtual bool EndInvocationVariant(Variant & responce) noexcept = 0;
			virtual bool EndInvocationVariantArray(array<Variant> & responce) noexcept = 0;
			virtual bool EndInvocationObjectArray(array<ucs1_string> & responce) noexcept = 0;
			virtual bool EndInvocationSessionArray(array<SessionDesc> & responce) noexcept = 0;
			virtual bool AddBooleanArgument(bool value) noexcept = 0;
			virtual bool AddInt32Argument(int32 value) noexcept = 0;
			virtual bool AddUInt32Argument(uint32 value) noexcept = 0;
			virtual bool AddStringArgument(const string & value) noexcept = 0;
			virtual bool AddStringArrayArgument(const array<string> & value) noexcept = 0;
			virtual bool AddIconArgument(Picturae::Picture * icon) noexcept = 0;
			virtual bool GetProperty(const char * service, const char * path, const char * interface, const char * property, Variant & responce) noexcept = 0;
			static oref<IConnection> Connect(BusType bus, ErrorContext & ectx) noexcept;
			static oref<IConnection> Query(BusType bus, ErrorContext & ectx) noexcept;
		};
	}
}