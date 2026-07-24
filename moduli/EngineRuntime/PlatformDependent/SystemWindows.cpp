#include "../Interfaces/SystemWindows.h"
#include <Fenestrae/Fenestrae.h>
#include <Graphica/Graphica.h>
#include "linux-stdui/CommonDialogs.h"
#include "SystemGraphicsEx.h"
#include "SystemWindowsEx.h"
#include "ESSE.h"

namespace Engine
{
	namespace ESSEIO
	{
		class EngineWindowClass : public ESSE::Windows::IWindowExtensionClass
		{
		public:
			virtual bool ExtensionAttached(ESSE::Windows::IWindow * window, ESSE::Object * extension) noexcept override { return true; }
			virtual void ExtensionDetached(ESSE::Windows::IWindow * window, ESSE::Object * extension) noexcept override {}
		};
		class EngineWindowLink : public ESSE::Object
		{
		public:
			Engine::Windows::IWindow * window;
		};
		ESSE::oref<ESSE::Windows::IWindowExtensionClass> _window_extension_class = ESSE::oref<ESSE::Windows::IWindowExtensionClass>::CreateOwned(new EngineWindowClass);
		class StaticPresentationEngine : public Windows::IPresentationEngine, public IRenderCallback
		{
			Windows::ICoreWindow * _window;
			ESSE::oref<ESSE::Graphica::IDeviceContext2D> _context;
			ESSE::oref<ESSE::Graphica::IColorBrush> _background_brush;
			ESSE::oref<ESSE::Graphica::IBitmapBrush> _image_brush;
			SafePointer<Codec::Frame> _image;
			Windows::ImageRenderMode _mode;
			ESSE::Color _filling;
		public:
			StaticPresentationEngine(Codec::Frame * image, Windows::ImageRenderMode mode, Color filling) noexcept : _window(0), _mode(mode), _filling(filling.Value) { _image.SetRetain(image); }
			virtual ~StaticPresentationEngine(void) override {}
			virtual string ToString(void) const override { return U"StaticPresentationEngine"; }
			virtual void Render(Windows::ICoreWindow * window, const ESSE::Rectangle & margins, const ESSE::Color & clear_color, bool clear) noexcept override
			{
				bool status;
				if (!_context) return;
				if (clear) status = _context->BeginRendering(ESSE::Graphica::TextureLoadAction::Clear, clear_color);
				else status = _context->BeginRendering(ESSE::Graphica::TextureLoadAction::DontCare, 0);
				if (!status) return;
				auto size = static_cast<Windows::IWindow *>(window)->GetClientSize();
				ESSE::Rectangle viewport(margins.left, margins.top, size.x - margins.right, size.y - margins.bottom);
				if (_filling.value) {
					if (!_background_brush) _background_brush = _context->CreateSolidColorBrush(_filling);
					_context->Render(_background_brush, viewport);
				}
				if (_image) {
					if (!_image_brush) {
						ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
						auto factory = ESSE::owrap(reinterpret_cast<ESSE::Graphica::IDeviceContextFactory2D *>(_context->DynamicCast(ESSE::Classes.IDeviceContextFactory2D, ectx)));
						if (!ESSE::ErrorTest(ectx)) try {
							auto bitmap = factory->LoadBitmap(WrapFrame(_image));
							if (bitmap) _image_brush = _context->CreateBitmapBrush(bitmap, ESSE::Rectangle(0, 0, bitmap->GetWidth(), bitmap->GetHeight()));
						} catch (...) {}
					}
					ESSE::Rectangle output;
					if (_mode == Windows::ImageRenderMode::FitKeepAspectRatio) {
						int w = _image->GetWidth();
						int h = _image->GetHeight();
						int vpw = viewport.right - viewport.left;
						int vph = viewport.bottom - viewport.top;
						if (vpw > 0 && vph > 0) {
							double ai = double(w) / double(h);
							double av = double(vph) / double(vpw);
							if (ai > av) { w = vpw; h = w / ai; }
							else { h = vph; w = h * ai; }
							output.left = (viewport.left + viewport.right - w) / 2;
							output.top = (viewport.top + viewport.bottom - h) / 2;
							output.right = output.left + w;
							output.bottom = output.top + h;
						} else output = ESSE::Rectangle(0, 0, 0, 0);
					} else if (_mode == Windows::ImageRenderMode::CoverKeepAspectRatio) {
						int w = _image->GetWidth();
						int h = _image->GetHeight();
						int vpw = viewport.right - viewport.left;
						int vph = viewport.bottom - viewport.top;
						if (vpw > 0 && vph > 0) {
							double ai = double(w) / double(h);
							double av = double(vph) / double(vpw);
							if (ai > av) { h = vph; w = h * ai; }
							else { w = vpw; h = w / ai; }
							output.left = (viewport.left + viewport.right - w) / 2;
							output.top = (viewport.top + viewport.bottom - h) / 2;
							output.right = output.left + w;
							output.bottom = output.top + h;
						} else output = ESSE::Rectangle(0, 0, 0, 0);
					} else if (_mode == Windows::ImageRenderMode::Blit) {
						int w = _image->GetWidth();
						int h = _image->GetHeight();
						output.left = (viewport.left + viewport.right - w) / 2;
						output.top = (viewport.top + viewport.bottom - h) / 2;
						output.right = output.left + w;
						output.bottom = output.top + h;
					} else output = viewport;
					_context->Render(_image_brush, output);
				}
				_context->EndRendering();
			}
			virtual void Attach(Windows::ICoreWindow * window) override
			{
				_window = window;
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				auto factory = ESSE::Graphica::CreateDeviceContextFactory2D(ectx);
				if (ESSE::ErrorTest(ectx)) return;
				_context = factory->CreatePresentationContext(reinterpret_cast<ESSE::Windows::IWindow *>(_window->GetOSHandle()), 0);
				if (!_context) return;
				SetWindowRenderCallback(_window, this);
			}
			virtual void Detach(void) override { _context.Clear(); _background_brush.Clear(); _image_brush.Clear(); _window = 0; }
			virtual void Invalidate(void) override { _image_brush.Clear(); if (_window) InvalidateWindow(_window); }
			virtual void Resize(int width, int height) override { if (_window) InvalidateWindow(_window); }
		};
		class RegularPresentationEngine : public Windows::I2DPresentationEngine, public IRenderCallback
		{
			Windows::DeviceClass _device_class;
			Windows::ICoreWindow * _window;
			SafePointer<Graphics::I2DDeviceContext> _context;
			ESSE::oref<ESSE::Graphica::IDeviceContext2D> _inner;
			ESSE::Color _clear_color;
			bool _clear;
		public:
			RegularPresentationEngine(Windows::DeviceClass device_class) noexcept : _device_class(device_class), _window(0) {}
			virtual ~RegularPresentationEngine(void) override {}
			virtual string ToString(void) const override { return U"RegularPresentationEngine"; }
			virtual void Render(Windows::ICoreWindow * window, const ESSE::Rectangle & margins, const ESSE::Color & clear_color, bool clear) noexcept override
			{
				_clear_color = clear_color; _clear = clear;
				auto callback = static_cast<Windows::IWindow *>(window)->GetCallback();
				if (callback) callback->RenderWindow(static_cast<Windows::IWindow *>(window));
			}
			virtual void Attach(Windows::ICoreWindow * window) override
			{
				_window = window;
				Graphics::IDevice * wrapper_device = 0;
				ESSE::oref<ESSE::Graphica::IDevice> device;
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				auto factory = ESSE::Graphica::CreateDeviceContextFactory2D(ectx);
				if (ESSE::ErrorTest(ectx)) return;
				if (_device_class == Windows::DeviceClass::Hardware || _device_class == Windows::DeviceClass::DontCare) {
					wrapper_device = Graphics::GetCommonDevice();
					if (wrapper_device && wrapper_device->GetDeviceClass() == Engine::Graphics::DeviceClass::Software && _device_class == Windows::DeviceClass::DontCare) wrapper_device = 0;
					if (wrapper_device) {
						device = UnwrapDevice(wrapper_device);
						if (!device) wrapper_device = 0;
					}
					if (_device_class == Windows::DeviceClass::Hardware && !device) return;
				}
				_inner = factory->CreatePresentationContext(reinterpret_cast<ESSE::Windows::IWindow *>(_window->GetOSHandle()), device);
				if (!_inner) return;
				try { _context = WrapContext(_inner, wrapper_device); } catch (...) { _inner.Clear(); }
				SetWindowRenderCallback(_window, this);
			}
			virtual void Detach(void) override { _context.SetReference(0); _inner.Clear(); _window = 0; }
			virtual void Invalidate(void) override { if (_window) InvalidateWindow(_window); }
			virtual void Resize(int width, int height) override {}
			virtual Graphics::I2DDeviceContext * GetContext(void) noexcept override { return _context; }
			virtual bool BeginRenderingPass(void) noexcept override
			{
				if (!_inner) return false;
				if (_clear) return _inner->BeginRendering(ESSE::Graphica::TextureLoadAction::Clear, _clear_color);
				else return _inner->BeginRendering(ESSE::Graphica::TextureLoadAction::DontCare, 0);
			}
			virtual bool EndRenderingPass(void) noexcept override
			{
				if (!_inner) return false;
				return _inner->EndRendering();
			}
		};
		class Screen : public Windows::IScreen
		{
			ESSE::oref<ESSE::Windows::IScreen> _inner;
		public:
			Screen(ESSE::Windows::IScreen * inner) noexcept : _inner(inner) {}
			virtual ~Screen(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual string GetName(void) override { return _inner->GetName().GetData(); }
			virtual Box GetScreenRectangle(void) noexcept override { auto rect = _inner->GetScreenRectangle(); return Box(rect.left, rect.top, rect.right, rect.bottom); }
			virtual Box GetUserRectangle(void) noexcept override { auto rect = _inner->GetUserRectangle(); return Box(rect.left, rect.top, rect.right, rect.bottom); }
			virtual Point GetResolution(void) noexcept override { auto size = _inner->GetResolution(); return Point(size.x, size.y); }
			virtual double GetDpiScale(void) noexcept override { return _inner->GetScaleFactor(); }
			virtual Codec::Frame * Capture(void) noexcept override { auto frame = _inner->Capture(); if (!frame) return 0; try { return WrapFrame(frame); } catch (...) { return 0; } }
			ESSE::Windows::IScreen * Unwrap(void) const noexcept { return _inner; }
		};
		class Theme : public Windows::ITheme
		{
			ESSE::oref<ESSE::Windows::ITheme> _inner;
		public:
			Theme(ESSE::Windows::ITheme * inner) noexcept : _inner(inner) {}
			virtual ~Theme(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Windows::ThemeClass GetClass(void) noexcept override
			{
				if (_inner->GetColorScheme() == ESSE::Windows::ThemeColorScheme::Dark) return Windows::ThemeClass::Dark;
				else return Windows::ThemeClass::Light;
			}
			virtual Color GetColor(Windows::ThemeColor color) noexcept override
			{
				if (color == Windows::ThemeColor::Accent) return _inner->GetColor(ESSE::Windows::ThemeColor::Accent).value;
				else if (color == Windows::ThemeColor::WindowBackgroup) return _inner->GetColor(ESSE::Windows::ThemeColor::WindowBackgroup).value;
				else if (color == Windows::ThemeColor::WindowText) return _inner->GetColor(ESSE::Windows::ThemeColor::WindowText).value;
				else if (color == Windows::ThemeColor::SelectedBackground) return _inner->GetColor(ESSE::Windows::ThemeColor::SelectedBackground).value;
				else if (color == Windows::ThemeColor::SelectedText) return _inner->GetColor(ESSE::Windows::ThemeColor::SelectedText).value;
				else if (color == Windows::ThemeColor::MenuBackground) return _inner->GetColor(ESSE::Windows::ThemeColor::MenuBackground).value;
				else if (color == Windows::ThemeColor::MenuText) return _inner->GetColor(ESSE::Windows::ThemeColor::MenuText).value;
				else if (color == Windows::ThemeColor::MenuHotBackground) return _inner->GetColor(ESSE::Windows::ThemeColor::MenuHotBackground).value;
				else if (color == Windows::ThemeColor::MenuHotText) return _inner->GetColor(ESSE::Windows::ThemeColor::MenuHotText).value;
				else if (color == Windows::ThemeColor::GrayedText) return _inner->GetColor(ESSE::Windows::ThemeColor::GrayedText).value;
				else if (color == Windows::ThemeColor::Hyperlink) return _inner->GetColor(ESSE::Windows::ThemeColor::Hyperlink).value;
				else return 0;
			}
			ESSE::Windows::ITheme * Unwrap(void) const noexcept { return _inner; }
		};
		class Cursor : public Windows::ICursor
		{
			ESSE::oref<ESSE::Windows::ICursor> _inner;
		public:
			Cursor(ESSE::Windows::ICursor * inner) noexcept : _inner(inner) {}
			virtual ~Cursor(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual handle GetOSHandle(void) noexcept override { return _inner.Inner(); }
		};
		class Window : public Windows::IWindow, public ESSE::Windows::IWindowCallback, public IRenderCallback
		{
			ESSE::oref<ESSE::Windows::IWindow> _inner;
			ESSE::oref<ESSE::Windows::ITheme> _theme;
			SafePointer<Windows::IPresentationEngine> _engine;
			SafePointer<EngineWindowLink> _link;
			Windows::IWindowCallback * _callback;
			IRenderCallback * _render_callback;
			ESSE::Rectangle _margins;
			uint _mouse_state_mask; // 1 - left button pressed, 2 - right button pressed, 4 - mouse entered, 8 - mouse virtually captured
			ESSE::Index2 _mouse_last_position;
		public:
			Window(ESSE::Windows::IWindow * inner, Windows::IWindowCallback * callback, const ESSE::Rectangle & margins) : _inner(inner), _callback(callback), _render_callback(0), _margins(margins), _mouse_state_mask(0)
			{
				_inner->SetCallback(this);
				_link = new EngineWindowLink;
				_link->window = this;
				if (!_inner->AddExtension(_link, _window_extension_class)) throw OutOfMemoryException();
				if (_callback) _callback->Created(this);
			}
			virtual ~Window(void) override { _link->window = 0; }
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual void SubmitTask(IDispatchTask * task) override { Windows::GetWindowSystem()->SubmitTask(task); }
			virtual void BeginSubmit(void) override {}
			virtual void AppendTask(IDispatchTask * task) override { Windows::GetWindowSystem()->SubmitTask(task); }
			virtual void EndSubmit(void) override {}
			virtual void SetPresentationEngine(Windows::IPresentationEngine * engine) override { _render_callback = 0; if (_engine) _engine->Detach(); _engine.SetRetain(engine); if (_engine) _engine->Attach(this); InvalidateContents(); }
			virtual Windows::IPresentationEngine * GetPresentationEngine(void) override { return _engine; }
			virtual void InvalidateContents(void) override { if (_engine) _engine->Invalidate(); }
			virtual handle GetOSHandle(void) override { return _inner.Inner(); }
			virtual void Show(bool show) override { _inner->SetVisibility(show); }
			virtual bool IsVisible(void) override { return _inner->GetVisibility(); }
			virtual void SetText(const string & text) override { _inner->SetTitle(static_cast<const ESSE::unichar32 *>(text)); }
			virtual string GetText(void) override { return _inner->GetTitle().GetData(); }
			virtual void SetPosition(const Box & box) override { _inner->SetPosition(ESSE::Rectangle(box.Left, box.Top, box.Right, box.Bottom)); }
			virtual Box GetPosition(void) override { auto pos = _inner->GetPosition(); return Box(pos.left, pos.top, pos.right, pos.bottom); }
			virtual Point GetClientSize(void) override { auto size = _inner->GetClientSize(); return Point(size.x, size.y); }
			virtual void SetMinimalConstraints(Point size) override { _inner->SetMinimalConstraints(ESSE::Index2(size.x, size.y)); }
			virtual Point GetMinimalConstraints(void) override { auto size = _inner->GetMinimalConstraints(); return Point(size.x, size.y); }
			virtual void SetMaximalConstraints(Point size) override { _inner->SetMaximalConstraints(ESSE::Index2(size.x, size.y)); }
			virtual Point GetMaximalConstraints(void) override { auto size = _inner->GetMaximalConstraints(); return Point(size.x, size.y); }
			virtual void Activate(void) override { _inner->Activate(); }
			virtual bool IsActive(void) override { return _inner->IsActive(); }
			virtual void Maximize(void) override { _inner->Maximize(); }
			virtual bool IsMaximized(void) override { return _inner->IsMaximized(); }
			virtual void Minimize(void) override { _inner->Minimize(); }
			virtual bool IsMinimized(void) override { return _inner->IsMinimized(); }
			virtual void Restore(void) override { _inner->Restore(); }
			virtual void RequireAttention(void) override { _inner->RequireAttention(); }
			virtual void SetOpacity(double opacity) override { _inner->SetOpacity(opacity); }
			virtual void SetCloseButtonState(Windows::CloseButtonState state) override
			{
				if (state == Windows::CloseButtonState::Alert) _inner->SetCloseButtonState(ESSE::Windows::CloseButtonState::Alert);
				else if (state == Windows::CloseButtonState::Disabled) _inner->SetCloseButtonState(ESSE::Windows::CloseButtonState::Disabled);
				else if (state == Windows::CloseButtonState::Enabled) _inner->SetCloseButtonState(ESSE::Windows::CloseButtonState::Enabled);
			}
			virtual IWindow * GetParentWindow(void) override
			{
				auto window = _inner->GetParentWindow();
				if (!window) return 0;
				auto ext = window->GetExtension(_window_extension_class);
				return ext ? static_cast<EngineWindowLink *>(ext)->window : 0;
			}
			virtual IWindow * GetChildWindow(int index) override
			{
				auto window = _inner->GetChildWindow(index);
				if (!window) return 0;
				auto ext = window->GetExtension(_window_extension_class);
				return ext ? static_cast<EngineWindowLink *>(ext)->window : 0;
			}
			virtual int GetChildrenCount(void) override { return _inner->GetChildrenCount(); }
			virtual void SetProgressMode(Windows::ProgressDisplayMode mode) override
			{
				if (mode == Windows::ProgressDisplayMode::Hide) _inner->SetProgressMode(ESSE::Windows::ProgressDisplayMode::Hide);
				else if (mode == Windows::ProgressDisplayMode::Indeterminated) _inner->SetProgressMode(ESSE::Windows::ProgressDisplayMode::Indeterminated);
				else if (mode == Windows::ProgressDisplayMode::Normal) _inner->SetProgressMode(ESSE::Windows::ProgressDisplayMode::Normal);
				else if (mode == Windows::ProgressDisplayMode::Paused) _inner->SetProgressMode(ESSE::Windows::ProgressDisplayMode::Paused);
				else if (mode == Windows::ProgressDisplayMode::Error) _inner->SetProgressMode(ESSE::Windows::ProgressDisplayMode::Error);
			}
			virtual void SetProgressValue(double value) override { _inner->SetProgressValue(value); }
			virtual void SetCocoaEffectMaterial(Windows::CocoaEffectMaterial material) override
			{
				if (material == Windows::CocoaEffectMaterial::Titlebar) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::Titlebar);
				else if (material == Windows::CocoaEffectMaterial::Selection) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::Selection);
				else if (material == Windows::CocoaEffectMaterial::Menu) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::Menu);
				else if (material == Windows::CocoaEffectMaterial::Popover) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::Popover);
				else if (material == Windows::CocoaEffectMaterial::Sidebar) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::Sidebar);
				else if (material == Windows::CocoaEffectMaterial::HeaderView) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::HeaderView);
				else if (material == Windows::CocoaEffectMaterial::Sheet) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::Sheet);
				else if (material == Windows::CocoaEffectMaterial::WindowBackground) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::WindowBackground);
				else if (material == Windows::CocoaEffectMaterial::HUD) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::HUD);
				else if (material == Windows::CocoaEffectMaterial::FullScreenUI) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::FullScreenUI);
				else if (material == Windows::CocoaEffectMaterial::ToolTip) _inner->SetCocoaEffectMaterial(ESSE::Windows::CocoaEffectMaterial::ToolTip);
			}
			virtual void SetCallback(Windows::IWindowCallback * callback) override { _callback = callback; _inner->SetCallback(this); }
			virtual Windows::IWindowCallback * GetCallback(void) override { return _callback; }
			virtual bool PointHitTest(Point at) override { return _inner->PerformHitTest(ESSE::Index2(at.x, at.y)); }
			virtual Point PointClientToGlobal(Point at) override { auto pos = _inner->ConvertClientToGlobal(ESSE::Index2(at.x, at.y)); return Point(pos.x, pos.y); }
			virtual Point PointGlobalToClient(Point at) override { auto pos = _inner->ConvertGlobalToClient(ESSE::Index2(at.x, at.y)); return Point(pos.x, pos.y); }
			virtual void SetFocus(void) override { _inner->SetFocus(); }
			virtual bool IsFocused(void) override { return _inner->IsFocused(); }
			virtual void SetCapture(void) override
			{
				if (_inner->GetEffectiveStyle(ESSE::Windows::CreateWindowDescType::CreateWindowDesc) & ESSE::Windows::WindowStylePopup) {
					_mouse_state_mask |= 8;
					_inner->ActAsPopup();
					if (_callback) _callback->CaptureChanged(this, true);
				} else if ((_mouse_state_mask & 12) == 4) {
					_mouse_state_mask |= 8;
					if (_callback) _callback->CaptureChanged(this, true);
				}
			}
			virtual void ReleaseCapture(void) override
			{
				if (_mouse_state_mask & 8) {
					_mouse_state_mask &= ~8;
					if (_callback) _callback->CaptureChanged(this, false);
				}
			}
			virtual bool IsCaptured(void) override { if (_mouse_state_mask & 8) return true; else return false; }
			virtual void SetTimer(uint32 id, uint32 period) override { _inner->SetTimer(id, period); }
			virtual void SetBackbufferedRenderingDevice(Codec::Frame * image, Windows::ImageRenderMode mode, Color filling) noexcept override
			{
				try {
					SafePointer<Windows::IPresentationEngine> engine = new StaticPresentationEngine(image, mode, filling);
					SetPresentationEngine(engine);
				} catch (...) {}
			}
			virtual Windows::I2DPresentationEngine * Set2DRenderingDevice(Windows::DeviceClass device_class) noexcept override
			{
				try {
					SafePointer<Windows::I2DPresentationEngine> engine = new RegularPresentationEngine(device_class);
					SetPresentationEngine(engine);
					return engine;
				} catch (...) { return 0; }
			}
			virtual double GetDpiScale(void) override { return _inner->GetScaleFactor(); }
			virtual Windows::IScreen * GetCurrentScreen(void) override
			{
				try {
					auto inner = _inner->GetScreen();
					if (!inner) return 0;
					return new Screen(inner);
				} catch (...) { return 0; }
			}
			virtual Windows::ITheme * GetCurrentTheme(void) override
			{
				try {
					auto inner = _inner->GetTheme();
					if (!inner) return 0;
					return new Theme(inner);
				} catch (...) { return 0; }
			}
			virtual uint GetBackgroundFlags(void) override
			{
				auto style = _inner->GetEffectiveStyle(ESSE::Windows::CreateWindowDescType::CreateWindowDesc);
				uint result = 0;
				if (style & ESSE::Windows::WindowStyleTransparent) result |= Windows::WindowFlagTransparent;
				if (style & ESSE::Windows::WindowStyleSetBlurBehind) result |= Windows::WindowFlagBlurBehind;
				if (style & ESSE::Windows::WindowStyleSetBlurFactor) result |= Windows::WindowFlagBlurFactor;
				return result;
			}
			virtual void Destroy(void) override { SetPresentationEngine(0); _inner->Destroy(); Release(); }
			virtual void Render(Windows::ICoreWindow * window, const ESSE::Rectangle & margins, const ESSE::Color & clear_color, bool clear) noexcept override { if (_callback) _callback->RenderWindow(this); }
			virtual void Destroyed(ESSE::Windows::IWindow * window) noexcept override { try { if (_callback) _callback->Destroyed(this); } catch (...) {} }
			virtual void Shown(ESSE::Windows::IWindow * window, bool show) noexcept override { try { if (_callback) _callback->Shown(this, show); } catch (...) {} }
			virtual void RenderWindow(ESSE::Windows::IWindow * window) noexcept override
			{
				if (!_render_callback) return;
				bool clear = false;
				ESSE::Color clear_color;
				if (_inner->GetEffectiveStyle(ESSE::Windows::CreateWindowDescType::CreateWindowDesc) & (ESSE::Windows::WindowStyleTransparent | ESSE::Windows::WindowStyleSetBlurBehind)) {
					clear = true; clear_color = 0;
				} else if (_margins.left || _margins.top || _margins.right || _margins.bottom) {
					clear = true;
					if (_inner->GetEffectiveStyle(ESSE::Windows::CreateWindowDescType::CreateWindowsWindowDesc) & ESSE::Windows::WindowWindowsStyleExtendedFrame) clear_color = 0;
					else { if (!_theme) _theme = _inner->GetTheme(); clear_color = _theme ? _theme->GetColor(ESSE::Windows::ThemeColor::WindowBackgroup) : ESSE::Color(0xFFFFFFFF); }
				} else if (_inner->GetEffectiveStyle(ESSE::Windows::CreateWindowDescType::CreateCocoaWindowDesc) & (ESSE::Windows::WindowCocoaStyleEffectBackground | ESSE::Windows::WindowCocoaStyleGlassBackground | ESSE::Windows::WindowCocoaStyleCustomBackground)) {
					clear = true; clear_color = 0;
				}
				_render_callback->Render(this, _margins, clear_color, clear);
			}
			virtual void WindowClosed(ESSE::Windows::IWindow * window) noexcept override { try { if (_callback) _callback->WindowClose(this); } catch (...) {} }
			virtual void WindowMaximized(ESSE::Windows::IWindow * window) noexcept override { try { if (_callback) _callback->WindowMaximize(this); } catch (...) {} }
			virtual void WindowMinimized(ESSE::Windows::IWindow * window) noexcept override { try { if (_callback) _callback->WindowMinimize(this); } catch (...) {} }
			virtual void WindowRestored(ESSE::Windows::IWindow * window) noexcept override { try { if (_callback) _callback->WindowRestore(this); } catch (...) {} }
			virtual void WindowHelpRequired(ESSE::Windows::IWindow * window) noexcept override { try { if (_callback) _callback->WindowHelp(this); } catch (...) {} }
			virtual void WindowActivated(ESSE::Windows::IWindow * window) noexcept override { try { if (_callback) _callback->WindowActivate(this); } catch (...) {} }
			virtual void WindowDeactivated(ESSE::Windows::IWindow * window) noexcept override
			{
				if (_mouse_state_mask & 8) ReleaseCapture();
				try { if (_callback) _callback->WindowDeactivate(this); } catch (...) {}
			}
			virtual void WindowMoved(ESSE::Windows::IWindow * window) noexcept override { try { if (_callback) _callback->WindowMove(this); } catch (...) {} }
			virtual void WindowResized(ESSE::Windows::IWindow * window) noexcept override { try { auto size = _inner->GetClientSize(); if (_engine) _engine->Resize(size.x, size.y); if (_callback) _callback->WindowSize(this); } catch (...) {} }
			virtual void FocusChanged(ESSE::Windows::IWindow * window, bool got) noexcept override
			{
				if (!got) if (_mouse_state_mask & 8) ReleaseCapture();
				try { if (_callback) _callback->FocusChanged(this, got); } catch (...) {}
			}
			virtual bool KeyIsDown(ESSE::Windows::IWindow * window, uint vkc, uint vkm) noexcept override { try { if (_callback) return _callback->KeyDown(this, vkc); else return false; } catch (...) { return false; } }
			virtual void KeyIsUp(ESSE::Windows::IWindow * window, uint vkc, uint vkm) noexcept override { try { if (_callback) _callback->KeyUp(this, vkc); } catch (...) {} }
			virtual void CharacterIsDown(ESSE::Windows::IWindow * window, ESSE::unichar32 ucs) noexcept override { try { if (_callback) _callback->CharDown(this, ucs); } catch (...) {} }
			virtual void MouseEntered(ESSE::Windows::IWindow * window, uint button_state) noexcept override { _mouse_state_mask |= 4; }
			virtual void MouseLeft(ESSE::Windows::IWindow * window, uint button_state) noexcept override
			{
				_mouse_state_mask &= ~4;
				if ((_mouse_state_mask & 8) && !(_mouse_state_mask & 3)) {
					auto client = _inner->GetClientSize();
					ESSE::Index2 v;
					if (_mouse_last_position.x < client.x / 2) v.x = -1; else v.x = client.x;
					if (_mouse_last_position.y < client.y / 2) v.y = -1; else v.y = client.y;
					MouseMoved(window, v, button_state);
				}
			}
			virtual void MouseMoved(ESSE::Windows::IWindow * window, const ESSE::Index2 & at, uint button_state) noexcept override
			{
				_mouse_last_position = at;
				try {
					if (_callback) {
						auto p = Point(at.x, at.y);
						_callback->MouseMove(this, p);
						_callback->SetCursor(this, p);
					}
					auto cursor = GetCurrentlySetCursor();
					if (cursor) _inner->SetCursor(reinterpret_cast<ESSE::Windows::ICursor *>(cursor->GetOSHandle()));
				} catch (...) {}
			}
			virtual void LeftButtonIsDown(ESSE::Windows::IWindow * window, const ESSE::Index2 & at, bool double_click) noexcept override
			{
				_mouse_last_position = at;
				_mouse_state_mask |= 1;
				try { if (_callback) if (double_click) _callback->LeftButtonDoubleClick(this, Point(at.x, at.y)); else _callback->LeftButtonDown(this, Point(at.x, at.y)); } catch (...) {}
			}
			virtual void LeftButtonIsUp(ESSE::Windows::IWindow * window, const ESSE::Index2 & at) noexcept override
			{
				_mouse_last_position = at;
				_mouse_state_mask &= ~1;
				try { if (_callback) _callback->LeftButtonUp(this, Point(at.x, at.y)); } catch (...) {}
			}
			virtual void RightButtonIsDown(ESSE::Windows::IWindow * window, const ESSE::Index2 & at, bool double_click) noexcept override
			{
				_mouse_last_position = at;
				_mouse_state_mask |= 2;
				try { if (_callback) if (double_click) _callback->RightButtonDoubleClick(this, Point(at.x, at.y)); else _callback->RightButtonDown(this, Point(at.x, at.y)); } catch (...) {}
			}
			virtual void RightButtonIsUp(ESSE::Windows::IWindow * window, const ESSE::Index2 & at) noexcept override
			{
				_mouse_last_position = at;
				_mouse_state_mask &= ~2;
				try { if (_callback) _callback->RightButtonUp(this, Point(at.x, at.y)); } catch (...) {}
			}
			virtual void ScrollVertically(ESSE::Windows::IWindow * window, const ESSE::Index2 & at, double delta) noexcept override { try { if (_callback) _callback->ScrollVertically(this, delta); } catch (...) {} }
			virtual void ScrollHorizontally(ESSE::Windows::IWindow * window, const ESSE::Index2 & at, double delta) noexcept override { try { if (_callback) _callback->ScrollHorizontally(this, delta); } catch (...) {} }
			virtual void EndPopup(ESSE::Windows::IWindow * window) noexcept override { ReleaseCapture(); }
			virtual void Timer(ESSE::Windows::IWindow * window, int timer_id) noexcept override { try { if (_callback) _callback->Timer(this, timer_id); } catch (...) {} }
			virtual void ThemeChanged(ESSE::Windows::IWindow * window) noexcept override { _theme.Clear(); try { if (_callback) _callback->ThemeChanged(this); } catch (...) {} }
			virtual bool IsWindowCommandEnabled(ESSE::Windows::IWindow * window, ESSE::Windows::WindowCommand command) noexcept override
			{
				if (!_callback) return false;
				try {
					if (command == ESSE::Windows::WindowCommand::Save) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Save);
					else if (command == ESSE::Windows::WindowCommand::SaveAs) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::SaveAs);
					else if (command == ESSE::Windows::WindowCommand::Export) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Export);
					else if (command == ESSE::Windows::WindowCommand::Print) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Print);
					else if (command == ESSE::Windows::WindowCommand::Undo) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Undo);
					else if (command == ESSE::Windows::WindowCommand::Redo) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Redo);
					else if (command == ESSE::Windows::WindowCommand::Cut) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Cut);
					else if (command == ESSE::Windows::WindowCommand::Copy) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Copy);
					else if (command == ESSE::Windows::WindowCommand::Paste) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Paste);
					else if (command == ESSE::Windows::WindowCommand::Duplicate) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Duplicate);
					else if (command == ESSE::Windows::WindowCommand::Delete) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Delete);
					else if (command == ESSE::Windows::WindowCommand::Find) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Find);
					else if (command == ESSE::Windows::WindowCommand::Replace) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::Replace);
					else if (command == ESSE::Windows::WindowCommand::SelectAll) return _callback->IsWindowEventEnabled(this, Windows::WindowHandler::SelectAll);
					else return false;
				} catch (...) { return false; }
			}
			virtual void HandleWindowCommand(ESSE::Windows::IWindow * window, ESSE::Windows::WindowCommand command) noexcept override
			{
				if (!_callback) return;
				try {
					if (command == ESSE::Windows::WindowCommand::Save) _callback->HandleWindowEvent(this, Windows::WindowHandler::Save);
					else if (command == ESSE::Windows::WindowCommand::SaveAs) _callback->HandleWindowEvent(this, Windows::WindowHandler::SaveAs);
					else if (command == ESSE::Windows::WindowCommand::Export) _callback->HandleWindowEvent(this, Windows::WindowHandler::Export);
					else if (command == ESSE::Windows::WindowCommand::Print) _callback->HandleWindowEvent(this, Windows::WindowHandler::Print);
					else if (command == ESSE::Windows::WindowCommand::Undo) _callback->HandleWindowEvent(this, Windows::WindowHandler::Undo);
					else if (command == ESSE::Windows::WindowCommand::Redo) _callback->HandleWindowEvent(this, Windows::WindowHandler::Redo);
					else if (command == ESSE::Windows::WindowCommand::Cut) _callback->HandleWindowEvent(this, Windows::WindowHandler::Cut);
					else if (command == ESSE::Windows::WindowCommand::Copy) _callback->HandleWindowEvent(this, Windows::WindowHandler::Copy);
					else if (command == ESSE::Windows::WindowCommand::Paste) _callback->HandleWindowEvent(this, Windows::WindowHandler::Paste);
					else if (command == ESSE::Windows::WindowCommand::Duplicate) _callback->HandleWindowEvent(this, Windows::WindowHandler::Duplicate);
					else if (command == ESSE::Windows::WindowCommand::Delete) _callback->HandleWindowEvent(this, Windows::WindowHandler::Delete);
					else if (command == ESSE::Windows::WindowCommand::Find) _callback->HandleWindowEvent(this, Windows::WindowHandler::Find);
					else if (command == ESSE::Windows::WindowCommand::Replace) _callback->HandleWindowEvent(this, Windows::WindowHandler::Replace);
					else if (command == ESSE::Windows::WindowCommand::SelectAll) _callback->HandleWindowEvent(this, Windows::WindowHandler::SelectAll);
				} catch (...) {}
			}
			void SetRenderCallback(IRenderCallback * callback) noexcept { _render_callback = callback; }
		};
		class MenuItem : public Windows::IMenuItem, public ESSE::Windows::IMenuItemCallback
		{
			ESSE::oref<ESSE::Windows::IMenuItem> _inner;
			SafePointer<Windows::IMenu> _submenu;
			handle _user_data;
			Windows::IMenuItemCallback * _callback;
		public:
			MenuItem(ESSE::Windows::IMenuItem * inner) noexcept : _inner(inner), _user_data(0), _callback(0) { inner->SetUserData(this); }
			virtual ~MenuItem(void) override { try { if (_callback) _callback->MenuItemDisposed(this); } catch (...) {} }
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual void SetCallback(Windows::IMenuItemCallback * callback) override { _callback = callback; _inner->SetCallback(_callback ? this : 0); }
			virtual Windows::IMenuItemCallback * GetCallback(void) override { return _callback; }
			virtual void SetUserData(void * data) override { _user_data = data; }
			virtual void * GetUserData(void) override { return _user_data; }
			virtual void SetSubmenu(Windows::IMenu * menu) override { _submenu.SetRetain(menu); _inner->SetSubmenu(_submenu ? reinterpret_cast<ESSE::Windows::IMenu *>(_submenu->GetOSHandle()) : 0); }
			virtual Windows::IMenu * GetSubmenu(void) override { return _submenu; }
			virtual void SetID(int id) override { _inner->SetID(id); }
			virtual int GetID(void) override { return _inner->GetID(); }
			virtual void SetText(const string & text) override { _inner->SetText(static_cast<const ESSE::unichar32 *>(text)); }
			virtual string GetText(void) override { return _inner->GetText().GetData(); }
			virtual void SetSideText(const string & text) override { _inner->SetSideText(static_cast<const ESSE::unichar32 *>(text)); }
			virtual string GetSideText(void) override { return _inner->GetSideText().GetData(); }
			virtual void SetIsSeparator(bool separator) override { _inner->SetIsSeparator(separator); }
			virtual bool IsSeparator(void) override { return _inner->IsSeparator(); }
			virtual void Enable(bool enable) override { _inner->Enable(enable); }
			virtual bool IsEnabled(void) override { return _inner->IsEnabled(); }
			virtual void Check(bool check) override { _inner->Check(check); }
			virtual bool IsChecked(void) override { return _inner->IsChecked(); }
			virtual ESSE::Index2 MeasureMenuItem(ESSE::Windows::IMenuItem * item, ESSE::Graphica::IDeviceContext2D * device, ESSE::Graphica::DeviceCache * common_cache) noexcept override
			{
				try {
					SafePointer<Graphics::I2DDeviceContext> context = WrapContext(device, common_cache);
					if (_callback) {
						auto size = _callback->MeasureMenuItem(this, context);
						return ESSE::Index2(size.x, size.y);
					} else return ESSE::Index2(1, 1);
				} catch (...) { return ESSE::Index2(1, 1); }
			}
			virtual void RenderMenuItem(ESSE::Windows::IMenuItem * item, ESSE::Graphica::IDeviceContext2D * device, ESSE::Graphica::DeviceCache * common_cache, const ESSE::Rectangle & at, bool hot_state) noexcept override
			{
				try {
					SafePointer<Graphics::I2DDeviceContext> context = WrapContext(device, common_cache);
					if (_callback) _callback->RenderMenuItem(this, context, Box(at.left, at.top, at.right, at.bottom), hot_state);
				} catch (...) {}
			}
			virtual void MenuClosed(ESSE::Windows::IMenuItem * item) noexcept override { try { if (_callback) _callback->MenuClosed(this); } catch (...) {} }
			virtual void MenuItemDisposed(ESSE::Windows::IMenuItem * item) noexcept override {}
			ESSE::Windows::IMenuItem * Unwrap(void) const noexcept { return _inner; }
		};
		class Menu : public Windows::IMenu
		{
			ESSE::oref<ESSE::Windows::IMenu> _inner;
			ObjectArray<Windows::IMenuItem> _items;
		public:
			Menu(ESSE::Windows::IMenu * inner) noexcept : _inner(inner), _items(0x20) { _inner->SetUserData(this); }
			virtual ~Menu(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual void AppendMenuItem(Windows::IMenuItem * item) noexcept override { try { _items.Append(item); _inner->AppendMenuItem(static_cast<MenuItem *>(item)->Unwrap()); } catch (...) {} }
			virtual void InsertMenuItem(Windows::IMenuItem * item, int at) noexcept override { try { if (at < 0 || at > _items.Length()) return; _items.Insert(item, at); _inner->InsertMenuItem(static_cast<MenuItem *>(item)->Unwrap(), at); } catch (...) {} }
			virtual void RemoveMenuItem(int at) noexcept override { if (at < 0 || at >= _items.Length()) return; _items.Remove(at); _inner->RemoveMenuItem(at); }
			virtual Windows::IMenuItem * ElementAt(int at) noexcept override { return _items.ElementAt(at); }
			virtual int Length(void) noexcept override { return _items.Length(); }
			virtual Windows::IMenuItem * FindMenuItem(int id) noexcept override
			{
				auto item = _inner->FindMenuItem(id);
				if (!item) return 0;
				return reinterpret_cast<Windows::IMenuItem *>(item->GetUserData());
			}
			virtual int Run(Windows::IWindow * owner, Point at) noexcept override { return _inner->Perform(owner ? reinterpret_cast<ESSE::Windows::IWindow *>(owner->GetOSHandle()) : 0, ESSE::Index2(at.x, at.y)); }
			virtual handle GetOSHandle(void) noexcept override { return _inner.Inner(); }
		};
		class StatusBarIcon : public Windows::IStatusBarIcon, public ESSE::Windows::IStatusCallback
		{
			ESSE::oref<ESSE::Windows::IStatusBarIcon> _inner;
			SafePointer<Windows::IMenu> _menu;
			Windows::IStatusCallback * _callback;
		public:
			StatusBarIcon(ESSE::Windows::IStatusBarIcon * inner) noexcept : _inner(inner), _callback(0) {}
			virtual ~StatusBarIcon(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual void SetCallback(Windows::IStatusCallback * callback) noexcept override { _callback = callback; }
			virtual Windows::IStatusCallback * GetCallback(void) noexcept override { return _callback; }
			virtual Point GetIconSize(void) noexcept override { auto size = _inner->GetIconSize(); return Point(size.x, size.y); }
			virtual void SetIcon(Codec::Image * image) noexcept override
			{
				try {
					if (!image) return;
					auto icon = ESSE::owrap(new ESSE::Picturae::Image);
					for (auto & f : image->Frames) icon->Append(WrapFrame(&f));
					_inner->SetIcon(icon);
				} catch (...) {}
			}
			virtual Codec::Image * GetIcon(void) noexcept override
			{
				try {
					auto inner = _inner->GetIcon();
					if (!inner) return 0;
					SafePointer<Codec::Image> result = new Codec::Image;
					for (auto & f : *inner) {
						SafePointer<Codec::Frame> frame = WrapFrame(&f);
						result->Frames.Append(frame);
					}
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual void SetIconColorUsage(Windows::StatusBarIconColorUsage color_usage) noexcept override
			{
				if (color_usage == Windows::StatusBarIconColorUsage::Monochromic) _inner->SetIconColorUsage(ESSE::Windows::StatusBarIconColorUsage::Monochromic);
				else _inner->SetIconColorUsage(ESSE::Windows::StatusBarIconColorUsage::Regular);
			}
			virtual Windows::StatusBarIconColorUsage GetIconColorUsage(void) noexcept override
			{
				if (_inner->GetIconColorUsage() == ESSE::Windows::StatusBarIconColorUsage::Monochromic) return Windows::StatusBarIconColorUsage::Monochromic;
				else return Windows::StatusBarIconColorUsage::Colourfull;
			}
			virtual void SetTooltip(const string & text) noexcept override { try { _inner->SetTooltip(static_cast<const ESSE::unichar32 *>(text)); } catch (...) {} }
			virtual string GetTooltip(void) noexcept override { try {  return _inner->GetTooltip().GetData(); } catch (...) { return U""; } }
			virtual void SetEventID(int ID) noexcept override { _inner->SetEventID(ID); }
			virtual int GetEventID(void) noexcept override { return _inner->GetEventID(); }
			virtual void SetMenu(Windows::IMenu * menu) noexcept override { _menu.SetRetain(menu); if (_menu) _inner->SetMenu(reinterpret_cast<ESSE::Windows::IMenu *>(_menu->GetOSHandle())); else _inner->SetMenu(0); }
			virtual Windows::IMenu * GetMenu(void) noexcept override { return _menu; }
			virtual bool PresentIcon(bool present) noexcept override { return _inner->PresentIcon(present); }
			virtual bool IsVisible(void) noexcept override { return _inner->IsVisible(); }
			virtual void HandleStatusIconCommand(IStatusBarIcon * icon, int id) noexcept { if (_callback) _callback->StatusIconCommand(this, id); }
		};
		class IPCClient : public Windows::IIPCClient
		{
			ESSE::oref<ESSE::Windows::IIPCClient> _inner;
		private:
			static Windows::IPCStatus _status_wrap(ESSE::Windows::IPCStatus status) noexcept
			{
				if (status == ESSE::Windows::IPCStatus::Accepted) return Windows::IPCStatus::Accepted;
				else if (status == ESSE::Windows::IPCStatus::Discarded) return Windows::IPCStatus::Discarded;
				else if (status == ESSE::Windows::IPCStatus::ServerClosed) return Windows::IPCStatus::ServerClosed;
				else if (status == ESSE::Windows::IPCStatus::InternalError) return Windows::IPCStatus::InternalError;
				else return Windows::IPCStatus::Unknown;
			}
		public:
			IPCClient(ESSE::Windows::IIPCClient * inner) noexcept : _inner(inner) {}
			virtual ~IPCClient(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual bool SendData(const string & verb, const DataBlock * data, IDispatchTask * on_responce, Windows::IPCStatus * result) noexcept override
			{
				try {
					SafePointer<IDispatchTask> hdlr;
					hdlr.SetRetain(on_responce);
					auto task = ESSE::CreateStructuredTask<ESSE::Windows::IPCStatus>([presult = result, hdlr](ESSE::Windows::IPCStatus status) {
						if (presult) *presult = _status_wrap(status);
						if (hdlr) hdlr->DoTask(Windows::GetWindowSystem());
					});
					return _inner->SendData(static_cast<const ESSE::unichar32 *>(verb), data ? data->GetBuffer() : 0, data ? data->Length() : 0, task, &task->Value1);
				} catch (...) { return false; }
			}
			virtual bool RequestData(const string & verb, IDispatchTask * on_responce, Windows::IPCStatus * result, DataBlock ** data) noexcept override
			{
				try {
					SafePointer<IDispatchTask> hdlr;
					hdlr.SetRetain(on_responce);
					auto task = ESSE::CreateStructuredTask<ESSE::Windows::IPCStatus, ESSE::oref<ESSE::DataBlock>>([presult = result, pdata = data, hdlr](ESSE::Windows::IPCStatus status, const ESSE::oref<ESSE::DataBlock> & data) {
						if (presult) *presult = _status_wrap(status);
						if (pdata) *pdata = 0;
						if (pdata) try {
							SafePointer<DataBlock> block = new DataBlock(data->GetLength());
							block->SetLength(data->GetLength());
							MemoryCopy(block->GetBuffer(), data->GetBuffer(), data->GetLength());
							block->Retain();
							*pdata = block;
						} catch (...) {
							if (presult) *presult = Windows::IPCStatus::InternalError;
							if (pdata) *pdata = 0;
						}
						if (hdlr) hdlr->DoTask(Windows::GetWindowSystem());
					});
					return _inner->RequireData(static_cast<const ESSE::unichar32 *>(verb), task, &task->Value1, &task->Value2);
				} catch (...) { return false; }
			}
			virtual Windows::IPCStatus GetStatus(void) noexcept override { return _status_wrap(_inner->GetStatus()); }
		};
		class WindowSystem : public Windows::IWindowSystem, public ESSE::Windows::IApplicationCallback
		{
			ESSE::oref<ESSE::Windows::IWindowSystem> _inner;
			Windows::IApplicationCallback * _callback;
			ESSE::Set<Windows::IWindow *> _top_level;
			SafePointer<Windows::ICursor> _user_set_cursor;
		private:
			static void _decompose_style(uint engine_flags, uint & common_style, uint & windows_style, uint & cocoa_style) noexcept
			{
				common_style = windows_style = cocoa_style = 0;
				if (engine_flags & Windows::WindowFlagHasTitle) common_style |= ESSE::Windows::WindowStyleHasTitle;
				if (engine_flags & Windows::WindowFlagSizeble) common_style |= ESSE::Windows::WindowStyleResizeble;
				if (engine_flags & Windows::WindowFlagCloseButton) common_style |= ESSE::Windows::WindowStyleCloseButton;
				if (engine_flags & Windows::WindowFlagMinimizeButton) common_style |= ESSE::Windows::WindowStyleMinimizeButton;
				if (engine_flags & Windows::WindowFlagMaximizeButton) common_style |= ESSE::Windows::WindowStyleMaximizeButton;
				if (engine_flags & Windows::WindowFlagHelpButton) common_style |= ESSE::Windows::WindowStyleHelpButton;
				if (engine_flags & Windows::WindowFlagToolWindow) common_style |= ESSE::Windows::WindowStyleToolWindow;
				if (engine_flags & Windows::WindowFlagPopup) common_style |= ESSE::Windows::WindowStylePopup;
				if (engine_flags & Windows::WindowFlagNonOpaque) common_style |= ESSE::Windows::WindowStyleSetOpacity;
				if (engine_flags & Windows::WindowFlagOverrideTheme) common_style |= ESSE::Windows::WindowStyleSetColorScheme;
				if (engine_flags & Windows::WindowFlagTransparent) common_style |= ESSE::Windows::WindowStyleTransparent;
				if (engine_flags & Windows::WindowFlagBlurBehind) common_style |= ESSE::Windows::WindowStyleSetBlurBehind;
				if (engine_flags & Windows::WindowFlagBlurFactor) common_style |= ESSE::Windows::WindowStyleSetBlurFactor;
				if (engine_flags & Windows::WindowFlagWindowsExtendedFrame) windows_style |= ESSE::Windows::WindowWindowsStyleExtendedFrame;
				if (engine_flags & Windows::WindowFlagWindowsNormalTitle) windows_style |= ESSE::Windows::WindowWindowsStyleNormalTitle;
				if (engine_flags & Windows::WindowFlagWindowsTabbedTitle) windows_style |= ESSE::Windows::WindowWindowsStyleTabbedTitle;
				if (engine_flags & Windows::WindowFlagWindowsTransientTitle) windows_style |= ESSE::Windows::WindowWindowsStyleTransientTitle;
				if (engine_flags & Windows::WindowFlagWindowsColoredTitle) windows_style |= ESSE::Windows::WindowWindowsStyleColoredTitle;
				if (engine_flags & Windows::WindowFlagCocoaTransparentTitle) cocoa_style |= ESSE::Windows::WindowCocoaStyleTransparentTitle;
				if (engine_flags & Windows::WindowFlagCocoaEffectBackground) cocoa_style |= ESSE::Windows::WindowCocoaStyleEffectBackground;
				if (engine_flags & Windows::WindowFlagCocoaShadowless) cocoa_style |= ESSE::Windows::WindowCocoaStyleShadowless;
				if (engine_flags & Windows::WindowFlagCocoaContentUnderTitle) cocoa_style |= ESSE::Windows::WindowCocoaStyleContentUnderTitle;
				if (engine_flags & Windows::WindowFlagCocoaCustomBackground) cocoa_style |= ESSE::Windows::WindowCocoaStyleCustomBackground;
			}
			Windows::IWindow * _create_window(const Windows::CreateWindowDesc & desc, bool modal) noexcept
			{
				try {
					ESSE::Windows::CreateWindowDesc mdesc;
					ESSE::Windows::CreateWindowsWindowDesc wdesc;
					ESSE::Windows::CreateCocoaWindowDesc cdesc;
					mdesc.next_desc = &wdesc;
					wdesc.next_desc = &cdesc;
					_decompose_style(desc.Flags, mdesc.style, wdesc.extended_style, cdesc.extended_style);
					if (modal) mdesc.style |= ESSE::Windows::WindowStyleModal;
					mdesc.title = static_cast<const ESSE::unichar32 *>(desc.Title);
					mdesc.callback = 0;
					mdesc.position = ESSE::Rectangle(desc.Position.Left, desc.Position.Top, desc.Position.Right, desc.Position.Bottom);
					mdesc.minimal_constraints = ESSE::Index2(desc.MinimalConstraints.x, desc.MinimalConstraints.y);
					mdesc.maximal_constraints = ESSE::Index2(desc.MaximalConstraints.x, desc.MaximalConstraints.y);
					mdesc.parent_window = desc.ParentWindow ? reinterpret_cast<ESSE::Windows::IWindow *>(desc.ParentWindow->GetOSHandle()) : 0;
					mdesc.screen = desc.Screen ? static_cast<Screen *>(desc.Screen)->Unwrap() : 0;
					if (mdesc.style & ESSE::Windows::WindowStyleSetOpacity) mdesc.opacity = desc.Opacity;
					if (mdesc.style & ESSE::Windows::WindowStyleSetBlurFactor) mdesc.blur_behind_factor = desc.BlurFactor;
					if (mdesc.style & ESSE::Windows::WindowStyleSetColorScheme) {
						if (desc.Theme == Windows::ThemeClass::Light) mdesc.color_scheme = ESSE::Windows::ThemeColorScheme::Light;
						else if (desc.Theme == Windows::ThemeClass::Dark) mdesc.color_scheme = ESSE::Windows::ThemeColorScheme::Dark;
						else return 0;
					}
					#if defined(ESSE_SYSTEMA_WINDOWS)
					if (wdesc.extended_style & ESSE::Windows::WindowWindowsStyleExtendedFrame) wdesc.margins = ESSE::Rectangle(desc.FrameMargins.Left, desc.FrameMargins.Top, desc.FrameMargins.Right, desc.FrameMargins.Bottom);
					else wdesc.margins = ESSE::Rectangle(0, 0, 0, 0);
					#else
					wdesc.margins = ESSE::Rectangle(0, 0, 0, 0);
					#endif
					if ((wdesc.extended_style & ESSE::Windows::WindowWindowsStyleTitleMask) == ESSE::Windows::WindowWindowsStyleColoredTitle) wdesc.title_color = desc.BackgroundColor.Value;
					if (cdesc.extended_style & ESSE::Windows::WindowCocoaStyleCustomBackground) cdesc.background_color = desc.BackgroundColor.Value;
					auto inner = _inner->CreateWindow(&mdesc);
					if (!inner) return 0;
					try { return new Window(inner, desc.Callback, wdesc.margins); } catch (...) { inner->Destroy(); return 0; }
				} catch (...) { return 0; }
			}
		public:
			WindowSystem(ESSE::Windows::IWindowSystem * inner) noexcept : _inner(inner), _callback(0) {}
			virtual ~WindowSystem(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual void SubmitTask(IDispatchTask * task) override { SafePointer<IDispatchTask> t; t.SetRetain(task); auto s = this; _inner->SubmitTask(ESSE::CreateFunctionalTask([t, s]() { t->DoTask(s); })); }
			virtual void BeginSubmit(void) override {}
			virtual void AppendTask(IDispatchTask * task) override { SubmitTask(task); }
			virtual void EndSubmit(void) override {}
			virtual Windows::IWindow * CreateWindow(const Windows::CreateWindowDesc & desc) noexcept override { return _create_window(desc, false); }
			virtual Windows::IWindow * CreateModalWindow(const Windows::CreateWindowDesc & desc) noexcept override
			{
				auto parent = desc.ParentWindow;
				auto window = _create_window(desc, true);
				if (!window) return 0;
				if (parent) {
					window->Show(true);
					return window;
				} else {
					window->Show(true);
					_inner->RunMainLoop();
					window->Show(false);
					window->Destroy();
					return 0;
				}
			}
			virtual Box ConvertClientToWindow(const Box & box, uint flags) noexcept override
			{
				uint style, wstyle, cstyle;
				_decompose_style(flags, style, wstyle, cstyle);
				auto result = _inner->ConvertClientToWindow(ESSE::Rectangle(box.Left, box.Top, box.Right, box.Bottom), style, wstyle, cstyle);
				return Box(result.left, result.top, result.right, result.bottom);
			}
			virtual Point ConvertClientToWindow(const Point & size, uint flags) noexcept override
			{
				uint style, wstyle, cstyle;
				_decompose_style(flags, style, wstyle, cstyle);
				auto result = _inner->ConvertClientToWindow(ESSE::Index2(size.x, size.y), style, wstyle, cstyle);
				return Point(result.x, result.y);
			}
			virtual void SetFilesToOpen(const string * files, int num_files) noexcept override
			{
				try {
					ESSE::array<ESSE::string> list(num_files);
					for (int i = 0; i < num_files; i++) list.Append(static_cast<const ESSE::unichar32 *>(files[i]));
					_inner->ScheduleFilesToBeOpened(list.GetBuffer(), list.GetLength());
				} catch (...) {}
			}
			virtual Windows::IApplicationCallback * GetCallback(void) noexcept override { return _callback; }
			virtual void SetCallback(Windows::IApplicationCallback * callback) noexcept override { _callback = callback; _inner->SetCallback(this); }
			virtual void RunMainLoop(void) noexcept override { _inner->RunMainLoop(); }
			virtual void ExitMainLoop(void) noexcept override { _inner->ExitMainLoop(); }
			virtual void ExitModalSession(Windows::IWindow * window) noexcept override
			{
				if (!window) return;
				if (window->GetParentWindow()) window->Destroy();
				else _inner->ExitMainLoop();
			}
			virtual void RegisterMainWindow(Windows::IWindow * window) noexcept override { try { _top_level.AddElement(window); } catch (...) {} }
			virtual void UnregisterMainWindow(Windows::IWindow * window) noexcept override { _top_level.RemoveElement(window); if (_top_level.IsEmpty()) _inner->ExitMainLoop(); }
			virtual Point GetCursorPosition(void) noexcept override { auto pos = _inner->GetCursorPosition(); return Point(pos.x, pos.y); }
			virtual void SetCursorPosition(Point position) noexcept override { _inner->SetCursorPosition(ESSE::Index2(position.x, position.y)); }
			virtual Windows::ICursor * LoadCursor(Codec::Frame * source) noexcept override
			{
				try {
					if (!source) return 0;
					auto inner = _inner->LoadCursor(WrapFrame(source));
					if (!inner) return 0;
					return new Cursor(inner);
				} catch (...) { return 0; }
			}
			virtual Windows::ICursor * GetSystemCursor(Windows::SystemCursorClass cursor) noexcept override
			{
				try {
					ESSE::Windows::SystemCursorClass cls;
					if (cursor == Windows::SystemCursorClass::Arrow) cls = ESSE::Windows::SystemCursorClass::Arrow;
					else if (cursor == Windows::SystemCursorClass::Beam) cls = ESSE::Windows::SystemCursorClass::Beam;
					else if (cursor == Windows::SystemCursorClass::Link) cls = ESSE::Windows::SystemCursorClass::Link;
					else if (cursor == Windows::SystemCursorClass::Null) cls = ESSE::Windows::SystemCursorClass::Null;
					else if (cursor == Windows::SystemCursorClass::SizeAll) cls = ESSE::Windows::SystemCursorClass::SizeAll;
					else if (cursor == Windows::SystemCursorClass::SizeLeftRight) cls = ESSE::Windows::SystemCursorClass::SizeLeftRight;
					else if (cursor == Windows::SystemCursorClass::SizeUpDown) cls = ESSE::Windows::SystemCursorClass::SizeUpDown;
					else if (cursor == Windows::SystemCursorClass::SizeLeftUpRightDown) cls = ESSE::Windows::SystemCursorClass::SizeLeftUpRightDown;
					else if (cursor == Windows::SystemCursorClass::SizeLeftDownRightUp) cls = ESSE::Windows::SystemCursorClass::SizeLeftDownRightUp;
					else return 0;
					auto inner = _inner->GetSystemCursor(cls);
					if (!inner) return 0;
					return new Cursor(inner);
				} catch (...) { return 0; }
			}
			virtual void SetCursor(Windows::ICursor * cursor) noexcept override { _user_set_cursor.SetRetain(cursor); }
			virtual Array<Point> * GetApplicationIconSizes(void) noexcept override
			{
				try {
					auto sizes = _inner->GetApplicationIconSizes();
					SafePointer<Array<Point>> result = new Array<Point>(sizes->GetLength());
					for (auto & s : *sizes) result->Append(Point(s.x, s.y));
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual void SetApplicationIcon(Codec::Image * icon) noexcept override
			{
				try {
					if (!icon) return;
					auto image = ESSE::owrap(new ESSE::Picturae::Image);
					for (auto & i : icon->Frames) image->Append(WrapFrame(&i));
					_inner->SetApplicationIcon(image);
				} catch (...) {}
			}
			virtual void SetApplicationBadge(const string & text) noexcept override { try { _inner->SetApplicationBadge(static_cast<const ESSE::unichar32 *>(text)); } catch (...) {} }
			virtual void SetApplicationIconVisibility(bool visible) noexcept override { _inner->SetApplicationIconVisibility(visible); }
			virtual bool OpenFileDialog(Windows::OpenFileInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept override
			{
				// TODO: REWORK LINUX-SPECIFIC STUB
				return Linux::CommonOpenFileDialog(info, parent, on_exit);
			}
			virtual bool SaveFileDialog(Windows::SaveFileInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept override
			{
				// TODO: REWORK LINUX-SPECIFIC STUB
				return Linux::CommonSaveFileDialog(info, parent, on_exit);
			}
			virtual bool ChooseDirectoryDialog(Windows::ChooseDirectoryInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept override
			{
				// TODO: REWORK LINUX-SPECIFIC STUB
				return Linux::CommonDirectoryDialog(info, parent, on_exit);
			}
			virtual bool MessageBox(Windows::MessageBoxResult * result, const string & text, const string & title, Windows::IWindow * parent, Windows::MessageBoxButtonSet buttons, Windows::MessageBoxStyle style, IDispatchTask * on_exit) noexcept override
			{
				// TODO: REWORK LINUX-SPECIFIC STUB
				return Linux::CommonMessageBox(result, text, title, parent, buttons, style, on_exit);
			}
			virtual Windows::IMenu * CreateMenu(void) noexcept override
			{
				try {
					auto inner = _inner->CreateMenu();
					if (!inner) return 0;
					return new Menu(inner);
				} catch (...) { return 0; }
			}
			virtual Windows::IMenuItem * CreateMenuItem(void) noexcept override
			{
				try {
					auto inner = _inner->CreateMenuItem();
					if (!inner) return 0;
					return new MenuItem(inner);
				} catch (...) { return 0; }
			}
			virtual Point GetUserNotificationIconSize(void) noexcept override { auto size = _inner->GetUserNotificationIconSize(); return Point(size.x, size.y); }
			virtual void PushUserNotification(const string & title, const string & text, Codec::Image * icon) noexcept override
			{
				try {
					ESSE::oref<ESSE::Picturae::Image> image;
					if (icon) {
						image = ESSE::owrap(new ESSE::Picturae::Image);
						for (auto & i : icon->Frames) image->Append(WrapFrame(&i));
					}
					_inner->PushUserNotification(static_cast<const ESSE::unichar32 *>(title), static_cast<const ESSE::unichar32 *>(text), image);
				} catch (...) {}
			}
			virtual Windows::IStatusBarIcon * CreateStatusBarIcon(void) noexcept override
			{
				try {
					auto inner = _inner->CreateStatusBarIcon();
					if (!inner) return 0;
					return new StatusBarIcon(inner);
				} catch (...) { return 0; }
			}
			virtual bool CreateHotKey(int event_id, int key_code, uint key_flags) noexcept override { return _inner->GetKeyboardManager()->RegisterHotKey(event_id, key_code, key_flags); }
			virtual void RemoveHotKey(int event_id) noexcept override { _inner->GetKeyboardManager()->UnregisterHotKey(event_id); }
			virtual bool LaunchIPCServer(const string & app_id, const string & auth_id) noexcept override { try { return _inner->LaunchIPCServer(static_cast<const ESSE::unichar32 *>(app_id), static_cast<const ESSE::unichar32 *>(auth_id)); } catch (...) { return false; } }
			virtual Windows::IIPCClient * CreateIPCClient(const string & server_app_id, const string & server_auth_id) noexcept override
			{
				try {
					auto inner = _inner->CreateIPCClient(static_cast<const ESSE::unichar32 *>(server_app_id), static_cast<const ESSE::unichar32 *>(server_auth_id));
					if (!inner) return 0;
					return new IPCClient(inner);
				} catch (...) { return 0; }
			}
			virtual bool AcceptsApplicationCommand(ESSE::Windows::ApplicationCommand command) noexcept override
			{
				if (!_callback) return false;
				try {
					if (command == ESSE::Windows::ApplicationCommand::CreateFile) return _callback->IsHandlerEnabled(Windows::ApplicationHandler::CreateFile);
					else if (command == ESSE::Windows::ApplicationCommand::OpenSomeFile) return _callback->IsHandlerEnabled(Windows::ApplicationHandler::OpenSomeFile);
					else if (command == ESSE::Windows::ApplicationCommand::OpenSpecificFile) return _callback->IsHandlerEnabled(Windows::ApplicationHandler::OpenExactFile);
					else if (command == ESSE::Windows::ApplicationCommand::ShowAbout) return _callback->IsHandlerEnabled(Windows::ApplicationHandler::ShowAbout);
					else if (command == ESSE::Windows::ApplicationCommand::ShowHelp) return _callback->IsHandlerEnabled(Windows::ApplicationHandler::ShowHelp);
					else if (command == ESSE::Windows::ApplicationCommand::ShowProperties) return _callback->IsHandlerEnabled(Windows::ApplicationHandler::ShowProperties);
					else if (command == ESSE::Windows::ApplicationCommand::Terminate) return _callback->IsHandlerEnabled(Windows::ApplicationHandler::Terminate);
					else return false;
				} catch (...) { return false; }
			}
			virtual bool AcceptsWindowCommand(ESSE::Windows::WindowCommand command) noexcept override
			{
				if (!_callback) return false;
				try {
					if (command == ESSE::Windows::WindowCommand::Save) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Save);
					else if (command == ESSE::Windows::WindowCommand::SaveAs) return _callback->IsWindowEventAccessible(Windows::WindowHandler::SaveAs);
					else if (command == ESSE::Windows::WindowCommand::Export) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Export);
					else if (command == ESSE::Windows::WindowCommand::Print) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Print);
					else if (command == ESSE::Windows::WindowCommand::Undo) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Undo);
					else if (command == ESSE::Windows::WindowCommand::Redo) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Redo);
					else if (command == ESSE::Windows::WindowCommand::Cut) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Cut);
					else if (command == ESSE::Windows::WindowCommand::Copy) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Copy);
					else if (command == ESSE::Windows::WindowCommand::Paste) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Paste);
					else if (command == ESSE::Windows::WindowCommand::Duplicate) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Duplicate);
					else if (command == ESSE::Windows::WindowCommand::Delete) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Delete);
					else if (command == ESSE::Windows::WindowCommand::Find) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Find);
					else if (command == ESSE::Windows::WindowCommand::Replace) return _callback->IsWindowEventAccessible(Windows::WindowHandler::Replace);
					else if (command == ESSE::Windows::WindowCommand::SelectAll) return _callback->IsWindowEventAccessible(Windows::WindowHandler::SelectAll);
					else return false;
				} catch (...) { return false; }
			}
			virtual bool HandleApplicationCommand(ESSE::Windows::ApplicationCommand command, const ESSE::string & argument) noexcept override
			{
				if (!_callback) return false;
				try {
					if (command == ESSE::Windows::ApplicationCommand::CreateFile) { _callback->CreateNewFile(); return true; }
					else if (command == ESSE::Windows::ApplicationCommand::OpenSomeFile) { _callback->OpenSomeFile(); return true; }
					else if (command == ESSE::Windows::ApplicationCommand::OpenSpecificFile) { _callback->OpenExactFile(argument.GetData()); return true; }
					else if (command == ESSE::Windows::ApplicationCommand::ShowAbout) { _callback->ShowAbout(); return true; }
					else if (command == ESSE::Windows::ApplicationCommand::ShowHelp) { _callback->ShowHelp(); return true; }
					else if (command == ESSE::Windows::ApplicationCommand::ShowProperties) { _callback->ShowProperties(); return true; }
					else if (command == ESSE::Windows::ApplicationCommand::Terminate) return _callback->Terminate();
					else return false;
				} catch (...) { return false; }
			}
			virtual void HandleHotKeyEvent(ESSE::uint event_id) noexcept override { try { if (_callback) _callback->HotKeyEvent(event_id); } catch (...) {} }
			virtual bool IPCReceiveData(ESSE::handle client, const ESSE::string & verb, const void * data, ESSE::uintptr length) noexcept override
			{
				try {
					if (!_callback) return false;
					SafePointer<DataBlock> block = new DataBlock(length);
					block->SetLength(length);
					MemoryCopy(block->GetBuffer(), data, length);
					return _callback->DataExchangeReceive(client, verb.GetData(), block);
				} catch (...) { return false; }
			}
			virtual ESSE::oref<ESSE::DataBlock> IPCSendData(ESSE::handle client, const ESSE::string & verb) noexcept override
			{
				try {
					if (!_callback) return 0;
					SafePointer<DataBlock> result = _callback->DataExchangeRespond(client, verb.GetData());
					auto block = ESSE::owrap(new ESSE::DataBlock(1));
					block->Append(result->GetBuffer(), result->Length());
					return block;
				} catch (...) { return 0; }
			}
			virtual void IPCClientDisconnect(ESSE::handle client) noexcept override { try { if (_callback) _callback->DataExchangeDisconnect(client); } catch (...) {} }
			Windows::ICursor * GetCurrentlySetCursor(void) noexcept { return _user_set_cursor; }
		};

		void SetWindowRenderCallback(Windows::ICoreWindow * window, IRenderCallback * callback) noexcept { static_cast<Window *>(window)->SetRenderCallback(callback); }
		void SetWindowUserRenderCallback(Windows::ICoreWindow * window) noexcept { static_cast<Window *>(window)->SetRenderCallback(static_cast<Window *>(window)); }
		void SetWindowNullRenderCallback(Windows::ICoreWindow * window) noexcept { static_cast<Window *>(window)->SetRenderCallback(0); }
		void InvalidateWindow(Windows::ICoreWindow * window) noexcept { reinterpret_cast<ESSE::Windows::IWindow *>(window->GetOSHandle())->Invalidate(); }
		Windows::ICursor * GetCurrentlySetCursor(void) noexcept
		{
			auto ws = Windows::GetWindowSystem();
			if (!ws) return 0;
			return static_cast<WindowSystem *>(ws)->GetCurrentlySetCursor();
		}
	}
	namespace Windows
	{
		SafePointer<IWindowSystem> _shared_system;

		ObjectArray<IScreen> * GetActiveScreens(void)
		{
			try {
				auto ws = ESSE::Windows::CreateWindowSystem();
				if (!ws) return 0;
				auto inner = ws->EnumerateScreens();
				if (!inner) return 0;
				SafePointer<ObjectArray<IScreen>> result = new ObjectArray<IScreen>(inner->GetLength());
				for (auto & s : *inner) {
					SafePointer<Windows::IScreen> screen = new ESSEIO::Screen(&s);
					result->Append(screen);
				}
				result->Retain();
				return result;
			} catch (...) { return 0; }
		}
		IScreen * GetDefaultScreen(void)
		{
			try {
				auto ws = ESSE::Windows::CreateWindowSystem();
				if (!ws) return 0;
				auto inner = ws->GetDefaultScreen();
				if (!inner) return 0;
				return new ESSEIO::Screen(inner);
			} catch (...) { return 0; }
		}
		ITheme * GetCurrentTheme(void)
		{
			try {
				auto ws = ESSE::Windows::CreateWindowSystem();
				if (!ws) return 0;
				auto inner = ws->GetSystemTheme();
				if (!inner) return 0;
				return new ESSEIO::Theme(inner);
			} catch (...) { return 0; }
		}
		IWindowSystem * GetWindowSystem(void)
		{
			if (!_shared_system) try {
				auto ws = ESSE::Windows::CreateWindowSystem();
				_shared_system = new ESSEIO::WindowSystem(ws);
			} catch (...) { return 0; }
			return _shared_system;
		}
	}
}