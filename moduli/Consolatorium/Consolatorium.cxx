#include "Consolatorium.h"

namespace ESSE
{
	namespace IO
	{
		ConsolePosition::ConsolePosition(void) noexcept {}
		ConsolePosition::ConsolePosition(uint sx, uint sy) noexcept : x(sx), y(sy) {}
		ConsolePosition::~ConsolePosition(void) {}

		Console::Control::Control(uint32 argument) noexcept : cc(argument) {}

		void Console::WriteRaw(const unichar32 * text, uintptr length) { ErrorContext ectx; ErrorClear(ectx); WriteRaw(text, length, ectx); ErrorThrow(ectx); }
		void Console::Write(const string & text) { ErrorContext ectx; ErrorClear(ectx); Write(text, ectx); ErrorThrow(ectx); }
		void Console::WriteLine(const string & text) { ErrorContext ectx; ErrorClear(ectx); WriteLine(text, ectx); ErrorThrow(ectx); }
		void Console::LineFeed(void) { ErrorContext ectx; ErrorClear(ectx); LineFeed(ectx); ErrorThrow(ectx); }
		void Console::ReadEvent(ConsoleEventDesc & desc) { ErrorContext ectx; ErrorClear(ectx); ReadEvent(desc, ectx); ErrorThrow(ectx); }
		unichar32 Console::ReadCharacter(void) { ErrorContext ectx; ErrorClear(ectx); auto result = ReadCharacter(ectx); ErrorThrow(ectx); return result; }
		string Console::ReadLine(void) { ErrorContext ectx; ErrorClear(ectx); auto result = ReadLine(ectx); ErrorThrow(ectx); return result; }
		void Console::SetTitle(const string & title) { ErrorContext ectx; ErrorClear(ectx); SetTitle(title, ectx); ErrorThrow(ectx); }
		void Console::SetTextColor(int color) { ErrorContext ectx; ErrorClear(ectx); SetTextColor(color, ectx); ErrorThrow(ectx); }
		void Console::SetTextColor(ConsoleColor color) { ErrorContext ectx; ErrorClear(ectx); SetTextColor(int(color), ectx); ErrorThrow(ectx); }
		void Console::SetBackgroundColor(int color) { ErrorContext ectx; ErrorClear(ectx); SetBackgroundColor(color, ectx); ErrorThrow(ectx); }
		void Console::SetBackgroundColor(ConsoleColor color) { ErrorContext ectx; ErrorClear(ectx); SetBackgroundColor(int(color), ectx); ErrorThrow(ectx); }
		ConsolePosition Console::GetDimensions(void) { ErrorContext ectx; ErrorClear(ectx); auto result = GetDimensions(ectx); ErrorThrow(ectx); return result; }
		ConsolePosition Console::GetCaretPosition(void) { ErrorContext ectx; ErrorClear(ectx); auto result = GetCaretPosition(ectx); ErrorThrow(ectx); return result; }
		void Console::SetCaretPosition(const ConsolePosition & pos) { ErrorContext ectx; ErrorClear(ectx); SetCaretPosition(pos, ectx); ErrorThrow(ectx); }
		void Console::SetSessionEventMode(uint sevent_mask, IConsoleSessionEventHandler * hdlr) { ErrorContext ectx; ErrorClear(ectx); SetSessionEventMode(sevent_mask, hdlr, ectx); ErrorThrow(ectx); }
		void Console::SetInputMode(ConsoleInputMode mode) { ErrorContext ectx; ErrorClear(ectx); SetInputMode(mode, ectx); ErrorThrow(ectx); }
		void Console::AlternateScreenBuffer(bool alternate) { ErrorContext ectx; ErrorClear(ectx); AlternateScreenBuffer(alternate, ectx); ErrorThrow(ectx); }
		void Console::ClearScreen(void) { ErrorContext ectx; ErrorClear(ectx); ClearScreen(ectx); ErrorThrow(ectx); }
		void Console::ClearLine(void) { ErrorContext ectx; ErrorClear(ectx); ClearLine(ectx); ErrorThrow(ectx); }
		void Console::WriteFormatted(const string & text)
		{
			auto data = text.GetData();
			auto length = text.GetLength();
			uintptr s = 0, i = 0;
			while (i < length) {
				if (data[i] == U'\33') {
					if (i != s) WriteRaw(data + s, i - s);
					auto cf = data[i + 1];
					auto cb = cf ? data[i + 2] : 0;
					if (cf >= U'0' && cf <= U'9') SetTextColor(cf - U'0');
					else if (cf >= U'A' && cf <= U'F') SetTextColor(10 + cf - U'A');
					else if (cf >= U'a' && cf <= U'f') SetTextColor(10 + cf - U'a');
					else if (cf == U'-') SetTextColor(-1);
					else if (cf != U'*') throw InvalidFormatException();
					if (cb >= U'0' && cb <= U'9') SetBackgroundColor(cb - U'0');
					else if (cb >= U'A' && cb <= U'F') SetBackgroundColor(10 + cb - U'A');
					else if (cb >= U'a' && cb <= U'f') SetBackgroundColor(10 + cb - U'a');
					else if (cb == U'-') SetBackgroundColor(-1);
					else if (cb != U'*') throw InvalidFormatException();
					s = i += 3;
				} else i++;
			}
			if (i != s) WriteRaw(data + s, i - s);
		}
		void Console::WriteLineFormatted(const string & text) { WriteFormatted(text); LineFeed(); }

		Console::Control ConsoleControl::LineFeed(void) noexcept { return Console::Control(0); }
		Console::Control ConsoleControl::TextColor(int color) noexcept { return Console::Control(0x00010000 | color); }
		Console::Control ConsoleControl::TextColor(ConsoleColor color) noexcept { return Console::Control(0x00010000 | uint16(color)); }
		Console::Control ConsoleControl::TextColorDefault(void) noexcept { return Console::Control(0x0001FFFF); }
		Console::Control ConsoleControl::TextBackground(int color) noexcept { return Console::Control(0x00020000 | color); }
		Console::Control ConsoleControl::TextBackground(ConsoleColor color) noexcept { return Console::Control(0x00020000 | uint16(color)); }
		Console::Control ConsoleControl::TextBackgroundDefault(void) noexcept { return Console::Control(0x0002FFFF); }

		Console & operator << (Console & console, const string & text) { console.Write(text); return console; }
		Console & operator << (Console & console, Console::Control ctl)
		{
			auto cmd = ctl.cc & 0xFFFF0000;
			if (cmd == 0) {
				console.LineFeed();
			} else if (cmd == 0x00010000) {
				auto clr = ctl.cc  & 0xFFFF;
				if (clr == 0xFFFF) console.SetTextColor(-1);
				else console.SetTextColor(clr);
			} else if (cmd == 0x00020000) {
				auto clr = ctl.cc  & 0xFFFF;
				if (clr == 0xFFFF) console.SetBackgroundColor(-1);
				else console.SetBackgroundColor(clr);
			}
			return console;
		}
		Console * operator << (Console * console, const string & text) { return &(*console << text); }
		Console * operator << (Console * console, Console::Control ctl) { return &(*console << ctl); }
		Console & operator >> (Console & console, string & text) { text = console.ReadLine(); return console; }
		Console & operator >> (Console & console, ConsoleEventDesc & desc) { console.ReadEvent(desc); return console; }
		Console * operator >> (Console * console, string & text) { return &(*console >> text); }
		Console * operator >> (Console * console, ConsoleEventDesc & desc) { return &(*console >> desc); }

		oref<Console> CreateConsole(void) { ErrorContext ectx; ErrorClear(ectx); auto result = CreateConsole(ectx); ErrorThrow(ectx); return result; }
		oref<Console> CreateConsole(handle output) { ErrorContext ectx; ErrorClear(ectx); auto result = CreateConsole(output, ectx); ErrorThrow(ectx); return result; }
		oref<Console> CreateConsole(handle output, handle input) { ErrorContext ectx; ErrorClear(ectx); auto result = CreateConsole(output, input, ectx); ErrorThrow(ectx); return result; }
	}
}