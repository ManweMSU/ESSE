#pragma once

#include "CorString.h"

namespace ESSE
{
	constexpr uint32 RegularMonthLength[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	constexpr uint32 OddMonthLength[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	class Time
	{
	public:
		uint64 Ticks;

		Time(void) noexcept;
		Time(uint64 ticks) noexcept;
		Time(uint32 year, uint32 month, uint32 day, uint32 hour, uint32 minute, uint32 second, uint32 millisecond) noexcept;
		Time(uint32 hour, uint32 minute, uint32 second, uint32 millisecond) noexcept;

		bool friend operator == (Time a, Time b) noexcept;
		bool friend operator != (Time a, Time b) noexcept;
		bool friend operator < (Time a, Time b) noexcept;
		bool friend operator > (Time a, Time b) noexcept;
		bool friend operator <= (Time a, Time b) noexcept;
		bool friend operator >= (Time a, Time b) noexcept;

		Time friend operator + (Time a, Time b) noexcept;
		Time friend operator - (Time a, Time b)noexcept;

		operator uint64(void) const noexcept;
		Time & operator += (Time a) noexcept;
		Time & operator -= (Time a) noexcept;

		void GetDate(uint32 & year, uint32 & month, uint32 & day) const noexcept;
		uint32 GetYear(void) const noexcept;
		uint32 GetMonth(void) const noexcept;
		uint32 GetDay(void) const noexcept;
		uint32 GetHour(void) const noexcept;
		uint32 GetMinute(void) const noexcept;
		uint32 GetSecond(void) const noexcept;
		uint32 GetMillisecond(void) const noexcept;

		string ToString(void) const;
		string ToShortString(void) const;
		string ToDateString(void) const;

		uint64 ToWindowsTime(void) const noexcept;
		uint64 ToUnixTime(void) const noexcept;
		static Time FromWindowsTime(uint64 time) noexcept;
		static Time FromUnixTime(uint64 time) noexcept;

		static bool IsYearOdd(uint32 year) noexcept;
		static Time GetCurrentTime(void) noexcept;

		Time ToLocal(void) const noexcept;
		Time ToUniversal(void) const noexcept;

		int DayOfWeek(void) const noexcept;
	};
}