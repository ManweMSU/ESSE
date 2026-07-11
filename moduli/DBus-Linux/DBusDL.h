#pragma once

#include <Cor/IO/CorDL.h>
#include <Cor/Classes/CorObject.h>

namespace ESSE
{
	namespace DBus
	{
		#define DEFINE_HANDLE_TYPE(NAME) typedef struct __internal_##NAME * NAME;
		#define DEFINE_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
		#define DEFINE_FUNCTION_IMPORT(NAME) NAME = reinterpret_cast<func_##NAME>(ESSE::IO::GetLibraryRoutine(_library, #NAME)); if (!NAME) throw NotImplementedException();

		DEFINE_HANDLE_TYPE(DBusConnection)
		DEFINE_HANDLE_TYPE(DBusMessage)

		typedef enum { DBUS_BUS_SESSION, DBUS_BUS_SYSTEM, DBUS_BUS_STARTER } DBusBusType;
		struct DBusError
		{
			const char * name;
			const char * message;
		};
		struct DBusMessageIter
		{
			void * dummy[16];
		};
		constexpr int DBUS_TIMEOUT_INFINITE	= 0x7fffffff;
		constexpr int DBUS_TYPE_INVALID		= '\0';
		constexpr int DBUS_TYPE_BYTE		= 'y';
		constexpr int DBUS_TYPE_BOOLEAN		= 'b';
		constexpr int DBUS_TYPE_INT16		= 'n';
		constexpr int DBUS_TYPE_UINT16		= 'q';
		constexpr int DBUS_TYPE_INT32		= 'i';
		constexpr int DBUS_TYPE_UINT32		= 'u';
		constexpr int DBUS_TYPE_INT64		= 'x';
		constexpr int DBUS_TYPE_UINT64		= 't';
		constexpr int DBUS_TYPE_DOUBLE		= 'd';
		constexpr int DBUS_TYPE_STRING		= 's';
		constexpr int DBUS_TYPE_OBJECT_PATH	= 'o';
		constexpr int DBUS_TYPE_SIGNATURE	= 'g';
		constexpr int DBUS_TYPE_UNIX_FD		= 'h';
		constexpr int DBUS_TYPE_ARRAY		= 'a';
		constexpr int DBUS_TYPE_DICT_ENTRY	= 'e';
		constexpr int DBUS_TYPE_VARIANT		= 'v';
		constexpr int DBUS_TYPE_STRUCT		= 'r';

		class DBusAPI : public Object
		{
			handle _library;
		public:
			DEFINE_FUNCTION_POINTER(dbus_bus_get, DBusConnection, (DBusBusType, DBusError *))
			DEFINE_FUNCTION_POINTER(dbus_connection_unref, void, (DBusConnection))
			DEFINE_FUNCTION_POINTER(dbus_connection_flush, void, (DBusConnection))
			DEFINE_FUNCTION_POINTER(dbus_message_new_method_call, DBusMessage, (const char *, const char *, const char *, const char *))
			DEFINE_FUNCTION_POINTER(dbus_message_unref, void, (DBusMessage))
			DEFINE_FUNCTION_POINTER(dbus_message_get_args, uint32, (DBusMessage, DBusError *, int, ...))
			DEFINE_FUNCTION_POINTER(dbus_message_append_args, uint32, (DBusMessage, int, ...))
			DEFINE_FUNCTION_POINTER(dbus_connection_send, uint32, (DBusConnection, DBusMessage, uint32 *))
			DEFINE_FUNCTION_POINTER(dbus_connection_send_with_reply_and_block, DBusMessage, (DBusConnection, DBusMessage, int, DBusError *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_init, uint32, (DBusMessage, DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_has_next, uint32, (DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_next, uint32, (DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_get_signature, char *, (DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_get_arg_type, int, (DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_get_element_type, int, (DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_recurse, void, (DBusMessageIter *, DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_get_basic, void, (DBusMessageIter *, void *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_get_element_count, int, (DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_init_append, void, (DBusMessage, DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_open_container, uint32, (DBusMessageIter *, int, const char *, DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_close_container, uint32, (DBusMessageIter *, DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_abandon_container, void, (DBusMessageIter *, DBusMessageIter *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_append_basic, uint32, (DBusMessageIter *, int, const void *))
			DEFINE_FUNCTION_POINTER(dbus_message_iter_append_fixed_array, uint32, (DBusMessageIter *, int, const void *, int))
		public:
			DBusAPI(void);
			virtual ~DBusAPI(void) override;
		};
	}
}