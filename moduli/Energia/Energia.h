#pragma once

#include <Cor/CorBasis.h>

namespace ESSE
{
	namespace Power
	{
		enum class PowerStatus : uint { Unknown = 0, LinePower = 1, BatteryCharging = 2, BatteryDischarging = 3 };
		enum class PowerControl : uint { Shutdown = 0, Reboot = 1, LockSession = 2, EndSession = 3, Suspend = 4, Hibernate = 5 };
		struct PowerStatusDesc { PowerStatus status; double battery_charge_level; };

		void GetPowerStatus(PowerStatusDesc & desc) noexcept;
		bool SystemPowerControl(PowerControl control, bool enforce) noexcept;
		bool PreventIdleSleep(bool prevent_system_sleep, bool prevent_display_sleep) noexcept;
	}
}