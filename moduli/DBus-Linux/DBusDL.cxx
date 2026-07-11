#include "DBusDL.h"
#include <dlfcn.h>

namespace ESSE
{
	namespace DBus
	{
		DBusAPI::DBusAPI(void)
		{
			_library = dlopen("libdbus-1.so", RTLD_NOW);
			if (!_library) throw NotImplementedException();
			try {
				DEFINE_FUNCTION_IMPORT(dbus_bus_get)
				DEFINE_FUNCTION_IMPORT(dbus_connection_unref)
				DEFINE_FUNCTION_IMPORT(dbus_connection_flush)
				DEFINE_FUNCTION_IMPORT(dbus_message_new_method_call)
				DEFINE_FUNCTION_IMPORT(dbus_message_unref)
				DEFINE_FUNCTION_IMPORT(dbus_message_get_args)
				DEFINE_FUNCTION_IMPORT(dbus_message_append_args)
				DEFINE_FUNCTION_IMPORT(dbus_connection_send)
				DEFINE_FUNCTION_IMPORT(dbus_connection_send_with_reply_and_block)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_init)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_has_next)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_next)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_get_signature)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_get_arg_type)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_get_element_type)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_recurse)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_get_basic)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_get_element_count)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_init_append)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_open_container)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_close_container)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_abandon_container)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_append_basic)
				DEFINE_FUNCTION_IMPORT(dbus_message_iter_append_fixed_array)
			} catch (...) { dlclose(_library); throw; }
		}
		DBusAPI::~DBusAPI(void) { dlclose(_library); }
    }
}