#include "CorWindows.h"

namespace ESSE
{
	namespace Windows
	{
		void IWindowCallback::Created(IWindow * window) noexcept {}
		void IWindowCallback::Destroyed(IWindow * window) noexcept {}
		void IWindowCallback::Shown(IWindow * window, bool show) noexcept {}
		void IWindowCallback::RenderWindow(IWindow * window) noexcept {}
		void IWindowCallback::WindowClosed(IWindow * window) noexcept {}
		void IWindowCallback::WindowMaximized(IWindow * window) noexcept {}
		void IWindowCallback::WindowMinimized(IWindow * window) noexcept {}
		void IWindowCallback::WindowRestored(IWindow * window) noexcept {}
		void IWindowCallback::WindowHelpRequired(IWindow * window) noexcept {}
		void IWindowCallback::WindowActivated(IWindow * window) noexcept {}
		void IWindowCallback::WindowDeactivated(IWindow * window) noexcept {}
		void IWindowCallback::WindowMoved(IWindow * window) noexcept {}
		void IWindowCallback::WindowResized(IWindow * window) noexcept {}
		void IWindowCallback::FocusChanged(IWindow * window, bool got) noexcept {}
		bool IWindowCallback::KeyIsDown(IWindow * window, uint vkc, uint vkm) noexcept { return false; }
		void IWindowCallback::KeyIsUp(IWindow * window, uint vkc, uint vkm) noexcept {}
		void IWindowCallback::CharacterIsDown(IWindow * window, unichar32 ucs) noexcept {}
		void IWindowCallback::MouseEntered(IWindow * window, uint button_state) noexcept {}
		void IWindowCallback::MouseLeft(IWindow * window, uint button_state) noexcept {}
		void IWindowCallback::MouseMoved(IWindow * window, const Index2 & at, uint button_state) noexcept {}
		void IWindowCallback::LeftButtonIsDown(IWindow * window, const Index2 & at, bool double_click) noexcept {}
		void IWindowCallback::LeftButtonIsUp(IWindow * window, const Index2 & at) noexcept {}
		void IWindowCallback::RightButtonIsDown(IWindow * window, const Index2 & at, bool double_click) noexcept {}
		void IWindowCallback::RightButtonIsUp(IWindow * window, const Index2 & at) noexcept {}
		void IWindowCallback::ScrollVertically(IWindow * window, const Index2 & at, double delta) noexcept {}
		void IWindowCallback::ScrollHorizontally(IWindow * window, const Index2 & at, double delta) noexcept {}
		void IWindowCallback::Timer(IWindow * window, int timer_id) noexcept {}
		void IWindowCallback::ThemeChanged(IWindow * window) noexcept {}
		bool IWindowCallback::IsWindowCommandEnabled(IWindow * window, WindowCommand command) noexcept { return false; }
		void IWindowCallback::HandleWindowCommand(IWindow * window, WindowCommand command) noexcept {}

		bool IApplicationCallback::AcceptsApplicationCommand(ApplicationCommand command) noexcept { return false; }
		bool IApplicationCallback::AcceptsWindowCommand(WindowCommand command) noexcept { return false; }
		bool IApplicationCallback::HandleApplicationCommand(ApplicationCommand command, const string & argument) noexcept { return false; }
		void IApplicationCallback::HandleHotKeyEvent(uint event_id) noexcept {}
		bool IApplicationCallback::IPCReceiveData(handle client, const string & verb, const void * data, uintptr length) noexcept { return false; }
		oref<DataBlock> IApplicationCallback::IPCSendData(handle client, const string & verb) noexcept { return 0; }
		void IApplicationCallback::IPCClientDisconnect(handle client) noexcept {}

		void IMenuItemCallback::MenuItemDisposed(IMenuItem * item) noexcept {}
		void IStatusCallback::HandleStatusIconCommand(IStatusBarIcon * icon, int id) noexcept {}
	}
}