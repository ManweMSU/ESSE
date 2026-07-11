#include "X11WindowSystem.h"
#include "X11Dispatch.h"
#include <Auxilia/Auxilia.h>
#include <Imagines/Imagines.h>
#include <Fenestrae/Fenestrae.h>
#include <Cor/CorVirtualKeyCodes.h>
#include <Cor/CorSystemInformation.h>
#include <Cor-Linux/CorLinuxClasses.h>
#include <Graphica/Graphica.h>
#include <Graphica-Linux/DeviceCairo.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define DEFINE_ATOM_FIELD(N) Atom N;
#define DEFINE_ATOM_LOAD(N, S) if (!(_atoms.N = _xlib_api->XInternAtom(_con->GetXDisplay(), S, false))) return false;

namespace ESSE
{
	namespace X11
	{
		template<class F> bool EnumerateWindows(XLibAPI * api, Display * display, Window window, Atom wm_pid_atom, pid_t desired, F f)
		{
			Window parent, root;
			Window * children;
			unsigned int count;
			api->XQueryTree(display, window, &parent, &root, &children, &count);
			if (children && count) {
				for (int i = 0; i < count; i++) {
					void * pdata;
					Atom type;
					int format;
					unsigned long read, size;
					api->XGetWindowProperty(display, children[i], wm_pid_atom, 0, __LONG_MAX__, False, AnyPropertyType, &type, &format, &read, &size, &pdata);
					if (format == 32 && read > 0) {
						unsigned long window_pid;
						Memory::MemoryCopy(&window_pid, pdata, sizeof(window_pid));
						if (window_pid == desired) {
							if (f(children[i])) { api->XFree(pdata); api->XFree(children); return true; }
						}
					}
					api->XFree(pdata);
					if (EnumerateWindows(api, display, children[i], wm_pid_atom, desired, f)) { api->XFree(children); return true; }
				}
				api->XFree(children);
			}
			return false;
		}
		oref<XServerConnection> ProvideServerConnection() noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto con = X11::XServerConnection::Query(ectx);
			if (ErrorTest(ectx)) return 0;
			return con;
		}
		bool IsGraphicalProcess(pid_t pid) noexcept
		{
			auto con = ProvideServerConnection();
			if (!con) return false;
			auto api = con->GetAPI();
			auto display = con->GetXDisplay();
			auto desktop = api->XRootWindow(display, api->XDefaultScreen(display));
			auto wm_pid_atom = api->XInternAtom(display, "_NET_WM_PID", False);
			return EnumerateWindows(api, display, desktop, wm_pid_atom, pid, [pid](Window object) -> bool { return true; });
		}
		bool ActivateProcess(pid_t pid) noexcept
		{
			auto con = ProvideServerConnection();
			if (!con) return false;
			auto api = con->GetAPI();
			auto display = con->GetXDisplay();
			auto desktop = api->XRootWindow(display, api->XDefaultScreen(display));
			auto wm_pid_atom = api->XInternAtom(display, "_NET_WM_PID", False);
			return EnumerateWindows(api, display, desktop, wm_pid_atom, pid, [pid, api, display](Window object) -> bool {
				XWindowAttributes attr;
				api->XGetWindowAttributes(display, object, &attr);
				if (attr.map_state != IsUnmapped) {
					XEvent event;
					Memory::ZeroMemory(&event, sizeof(event));
					event.xclient.type = ClientMessage;
					event.xclient.window = object;
					event.xclient.message_type = api->XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
					event.xclient.format = 32;
					event.xclient.data.l[0] = 1;
					event.xclient.data.l[1] = 0;
					event.xclient.data.l[2] = 0;
					api->XSendEvent(display, api->XDefaultRootWindow(display), False, SubstructureNotifyMask | SubstructureRedirectMask, &event);
					api->XSync(display, false);
					return true;
				} else return false;
			});
		}

		Pixmap LoadPixmap(XServerConnection * con, Picturae::Picture * image) noexcept
		{
			oref<Picturae::Picture> conv;
			if (image->GetDesc().format == Picturae::PixelFormat::B8G8R8A8 && image->GetDesc().alpha_mode == Picturae::AlphaMode::Premultiplied && image->GetDesc().origin == Picturae::ScanOrigin::TopLeft) {
				conv = image;
			} else {
				try { conv = image->Convert(Picturae::PixelFormat::B8G8R8A8, Picturae::AlphaMode::Premultiplied, Picturae::ScanOrigin::TopLeft); } catch (...) { return 0; }
			}
			auto api = con->GetAPI();
			auto display = con->GetXDisplay();
			uint size = conv->GetDesc().stride * conv->GetDesc().height;
			auto image_desc = reinterpret_cast<XImage *>(malloc(sizeof(XImage)));
			if (!image_desc) return 0;
			image_desc->width = conv->GetDesc().width;
			image_desc->height = conv->GetDesc().height;
			image_desc->xoffset = 0;
			image_desc->format = ZPixmap;
			image_desc->data = reinterpret_cast<char *>(malloc(size));
			image_desc->byte_order = LSBFirst;
			image_desc->bitmap_unit = 32;
			image_desc->bitmap_bit_order = LSBFirst;
			image_desc->bitmap_pad = 32;
			image_desc->depth = 32;
			image_desc->bytes_per_line = conv->GetDesc().stride;
			image_desc->bits_per_pixel = 32;
			image_desc->red_mask = 0;
			image_desc->green_mask = 0;
			image_desc->blue_mask = 0;
			image_desc->obdata = 0;
			if (!image_desc->data) { free(image_desc); return 0; }
			Memory::MemoryCopy(image_desc->data, conv->GetDesc().data, size);
			if (!api->XInitImage(image_desc)) { free(image_desc->data); free(image_desc); return 0; }
			Pixmap result = api->XCreatePixmap(display, api->XRootWindow(display, api->XDefaultScreen(display)), conv->GetDesc().width, conv->GetDesc().height, 32);
			if (!result) { image_desc->f.destroy_image(image_desc); return 0; }
			GC gc = api->XCreateGC(display, result, 0, 0);
			if (!gc) { api->XFreePixmap(display, result); image_desc->f.destroy_image(image_desc); return 0; }
			api->XPutImage(display, result, gc, image_desc, 0, 0, 0, 0, conv->GetDesc().width, conv->GetDesc().height);
			api->XFreeGC(display, gc);
			image_desc->f.destroy_image(image_desc);
			return result;
		}
		Pixmap CreatePixmap(XServerConnection * con, int width, int height, Color color) noexcept
		{
			auto api = con->GetAPI();
			auto display = con->GetXDisplay();
			Pixmap result = api->XCreatePixmap(display, api->XRootWindow(display, api->XDefaultScreen(display)), width, height, 32);
			if (!result) return 0;
			GC gc = api->XCreateGC(display, result, 0, 0);
			if (!gc) { api->XFreePixmap(display, result); return 0; }
			auto ctr = Picturae::ConvertPixelValue(color.value, Picturae::PixelFormat::R8G8B8A8, Picturae::AlphaMode::Straight, Picturae::PixelFormat::B8G8R8A8, Picturae::AlphaMode::Premultiplied);
			api->XSetForeground(display, gc, ctr);
			api->XFillRectangle(display, result, gc, 0, 0, width, height);
			api->XFlushGC(display, gc);
			api->XFreeGC(display, gc);
			return result;
		}
		Cursor LoadCursor(XServerConnection * con, XRenderAPI * xrndr, Picturae::Picture * image) noexcept
		{
			auto api = con->GetAPI();
			auto display = con->GetXDisplay();
			int u1, u2;
			if (!xrndr->XRenderQueryExtension(display, &u1, &u2)) return 0;
			Cursor result = 0;
			auto pixmap = LoadPixmap(con, image);
			if (pixmap) {
				unsigned int w, h;
				api->XQueryBestCursor(display, pixmap, image->GetDesc().width, image->GetDesc().height, &w, &h);
				if (w == image->GetDesc().width && h == image->GetDesc().height) {
					auto format = xrndr->XRenderFindStandardFormat(display, PictStandardARGB32);
					auto picture = xrndr->XRenderCreatePicture(display, pixmap, format, 0, 0);
					if (picture) {
						result = xrndr->XRenderCreateCursor(display, picture, image->GetAttributes().pointer_offset_x, image->GetAttributes().pointer_offset_y);
						xrndr->XRenderFreePicture(display, picture);
					}
				}
				api->XFreePixmap(display, pixmap);
			}
			return result;
		}

		struct XStandardAtoms
		{
			DEFINE_ATOM_FIELD(net_wm_pid)
			DEFINE_ATOM_FIELD(cardinal)
			DEFINE_ATOM_FIELD(clipboard)
			DEFINE_ATOM_FIELD(incremental)
			DEFINE_ATOM_FIELD(atom)
			DEFINE_ATOM_FIELD(targets)
			DEFINE_ATOM_FIELD(text)
			DEFINE_ATOM_FIELD(text_utf8)
			DEFINE_ATOM_FIELD(pixmap)
			DEFINE_ATOM_FIELD(file_uri)
			DEFINE_ATOM_FIELD(data)
			DEFINE_ATOM_FIELD(data_format)
			DEFINE_ATOM_FIELD(net_wm_state)
			DEFINE_ATOM_FIELD(net_wm_state_maximized_horz)
			DEFINE_ATOM_FIELD(net_wm_state_maximized_vert)
			DEFINE_ATOM_FIELD(net_wm_state_fullscreen)
			DEFINE_ATOM_FIELD(net_wm_state_hidden)
			DEFINE_ATOM_FIELD(net_wm_state_shaded)
			DEFINE_ATOM_FIELD(wm_normal_hints)
			DEFINE_ATOM_FIELD(net_wm_state_demands_attention)
			DEFINE_ATOM_FIELD(net_frame_extents)
			DEFINE_ATOM_FIELD(wm_delete_window)
			DEFINE_ATOM_FIELD(net_wm_context_help)
			DEFINE_ATOM_FIELD(net_request_frame_extents)
			DEFINE_ATOM_FIELD(net_wm_icon)
			DEFINE_ATOM_FIELD(net_wm_window_type_combo)
			DEFINE_ATOM_FIELD(net_wm_window_type_utility)
			DEFINE_ATOM_FIELD(net_wm_window_type_dialog)
			DEFINE_ATOM_FIELD(net_wm_window_type_normal)
			DEFINE_ATOM_FIELD(net_wm_window_type)
			DEFINE_ATOM_FIELD(net_wm_state_modal)
			DEFINE_ATOM_FIELD(net_wm_state_skip_taskbar)
			DEFINE_ATOM_FIELD(net_wm_state_skip_pager)
			DEFINE_ATOM_FIELD(wm_protocols)
			DEFINE_ATOM_FIELD(wm_transient_for)
			DEFINE_ATOM_FIELD(window)
			DEFINE_ATOM_FIELD(kde_net_wm_blur_behind_region)
			DEFINE_ATOM_FIELD(net_wm_cm_s0)
			DEFINE_ATOM_FIELD(wm_name)
			DEFINE_ATOM_FIELD(net_wm_name)
			DEFINE_ATOM_FIELD(net_wm_visible_name)
			DEFINE_ATOM_FIELD(net_active_window)
			DEFINE_ATOM_FIELD(net_wm_window_opacity)
			DEFINE_ATOM_FIELD(net_wm_state_above)
			DEFINE_ATOM_FIELD(net_wm_state_below)
			DEFINE_ATOM_FIELD(xembed_info)
			DEFINE_ATOM_FIELD(net_system_tray_opcode)
			DEFINE_ATOM_FIELD(net_system_tray_visual)
		};
		class XTheme : public Windows::ITheme
		{
			Windows::ThemeColorScheme _scheme;
			Color _accent;
		public:
			XTheme(Windows::ThemeColorScheme scheme, Color accent) : _scheme(scheme), _accent(accent) {}
			virtual ~XTheme(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"X11 Theme"; ESSE_TRY_OUTRO(string()) }
			virtual Windows::ThemeColorScheme GetColorScheme(void) noexcept override { return _scheme; }
			virtual Color GetColor(Windows::ThemeColor color) noexcept override
			{
				if (color == Windows::ThemeColor::Accent) return _accent;
				if (_scheme == Windows::ThemeColorScheme::Light) {
					if (color == Windows::ThemeColor::WindowBackgroup) return Color(0xE0, 0xE0, 0xE0);
					else if (color == Windows::ThemeColor::WindowText) return Color(0x00, 0x00, 0x00);
					else if (color == Windows::ThemeColor::SelectedBackground) return Color(0x36, 0x47, 0xFF, 0x40);
					else if (color == Windows::ThemeColor::SelectedText) return Color(0x00, 0x00, 0x00);
					else if (color == Windows::ThemeColor::MenuBackground) return Color(0x00, 0x00, 0x00, 0x00);
					else if (color == Windows::ThemeColor::MenuText) return Color(0x00, 0x00, 0x00);
					else if (color == Windows::ThemeColor::MenuHotBackground) return Color(0x36, 0x47, 0xFF);
					else if (color == Windows::ThemeColor::MenuHotText) return Color(0xFF, 0xFF, 0xFF);
					else if (color == Windows::ThemeColor::GrayedText) return Color(0x80, 0x80, 0x80);
					else if (color == Windows::ThemeColor::Hyperlink) return Color(0x00, 0x00, 0xFF);
				} else {
					if (color == Windows::ThemeColor::WindowBackgroup) return Color(0x20, 0x20, 0x20);
					else if (color == Windows::ThemeColor::WindowText) return Color(0xFF, 0xFF, 0xFF);
					else if (color == Windows::ThemeColor::SelectedBackground) return Color(0x1B, 0x23, 0xFF, 0x40);
					else if (color == Windows::ThemeColor::SelectedText) return Color(0xFF, 0xFF, 0xFF);
					else if (color == Windows::ThemeColor::MenuBackground) return Color(0x00, 0x00, 0x00, 0x00);
					else if (color == Windows::ThemeColor::MenuText) return Color(0xFF, 0xFF, 0xFF);
					else if (color == Windows::ThemeColor::MenuHotBackground) return Color(0x36, 0x47, 0xFF);
					else if (color == Windows::ThemeColor::MenuHotText) return Color(0xFF, 0xFF, 0xFF);
					else if (color == Windows::ThemeColor::GrayedText) return Color(0x80, 0x80, 0x80);
					else if (color == Windows::ThemeColor::Hyperlink) return Color(0x80, 0x80, 0xFF);
				}
				return 0;
			}
		};
		class XCursorImage : public Windows::ICursor
		{
			oref<XServerConnection> _con;
			Cursor _cursor;
		public:
			XCursorImage(XServerConnection * con, uint shape) : _con(con), _cursor(_con->GetAPI()->XCreateFontCursor(_con->GetXDisplay(), shape)) {}
			XCursorImage(XServerConnection * con, XCursorAPI * exapi, const char * name) : _con(con) { if (exapi) _cursor = exapi->XcursorLibraryLoadCursor(_con->GetXDisplay(), name); else _cursor = 0; }
			XCursorImage(XServerConnection * con, XRenderAPI * exapi, Picturae::Picture * image) : _con(con)
			{
				if (!exapi) throw NotImplementedException();
				_cursor = LoadCursor(con, exapi, image);
				if (!_cursor) throw Exception();
			}
			virtual ~XCursorImage(void) override { if (_cursor) _con->GetAPI()->XFreeCursor(_con->GetXDisplay(), _cursor); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"X11 Cursor"; ESSE_TRY_OUTRO(string()) }
			virtual handle GetOSHandle(void) noexcept override { return reinterpret_cast<handle>(intptr(_cursor)); }
			Cursor GetXHandle(void) noexcept { return _cursor; }
		};
		class XScreen : public Windows::IScreen
		{
			oref<XServerConnection> _con;
			string _name;
			Rectangle _rect, _user;
			Index2 _size;
			double _scale;
		private:
			static oref<Picturae::Picture> _query_pixmap_surface(XServerConnection * con, Pixmap pixmap, int width, int height, int xorg, int yorg) noexcept
			{
				auto api = con->GetAPI();
				oref<Picturae::Picture> result;
				auto image = api->XGetImage(con->GetXDisplay(), pixmap, xorg, yorg, width, height, 0xFFFFFFFF, ZPixmap);
				if (!image) return 0;
				try {
					Picturae::PictureDesc desc;
					desc.width = width;
					desc.height = height;
					desc.stride = image->bytes_per_line;
					if (image->depth == 32) {
						desc.format = Picturae::PixelFormat::B8G8R8A8;
					} else if (image->depth == 24) {
						if (image->red_mask == 0x0000FF && image->green_mask == 0x00FF00 && image->blue_mask == 0xFF0000) {
							if (image->bits_per_pixel == 32) desc.format = Picturae::PixelFormat::R8G8B8X8;
							else if (image->bits_per_pixel == 24) desc.format = Picturae::PixelFormat::R8G8B8;
							else throw Exception();
						} else if (image->red_mask == 0xFF0000 && image->green_mask == 0x00FF00 && image->blue_mask == 0x0000FF) {
							if (image->bits_per_pixel == 32) desc.format = Picturae::PixelFormat::B8G8R8X8;
							else if (image->bits_per_pixel == 24) desc.format = Picturae::PixelFormat::B8G8R8;
							else throw Exception();
						} else throw Exception();
					} else throw Exception();
					desc.alpha_mode = Picturae::AlphaMode::Premultiplied;
					desc.origin = Picturae::ScanOrigin::TopLeft;
					desc.palette_size = 0;
					desc.data = image->data;
					desc.palette = 0;
					result = owrap(new Picturae::Picture(desc, Picturae::PictureInit::AllocateCopy));
				} catch (...) { image->f.destroy_image(image); return 0; }
				image->f.destroy_image(image);
				result->Retain();
				return result;
			}
		public:
			XScreen(XServerConnection * con, const string & name, const Rectangle & rect, const Rectangle & user, double scale) : _con(con), _name(name), _rect(rect), _user(user), _scale(scale)
			{
				_size.x = _rect.right - _rect.left;
				_size.y = _rect.bottom - _rect.top;
			}
			virtual ~XScreen(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"X11 Screen"; ESSE_TRY_OUTRO(string()) }
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept override
			{
				if (cls == Classes.Object || cls == Classes.DynamicObject || cls == Classes.IScreen) {
					Retain(); return this;
				} else if (cls == Linux::Classes::X11_Screen) {
					Retain(); return this;
				} else { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			}
			virtual const void * GetType(void) noexcept override { return Classes.IScreen; }
			virtual string GetName(void) noexcept override { try { return _name; } catch (...) { return string(); } }
			virtual Rectangle GetScreenRectangle(void) noexcept override { return _rect; }
			virtual Rectangle GetUserRectangle(void) noexcept override { return _user; }
			virtual Index2 GetResolution(void) noexcept override { return _size; }
			virtual double GetScaleFactor(void) noexcept override { return _scale; }
			virtual oref<Picturae::Picture> Capture(void) noexcept override
			{
				auto api = _con->GetAPI();
				auto root = api->XRootWindow(_con->GetXDisplay(), api->XDefaultScreen(_con->GetXDisplay()));
				return _query_pixmap_surface(_con, root, _size.x, _size.y, _rect.left, _rect.top);
			}
		};
		class XWindow : public Windows::IWindow, IX11Window, IXWindowEventHandler
		{
			oref<XServerConnection> _con;
			Windows::IWindowSystem * _system;
			IX11WindowSystem * _system_x11;
			XWindow * _parent;
			object_array<XWindow> _children;
			oref<Windows::ICursor> _cursor, _last_cursor_set;
			Windows::IWindowCallback * _callback;
			Dictionary<oref<Windows::IWindowExtensionClass>, oref<Object>> _extensions;
			XStandardAtoms * _atoms;
			Window _window;
			Visual * _visual;
			Colormap _colormap;
			XIC _ic;
			XIM _im;
			bool _modal, _sizeble, _visible, _mapped, _allow_close;
			bool _margins_unknown, _locked;
			int _override_z_order;
			double _opacity;
			Index2 _origin, _size;
			Index2 _min, _max, _last_click_pos;
			Rectangle _margins;
			uint32 _modal_level, _effective_style;
			uint32 _state_mask; // 1 - maximized horz, 2 - maximized vert, 4 - minimized, 8 - fullscreen
			uint32 _last_click_mask, _last_click_time;
		private:
			bool _is_locked(void) noexcept
			{
				if (_locked) return true;
				else if (_modal_level >= _system_x11->GetModalityLevel()) return false; 
				else return true;
			}
			void _internal_show_window(void) noexcept
			{
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				bool must_be_visible = _visible && (!_parent || _parent->_mapped) && (!_parent || !_parent->_locked || _modal);
				if (!_mapped && must_be_visible) {
					api->XMapRaised(display, _window);
					_mapped = true;
					for (auto & c : _children) c._internal_show_window();
					if (_override_z_order) {
						XEvent event;
						Memory::ZeroMemory(&event, sizeof(event));
						event.xclient.type = ClientMessage;
						event.xclient.window = _window;
						event.xclient.message_type = _atoms->net_wm_state;
						event.xclient.format = 32;
						event.xclient.data.l[0] = 1;
						event.xclient.data.l[1] = _override_z_order > 0 ? _atoms->net_wm_state_above : _atoms->net_wm_state_below;
						event.xclient.data.l[2] = 0;
						event.xclient.data.l[3] = 1;
						api->XSendEvent(display, api->XDefaultRootWindow(display), False, SubstructureNotifyMask | SubstructureRedirectMask, &event);
						api->XSync(display, false);
					}
				} else if (_mapped && !must_be_visible) {
					_mapped = false;
					for (auto & c : _children) c._internal_show_window();
					if (_state_mask & 4) api->XWithdrawWindow(display, _window, api->XDefaultScreen(display));
					else api->XUnmapWindow(display, _window);
				}
			}
			uint32 _query_current_state(void) noexcept
			{
				uint32 state = 0;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				Atom * pdata = 0;
				Atom act_type;
				int act_format;
				unsigned long read, size;
				if (api->XGetWindowProperty(display, _window, _atoms->net_wm_state, 0, __LONG_MAX__, false, AnyPropertyType, &act_type, &act_format, &read, &size, reinterpret_cast<void **>(&pdata)) != Success) return 0;
				for (unsigned long i = 0; i < read; i++) {
					auto atom = pdata[i];
					if (atom == _atoms->net_wm_state_maximized_horz) state |= 1;
					else if (atom == _atoms->net_wm_state_maximized_vert) state |= 2;
					else if (atom == _atoms->net_wm_state_fullscreen) state |= (3 | 8);
					else if (atom == _atoms->net_wm_state_hidden) state |= 4;
					else if (atom == _atoms->net_wm_state_shaded) state |= 4;
				}
				api->XFree(pdata);
				return state;
			}
			void _update_window_state(void) noexcept
			{
				uint32 old_state = _state_mask;
				_state_mask = _query_current_state();
				uint32 omx = old_state & 3, nmx = _state_mask & 3;
				uint32 omn = old_state & 4, nmn = _state_mask & 4;
				if (omx < 3) omx = 0;
				if (nmx < 3) nmx = 0;
				if (_callback && (omx != nmx || omn != nmn)) {
					if (nmn == 4) _callback->WindowMinimized(this);
					else if (nmx == 3) _callback->WindowMaximized(this);
					else _callback->WindowRestored(this);
				}
			}
			void _update_hints(void) noexcept
			{
				XSizeHints sz_hints;
				sz_hints.flags = PMinSize | PMaxSize | PPosition;
				sz_hints.x = _origin.x;
				sz_hints.y = _origin.y;
				if (_sizeble) {
					sz_hints.min_width = max(_min.x, 1);
					sz_hints.max_width = _max.x ? _max.x : 0x7FFF;
					sz_hints.min_height = max(_min.y, 1);
					sz_hints.max_height = _max.y ? _max.y : 0x7FFF;
				} else {
					sz_hints.min_width = sz_hints.max_width = _size.x;
					sz_hints.min_height = sz_hints.max_height = _size.y;
				}
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				api->XSetSizeHints(display, _window, &sz_hints, _atoms->wm_normal_hints);
			}
		public:
			XWindow(Windows::IWindowSystem * system, const void * desc) : _system(system), _children(0x20)
			{
				ErrorContext ectx; ErrorClear(ectx);
				_system_x11 = reinterpret_cast<IX11WindowSystem *>(system->DynamicCast(Linux::Classes::X11_WindowSystem, ectx));
				ErrorThrow(ectx);
				_con = _system_x11->GetConnection();
				_cursor = _system->GetSystemCursor(Windows::SystemCursorClass::Arrow);
				_atoms = reinterpret_cast<XStandardAtoms *>(_system_x11->GetStandardAtoms());
				_effective_style = _state_mask = 0;
				_override_z_order = 0;
				_last_click_pos = Index2(0, 0);
				_last_click_mask = _last_click_time = 0;
				_margins = Rectangle(0, 0, 0, 0);
				_visible = _mapped = false;
				_margins_unknown = true;
				_locked = false;
				const Windows::CreateWindowDesc * main_desc = 0;
				const Windows::CreateWindowDescBase * current_desc = reinterpret_cast<const Windows::CreateWindowDescBase *>(desc);
				while (current_desc) {
					if (current_desc->desc_type == Windows::CreateWindowDescType::CreateWindowDesc) main_desc = reinterpret_cast<const Windows::CreateWindowDesc *>(current_desc);
					current_desc = reinterpret_cast<const Windows::CreateWindowDescBase *>(current_desc->next_desc);
				}
				if (!main_desc) throw InvalidArgumentException();
				_callback = main_desc->callback;
				_parent = static_cast<XWindow *>(main_desc->parent_window.Inner());
				if (_parent && !_parent->_window) throw InvalidArgumentException();
				Rectangle rect;
				if (main_desc->screen) {
					auto screen_rect = main_desc->screen->GetScreenRectangle();
					rect.left = main_desc->position.left + screen_rect.left;
					rect.top = main_desc->position.top + screen_rect.top;
					rect.right = main_desc->position.right + screen_rect.left;
					rect.bottom = main_desc->position.bottom + screen_rect.top;
				} else rect = main_desc->position;
				_origin.x = rect.left;
				_origin.y = rect.top;
				int w = max(rect.right - rect.left, 1);
				int h = max(rect.bottom - rect.top, 1);
				if (main_desc->style & Windows::WindowStyleResizeble) _sizeble = true; else _sizeble = false;
				XSetWindowAttributes attr;
				attr.override_redirect = (main_desc->style & Windows::WindowStylePopup) ? true : false;
				attr.save_under = (main_desc->style & Windows::WindowStylePopup) ? true : false;
				attr.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask | FocusChangeMask | PropertyChangeMask | ExposureMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				auto root = api->XRootWindow(display, api->XDefaultScreen(display));
				_min = main_desc->minimal_constraints;
				_max = main_desc->maximal_constraints;
				_size = Index2(w, h);
				if (main_desc->style & Windows::WindowStylePopup) _effective_style |= Windows::WindowStylePopup;
				else _effective_style |= Windows::WindowStyleHasTitle;
				if (_sizeble) _effective_style |= Windows::WindowStyleResizeble;
				_effective_style |= Windows::WindowStyleMinimizeButton | Windows::WindowStyleMaximizeButton;
				Visual * use_visual = 0;
				int use_depth = 0;
				unsigned long use_mask = CWEventMask | CWSaveUnder | CWOverrideRedirect;
				_visual = 0;
				if (main_desc->style & Windows::WindowStyleTopmost) { _override_z_order = 1; _effective_style |= Windows::WindowStyleTopmost; }
				if (main_desc->style & Windows::WindowStyleBottommost) { _override_z_order = -1; _effective_style |= Windows::WindowStyleBottommost; }
				if (main_desc->style & Windows::WindowStyleTransparent) _effective_style |= Windows::WindowStyleTransparent;
				if (main_desc->style & Windows::WindowStyleSetBlurBehind) _effective_style |= Windows::WindowStyleTransparent | Windows::WindowStyleSetBlurBehind;
				if (!api->XGetSelectionOwner(display, _atoms->net_wm_cm_s0)) _effective_style &= (Windows::WindowStyleTransparent | Windows::WindowStyleSetBlurBehind);
				if (_effective_style & Windows::WindowStyleTransparent) {
					XVisualInfo use_info;
					if (api->XMatchVisualInfo(display, api->XDefaultScreen(display), 32, TrueColor, &use_info)) {
						use_depth = use_info.depth;
						use_visual = _visual = use_info.visual;
						attr.colormap = api->XCreateColormap(display, root, use_visual, AllocNone);
						attr.background_pixel = attr.border_pixel = 0;
						use_mask |= CWColormap | CWBorderPixel | CWBackPixel;
					} else _effective_style &= (Windows::WindowStyleTransparent | Windows::WindowStyleSetBlurBehind);
				}
				if (_effective_style & Windows::WindowStyleSetBlurBehind) {
					void * pdata = 0;
					Atom type;
					int format;
					unsigned long read, size;
					api->XGetWindowProperty(display, root, _atoms->kde_net_wm_blur_behind_region, 0, 0, false, AnyPropertyType, &type, &format, &read, &size, &pdata);
					api->XFree(pdata);
					if (!type) _effective_style &= ~Windows::WindowStyleSetBlurBehind;
				}
				_window = api->XCreateWindow(display, root, rect.left, rect.top, w, h, 0, use_depth, InputOutput, use_visual, use_mask, &attr);
				if (_visual) _colormap = attr.colormap; else _colormap = 0;
				if (!_system_x11->GetDispatch()->RegisterWindowHandler(_window, this)) {
					api->XDestroyWindow(display, _window);
					if (_colormap) api->XFreeColormap(display, _colormap);
					throw OutOfMemoryException();
				}
				if (main_desc->style & Windows::WindowStyleModal) {
					_modal = true;
					_effective_style |= Windows::WindowStyleModal;
				} else _modal = false;
				if (_parent) _modal_level = _parent->_modal_level;
				else if (_modal) {
					_modal_level = _system_x11->GetModalityLevel() + 1;
					_system_x11->SetModalityLevel(_modal_level);
				} else _modal_level = 0;
				if (main_desc->style & Windows::WindowStyleSetOpacity) _opacity = main_desc->opacity;
				else _opacity = 1.0;
				_update_hints();
				SetTitle(main_desc->title);
				unsigned long wnd_pid = getpid();
				api->XChangeProperty(display, _window, _atoms->net_wm_pid, _atoms->cardinal, 32, PropModeReplace, reinterpret_cast<uint8 *>(&wnd_pid), 1);
				Atom wnd_type;
				if (main_desc->style & Windows::WindowStylePopup) wnd_type = _atoms->net_wm_window_type_combo;
				else if (main_desc->style & Windows::WindowStyleToolWindow) {
					wnd_type = _atoms->net_wm_window_type_utility;
					_effective_style |= Windows::WindowStyleToolWindow;
				}
				else if (_parent) wnd_type = _atoms->net_wm_window_type_dialog;
				else wnd_type = _atoms->net_wm_window_type_normal;
				api->XChangeProperty(display, _window, _atoms->net_wm_window_type, _atoms->atom, 32, PropModeReplace, reinterpret_cast<uint8 *>(&wnd_type), 1);
				if (_modal && _parent) {
					wnd_type = _atoms->net_wm_state_modal;
					api->XChangeProperty(display, _window, _atoms->net_wm_state, _atoms->atom, 32, PropModeAppend, reinterpret_cast<uint8 *>(&wnd_type), 1);
				}
				if (_parent || (main_desc->style & Windows::WindowStylePopup)) {
					wnd_type = _atoms->net_wm_state_skip_taskbar;
					api->XChangeProperty(display, _window, _atoms->net_wm_state, _atoms->atom, 32, PropModeAppend, reinterpret_cast<uint8 *>(&wnd_type), 1);
					wnd_type = _atoms->net_wm_state_skip_pager;
					api->XChangeProperty(display, _window, _atoms->net_wm_state, _atoms->atom, 32, PropModeAppend, reinterpret_cast<uint8 *>(&wnd_type), 1);
				}
				wnd_type = _atoms->wm_delete_window;
				api->XChangeProperty(display, _window, _atoms->wm_protocols, _atoms->atom, 32, PropModeReplace, reinterpret_cast<uint8 *>(&wnd_type), 1);
				if (main_desc->style & Windows::WindowStyleHelpButton) {
					_effective_style |= Windows::WindowStyleHelpButton;
					wnd_type = _atoms->net_wm_context_help;
					api->XChangeProperty(display, _window, _atoms->wm_protocols, _atoms->atom, 32, PropModeAppend, reinterpret_cast<uint8 *>(&wnd_type), 1);
				}
				if (_parent) api->XChangeProperty(display, _window, _atoms->wm_transient_for, _atoms->window, 32, PropModeReplace, reinterpret_cast<uint8 *>(&_parent->_window), 1);
				_allow_close = (main_desc->style & Windows::WindowStyleCloseButton) ? true : false;
				if (_allow_close) _effective_style |= Windows::WindowStyleCloseButton;
				Picturae::Image * icon;
				if (icon = _system_x11->GetApplicationIcon()) try {
					array<unsigned long> icon_data(0x10000);
					for (auto & f : *icon) {
						auto fc = f.Convert(Picturae::PixelFormat::B8G8R8A8, Picturae::AlphaMode::Straight, Picturae::ScanOrigin::TopLeft);
						icon_data << fc->GetDesc().width;
						icon_data << fc->GetDesc().height;
						for (uint y = 0; y < fc->GetDesc().height; y++) for (uint x = 0; x < fc->GetDesc().width; x++) icon_data << fc->GetPixel(x, y);
					}
					api->XChangeProperty(display, _window, _atoms->net_wm_icon, _atoms->cardinal, 32, PropModeReplace, reinterpret_cast<uint8 *>(icon_data.GetBuffer()), icon_data.GetLength());
				} catch (...) {}
				if (_effective_style & Windows::WindowStyleSetBlurBehind) {
					unsigned long value = 0;
					auto type = _atoms->cardinal;
					api->XChangeProperty(display, _window, _atoms->kde_net_wm_blur_behind_region, type, 32, PropModeReplace, reinterpret_cast<uint8 *>(&value), 1);
				}
				if (!(main_desc->style & Windows::WindowStylePopup)) {
					XWMHints hints;
					hints.flags = InputHint;
					hints.input = true;
					api->XSetWMHints(display, _window, &hints);
				}
				if (main_desc->style & Windows::WindowStyleSetOpacity) SetOpacity(_opacity);
				_im = api->XOpenIM(display, 0, 0, 0);
				_ic = api->XCreateIC(_im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, _window, NULL);
				if (!_im || !_ic) {
					if (_ic) api->XDestroyIC(_ic);
					if (_im) api->XCloseIM(_im);
					_system_x11->GetDispatch()->UnregisterWindowHandler(_window);
					api->XDestroyWindow(display, _window);
					if (_colormap) api->XFreeColormap(display, _colormap);
					throw OutOfMemoryException();
				}
				XEvent event;
				Memory::ZeroMemory(&event, sizeof(event));
				event.xclient.type = ClientMessage;
				event.xclient.display = display;
				event.xclient.send_event = true;
				event.xclient.window = _window;
				event.xclient.message_type = _atoms->net_request_frame_extents;
				event.xclient.format = 32;
				api->XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
				if (_parent) {
					try { _parent->_children.Append(this); } catch (...) {
						api->XDestroyIC(_ic);
						api->XCloseIM(_im);
						_system_x11->GetDispatch()->UnregisterWindowHandler(_window);
						api->XDestroyWindow(display, _window);
						if (_colormap) api->XFreeColormap(display, _colormap);
						throw;
					}
				} else if (!_system_x11->RegisterTopLevelWindow(this)) {
					api->XDestroyIC(_ic);
					api->XCloseIM(_im);
					_system_x11->GetDispatch()->UnregisterWindowHandler(_window);
					api->XDestroyWindow(display, _window);
					if (_colormap) api->XFreeColormap(display, _colormap);
					throw;
				}
				if (_modal && _parent) _parent->LockWindow(true);
				if (_callback) _callback->Created(this);
			}
			virtual ~XWindow(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"X11 Window"; ESSE_TRY_OUTRO(string()) }
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept override
			{
				if (cls == Classes.Object || cls == Classes.DynamicObject || cls == Classes.IWindow) {
					Retain(); return this;
				} else if (cls == Linux::Classes::X11_Window) {
					return static_cast<IX11Window *>(this);
				} else { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			}
			virtual const void * GetType(void) noexcept override { return Classes.IWindow; }
			virtual void HandleEvent(Window window, XEvent * event) noexcept override
			{
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				if (event->type == Expose) {
					if (!event->xexpose.count && _callback) _callback->RenderWindow(this);
				} else if (event->type == MapNotify) {
					if (_callback) _callback->Shown(this, true);
					api->XResizeWindow(display, _window, _size.x, _size.y);
					api->XFlush(display);
				} else if (event->type == UnmapNotify) {
					_update_hints();
					if (_callback) _callback->Shown(this, false);
				} else if (event->type == ClientMessage) {
					if (!_is_locked()) {
						if (event->xclient.data.l[0] == _atoms->wm_delete_window && _allow_close) {
							if (_callback) _callback->WindowClosed(this);
						}
						if (event->xclient.data.l[0] == _atoms->net_wm_context_help) {
							if (_callback) _callback->WindowHelpRequired(this);
						}
					}
				} else if (event->type == ConfigureNotify) {
					int dx = event->xconfigure.x - _origin.x;
					int dy = event->xconfigure.y - _origin.y;
					bool moved = (dx || dy);
					_origin.x = event->xconfigure.x;
					_origin.y = event->xconfigure.y;
					bool sized = (event->xconfigure.width != _size.x || event->xconfigure.height != _size.y);
					if (moved) {
						if (_callback) _callback->WindowMoved(this);
					}
					if (sized) {
						_size.x = event->xconfigure.width;
						_size.y = event->xconfigure.height;
						if (_callback) _callback->WindowResized(this);
					}
				} else if (event->type == PropertyNotify) {
					auto ext_prop = _atoms->net_frame_extents;
					if (event->xproperty.atom == ext_prop && event->xproperty.state == PropertyNewValue) {
						unsigned long * pdata = 0;
						Atom act_type;
						int act_format;
						unsigned long read, size;
						api->XGetWindowProperty(display, _window, ext_prop, 0, __LONG_MAX__, false, AnyPropertyType, &act_type, &act_format, &read, &size, reinterpret_cast<void **>(&pdata));
						_margins.left = (read >= 4) ? pdata[0] : 0;
						_margins.top = (read >= 4) ? pdata[2] : 0;
						_margins.right = (read >= 4) ? pdata[1] : 0;
						_margins.bottom = (read >= 4) ? pdata[3] : 0;
						api->XFree(pdata);
						if (_margins_unknown) {
							_margins_unknown = false;
							api->XMoveWindow(display, _window, _origin.x - _margins.left, _origin.y - _margins.top);
						}
					} else if (event->xproperty.atom == _atoms->net_wm_state) {
						_update_window_state();
					}
				} else if (event->type == FocusIn) {
					if (_callback) { _callback->WindowActivated(this); _callback->FocusChanged(this, true); }
					api->XSetICFocus(_ic);
					auto root = api->XRootWindow(display, api->XDefaultScreen(display));
					XEvent event;
					Memory::ZeroMemory(&event, sizeof(event));
					event.xclient.type = ClientMessage;
					event.xclient.display = display;
					event.xclient.send_event = true;
					event.xclient.window = _window;
					event.xclient.message_type = _atoms->net_wm_state;
					event.xclient.format = 32;
					event.xclient.data.l[0] = 0;
					event.xclient.data.l[1] = _atoms->net_wm_state_demands_attention;
					event.xclient.data.l[2] = 0;
					event.xclient.data.l[3] = 1;
					api->XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
				} else if (event->type == FocusOut) {
					api->XUnsetICFocus(_ic);
					if (_callback) { _callback->FocusChanged(this, false); _callback->WindowDeactivated(this); }
				} else if (event->type == KeyPress) {
					if (!_is_locked() && _callback) {
						auto code = XKeyCodeToESSE(event->xkey.keycode);
						uint mask = 0;
						if (event->xkey.state & ShiftMask) mask |= VirtualKeyModifiers::Shift;
						if (event->xkey.state & ControlMask) mask |= VirtualKeyModifiers::Control;
						if (event->xkey.state & _system_x11->GetAlternativeKeyMod()) mask |= VirtualKeyModifiers::Alternative;
						if (event->xkey.state & _system_x11->GetSystemKeyMod()) mask |= VirtualKeyModifiers::System;
						auto status = code ? _callback->KeyIsDown(this, code, mask) : false;
						if (!status) {
							int len;
							KeySym sym;
							Status lookup_status;
							array<unichar32> ucs(0x100);
							ucs.SetLength(0x10);
							while (true) {
								len = api->XwcLookupString(_ic, &event->xkey, ucs.GetBuffer(), ucs.GetLength(), &sym, &lookup_status);
								if (lookup_status == XBufferOverflow) { ucs.SetLength(len + 1); continue; }
								else break;
							}
							if (lookup_status == XLookupChars || lookup_status == XLookupBoth) {
								for (int i = 0; i < len; i++) _callback->CharacterIsDown(this, ucs[i]);
							} else if (code == VirtualKeyCodes::Tab) {
								_callback->CharacterIsDown(this, U'\t');
							}
						}
					}
				} else if (event->type == KeyRelease) {
					if (!_is_locked() && _callback) {
						auto code = XKeyCodeToESSE(event->xkey.keycode);
						uint mask = 0;
						if (event->xkey.state & ShiftMask) mask |= VirtualKeyModifiers::Shift;
						if (event->xkey.state & ControlMask) mask |= VirtualKeyModifiers::Control;
						if (event->xkey.state & _system_x11->GetAlternativeKeyMod()) mask |= VirtualKeyModifiers::Alternative;
						if (event->xkey.state & _system_x11->GetSystemKeyMod()) mask |= VirtualKeyModifiers::System;
						if (code) _callback->KeyIsUp(this, code, mask);
					}
				} else if (event->type == ButtonPress) {
					if (!_is_locked() && _callback) {
						auto pos = Index2(event->xbutton.x, event->xbutton.y);
						uint32 mask = 0;
						if (event->xbutton.button == Button1) mask = 1;
						else if (event->xbutton.button == Button3) mask = 2;
						else {
							if (event->xbutton.button == Button4) _callback->ScrollVertically(this, pos, -1.0);
							else if (event->xbutton.button == Button5) _callback->ScrollVertically(this, pos, 1.0);
							else if (event->xbutton.button == 6) _callback->ScrollHorizontally(this, pos, -1.0);
							else if (event->xbutton.button == 7) _callback->ScrollHorizontally(this, pos, 1.0);
						}
						if (mask) {
							uint32 ts = System::GetMonotonicTime();
							if (pos == _last_click_pos && mask == _last_click_mask && ts - _last_click_time <= 750) {
								if (mask == 1) _callback->LeftButtonIsDown(this, pos, true);
								else if (mask == 2) _callback->RightButtonIsDown(this, pos, true);
								mask = 0;
							} else {
								if (mask == 1) _callback->LeftButtonIsDown(this, pos, false);
								else if (mask == 2) _callback->RightButtonIsDown(this, pos, false);
							}
							_last_click_pos = pos;
							_last_click_mask = mask;
							_last_click_time = ts;
						}
					}
				} else if (event->type == ButtonRelease) {
					if (!_is_locked() && _callback) {
						auto pos = Index2(event->xbutton.x, event->xbutton.y);
						if (event->xbutton.button == Button1) _callback->LeftButtonIsUp(this, pos);
						else if (event->xbutton.button == Button3) _callback->RightButtonIsUp(this, pos);
					}
				} else if (event->type == MotionNotify) {
					oref<Windows::ICursor> cursor;
					if (!_is_locked() && _callback) {
						auto pos = Index2(event->xmotion.x, event->xmotion.y);
						uint mask = 0;
						if (event->xmotion.state & Button1Mask) mask |= Windows::MouseLeftButtonIsDown;
						if (event->xmotion.state & Button3Mask) mask |= Windows::MouseRightButtonIsDown;
						_callback->MouseMoved(this, pos, mask);
						cursor = _cursor;
					} else cursor = _system->GetSystemCursor(Windows::SystemCursorClass::Arrow);
					if (cursor && _last_cursor_set.Inner() != cursor.Inner()) {
						auto cursor_handle = Cursor(reinterpret_cast<uintptr>(cursor->GetOSHandle()));
						api->XDefineCursor(display, _window, cursor_handle);
						_last_cursor_set = cursor;
					}
				} else if (event->type == EnterNotify) {
					if (!_is_locked() && _callback) {
						uint mask = 0;
						if (event->xmotion.state & Button1Mask) mask |= Windows::MouseLeftButtonIsDown;
						if (event->xmotion.state & Button3Mask) mask |= Windows::MouseRightButtonIsDown;
						_callback->MouseEntered(this, mask);
					}
				} else if (event->type == LeaveNotify) {
					if (!_is_locked() && _callback) {
						uint mask = 0;
						if (event->xmotion.state & Button1Mask) mask |= Windows::MouseLeftButtonIsDown;
						if (event->xmotion.state & Button3Mask) mask |= Windows::MouseRightButtonIsDown;
						_callback->MouseLeft(this, mask);
					}
				}
			}
			virtual void HandleTimer(Window window, int timer) noexcept override { if (_callback) _callback->Timer(this, timer); }
			virtual Window GetHandle(void) noexcept override { return _window; }
			virtual Visual * GetVisual(void) noexcept override { return _visual; }
			virtual bool GetFullscreenState(void) noexcept override { return (_state_mask & 8) == 8; }
			virtual void SetFullscreenState(bool set) noexcept override
			{
				if (set == GetFullscreenState()) return;
				if (set) {
					auto api = _con->GetAPI();
					auto display = _con->GetXDisplay();
					auto root = api->XRootWindow(display, api->XDefaultScreen(display));
					XEvent event;
					Memory::ZeroMemory(&event, sizeof(event));
					event.xclient.type = ClientMessage;
					event.xclient.display = display;
					event.xclient.send_event = true;
					event.xclient.window = _window;
					event.xclient.message_type = _atoms->net_wm_state;
					event.xclient.format = 32;
					event.xclient.data.l[0] = 0;
					event.xclient.data.l[1] = _atoms->net_wm_state_maximized_vert;
					event.xclient.data.l[2] = _atoms->net_wm_state_maximized_horz;
					event.xclient.data.l[3] = 1;
					api->XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
					event.xclient.data.l[0] = 1;
					event.xclient.data.l[1] = _atoms->net_wm_state_fullscreen;
					event.xclient.data.l[2] = 0;
					api->XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
				} else Restore();
			}
			virtual bool IsWindowLocked(void) noexcept override { return _locked; }
			virtual void LockWindow(bool lock) noexcept override { _locked = lock; for (auto & c : _children) c._internal_show_window(); }
			virtual Windows::IWindowSystem * GetWindowSystem(void) noexcept override { return _system; }
			virtual IWindow * GetParentWindow(void) noexcept override { return _parent; }
			virtual IWindow * GetChildWindow(uintptr index) noexcept override { return &_children[index]; }
			virtual uintptr GetChildrenCount(void) noexcept override { return _children.GetLength(); }
			virtual void Destroy(void) noexcept override
			{
				if (!_window || (_modal_level && _modal_level < _system_x11->GetModalityLevel())) return;
				if (_callback) _callback->Destroyed(this);
				_callback = 0;
				while (_children.GetLength()) _children.LastElement().Destroy();
				for (auto & x : _extensions) x.key->ExtensionDetached(this, x.value);
				_extensions.Clear();
				Retain();
				if (_modal) {
					if (_parent) {
						bool unlock = true;
						for (auto & c : _parent->_children) if (c._modal && &c != this) { unlock = false; break; }
						if (unlock) _parent->LockWindow(false);
					} else _system_x11->SetModalityLevel(_modal_level - 1);
				}
				if (_parent) {
					for (uintptr i = 0; i < _parent->_children.GetLength(); i++) if (_parent->_children(i) == this) {
						_parent->_children.Remove(i);
						break;
					}
				} else _system_x11->UnregisterTopLevelWindow(this);
				_system_x11->GetDispatch()->UnregisterWindowHandler(_window);
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				api->XDestroyIC(_ic);
				api->XCloseIM(_im);
				api->XDestroyWindow(display, _window);
				if (_colormap) api->XFreeColormap(display, _colormap);
				_ic = 0; _im = 0; _window = 0; _colormap = 0;
				_cursor.Clear(); _last_cursor_set.Clear();
				Release();
			}
			virtual uint GetEffectiveStyle(Windows::CreateWindowDescType domain) noexcept override { return _effective_style; }
			virtual bool GetVisibility(void) noexcept override { return _visible; }
			virtual void SetVisibility(const bool & show) noexcept override { if (!_window) return; _visible = show; _internal_show_window(); }
			virtual string GetTitle(void) noexcept override
			{
				try {
					if (!_window) return string();
					string result;
					char * pdata = 0;
					Atom act_type;
					int act_format;
					unsigned long read, size;
					auto api = _con->GetAPI();
					auto display = _con->GetXDisplay();
					if (api->XGetWindowProperty(display, _window, _atoms->net_wm_name, 0, __LONG_MAX__, false, AnyPropertyType, &act_type, &act_format, &read, &size, reinterpret_cast<void **>(&pdata)) != Success) return string();
					try { result = string(pdata, read); } catch (...) {}
					api->XFree(pdata);
					return result;
				} catch (...) { return string(); }
			}
			virtual void SetTitle(const string & text) noexcept override
			{
				if (!_window) return;
				try {
					ucs1_string utf8 = text;
					auto api = _con->GetAPI();
					auto display = _con->GetXDisplay();
					api->XChangeProperty(display, _window, _atoms->wm_name, _atoms->text_utf8, 8, PropModeReplace, utf8.GetData(), utf8.GetLength() + 1);
					api->XChangeProperty(display, _window, _atoms->net_wm_name, _atoms->text_utf8, 8, PropModeReplace, utf8.GetData(), utf8.GetLength() + 1);
					api->XChangeProperty(display, _window, _atoms->net_wm_visible_name, _atoms->text_utf8, 8, PropModeReplace, utf8.GetData(), utf8.GetLength() + 1);
				} catch (...) {}
			}
			virtual Rectangle GetPosition(void) noexcept override { return Rectangle(_origin.x, _origin.y, _origin.x + _size.x, _origin.y + _size.y); }
			virtual void SetPosition(const Rectangle & rect) noexcept override
			{
				if (!_window) return;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				auto old_org = _origin, old_size = _size;
				_origin = Index2(rect.left, rect.top);
				_size = Index2(rect.right - rect.left, rect.bottom - rect.top);
				if (_size.x < 1) _size.x = 1;
				if (_size.y < 1) _size.y = 1;
				if (_callback) {
					if (old_org != _origin) _callback->WindowMoved(this);
					if (old_size != _size) _callback->WindowResized(this);
				}
				_update_hints();
				api->XMoveWindow(display, _window, _origin.x - _margins.left, _origin.y - _margins.top);
				api->XResizeWindow(display, _window, _size.x, _size.y);
				api->XFlush(display);
			}
			virtual Index2 GetClientSize(void) noexcept override { return _size; }
			virtual Index2 GetMinimalConstraints(void) noexcept override { return _min; }
			virtual void SetMinimalConstraints(const Index2 & size) noexcept override { if (!_window) return; _min = size; _update_hints(); }
			virtual Index2 GetMaximalConstraints(void) noexcept override { return _max; }
			virtual void SetMaximalConstraints(const Index2 & size) noexcept override { if (!_window) return; _max = size; _update_hints(); }
			virtual double GetOpacity(void) noexcept override { return _opacity; }
			virtual void SetOpacity(const double & opacity) noexcept override
			{
				if (!_window) return;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				_opacity = opacity;
				unsigned long value = uint(0xFFFFFFFF * min(max(_opacity, 0.0), 1.0));
				api->XChangeProperty(display, _window, _atoms->net_wm_window_opacity, _atoms->cardinal, 32, PropModeReplace, reinterpret_cast<uint8 *>(&value), 1);
			}
			virtual Windows::CloseButtonState GetCloseButtonState(void) noexcept override { return _allow_close ? Windows::CloseButtonState::Enabled : Windows::CloseButtonState::Disabled; }
			virtual void SetCloseButtonState(const Windows::CloseButtonState & state) noexcept override { _allow_close = state != Windows::CloseButtonState::Disabled; }
			virtual bool IsActive(void) noexcept override { return IsFocused(); }
			virtual bool IsMaximized(void) noexcept override { return (_state_mask & 3) == 3; }
			virtual bool IsMinimized(void) noexcept override { return (_state_mask & 4) == 4; }
			virtual void Activate(void) noexcept override
			{
				if (!_window) return;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				XEvent event;
				Memory::ZeroMemory(&event, sizeof(event));
				event.xclient.type = ClientMessage;
				event.xclient.window = _window;
				event.xclient.message_type = _atoms->net_active_window;
				event.xclient.format = 32;
				event.xclient.data.l[0] = 1;
				event.xclient.data.l[1] = 0;
				event.xclient.data.l[2] = 0;
				api->XSendEvent(display, api->XDefaultRootWindow(display), False, SubstructureNotifyMask | SubstructureRedirectMask, &event);
				api->XSync(display, false);
			}
			virtual void Maximize(void) noexcept override
			{
				if (!_window) return;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				auto root = api->XRootWindow(display, api->XDefaultScreen(display));
				XEvent event;
				Memory::ZeroMemory(&event, sizeof(event));
				event.xclient.type = ClientMessage;
				event.xclient.display = display;
				event.xclient.send_event = true;
				event.xclient.window = _window;
				event.xclient.message_type = _atoms->net_wm_state;
				event.xclient.format = 32;
				event.xclient.data.l[0] = 1;
				event.xclient.data.l[1] = _atoms->net_wm_state_maximized_vert;
				event.xclient.data.l[2] = _atoms->net_wm_state_maximized_horz;
				event.xclient.data.l[3] = 1;
				api->XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
				event.xclient.data.l[0] = 0;
				event.xclient.data.l[1] = _atoms->net_wm_state_fullscreen;
				event.xclient.data.l[2] = 0;
				api->XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
			}
			virtual void Minimize(void) noexcept override
			{
				if (!_window) return;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				if (!(_state_mask & 4)) api->XIconifyWindow(display, _window, api->XDefaultScreen(display));
			}
			virtual void Restore(void) noexcept override
			{
				if (!_window) return;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				if (_state_mask & 4) api->XMapWindow(display, _window);
				auto root = api->XRootWindow(display, api->XDefaultScreen(display));
				XEvent event;
				Memory::ZeroMemory(&event, sizeof(event));
				event.xclient.type = ClientMessage;
				event.xclient.display = display;
				event.xclient.send_event = true;
				event.xclient.window = _window;
				event.xclient.message_type = _atoms->net_wm_state;
				event.xclient.format = 32;
				event.xclient.data.l[0] = 0;
				event.xclient.data.l[1] = _atoms->net_wm_state_maximized_vert;
				event.xclient.data.l[2] = _atoms->net_wm_state_maximized_horz;
				event.xclient.data.l[3] = 1;
				api->XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
				event.xclient.data.l[0] = 0;
				event.xclient.data.l[1] = _atoms->net_wm_state_fullscreen;
				event.xclient.data.l[2] = 0;
				api->XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
			}
			virtual void RequireAttention(void) noexcept override
			{
				if (!_window) return;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				auto root = api->XRootWindow(display, api->XDefaultScreen(display));
				XEvent event;
				Memory::ZeroMemory(&event, sizeof(event));
				event.xclient.type = ClientMessage;
				event.xclient.display = display;
				event.xclient.send_event = true;
				event.xclient.window = _window;
				event.xclient.message_type = _atoms->net_wm_state;
				event.xclient.format = 32;
				event.xclient.data.l[0] = 1;
				event.xclient.data.l[1] = _atoms->net_wm_state_demands_attention;
				event.xclient.data.l[2] = 0;
				event.xclient.data.l[3] = 1;
				api->XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
			}
			virtual void SetProgressMode(const Windows::ProgressDisplayMode & mode) noexcept override {}
			virtual void SetProgressValue(const double & value) noexcept override {}
			virtual void SetCocoaEffectMaterial(const Windows::CocoaEffectMaterial & material) noexcept override {}
			virtual Windows::IWindowCallback * GetCallback(void) noexcept override { return _callback; }
			virtual void SetCallback(Windows::IWindowCallback * const & callback) noexcept override { if (!_window) return; _callback = callback; }
			virtual void Invalidate(void) noexcept override { if (!_window) return; _system_x11->GetDispatch()->ScheduleWindowUpdate(_window); }
			virtual bool PerformHitTest(const Index2 & at) noexcept override
			{
				if (!_window) return false;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				Window root = api->XRootWindow(display, api->XDefaultScreen(display)), child, null;
				int x, y, atx = at.x, aty = at.y;
				while (true) {
					if (!api->XTranslateCoordinates(display, root, root, at.x, at.y, &x, &y, &child)) return false;
					if (child) {
						if (child == _window) return true;
						if (!api->XTranslateCoordinates(display, root, child, x, y, &atx, &aty, &null)) return false;
						root = child;
					} else return false;
				}
			}
			virtual Index2 ConvertClientToGlobal(const Index2 & at) noexcept override
			{
				if (!_window) return Index2(0, 0);
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				auto root = api->XRootWindow(display, api->XDefaultScreen(display));
				Window child;
				int x, y;
				if (api->XTranslateCoordinates(display, _window, root, at.x, at.y, &x, &y, &child)) return Index2(x, y);
				else return Index2(0, 0);
			}
			virtual Index2 ConvertGlobalToClient(const Index2 & at) noexcept override
			{
				if (!_window) return Index2(0, 0);
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				auto root = api->XRootWindow(display, api->XDefaultScreen(display));
				Window child;
				int x, y;
				if (api->XTranslateCoordinates(display, root, _window, at.x, at.y, &x, &y, &child)) return Index2(x, y);
				else return Index2(0, 0);
			}
			virtual Windows::ICursor * GetCursor(void) noexcept override { return _cursor; }
			virtual void SetCursor(Windows::ICursor * const & cursor) noexcept override { if (!_window) return; _cursor = cursor; }
			virtual bool IsFocused(void) noexcept override
			{
				if (!_window) return false;
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				Window focused;
				int revert;
				api->XGetInputFocus(display, &focused, &revert);
				return focused == _window;
			}
			virtual void SetFocus(void) noexcept override {}
			virtual void SetTimer(uint32 id, uint32 period) noexcept override
			{
				if (!_window) return;
				auto dispatch = _system_x11->GetDispatch();
				dispatch->DestroyTimer(_window, id);
				if (period) dispatch->CreateTimer(_window, id, period);
			}
			virtual bool AddExtension(Object * ext, Windows::IWindowExtensionClass * extcls) noexcept override
			{
				try {
					if (!_window || !ext || !extcls) return false;
					if (_extensions.Append(extcls, ext)) {
						if (!extcls->ExtensionAttached(this, ext)) { _extensions.Remove(extcls); return false; }
						else return true;
					} else return false;
				} catch (...) { return false; }
			}
			virtual bool RemoveExtension(Windows::IWindowExtensionClass * extcls) noexcept override
			{
				if (!_window) return false;
				auto object = _extensions[extcls];
				if (!object) return false;
				extcls->ExtensionDetached(this, *object);
				_extensions.Remove(extcls);
				return true;
			}
			virtual double GetScaleFactor(void) noexcept override { if (!_window) return 0.0; return _system_x11->GetSystemScaleFactor(); }
			virtual oref<Windows::IScreen> GetScreen(void) noexcept override { if (!_window) return 0; return _system_x11->GetScreenWithBestCoverage(GetPosition()); }
			virtual oref<Windows::ITheme> GetTheme(void) noexcept override { if (!_window) return 0; return _system->GetSystemTheme(); }
		};
		class XStatusBarIcon : public Windows::IStatusBarIcon, IXWindowEventHandler
		{
			oref<XServerConnection> _con;
			oref<Windows::IWindowSystem> _system;
			oref<Cairo::CairoAPI> _cairo_api;
			IX11WindowSystem * _system_x11;
			Window _window;
			Colormap _colormap;
			XStandardAtoms * _atoms;
			Index2 _surface_size, _assigned_size;
			oref<Graphica::IDeviceContextFactory2D> _factory;
			oref<Graphica::IBitmap> _bitmap;
			Cairo::cairo_t _cairo;
			Cairo::cairo_surface_t _cairo_surface;
			Cairo::cairo_pattern_t _cairo_pattern;
			XRenderPictFormat * _format;
			Windows::IStatusCallback * _callback;
			Windows::StatusBarIconColorUsage _icon_image_color;
			oref<Picturae::Image> _icon_image;
			oref<Windows::IMenu> _icon_menu;
			string _tooltip;
			int _icon_event;
		private:
			void _render(void) noexcept
			{
				if (!_factory) _factory = Graphica::CreateDeviceContextFactory2D();
				if (!_factory) return;
				if (!_bitmap) {
					auto frame = _icon_image->FindBestSizeMatch(_surface_size.x, _surface_size.y);
					if (_icon_image_color == Windows::StatusBarIconColorUsage::Monochromic) {
						auto theme = _system->GetSystemTheme();
						uint8 V = (theme && theme->GetColorScheme() == Windows::ThemeColorScheme::Light) ? 0x00 : 0xFF;
						auto colored = frame->Convert(Picturae::PixelFormat::B8G8R8A8, Picturae::AlphaMode::Straight);
						for (uint y = 0; y < colored->GetDesc().height; y++) for (uint x = 0; x < colored->GetDesc().width; x++) {
							Color color = colored->GetPixel(x, y);
							color.r = color.g = color.b = V;
							colored->SetPixel(x, y, color);
						}
						_bitmap = _factory->LoadBitmap(colored);
					} else _bitmap = _factory->LoadBitmap(frame);
					_cairo_api = static_cast<Cairo::CairoBitmap *>(_bitmap.Inner())->GetAPI();
					if (_cairo_pattern) { _cairo_api->cairo_pattern_destroy(_cairo_pattern); _cairo_pattern = 0; }
					_cairo_pattern = _cairo_api->cairo_pattern_create_for_surface(static_cast<Cairo::CairoBitmap *>(_bitmap.Inner())->GetSurface());
					if (_cairo_api->cairo_pattern_status(_cairo_pattern) != CAIRO_STATUS_SUCCESS) {
						_cairo_api->cairo_pattern_destroy(_cairo_pattern);
						_cairo_pattern = 0;
					}
				}
				if (!_cairo_api) return;
				if (!_cairo_surface) {
					auto api = _con->GetAPI();
					auto display = _con->GetXDisplay();
					_cairo_surface = _cairo_api->cairo_xlib_surface_create_with_xrender_format(display, _window, api->XDefaultScreenOfDisplay(display), _format, _surface_size.x, _surface_size.y);
					_assigned_size = _surface_size;
					if (_cairo_api->cairo_surface_status(_cairo_surface) != CAIRO_STATUS_SUCCESS) {
						_cairo_api->cairo_surface_destroy(_cairo_surface);
						_cairo_surface = 0;
					}
				} else if (_assigned_size.x != _surface_size.x || _assigned_size.y != _surface_size.y) {
					_cairo_api->cairo_xlib_surface_set_size(_cairo_surface, _surface_size.x, _surface_size.y);
					_assigned_size = _surface_size;
				}
				if (!_cairo_surface) return;
				if (!_cairo) {
					_cairo = _cairo_api->cairo_create(_cairo_surface);
					if (_cairo_api->cairo_status(_cairo) != CAIRO_STATUS_SUCCESS) {
						_cairo_api->cairo_destroy(_cairo);
						_cairo = 0;
					}
				}
				if (!_cairo || !_cairo_pattern) return;
				_cairo_api->cairo_set_source_rgba(_cairo, 0.0, 0.0, 0.0, 0.005);
				_cairo_api->cairo_set_operator(_cairo, 1);
				_cairo_api->cairo_paint(_cairo);
				_cairo_api->cairo_set_operator(_cairo, 2);
				Cairo::cairo_matrix_t matrix;
				_cairo_api->cairo_matrix_init_scale(matrix, _bitmap->GetWidth() / double(_assigned_size.x), _bitmap->GetHeight() / double(_assigned_size.y));
				_cairo_api->cairo_pattern_set_matrix(_cairo_pattern, matrix);
				_cairo_api->cairo_set_source(_cairo, _cairo_pattern);
				_cairo_api->cairo_rectangle(_cairo, 0, 0, _assigned_size.x, _assigned_size.y);
				_cairo_api->cairo_fill(_cairo);
			}
			void _update_icon_window(void) noexcept
			{
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				try {
					ucs1_string tt = _tooltip;
					api->XChangeProperty(display, _window, _atoms->wm_name, _atoms->text_utf8, 8, PropModeReplace, tt.GetData(), tt.GetLength());
					api->XChangeProperty(display, _window, _atoms->net_wm_name, _atoms->text_utf8, 8, PropModeReplace, tt.GetData(), tt.GetLength());
					api->XChangeProperty(display, _window, _atoms->net_wm_visible_name, _atoms->text_utf8, 8, PropModeReplace, tt.GetData(), tt.GetLength());
				} catch (...) {}
				array<unsigned long> icon_data(0x10000);
				for (auto & f : *_icon_image) {
					auto fc = f.Convert(Picturae::PixelFormat::B8G8R8A8, Picturae::AlphaMode::Straight, Picturae::ScanOrigin::TopLeft);
					icon_data << fc->GetDesc().width;
					icon_data << fc->GetDesc().height;
					for (uint y = 0; y < fc->GetDesc().height; y++) for (uint x = 0; x < fc->GetDesc().width; x++) icon_data << fc->GetPixel(x, y);
				}
				api->XChangeProperty(display, _window, _atoms->net_wm_icon, _atoms->cardinal, 32, PropModeReplace, icon_data.GetBuffer(), icon_data.GetLength());
				_bitmap.Clear();
				_render();
			}
		public:
			XStatusBarIcon(Windows::IWindowSystem * system, IX11WindowSystem * system_x11) : _window(0), _colormap(0), _callback(0), _icon_image_color(Windows::StatusBarIconColorUsage::Regular), _icon_event(0)
			{
				_con = system_x11->GetConnection();
				_system = system; _system_x11 = system_x11;
				_atoms = reinterpret_cast<XStandardAtoms *>(_system_x11->GetStandardAtoms());
				_cairo = 0; _cairo_surface = 0; _cairo_pattern = 0; _format = 0;
			}
			virtual ~XStatusBarIcon(void) override { PresentIcon(false); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"X11 Status Bar Icon"; ESSE_TRY_OUTRO(string()) }
			virtual void HandleEvent(Window window, XEvent * event) noexcept override
			{
				if (event->type == Expose) {
					_render();
				} else if (event->type == ButtonRelease) {
					if (_icon_menu) {
						int event = _icon_menu->Perform(0, _system->GetCursorPosition());
						if (_callback && event) _callback->HandleStatusIconCommand(this, event);
					} else if (_icon_event) {
						if (_callback) _callback->HandleStatusIconCommand(this, _icon_event);
					}
				} else if (event->type == ConfigureNotify) {
					_surface_size.x = event->xconfigure.width;
					_surface_size.y = event->xconfigure.height;
					_bitmap.Clear();
					_render();
				}
			}
			virtual void HandleTimer(Window window, int timer) noexcept override {}
			virtual Windows::IStatusCallback * GetCallback(void) noexcept override { return _callback; }
			virtual void SetCallback(Windows::IStatusCallback * const & callback) noexcept override { _callback = callback; }
			virtual Index2 GetIconSize(void) noexcept override { auto scale = _system_x11->GetSystemScaleFactor(); return Index2(16 * scale, 16 * scale); }
			virtual Picturae::Image * GetIcon(void) noexcept override { return _icon_image; }
			virtual void SetIcon(Picturae::Image * const & image) noexcept override { _icon_image = image; if (_window) _update_icon_window(); }
			virtual Windows::StatusBarIconColorUsage GetIconColorUsage(void) noexcept override { return _icon_image_color; }
			virtual void SetIconColorUsage(const Windows::StatusBarIconColorUsage & color_usage) noexcept override { _icon_image_color = color_usage; if (_window) _update_icon_window(); }
			virtual string GetTooltip(void) noexcept override { try { return _tooltip; } catch (...) { return string(); } }
			virtual void SetTooltip(const string & text) noexcept override { try { _tooltip = text; } catch (...) { return; } if (_window) _update_icon_window(); }
			virtual int GetEventID(void) noexcept override { return _icon_event; }
			virtual void SetEventID(const int & id) noexcept override { _icon_event = id; }
			virtual Windows::IMenu * GetMenu(void) noexcept override { return _icon_menu; }
			virtual void SetMenu(Windows::IMenu * const & menu) noexcept override { _icon_menu = menu; }
			virtual bool PresentIcon(bool present) noexcept override
			{
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				if (present) {
					if (_window) return true;
					int u1, u2;
					if (!_system_x11->GetXRenderAPI()->XRenderQueryExtension(display, &u1, &u2)) return false;
					if (!_icon_image || !_icon_image->GetLength()) return false;
					_surface_size = GetIconSize();
					_assigned_size = Index2(0, 0);
					Window tray_window = 0;
					try {
						int screen_number = api->XDefaultScreen(display);
						auto tray_window_name = U"_NET_SYSTEM_TRAY_S" + string(screen_number);
						Atom tray_window_atom = api->XInternAtom(display, ucs1_string(tray_window_name), true);
						if (tray_window_atom == 0) return false;
						tray_window = api->XGetSelectionOwner(display, tray_window_atom);
						if (tray_window == 0) return false;
					} catch (...) { return 0; }
					Atom prop_tray_visual_type;
					void * pdata = 0;
					int format;
					unsigned long read, size;
					VisualID tray_visual_id;
					auto prop_tray_visual_status = api->XGetWindowProperty(display, tray_window, _atoms->net_system_tray_visual, 0, __LONG_MAX__, False, AnyPropertyType, &prop_tray_visual_type, &format, &read, &size, &pdata);
					if (prop_tray_visual_type && prop_tray_visual_status == Success) Memory::MemoryCopy(&tray_visual_id, pdata, sizeof(tray_visual_id));
					else tray_visual_id = 0;
					if (prop_tray_visual_status == Success) api->XFree(pdata);
					Visual * tray_visual = 0;
					int tray_depth = 0;
					XVisualInfo * visuals = 0;
					auto root = api->XRootWindow(display, api->XDefaultScreen(display));
					unsigned long use_mask = CWEventMask | CWSaveUnder | CWOverrideRedirect;
					XSetWindowAttributes attr;
					attr.override_redirect = false;
					attr.save_under = false;
					attr.event_mask = ButtonReleaseMask | ExposureMask | StructureNotifyMask;
					if (tray_visual_id) {
						XVisualInfo visual_info_base;
						visual_info_base.visualid = tray_visual_id;
						int count;
						visuals = api->XGetVisualInfo(display, VisualIDMask, &visual_info_base, &count);
						if (!count) return false;
						tray_visual = visuals[0].visual;
						tray_depth = visuals[0].depth;
						attr.colormap = _colormap = api->XCreateColormap(display, root, tray_visual, AllocNone);
						attr.background_pixel = attr.border_pixel = 0;
						use_mask |= CWColormap | CWBorderPixel | CWBackPixel;
					} else _colormap = 0;
					_window = api->XCreateWindow(display, root, 0, 0, _surface_size.x, _surface_size.y, 0, tray_depth, InputOutput, tray_visual, use_mask, &attr);
					if (!_window) {
						api->XFreeColormap(display, _colormap); _colormap = 0;
						if (visuals) api->XFree(visuals);
						return false;
					}
					if (!_system_x11->GetDispatch()->RegisterWindowHandler(_window, this)) {
						api->XDestroyWindow(display, _window); _window = 0;
						api->XFreeColormap(display, _colormap); _colormap = 0;
						if (visuals) api->XFree(visuals);
						return false;
					}
					if (!tray_visual) tray_visual = api->XDefaultVisual(display, api->XDefaultScreen(display));
					_format = _system_x11->GetXRenderAPI()->XRenderFindVisualFormat(display, tray_visual);
					_update_icon_window();
					unsigned long embed_data[2] = { 0, 1 };
					api->XChangeProperty(display, _window, _atoms->xembed_info, _atoms->xembed_info, 32, PropModeReplace, reinterpret_cast<unsigned char *>(&embed_data), 2);
					unsigned long wnd_pid = getpid();
					api->XChangeProperty(display, _window, _atoms->net_wm_pid, _atoms->cardinal, 32, PropModeReplace, reinterpret_cast<uint8 *>(&wnd_pid), 1);
					XEvent event;
					Memory::ZeroMemory(&event, sizeof(event));
					event.xclient.type = ClientMessage;
					event.xclient.window = tray_window;
					event.xclient.message_type = _atoms->net_system_tray_opcode;
					event.xclient.format = 32;
					event.xclient.data.l[0] = CurrentTime;
					event.xclient.data.l[1] = 0;
					event.xclient.data.l[2] = _window;
					event.xclient.data.l[3] = 0;
					event.xclient.data.l[4] = 0;
					api->XSendEvent(display, tray_window, False, NoEventMask, &event);
					api->XSync(display, False);
					if (visuals) api->XFree(visuals);
					return true;
				} else {
					if (!_window) return true;
					if (_cairo && _cairo_api) _cairo_api->cairo_destroy(_cairo);
					if (_cairo_surface && _cairo_api) _cairo_api->cairo_surface_destroy(_cairo_surface);
					if (_cairo_pattern && _cairo_api) _cairo_api->cairo_pattern_destroy(_cairo_pattern);
					_cairo = 0; _cairo_surface = 0; _cairo_pattern = 0;
					_system_x11->GetDispatch()->UnregisterWindowHandler(_window);
					api->XDestroyWindow(display, _window);
					if (_colormap) api->XFreeColormap(display, _colormap);
					_window = 0; _colormap = 0;
					_factory.Clear();
					_bitmap.Clear();
					return true;
				}
			}
			virtual bool IsVisible(void) noexcept override { return _window != 0; }
		};
		class XIPCClient : public Windows::IIPCClient, IXFileEventHandler
		{
			struct _request {
				uint64 serial;
				oref<XIPCClient> retain;
				oref<IDispatchTask> task;
				Windows::IPCStatus * status;
				oref<DataBlock> * result;
			};
		private:
			Windows::IPCStatus _status;
			oref<XDispatch> _dispatch;
			Windows::IWindowSystem * _system;
			int _socket;
			uint64 _serial;
			array<_request> _requests;
		private:
			void _cancel_requests(Windows::IPCStatus with_status) noexcept
			{
				Retain();
				_status = with_status;
				for (auto & r : _requests) {
					if (r.status) *r.status = with_status;
					if (r.result) r.result->Clear();
					if (r.task) r.task->DoTask(_system);
				}
				_requests.Clear();
				Release();
			}
		public:
			XIPCClient(Windows::IWindowSystem * system, const string & app, const string & auth) : _system(system), _serial(1), _requests(0x10)
			{
				_status = Windows::IPCStatus::Unknown;
				ErrorContext ectx; ErrorClear(ectx);
				auto x11_ws = reinterpret_cast<IX11WindowSystem *>(system->DynamicCast(Linux::Classes::X11_WindowSystem, ectx));
				ErrorThrow(ectx);
				_dispatch = x11_ws->GetDispatch();
				ucs1_string path = U"/tmp/eipc." + auth + U"." + app;
				_socket = socket(AF_UNIX, SOCK_STREAM, 0);
				if (_socket < 0) throw Exception();
				struct sockaddr_un addr;
				Memory::ZeroMemory(&addr, sizeof(addr));
				addr.sun_family = AF_UNIX;
				if (path.GetLength() >= sizeof(addr.sun_path)) { close(_socket); throw InvalidArgumentException(); }
				Memory::MemoryCopy(&addr.sun_path, path.GetData(), path.GetLength() + 1);
				while (true) {
					auto io = connect(_socket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
					if (io >= 0) break;
					else if (errno != EINTR) throw InputOutputException(Errores::SuberrorIO::Unknown);
				}
				if (!_dispatch->RegisterFileHandler(_socket, this)) { close(_socket); throw OutOfMemoryException(); }
			}
			virtual ~XIPCClient(void) override
			{
				_dispatch->UnregisterFileHandler(_socket);
				shutdown(_socket, SHUT_RDWR);
				close(_socket);
			}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"X11 IPC Client"; ESSE_TRY_OUTRO(string()) }
			virtual void HandleFile(int file) noexcept override
			{
				handle hfile = handle(intptr(file));
				uint64 resp_serial, resp_hdr;
				try {
					Windows::IPCStatus status;
					uintptr received0 = IO::ReadFile(hfile, &resp_serial, 8);
					if (received0 != 8) throw received0;
					uintptr received1 = IO::ReadFile(hfile, &resp_hdr, 8);
					if (received1 != 8) throw received1;
					if (resp_hdr & 0x8000000000000000UL) status = Windows::IPCStatus::Accepted; else status = Windows::IPCStatus::Discarded;
					uint32 data_size = resp_hdr & 0xFFFFFFFF;
					oref<DataBlock> data;
					if (data_size) {
						data = owrap(new DataBlock(1));
						data->SetLength(data_size);
						received0 = IO::ReadFile(hfile, data->GetBuffer(), data->GetLength());
						if (received0 != data->GetLength()) throw received0;
					}
					_status = status;
					uintptr idx = -1;
					for (uintptr i = 0; i < _requests.GetLength(); i++) if (_requests[i].serial == resp_serial) { idx = i; break; }
					if (idx >= 0) {
						auto & req = _requests[idx];
						if (req.status) *req.status = status;
						if (req.result) {
							if (!data && status == Windows::IPCStatus::Accepted) data = owrap(new DataBlock(1));
							*req.result = data;
						}
						if (req.task) req.task->DoTask(_system);
						Retain();
						_requests.Remove(idx);
						Release();
					}
				} catch (uintptr received) {
					_dispatch->UnregisterFileHandler(_socket);
					_cancel_requests(received ? Windows::IPCStatus::InternalError : Windows::IPCStatus::ServerClosed);
				} catch (...) {
					_dispatch->UnregisterFileHandler(_socket);
					_cancel_requests(Windows::IPCStatus::InternalError);
				}
			}
			virtual bool SendData(const string & verb, const void * data, uintptr length, IDispatchTask * on_responce, Windows::IPCStatus * result) noexcept override
			{
				if (_status == Windows::IPCStatus::InternalError || _status == Windows::IPCStatus::ServerClosed) return false;
				try {
					_request req;
					req.serial = _serial;
					req.retain.SetRetain(this);
					req.task.SetRetain(on_responce);
					req.status = result;
					req.result = 0;
					_requests << req;
					_serial++;
				} catch (...) { return false; }
				try {
					auto verb_block = EncodeString(verb, Unicode::Encoding::UTF8, true);
					if (verb_block->GetLength() > 0xFF || length > 0x7FFFFFFF) throw InvalidArgumentException();
					uint64 hdr = length | (uint64(verb_block->GetLength()) << 32);
					handle hfile = handle(intptr(_socket));
					IO::WriteFile(hfile, &_requests.LastElement().serial, 8);
					IO::WriteFile(hfile, &hdr, 8);
					IO::WriteFile(hfile, verb_block->GetBuffer(), verb_block->GetLength());
					if (length) IO::WriteFile(hfile, data, length);
				} catch (...) { _requests.RemoveLast(); return false; }
				return true;
			}
			virtual bool RequireData(const string & verb, IDispatchTask * on_responce, Windows::IPCStatus * result, oref<DataBlock> * data) noexcept override
			{
				if (_status == Windows::IPCStatus::InternalError || _status == Windows::IPCStatus::ServerClosed) return false;
				try {
					_request req;
					req.serial = _serial;
					req.retain.SetRetain(this);
					req.task.SetRetain(on_responce);
					req.status = result;
					req.result = data;
					_requests << req;
					_serial++;
				} catch (...) { return false; }
				try {
					auto verb_block = EncodeString(verb, Unicode::Encoding::UTF8, true);
					if (verb_block->GetLength() > 0xFF) throw InvalidArgumentException();
					uint64 hdr = 0x8000000000000000UL | (uint64(verb_block->GetLength()) << 32);
					handle hfile = handle(intptr(_socket));
					IO::WriteFile(hfile, &_requests.LastElement().serial, 8);
					IO::WriteFile(hfile, &hdr, 8);
					IO::WriteFile(hfile, verb_block->GetBuffer(), verb_block->GetLength());
				} catch (...) { _requests.RemoveLast(); return false; }
				return true;
			}
			virtual Windows::IPCStatus GetStatus(void) noexcept override { return _status; }
		};
		class XIPCSession : public Object, IXFileEventHandler
		{
			Windows::IWindowSystem * _system;
			oref<XDispatch> _dispatch;
			int _socket;
		public:
			XIPCSession(Windows::IWindowSystem * system, int file) : _system(system), _socket(file)
			{
				ErrorContext ectx; ErrorClear(ectx);
				auto x11_ws = reinterpret_cast<IX11WindowSystem *>(system->DynamicCast(Linux::Classes::X11_WindowSystem, ectx));
				ErrorThrow(ectx);
				_dispatch = x11_ws->GetDispatch();
				if (!_dispatch->RegisterFileHandler(_socket, this)) throw OutOfMemoryException();
			}
			virtual ~XIPCSession(void) override
			{
				_dispatch->UnregisterFileHandler(_socket);
				auto callback = _system->GetCallback();
				if (callback) callback->IPCClientDisconnect(handle(intptr(_socket)));
				close(_socket);
			}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"X11 IPC Session"; ESSE_TRY_OUTRO(string()) }
			virtual void HandleFile(int file) noexcept override
			{
				handle hfile = handle(intptr(file));
				try {
					uint64 serial, hdr;
					IO::ReadFile(hfile, &serial, 8);
					IO::ReadFile(hfile, &hdr, 8);
					uint32 verb_size = (hdr >> 32) & 0xFF;
					uint32 main_size = hdr & 0xFFFFFFFF;
					oref<DataBlock> verb, main;
					verb = owrap(new DataBlock(1)); main = owrap(new DataBlock(1));
					verb->SetLength(verb_size); main->SetLength(main_size);
					if (verb_size) {
						auto size = IO::ReadFile(hfile, verb->GetBuffer(), verb->GetLength());
						if (size != verb->GetLength()) throw InputOutputException(Errores::SuberrorIO::Unknown);
					}
					if (main_size) {
						auto size = IO::ReadFile(hfile, main->GetBuffer(), main->GetLength());
						if (size != main->GetLength()) throw InputOutputException(Errores::SuberrorIO::Unknown);
					}
					auto verb_str = string(verb->GetBuffer(), verb->GetLength(), Unicode::Encoding::UTF8);
					auto callback = _system->GetCallback();
					if (hdr & 0x8000000000000000UL) {
						oref<DataBlock> resp;
						if (callback) resp = callback->IPCSendData(hfile, verb_str);
						hdr = resp ? 0x8000000000000000UL : 0;
						if (resp) hdr |= resp->GetLength();
						IO::WriteFile(hfile, &serial, 8);
						IO::WriteFile(hfile, &hdr, 8);
						if (resp) IO::WriteFile(hfile, resp->GetBuffer(), resp->GetLength());
					} else {
						bool resp = false;
						if (callback) resp = callback->IPCReceiveData(hfile, verb_str, main->GetBuffer(), main->GetLength());
						hdr = resp ? 0x8000000000000000UL : 0;
						IO::WriteFile(hfile, &serial, 8);
						IO::WriteFile(hfile, &hdr, 8);
					}
				} catch (...) { shutdown(_socket, SHUT_RDWR); Release(); }
			}
		};
		class XWindowSystem : public Windows::IWindowSystem, Windows::IKeyboardManager, Windows::IClipboardManager, IX11WindowSystem, IXWindowEventHandler, IXFileEventHandler
		{
			struct _hotkey_record { uint id, vkc, vkm; };
			struct _xkb_status
			{
				bool present;
				int ver_minor, ver_major;
				int opcode, event_base, error_base;
				int numlock_mod, scrolllock_mod, alt_mod, system_mod;
			};
		private:
			oref<XLibAPI> _xlib_api;
			oref<XRANDRAPI> _xrandr_api;
			oref<XRenderAPI> _xrender_api;
			oref<XCursorAPI> _xcursor_api;
			oref<XServerConnection> _con;
			oref<XDispatch> _dispatch;
			oref<DBus::IConnection> _dbus;
			oref<Semaphore> _ibus_sync;
			oref<Thread> _signal_thread;
			handle _ibus_in, _ibus_out;
			int _ebus;
			ucs1_string _ebus_socket_name;
			Windows::IApplicationCallback * _callback;
			Window _service_window;
			_xkb_status _xkb;
			XStandardAtoms _atoms;
			array<_hotkey_record> _hotkeys;
			array<string> _file_list_to_open;
			Windows::ClipboardDataDesc _clipboard;
			ObjectDictionary<uint, XCursorImage> _cursors;
			Set<oref<Windows::IWindow>> _root_windows;
			oref<Picturae::Image> _appicon;
			bool _first_time_loop, _break_without_windows;
			uint32 _modal_level;
		private:
			static int _signal_handler(void *) noexcept
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
						if (number != SIGINT && number != SIGQUIT && number != SIGHUP && number != SIGTERM) continue;
						try {
							auto ws = Windows::GetWindowSystem();
							if (!ws) _exit(-1);
							ws->SubmitTask(CreateFunctionalTask([]() {
								auto callback = Windows::GetWindowSystem()->GetCallback();
								if (callback && callback->AcceptsApplicationCommand(Windows::ApplicationCommand::Terminate)) {
									callback->HandleApplicationCommand(Windows::ApplicationCommand::Terminate, string());
								} else _exit(-1);
							}));
						} catch (...) { _exit(-1); }
					} else if (status != EINTR) abort();
				}
				return 0;
			}
			bool _init_standard_atoms(void) noexcept
			{
				DEFINE_ATOM_LOAD(net_wm_pid, "_NET_WM_PID")
				DEFINE_ATOM_LOAD(cardinal, "CARDINAL")
				DEFINE_ATOM_LOAD(clipboard, "CLIPBOARD")
				DEFINE_ATOM_LOAD(incremental, "INCR")
				DEFINE_ATOM_LOAD(atom, "ATOM")
				DEFINE_ATOM_LOAD(targets, "TARGETS")
				DEFINE_ATOM_LOAD(text, "STRING")
				DEFINE_ATOM_LOAD(text_utf8, "UTF8_STRING")
				DEFINE_ATOM_LOAD(pixmap, "image/png")
				DEFINE_ATOM_LOAD(file_uri, "text/uri-list")
				DEFINE_ATOM_LOAD(data, "ESSE.Data")
				DEFINE_ATOM_LOAD(data_format, "ESSE.EfformatioDatorum")
				DEFINE_ATOM_LOAD(net_wm_state, "_NET_WM_STATE")
				DEFINE_ATOM_LOAD(net_wm_state_maximized_horz, "_NET_WM_STATE_MAXIMIZED_HORZ")
				DEFINE_ATOM_LOAD(net_wm_state_maximized_vert, "_NET_WM_STATE_MAXIMIZED_VERT")
				DEFINE_ATOM_LOAD(net_wm_state_fullscreen, "_NET_WM_STATE_FULLSCREEN")
				DEFINE_ATOM_LOAD(net_wm_state_hidden, "_NET_WM_STATE_HIDDEN")
				DEFINE_ATOM_LOAD(net_wm_state_shaded, "_NET_WM_STATE_SHADED")
				DEFINE_ATOM_LOAD(wm_normal_hints, "WM_NORMAL_HINTS")
				DEFINE_ATOM_LOAD(net_wm_state_demands_attention, "_NET_WM_STATE_DEMANDS_ATTENTION")
				DEFINE_ATOM_LOAD(net_frame_extents, "_NET_FRAME_EXTENTS")
				DEFINE_ATOM_LOAD(wm_delete_window, "WM_DELETE_WINDOW")
				DEFINE_ATOM_LOAD(net_wm_context_help, "_NET_WM_CONTEXT_HELP")
				DEFINE_ATOM_LOAD(net_request_frame_extents, "_NET_REQUEST_FRAME_EXTENTS")
				DEFINE_ATOM_LOAD(net_wm_icon, "_NET_WM_ICON")
				DEFINE_ATOM_LOAD(net_wm_window_type_combo, "_NET_WM_WINDOW_TYPE_COMBO")
				DEFINE_ATOM_LOAD(net_wm_window_type_utility, "_NET_WM_WINDOW_TYPE_UTILITY")
				DEFINE_ATOM_LOAD(net_wm_window_type_dialog, "_NET_WM_WINDOW_TYPE_DIALOG")
				DEFINE_ATOM_LOAD(net_wm_window_type_normal, "_NET_WM_WINDOW_TYPE_NORMAL")
				DEFINE_ATOM_LOAD(net_wm_window_type, "_NET_WM_WINDOW_TYPE")
				DEFINE_ATOM_LOAD(net_wm_state_modal, "_NET_WM_STATE_MODAL")
				DEFINE_ATOM_LOAD(net_wm_state_skip_taskbar, "_NET_WM_STATE_SKIP_TASKBAR")
				DEFINE_ATOM_LOAD(net_wm_state_skip_pager, "_NET_WM_STATE_SKIP_PAGER")
				DEFINE_ATOM_LOAD(wm_protocols, "WM_PROTOCOLS")
				DEFINE_ATOM_LOAD(wm_transient_for, "WM_TRANSIENT_FOR")
				DEFINE_ATOM_LOAD(window, "WINDOW")
				DEFINE_ATOM_LOAD(kde_net_wm_blur_behind_region, "_KDE_NET_WM_BLUR_BEHIND_REGION")
				DEFINE_ATOM_LOAD(net_wm_cm_s0, "_NET_WM_CM_S0")
				DEFINE_ATOM_LOAD(wm_name, "WM_NAME")
				DEFINE_ATOM_LOAD(net_wm_name, "_NET_WM_NAME")
				DEFINE_ATOM_LOAD(net_wm_visible_name, "_NET_WM_VISIBLE_NAME")
				DEFINE_ATOM_LOAD(net_active_window, "_NET_ACTIVE_WINDOW")
				DEFINE_ATOM_LOAD(net_wm_window_opacity, "_NET_WM_WINDOW_OPACITY")
				DEFINE_ATOM_LOAD(net_wm_state_above, "_NET_WM_STATE_ABOVE")
				DEFINE_ATOM_LOAD(net_wm_state_below, "_NET_WM_STATE_BELOW")
				DEFINE_ATOM_LOAD(xembed_info, "_XEMBED_INFO")
				DEFINE_ATOM_LOAD(net_system_tray_opcode, "_NET_SYSTEM_TRAY_OPCODE")
				DEFINE_ATOM_LOAD(net_system_tray_visual, "_NET_SYSTEM_TRAY_VISUAL")
				return true;
			}
			bool _xrandr_is_available(void) noexcept
			{
				if (!_xrandr_api) return false;
				int base_event, base_error, ver_major, ver_minor;
				return _xrandr_api->XRRQueryExtension(_con->GetXDisplay(), &base_event, &base_error) && _xrandr_api->XRRQueryVersion(_con->GetXDisplay(), &ver_major, &ver_minor);
			}
			double _get_system_scale_factor(void) noexcept
			{
				try {
					string rsrc = _xlib_api->XResourceManagerString(_con->GetXDisplay());
					auto records = SplitString(rsrc, U'\n');
					for (auto & r : records) {
						auto del = r.FindFirst(U":\t");
						if (del < 0) continue;
						if (r.Substring(0, del) == U"Xft.dpi") {
							auto dpi = r.Substring(del + 2, -1).ToUInt32();
							return double(dpi) / double(96.0);
						}
					}
					return 1.0;
				} catch (...) { return 1.0; }
			}
			Rectangle _get_user_rectangle(const Rectangle & rect) noexcept
			{
				auto atom = _xlib_api->XInternAtom(_con->GetXDisplay(), "_NET_WORKAREA", true);
				if (!atom) return rect;
				auto root = _xlib_api->XRootWindow(_con->GetXDisplay(), _xlib_api->XDefaultScreen(_con->GetXDisplay()));
				Atom effective_type;
				int effective_format;
				unsigned long num_elements, remainder;
				long * value;
				if (_xlib_api->XGetWindowProperty(_con->GetXDisplay(), root, atom, 0, __LONG_MAX__, false, AnyPropertyType, &effective_type, &effective_format, &num_elements, &remainder, reinterpret_cast<void **>(&value)) != Success) return rect;
				Rectangle result = rect;
				int max_area = -1;
				for (unsigned long i = 0; i + 3 < num_elements; i += 4) {
					Rectangle rect_test = Rectangle(value[i], value[i + 1], value[i] + value[i + 2], value[i + 1] + value[i + 3]);
					Rectangle intersect = Rectangle::Intersect(rect_test, rect);
					int area = (intersect.right - intersect.left) * (intersect.bottom - intersect.top);
					if (area > max_area) { max_area = area; result = intersect; }
				}
				_xlib_api->XFree(value);
				return result;
			}
			oref<Windows::IScreen> _create_unified_screen(void) noexcept
			{
				try {
					auto screen = _xlib_api->XDefaultScreenOfDisplay(_con->GetXDisplay());
					auto w = _xlib_api->XWidthOfScreen(screen);
					auto h = _xlib_api->XHeightOfScreen(screen);
					auto rect = Rectangle(0, 0, w, h);
					auto user = _get_user_rectangle(rect);
					return oref<Windows::IScreen>::CreateOwned(new XScreen(_con, U"", rect, user, _get_system_scale_factor()));
				} catch (...) { return 0; }
			}
			oref<Windows::IScreen> _create_screen_for_monitor(const XRRMonitorInfo & m) noexcept
			{
				auto name = _xlib_api->XGetAtomName(_con->GetXDisplay(), m.name);
				if (!name) return 0;
				auto rect = Rectangle(m.x, m.y, m.x + m.width, m.y + m.height);
				auto user = _get_user_rectangle(rect);
				oref<Windows::IScreen> result;
				try { result = oref<Windows::IScreen>::CreateOwned(new XScreen(_con, name, rect, user, _get_system_scale_factor())); } catch (...) {}
				_xlib_api->XFree(name);
				return result;
			}
			static string _uri_path_transform(const string & uri, bool decompose)
			{
				if (decompose) {
					dynamic_string_ucs1 result;
					for (uintptr i = 0; i < uri.GetLength(); i++) {
						if (uri[i] == U'%') {
							result.Append(unichar8(uri.Substring(i + 1, 2).ToUInt32(HexadecimalBase)));
							i += 2;
						} else result.Append(unichar8(uri[i]));
					}
					return result.ToString();
				} else {
					ucs1_string input = uri;
					dynamic_string_ucs4 result;
					for (uintptr i = 0; i < input.GetLength(); i++) {
						auto c = input[i];
						if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= 'z') || c == '-' || c == '.' || c == '_' || c == '~' || c == '/') result.Append(unichar32(c));
						else result << U'%' << string(uint(c), HexadecimalBase, 2);
					}
					return result;
				}
			}
			void _clipboard_respond_na(XSelectionRequestEvent * event) noexcept
			{
				XEvent resp;
				XSelectionEvent & sel = resp.xselection;
				Memory::ZeroMemory(&resp, sizeof(resp));
				sel.type = SelectionNotify;
				sel.display = _con->GetXDisplay();
				sel.requestor = event->requestor;
				sel.selection = event->selection;
				sel.target = event->target;
				sel.property = 0;
				sel.time = event->time;
				_xlib_api->XSendEvent(sel.display, event->requestor, false, 0, &resp);
				_xlib_api->XFlush(sel.display);
			}
			void _clipboard_respond_atoms(XSelectionRequestEvent * event, const array<Atom> & atoms) noexcept
			{
				if (!event->property) event->property = _atoms.clipboard;
				auto display = _con->GetXDisplay();
				_xlib_api->XChangeProperty(display, event->requestor, event->property, _atoms.atom, 32, PropModeReplace, atoms.GetBuffer(), atoms.GetLength());
				XEvent resp;
				XSelectionEvent & sel = resp.xselection;
				Memory::ZeroMemory(&resp, sizeof(resp));
				sel.type = SelectionNotify;
				sel.display = display;
				sel.requestor = event->requestor;
				sel.selection = event->selection;
				sel.target = event->target;
				sel.property = event->property;
				sel.time = event->time;
				_xlib_api->XSendEvent(display, event->requestor, false, 0, &resp);
				_xlib_api->XFlush(display);
			}
			void _clipboard_respond_data(XSelectionRequestEvent * event, const DataBlock & data) noexcept
			{
				if (!event->property) event->property = _atoms.clipboard;
				auto display = _con->GetXDisplay();
				LastXError = 0;
				_xlib_api->XChangeProperty(display, event->requestor, event->property, event->target, 8, PropModeReplace, data.GetBuffer(), data.GetLength());
				_xlib_api->XSync(display, false);
				if (LastXError == BadAlloc) {
					auto block_size = data.GetLength();
					_xlib_api->XChangeProperty(display, event->requestor, event->property, _atoms.incremental, 8, PropModeReplace, &block_size, 4);
					_xlib_api->XSync(display, false);
					_dispatch->DrainEvent(PropertyNotify);
					XSetWindowAttributes attr;
					attr.event_mask = PropertyChangeMask;
					_xlib_api->XChangeWindowAttributes(display, event->requestor, CWEventMask, &attr);
					XEvent event_send;
					XSelectionEvent & sel = event_send.xselection;
					Memory::ZeroMemory(&event, sizeof(event));
					sel.type = SelectionNotify;
					sel.display = display;
					sel.requestor = event->requestor;
					sel.selection = event->selection;
					sel.target = event->target;
					sel.property = event->property;
					sel.time = event->time;
					_xlib_api->XSendEvent(display, event->requestor, false, 0, &event_send);
					int blt_size = 0x100000;
					int current = 0;
					while (current < block_size) {
						while (true) {
							if (_dispatch->WaitForEvent(&event_send, PropertyNotify, 1000)) {
								if (event_send.xproperty.window == event->requestor && event_send.xproperty.state == PropertyDelete && event_send.xproperty.atom == event->property) break;
							} else return;
						}
						while (true) {
							int cblt = min<intptr>(blt_size, data.GetLength() - current);
							LastXError = 0;
							_xlib_api->XChangeProperty(display, event->requestor, event->property, event->target, 8, PropModeReplace, data.GetBuffer() + current, cblt);
							_xlib_api->XSync(display, false);
							if (LastXError == BadAlloc) {
								blt_size /= 2;
								if (!blt_size) return;
								continue;
							} else if (!LastXError) {
								current += cblt;
								break;
							} else return;
						}
					}
					while (true) {
						if (_dispatch->WaitForEvent(&event_send, PropertyNotify, 1000)) {
							if (event_send.xproperty.window == event->requestor && event_send.xproperty.state == PropertyDelete && event_send.xproperty.atom == event->property) break;
						} else return;
					}
					attr.event_mask = NoEventMask;
					_xlib_api->XChangeWindowAttributes(display, event->requestor, CWEventMask, &attr);
					_xlib_api->XChangeProperty(display, event->requestor, event->property, event->target, 8, PropModeReplace, 0, 0);
					_xlib_api->XFlush(display);
				} else if (LastXError) {
					_clipboard_respond_na(event);
				} else {
					XEvent resp;
					XSelectionEvent & sel = resp.xselection;
					Memory::ZeroMemory(&resp, sizeof(resp));
					sel.type = SelectionNotify;
					sel.display = display;
					sel.requestor = event->requestor;
					sel.selection = event->selection;
					sel.target = event->target;
					sel.property = event->property;
					sel.time = event->time;
					_xlib_api->XSendEvent(display, event->requestor, false, 0, &resp);
					_xlib_api->XFlush(display);
				}
			}
			void _clipboard_respond(XSelectionRequestEvent * event) noexcept
			{
				if (_clipboard.format_mask && event->selection == _atoms.clipboard) {
					try {
						if (event->target == _atoms.targets) {
							array<Atom> atoms(0x10);
							atoms << _atoms.targets;
							if (_clipboard.format_mask & Windows::ClipboardDataFormatText) atoms << _atoms.text_utf8 << _atoms.text;
							if (_clipboard.format_mask & Windows::ClipboardDataFormatImage) atoms << _atoms.pixmap;
							if (_clipboard.format_mask & Windows::ClipboardDataFormatFiles) atoms << _atoms.file_uri;
							if (_clipboard.format_mask & Windows::ClipboardDataFormatDataTypeless) atoms << _atoms.data;
							if (_clipboard.format_mask & Windows::ClipboardDataFormatDataFormat) atoms << _atoms.data_format;
							_clipboard_respond_atoms(event, atoms);
						} else if (event->target == _atoms.text || event->target == _atoms.text_utf8) {
							if (_clipboard.format_mask & Windows::ClipboardDataFormatText) {
								auto result = EncodeString(_clipboard.text, Unicode::Encoding::UTF8, false);
								_clipboard_respond_data(event, *result);
							} else _clipboard_respond_na(event);
						} else if (event->target == _atoms.pixmap) {
							if (_clipboard.format_mask & Windows::ClipboardDataFormatImage) {
								auto stream = MemoryStream::Create(0x10000);
								Picturae::Encode(stream, _clipboard.image, Picturae::ImageFormatPNG);
								_clipboard_respond_data(event, *stream->GetStorage());
							} else _clipboard_respond_na(event);
						} else if (event->target == _atoms.file_uri) {
							if (_clipboard.format_mask & Windows::ClipboardDataFormatFiles) {
								string uris;
								for (auto & f : *_clipboard.files) uris += U"file://" + _uri_path_transform(f, false) + U"\r\n";
								auto result = EncodeString(uris, Unicode::Encoding::UTF8, false);
								_clipboard_respond_data(event, *result);
							}
						} else if (event->target == _atoms.data) {
							if (_clipboard.format_mask & Windows::ClipboardDataFormatDataTypeless) {
								_clipboard_respond_data(event, *_clipboard.data);
							} else _clipboard_respond_na(event);
						} else if (event->target == _atoms.data_format) {
							if (_clipboard.format_mask & Windows::ClipboardDataFormatDataFormat) {
								auto result = EncodeString(_clipboard.data_format, Unicode::Encoding::UTF8, false);
								_clipboard_respond_data(event, *result);
							} else _clipboard_respond_na(event);
						} else _clipboard_respond_na(event);
					} catch (...) { _clipboard_respond_na(event); }
				} else _clipboard_respond_na(event);
			}
			Atom _clipboard_wait_for_responce(Atom propname) noexcept
			{
				while (true) {
					XEvent event;
					if (_dispatch->WaitForEvent(&event, PropertyNotify, 1000)) {
						if (event.xproperty.atom != propname || event.xproperty.state != PropertyNewValue) continue;
						return event.xproperty.atom;
					} else return 0;
				}
			}
			Atom _clipboard_send_request(Atom proptype) noexcept
			{
				XEvent event;
				_xlib_api->XConvertSelection(_con->GetXDisplay(), _atoms.clipboard, proptype, _atoms.clipboard, _service_window, CurrentTime);
				if (_dispatch->WaitForEvent(&event, SelectionNotify, 1000)) return event.xselection.property; else return 0;
			}
			oref<DataBlock> _clipboard_query_property(Atom proptype, bool bulk = false) noexcept
			{
				auto prop = bulk ? _clipboard_wait_for_responce(_atoms.clipboard) : _clipboard_send_request(proptype);
				if (prop) {
					void * pdata = 0;
					Atom act_type;
					int act_format;
					unsigned long read, size;
					if (!bulk) _dispatch->DrainEvent(PropertyNotify);
					_xlib_api->XGetWindowProperty(_con->GetXDisplay(), _service_window, prop, 0, __LONG_MAX__, true, AnyPropertyType, &act_type, &act_format, &read, &size, &pdata);
					if (act_type == _atoms.atom) {
						oref<DataBlock> data;
						try {
							data = owrap(new DataBlock(1));
							data->SetLength(read * sizeof(Atom));
							Memory::MemoryCopy(data->GetBuffer(), pdata, read * sizeof(Atom));
						} catch (...) {}
						_xlib_api->XFree(pdata);
						return data;
					} else if (act_type == proptype) {
						oref<DataBlock> data;
						try {
							data = owrap(new DataBlock(1));
							data->SetLength(read);
							Memory::MemoryCopy(data->GetBuffer(), pdata, read);
						} catch (...) {}
						_xlib_api->XFree(pdata);
						return data;
					} else if (!bulk && act_type == _atoms.incremental) {
						_xlib_api->XFree(pdata);
						oref<DataBlock> data;
						try { data = owrap(new DataBlock(1)); } catch (...) { return 0; }
						while (true) {
							auto block = _clipboard_query_property(proptype, true);
							if (!block) return 0;
							if (!block->GetLength()) break;
							try { data->Append(block); } catch (...) { return 0; }
						}
						return data;
					} else { _xlib_api->XFree(pdata); return 0; }
				} else return 0;
			}
		public:
			XWindowSystem(void) : _ebus(-1), _callback(0), _hotkeys(0x10), _file_list_to_open(1), _first_time_loop(true), _break_without_windows(false)
			{
				ErrorContext ectx;
				_xlib_api = owrap(new XLibAPI);
				_con = owrap(new XServerConnection(_xlib_api));
				_dispatch = owrap(new XDispatch(_con));
				try { _xrandr_api = owrap(new XRANDRAPI); } catch (...) {}
				try { _xrender_api = owrap(new XRenderAPI); } catch (...) {}
				try { _xcursor_api = owrap(new XCursorAPI); } catch (...) {}
				_ibus_sync = CreateSemaphore(1);
				if (!_ibus_sync) throw OutOfMemoryException();
				sigset_t set;
				if (sigemptyset(&set) < 0) throw InputOutputException(Errores::SuberrorIO::Unknown);
				if (sigaddset(&set, SIGINT) < 0 || signal(SIGINT, SIG_IGN)) throw InputOutputException(Errores::SuberrorIO::Unknown);
				if (sigaddset(&set, SIGQUIT) < 0 || signal(SIGQUIT, SIG_IGN)) throw InputOutputException(Errores::SuberrorIO::Unknown);
				if (sigaddset(&set, SIGHUP) < 0 || signal(SIGHUP, SIG_IGN)) throw InputOutputException(Errores::SuberrorIO::Unknown);
				if (sigaddset(&set, SIGTERM) < 0 || signal(SIGTERM, SIG_IGN)) throw InputOutputException(Errores::SuberrorIO::Unknown);
				if (sigprocmask(SIG_SETMASK, &set, 0) < 0) throw InputOutputException(Errores::SuberrorIO::Unknown);
				if (!_init_standard_atoms()) throw OutOfMemoryException();
				try {
					uint zero = 0;
					Picturae::PictureDesc desc;
					desc.width = desc.height = 1;
					desc.stride = sizeof(zero);
					desc.format = Picturae::PixelFormat::B8G8R8A8;
					desc.alpha_mode = Picturae::AlphaMode::Premultiplied;
					desc.origin = Picturae::ScanOrigin::TopLeft;
					desc.data = &zero;
					desc.palette = 0;
					desc.palette_size = 0;
					auto zero_image = owrap(new Picturae::Picture(desc, Picturae::PictureInit::Refer));
					_cursors.Append(uint(Windows::SystemCursorClass::Arrow), owrap(new XCursorImage(_con, XC_arrow)));
					_cursors.Append(uint(Windows::SystemCursorClass::Beam), owrap(new XCursorImage(_con, XC_xterm)));
					_cursors.Append(uint(Windows::SystemCursorClass::Link), owrap(new XCursorImage(_con, XC_hand1)));
					_cursors.Append(uint(Windows::SystemCursorClass::SizeLeftRight), owrap(new XCursorImage(_con, XC_sb_h_double_arrow)));
					_cursors.Append(uint(Windows::SystemCursorClass::SizeUpDown), owrap(new XCursorImage(_con, XC_sb_v_double_arrow)));
					_cursors.Append(uint(Windows::SystemCursorClass::SizeLeftUpRightDown), owrap(new XCursorImage(_con, _xcursor_api, "nwse-resize")));
					_cursors.Append(uint(Windows::SystemCursorClass::SizeLeftDownRightUp), owrap(new XCursorImage(_con, _xcursor_api, "nesw-resize")));
					_cursors.Append(uint(Windows::SystemCursorClass::SizeAll), owrap(new XCursorImage(_con, XC_fleur)));
					_cursors.Append(uint(Windows::SystemCursorClass::Null), owrap(new XCursorImage(_con, _xrender_api, zero_image)));
				} catch (...) {}
				try {
					const void * resdata;
					uintptr reslength;
					if (QueryResource(&resdata, &reslength, "1", "ICON")) _appicon = Picturae::DecodeImage(StaticMemoryStream::Create(resdata, reslength));
				} catch (...) {}
				ErrorClear(ectx);
				_dbus = DBus::IConnection::Connect(DBus::BusType::SessionBus, ectx);
				if (ErrorTest(ectx)) _dbus.Clear();
				_xkb.ver_major = XkbMajorVersion; _xkb.ver_minor = XkbMinorVersion;
				_xkb.numlock_mod = _xkb.scrolllock_mod = _xkb.alt_mod = _xkb.system_mod = 0;
				if (_xlib_api->XkbLibraryVersion(&_xkb.ver_major, &_xkb.ver_minor)) {
					if (_xlib_api->XkbQueryExtension(_con->GetXDisplay(), &_xkb.opcode, &_xkb.event_base, &_xkb.error_base, &_xkb.ver_major, &_xkb.ver_minor)) {
						auto keyboard_desc = _xlib_api->XkbGetMap(_con->GetXDisplay(), XkbAllComponentsMask, XkbUseCoreKbd);
						if (keyboard_desc) {
							if (keyboard_desc->names) for (int i = 0; i < 16; i++) {
								if (!keyboard_desc->names->vmods[i]) continue;
								auto mod = keyboard_desc->server->vmods[i];
								auto name = _xlib_api->XGetAtomName(_con->GetXDisplay(), keyboard_desc->names->vmods[i]);
								if (name) {
									if (strcmp(name, "NumLock") == 0) _xkb.numlock_mod = mod;
									else if (strcmp(name, "ScrollLock") == 0) _xkb.scrolllock_mod = mod;
									else if (strcmp(name, "Alt") == 0) _xkb.alt_mod = mod;
									else if (strcmp(name, "Meta") == 0) _xkb.system_mod = mod;
									_xlib_api->XFree(name);
								}
							}
							_xlib_api->XkbFreeKeyboard(keyboard_desc, 0, true);
						}
						_xkb.present = true;
					} else _xkb.present = false;
				} else _xkb.present = false;
				IO::CreatePipe(_ibus_in, _ibus_out);
				XSetWindowAttributes attr;
				attr.event_mask = PropertyChangeMask;
				_service_window = _xlib_api->XCreateWindow(_con->GetXDisplay(), _xlib_api->XRootWindow(_con->GetXDisplay(), _xlib_api->XDefaultScreen(_con->GetXDisplay())), 0, 0, 1, 1, 0, 0, InputOnly, 0, CWEventMask, &attr);
				if (!_service_window) { IO::CloseHandle(_ibus_in); IO::CloseHandle(_ibus_out); throw Exception(); }
				if (!_dispatch->RegisterFileHandler(int(intptr(_ibus_out)), this) || !_dispatch->RegisterWindowHandler(_service_window, this)) {
					_xlib_api->XDestroyWindow(_con->GetXDisplay(), _service_window);
					IO::CloseHandle(_ibus_in); IO::CloseHandle(_ibus_out); throw OutOfMemoryException();
				}
				_signal_thread = CreateThread(_signal_handler);
				if (!_signal_thread) {
					_dispatch->UnregisterFileHandler(int(intptr(_ibus_out))); _dispatch->UnregisterWindowHandler(_service_window);
					_xlib_api->XDestroyWindow(_con->GetXDisplay(), _service_window);
					IO::CloseHandle(_ibus_in); IO::CloseHandle(_ibus_out); throw OutOfMemoryException();
				}
				_clipboard.format_mask = 0;
				unsigned long wnd_pid = getpid();
				auto display = _con->GetXDisplay();
				_xlib_api->XChangeProperty(display, _service_window, _atoms.net_wm_pid, _atoms.cardinal, 32, PropModeReplace, reinterpret_cast<uint8 *>(&wnd_pid), 1);
				_modal_level = 0;
			}
			virtual ~XWindowSystem(void) override
			{
				_xlib_api->XDestroyWindow(_con->GetXDisplay(), _service_window);
				IO::CloseHandle(_ibus_in); IO::CloseHandle(_ibus_out);
				if (_ebus >= 0) { close(_ebus); unlink(_ebus_socket_name); }
			}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"X11 Window System"; ESSE_TRY_OUTRO(string()) }
			virtual XDispatch * GetDispatch(void) noexcept override { return _dispatch; }
			virtual XServerConnection * GetConnection(void) noexcept override { return _con; }
			virtual XRenderAPI * GetXRenderAPI(void) noexcept override { return _xrender_api; }
			virtual DBus::IConnection * GetDBus(void) noexcept override { return _dbus; }
			virtual Picturae::Image * GetApplicationIcon(void) noexcept override { return _appicon; }
			virtual Window GetServiceWindow(void) noexcept override { return _service_window; }
			virtual void * GetStandardAtoms(void) noexcept override { return &_atoms; }
			virtual void SubmitTaskE(IDispatchTask * task, ErrorContext & ectx) noexcept override { SubmitTasksE(&task, 1, ectx); }
			virtual void SubmitTasksE(IDispatchTask ** tasks, uintptr count, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!count) return;
					_ibus_sync->Wait();
					for (uintptr i = 0; i < count; i++) {
						auto task = tasks[i];
						if (!task) continue;
						task->Retain();
						try { IO::WriteFile(_ibus_in, &task, sizeof(task)); }
						catch (...) { _ibus_sync->Open(); task->Release(); throw; }
					}
					_ibus_sync->Open();
				ESSE_TRY_OUTRO()
			}
			virtual void HandleEvent(Window window, XEvent * event) noexcept override
			{
				if (event->type == SelectionClear) {
					_clipboard.format_mask = 0;
					_clipboard.text = string();
					_clipboard.data_format = string();
					_clipboard.image.Clear();
					_clipboard.files.Clear();
					_clipboard.data.Clear();
				} else if (event->type == SelectionRequest) {
					_clipboard_respond(&event->xselectionrequest);
				} else if (event->type == KeyPress) {
					if (_callback) for (auto & hk : _hotkeys) if (hk.vkc == event->xkey.keycode && hk.vkm == event->xkey.state) {
						_callback->HandleHotKeyEvent(hk.id);
					}
				}
			}
			virtual void HandleTimer(Window window, int timer) noexcept override {}
			virtual void HandleFile(int file) noexcept override
			{
				if (file == int(intptr(_ibus_out))) {
					IDispatchTask * task;
					try { if (IO::ReadFile(_ibus_out, &task, sizeof(task)) != sizeof(task)) throw Exception(); } catch (...) { task = 0; }
					if (task) { task->DoTask(this); task->Release(); }
				} else if (_ebus >= 0 && file == _ebus) {
					int client = accept(_ebus, 0, 0);
					if (client >= 0) {
						try {
							auto session = owrap(new XIPCSession(this, client));
							session->Retain();
						} catch (...) { close(client); }
					} else if (errno != EINTR) {
						_dispatch->UnregisterFileHandler(_ebus);
						close(_ebus); unlink(_ebus_socket_name);
						_ebus = -1; _ebus_socket_name = ucs1_string();
					}
				}
			}
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept override
			{
				if (cls == Classes.Object || cls == Classes.DynamicObject || cls == Classes.IWindowSystem) {
					Retain(); return this;
				} else if (cls == Linux::Classes::X11_WindowSystem) {
					return static_cast<IX11WindowSystem *>(this);
				} else { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			}
			virtual double GetSystemScaleFactor(void) noexcept override { return _get_system_scale_factor(); }
			virtual oref<object_array<Windows::IScreen>> EnumerateScreens(void) noexcept override
			{
				try {
					auto result = owrap(new object_array<Windows::IScreen>(0x10));
					if (_xrandr_is_available()) {
						int num_monitors;
						auto root = _xlib_api->XRootWindow(_con->GetXDisplay(), _xlib_api->XDefaultScreen(_con->GetXDisplay()));
						auto monitors = _xrandr_api->XRRGetMonitors(_con->GetXDisplay(), root, 1, &num_monitors);
						if (monitors) {
							for (int i = 0; i < num_monitors; i++) {
								auto & m = monitors[i];
								auto screen = _create_screen_for_monitor(m);
								if (screen) result->Append(screen);
							}
							_xrandr_api->XRRFreeMonitors(monitors);
						}
					} else {
						auto unified = _create_unified_screen();
						if (!unified) return 0;
						result->Append(unified);
					}
					return result;
				} catch (...) { return 0; }
			}
			virtual oref<Windows::IScreen> GetDefaultScreen(void) noexcept override
			{
				if (!_xrandr_is_available()) return _create_unified_screen();
				int num_monitors;
				auto root = _xlib_api->XRootWindow(_con->GetXDisplay(), _xlib_api->XDefaultScreen(_con->GetXDisplay()));
				auto monitors = _xrandr_api->XRRGetMonitors(_con->GetXDisplay(), root, 1, &num_monitors);
				if (monitors) {
					if (!num_monitors) { _xrandr_api->XRRFreeMonitors(monitors); return 0; }
					int selected = 0;
					for (int i = 0; i < num_monitors; i++) {
						auto & m = monitors[i];
						if (m.primary) { selected = i; break; }
					}
					auto result = _create_screen_for_monitor(monitors[selected]);
					_xrandr_api->XRRFreeMonitors(monitors);
					return result;
				} else return 0;
			}
			virtual oref<Windows::IScreen> GetScreenWithBestCoverage(const Rectangle & rect) noexcept override
			{
				if (!_xrandr_is_available()) return _create_unified_screen();
				int num_monitors;
				auto root = _xlib_api->XRootWindow(_con->GetXDisplay(), _xlib_api->XDefaultScreen(_con->GetXDisplay()));
				auto monitors = _xrandr_api->XRRGetMonitors(_con->GetXDisplay(), root, 1, &num_monitors);
				if (monitors) {
					if (!num_monitors) { _xrandr_api->XRRFreeMonitors(monitors); return 0; }
					int selected = 0;
					int max_area = 0;
					for (int i = 0; i < num_monitors; i++) {
						auto & m = monitors[i];
						auto screen = Rectangle(m.x, m.y, m.x + m.width, m.y + m.height);
						auto clip = Rectangle::Intersect(screen, rect);
						int area = (clip.right - clip.left) * (clip.bottom - clip.top);
						if (area > max_area) { max_area = area; selected = i; }
					}
					auto result = _create_screen_for_monitor(monitors[selected]);
					_xrandr_api->XRRFreeMonitors(monitors);
					return result;
				} else return 0;
			}
			virtual oref<Windows::ITheme> GetSystemTheme(void) noexcept override
			{
				auto x_color_scheme = Windows::ThemeColorScheme::Light;
				auto x_accent_color = Color(0x36, 0x47, 0xFF);
				DBus::Variant color_scheme;
				array<DBus::Variant> accent_color(3);
				if (!_dbus->BeginInvocation("org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Settings", "ReadOne")) goto no_system_theme;
				if (!_dbus->AddStringArgument(U"org.freedesktop.appearance")) goto no_system_theme;
				if (!_dbus->AddStringArgument(U"color-scheme")) goto no_system_theme;
				if (!_dbus->EndInvocationVariant(color_scheme)) goto no_system_theme;
				if (!_dbus->BeginInvocation("org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Settings", "ReadOne")) goto no_system_theme;
				if (!_dbus->AddStringArgument(U"org.freedesktop.appearance")) goto no_system_theme;
				if (!_dbus->AddStringArgument(U"accent-color")) goto no_system_theme;
				if (!_dbus->EndInvocationVariantArray(accent_color)) goto no_system_theme;
				if (color_scheme.type == 'u') {
					if (color_scheme.ui32 == 1) x_color_scheme = Windows::ThemeColorScheme::Dark;
					else if (color_scheme.ui32 == 2) x_color_scheme = Windows::ThemeColorScheme::Light;
				}
				if (accent_color.GetLength() == 3 && accent_color[0].type == 'd' && accent_color[1].type == 'd' && accent_color[2].type == 'd') {
					auto r = accent_color[0].d;
					auto g = accent_color[1].d;
					auto b = accent_color[2].d;
					if (r >= 0.0 && r <= 1.0 && g >= 0.0 && g <= 1.0 && b >= 0.0 && b <= 1.0) x_accent_color = Color(r, g, b);
				}
			no_system_theme:
				try { return oref<Windows::ITheme>::CreateOwned(new XTheme(x_color_scheme, x_accent_color)); } catch (...) { return 0; }
			}
			virtual IKeyboardManager * GetKeyboardManager(void) noexcept override { return this; }
			virtual bool IsKeyPressed(uint vkc) noexcept override
			{
				if (vkc == VirtualKeyCodes::Control) return IsKeyPressed(VirtualKeyCodes::LeftControl) || IsKeyPressed(VirtualKeyCodes::RightControl);
				else if (vkc == VirtualKeyCodes::Shift) return IsKeyPressed(VirtualKeyCodes::LeftShift) || IsKeyPressed(VirtualKeyCodes::RightShift);
				else if (vkc == VirtualKeyCodes::Alternative) return IsKeyPressed(VirtualKeyCodes::LeftAlternative) || IsKeyPressed(VirtualKeyCodes::RightAlternative);
				else if (vkc == VirtualKeyCodes::System) return IsKeyPressed(VirtualKeyCodes::LeftSystem) || IsKeyPressed(VirtualKeyCodes::RightSystem); else {
					uint ktr = XKeyCodeFromESSE(vkc);
					char status[32];
					_xlib_api->XQueryKeymap(_con->GetXDisplay(), status);
					return (status[ktr >> 3] >> (ktr & 7)) & 1;
				}
			}
			virtual bool IsKeyToggled(uint vkc) noexcept override
			{
				if (_xkb.present) {
					XkbStateRec state;
					_xlib_api->XkbGetState(_con->GetXDisplay(), XkbUseCoreKbd, &state);
					if (vkc == VirtualKeyCodes::CapsLock) return (state.mods & LockMask) != 0;
					else if (vkc == VirtualKeyCodes::NumLock) return (state.mods & _xkb.numlock_mod) != 0;
					else if (vkc == VirtualKeyCodes::ScrollLock) return (state.mods & _xkb.scrolllock_mod) != 0;
					else return false;
				} else return false;
			}
			virtual bool RegisterHotKey(int event_id, uint vkc, uint vkm) noexcept override
			{
				auto root = _xlib_api->XRootWindow(_con->GetXDisplay(), _xlib_api->XDefaultScreen(_con->GetXDisplay()));
				try {
					_hotkey_record rec;
					rec.vkc = XKeyCodeFromESSE(vkc);
					rec.vkm = 0;
					rec.id = event_id;
					if (vkm & VirtualKeyModifiers::Shift) rec.vkm |= ShiftMask;
					if (vkm & VirtualKeyModifiers::Control) rec.vkm |= ControlMask;
					if (vkm & VirtualKeyModifiers::Alternative) rec.vkm |= _xkb.alt_mod;
					if (vkm & VirtualKeyModifiers::System) rec.vkm |= _xkb.system_mod;
					_hotkeys << rec;
				} catch (...) { return false; }
				if (_hotkeys.GetLength() == 1) {
					XSetWindowAttributes attr;
					attr.event_mask = KeyPress;
					_xlib_api->XChangeWindowAttributes(_con->GetXDisplay(), root, CWEventMask, &attr);
					_dispatch->RegisterWindowHandler(root, this);
				}
				LastXError = 0;
				_xlib_api->XGrabKey(_con->GetXDisplay(), _hotkeys.LastElement().vkc, _hotkeys.LastElement().vkm, root, true, GrabModeAsync, GrabModeAsync);
				_xlib_api->XSync(_con->GetXDisplay(), false);
				if (LastXError) { UnregisterHotKey(event_id); return false; }
				return true;
			}
			virtual void UnregisterHotKey(int event_id) noexcept override
			{
				if (!_hotkeys.GetLength()) return;
				auto root = _xlib_api->XRootWindow(_con->GetXDisplay(), _xlib_api->XDefaultScreen(_con->GetXDisplay()));
				for (uintptr i = 0; i < _hotkeys.GetLength(); i++) if (_hotkeys[i].id == event_id) {
					_xlib_api->XUngrabKey(_con->GetXDisplay(), _hotkeys[i].vkc, _hotkeys[i].vkm, root);
					_xlib_api->XSync(_con->GetXDisplay(), false);
					_hotkeys.Remove(i);
					break;
				}
				if (!_hotkeys.GetLength()) {
					_dispatch->UnregisterWindowHandler(root);
					XSetWindowAttributes attr;
					attr.event_mask = 0;
					_xlib_api->XChangeWindowAttributes(_con->GetXDisplay(), root, CWEventMask, &attr);
				}
			}
			virtual uint GetKeyboardDelay(void) noexcept override
			{
				if (_xkb.present) {
					unsigned int primary, period;
					_xlib_api->XkbGetAutoRepeatRate(_con->GetXDisplay(), XkbUseCoreKbd, &primary, &period);
					return primary;
				} else return 1000;
			}
			virtual uint GetKeyboardSpeed(void) noexcept override
			{
				if (_xkb.present) {
					unsigned int primary, period;
					_xlib_api->XkbGetAutoRepeatRate(_con->GetXDisplay(), XkbUseCoreKbd, &primary, &period);
					return period;
				} else return 100;
			}
			virtual uint GetAlternativeKeyMod(void) noexcept override { return _xkb.present ? _xkb.alt_mod : 0; }
			virtual uint GetSystemKeyMod(void) noexcept override { return _xkb.present ? _xkb.system_mod : 0; }
			virtual IClipboardManager * GetClipboardManager(void) noexcept override { return this; }
			virtual uint ProbeClipboardFormats(uint format_mask) noexcept override
			{
				auto owner = _xlib_api->XGetSelectionOwner(_con->GetXDisplay(), _atoms.clipboard);
				if (!owner) return 0;
				if (owner == _service_window) {
					return _clipboard.format_mask;
				} else {
					auto targets = _clipboard_query_property(_atoms.targets);
					if (!targets) return 0;
					intptr num_atoms = targets->GetLength() / sizeof(Atom);
					auto atoms = reinterpret_cast<Atom *>(targets->GetBuffer());
					uint result = 0;
					for (intptr i = 0; i < num_atoms; i++) {
						if (atoms[i] == _atoms.text || atoms[i] == _atoms.text_utf8) result |= Windows::ClipboardDataFormatText & format_mask;
						if (atoms[i] == _atoms.pixmap) result |= Windows::ClipboardDataFormatImage & format_mask;
						if (atoms[i] == _atoms.file_uri) result |= Windows::ClipboardDataFormatFiles & format_mask;
						if (atoms[i] == _atoms.data) result |= Windows::ClipboardDataFormatDataTypeless & format_mask;
						if (atoms[i] == _atoms.data_format) result |= Windows::ClipboardDataFormatDataFormat & format_mask;
					}
					return result;
				}
			}
			virtual bool ReadClipboard(uint format_mask, Windows::ClipboardDataDesc & dest) noexcept override
			{
				auto owner = _xlib_api->XGetSelectionOwner(_con->GetXDisplay(), _atoms.clipboard);
				if (!owner) return false;
				dest.format_mask = 0;
				if (owner == _service_window) {
					try {
						if (_clipboard.format_mask & format_mask & Windows::ClipboardDataFormatText) {
							dest.text = _clipboard.text;
							dest.format_mask |= Windows::ClipboardDataFormatText;
						}
						if (_clipboard.format_mask & format_mask & Windows::ClipboardDataFormatImage) {
							dest.image = _clipboard.image ? owrap(new Picturae::Picture(_clipboard.image)) : oref<Picturae::Picture>();
							dest.format_mask |= Windows::ClipboardDataFormatImage;
						}
						if (_clipboard.format_mask & format_mask & Windows::ClipboardDataFormatFiles) {
							dest.files = _clipboard.files ? owrap(new array<string>(*_clipboard.files)) : oref<array<string>>();
							dest.format_mask |= Windows::ClipboardDataFormatFiles;
						}
						if (_clipboard.format_mask & format_mask & Windows::ClipboardDataFormatDataTypeless) {
							dest.data = _clipboard.data ? owrap(new DataBlock(*_clipboard.data)) : oref<DataBlock>();
							dest.format_mask |= Windows::ClipboardDataFormatDataTypeless;
						}
						if (_clipboard.format_mask & format_mask & Windows::ClipboardDataFormatDataFormat) {
							dest.data_format = _clipboard.data_format;
							dest.format_mask |= Windows::ClipboardDataFormatDataFormat;
						}
					} catch (...) { return false; }
				} else {
					try {
						if (format_mask & Windows::ClipboardDataFormatText) {
							auto data = _clipboard_query_property(_atoms.text_utf8);
							if (!data) data = _clipboard_query_property(_atoms.text);
							if (data) {
								dest.text = string(data->GetBuffer(), data->GetLength(), Unicode::Encoding::UTF8);
								dest.format_mask |= Windows::ClipboardDataFormatText;
							}
						}
						if (format_mask & Windows::ClipboardDataFormatImage) {
							auto data = _clipboard_query_property(_atoms.pixmap);
							if (data) {
								dest.image = Picturae::DecodePicture(StaticMemoryStream::Create(data->GetBuffer(), data->GetLength()));
								dest.format_mask |= Windows::ClipboardDataFormatImage;
							}
						}
						if (format_mask & Windows::ClipboardDataFormatFiles) {
							auto data = _clipboard_query_property(_atoms.file_uri);
							if (data) {
								auto uri = SplitString(string(data->GetBuffer(), data->GetLength(), Unicode::Encoding::UTF8).Replace(U'\r', string()), U'\n');
								dest.files = owrap(new array<string>(uri.GetLength()));
								for (auto & u : uri) if (u.Substring(0, 7) == U"file://") dest.files->Append(_uri_path_transform(u.Substring(7, -1), true));
								dest.format_mask |= Windows::ClipboardDataFormatFiles;
							}
						}
						if (format_mask & Windows::ClipboardDataFormatDataTypeless) {
							auto data = _clipboard_query_property(_atoms.data);
							if (data) {
								dest.data = data;
								dest.format_mask |= Windows::ClipboardDataFormatDataTypeless;
							}
						}
						if (format_mask & Windows::ClipboardDataFormatDataFormat) {
							auto data = _clipboard_query_property(_atoms.data_format);
							if (data) {
								dest.data_format = string(data->GetBuffer(), data->GetLength(), Unicode::Encoding::UTF8);
								dest.format_mask |= Windows::ClipboardDataFormatDataFormat;
							}
						}
					} catch (...) { return false; }
				}
				return dest.format_mask != 0;
			}
			virtual bool WriteClipboard(const Windows::ClipboardDataDesc & dest) noexcept override
			{
				if (!dest.format_mask) return false;
				auto previously_set = _clipboard.format_mask != 0;
				uint mask_written = 0;
				try {
					if (dest.format_mask & Windows::ClipboardDataFormatText) {
						_clipboard.text = dest.text;
						mask_written |= Windows::ClipboardDataFormatText;
					} else _clipboard.text = string();
					if ((dest.format_mask & Windows::ClipboardDataFormatImage) && dest.image) {
						_clipboard.image = owrap(new Picturae::Picture(dest.image));
						mask_written |= Windows::ClipboardDataFormatImage;
					} else _clipboard.image.Clear();
					if ((dest.format_mask & Windows::ClipboardDataFormatFiles) && dest.files) {
						_clipboard.files = owrap(new array<string>(*dest.files));
						mask_written |= Windows::ClipboardDataFormatFiles;
					} else _clipboard.files.Clear();
					if (((dest.format_mask & Windows::ClipboardDataFormatData) == Windows::ClipboardDataFormatData) && dest.data) {
						_clipboard.data_format = dest.data_format;
						_clipboard.data = owrap(new DataBlock(*dest.data));
						mask_written |= Windows::ClipboardDataFormatData;
					} else {
						_clipboard.data_format = string();
						_clipboard.data.Clear();
					}
				} catch (...) { return false; }
				if (!previously_set && mask_written) _xlib_api->XSetSelectionOwner(_con->GetXDisplay(), _atoms.clipboard, _service_window, CurrentTime);
				return mask_written != 0;
			}
			virtual Index2 GetCursorPosition(void) noexcept override
			{
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				auto root = api->XRootWindow(display, api->XDefaultScreen(display));
				Window __root, __child;
				int root_x, root_y, child_x, child_y;
				uint mask;
				if (api->XQueryPointer(display, root, &__root, &__child, &root_x, &root_y, &child_x, &child_y, &mask)) return Index2(root_x, root_y);
				else return Index2(0, 0);
			}
			virtual void SetCursorPosition(const Index2 & position) noexcept override
			{
				auto api = _con->GetAPI();
				auto display = _con->GetXDisplay();
				auto root = api->XRootWindow(display, api->XDefaultScreen(display));
				api->XWarpPointer(display, 0, root, 0, 0, 0, 0, position.x, position.y);
			}
			virtual oref<Windows::ICursor> LoadCursor(Picturae::Picture * source) noexcept override
			{
				if (!source) return 0;
				return oref<Windows::ICursor>::CreateOwned(new XCursorImage(_con, _xrender_api, source));
			}
			virtual oref<Windows::ICursor> GetSystemCursor(Windows::SystemCursorClass cursor) noexcept override { return _cursors[uint(cursor)]; }
			virtual oref<array<Index2>> GetApplicationIconSizes(void) noexcept override
			{
				try {
					auto result = owrap(new array<Index2>(6));
					result->Append(Index2(128, 128));
					result->Append(Index2(64, 64));
					result->Append(Index2(48, 48));
					result->Append(Index2(32, 32));
					result->Append(Index2(24, 24));
					result->Append(Index2(16, 16));
					return result;
				} catch (...) { return 0; }
			}
			virtual void SetApplicationIcon(Picturae::Image * icon) noexcept override { _appicon = icon; }
			virtual void SetApplicationBadge(const string & text) noexcept override {}
			virtual void SetApplicationIconVisibility(bool visible) noexcept override {}
			virtual void Beep(void) noexcept override
			{
				_con->GetAPI()->XkbBell(_con->GetXDisplay(), _service_window, 100, 0);
				_con->GetAPI()->XFlush(_con->GetXDisplay());
			}
			virtual oref<array<Windows::IWindow *>> EnumerateTopLevelWindows(void) noexcept override
			{
				try {
					auto result = owrap(new array<Windows::IWindow *>(0x20));
					for (auto & w : _root_windows) result->Append(w);
					return result;
				} catch (...) { return 0; }
			}
			virtual Windows::IWindow * CreateWindow(const void * desc) noexcept override { try { return owrap(new XWindow(this, desc)); } catch (...) { return 0; } }
			virtual Rectangle ConvertClientToWindow(const Rectangle & rect, uint style, uint wstyle, uint cstyle) noexcept override { return rect; }
			virtual Index2 ConvertClientToWindow(const Index2 & size, uint style, uint wstyle, uint cstyle) noexcept override { return size; }
			virtual bool RegisterTopLevelWindow(Windows::IWindow * window) noexcept override { try { _root_windows.AddElement(window); return true; } catch (...) { return false; } }
			virtual bool UnregisterTopLevelWindow(Windows::IWindow * window) noexcept override { _root_windows.RemoveElement(window); if (_root_windows.IsEmpty() && _break_without_windows) ExitMainLoop(); return true; }
			virtual uint GetModalityLevel(void) noexcept override { return _modal_level; }
			virtual void SetModalityLevel(uint level) noexcept override { _modal_level = level; }
			virtual void ScheduleFilesToBeOpened(const string * files, int num_files) noexcept override { try { _file_list_to_open.Append(files, num_files); } catch (...) {} }
			virtual Windows::IApplicationCallback * GetCallback(void) noexcept override { return _callback; }
			virtual void SetCallback(Windows::IApplicationCallback * callback) noexcept override { _callback = callback; }
			virtual void RunMainLoop(bool while_there_are_windows) noexcept override
			{
				_break_without_windows = while_there_are_windows;
				if (_first_time_loop || _file_list_to_open.GetLength()) {
					bool opened = false, ftl = _first_time_loop;
					if (_callback && _callback->AcceptsApplicationCommand(Windows::ApplicationCommand::OpenSpecificFile)) {
						for (auto & file : _file_list_to_open) try { if (_callback->HandleApplicationCommand(Windows::ApplicationCommand::OpenSpecificFile, file)) opened = true; } catch (...) {}
					}
					_file_list_to_open.Clear();
					_first_time_loop = false;
					if (!opened && ftl && _callback && _callback->AcceptsApplicationCommand(Windows::ApplicationCommand::CreateFile)) _callback->HandleApplicationCommand(Windows::ApplicationCommand::CreateFile, string());
				}
				_dispatch->RunEventLoop();
			}
			virtual void ExitMainLoop(void) noexcept override { _dispatch->BreakEventLoop(); }
			virtual bool OpenFileDialog(Windows::OpenFileDialogDesc * desc, Windows::IWindow * parent, IDispatchTask * on_responce) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool SaveFileDialog(Windows::SaveFileDialogDesc * desc, Windows::IWindow * parent, IDispatchTask * on_responce) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool ChooseDirectoryDialog(Windows::ChooseDirectoryDialogDesc * desc, Windows::IWindow * parent, IDispatchTask * on_responce) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool AlertDialog(Windows::AlertDialogDesc * desc, Windows::IWindow * parent, IDispatchTask * on_responce) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual oref<Windows::IMenu> CreateMenu(void) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual oref<Windows::IMenuItem> CreateMenuItem(void) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual Index2 GetUserNotificationIconSize(void) noexcept override { double scale = _get_system_scale_factor(); return Index2(32 * scale, 32 * scale); }
			virtual void PushUserNotification(const string & title, const string & text, Picturae::Image * icon) noexcept override
			{
				if (!_dbus) return;
				auto size = GetUserNotificationIconSize();
				array<string> args(1);
				if (!_dbus->BeginInvocation("/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify")) return;
				if (!_dbus->AddStringArgument(title)) return;
				if (!_dbus->AddUInt32Argument(0)) return;
				if (!_dbus->AddStringArgument(U"")) return;
				if (!_dbus->AddStringArgument(U"")) return;
				if (!_dbus->AddStringArgument(text.Replace(U'&', U"&amp;").Replace(U'<', U"&lt;").Replace(U'>', U"&gt;"))) return;
				if (!_dbus->AddStringArrayArgument(args)) return;
				if (!_dbus->AddIconArgument(icon ? icon->FindBestSizeMatch(size.x, size.y) : 0)) return;
				if (!_dbus->AddInt32Argument(-1)) return;
				if (!_dbus->EndInvocationNoWait()) return;
			}
			virtual oref<Windows::IStatusBarIcon> CreateStatusBarIcon(void) noexcept override { try { return oref<Windows::IStatusBarIcon>::CreateOwned(new XStatusBarIcon(this, this)); } catch (...) { return 0; } }
			virtual bool LaunchIPCServer(const string & app_id, const string & auth_id) noexcept override
			{
				if (_ebus >= 0) return false;
				try { _ebus_socket_name = U"/tmp/eipc." + auth_id + U"." + app_id; } catch (...) { return false; }
				_ebus = socket(AF_UNIX, SOCK_STREAM, 0);
				if (_ebus < 0) return false;
				struct sockaddr_un addr;
				Memory::ZeroMemory(&addr, sizeof(addr));
				addr.sun_family = AF_UNIX;
				if (_ebus_socket_name.GetLength() >= sizeof(addr.sun_path)) { close(_ebus); _ebus = -1; return false; }
				Memory::MemoryCopy(&addr.sun_path, _ebus_socket_name.GetData(), _ebus_socket_name.GetLength() + 1);
				if (bind(_ebus, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) { close(_ebus); _ebus = -1; return false; }
				if (listen(_ebus, SOMAXCONN) < 0) { close(_ebus); _ebus = -1; return false; }
				if (!_dispatch->RegisterFileHandler(_ebus, this)) { close(_ebus); _ebus = -1; return false; }
				return true;
			}
			virtual oref<Windows::IIPCClient> CreateIPCClient(const string & server_app_id, const string & server_auth_id) noexcept override { try { return oref<Windows::IIPCClient>::CreateOwned(new XIPCClient(this, server_app_id, server_auth_id)); } catch (...) { return 0; } }
		};
	}
	namespace Windows
	{
		oref<IWindowSystem> ESSE_MODULUS_FENESTRARUM_LINUX_X11_ENTRY(ErrorContext & ectx) noexcept { ESSE_TRY_INTRO return oref<IWindowSystem>(new X11::XWindowSystem); ESSE_TRY_OUTRO(0) }
	}
}