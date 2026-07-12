#include "../Interfaces/SystemPower.h"
#include <Energia/Energia.h>

namespace Engine
{
	namespace Power
	{
		BatteryStatus GetBatteryStatus(void)
		{
			ESSE::Power::PowerStatusDesc status;
			ESSE::Power::GetPowerStatus(status);
			if (status.status == ESSE::Power::PowerStatus::LinePower) return BatteryStatus::NoBattery;
			else if (status.status == ESSE::Power::PowerStatus::BatteryCharging) return BatteryStatus::Charging;
			else if (status.status == ESSE::Power::PowerStatus::BatteryDischarging) return BatteryStatus::InUse;
			else return BatteryStatus::Unknown;
		}
		double GetBatteryChargeLevel(void)
		{
			ESSE::Power::PowerStatusDesc status;
			ESSE::Power::GetPowerStatus(status);
			return status.battery_charge_level;
		}
		void PreventIdleSleep(Prevent prevent)
		{
			if (prevent == Prevent::IdleDisplaySleep) ESSE::Power::PreventIdleSleep(true, true);
			else if (prevent == Prevent::IdleSystemSleep) ESSE::Power::PreventIdleSleep(true, false);
			else if (prevent == Prevent::None) ESSE::Power::PreventIdleSleep(false, false);
		}
		bool ExitSystem(Exit exit, bool forced)
		{
			if (exit == Exit::Shutdown) return ESSE::Power::SystemPowerControl(ESSE::Power::PowerControl::Shutdown, forced);
			else if (exit == Exit::Reboot) return ESSE::Power::SystemPowerControl(ESSE::Power::PowerControl::Reboot, forced);
			else if (exit == Exit::Logout) return ESSE::Power::SystemPowerControl(ESSE::Power::PowerControl::EndSession, forced);
			else return false;
		}
		bool SuspendSystem(bool hibernate, bool allow_wakeup)
		{
			if (hibernate) return ESSE::Power::SystemPowerControl(ESSE::Power::PowerControl::Hibernate, !allow_wakeup);
			else return ESSE::Power::SystemPowerControl(ESSE::Power::PowerControl::Suspend, !allow_wakeup);
		}
	}
}