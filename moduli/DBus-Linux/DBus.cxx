#include "DBus.h"
#include "DBusDL.h"

namespace ESSE
{
	namespace DBus
	{
		oref<DBusAPI> _common_api;
		oref<IConnection> _common_system_bus, _common_session_bus;
		class Connection : public IConnection
		{
			oref<DBusAPI> _api;
			DBusConnection _con;
			DBusMessage _current;
		public:
			Connection(DBusAPI * api, BusType bus) : _api(api), _current(0)
			{
				if (bus == BusType::SystemBus) _con = _api->dbus_bus_get(DBUS_BUS_SYSTEM, 0);
				else if (bus == BusType::SessionBus) _con = _api->dbus_bus_get(DBUS_BUS_SESSION, 0);
				else throw InvalidArgumentException();
				if (!_con) throw InputOutputException(Errores::SuberrorIO::Unknown);
			}
			virtual ~Connection(void) override { if (_current) _api->dbus_message_unref(_current); _api->dbus_connection_unref(_con); }
			virtual bool BeginInvocation(const char * service, const char * path, const char * interface, const char * method) noexcept override
			{
				if (_current) _api->dbus_message_unref(_current);
				_current = _api->dbus_message_new_method_call(service, path, interface, method);
				return _current != 0;
			}
			virtual bool BeginInvocation(const char * path, const char * interface, const char * method) noexcept override
			{
				if (_current) _api->dbus_message_unref(_current);
				_current = _api->dbus_message_new_method_call(interface, path, interface, method);
				return _current != 0;
			}
			virtual bool EndInvocationNoWait(void) noexcept override
			{
				if (_current) {
					auto status = _api->dbus_connection_send(_con, _current, 0);
					_api->dbus_connection_flush(_con);
					_api->dbus_message_unref(_current); _current = 0;
					return status;
				} else return false;
			}
			virtual bool EndInvocationVoid(void) noexcept override
			{
				if (_current) {
					auto reply = _api->dbus_connection_send_with_reply_and_block(_con, _current, DBUS_TIMEOUT_INFINITE, 0);
					if (reply) _api->dbus_message_unref(reply);
					_api->dbus_message_unref(_current); _current = 0;
					return reply;
				} else return false;
			}
			virtual bool EndInvocationVariant(Variant & responce) noexcept override
			{
				if (_current) {
					auto reply = _api->dbus_connection_send_with_reply_and_block(_con, _current, DBUS_TIMEOUT_INFINITE, 0);
					bool rv_ok = true;
					if (reply) {
						DBusMessageIter main, var;
						DBusMessageIter * use;
						if (_api->dbus_message_iter_init(reply, &main)) {
							if (_api->dbus_message_iter_get_arg_type(&main) == DBUS_TYPE_VARIANT) {
								_api->dbus_message_iter_recurse(&main, &var);
								use = &var;
							} else use = &main;
							auto type = _api->dbus_message_iter_get_arg_type(use);
							if (type == DBUS_TYPE_BYTE || type == DBUS_TYPE_BOOLEAN || type == DBUS_TYPE_UINT16 || type == DBUS_TYPE_INT16 || type == DBUS_TYPE_UINT32 || type == DBUS_TYPE_INT32) {
								responce.type = type;
								responce.ui32 = 0;
								responce.s = string();
								_api->dbus_message_iter_get_basic(use, &responce.ui32);
							} else if (type == DBUS_TYPE_UINT64 || type == DBUS_TYPE_INT64 || type == DBUS_TYPE_DOUBLE) {
								responce.type = type;
								responce.ui64 = 0;
								responce.s = string();
								_api->dbus_message_iter_get_basic(use, &responce.ui64);
							} else if (type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH) {
								const char * pstring;
								_api->dbus_message_iter_get_basic(use, &pstring);
								responce.type = type;
								responce.ui64 = 0;
								try { responce.s = pstring; } catch (...) { rv_ok = false; }
							} else rv_ok = false;
						} else rv_ok = false;
						_api->dbus_message_unref(reply);
					}
					_api->dbus_message_unref(_current); _current = 0;
					if (!rv_ok) {
						responce.type = DBUS_TYPE_INVALID;
						responce.ui64 = 0;
						responce.s = string();
					}
					return reply;
				} else return false;
			}
			virtual bool EndInvocationVariantArray(array<Variant> & responce) noexcept override
			{
				if (_current) {
					auto reply = _api->dbus_connection_send_with_reply_and_block(_con, _current, DBUS_TIMEOUT_INFINITE, 0);
					bool rv_ok = true;
					if (reply) {
						DBusMessageIter main, record, var;
						DBusMessageIter * use, * main_use;
						if (_api->dbus_message_iter_init(reply, &main)) {
							if (_api->dbus_message_iter_get_arg_type(&main) == DBUS_TYPE_VARIANT) {
								_api->dbus_message_iter_recurse(&main, &var);
								main_use = &var;
							} else main_use = &main;
							if (_api->dbus_message_iter_get_arg_type(main_use) == DBUS_TYPE_ARRAY || _api->dbus_message_iter_get_arg_type(main_use) == DBUS_TYPE_STRUCT) {
								_api->dbus_message_iter_recurse(main_use, &record);
								while (_api->dbus_message_iter_get_arg_type(&record) != DBUS_TYPE_INVALID && rv_ok) {
									if (_api->dbus_message_iter_get_arg_type(&record) == DBUS_TYPE_VARIANT) {
										_api->dbus_message_iter_recurse(&record, &var);
										use = &var;
									} else use = &record;
									Variant v;
									auto type = _api->dbus_message_iter_get_arg_type(use);
									if (type == DBUS_TYPE_BYTE || type == DBUS_TYPE_BOOLEAN || type == DBUS_TYPE_UINT16 || type == DBUS_TYPE_INT16 || type == DBUS_TYPE_UINT32 || type == DBUS_TYPE_INT32) {
										v.type = type;
										v.ui32 = 0;
										v.s = string();
										_api->dbus_message_iter_get_basic(use, &v.ui32);
									} else if (type == DBUS_TYPE_UINT64 || type == DBUS_TYPE_INT64 || type == DBUS_TYPE_DOUBLE) {
										v.type = type;
										v.ui64 = 0;
										v.s = string();
										_api->dbus_message_iter_get_basic(use, &v.ui64);
									} else if (type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH) {
										const char * pstring;
										_api->dbus_message_iter_get_basic(use, &pstring);
										v.type = type;
										v.ui64 = 0;
										try { v.s = pstring; } catch (...) { rv_ok = false; }
									} else rv_ok = false;
									try { responce.Append(v); } catch (...) { rv_ok = false; }
									_api->dbus_message_iter_next(&record);
								}
							} else rv_ok = false;
						} else rv_ok = false;
						_api->dbus_message_unref(reply);
					}
					_api->dbus_message_unref(_current); _current = 0;
					if (!rv_ok) responce.Clear();
					return reply;
				} else return false;
			}
			virtual bool EndInvocationObjectArray(array<ucs1_string> & responce) noexcept override
			{
				if (_current) {
					auto reply = _api->dbus_connection_send_with_reply_and_block(_con, _current, DBUS_TIMEOUT_INFINITE, 0);
					if (reply) {
						DBusMessageIter main, array;
						if (_api->dbus_message_iter_init(reply, &main) && _api->dbus_message_iter_get_arg_type(&main) == DBUS_TYPE_ARRAY) {
							_api->dbus_message_iter_recurse(&main, &array);
							while (_api->dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_OBJECT_PATH) {
								const char * path;
								_api->dbus_message_iter_get_basic(&array, &path);
								try { responce.Append(path); } catch (...) {}
								if (!_api->dbus_message_iter_next(&array)) break;
							}
						}
						_api->dbus_message_unref(reply);
					}
					_api->dbus_message_unref(_current); _current = 0;
					return reply;
				} else return false;
			}
			virtual bool EndInvocationSessionArray(array<SessionDesc> & responce) noexcept override
			{
				if (_current) {
					auto reply = _api->dbus_connection_send_with_reply_and_block(_con, _current, DBUS_TIMEOUT_INFINITE, 0);
					if (reply) {
						DBusMessageIter main, array, record;
						if (_api->dbus_message_iter_init(reply, &main) && _api->dbus_message_iter_get_arg_type(&main) == DBUS_TYPE_ARRAY) {
							_api->dbus_message_iter_recurse(&main, &array);
							while (_api->dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRUCT) {
								_api->dbus_message_iter_recurse(&array, &record);
								try {
									const char * pstring;
									SessionDesc desc;
									if (_api->dbus_message_iter_get_arg_type(&record) != DBUS_TYPE_STRING) break;
									_api->dbus_message_iter_get_basic(&record, &pstring);
									desc.sid = pstring;
									if (!_api->dbus_message_iter_next(&record)) break;
									if (_api->dbus_message_iter_get_arg_type(&record) != DBUS_TYPE_UINT32) break;
									_api->dbus_message_iter_get_basic(&record, &desc.uid);
									if (!_api->dbus_message_iter_next(&record)) break;
									if (_api->dbus_message_iter_get_arg_type(&record) != DBUS_TYPE_STRING) break;
									_api->dbus_message_iter_get_basic(&record, &pstring);
									desc.user_name = pstring;
									if (!_api->dbus_message_iter_next(&record)) break;
									if (_api->dbus_message_iter_get_arg_type(&record) != DBUS_TYPE_STRING) break;
									_api->dbus_message_iter_get_basic(&record, &pstring);
									desc.seat_id = pstring;
									if (!_api->dbus_message_iter_next(&record)) break;
									if (_api->dbus_message_iter_get_arg_type(&record) != DBUS_TYPE_OBJECT_PATH) break;
									_api->dbus_message_iter_get_basic(&record, &pstring);
									desc.session_path = pstring;
									responce.Append(desc);
								} catch (...) { break; }
								if (!_api->dbus_message_iter_next(&array)) break;
							}
						}
						_api->dbus_message_unref(reply);
					}
					_api->dbus_message_unref(_current); _current = 0;
					return reply;
				} else return false;
			}
			virtual bool AddBooleanArgument(bool value) noexcept override
			{
				if (!_current) return false;
				int32 send = value;
				return _api->dbus_message_append_args(_current, DBUS_TYPE_BOOLEAN, &send, DBUS_TYPE_INVALID);
			}
			virtual bool AddInt32Argument(int32 value) noexcept override
			{
				if (!_current) return false;
				int32 send = value;
				return _api->dbus_message_append_args(_current, DBUS_TYPE_INT32, &send, DBUS_TYPE_INVALID);
			}
			virtual bool AddUInt32Argument(uint32 value) noexcept override
			{
				if (!_current) return false;
				uint32 send = value;
				return _api->dbus_message_append_args(_current, DBUS_TYPE_UINT32, &send, DBUS_TYPE_INVALID);
			}
			virtual bool AddStringArgument(const string & value) noexcept override
			{
				if (!_current) return false;
				try {
					ucs1_string ucs1 = value;
					const char * pstr = ucs1.GetData();
					return _api->dbus_message_append_args(_current, DBUS_TYPE_STRING, &pstr, DBUS_TYPE_INVALID);
				} catch (...) { return false; }
			}
			virtual bool AddStringArrayArgument(const array<string> & value) noexcept override
			{
				if (!_current) return false;
				try {
					array<ucs1_string> array_utf8(value.GetLength());
					array<const char *> array_ptr(value.GetLength());
					for (auto & v : value) array_utf8.Append(v);
					for (auto & v : array_utf8) array_ptr.Append(v.GetData());
					const char ** parray = array_ptr.GetBuffer();
					return _api->dbus_message_append_args(_current, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &parray, value.GetLength(), DBUS_TYPE_INVALID);
				} catch (...) { return false; }
			}
			virtual bool AddIconArgument(Picturae::Picture * icon) noexcept override
			{
				if (!_current) return false;
				try {
					auto rec = icon ? icon->Convert(Picturae::PixelFormat::R8G8B8A8, Picturae::AlphaMode::Straight, Picturae::ScanOrigin::TopLeft) : oref<Picturae::Picture>();
					DBusMessageIter iter_main, iter_array, iter_struct, iter_variant, iter_image, iter_bytes;
					_api->dbus_message_iter_init_append(_current, &iter_main);
					if (!_api->dbus_message_iter_open_container(&iter_main, DBUS_TYPE_ARRAY, "{sv}", &iter_array)) return false;
					if (icon) {
						if (!_api->dbus_message_iter_open_container(&iter_array, DBUS_TYPE_DICT_ENTRY, 0, &iter_struct)) {
							_api->dbus_message_iter_abandon_container(&iter_main, &iter_array);
							return false;
						}
						const char * icon_key = "image-data";
						if (!_api->dbus_message_iter_append_basic(&iter_struct, DBUS_TYPE_STRING, &icon_key)) {
							_api->dbus_message_iter_abandon_container(&iter_array, &iter_struct);
							_api->dbus_message_iter_abandon_container(&iter_main, &iter_array);
							return false;
						}
						if (!_api->dbus_message_iter_open_container(&iter_struct, DBUS_TYPE_VARIANT, "(iiibiiay)", &iter_variant)) {
							_api->dbus_message_iter_abandon_container(&iter_array, &iter_struct);
							_api->dbus_message_iter_abandon_container(&iter_main, &iter_array);
							return false;
						}
						if (!_api->dbus_message_iter_open_container(&iter_variant, DBUS_TYPE_STRUCT, 0, &iter_image)) {
							_api->dbus_message_iter_abandon_container(&iter_struct, &iter_variant);
							_api->dbus_message_iter_abandon_container(&iter_array, &iter_struct);
							_api->dbus_message_iter_abandon_container(&iter_main, &iter_array);
							return false;
						}
						int width = rec->GetDesc().width, height = rec->GetDesc().height, stride = rec->GetDesc().stride;
						uint alpha = true;
						int bps = 8, num_channels = 4;
						const void * pdata = rec->GetDesc().data;
						try {
							if (!_api->dbus_message_iter_append_basic(&iter_image, DBUS_TYPE_INT32, &width)) throw Exception();
							if (!_api->dbus_message_iter_append_basic(&iter_image, DBUS_TYPE_INT32, &height)) throw Exception();
							if (!_api->dbus_message_iter_append_basic(&iter_image, DBUS_TYPE_INT32, &stride)) throw Exception();
							if (!_api->dbus_message_iter_append_basic(&iter_image, DBUS_TYPE_BOOLEAN, &alpha)) throw Exception();
							if (!_api->dbus_message_iter_append_basic(&iter_image, DBUS_TYPE_INT32, &bps)) throw Exception();
							if (!_api->dbus_message_iter_append_basic(&iter_image, DBUS_TYPE_INT32, &num_channels)) throw Exception();
							if (!_api->dbus_message_iter_open_container(&iter_image, DBUS_TYPE_ARRAY, "y", &iter_bytes)) throw Exception();
							if (!_api->dbus_message_iter_append_fixed_array(&iter_bytes, DBUS_TYPE_BYTE, &pdata, height * stride)) {
								_api->dbus_message_iter_abandon_container(&iter_image, &iter_bytes);
								throw Exception();
							}
							if (!_api->dbus_message_iter_close_container(&iter_image, &iter_bytes)) {
								_api->dbus_message_iter_abandon_container(&iter_image, &iter_bytes);
								throw Exception();
							}
						} catch (...) {
							_api->dbus_message_iter_abandon_container(&iter_variant, &iter_image);
							_api->dbus_message_iter_abandon_container(&iter_struct, &iter_variant);
							_api->dbus_message_iter_abandon_container(&iter_array, &iter_struct);
							_api->dbus_message_iter_abandon_container(&iter_main, &iter_array);
							return false;
						}
						if (!_api->dbus_message_iter_close_container(&iter_variant, &iter_image)) {
							_api->dbus_message_iter_abandon_container(&iter_variant, &iter_image);
							_api->dbus_message_iter_abandon_container(&iter_struct, &iter_variant);
							_api->dbus_message_iter_abandon_container(&iter_array, &iter_struct);
							_api->dbus_message_iter_abandon_container(&iter_main, &iter_array);
							return false;
						}
						if (!_api->dbus_message_iter_close_container(&iter_struct, &iter_variant)) {
							_api->dbus_message_iter_abandon_container(&iter_struct, &iter_variant);
							_api->dbus_message_iter_abandon_container(&iter_array, &iter_struct);
							_api->dbus_message_iter_abandon_container(&iter_main, &iter_array);
							return false;
						}
						if (!_api->dbus_message_iter_close_container(&iter_array, &iter_struct)) {
							_api->dbus_message_iter_abandon_container(&iter_array, &iter_struct);
							_api->dbus_message_iter_abandon_container(&iter_main, &iter_array);
							return false;
						}
					}
					if (!_api->dbus_message_iter_close_container(&iter_main, &iter_array)) {
						_api->dbus_message_iter_abandon_container(&iter_main, &iter_array);
						return false;
					}
					return true;
				} catch (...) { return false; }
			}
			virtual bool GetProperty(const char * service, const char * path, const char * interface, const char * property, Variant & responce) noexcept override
			{
				if (!BeginInvocation(service, path, "org.freedesktop.DBus.Properties", "Get")) return false;
				if (!AddStringArgument(interface)) return false;
				if (!AddStringArgument(property)) return false;
				return EndInvocationVariant(responce);
			}
		};
		oref<IConnection> IConnection::Connect(BusType bus, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			oref<DBusAPI> api;
			Memory::AcquireRootLock();
			try {
				if (!_common_api) _common_api = new DBusAPI;
				api = _common_api;
			} catch (...) { Memory::ReleaseRootLock(); throw; }
			Memory::ReleaseRootLock();
			return oref<IConnection>::CreateOwned(new Connection(api, bus));
			ESSE_TRY_OUTRO(0)
		}
		oref<IConnection> IConnection::Query(BusType bus, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			oref<IConnection> * storage;
			oref<IConnection> result;
			if (bus == BusType::SystemBus) storage = &_common_system_bus;
			else if (bus == BusType::SessionBus) storage = &_common_session_bus;
			else { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
			Memory::AcquireRootLock();
			try {
				if (!*storage) {
					if (!_common_api) _common_api = new DBusAPI;
					*storage = oref<IConnection>::CreateOwned(new Connection(_common_api, bus));
				}
				result = *storage;
			} catch (...) { Memory::ReleaseRootLock(); throw; }
			Memory::ReleaseRootLock();
			return result;
			ESSE_TRY_OUTRO(0)
		}
	}
}