#include <Energia/Energia.h>
#include <Cor/IO/CorStreams.h>
#include <DBus-Linux/DBus.h>
#include <unistd.h>

namespace ESSE
{
	namespace Power
	{
		uint _system_lock_type = 0;
		handle _system_lock_file = IO::InvalidHandle;
		void GetPowerStatus(PowerStatusDesc & desc) noexcept
		{
			try {
				ErrorContext ectx; ErrorClear(ectx);
				auto bus = DBus::IConnection::Query(DBus::BusType::SystemBus, ectx);
				ErrorThrow(ectx);
				array<ucs1_string> pwdev(0x20);
				if (!bus->BeginInvocation("/org/freedesktop/UPower", "org.freedesktop.UPower", "EnumerateDevices")) throw Exception();
				if (!bus->EndInvocationObjectArray(pwdev)) throw Exception();
				bool has_line_power = false;
				bool has_battery_power = false;
				bool battery_is_charging = false;
				double battery_level = 0.0;
				for (auto & dev : pwdev) {
					DBus::Variant is_power_supply;
					if (!bus->GetProperty("org.freedesktop.UPower", dev, "org.freedesktop.UPower.Device", "PowerSupply", is_power_supply)) throw Exception();
					if (is_power_supply.type == 'b' && is_power_supply.ui32) {
						DBus::Variant type;
						if (!bus->GetProperty("org.freedesktop.UPower", dev, "org.freedesktop.UPower.Device", "Type", type)) throw Exception();
						if (type.type == 'u') {
							if (type.ui32 == 1 || type.ui32 == 3) {
								DBus::Variant online;
								if (!bus->GetProperty("org.freedesktop.UPower", dev, "org.freedesktop.UPower.Device", "Online", online)) throw Exception();
								if (online.type == 'b' && online.ui32) has_line_power = true;
							} else if (type.ui32 == 2) {
								has_battery_power = true;
								DBus::Variant level, state;
								if (!bus->GetProperty("org.freedesktop.UPower", dev, "org.freedesktop.UPower.Device", "Percentage", level)) throw Exception();
								if (!bus->GetProperty("org.freedesktop.UPower", dev, "org.freedesktop.UPower.Device", "State", state)) throw Exception();
								if (level.type == 'd') battery_level = level.d;
								if (state.type == 'u') battery_is_charging = state.ui32 == 1;
							} else if (type.ui32 == 3) {
								has_line_power = true;
							}
						}
					}
				}
				if (has_battery_power) {
					if (battery_is_charging) desc.status = PowerStatus::BatteryCharging;
					else desc.status = PowerStatus::BatteryDischarging;
					desc.battery_charge_level = battery_level;
				} else {
					desc.battery_charge_level = 0.0;
					if (has_line_power) desc.status = PowerStatus::LinePower;
					else desc.status = PowerStatus::Unknown;
				}
			} catch (...) {
				desc.status = PowerStatus::Unknown;
				desc.battery_charge_level = 0.0;
			}
		}
		bool SystemPowerControl(PowerControl control, bool enforce) noexcept
		{
			try {
				ErrorContext ectx; ErrorClear(ectx);
				auto bus = DBus::IConnection::Query(DBus::BusType::SystemBus, ectx);
				if (ErrorTest(ectx)) return false;
				if (control == PowerControl::Shutdown || control == PowerControl::Reboot || control == PowerControl::Suspend || control == PowerControl::Hibernate) {
					const char * check_method, * invoke_method;
					if (control == PowerControl::Shutdown) { check_method = "CanPowerOff"; invoke_method = "PowerOff"; }
					else if (control == PowerControl::Reboot) { check_method = "CanReboot"; invoke_method = "Reboot"; }
					else if (control == PowerControl::Suspend) { check_method = "CanSuspend"; invoke_method = "Suspend"; }
					else { check_method = "CanHibernate"; invoke_method = "Hibernate"; }
					DBus::Variant cap;
					if (!bus->BeginInvocation("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", check_method)) return false;
					if (!bus->EndInvocationVariant(cap)) return false;
					if (cap.type != 's' || cap.s != U"yes") return false;
					if (!bus->BeginInvocation("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", invoke_method)) return false;
					if (!bus->AddBooleanArgument(false)) return false;
					return bus->EndInvocationVoid();
				} else if (control == PowerControl::LockSession) {
					uint uid = getuid();
					array<DBus::SessionDesc> sessions(0x10);
					if (!bus->BeginInvocation("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "ListSessions")) return false;
					if (!bus->EndInvocationSessionArray(sessions)) return false;
					for (auto & session : sessions) if (session.uid == uid) {
						DBus::Variant can_lock;
						if (!bus->GetProperty("org.freedesktop.login1", session.session_path, "org.freedesktop.login1.Session", "CanLock", can_lock)) return false;
						if (can_lock.type == 'b' && can_lock.ui32) {
							if (!bus->BeginInvocation("org.freedesktop.login1", session.session_path, "org.freedesktop.login1.Session", "Lock")) return false;
							if (!bus->EndInvocationVoid()) return false;
						}
					}
					return true;
				} else return false;
			} catch (...) { return false; }
		}
		bool PreventIdleSleep(bool prevent_system_sleep, bool prevent_display_sleep) noexcept
		{
			uint type;
			if (prevent_display_sleep) type = 2;
			else if (prevent_system_sleep) type = 1;
			else type = 0;
			Memory::AcquireRootLock();
			if (_system_lock_type == type) { Memory::ReleaseRootLock(); return true; }
			Memory::ReleaseRootLock();
			if (type) {
				ErrorContext ectx; ErrorClear(ectx);
				auto bus = DBus::IConnection::Query(DBus::BusType::SystemBus, ectx);
				if (ErrorTest(ectx)) return false;
				if (!bus->BeginInvocation("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "Inhibit")) return false;
				try {
					if (type == 2) { if (!bus->AddStringArgument(U"idle")) return false; }
					else if (type == 1) { if (!bus->AddStringArgument(U"sleep")) return false; }
					if (!bus->AddStringArgument(IO::Path::GetFileName(IO::GetExecutablePath()))) return false;
					if (!bus->AddStringArgument(U"")) return false;
					if (!bus->AddStringArgument(U"block")) return false;
				} catch (...) { return false; }
				handle lock;
				if (!bus->EndInvocationHandle(lock)) return false;
				Memory::AcquireRootLock();
				if (_system_lock_file != IO::InvalidHandle) IO::CloseHandle(_system_lock_file);
				_system_lock_file = lock;
				_system_lock_type = type;
				Memory::ReleaseRootLock();
				return true;
			} else {
				Memory::AcquireRootLock();
				if (_system_lock_file != IO::InvalidHandle) {
					IO::CloseHandle(_system_lock_file);
					_system_lock_file = IO::InvalidHandle;
					_system_lock_type = 0;
				}
				Memory::ReleaseRootLock();
				return true;
			}
		}
	}
}