#include "X11Dispatch.h"
#include <Cor/CorSystemInformation.h>
#include <Cor/CorVirtualKeyCodes.h>
#include <poll.h>
#include <errno.h>
#include <linux/input-event-codes.h>

namespace ESSE
{
	namespace X11
	{
		XServerConnection::XServerConnection(XLibAPI * xlib_api) noexcept : _xlib_api(xlib_api)
		{
			_display = _xlib_api->XOpenDisplay(0);
			if (!_display) _display = _xlib_api->XOpenDisplay(":0");
			if (!_display) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
		}
		XServerConnection::~XServerConnection(void) { _xlib_api->XCloseDisplay(_display); }
		Display * XServerConnection::GetXDisplay(void) const noexcept { return _display; }
		XLibAPI * XServerConnection::GetAPI(void) const noexcept { return _xlib_api; }
		oref<XServerConnection> XServerConnection::Query(ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			auto api = owrap(new XLibAPI);
			return owrap(new XServerConnection(api));
			ESSE_TRY_OUTRO(0)
		}

		Bool XDispatch::_peek_predicate(Display * display, XEvent * event, XPointer arg) noexcept { return event->type == int(intptr(arg)); }
		void XDispatch::_set_sources_updated(void) noexcept { for (auto & c : _context_stack) c.sources_updated_flag = true; }
		bool XDispatch::_silent_window_append(Window window, IXWindowEventHandler * handler) noexcept { try { return _window_inputs.Append(window, handler); } catch (...) { return false; } }
		XDispatch::XDispatch(XServerConnection * con) : _context_stack(4), _file_inputs(0x20), _timers(0x40), _con(con), _api(con->GetAPI()), _current_context(0) {}
		XDispatch::~XDispatch(void) {}
		bool XDispatch::RegisterWindowHandler(Window window, IXWindowEventHandler * handler) noexcept
		{
			auto result = _silent_window_append(window, handler);
			if (result) _set_sources_updated();
			return result;
		}
		void XDispatch::UnregisterWindowHandler(Window window) noexcept
		{
			_window_inputs.Remove(window);
			_dirty_windows.RemoveElement(window);
			for (uintptr i = _timers.GetLength() - 1; i < _timers.GetLength(); i--) if (_timers[i].window == window) _timers.Remove(i);
			_set_sources_updated();
		}
		bool XDispatch::RegisterFileHandler(int file, IXFileEventHandler * handler) noexcept
		{
			try {
				_file_input in;
				in.file = file;
				in.handler = handler;
				_file_inputs << in;
				_set_sources_updated();
				return true;
			} catch (...) { return false; }
		}
		void XDispatch::UnregisterFileHandler(int file) noexcept
		{
			for (uintptr i = 0; i < _file_inputs.GetLength(); i++) if (_file_inputs[i].file == file) {
				_file_inputs.Remove(i);
				_set_sources_updated();
				break;
			}
		}
		bool XDispatch::CreateTimer(Window window, int timer, uint period) noexcept
		{
			try {
				_timer_input t;
				t.window = window;
				t.timer_id = timer;
				t.period = period;
				t.last_fired = System::GetMonotonicTime();
				_timers << t;
				_set_sources_updated();
				return true;
			} catch (...) { return false; }
		}
		void XDispatch::DestroyTimer(Window window, int timer) noexcept
		{
			for (uintptr i = 0; i < _timers.GetLength(); i++) if (_timers[i].window == window && _timers[i].timer_id == timer) {
				_timers.Remove(i);
				_set_sources_updated();
				break;
			}
		}
		bool XDispatch::RunEventLoop(void) noexcept
		{
			try {
				_context_stack << _loop_context { .exit_flag = false, .sources_updated_flag = true };
				_current_context = &_context_stack.LastElement();
			} catch (...) { return false; }
			try {
				bool update_timer = false;
				array<pollfd> poll_array(0x80);
				intptr nearest_timer = -1;
				intptr timeout, poll_result;
				uint date_fire;
				XEvent event;
				while (!_current_context->exit_flag) {
					uint date_now = System::GetMonotonicTime();
					if (_current_context->sources_updated_flag) {
						_current_context->sources_updated_flag = false;
						update_timer = true;
						poll_array.SetLength(1 + _file_inputs.GetLength());
						poll_array[0].fd = _api->XConnectionNumber(_con->GetXDisplay());
						poll_array[0].events = POLLIN;
						for (intptr i = 0; i < _file_inputs.GetLength(); i++) {
							poll_array[i + 1].fd = _file_inputs[i].file;
							poll_array[i + 1].events = POLLIN;
						}
					}
					if (update_timer) {
						update_timer = false;
						timeout = nearest_timer = -1;
						for (intptr i = 0; i < _timers.GetLength(); i++) {
							intptr cto = _timers[i].last_fired + _timers[i].period - date_now;
							if (cto < 0) cto = 0;
							if (timeout == -1 || timeout < cto) {
								timeout = cto;
								nearest_timer = i;
								date_fire = _timers[i].last_fired + _timers[i].period;
							}
						}
					}
					if (nearest_timer >= 0) {
						timeout = date_fire - date_now;
						if (timeout < 0) timeout = 0;
					} else timeout = -1;
					if (_api->XPending(_con->GetXDisplay())) {
						poll_result = 1;
						poll_array[0].revents = POLLIN;
						for (uintptr i = 1; i < poll_array.GetLength(); i++) poll_array[i].revents = 0;
					} else {
						poll_result = poll(poll_array.GetBuffer(), poll_array.GetLength(), timeout);
					}
					if (poll_result > 0) {
						if (poll_array[0].revents) {
							_api->XNextEvent(_con->GetXDisplay(), &event);
							if (event.type == MappingNotify) _api->XRefreshKeyboardMapping(&event.xmapping);
							else if (event.type == Expose) {
								auto responder = _window_inputs[event.xany.window];
								if (responder) ScheduleWindowUpdate(event.xany.window);
							} else {
								auto responder = _window_inputs[event.xany.window];
								if (responder) (*responder)->HandleEvent(event.xany.window, &event);
							}
						}
						for (uintptr i = 1; i < poll_array.GetLength(); i++) if (poll_array[i].revents) {
							_file_inputs[i - 1].handler->HandleFile(_file_inputs[i - 1].file);
							if (_current_context->sources_updated_flag) break;
						}
					} else if (poll_result == 0) {
						if (nearest_timer >= 0) {
							update_timer = true;
							date_now = System::GetMonotonicTime();
							for (auto & timer : _timers) if (date_now - timer.last_fired >= timer.period) {
								timer.last_fired += timer.period;
								auto responder = _window_inputs[timer.window];
								if (responder) (*responder)->HandleTimer(timer.window, timer.timer_id);
							}
						}
					} else if (poll_result == -1) {
						if (errno == EINTR) continue; else throw Exception();
					}
					if (!_dirty_windows.IsEmpty()) {
						XExposeEvent expose;
						expose.type = Expose;
						expose.serial = 0;
						expose.send_event = True;
						expose.display = _con->GetXDisplay();
						expose.x = expose.y = expose.width = expose.height = 0;
						expose.count = 0;
						for (auto & w : _dirty_windows) {
							expose.window = w;
							auto responder = _window_inputs[w];
							if (responder) (*responder)->HandleEvent(w, reinterpret_cast<XEvent *>(&expose));
						}
						_dirty_windows.Clear();
					}
				}
			} catch (...) {
				_context_stack.RemoveLast();
				_current_context = _context_stack.GetLength() ? &_context_stack.LastElement() : 0;
				return false;
			}
			_context_stack.RemoveLast();
			_current_context = _context_stack.GetLength() ? &_context_stack.LastElement() : 0;
			return true;
		}
		void XDispatch::BreakEventLoop(void) noexcept { if (_current_context) _current_context->exit_flag = true; }
		void XDispatch::DrainEvent(int type) noexcept
		{
			_api->XFlush(_con->GetXDisplay());
			XEvent event;
			while (_api->XCheckIfEvent(_con->GetXDisplay(), &event, _peek_predicate, XPointer(intptr(type))));
		}
		bool XDispatch::WaitForEvent(XEvent * event, int type, uint timeout) noexcept
		{
			_api->XFlush(_con->GetXDisplay());
			if (_api->XCheckIfEvent(_con->GetXDisplay(), event, _peek_predicate, XPointer(intptr(type)))) return true;
			pollfd poll_struct;
			poll_struct.fd = _api->XConnectionNumber(_con->GetXDisplay());
			poll_struct.events = POLLIN;
			while (true) {
				auto poll_result = poll(&poll_struct, 1, timeout);
				if (poll_result == -1) {
					if (errno == EINTR) continue; else return false;
				} else if (poll_result == 0) {
					return false;
				} else {
					_api->XIfEvent(_con->GetXDisplay(), event, _peek_predicate, XPointer(intptr(type)));
					return true;
				}
			}
		}
		void XDispatch::ScheduleWindowUpdate(Window window) noexcept { try { _dirty_windows.AddElement(window); } catch (...) {} }

		uint XKeyCodeToESSE(uint code) noexcept
		{
			code -= 8;
			if (code == KEY_ESC) return VirtualKeyCodes::Escape;
			else if (code == KEY_0) return VirtualKeyCodes::D0;
			else if (code == KEY_1) return VirtualKeyCodes::D1;
			else if (code == KEY_2) return VirtualKeyCodes::D2;
			else if (code == KEY_3) return VirtualKeyCodes::D3;
			else if (code == KEY_4) return VirtualKeyCodes::D4;
			else if (code == KEY_5) return VirtualKeyCodes::D5;
			else if (code == KEY_6) return VirtualKeyCodes::D6;
			else if (code == KEY_7) return VirtualKeyCodes::D7;
			else if (code == KEY_8) return VirtualKeyCodes::D8;
			else if (code == KEY_9) return VirtualKeyCodes::D9;
			else if (code == KEY_MINUS) return VirtualKeyCodes::OemMinus;
			else if (code == KEY_EQUAL) return VirtualKeyCodes::OemPlus;
			else if (code == KEY_BACKSPACE) return VirtualKeyCodes::Back;
			else if (code == KEY_TAB) return VirtualKeyCodes::Tab;
			else if (code == KEY_Q) return VirtualKeyCodes::Q;
			else if (code == KEY_W) return VirtualKeyCodes::W;
			else if (code == KEY_E) return VirtualKeyCodes::E;
			else if (code == KEY_R) return VirtualKeyCodes::R;
			else if (code == KEY_T) return VirtualKeyCodes::T;
			else if (code == KEY_Y) return VirtualKeyCodes::Y;
			else if (code == KEY_U) return VirtualKeyCodes::U;
			else if (code == KEY_I) return VirtualKeyCodes::I;
			else if (code == KEY_O) return VirtualKeyCodes::O;
			else if (code == KEY_P) return VirtualKeyCodes::P;
			else if (code == KEY_LEFTBRACE) return VirtualKeyCodes::Oem4;
			else if (code == KEY_RIGHTBRACE) return VirtualKeyCodes::Oem6;
			else if (code == KEY_ENTER) return VirtualKeyCodes::Return;
			else if (code == KEY_LEFTCTRL) return VirtualKeyCodes::LeftControl;
			else if (code == KEY_A) return VirtualKeyCodes::A;
			else if (code == KEY_S) return VirtualKeyCodes::S;
			else if (code == KEY_D) return VirtualKeyCodes::D;
			else if (code == KEY_F) return VirtualKeyCodes::F;
			else if (code == KEY_G) return VirtualKeyCodes::G;
			else if (code == KEY_H) return VirtualKeyCodes::H;
			else if (code == KEY_J) return VirtualKeyCodes::J;
			else if (code == KEY_K) return VirtualKeyCodes::K;
			else if (code == KEY_L) return VirtualKeyCodes::L;
			else if (code == KEY_SEMICOLON) return VirtualKeyCodes::Oem1;
			else if (code == KEY_APOSTROPHE) return VirtualKeyCodes::Oem7;
			else if (code == KEY_GRAVE) return VirtualKeyCodes::Oem3;
			else if (code == KEY_LEFTSHIFT) return VirtualKeyCodes::LeftShift;
			else if (code == KEY_BACKSLASH) return VirtualKeyCodes::Oem5;
			else if (code == KEY_Z) return VirtualKeyCodes::Z;
			else if (code == KEY_X) return VirtualKeyCodes::X;
			else if (code == KEY_C) return VirtualKeyCodes::C;
			else if (code == KEY_V) return VirtualKeyCodes::V;
			else if (code == KEY_B) return VirtualKeyCodes::B;
			else if (code == KEY_N) return VirtualKeyCodes::N;
			else if (code == KEY_M) return VirtualKeyCodes::M;
			else if (code == KEY_COMMA) return VirtualKeyCodes::OemComma;
			else if (code == KEY_DOT) return VirtualKeyCodes::OemPeriod;
			else if (code == KEY_SLASH) return VirtualKeyCodes::Oem2;
			else if (code == KEY_RIGHTSHIFT) return VirtualKeyCodes::RightShift;
			else if (code == KEY_KPASTERISK) return VirtualKeyCodes::Multiply;
			else if (code == KEY_LEFTALT) return VirtualKeyCodes::LeftAlternative;
			else if (code == KEY_SPACE) return VirtualKeyCodes::Space;
			else if (code == KEY_CAPSLOCK) return VirtualKeyCodes::CapsLock;
			else if (code == KEY_F1) return VirtualKeyCodes::F1;
			else if (code == KEY_F2) return VirtualKeyCodes::F2;
			else if (code == KEY_F3) return VirtualKeyCodes::F3;
			else if (code == KEY_F4) return VirtualKeyCodes::F4;
			else if (code == KEY_F5) return VirtualKeyCodes::F5;
			else if (code == KEY_F6) return VirtualKeyCodes::F6;
			else if (code == KEY_F7) return VirtualKeyCodes::F7;
			else if (code == KEY_F8) return VirtualKeyCodes::F8;
			else if (code == KEY_F9) return VirtualKeyCodes::F9;
			else if (code == KEY_F10) return VirtualKeyCodes::F10;
			else if (code == KEY_NUMLOCK) return VirtualKeyCodes::NumLock;
			else if (code == KEY_SCROLLLOCK) return VirtualKeyCodes::ScrollLock;
			else if (code == KEY_KP7) return VirtualKeyCodes::Num7;
			else if (code == KEY_KP8) return VirtualKeyCodes::Num8;
			else if (code == KEY_KP9) return VirtualKeyCodes::Num9;
			else if (code == KEY_KPMINUS) return VirtualKeyCodes::Subtract;
			else if (code == KEY_KP4) return VirtualKeyCodes::Num4;
			else if (code == KEY_KP5) return VirtualKeyCodes::Num5;
			else if (code == KEY_KP6) return VirtualKeyCodes::Num6;
			else if (code == KEY_KPPLUS) return VirtualKeyCodes::Add;
			else if (code == KEY_KP1) return VirtualKeyCodes::Num1;
			else if (code == KEY_KP2) return VirtualKeyCodes::Num2;
			else if (code == KEY_KP3) return VirtualKeyCodes::Num3;
			else if (code == KEY_KP0) return VirtualKeyCodes::Num0;
			else if (code == KEY_KPDOT) return VirtualKeyCodes::Decimal;
			else if (code == KEY_102ND) return VirtualKeyCodes::Oem8;
			else if (code == KEY_F11) return VirtualKeyCodes::F11;
			else if (code == KEY_F12) return VirtualKeyCodes::F12;
			else if (code == KEY_KPENTER) return VirtualKeyCodes::Return;
			else if (code == KEY_RIGHTCTRL) return VirtualKeyCodes::RightControl;
			else if (code == KEY_KPSLASH) return VirtualKeyCodes::Divide;
			else if (code == KEY_RIGHTALT) return VirtualKeyCodes::RightAlternative;
			else if (code == KEY_HOME) return VirtualKeyCodes::Home;
			else if (code == KEY_UP) return VirtualKeyCodes::Up;
			else if (code == KEY_PAGEUP) return VirtualKeyCodes::PageUp;
			else if (code == KEY_LEFT) return VirtualKeyCodes::Left;
			else if (code == KEY_RIGHT) return VirtualKeyCodes::Right;
			else if (code == KEY_END) return VirtualKeyCodes::End;
			else if (code == KEY_DOWN) return VirtualKeyCodes::Down;
			else if (code == KEY_PAGEDOWN) return VirtualKeyCodes::PageDown;
			else if (code == KEY_INSERT) return VirtualKeyCodes::Insert;
			else if (code == KEY_DELETE) return VirtualKeyCodes::Delete;
			else if (code == KEY_MUTE) return VirtualKeyCodes::VolumeMute;
			else if (code == KEY_VOLUMEDOWN) return VirtualKeyCodes::VolumeDown;
			else if (code == KEY_VOLUMEUP) return VirtualKeyCodes::VolumeUp;
			else if (code == KEY_PAUSE) return VirtualKeyCodes::Pause;
			else if (code == KEY_LEFTMETA) return VirtualKeyCodes::LeftSystem;
			else if (code == KEY_RIGHTMETA) return VirtualKeyCodes::RightSystem;
			else if (code == KEY_HELP) return VirtualKeyCodes::Help;
			else if (code == KEY_SLEEP) return VirtualKeyCodes::Sleep;
			else if (code == KEY_PRINT) return VirtualKeyCodes::Print;
			else if (code == KEY_SELECT) return VirtualKeyCodes::Select;
			else if (code == KEY_CLEAR) return VirtualKeyCodes::OemClear;
			else if (code == KEY_F13) return VirtualKeyCodes::F13;
			else if (code == KEY_F14) return VirtualKeyCodes::F14;
			else if (code == KEY_F15) return VirtualKeyCodes::F15;
			else if (code == KEY_F16) return VirtualKeyCodes::F16;
			else if (code == KEY_F17) return VirtualKeyCodes::F17;
			else if (code == KEY_F18) return VirtualKeyCodes::F18;
			else if (code == KEY_F19) return VirtualKeyCodes::F19;
			else if (code == KEY_F20) return VirtualKeyCodes::F20;
			else if (code == KEY_F21) return VirtualKeyCodes::F21;
			else if (code == KEY_F22) return VirtualKeyCodes::F22;
			else if (code == KEY_F23) return VirtualKeyCodes::F23;
			else if (code == KEY_F24) return VirtualKeyCodes::F24;
			else return 0;
		}
		uint XKeyCodeFromESSE(uint code) noexcept
		{
			uint ret = 0;
			if (code == VirtualKeyCodes::Escape) ret = KEY_ESC;
			else if (code == VirtualKeyCodes::D0) ret = KEY_0;
			else if (code == VirtualKeyCodes::D1) ret = KEY_1;
			else if (code == VirtualKeyCodes::D2) ret = KEY_2;
			else if (code == VirtualKeyCodes::D3) ret = KEY_3;
			else if (code == VirtualKeyCodes::D4) ret = KEY_4;
			else if (code == VirtualKeyCodes::D5) ret = KEY_5;
			else if (code == VirtualKeyCodes::D6) ret = KEY_6;
			else if (code == VirtualKeyCodes::D7) ret = KEY_7;
			else if (code == VirtualKeyCodes::D8) ret = KEY_8;
			else if (code == VirtualKeyCodes::D9) ret = KEY_9;
			else if (code == VirtualKeyCodes::OemMinus) ret = KEY_MINUS;
			else if (code == VirtualKeyCodes::OemPlus) ret = KEY_EQUAL;
			else if (code == VirtualKeyCodes::Back) ret = KEY_BACKSPACE;
			else if (code == VirtualKeyCodes::Tab) ret = KEY_TAB;
			else if (code == VirtualKeyCodes::Q) ret = KEY_Q;
			else if (code == VirtualKeyCodes::W) ret = KEY_W;
			else if (code == VirtualKeyCodes::E) ret = KEY_E;
			else if (code == VirtualKeyCodes::R) ret = KEY_R;
			else if (code == VirtualKeyCodes::T) ret = KEY_T;
			else if (code == VirtualKeyCodes::Y) ret = KEY_Y;
			else if (code == VirtualKeyCodes::U) ret = KEY_U;
			else if (code == VirtualKeyCodes::I) ret = KEY_I;
			else if (code == VirtualKeyCodes::O) ret = KEY_O;
			else if (code == VirtualKeyCodes::P) ret = KEY_P;
			else if (code == VirtualKeyCodes::Oem4) ret = KEY_LEFTBRACE;
			else if (code == VirtualKeyCodes::Oem6) ret = KEY_RIGHTBRACE;
			else if (code == VirtualKeyCodes::Return) ret = KEY_ENTER;
			else if (code == VirtualKeyCodes::LeftControl) ret = KEY_LEFTCTRL;
			else if (code == VirtualKeyCodes::A) ret = KEY_A;
			else if (code == VirtualKeyCodes::S) ret = KEY_S;
			else if (code == VirtualKeyCodes::D) ret = KEY_D;
			else if (code == VirtualKeyCodes::F) ret = KEY_F;
			else if (code == VirtualKeyCodes::G) ret = KEY_G;
			else if (code == VirtualKeyCodes::H) ret = KEY_H;
			else if (code == VirtualKeyCodes::J) ret = KEY_J;
			else if (code == VirtualKeyCodes::K) ret = KEY_K;
			else if (code == VirtualKeyCodes::L) ret = KEY_L;
			else if (code == VirtualKeyCodes::Oem1) ret = KEY_SEMICOLON;
			else if (code == VirtualKeyCodes::Oem7) ret = KEY_APOSTROPHE;
			else if (code == VirtualKeyCodes::Oem3) ret = KEY_GRAVE;
			else if (code == VirtualKeyCodes::LeftShift) ret = KEY_LEFTSHIFT;
			else if (code == VirtualKeyCodes::Oem5) ret = KEY_BACKSLASH;
			else if (code == VirtualKeyCodes::Z) ret = KEY_Z;
			else if (code == VirtualKeyCodes::X) ret = KEY_X;
			else if (code == VirtualKeyCodes::C) ret = KEY_C;
			else if (code == VirtualKeyCodes::V) ret = KEY_V;
			else if (code == VirtualKeyCodes::B) ret = KEY_B;
			else if (code == VirtualKeyCodes::N) ret = KEY_N;
			else if (code == VirtualKeyCodes::M) ret = KEY_M;
			else if (code == VirtualKeyCodes::OemComma) ret = KEY_COMMA;
			else if (code == VirtualKeyCodes::OemPeriod) ret = KEY_DOT;
			else if (code == VirtualKeyCodes::Oem2) ret = KEY_SLASH;
			else if (code == VirtualKeyCodes::RightShift) ret = KEY_RIGHTSHIFT;
			else if (code == VirtualKeyCodes::Multiply) ret = KEY_KPASTERISK;
			else if (code == VirtualKeyCodes::LeftAlternative) ret = KEY_LEFTALT;
			else if (code == VirtualKeyCodes::Space) ret = KEY_SPACE;
			else if (code == VirtualKeyCodes::CapsLock) ret = KEY_CAPSLOCK;
			else if (code == VirtualKeyCodes::F1) ret = KEY_F1;
			else if (code == VirtualKeyCodes::F2) ret = KEY_F2;
			else if (code == VirtualKeyCodes::F3) ret = KEY_F3;
			else if (code == VirtualKeyCodes::F4) ret = KEY_F4;
			else if (code == VirtualKeyCodes::F5) ret = KEY_F5;
			else if (code == VirtualKeyCodes::F6) ret = KEY_F6;
			else if (code == VirtualKeyCodes::F7) ret = KEY_F7;
			else if (code == VirtualKeyCodes::F8) ret = KEY_F8;
			else if (code == VirtualKeyCodes::F9) ret = KEY_F9;
			else if (code == VirtualKeyCodes::F10) ret = KEY_F10;
			else if (code == VirtualKeyCodes::NumLock) ret = KEY_NUMLOCK;
			else if (code == VirtualKeyCodes::ScrollLock) ret = KEY_SCROLLLOCK;
			else if (code == VirtualKeyCodes::Num7) ret = KEY_KP7;
			else if (code == VirtualKeyCodes::Num8) ret = KEY_KP8;
			else if (code == VirtualKeyCodes::Num9) ret = KEY_KP9;
			else if (code == VirtualKeyCodes::Subtract) ret = KEY_KPMINUS;
			else if (code == VirtualKeyCodes::Num4) ret = KEY_KP4;
			else if (code == VirtualKeyCodes::Num5) ret = KEY_KP5;
			else if (code == VirtualKeyCodes::Num6) ret = KEY_KP6;
			else if (code == VirtualKeyCodes::Add) ret = KEY_KPPLUS;
			else if (code == VirtualKeyCodes::Num1) ret = KEY_KP1;
			else if (code == VirtualKeyCodes::Num2) ret = KEY_KP2;
			else if (code == VirtualKeyCodes::Num3) ret = KEY_KP3;
			else if (code == VirtualKeyCodes::Num0) ret = KEY_KP0;
			else if (code == VirtualKeyCodes::Decimal) ret = KEY_KPDOT;
			else if (code == VirtualKeyCodes::Oem8) ret = KEY_102ND;
			else if (code == VirtualKeyCodes::F11) ret = KEY_F11;
			else if (code == VirtualKeyCodes::F12) ret = KEY_F12;
			else if (code == VirtualKeyCodes::RightControl) ret = KEY_RIGHTCTRL;
			else if (code == VirtualKeyCodes::Divide) ret = KEY_KPSLASH;
			else if (code == VirtualKeyCodes::RightAlternative) ret = KEY_RIGHTALT;
			else if (code == VirtualKeyCodes::Home) ret = KEY_HOME;
			else if (code == VirtualKeyCodes::Up) ret = KEY_UP;
			else if (code == VirtualKeyCodes::PageUp) ret = KEY_PAGEUP;
			else if (code == VirtualKeyCodes::Left) ret = KEY_LEFT;
			else if (code == VirtualKeyCodes::Right) ret = KEY_RIGHT;
			else if (code == VirtualKeyCodes::End) ret = KEY_END;
			else if (code == VirtualKeyCodes::Down) ret = KEY_DOWN;
			else if (code == VirtualKeyCodes::PageDown) ret = KEY_PAGEDOWN;
			else if (code == VirtualKeyCodes::Insert) ret = KEY_INSERT;
			else if (code == VirtualKeyCodes::Delete) ret = KEY_DELETE;
			else if (code == VirtualKeyCodes::VolumeMute) ret = KEY_MUTE;
			else if (code == VirtualKeyCodes::VolumeDown) ret = KEY_VOLUMEDOWN;
			else if (code == VirtualKeyCodes::VolumeUp) ret = KEY_VOLUMEUP;
			else if (code == VirtualKeyCodes::Pause) ret = KEY_PAUSE;
			else if (code == VirtualKeyCodes::LeftSystem) ret = KEY_LEFTMETA;
			else if (code == VirtualKeyCodes::RightSystem) ret = KEY_RIGHTMETA;
			else if (code == VirtualKeyCodes::Help) ret = KEY_HELP;
			else if (code == VirtualKeyCodes::Sleep) ret = KEY_SLEEP;
			else if (code == VirtualKeyCodes::Print) ret = KEY_PRINT;
			else if (code == VirtualKeyCodes::Select) ret = KEY_SELECT;
			else if (code == VirtualKeyCodes::OemClear) ret = KEY_CLEAR;
			else if (code == VirtualKeyCodes::F13) ret = KEY_F13;
			else if (code == VirtualKeyCodes::F14) ret = KEY_F14;
			else if (code == VirtualKeyCodes::F15) ret = KEY_F15;
			else if (code == VirtualKeyCodes::F16) ret = KEY_F16;
			else if (code == VirtualKeyCodes::F17) ret = KEY_F17;
			else if (code == VirtualKeyCodes::F18) ret = KEY_F18;
			else if (code == VirtualKeyCodes::F19) ret = KEY_F19;
			else if (code == VirtualKeyCodes::F20) ret = KEY_F20;
			else if (code == VirtualKeyCodes::F21) ret = KEY_F21;
			else if (code == VirtualKeyCodes::F22) ret = KEY_F22;
			else if (code == VirtualKeyCodes::F23) ret = KEY_F23;
			else if (code == VirtualKeyCodes::F24) ret = KEY_F24;
			return ret + 8;
		}
	}
}