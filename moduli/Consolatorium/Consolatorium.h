#pragma once

#include <Cor/IO/CorStreams.h>

namespace ESSE
{
	namespace IO
	{
		enum class ConsoleColor : int {
			Default = -1, Black = 0, DarkBlue = 1, DarkGreen = 2, DarkCyan = 3,
			DarkRed = 4, DarkMagenta = 5, DarkYellow = 6, Gray = 7,
			DarkGray = 8, Blue = 9, Green = 10, Cyan = 11,
			Red = 12, Magenta = 13, Yellow = 14, White = 15
		};
		enum class ConsoleInputMode : uint { Raw = 0, Echo = 1 };
		enum class ConsoleInputEvent : uint { EndOfStream = 0, CharacterInput = 1, KeyInput = 2, ConsoleResized = 3 };
		enum class ConsoleSessionEvent : uint { ControlC = 0, Quit = 1, ConsoleClosed = 2, Terminate = 3 };
		namespace ConsoleVirtualKeyModifier { enum ConsoleVirtualKeyModifiers : uint {
			Shift		= 0x01,
			Control		= 0x02,
			Alternate	= 0x04
		}; }
		namespace ConsoleSessionEventMask { enum ConsoleSessionEventMasks : uint {
			ControlC		= 0x01,
			Quit			= 0x02,
			ConsoleClosed	= 0x04,
			Terminate		= 0x08
		}; }

		struct ConsolePosition
		{
			uint x, y;
			ConsolePosition(void) noexcept;
			ConsolePosition(uint sx, uint sy) noexcept;
			~ConsolePosition(void);
		};
		struct ConsoleEventDesc
		{
			ConsoleInputEvent event;
			union {
				struct { unichar32 character; };
				struct { uint virtual_key_code, virtual_key_modifiers; };
				struct { uint width, height; };
			};
		};
		class IConsoleSessionEventHandler : public Object
		{
		public:
			virtual void HandleConsoleSessionEvent(ConsoleSessionEvent event) noexcept = 0;
		};
		
		class Console : public DynamicObject
		{
		public:
			virtual void WriteRaw(const unichar32 * text, uintptr length, ErrorContext & ectx) noexcept = 0;
			virtual void Write(const string & text, ErrorContext & ectx) noexcept = 0;
			virtual void WriteLine(const string & text, ErrorContext & ectx) noexcept = 0;
			virtual void LineFeed(ErrorContext & ectx) noexcept = 0;

			virtual void ReadEvent(ConsoleEventDesc & desc, ErrorContext & ectx) noexcept = 0;
			virtual unichar32 ReadCharacter(ErrorContext & ectx) noexcept = 0;
			virtual string ReadLine(ErrorContext & ectx) noexcept = 0;

			virtual void WaitEvent(void) noexcept = 0;
			virtual bool WaitEventFor(uint32 ms) noexcept = 0;

			virtual bool IsConsoleDevice(void) noexcept = 0;
			virtual void SetTitle(const string & title, ErrorContext & ectx) noexcept = 0;
			virtual void SetTextColor(int color, ErrorContext & ectx) noexcept = 0;
			virtual void SetBackgroundColor(int color, ErrorContext & ectx) noexcept = 0;
			virtual ConsolePosition GetDimensions(ErrorContext & ectx) noexcept = 0;
			virtual ConsolePosition GetCaretPosition(ErrorContext & ectx) noexcept = 0;
			virtual void SetCaretPosition(const ConsolePosition & pos, ErrorContext & ectx) noexcept = 0;

			virtual void SetSessionEventMode(uint sevent_mask, IConsoleSessionEventHandler * hdlr, ErrorContext & ectx) noexcept = 0;
			virtual void SetInputMode(ConsoleInputMode mode, ErrorContext & ectx) noexcept = 0;
			virtual void AlternateScreenBuffer(bool alternate, ErrorContext & ectx) noexcept = 0;
			virtual void ClearScreen(ErrorContext & ectx) noexcept = 0;
			virtual void ClearLine(ErrorContext & ectx) noexcept = 0;

			void WriteRaw(const unichar32 * text, uintptr length);
			void Write(const string & text);
			void WriteLine(const string & text);
			void LineFeed(void);
			void ReadEvent(ConsoleEventDesc & desc);
			unichar32 ReadCharacter(void);
			string ReadLine(void);
			void SetTitle(const string & title);
			void SetTextColor(int color);
			void SetTextColor(ConsoleColor color);
			void SetBackgroundColor(int color);
			void SetBackgroundColor(ConsoleColor color);
			ConsolePosition GetDimensions(void);
			ConsolePosition GetCaretPosition(void);
			void SetCaretPosition(const ConsolePosition & pos);
			void SetSessionEventMode(uint sevent_mask, IConsoleSessionEventHandler * hdlr);
			void SetInputMode(ConsoleInputMode mode);
			void AlternateScreenBuffer(bool alternate);
			void ClearScreen(void);
			void ClearLine(void);

			void WriteFormatted(const string & text);
			void WriteLineFormatted(const string & text);

			struct Control { uint32 cc; explicit Control(uint32 argument) noexcept; };
		};

		oref<Console> CreateConsole(ErrorContext & ectx) noexcept;
		oref<Console> CreateConsole(handle output, ErrorContext & ectx) noexcept;
		oref<Console> CreateConsole(handle output, handle input, ErrorContext & ectx) noexcept;

		namespace ConsoleControl
		{
			Console::Control LineFeed(void) noexcept;
			Console::Control TextColor(int color) noexcept;
			Console::Control TextColor(ConsoleColor color) noexcept;
			Console::Control TextColorDefault(void) noexcept;
			Console::Control TextBackground(int color) noexcept;
			Console::Control TextBackground(ConsoleColor color) noexcept;
			Console::Control TextBackgroundDefault(void) noexcept;
		}

		Console & operator << (Console & console, const string & text);
		Console & operator << (Console & console, Console::Control ctl);
		Console * operator << (Console * console, const string & text);
		Console * operator << (Console * console, Console::Control ctl);
		Console & operator >> (Console & console, string & text);
		Console & operator >> (Console & console, ConsoleEventDesc & desc);
		Console * operator >> (Console * console, string & text);
		Console * operator >> (Console * console, ConsoleEventDesc & desc);

		oref<Console> CreateConsole(void);
		oref<Console> CreateConsole(handle output);
		oref<Console> CreateConsole(handle output, handle input);
	}
}