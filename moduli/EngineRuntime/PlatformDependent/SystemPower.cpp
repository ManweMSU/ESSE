#include "../Interfaces/SystemPower.h"

namespace Engine
{
	namespace Power
	{
		BatteryStatus GetBatteryStatus(void) { return BatteryStatus::Unknown; }
		double GetBatteryChargeLevel(void) { return 0.0; }
		void PreventIdleSleep(Prevent prevent) {}
		bool ExitSystem(Exit exit, bool forced) { return false; }
		bool SuspendSystem(bool hibernate, bool allow_wakeup) { return false; }
	}
}