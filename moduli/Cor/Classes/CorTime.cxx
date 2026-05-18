#include "CorTime.h"
#include "../CorSystemInformation.h"

namespace ESSE
{
	Time::Time(void) noexcept {}
	Time::Time(uint64 ticks) noexcept : Ticks(ticks) {}
	Time::Time(uint32 year, uint32 month, uint32 day, uint32 hour, uint32 minute, uint32 second, uint32 millisecond) noexcept
	{
		if (year < 1) year = 1;
		if (month < 1) month = 1;
		if (day < 1) day = 1;
		uint64 base = millisecond + (second + (minute + hour * 60) * 60) * 1000;
		uint64 days = day - 1;
		if (IsYearOdd(year)) {
			for (uint i = 1; i < month; i++) days += OddMonthLength[i - 1];
		} else {
			for (uint i = 1; i < month; i++) days += RegularMonthLength[i - 1];
		}
		uint32 raw_year = year - 1;
		uint32 cycles = raw_year / 400;
		days += cycles * (365 * 303 + 366 * 97);
		for (uint y = cycles * 400; y < raw_year; y++) days += IsYearOdd(y + 1) ? 366 : 365;
		Ticks = base + days * 1000 * 60 * 60 * 24;
	}
	Time::Time(uint32 hour, uint32 minute, uint32 second, uint32 millisecond) noexcept { Ticks = millisecond + (second + (minute + hour * 60) * 60) * 1000; }
	bool operator==(Time a, Time b) noexcept { return a.Ticks == b.Ticks; }
	bool operator!=(Time a, Time b) noexcept { return a.Ticks != b.Ticks; }
	bool operator<(Time a, Time b) noexcept { return a.Ticks < b.Ticks; }
	bool operator>(Time a, Time b) noexcept { return a.Ticks > b.Ticks; }
	bool operator<=(Time a, Time b) noexcept { return a.Ticks <= b.Ticks; }
	bool operator>=(Time a, Time b) noexcept { return a.Ticks >= b.Ticks; }
	Time operator+(Time a, Time b) noexcept { return Time(a.Ticks + b.Ticks); }
	Time operator-(Time a, Time b) noexcept { return Time(a.Ticks - b.Ticks); }
	Time::operator uint64(void) const noexcept { return Ticks; }
	Time & Time::operator+=(Time a) noexcept { Ticks += a.Ticks; return *this; }
	Time & Time::operator-=(Time a) noexcept { Ticks -= a.Ticks; return *this; }
	void Time::GetDate(uint32 & year, uint32 & month, uint32 & day) const noexcept
	{
		year = 1;
		month = 1;
		day = 1;
		uint64 days = Ticks / (1000 * 60 * 60 * 24);
		uint64 cycles = days / (365 * 303 + 366 * 97);
		days -= cycles * (365 * 303 + 366 * 97);
		year += uint32(cycles * 400);
		while (true) {
			uint32 len = IsYearOdd(year) ? 366 : 365;
			if (days < len) break;
			year++;
			days -= len;
		}
		if (IsYearOdd(year)) {
			while (days + 1 > OddMonthLength[month - 1]) {
				days -= OddMonthLength[month - 1];
				month++;
			}
		} else {
			while (days + 1 > RegularMonthLength[month - 1]) {
				days -= RegularMonthLength[month - 1];
				month++;
			}
		}
		day += uint32(days);
	}
	uint32 Time::GetYear(void) const noexcept { uint32 d, m, y; GetDate(y, m, d); return y; }
	uint32 Time::GetMonth(void) const noexcept { uint32 d, m, y; GetDate(y, m, d); return m; }
	uint32 Time::GetDay(void) const noexcept { uint32 d, m, y; GetDate(y, m, d); return d; }
	uint32 Time::GetHour(void) const noexcept { return (((Ticks / 1000) / 60) / 60) % 24; }
	uint32 Time::GetMinute(void) const noexcept { return ((Ticks / 1000) / 60) % 60; }
	uint32 Time::GetSecond(void) const noexcept { return (Ticks / 1000) % 60; }
	uint32 Time::GetMillisecond(void) const noexcept { return Ticks % 1000; }
	string Time::ToString(void) const
	{
		return string(GetDay(), DecimalBase, 2) + U"." + string(GetMonth(), DecimalBase, 2) + U"." + string(GetYear(), DecimalBase, 4) +
			U" " + string(GetHour(), DecimalBase, 2) + U":" + string(GetMinute(), DecimalBase, 2) + U":" + string(GetSecond(), DecimalBase, 2);
	}
	string Time::ToShortString(void) const { return string(GetHour(), DecimalBase, 2) + U":" + string(GetMinute(), DecimalBase, 2) + U":" + string(GetSecond(), DecimalBase, 2); }
	string Time::ToDateString(void) const { return string(GetDay(), DecimalBase, 2) + U"." + string(GetMonth(), DecimalBase, 2) + U"." + string(GetYear(), DecimalBase, 4); }
	uint64 Time::ToWindowsTime(void) const noexcept { return (Ticks - Time(1601, 1, 1, 0, 0, 0, 0).Ticks); }
	uint64 Time::ToUnixTime(void) const noexcept { return (Ticks - Time(1970, 1, 1, 0, 0, 0, 0).Ticks); }
	Time Time::FromWindowsTime(uint64 time) noexcept { return time + Time(1601, 1, 1, 0, 0, 0, 0).Ticks; }
	Time Time::FromUnixTime(uint64 time) noexcept { return time + Time(1970, 1, 1, 0, 0, 0, 0).Ticks; }
	bool Time::IsYearOdd(uint32 year) noexcept
	{
		if (year % 400 == 0) return true;
		else if (year % 100 == 0) return false;
		else if (year % 4 == 0) return true;
		else return false;
	}
	Time Time::GetCurrentTime(void) noexcept
	{
		#if defined(ESSE_SYSTEMA_WINDOWS)
			return FromWindowsTime(System::GetSystemTime());
		#elif defined(ESSE_SYSTEMA_UNIX)
			return FromUnixTime(System::GetSystemTime());
		#else
			return 0;
		#endif
	}
	Time Time::ToLocal(void) const noexcept
	{
		#if defined(ESSE_SYSTEMA_WINDOWS)
			return FromWindowsTime(System::TimeConvertToLocal(ToWindowsTime()));
		#elif defined(ESSE_SYSTEMA_UNIX)
			return FromUnixTime(System::TimeConvertToLocal(ToUnixTime()));
		#else
			return *this;
		#endif
	}
	Time Time::ToUniversal(void) const noexcept
	{
		#if defined(ESSE_SYSTEMA_WINDOWS)
			return FromWindowsTime(System::TimeConvertToUniversal(ToWindowsTime()));
		#elif defined(ESSE_SYSTEMA_UNIX)
			return FromUnixTime(System::TimeConvertToUniversal(ToUnixTime()));
		#else
			return *this;
		#endif
	}
	int Time::DayOfWeek(void) const noexcept
	{
		uint64 days = Ticks / (1000 * 60 * 60 * 24);
		return int(days % 7);
	}
}