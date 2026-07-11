#pragma once

#include <Cor/IO/CorWindows.h>
#include <DBus-Linux/DBus.h>
#include "X11Dispatch.h"

namespace ESSE
{
	namespace X11
	{
		bool IsGraphicalProcess(pid_t pid) noexcept;
		bool ActivateProcess(pid_t pid) noexcept;

		class IX11Window
		{
		public:
			virtual Window GetHandle(void) noexcept = 0;
			virtual Visual * GetVisual(void) noexcept = 0;
			virtual bool GetFullscreenState(void) noexcept = 0;
			virtual void SetFullscreenState(bool set) noexcept = 0;
			virtual bool IsWindowLocked(void) noexcept = 0;
			virtual void LockWindow(bool lock) noexcept = 0;
			virtual Windows::IWindowSystem * GetWindowSystem(void) noexcept = 0;
		};
		class IX11WindowSystem
		{
		public:
			virtual oref<Windows::IScreen> GetScreenWithBestCoverage(const Rectangle & rect) noexcept = 0;
			virtual XDispatch * GetDispatch(void) noexcept = 0;
			virtual XServerConnection * GetConnection(void) noexcept = 0;
			virtual XRenderAPI * GetXRenderAPI(void) noexcept = 0;
			virtual DBus::IConnection * GetDBus(void) noexcept = 0;
			virtual Picturae::Image * GetApplicationIcon(void) noexcept = 0;
			virtual Window GetServiceWindow(void) noexcept = 0;
			virtual void * GetStandardAtoms(void) noexcept = 0;
			virtual bool RegisterTopLevelWindow(Windows::IWindow * window) noexcept = 0;
			virtual bool UnregisterTopLevelWindow(Windows::IWindow * window) noexcept = 0;
			virtual uint GetModalityLevel(void) noexcept = 0;
			virtual void SetModalityLevel(uint level) noexcept = 0;
			virtual double GetSystemScaleFactor(void) noexcept = 0;
			virtual uint GetAlternativeKeyMod(void) noexcept = 0;
			virtual uint GetSystemKeyMod(void) noexcept = 0;
		};
	}
	namespace Windows
	{
		#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_WAYLAND_X11
			#define ESSE_MODULUS_FENESTRARUM_LINUX_X11_ENTRY AllocateWindowSystemX11
		#else
			#define ESSE_MODULUS_FENESTRARUM_LINUX_X11_ENTRY AllocateWindowSystem
		#endif
		oref<IWindowSystem> ESSE_MODULUS_FENESTRARUM_LINUX_X11_ENTRY(ErrorContext & ectx) noexcept;
	}
}