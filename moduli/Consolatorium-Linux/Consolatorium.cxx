#include <Consolatorium/Consolatorium.h>
#include <Cor/CorVirtualKeyCodes.h>
#include <Cor/Tasks/CorThreads.h>
#include <Cor-Linux/CorIOEx.h>

#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace ESSE
{
	namespace IO
	{
		class SystemConsoleWriter : public ITextEncoder
		{
			oref<Console> _con;
		public:
			SystemConsoleWriter(Console * console) : _con(console) {}
			virtual ~SystemConsoleWriter(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Console"; ESSE_TRY_OUTRO(string()) }
			virtual void WriteE(const string & text, ErrorContext & ectx) noexcept override { _con->Write(text, ectx); }
			virtual void WriteLineE(const string & text, ErrorContext & ectx) noexcept override { _con->WriteLine(text, ectx); }
			virtual void LineFeedE(ErrorContext & ectx) noexcept override { _con->LineFeed(ectx); }
			virtual void WriteEncodingSignatureE(ErrorContext & ectx) noexcept override {}
		};
		class SystemConsoleReader : public ITextDecoder
		{
			bool _eos_faced;
			oref<Console> _con;
		public:
			SystemConsoleReader(Console * console) : _con(console), _eos_faced(false) {}
			virtual ~SystemConsoleReader(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Console"; ESSE_TRY_OUTRO(string()) }
			virtual unichar32 ReadCharacterE(ErrorContext & ectx) noexcept override
			{
				auto result = _con->ReadCharacter(ectx);
				if (ErrorTest(ectx)) return 0;
				if (result == Unicode::CharacterInvalid) _eos_faced = true;
				return result;
			}
			virtual string ReadLineE(ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					ErrorContext ectx; ErrorClear(ectx);
					dynamic_string_ucs4 result;
					unichar32 chr;
					do {
						chr = SystemConsoleReader::ReadCharacterE(ectx);
						ErrorThrow(ectx);
						if (chr != Unicode::CharacterInvalid && (chr >= 0x20 || chr == U'\t')) result += chr;
					} while (chr != Unicode::CharacterInvalid && chr != U'\n');
					return result;
				ESSE_TRY_OUTRO(string());
			}
			virtual string ReadAllE(ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					ErrorContext ectx; ErrorClear(ectx);
					dynamic_string_ucs4 result;
					unichar32 chr;
					do {
						chr = SystemConsoleReader::ReadCharacterE(ectx);
						ErrorThrow(ectx);
						if (chr != Unicode::CharacterInvalid) result += chr;
					} while (chr != Unicode::CharacterInvalid);
					return result;
				ESSE_TRY_OUTRO(string());
			}
			virtual bool IsEOF(void) noexcept override { return _eos_faced; }
		};
		class SystemConsole : public Console
		{
			static volatile bool _console_was_resized;
			static oref<IConsoleSessionEventHandler> _console_event_handler;
			static oref<Thread> _console_signal_handler;
		private:
			handle _input, _output;
			uintptr _unprocessed_input_pointer;
			array<unichar32> _unprocessed_input;
			uint _undecoded_input_size;
			unichar8 _undecoded_input[4];
			struct termios _echo_tio;
			bool _screen_buffer_alternated, _raw_mode_used;
		private:
			static void _console_size_changed(int) noexcept { _console_was_resized = true; }
			static int _console_signal_thread(void *) noexcept
			{
				sigset_t set;
				if (sigemptyset(&set) < 0) abort();
				if (sigaddset(&set, SIGINT) < 0) abort();
				if (sigaddset(&set, SIGQUIT) < 0) abort();
				if (sigaddset(&set, SIGHUP) < 0) abort();
				if (sigaddset(&set, SIGTERM) < 0) abort();
				while (true) {
					int number;
					auto status = sigwait(&set, &number);
					if (!status) {
						ConsoleSessionEvent ev;
						if (number == SIGINT) ev = ConsoleSessionEvent::ControlC;
						else if (number == SIGQUIT) ev = ConsoleSessionEvent::Quit;
						else if (number == SIGHUP) ev = ConsoleSessionEvent::ConsoleClosed;
						else if (number == SIGTERM) ev = ConsoleSessionEvent::Terminate;
						else continue;
						Memory::AcquireRootLock();
						auto hdlr = _console_event_handler;
						Memory::ReleaseRootLock();
						if (hdlr) hdlr->HandleConsoleSessionEvent(ev);
					} else if (status != EINTR) abort();
				}
				return 0;
			}
			bool _is_output_terminal(void) noexcept { return (isatty(reinterpret_cast<intptr>(_output)) > 0); }
			bool _is_input_terminal(void) noexcept { return (isatty(reinterpret_cast<intptr>(_input)) > 0); }
			bool _is_duplex_terminal(void) noexcept { return _is_output_terminal() && _is_input_terminal(); }
			void _write_raw(const unichar8 * text, uintptr length, ErrorContext & ectx) noexcept { IO::WriteFile(_output, text, length, ectx); }
			void _character_decode_low_level(unichar32 & chr, bool & interrupt, ErrorContext & ectx) noexcept
			{
				ErrorContext local;
				while (true) {
					ErrorClear(local);
					uintptr ptr = 0;
					auto chrr = Unicode::ReadCharacter(_undecoded_input, _undecoded_input_size, ptr, Unicode::Encoding::UTF8, local);
					if (!ErrorTest(local)) { chr = chrr; interrupt = false; _undecoded_input_size = 0; return; }
					if (_undecoded_input_size < 4) {
						int status = read(reinterpret_cast<intptr>(_input), _undecoded_input + _undecoded_input_size, 1);
						if (status > 0) { _undecoded_input_size++; }
						else if (status == 0) { chr = Unicode::CharacterInvalid; interrupt = false; _undecoded_input_size = 0; return; }
						else if (errno == EINTR) { chr = 0; interrupt = true; return; }
						else { Linux::ErrorSetPosix(ectx); return; }
					} else { ErrorSet(ectx, Errores::ErrorInvalidFormat); return; }
				}
			}
			void _character_decode_low_level_nointr(unichar32 & chr, ErrorContext & ectx) noexcept
			{
				bool interrupt;
				while (true) {
					_character_decode_low_level(chr, interrupt, ectx);
					if (ErrorTest(ectx) || !interrupt) return;
				}
			}
			void _character_read(unichar32 & chr, bool & interrupt, ErrorContext & ectx) noexcept
			{
				if (_unprocessed_input.GetLength() && _unprocessed_input_pointer < _unprocessed_input.GetLength()) {
					chr = _unprocessed_input[_unprocessed_input_pointer++];
					interrupt = false;
					if (_unprocessed_input_pointer == _unprocessed_input.GetLength()) {
						_unprocessed_input_pointer = 0;
						_unprocessed_input.Clear();
					}
				} else _character_decode_low_level(chr, interrupt, ectx);
			}
			void _character_read_nointr(unichar32 & chr, ErrorContext & ectx) noexcept
			{
				bool interrupt;
				while (true) {
					_character_read(chr, interrupt, ectx);
					if (ErrorTest(ectx) || !interrupt) return;
				}
			}
			bool _escaped_key_code_read(ConsoleEventDesc & event, ErrorContext & ectx) noexcept
			{
				uintptr w = 0;
				unichar32 escaped[0x11];
				_character_read_nointr(escaped[w++], ectx);
				if (ErrorTest(ectx)) return false;
				if (escaped[0] == U'O' || escaped[0] == U'[') {
					while (true) {
						if (w >= 0x10) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return false; }
						_character_read_nointr(escaped[w++], ectx);
						if (ErrorTest(ectx)) return false;
						auto chr = escaped[w - 1];
						if (chr >= U'A' && chr <= U'Z') break;
						else if (chr == U'~' || chr == Unicode::CharacterInvalid) { w--; break; }
					}
					escaped[w] = 0;
					event.event = ConsoleInputEvent::KeyInput;
					event.virtual_key_code = 0;
					event.virtual_key_modifiers = 0;
					if (escaped[0] == U'O') {
						if (Memory::StringCompare(escaped + 1, U"A") == 0) event.virtual_key_code = VirtualKeyCodes::Up;
						else if (Memory::StringCompare(escaped + 1, U"B") == 0) event.virtual_key_code = VirtualKeyCodes::Down;
						else if (Memory::StringCompare(escaped + 1, U"C") == 0) event.virtual_key_code = VirtualKeyCodes::Right;
						else if (Memory::StringCompare(escaped + 1, U"D") == 0) event.virtual_key_code = VirtualKeyCodes::Left;
						else if (Memory::StringCompare(escaped + 1, U"H") == 0) event.virtual_key_code = VirtualKeyCodes::Home;
						else if (Memory::StringCompare(escaped + 1, U"F") == 0) event.virtual_key_code = VirtualKeyCodes::End;
						else if (Memory::StringCompare(escaped + 1, U"P") == 0) event.virtual_key_code = VirtualKeyCodes::F1;
						else if (Memory::StringCompare(escaped + 1, U"Q") == 0) event.virtual_key_code = VirtualKeyCodes::F2;
						else if (Memory::StringCompare(escaped + 1, U"R") == 0) event.virtual_key_code = VirtualKeyCodes::F3;
						else if (Memory::StringCompare(escaped + 1, U"S") == 0) event.virtual_key_code = VirtualKeyCodes::F4;
						else return false;
					} else {
						if (Memory::StringCompare(escaped + 1, U"1") == 0) event.virtual_key_code = VirtualKeyCodes::Home;
						else if (Memory::StringCompare(escaped + 1, U"2") == 0) event.virtual_key_code = VirtualKeyCodes::Insert;
						else if (Memory::StringCompare(escaped + 1, U"3") == 0) event.virtual_key_code = VirtualKeyCodes::Delete;
						else if (Memory::StringCompare(escaped + 1, U"4") == 0) event.virtual_key_code = VirtualKeyCodes::End;
						else if (Memory::StringCompare(escaped + 1, U"5") == 0) event.virtual_key_code = VirtualKeyCodes::PageUp;
						else if (Memory::StringCompare(escaped + 1, U"6") == 0) event.virtual_key_code = VirtualKeyCodes::PageDown;
						else if (Memory::StringCompare(escaped + 1, U"7") == 0) event.virtual_key_code = VirtualKeyCodes::Home;
						else if (Memory::StringCompare(escaped + 1, U"8") == 0) event.virtual_key_code = VirtualKeyCodes::End;
						else if (Memory::StringCompare(escaped + 1, U"11") == 0) event.virtual_key_code = VirtualKeyCodes::F1;
						else if (Memory::StringCompare(escaped + 1, U"12") == 0) event.virtual_key_code = VirtualKeyCodes::F2;
						else if (Memory::StringCompare(escaped + 1, U"13") == 0) event.virtual_key_code = VirtualKeyCodes::F3;
						else if (Memory::StringCompare(escaped + 1, U"14") == 0) event.virtual_key_code = VirtualKeyCodes::F4;
						else if (Memory::StringCompare(escaped + 1, U"15") == 0) event.virtual_key_code = VirtualKeyCodes::F5;
						else if (Memory::StringCompare(escaped + 1, U"17") == 0) event.virtual_key_code = VirtualKeyCodes::F6;
						else if (Memory::StringCompare(escaped + 1, U"18") == 0) event.virtual_key_code = VirtualKeyCodes::F7;
						else if (Memory::StringCompare(escaped + 1, U"19") == 0) event.virtual_key_code = VirtualKeyCodes::F8;
						else if (Memory::StringCompare(escaped + 1, U"20") == 0) event.virtual_key_code = VirtualKeyCodes::F9;
						else if (Memory::StringCompare(escaped + 1, U"21") == 0) event.virtual_key_code = VirtualKeyCodes::F10;
						else if (Memory::StringCompare(escaped + 1, U"23") == 0) event.virtual_key_code = VirtualKeyCodes::F11;
						else if (Memory::StringCompare(escaped + 1, U"24") == 0) event.virtual_key_code = VirtualKeyCodes::F12;
						else if (Memory::StringCompare(escaped + 1, U"25") == 0) event.virtual_key_code = VirtualKeyCodes::F13;
						else if (Memory::StringCompare(escaped + 1, U"26") == 0) event.virtual_key_code = VirtualKeyCodes::F14;
						else if (Memory::StringCompare(escaped + 1, U"28") == 0) event.virtual_key_code = VirtualKeyCodes::F15;
						else if (Memory::StringCompare(escaped + 1, U"29") == 0) event.virtual_key_code = VirtualKeyCodes::F16;
						else if (Memory::StringCompare(escaped + 1, U"31") == 0) event.virtual_key_code = VirtualKeyCodes::F17;
						else if (Memory::StringCompare(escaped + 1, U"32") == 0) event.virtual_key_code = VirtualKeyCodes::F18;
						else if (Memory::StringCompare(escaped + 1, U"33") == 0) event.virtual_key_code = VirtualKeyCodes::F19;
						else if (Memory::StringCompare(escaped + 1, U"34") == 0) event.virtual_key_code = VirtualKeyCodes::F20;
						else if (Memory::StringCompare(escaped + 1, U"A") == 0) event.virtual_key_code = VirtualKeyCodes::Up;
						else if (Memory::StringCompare(escaped + 1, U"B") == 0) event.virtual_key_code = VirtualKeyCodes::Down;
						else if (Memory::StringCompare(escaped + 1, U"C") == 0) event.virtual_key_code = VirtualKeyCodes::Right;
						else if (Memory::StringCompare(escaped + 1, U"D") == 0) event.virtual_key_code = VirtualKeyCodes::Left;
						else if (Memory::StringCompare(escaped + 1, U"F") == 0) event.virtual_key_code = VirtualKeyCodes::End;
						else if (Memory::StringCompare(escaped + 1, U"H") == 0) event.virtual_key_code = VirtualKeyCodes::Home;
						else if (Memory::StringCompare(escaped + 1, U"1P") == 0) event.virtual_key_code = VirtualKeyCodes::F1;
						else if (Memory::StringCompare(escaped + 1, U"1Q") == 0) event.virtual_key_code = VirtualKeyCodes::F2;
						else if (Memory::StringCompare(escaped + 1, U"1R") == 0) event.virtual_key_code = VirtualKeyCodes::F3;
						else if (Memory::StringCompare(escaped + 1, U"1S") == 0) event.virtual_key_code = VirtualKeyCodes::F4;
						else if (Memory::StringCompare(escaped + 1, U"1;5A") == 0) { event.virtual_key_code = VirtualKeyCodes::Up; event.virtual_key_modifiers |= ConsoleVirtualKeyModifier::Control; }
						else if (Memory::StringCompare(escaped + 1, U"1;5B") == 0) { event.virtual_key_code = VirtualKeyCodes::Down; event.virtual_key_modifiers |= ConsoleVirtualKeyModifier::Control; }
						else if (Memory::StringCompare(escaped + 1, U"1;5C") == 0) { event.virtual_key_code = VirtualKeyCodes::Right; event.virtual_key_modifiers |= ConsoleVirtualKeyModifier::Control; }
						else if (Memory::StringCompare(escaped + 1, U"1;5D") == 0) { event.virtual_key_code = VirtualKeyCodes::Left; event.virtual_key_modifiers |= ConsoleVirtualKeyModifier::Control; }
						else return false;
					}
					return true;
				} else {
					if (_unprocessed_input_pointer) {
						_unprocessed_input[--_unprocessed_input_pointer] = escaped[0];
					} else try {
						_unprocessed_input.Insert(escaped[0], 0);
					} catch (...) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return false; }
					event.event == ConsoleInputEvent::KeyInput;
					event.virtual_key_code = VirtualKeyCodes::Escape;
					event.virtual_key_modifiers = 0;
					return true;
				}
			}
		public:
			SystemConsole(handle output, handle input) : _input(input), _output(output),
				_unprocessed_input_pointer(0), _unprocessed_input(0x100),
				_undecoded_input_size(0), _screen_buffer_alternated(false), _raw_mode_used(false)
			{ signal(SIGWINCH, _console_size_changed); siginterrupt(SIGWINCH, 1); }
			virtual ~SystemConsole(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Console"; ESSE_TRY_OUTRO(string()) }
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (cls == Classes.Object || cls == Classes.DynamicObject || cls == Classes.Console) {
						Retain(); return this;
					} else if (cls == Classes.ITextEncoder) {
						auto result = owrap(new SystemConsoleWriter(this));
						result->Retain(); return result.Inner();
					} else if (cls == Classes.ITextDecoder) {
						auto result = owrap(new SystemConsoleReader(this));
						result->Retain(); return result.Inner();
					} else { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
				ESSE_TRY_OUTRO(0)
			}
			virtual const void * GetType(void) noexcept override { return Classes.Console; }
			virtual void WriteRaw(const unichar32 * text, uintptr length, ErrorContext & ectx) noexcept override { ESSE_TRY_INTRO ucs1_string raw(text, length); _write_raw(raw.GetData(), raw.GetLength(), ectx); ESSE_TRY_OUTRO() }
			virtual void Write(const string & text, ErrorContext & ectx) noexcept override { WriteRaw(text.GetData(), text.GetLength(), ectx); }
			virtual void WriteLine(const string & text, ErrorContext & ectx) noexcept override { WriteRaw(text.GetData(), text.GetLength(), ectx); if (!ErrorTest(ectx)) LineFeed(ectx); }
			virtual void LineFeed(ErrorContext & ectx) noexcept override { if (_raw_mode_used) WriteFile(_output, "\n\r", 2, ectx); else WriteFile(_output, "\n", 1, ectx); }
			virtual void ReadEvent(ConsoleEventDesc & desc, ErrorContext & ectx) noexcept override
			{
				if (_is_input_terminal()) {
					while (true) {
						if (_console_was_resized) {
							_console_was_resized = false;
							auto size = GetDimensions(ectx);
							if (ErrorTest(ectx)) return;
							desc.event = ConsoleInputEvent::ConsoleResized;
							desc.width = size.x;
							desc.height = size.y;
							return;
						}
						unichar32 chr;
						bool interrupt;
						_character_read(chr, interrupt, ectx);
						if (ErrorTest(ectx)) return;
						if (interrupt) continue;
						if (chr == Unicode::CharacterInvalid) {
							desc.event = ConsoleInputEvent::EndOfStream;
							return;
						} else if (chr == 0) {
							desc.event = ConsoleInputEvent::KeyInput;
							desc.virtual_key_code = VirtualKeyCodes::Space;
							desc.virtual_key_modifiers = ConsoleVirtualKeyModifier::Control;
							return;
						} else if (chr == 9 || chr == 10 || chr == 13) {
							desc.event = ConsoleInputEvent::CharacterInput;
							desc.character = chr;
							return;
						} else if (chr < 27) {
							desc.event = ConsoleInputEvent::KeyInput;
							desc.virtual_key_code = 'A' + chr - 1;
							desc.virtual_key_modifiers = ConsoleVirtualKeyModifier::Control;
							return;
						} else if (chr == 27) {
							pollfd pfd;
							pfd.fd = reinterpret_cast<intptr>(_input);
							pfd.events = POLL_IN;
							if (poll(&pfd, 1, 0) == 0) {
								desc.event = ConsoleInputEvent::KeyInput;
								desc.virtual_key_code = VirtualKeyCodes::Escape;
								desc.virtual_key_modifiers = 0;
								return;
							} else {
								auto status = _escaped_key_code_read(desc, ectx);
								if (status || ErrorTest(ectx)) return;
							}
						} else if (chr == 0x7F) {
							desc.event = ConsoleInputEvent::KeyInput;
							desc.virtual_key_code = VirtualKeyCodes::Back;
							desc.virtual_key_modifiers = 0;
							return;
						} else if (chr >= 32) {
							desc.event = ConsoleInputEvent::CharacterInput;
							desc.character = chr;
							return;
						}
					}
				} else ErrorSet(ectx, Errores::ErrorNotImplemented);
			}
			virtual unichar32 ReadCharacter(ErrorContext & ectx) noexcept override
			{
				if (_is_input_terminal()) while (true) {
					ConsoleEventDesc ev;
					ReadEvent(ev, ectx);
					if (ErrorTest(ectx)) return 0;
					if (ev.event == ConsoleInputEvent::CharacterInput) return ev.character;
					else if (ev.event == ConsoleInputEvent::EndOfStream) return Unicode::CharacterInvalid;
				} else {
					unichar32 chr;
					_character_decode_low_level_nointr(chr, ectx);
					return chr;
				}
			}
			virtual string ReadLine(ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					ErrorContext ectx; ErrorClear(ectx);
					dynamic_string_ucs4 result;
					unichar32 chr;
					do {
						chr = ReadCharacter(ectx);
						ErrorThrow(ectx);
						if (chr != Unicode::CharacterInvalid && (chr >= 0x20 || chr == U'\t')) result += chr;
					} while (chr != Unicode::CharacterInvalid && chr != U'\n');
					return result;
				ESSE_TRY_OUTRO(string());
			}
			virtual void WaitEvent(void) noexcept override
			{
				if (!_is_input_terminal()) return;
				if (_console_was_resized) return;
				if (_unprocessed_input.GetLength() && _unprocessed_input_pointer < _unprocessed_input.GetLength()) return;
				pollfd pfd;
				pfd.fd = reinterpret_cast<intptr>(_input);
				pfd.events = POLLIN;
				while (true) {
					int status = poll(&pfd, 1, -1);
					if (status >= 0) break;
					if (errno == EINTR) { if (_console_was_resized) return; } else return;
				}
			}
			virtual bool WaitEventFor(uint32 ms) noexcept override
			{
				if (!_is_input_terminal()) return false;
				if (_console_was_resized) return true;
				if (_unprocessed_input.GetLength() && _unprocessed_input_pointer < _unprocessed_input.GetLength()) return true;
				pollfd pfd;
				pfd.fd = reinterpret_cast<intptr>(_input);
				pfd.events = POLLIN;
				while (true) {
					int status = poll(&pfd, 1, ms);
					if (status > 0) return true;
					else if (status == 0) return false;
					if (errno == EINTR) { if (_console_was_resized) return true; }
					else return false;
				}
			}
			virtual bool IsConsoleDevice(void) noexcept override { return _is_duplex_terminal(); }
			virtual void SetTitle(const string & title, ErrorContext & ectx) noexcept override
			{
				if (_is_output_terminal()) {
					_write_raw("\033]0;", 4, ectx);
					if (!ErrorTest(ectx)) Write(title, ectx);
					if (!ErrorTest(ectx)) _write_raw("\a", 1, ectx);
				}
			}
			virtual void SetTextColor(int color, ErrorContext & ectx) noexcept override
			{
				if (_is_output_terminal()) {
					if (color == -1) _write_raw("\033[39m", 5, ectx); // revert
					else if (color == 0) _write_raw("\033[38;5;0m", 9, ectx); // black
					else if (color == 1) _write_raw("\033[38;5;4m", 9, ectx); // dark blue
					else if (color == 2) _write_raw("\033[38;5;2m", 9, ectx); // dark green
					else if (color == 3) _write_raw("\033[38;5;6m", 9, ectx); // dark cyan
					else if (color == 4) _write_raw("\033[38;5;1m", 9, ectx); // dark red
					else if (color == 5) _write_raw("\033[38;5;5m", 9, ectx); // dark magenta
					else if (color == 6) _write_raw("\033[38;5;3m", 9, ectx); // dark yellow
					else if (color == 7) _write_raw("\033[38;5;7m", 9, ectx); // grey
					else if (color == 8) _write_raw("\033[38;5;8m", 9, ectx); // dark grey
					else if (color == 9) _write_raw("\033[38;5;12m", 10, ectx); // blue
					else if (color == 10) _write_raw("\033[38;5;10m", 10, ectx); // green
					else if (color == 11) _write_raw("\033[38;5;14m", 10, ectx); // cyan
					else if (color == 12) _write_raw("\033[38;5;9m", 9, ectx); // red
					else if (color == 13) _write_raw("\033[38;5;13m", 10, ectx); // magenta
					else if (color == 14) _write_raw("\033[38;5;11m", 10, ectx); // yellow
					else if (color == 15) _write_raw("\033[38;5;15m", 10, ectx); // white
				}
			}
			virtual void SetBackgroundColor(int color, ErrorContext & ectx) noexcept override
			{
				if (_is_output_terminal()) {
					if (color == -1) _write_raw("\033[49m", 5, ectx); // revert
					else if (color == 0) _write_raw("\033[48;5;0m", 9, ectx); // black
					else if (color == 1) _write_raw("\033[48;5;4m", 9, ectx); // dark blue
					else if (color == 2) _write_raw("\033[48;5;2m", 9, ectx); // dark green
					else if (color == 3) _write_raw("\033[48;5;6m", 9, ectx); // dark cyan
					else if (color == 4) _write_raw("\033[48;5;1m", 9, ectx); // dark red
					else if (color == 5) _write_raw("\033[48;5;5m", 9, ectx); // dark magenta
					else if (color == 6) _write_raw("\033[48;5;3m", 9, ectx); // dark yellow
					else if (color == 7) _write_raw("\033[48;5;7m", 9, ectx); // gray
					else if (color == 8) _write_raw("\033[48;5;8m", 9, ectx); // dark gray
					else if (color == 9) _write_raw("\033[48;5;12m", 10, ectx); // blue
					else if (color == 10) _write_raw("\033[48;5;10m", 10, ectx); // green
					else if (color == 11) _write_raw("\033[48;5;14m", 10, ectx); // cyan
					else if (color == 12) _write_raw("\033[48;5;9m", 9, ectx); // red
					else if (color == 13) _write_raw("\033[48;5;13m", 10, ectx); // magenta
					else if (color == 14) _write_raw("\033[48;5;11m", 10, ectx); // yellow
					else if (color == 15) _write_raw("\033[48;5;15m", 10, ectx); // white
				}
			}
			virtual ConsolePosition GetDimensions(ErrorContext & ectx) noexcept override
			{
				if (_is_output_terminal()) {
					winsize size;
					if (ioctl(reinterpret_cast<intptr>(_output), TIOCGWINSZ, &size) == -1) { Linux::ErrorSetPosix(ectx); return ConsolePosition(0, 0); }
					return ConsolePosition(size.ws_col, size.ws_row);
				} else return ConsolePosition(0, 0);
			}
			virtual ConsolePosition GetCaretPosition(ErrorContext & ectx) noexcept override
			{
				if (!_is_duplex_terminal()) return ConsolePosition(0, 0);
				ESSE_TRY_INTRO
					auto in = reinterpret_cast<intptr>(_input);
					tcdrain(reinterpret_cast<intptr>(_output));
					tcflush(in, TCIOFLUSH);
					struct termios preserve, raw;
					if (tcgetattr(in, &preserve) < 0) { Linux::ErrorSetPosix(ectx); return ConsolePosition(0, 0); }
					raw = preserve;
					cfmakeraw(&raw);
					if (tcsetattr(in, TCSANOW, &raw) < 0) { Linux::ErrorSetPosix(ectx); return ConsolePosition(0, 0); }
					_write_raw("\033[6n", 4, ectx);
					if (ErrorTest(ectx)) return ConsolePosition(0, 0);
					while (true) try {
						uintptr w = 0;
						unichar32 escaped[0x41];
						_character_decode_low_level_nointr(escaped[w++], ectx);
						if (ErrorTest(ectx)) return ConsolePosition(0, 0);
						if (escaped[0] == 27) {
							_character_decode_low_level_nointr(escaped[w++], ectx);
							if (ErrorTest(ectx)) return ConsolePosition(0, 0);
							if (escaped[1] == U'[') {
								while (true) {
									if (w >= 0x40) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return ConsolePosition(0, 0); }
									_character_decode_low_level_nointr(escaped[w++], ectx);
									if (ErrorTest(ectx)) return ConsolePosition(0, 0);
									auto chr = escaped[w - 1];
									if ((chr >= U'A' && chr <= U'Z') || chr == U'~') break;
									else if (chr == Unicode::CharacterInvalid) { w--; break; }
								}
								escaped[w] = 0;
								uintptr del = 0;
								for (uintptr i = 2; i < w; i++) if (escaped[i] == U';') { del = i; break; }
								if (escaped[w - 1] == U'R' && del) {
									auto y = string(escaped + 2, del - 2).ToUInt32();
									auto x = string(escaped + del + 1, w - del - 2).ToUInt32();
									if (tcsetattr(in, TCSANOW, &preserve) < 0) { Linux::ErrorSetPosix(ectx); return ConsolePosition(0, 0); }
									return ConsolePosition(x, y);
								} else _unprocessed_input.Append(escaped, w);
							} else _unprocessed_input.Append(escaped, 2);
						} else if (escaped[0] == Unicode::CharacterInvalid) {
							_unprocessed_input.Append(escaped[0]);
							if (tcsetattr(in, TCSANOW, &preserve) < 0) Linux::ErrorSetPosix(ectx);
							return ConsolePosition(0, 0);
						} else _unprocessed_input.Append(escaped[0]);
					} catch (...) { tcsetattr(in, TCSANOW, &preserve); throw; }
				ESSE_TRY_OUTRO(ConsolePosition(0, 0))
			}
			virtual void SetCaretPosition(const ConsolePosition & pos, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
				if (_is_output_terminal()) {
					ucs1_string cmd = U"\033[" + string(pos.y + 1) + U";" + string(pos.x + 1) + U"H";
					_write_raw(cmd.GetData(), cmd.GetLength(), ectx);
				}
				ESSE_TRY_OUTRO()
			}
			virtual void SetSessionEventMode(uint sevent_mask, IConsoleSessionEventHandler * hdlr, ErrorContext & ectx) noexcept override
			{
				Memory::AcquireRootLock();
				_console_event_handler = hdlr;
				if (sevent_mask && !_console_signal_handler) {
					_console_signal_handler = CreateThread(_console_signal_thread);
					if (!_console_signal_handler) { _console_event_handler.Clear(); ErrorSet(ectx, Errores::ErrorOutOfMemory); Memory::ReleaseRootLock(); return; }
				}
				sigset_t set;
				if (sigemptyset(&set) < 0) { Linux::ErrorSetPosix(ectx); _console_event_handler.Clear(); Memory::ReleaseRootLock(); return; }
				if (sevent_mask & ConsoleSessionEventMask::ControlC) {
					if (sigaddset(&set, SIGINT) < 0 || signal(SIGINT, SIG_IGN)) { Linux::ErrorSetPosix(ectx); _console_event_handler.Clear(); Memory::ReleaseRootLock(); return; }
				} else signal(SIGINT, SIG_DFL);
				if (sevent_mask & ConsoleSessionEventMask::Quit) {
					if (sigaddset(&set, SIGQUIT) < 0 || signal(SIGQUIT, SIG_IGN)) { Linux::ErrorSetPosix(ectx); _console_event_handler.Clear(); Memory::ReleaseRootLock(); return; }
				} else signal(SIGQUIT, SIG_DFL);
				if (sevent_mask & ConsoleSessionEventMask::ConsoleClosed) {
					if (sigaddset(&set, SIGHUP) < 0 || signal(SIGHUP, SIG_IGN)) { Linux::ErrorSetPosix(ectx); _console_event_handler.Clear(); Memory::ReleaseRootLock(); return; }
				} else signal(SIGHUP, SIG_DFL);
				if (sevent_mask & ConsoleSessionEventMask::Terminate) {
					if (sigaddset(&set, SIGTERM) < 0 || signal(SIGTERM, SIG_IGN)) { Linux::ErrorSetPosix(ectx); _console_event_handler.Clear(); Memory::ReleaseRootLock(); return; }
				} else signal(SIGTERM, SIG_DFL);
				if (sigprocmask(SIG_SETMASK, &set, 0) < 0) { Linux::ErrorSetPosix(ectx); _console_event_handler.Clear(); Memory::ReleaseRootLock(); return; }
				Memory::ReleaseRootLock();
			}
			virtual void SetInputMode(ConsoleInputMode mode, ErrorContext & ectx) noexcept override
			{
				if (_is_input_terminal()) {
					if (_is_output_terminal()) tcdrain(reinterpret_cast<intptr>(_output));
					int in = reinterpret_cast<intptr>(_input);
					if (mode == ConsoleInputMode::Raw && !_raw_mode_used) {
						struct termios tio_new;
						if (tcflush(in, TCIOFLUSH) < 0) { Linux::ErrorSetPosix(ectx); return; }
						if (tcgetattr(in, &_echo_tio) < 0) { Linux::ErrorSetPosix(ectx); return; }
						tio_new = _echo_tio;
						cfmakeraw(&tio_new);
						if (tcsetattr(in, TCSANOW, &tio_new) < 0) { Linux::ErrorSetPosix(ectx); return; }
						_raw_mode_used = true;
					} else if (mode == ConsoleInputMode::Echo && _raw_mode_used) {
						if (tcflush(in, TCIOFLUSH) < 0) { Linux::ErrorSetPosix(ectx); return; }
						if (tcsetattr(in, TCSANOW, &_echo_tio) < 0) { Linux::ErrorSetPosix(ectx); return; }
						_raw_mode_used = false;
					}
				}
			}
			virtual void AlternateScreenBuffer(bool alternate, ErrorContext & ectx) noexcept override
			{
				if (_is_output_terminal()) {
					tcdrain(reinterpret_cast<intptr>(_output));
					if (alternate && !_screen_buffer_alternated) {
						_write_raw("\033[?1049h", 8, ectx);
						if (!ErrorTest(ectx)) _screen_buffer_alternated = true;
					} else if (!alternate && _screen_buffer_alternated) {
						_write_raw("\033[?1049l", 8, ectx);
						if (!ErrorTest(ectx)) _screen_buffer_alternated = false;
					}
				}
			}
			virtual void ClearScreen(ErrorContext & ectx) noexcept override { if (_is_output_terminal()) _write_raw("\033[2J\033[1;1H", 10, ectx); }
			virtual void ClearLine(ErrorContext & ectx) noexcept override { if (_is_output_terminal()) _write_raw("\033[2K\033[1G", 8, ectx); }
		};
		volatile bool SystemConsole::_console_was_resized = false;
		oref<IConsoleSessionEventHandler> SystemConsole::_console_event_handler;
		oref<Thread> SystemConsole::_console_signal_handler;

		oref<Console> CreateConsole(ErrorContext & ectx) noexcept { ESSE_TRY_INTRO return oref<Console>::CreateOwned(new SystemConsole(GetStandardHandle(StandardHandleType::Output), GetStandardHandle(StandardHandleType::Input))); ESSE_TRY_OUTRO(0) }
		oref<Console> CreateConsole(handle output, ErrorContext & ectx) noexcept { ESSE_TRY_INTRO return oref<Console>::CreateOwned(new SystemConsole(output, GetStandardHandle(StandardHandleType::Input))); ESSE_TRY_OUTRO(0) }
		oref<Console> CreateConsole(handle output, handle input, ErrorContext & ectx) noexcept { ESSE_TRY_INTRO return oref<Console>::CreateOwned(new SystemConsole(output, input)); ESSE_TRY_OUTRO(0) }
	}
}