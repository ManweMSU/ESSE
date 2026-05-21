#include "../Interfaces/Console.h"

using namespace Engine::Streaming;

namespace Engine
{
	namespace IO
	{
		Console::Console(void) : _con(ESSE::IO::CreateConsole()), _event_pending(false) { eof = false; }
		Console::Console(handle output) : _con(ESSE::IO::CreateConsole(output)), _event_pending(false) { eof = false; }
		Console::Console(handle output, handle input) : _con(ESSE::IO::CreateConsole(output, input)), _event_pending(false) { eof = false; }
		Console::~Console(void) {}
		void Console::Write(const string & text) const { _con->Write(static_cast<const ESSE::unichar32 *>(text)); }
		void Console::WriteLine(const string & text) const { _con->WriteLine(static_cast<const ESSE::unichar32 *>(text)); }
		void Console::WriteEncodingSignature(void) const {}
		uint32 Console::ReadChar(void) const
		{
			ConsoleEventDesc ev;
			do { ReadEvent(ev); } while (ev.Event != ConsoleEvent::CharacterInput && ev.Event != ConsoleEvent::EndOfFile);
			if (ev.Event == ConsoleEvent::EndOfFile) {
				eof = true;
				return ESSE::Unicode::CharacterInvalid;
			} else return ev.CharacterCode;
		}
		void Console::ReadEvent(ConsoleEventDesc & event) const
		{
			ConsoleEventDesc ev2;
			ESSE::IO::ConsoleEventDesc ev;
			if (_event_pending) { ev = _event_buffer; _event_pending = false; } else _con->ReadEvent(ev);
			if (ev.event == ESSE::IO::ConsoleInputEvent::CharacterInput) {
				event.Event = ConsoleEvent::CharacterInput;
				event.CharacterCode = ev.character;
				if (ev.character == ESSE::Unicode::CharacterInvalid) eof = true;
			} else if (ev.event == ESSE::IO::ConsoleInputEvent::KeyInput) {
				event.Event = ConsoleEvent::KeyInput;
				event.KeyCode = ev.virtual_key_code;
				event.KeyFlags = ev.virtual_key_modifiers;
			} else if (ev.event == ESSE::IO::ConsoleInputEvent::ConsoleResized) {
				event.Event = ConsoleEvent::ConsoleResized;
				event.Width = ev.width;
				event.Height = ev.height;
			} else if (ev.event == ESSE::IO::ConsoleInputEvent::EndOfStream) {
				event.Event = ConsoleEvent::EndOfFile;
			} else throw ESSE::NotImplementedException();
		}
		bool Console::WaitEvent(uint timeout) const
		{
			if (_event_pending) return true;
			auto status = _con->WaitEvent(_event_buffer, timeout);
			if (status) _event_pending = true;
			return status;
		}
		void Console::WaitEvent(void) const
		{
			if (_event_pending) return;
			_con->ReadEvent(_event_buffer);
			_event_pending = true;
		}
		void Console::SetTextColor(int color) const { _con->SetTextColor(color); }
		void Console::SetTextColor(ConsoleColor color) const { _con->SetTextColor(int(color)); }
		void Console::SetBackgroundColor(int color) const { _con->SetBackgroundColor(color); }
		void Console::SetBackgroundColor(ConsoleColor color) const { _con->SetBackgroundColor(int(color)); }
		void Console::ClearScreen(void) const { _con->ClearScreen(); }
		void Console::ClearLine(void) const { _con->ClearLine(); }
		void Console::MoveCaret(int x, int y) const { _con->SetCaretPosition(ESSE::IO::ConsolePosition(x, y)); }
		void Console::SetTitle(const string & title) const { _con->Write(static_cast<const ESSE::unichar32 *>(title)); }
		void Console::SetInputMode(ConsoleInputMode mode) const
		{
			if (mode == ConsoleInputMode::Raw) _con->SetInputMode(ESSE::IO::ConsoleInputMode::Raw);
			else if (mode == ConsoleInputMode::Echo) _con->SetInputMode(ESSE::IO::ConsoleInputMode::Echo);
			else throw InvalidArgumentException();
		}
		void Console::AlternateScreenBuffer(bool alternate) const { _con->AlternateScreenBuffer(alternate); }
		void Console::GetCaretPosition(int & x, int & y) const { auto cp = _con->GetCaretPosition(); x = cp.x; y = cp.y; }
		void Console::GetScreenBufferDimensions(int & w, int & h) const { auto size = _con->GetDimensions(); w = size.x; h = size.y; }
		bool Console::IsConsoleDevice(void) const { return _con->IsConsoleDevice(); }
	}
}