#include "X11DL.h"
#include <dlfcn.h>

namespace ESSE
{
	namespace X11
	{
		int LastXError = 0;
		int XSilentErrorHandler(Display * display, XErrorEvent * error) { LastXError = error->error_code; return 0; }
		XLibAPI::XLibAPI(void)
		{
			_library = dlopen("libX11.so", RTLD_NOW);
			if (!_library) throw NotImplementedException();
			try {
				DEFINE_FUNCTION_IMPORT(XOpenDisplay)
				DEFINE_FUNCTION_IMPORT(XCloseDisplay)
				DEFINE_FUNCTION_IMPORT(XConnectionNumber)
				DEFINE_FUNCTION_IMPORT(XPending)
				DEFINE_FUNCTION_IMPORT(XNextEvent)
				DEFINE_FUNCTION_IMPORT(XSendEvent)
				DEFINE_FUNCTION_IMPORT(XRefreshKeyboardMapping)
				DEFINE_FUNCTION_IMPORT(XFlush)
				DEFINE_FUNCTION_IMPORT(XCheckIfEvent)
				DEFINE_FUNCTION_IMPORT(XIfEvent)
				DEFINE_FUNCTION_IMPORT(XDefaultScreen)
				DEFINE_FUNCTION_IMPORT(XRootWindow)
				DEFINE_FUNCTION_IMPORT(XGetImage)
				DEFINE_FUNCTION_IMPORT(XDefaultScreenOfDisplay)
				DEFINE_FUNCTION_IMPORT(XWidthOfScreen)
				DEFINE_FUNCTION_IMPORT(XHeightOfScreen)
				DEFINE_FUNCTION_IMPORT(XGetAtomName)
				DEFINE_FUNCTION_IMPORT(XFree)
				DEFINE_FUNCTION_IMPORT(XResourceManagerString)
				DEFINE_FUNCTION_IMPORT(XSetErrorHandler)
				DEFINE_FUNCTION_IMPORT(XGetWindowProperty)
				DEFINE_FUNCTION_IMPORT(XInternAtom)
				DEFINE_FUNCTION_IMPORT(XCreateWindow)
				DEFINE_FUNCTION_IMPORT(XDestroyWindow)
				DEFINE_FUNCTION_IMPORT(XChangeProperty)
				DEFINE_FUNCTION_IMPORT(XSync)
				DEFINE_FUNCTION_IMPORT(XChangeWindowAttributes)
				DEFINE_FUNCTION_IMPORT(XQueryKeymap)
				DEFINE_FUNCTION_IMPORT(XGrabKey)
				DEFINE_FUNCTION_IMPORT(XUngrabKey)
				DEFINE_FUNCTION_IMPORT(XGrabPointer)
				DEFINE_FUNCTION_IMPORT(XUngrabPointer)
				DEFINE_FUNCTION_IMPORT(XGetSelectionOwner)
				DEFINE_FUNCTION_IMPORT(XSetSelectionOwner)
				DEFINE_FUNCTION_IMPORT(XConvertSelection)
				DEFINE_FUNCTION_IMPORT(XCreatePixmap)
				DEFINE_FUNCTION_IMPORT(XFreePixmap)
				DEFINE_FUNCTION_IMPORT(XCreateGC)
				DEFINE_FUNCTION_IMPORT(XFreeGC)
				DEFINE_FUNCTION_IMPORT(XFlushGC)
				DEFINE_FUNCTION_IMPORT(XSetForeground)
				DEFINE_FUNCTION_IMPORT(XFillRectangle)
				DEFINE_FUNCTION_IMPORT(XInitImage)
				DEFINE_FUNCTION_IMPORT(XPutImage)
				DEFINE_FUNCTION_IMPORT(XCreateFontCursor)
				DEFINE_FUNCTION_IMPORT(XQueryBestCursor)
				DEFINE_FUNCTION_IMPORT(XFreeCursor)
				DEFINE_FUNCTION_IMPORT(XQueryPointer)
				DEFINE_FUNCTION_IMPORT(XWarpPointer)
				DEFINE_FUNCTION_IMPORT(XMapRaised)
				DEFINE_FUNCTION_IMPORT(XUnmapWindow)
				DEFINE_FUNCTION_IMPORT(XWithdrawWindow)
				DEFINE_FUNCTION_IMPORT(XSetSizeHints)
				DEFINE_FUNCTION_IMPORT(XResizeWindow)
				DEFINE_FUNCTION_IMPORT(XMoveWindow)
				DEFINE_FUNCTION_IMPORT(XCreateIC)
				DEFINE_FUNCTION_IMPORT(XDestroyIC)
				DEFINE_FUNCTION_IMPORT(XSetICFocus)
				DEFINE_FUNCTION_IMPORT(XUnsetICFocus)
				DEFINE_FUNCTION_IMPORT(XwcLookupString)
				DEFINE_FUNCTION_IMPORT(XDefineCursor)
				DEFINE_FUNCTION_IMPORT(XMatchVisualInfo)
				DEFINE_FUNCTION_IMPORT(XCreateColormap)
				DEFINE_FUNCTION_IMPORT(XFreeColormap)
				DEFINE_FUNCTION_IMPORT(XSetWMHints)
				DEFINE_FUNCTION_IMPORT(XOpenIM)
				DEFINE_FUNCTION_IMPORT(XCloseIM)
				DEFINE_FUNCTION_IMPORT(XMapWindow)
				DEFINE_FUNCTION_IMPORT(XIconifyWindow)
				DEFINE_FUNCTION_IMPORT(XDefaultRootWindow)
				DEFINE_FUNCTION_IMPORT(XGetInputFocus)
				DEFINE_FUNCTION_IMPORT(XSetInputFocus)
				DEFINE_FUNCTION_IMPORT(XTranslateCoordinates)
				DEFINE_FUNCTION_IMPORT(XCopyArea)
				DEFINE_FUNCTION_IMPORT(XGetWindowAttributes)
				DEFINE_FUNCTION_IMPORT(XDefaultVisual)
				DEFINE_FUNCTION_IMPORT(XQueryTree)
				DEFINE_FUNCTION_IMPORT(XGetVisualInfo)
				DEFINE_FUNCTION_IMPORT(XSelectInput)
				DEFINE_FUNCTION_IMPORT(XkbLibraryVersion)
				DEFINE_FUNCTION_IMPORT(XkbQueryExtension)
				DEFINE_FUNCTION_IMPORT(XkbGetMap)
				DEFINE_FUNCTION_IMPORT(XkbFreeKeyboard)
				DEFINE_FUNCTION_IMPORT(XkbGetState)
				DEFINE_FUNCTION_IMPORT(XkbGetAutoRepeatRate)
				DEFINE_FUNCTION_IMPORT(XkbBell)
			} catch (...) { dlclose(_library); throw; }
			XSetErrorHandler(XSilentErrorHandler);
		}
		XLibAPI::~XLibAPI(void) { dlclose(_library); }
		XRANDRAPI::XRANDRAPI(void)
		{
			_library = dlopen("libXrandr.so", RTLD_NOW);
			if (!_library) throw NotImplementedException();
			try {
				DEFINE_FUNCTION_IMPORT(XRRQueryExtension)
				DEFINE_FUNCTION_IMPORT(XRRQueryVersion)
				DEFINE_FUNCTION_IMPORT(XRRGetMonitors)
				DEFINE_FUNCTION_IMPORT(XRRFreeMonitors)
			} catch (...) { dlclose(_library); throw; }
		}
		XRANDRAPI::~XRANDRAPI(void) { dlclose(_library); }
		XRenderAPI::XRenderAPI(void)
		{
			_library = dlopen("libXrender.so", RTLD_NOW);
			if (!_library) throw NotImplementedException();
			try {
				DEFINE_FUNCTION_IMPORT(XRenderQueryExtension)
				DEFINE_FUNCTION_IMPORT(XRenderFindStandardFormat)
				DEFINE_FUNCTION_IMPORT(XRenderCreatePicture)
				DEFINE_FUNCTION_IMPORT(XRenderFreePicture)
				DEFINE_FUNCTION_IMPORT(XRenderCreateCursor)
				DEFINE_FUNCTION_IMPORT(XRenderFindVisualFormat)
			} catch (...) { dlclose(_library); throw; }
		}
		XRenderAPI::~XRenderAPI(void) { dlclose(_library); }
		XCursorAPI::XCursorAPI(void)
		{
			_library = dlopen("libXcursor.so", RTLD_NOW);
			if (!_library) throw NotImplementedException();
			try {
				DEFINE_FUNCTION_IMPORT(XcursorLibraryLoadCursor)
			} catch (...) { dlclose(_library); throw; }
		}
		XCursorAPI::~XCursorAPI(void) { dlclose(_library); }
	}
}