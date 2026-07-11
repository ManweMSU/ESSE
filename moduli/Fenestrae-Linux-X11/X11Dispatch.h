#pragma once

#include "X11DL.h"
#include <Cor/Images/CorGraphics.h>

namespace ESSE
{
	namespace X11
	{
		class IXWindowEventHandler
		{
		public:
			virtual void HandleEvent(Window window, XEvent * event) noexcept = 0;
			virtual void HandleTimer(Window window, int timer) noexcept = 0;
		};
		class IXFileEventHandler
		{
		public:
			virtual void HandleFile(int file) noexcept = 0;
		};
		class XServerConnection : public Object
		{
			oref<XLibAPI> _xlib_api;
			Display * _display;
		public:
			XServerConnection(XLibAPI * xlib_api) noexcept;
			virtual ~XServerConnection(void) override;
			Display * GetXDisplay(void) const noexcept;
			XLibAPI * GetAPI(void) const noexcept;
			static oref<XServerConnection> Query(ErrorContext & ectx) noexcept;
		};
		class XDispatch : public Object
		{
			struct _loop_context
			{
				bool exit_flag;
				bool sources_updated_flag;
			};
			struct _file_input
			{
				int file;
				IXFileEventHandler * handler;
			};
			struct _timer_input
			{
				Window window;
				int timer_id;
				uint last_fired;
				uint period;
			};
		private:
			_loop_context * _current_context;
			Dictionary<Window, IXWindowEventHandler *> _window_inputs;
			Set<Window> _dirty_windows;
			array<_loop_context> _context_stack;
			array<_file_input> _file_inputs;
			array<_timer_input> _timers;
			oref<XLibAPI> _api;
			oref<XServerConnection> _con;
		private:
			static Bool _peek_predicate(Display * display, XEvent * event, XPointer arg) noexcept;
			void _set_sources_updated(void) noexcept;
			bool _silent_window_append(Window window, IXWindowEventHandler * handler) noexcept;
		public:
			XDispatch(XServerConnection * con);
			virtual ~XDispatch(void) override;
			bool RegisterWindowHandler(Window window, IXWindowEventHandler * handler) noexcept;
			void UnregisterWindowHandler(Window window) noexcept;
			bool RegisterFileHandler(int file, IXFileEventHandler * handler) noexcept;
			void UnregisterFileHandler(int file) noexcept;
			bool CreateTimer(Window window, int timer, uint period) noexcept;
			void DestroyTimer(Window window, int timer) noexcept;
			bool RunEventLoop(void) noexcept;
			void BreakEventLoop(void) noexcept;
			void DrainEvent(int type) noexcept;
			bool WaitForEvent(XEvent * event, int type, uint timeout) noexcept;
			void ScheduleWindowUpdate(Window window) noexcept;
		};
		uint XKeyCodeFromESSE(uint code) noexcept;
		uint XKeyCodeToESSE(uint code) noexcept;
	}
}