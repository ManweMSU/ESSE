#include "DeviceWindowedCairo.h"
#include <Cor-Linux/CorLinuxClasses.h>

namespace ESSE
{
	namespace Cairo
	{
		#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
		CairoDeviceX11::CairoDeviceX11(CairoAPI * api, Graphica::IDeviceContextFactory2D * parent, Windows::IWindow * window, X11::IX11Window * wx11) : CairoDevice(api, parent)
		{
			_window_object = window;
			ErrorContext ectx; ErrorClear(ectx);
			auto ws = wx11->GetWindowSystem();
			auto ws11 = reinterpret_cast<X11::IX11WindowSystem *>(ws->DynamicCast(Linux::Classes::X11_WindowSystem, ectx));
			ErrorThrow(ectx);
			_con = ws11->GetConnection();
			_xrender_api = ws11->GetXRenderAPI();
			_window = wx11->GetHandle();
			if (!_xrender_api) throw NotImplementedException();
			if (!_window) throw InvalidStateException();
			auto visual = wx11->GetVisual();
			auto xapi = _con->GetAPI();
			auto display = _con->GetXDisplay();
			int u1, u2;
			if (!_xrender_api->XRenderQueryExtension(display, &u1, &u2)) throw NotImplementedException();
			if (!visual) visual = xapi->XDefaultVisual(display, xapi->XDefaultScreen(display));
			_xrender_format = _xrender_api->XRenderFindVisualFormat(display, visual);
			if (_xrender_format->depth == 24) {
				if (_xrender_format->direct.red == 16 && _xrender_format->direct.blue == 0) _esse_format = Picturae::PixelFormat::B8G8R8;
				else _esse_format = Picturae::PixelFormat::R8G8B8;
			} else {
				if (_xrender_format->direct.red == 16 && _xrender_format->direct.blue == 0) _esse_format = Picturae::PixelFormat::B8G8R8A8;
				else _esse_format = Picturae::PixelFormat::R8G8B8A8;
			}
			_context = 0;
			_viewport = _backbuffer_size = Index2(0, 0);
			_backbuffer = 0;
			_blt_context = 0;
			_surface = 0;
			_online = false;
			if (!Resize(window->GetClientSize())) { Reset(); throw InvalidStateException(); }
		}
		CairoDeviceX11::~CairoDeviceX11(void) { Reset(); }
		string CairoDeviceX11::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Cairo X11 device context"; ESSE_TRY_OUTRO(string()) }
		uint32 CairoDeviceX11::GetImplementationFeatures(void) noexcept
		{
			uint result = Graphica::DeviceContextSupportsInversionEffect | Graphica::DeviceContextSupportsPolygons | Graphica::DeviceContextSupportsLayers;
			result |= Graphica::DeviceContextPresentationContext;
			return result;
		}
		bool CairoDeviceX11::BeginRendering(Graphica::TextureLoadAction load, const Color & clear_color) noexcept
		{
			if (!_online || _state || !_window || !_window_object) return false;
			if (!Resize(_window_object->GetClientSize())) return false;
			if (load == Graphica::TextureLoadAction::Clear) {
				_api->cairo_set_source_rgba(_context, clear_color.r / 255.0, clear_color.g / 255.0, clear_color.b / 255.0, clear_color.a / 255.0);
				_api->cairo_set_operator(_context, 1);
				_api->cairo_paint(_context);
				_api->cairo_set_operator(_context, 2);
			}
			_state = true;
			return true;
		}
		bool CairoDeviceX11::EndRendering(void) noexcept
		{
			if (!_online || !_state) return false;
			auto status = _api->cairo_status(_context);
			_state = false;
			_con->GetAPI()->XCopyArea(_con->GetXDisplay(), _backbuffer, _window, _blt_context, 0, 0, _viewport.x, _viewport.y, 0, 0);
			return status == CAIRO_STATUS_SUCCESS;
		}
		void CairoDeviceX11::Reset(void) noexcept
		{
			_online = _state = false;
			auto xapi = _con->GetAPI();
			auto display = _con->GetXDisplay();
			if (_context) { _api->cairo_destroy(_context); _context = 0; }
			if (_surface) { _api->cairo_surface_destroy(_surface); _surface = 0; }
			if (_backbuffer) { xapi->XFreePixmap(display, _backbuffer); _backbuffer = 0; }
			if (_blt_context) { xapi->XFreeGC(display, _blt_context); _blt_context = 0; }
		}
		void CairoDeviceX11::Invalidate(void) noexcept { _window = 0; _window_object = 0; Reset(); }
		bool CairoDeviceX11::Resize(Index2 size) noexcept
		{
			auto new_size = Index2(max(size.x, 1), max(size.y, 1));
			if (new_size != _viewport && _xrender_format) {
				_viewport = new_size;
				if (_viewport.x <= _backbuffer_size.x && _viewport.y <= _backbuffer_size.y && _viewport.x + 512 > _backbuffer_size.x && _viewport.y + 512 > _backbuffer_size.y) return true;
				_backbuffer_size = Index2(_viewport.x + 256, _viewport.y + 256);
				auto xapi = _con->GetAPI();
				auto display = _con->GetXDisplay();
				if (!_blt_context) {
					_blt_context = xapi->XCreateGC(display, _window, 0, 0);
					if (!_blt_context) { _online = false; return false; }
				}
				if (_backbuffer) {
					xapi->XFreePixmap(display, _backbuffer);
					_backbuffer = 0;
				}
				X11::XWindowAttributes wattr;
				xapi->XGetWindowAttributes(display, _window, &wattr);
				_backbuffer = xapi->XCreatePixmap(display, _window, _backbuffer_size.x, _backbuffer_size.y, wattr.depth);
				if (!_backbuffer) { _online = false; return false; }
				if (_surface) {
					_api->cairo_xlib_surface_set_drawable(_surface, _backbuffer, _backbuffer_size.x, _backbuffer_size.y);
				} else {
					_surface = _api->cairo_xlib_surface_create_with_xrender_format(display, _backbuffer, xapi->XDefaultScreenOfDisplay(display), _xrender_format, _backbuffer_size.x, _backbuffer_size.y);
					if (_api->cairo_surface_status(_surface) != CAIRO_STATUS_SUCCESS) {
						_api->cairo_surface_destroy(_surface);
						_surface = 0;
						_online = false;
						return false;
					}
				}
				if (!_context) {
					_context = _api->cairo_create(_surface);
					if (_api->cairo_status(_context) != CAIRO_STATUS_SUCCESS) {
						_api->cairo_destroy(_context);
						_context = 0;
						_online = false;
						return false;
					}
				}
				_online = true;
				_state = false;
				return true;
			} else return true;
		}
		#endif
	}
}