#include "Menues.h"

#include <Graphica/Graphica.h>
#include <math.h>

namespace ESSE
{
	namespace Linux
	{
		struct MenuVisuals : public Object
		{
			double scale;
			Windows::ThemeColorScheme scheme;
			Color accent, background, background_solid, border, selection_accent, normal, hot, grayed;
			oref<Graphica::IDeviceContextFactory2D> factory;
			oref<Graphica::IBitmap> background_transparent, background_fallback;
			oref<Graphica::IBitmap> arrow_normal, arrow_hot, arrow_grayed;
			oref<Graphica::IBitmap> check_normal, check_hot, check_grayed;
			oref<Graphica::IBitmap> selection;
			oref<Graphica::AggregateFont> font;
			int background_subsize, selection_subsize, frame_size, border_size, font_size, item_size, separator_size;
		private:
			static Color _blend(const Color & a, const Color & b, double alpha) noexcept
			{
				uint an = min(max(alpha * 255.0, 0.0), 255.0);
				Color r;
				r.r = (uint(a.r) * uint(a.a) * uint(255U - an) + uint(b.r) * uint(b.a) * uint(an)) / (255U * 255U);
				r.g = (uint(a.g) * uint(a.a) * uint(255U - an) + uint(b.g) * uint(b.a) * uint(an)) / (255U * 255U);
				r.b = (uint(a.b) * uint(a.a) * uint(255U - an) + uint(b.b) * uint(b.a) * uint(an)) / (255U * 255U);
				r.a = (uint(a.a) * uint(255U - an) + uint(b.a) * uint(an)) / 255U;
				if (r.a) {
					r.r = uint(r.r) * uint(255U) / uint(r.a);
					r.g = uint(r.g) * uint(255U) / uint(r.a);
					r.b = uint(r.b) * uint(255U) / uint(r.a);
				}
				return r;
			}
			oref<Graphica::IBitmap> _create_rounded_shape(int size, int subsize, int brdsize, const Color & back, const Color & frame)
			{
				Picturae::PictureDesc desc;
				desc.width = desc.height = size;
				desc.stride = 4 * desc.width;
				desc.palette_size = 0;
				desc.format = Picturae::PixelFormat::R8G8B8A8;
				desc.alpha_mode = Picturae::AlphaMode::Straight;
				desc.origin = Picturae::ScanOrigin::TopLeft;
				auto picture = owrap(new Picturae::Picture(desc, Picturae::PictureInit::AllocateUninitialized));
				desc = picture->GetDesc();
				for (int y = 0; y < desc.height; y++) for (int x = 0; x < desc.width; x++) {
					if ((x >= subsize && x < desc.width - subsize) || (y >= subsize && y < desc.height - subsize)) {
						if (x < brdsize || y < brdsize || x >= desc.width - brdsize || y >= desc.height - brdsize) Picturae::SetPixel(desc, x, y, frame);
						else Picturae::SetPixel(desc, x, y, back);
					} else {
						int r = subsize - 1, ri = r - brdsize;
						double dx = x < subsize ? r - x : x + r - (desc.width - 1);
						double dy = y < subsize ? r - y : y + r - (desc.height - 1);
						double d = sqrt(dx * dx + dy * dy);
						if (d < ri) Picturae::SetPixel(desc, x, y, back);
						else if (d < ri + 1.0) Picturae::SetPixel(desc, x, y, _blend(back, frame, d - ri));
						else if (d < r) Picturae::SetPixel(desc, x, y, frame);
						else if (d < r + 1.0) Picturae::SetPixel(desc, x, y, _blend(frame, 0, d - r));
						else Picturae::SetPixel(desc, x, y, 0);
					}
				}
				return factory->LoadBitmap(picture);
			}
			oref<Graphica::IBitmap> _create_check_shape(int size, const Color & front)
			{
				int cw = size / 8;
				int w3 = size / 3;
				int delta = (w3 - cw) / 2;
				Color partial(front.r, front.g, front.b, front.a / 2);
				Picturae::PictureDesc desc;
				desc.width = desc.height = size;
				desc.stride = 4 * desc.width;
				desc.palette_size = 0;
				desc.format = Picturae::PixelFormat::R8G8B8A8;
				desc.alpha_mode = Picturae::AlphaMode::Straight;
				desc.origin = Picturae::ScanOrigin::TopLeft;
				auto picture = owrap(new Picturae::Picture(desc, Picturae::PictureInit::AllocateUninitialized));
				desc = picture->GetDesc();
				for (int y = 0; y < desc.height; y++) for (int x = 0; x < desc.width; x++) {
					int h = abs(x - w3) + delta;
					int z = desc.height - 1 - y;
					if (z < h) Picturae::SetPixel(desc, x, y, 0);
					else if (z == h) Picturae::SetPixel(desc, x, y, partial);
					else if (z < h + cw + 1) Picturae::SetPixel(desc, x, y, front);
					else if (z == h + cw + 1) Picturae::SetPixel(desc, x, y, partial);
					else Picturae::SetPixel(desc, x, y, 0);
					if (x < w3 && z > w3 + delta) Picturae::SetPixel(desc, x, y, 0);
					if (x > w3 && z > 2 * w3 + delta) Picturae::SetPixel(desc, x, y, 0);
				}
				return factory->LoadBitmap(picture);
			}
			oref<Graphica::IBitmap> _create_arrow_shape(int size, const Color & front)
			{
				int aw = size / 8;
				int h2 = size / 2;
				int h4 = size / 4;
				Color partial(front.r, front.g, front.b, front.a / 2);
				Picturae::PictureDesc desc;
				desc.width = desc.height = size;
				desc.stride = 4 * desc.width;
				desc.palette_size = 0;
				desc.format = Picturae::PixelFormat::R8G8B8A8;
				desc.alpha_mode = Picturae::AlphaMode::Straight;
				desc.origin = Picturae::ScanOrigin::TopLeft;
				auto picture = owrap(new Picturae::Picture(desc, Picturae::PictureInit::AllocateUninitialized));
				desc = picture->GetDesc();
				for (int y = 0; y < desc.height; y++) {
					int dy = y < h2 ? h2 - 1 - y : y - h2;
					int o = (size - aw) / 2 + h2 - dy - h4;
					for (int x = 0; x < desc.width; x++) {
						if (x < o - 1) Picturae::SetPixel(desc, x, y, 0);
						else if (x == o - 1) Picturae::SetPixel(desc, x, y, partial);
						else if (x < o + aw) Picturae::SetPixel(desc, x, y, front);
						else if (x == o + aw) Picturae::SetPixel(desc, x, y, partial);
						else Picturae::SetPixel(desc, x, y, 0);
					}
				}
				return factory->LoadBitmap(picture);
			}
		public:
			MenuVisuals(Windows::IScreen * screen, Windows::ITheme * theme)
			{
				scale = screen->GetScaleFactor();
				accent = theme->GetColor(Windows::ThemeColor::Accent);
				scheme = theme->GetColorScheme();
				int icon_size = 12.0 * scale;
				selection_subsize = 4.0 * scale;
				font_size = 16.0 * scale;
				item_size = font_size + selection_subsize;
				frame_size = scale;
				border_size = separator_size = 4.0 * scale;
				background_subsize = border_size + selection_subsize;
				factory = Graphica::CreateDeviceContextFactory2D();
				font = owrap(new Graphica::AggregateFont(factory, string(), Graphica::CreateFontSystemDefault | Graphica::CreateFontSansSerif | Graphica::CreateFontWeight400, font_size));
				if (scheme == Windows::ThemeColorScheme::Dark) {
					background = Color(0x00, 0x00, 0x00, 0xC0);
					background_solid = Color(0x20, 0x20, 0x20);
				} else {
					background = Color(0xFF, 0xFF, 0xFF, 0xC0);
					background_solid = Color(0xE0, 0xE0, 0xE0);
				}
				border = theme->GetColor(Windows::ThemeColor::GrayedText);
				normal = theme->GetColor(Windows::ThemeColor::MenuText);
				hot = theme->GetColor(Windows::ThemeColor::MenuHotText);
				selection_accent = theme->GetColor(Windows::ThemeColor::MenuHotBackground);
				background_transparent = _create_rounded_shape(background_subsize + background_subsize + 2, background_subsize, frame_size, background, border);
				background_fallback = _create_rounded_shape(background_subsize + background_subsize + 2, 0, frame_size, background_solid, border);
				selection = _create_rounded_shape(selection_subsize + selection_subsize + 2, selection_subsize, 1, selection_accent, selection_accent);
				arrow_normal = _create_arrow_shape(icon_size, normal);
				arrow_hot = _create_arrow_shape(icon_size, hot);
				arrow_grayed = _create_arrow_shape(icon_size, grayed);
				check_normal = _create_check_shape(icon_size, normal);
				check_hot = _create_check_shape(icon_size, hot);
				check_grayed = _create_check_shape(icon_size, grayed);
			}
			virtual ~MenuVisuals(void) override {}
		};
		class BackgroundVisual : public Object
		{
			int _subsize;
			oref<Graphica::IBitmap> _bitmap;
			oref<Graphica::IBitmapBrush> _00;
			oref<Graphica::IBitmapBrush> _01;
			oref<Graphica::IBitmapBrush> _02;
			oref<Graphica::IBitmapBrush> _10;
			oref<Graphica::IBitmapBrush> _11;
			oref<Graphica::IBitmapBrush> _12;
			oref<Graphica::IBitmapBrush> _20;
			oref<Graphica::IBitmapBrush> _21;
			oref<Graphica::IBitmapBrush> _22;
		public:
			BackgroundVisual(Graphica::IBitmap * map, int subsize) : _bitmap(map), _subsize(subsize) {}
			virtual ~BackgroundVisual(void) override {}
			void Render(Graphica::IDeviceContext2D * context, Graphica::DeviceCache * cache, const Rectangle & at)
			{
				if (!_00) _00 = cache->CreateBitmapBrush(_bitmap, Rectangle(0, 0, _subsize, _subsize));
				if (!_01) _01 = cache->CreateBitmapBrush(_bitmap, Rectangle(_subsize, 0, _bitmap->GetWidth() - _subsize, _subsize));
				if (!_02) _02 = cache->CreateBitmapBrush(_bitmap, Rectangle(_bitmap->GetWidth() - _subsize, 0, _bitmap->GetWidth(), _subsize));
				if (!_10) _10 = cache->CreateBitmapBrush(_bitmap, Rectangle(0, _subsize, _subsize, _bitmap->GetHeight() - _subsize));
				if (!_11) _11 = cache->CreateBitmapBrush(_bitmap, Rectangle(_subsize, _subsize, _bitmap->GetWidth() - _subsize, _bitmap->GetHeight() - _subsize));
				if (!_12) _12 = cache->CreateBitmapBrush(_bitmap, Rectangle(_bitmap->GetWidth() - _subsize, _subsize, _bitmap->GetWidth(), _bitmap->GetHeight() - _subsize));
				if (!_20) _20 = cache->CreateBitmapBrush(_bitmap, Rectangle(0, _bitmap->GetHeight() - _subsize, _subsize, _bitmap->GetHeight()));
				if (!_21) _21 = cache->CreateBitmapBrush(_bitmap, Rectangle(_subsize, _bitmap->GetHeight() - _subsize, _bitmap->GetWidth() - _subsize, _bitmap->GetHeight()));
				if (!_22) _22 = cache->CreateBitmapBrush(_bitmap, Rectangle(_bitmap->GetWidth() - _subsize, _bitmap->GetHeight() - _subsize, _bitmap->GetWidth(), _bitmap->GetHeight()));
				context->Render(_00, Rectangle(at.left, at.top, at.left + _subsize, at.top + _subsize));
				context->Render(_01, Rectangle(at.left + _subsize, at.top, at.right - _subsize, at.top + _subsize));
				context->Render(_02, Rectangle(at.right - _subsize, at.top, at.right, at.top + _subsize));
				context->Render(_10, Rectangle(at.left, at.top + _subsize, at.left + _subsize, at.bottom - _subsize));
				context->Render(_11, Rectangle(at.left + _subsize, at.top + _subsize, at.right - _subsize, at.bottom - _subsize));
				context->Render(_12, Rectangle(at.right - _subsize, at.top + _subsize, at.right, at.bottom - _subsize));
				context->Render(_20, Rectangle(at.left, at.bottom - _subsize, at.left + _subsize, at.bottom));
				context->Render(_21, Rectangle(at.left + _subsize, at.bottom - _subsize, at.right - _subsize, at.bottom));
				context->Render(_22, Rectangle(at.right - _subsize, at.bottom - _subsize, at.right, at.bottom));
			}
		};
		class ImageVisual : public Object
		{
			oref<Graphica::IBitmap> _bitmap;
			oref<Graphica::IBitmapBrush> _00;
		public:
			ImageVisual(Graphica::IBitmap * map) : _bitmap(map) {}
			virtual ~ImageVisual(void) override {}
			void Render(Graphica::IDeviceContext2D * context, Graphica::DeviceCache * cache, const Rectangle & at)
			{
				if (!_00) _00 = cache->CreateBitmapBrush(_bitmap, Rectangle(0, 0, _bitmap->GetWidth(), _bitmap->GetHeight()));
				Rectangle rect;
				rect.left = (at.left + at.right - _bitmap->GetWidth()) / 2;
				rect.top = (at.top + at.bottom - _bitmap->GetHeight()) / 2;
				rect.right = rect.left + _bitmap->GetWidth();
				rect.bottom = rect.top + _bitmap->GetHeight();
				context->Render(_00, rect);
			}
		};
		struct RunningMenuCallback : public Windows::IWindowCallback
		{
			class _default_menu_item_callback : public Windows::IMenuItemCallback
			{
				oref<MenuVisuals> _visuals;
				oref<Graphica::Typesetter> _ts_0_normal, _ts_1_normal, _ts_0_hot, _ts_1_hot;
				oref<Graphica::IColorBrush> _separator;
				oref<BackgroundVisual> _background;
				oref<ImageVisual> _check_normal, _check_hot;
			public:
				_default_menu_item_callback(MenuVisuals * visuals) : _visuals(visuals) {}
				virtual Index2 MeasureMenuItem(Windows::IMenuItem * item, Graphica::IDeviceContext2D * device, Graphica::DeviceCache * common_cache) noexcept override
				{
					if (item->IsSeparator()) {
						return Index2(1, _visuals->separator_size);
					} else {
						if (!_visuals) return Index2(1, 1);
						try {
							_ts_0_normal = owrap(new Graphica::Typesetter(_visuals->font, item->GetText(),
								Graphica::TypesetterFlags::HorizontalAlignmentLeft | Graphica::TypesetterFlags::VerticalAlignmentCenter | Graphica::TypesetterFlags::EnableClipping,
								item->IsEnabled() ? _visuals->normal : _visuals->grayed));
							_ts_1_normal = owrap(new Graphica::Typesetter(_visuals->font, item->GetSideText(),
								Graphica::TypesetterFlags::HorizontalAlignmentRight | Graphica::TypesetterFlags::VerticalAlignmentCenter | Graphica::TypesetterFlags::EnableClipping,
								item->IsEnabled() ? _visuals->normal : _visuals->grayed));
							return Index2(_ts_0_normal->GetExtents().x + _ts_1_normal->GetExtents().x + 3 * _visuals->item_size, _visuals->item_size);
						} catch (...) { return Index2(1, 1); }
					}
				}
				virtual void RenderMenuItem(Windows::IMenuItem * item, Graphica::IDeviceContext2D * device, Graphica::DeviceCache * common_cache, const Rectangle & at, bool hot_state) noexcept override
				{
					if (item->IsSeparator()) {
						if (!_separator) _separator = common_cache->CreateSolidColorBrush(_visuals->border);
						auto h = _visuals->separator_size / 2;
						device->Render(_separator, Rectangle(at.left + _visuals->item_size / 2, at.top + h, at.right - _visuals->item_size / 2, at.top + h + _visuals->frame_size));
					} else {
						if (hot_state) {
							try {
								if (!_ts_0_hot) _ts_0_hot = owrap(new Graphica::Typesetter(_visuals->font, item->GetText(),
									Graphica::TypesetterFlags::HorizontalAlignmentLeft | Graphica::TypesetterFlags::VerticalAlignmentCenter | Graphica::TypesetterFlags::EnableClipping, _visuals->hot));
								if (!_ts_1_hot) _ts_1_hot = owrap(new Graphica::Typesetter(_visuals->font, item->GetSideText(),
									Graphica::TypesetterFlags::HorizontalAlignmentRight | Graphica::TypesetterFlags::VerticalAlignmentCenter | Graphica::TypesetterFlags::EnableClipping, _visuals->hot));
								if (!_background) _background = owrap(new BackgroundVisual(_visuals->selection, _visuals->selection_subsize));
							} catch (...) {}
							if (_background) _background->Render(device, common_cache, at);
							if (item->IsChecked()) try {
								if (!_check_hot) _check_hot = owrap(new ImageVisual(_visuals->check_hot));
								if (_check_hot) _check_hot->Render(device, common_cache, Rectangle(at.left, at.top, at.left + _visuals->item_size, at.bottom));
							} catch (...) {}
							if (_ts_0_hot) _ts_0_hot->Render(device, common_cache, Rectangle(at.left + _visuals->item_size, at.top, at.right, at.bottom));
							if (_ts_1_hot) _ts_1_hot->Render(device, common_cache, Rectangle(at.left, at.top, at.right - _visuals->item_size, at.bottom));
						} else {
							if (item->IsChecked()) try {
								if (!_check_normal) _check_normal = owrap(new ImageVisual(item->IsEnabled() ? _visuals->check_normal : _visuals->check_grayed));
								if (_check_normal) _check_normal->Render(device, common_cache, Rectangle(at.left, at.top, at.left + _visuals->item_size, at.bottom));
							} catch (...) {}
							_ts_0_normal->Render(device, common_cache, Rectangle(at.left + _visuals->item_size, at.top, at.right, at.bottom));
							_ts_1_normal->Render(device, common_cache, Rectangle(at.left, at.top, at.right - _visuals->item_size, at.bottom));
						}
					}
				}
				virtual void MenuClosed(Windows::IMenuItem * item) noexcept override { delete this; }
			};
			struct _running_menu_item_desc
			{
				Rectangle area;
				Index2 extents;
				Windows::IMenuItem * item;
				Windows::IMenuItemCallback * callback;
				Windows::IWindow * submenu_host;
				uint state; // 1 - hot, 2 - pinned
			};
			struct _running_menu_desc
			{
				Rectangle area;
				Index2 extents;
				oref<Graphica::IDeviceContext2D> context;
				oref<Graphica::DeviceCache> context_cache;
				Windows::IMenu * menu;
				array<_running_menu_item_desc> items = array<_running_menu_item_desc>(1);
				oref<BackgroundVisual> background;
				oref<ImageVisual> arrow_normal, arrow_hot, arrow_grayed;
			};
		public:
			oref<MenuVisuals> _visuals;
			Rectangle _screen_box, _create_relative_to;
			Windows::IMenu * _root_menu, * _menu_being_created;
			Windows::IWindow * _root_window;
			Dictionary<Windows::IWindow *, _running_menu_desc> _windows;
			int _result, _exiting;
		public:
			RunningMenuCallback(MenuVisuals * visuals) : _visuals(visuals), _result(0), _exiting(0), _menu_being_created(0) {}
			void OpenPrimaryMenu(void)
			{
				_menu_being_created = _root_menu;
				Windows::CreateWindowDesc desc;
				desc.style = Windows::WindowStylePopup | Windows::WindowStyleTopmost | Windows::WindowStyleTransparent | Windows::WindowStyleModal;
				desc.position = Rectangle(0, 0, 1, 1);
				desc.maximal_constraints = desc.minimal_constraints = Index2(0, 0);
				desc.callback = this;
				_root_window = Windows::GetWindowSystem()->CreateWindow(&desc);
				_root_window->SetVisibility(true);
				Windows::GetWindowSystem()->RunMainLoop();
			}
			void OpenMenu(Windows::IMenu * submenu, const Rectangle & relative)
			{
				_menu_being_created = submenu;
				_create_relative_to = relative;
				_create_relative_to.left -= _visuals->border_size;
				_create_relative_to.right += _visuals->border_size;
				Windows::CreateWindowDesc desc;
				desc.style = Windows::WindowStylePopup | Windows::WindowStyleTopmost | Windows::WindowStyleTransparent;
				desc.position = Rectangle(0, 0, 1, 1);
				desc.maximal_constraints = desc.minimal_constraints = Index2(0, 0);
				desc.callback = this;
				desc.parent_window = _root_window;
				auto host = Windows::GetWindowSystem()->CreateWindow(&desc);
				for (auto & w : _windows) for (auto & i : w.value.items) if (i.item->GetSubmenu() == submenu) {
					i.submenu_host = host;
					goto loop_after;
				}
				loop_after:
				host->SetVisibility(true);
			}
			void CloseMenu(Windows::IMenu * menu)
			{
				Windows::IWindow * host = 0;
				for (auto & w : _windows) if (w.value.menu == menu) { host = w.key; break; }
				if (!host) return;
				auto & rec = *_windows[host];
				for (auto & i : rec.items) if (i.submenu_host) CloseMenu(i.item->GetSubmenu());
				for (auto & w : _windows) for (auto & i : w.value.items) if (i.submenu_host == host) i.submenu_host = 0;
				bool root = rec.menu == _root_menu;
				host->Destroy();
				if (root) Windows::GetWindowSystem()->ExitMainLoop();
			}
			virtual void Created(Windows::IWindow * window) noexcept override
			{
				bool primary = _windows.IsEmpty();
				_windows.Append(window, _running_menu_desc());
				auto & desc = *_windows[window];
				desc.context = _visuals->factory->CreatePresentationContext(window, 0);
				desc.context_cache = owrap(new (std::nothrow) Graphica::DeviceCache(desc.context));
				desc.menu = _menu_being_created;
				desc.items.SetLength(desc.menu->GetLength());
				desc.extents = Index2(0, 0);
				for (int i = 0; i < desc.menu->GetLength(); i++) {
					auto item = desc.menu->ElementAt(i);
					if (item->GetCallback()) desc.items[i].callback = item->GetCallback();
					else desc.items[i].callback = new _default_menu_item_callback(_visuals);
					desc.items[i].item = item;
					desc.items[i].state = 0;
					desc.items[i].extents = desc.items[i].callback->MeasureMenuItem(item, desc.context, desc.context_cache);
					desc.items[i].submenu_host = 0;
					if (desc.items[i].extents.x > desc.extents.x) desc.extents.x = desc.items[i].extents.x;
					desc.extents.y += desc.items[i].extents.y;
				}
				for (int i = 0; i < desc.menu->GetLength(); i++) desc.items[i].extents.x = desc.extents.x;
				if (_create_relative_to.top + desc.extents.y <= _screen_box.bottom) {
					desc.area.top = _create_relative_to.top - _visuals->border_size;
					desc.area.bottom = desc.area.top + desc.extents.y + 2 * _visuals->border_size;
				} else if (_create_relative_to.bottom - desc.extents.y >= _screen_box.top) {
					desc.area.bottom = _create_relative_to.bottom + _visuals->border_size;
					desc.area.top = desc.area.bottom - desc.extents.y - 2 * _visuals->border_size;
				} else {
					desc.area.top = _create_relative_to.top - _visuals->border_size;
					desc.area.bottom = desc.area.top + desc.extents.y + 2 * _visuals->border_size;
				}
				if (_create_relative_to.right + desc.extents.x + 2 * _visuals->border_size <= _screen_box.right) {
					desc.area.left = _create_relative_to.right;
					desc.area.right = desc.area.left + desc.extents.x + 2 * _visuals->border_size;
				} else if (_create_relative_to.left - desc.extents.x - 2 * _visuals->border_size >= _screen_box.left) {
					desc.area.right = _create_relative_to.left;
					desc.area.left = desc.area.right - desc.extents.x - 2 * _visuals->border_size;
				} else {
					desc.area.left = _create_relative_to.right;
					desc.area.right = desc.area.left + desc.extents.x + 2 * _visuals->border_size;
				}
				window->SetPosition(desc.area);
				_menu_being_created = 0;
			}
			virtual void Destroyed(Windows::IWindow * window) noexcept override
			{
				auto & desc = *_windows[window];
				for (auto & i : desc.items) i.callback->MenuClosed(i.item);
				_windows.Remove(window);
			}
			virtual void Shown(Windows::IWindow * window, bool show) noexcept override { if (show && _windows[window]->menu == _root_menu) { window->SetFocus(); window->ActAsPopup(); } }
			virtual void RenderWindow(Windows::IWindow * window) noexcept override
			{
				auto & desc = *_windows[window];
				if (desc.context->BeginRendering(Graphica::TextureLoadAction::Clear, 0)) {
					if (!desc.background) {
						Graphica::IBitmap * src;
						if (window->GetEffectiveStyle(Windows::CreateWindowDescType::CreateWindowDesc) & Windows::WindowStyleTransparent) src = _visuals->background_transparent;
						else src = _visuals->background_fallback;
						desc.background = owrap(new (std::nothrow) BackgroundVisual(src, _visuals->background_subsize));
					}
					if (desc.background) desc.background->Render(desc.context, desc.context_cache, Rectangle(0, 0, desc.area.right - desc.area.left, desc.area.bottom - desc.area.top));
					for (auto & i : desc.items) {
						auto area = Rectangle(i.area.left - desc.area.left, i.area.top - desc.area.top, i.area.right - desc.area.left, i.area.bottom - desc.area.top);
						i.callback->RenderMenuItem(i.item, desc.context, desc.context_cache, area, i.state);
						if (i.item->GetSubmenu()) try {
							if (i.state) {
								if (!desc.arrow_hot) desc.arrow_hot = owrap(new ImageVisual(_visuals->arrow_hot));
								if (desc.arrow_hot) desc.arrow_hot->Render(desc.context, desc.context_cache, Rectangle(area.right - _visuals->item_size, area.top, area.right, area.bottom));
							} else if (i.item->IsEnabled()) {
								if (!desc.arrow_normal) desc.arrow_normal = owrap(new ImageVisual(_visuals->arrow_normal));
								if (desc.arrow_normal) desc.arrow_normal->Render(desc.context, desc.context_cache, Rectangle(area.right - _visuals->item_size, area.top, area.right, area.bottom));
							} else {
								if (!desc.arrow_grayed) desc.arrow_grayed = owrap(new ImageVisual(_visuals->arrow_grayed));
								if (desc.arrow_grayed) desc.arrow_grayed->Render(desc.context, desc.context_cache, Rectangle(area.right - _visuals->item_size, area.top, area.right, area.bottom));
							}
						} catch (...) {}
					}
					desc.context->EndRendering();
				}
			}
			virtual void WindowMoved(Windows::IWindow * window) noexcept override
			{
				auto & desc = *_windows[window];
				desc.area = window->GetPosition();
				Index2 position(desc.area.left + _visuals->border_size, desc.area.top + _visuals->border_size);
				for (auto & i : desc.items) {
					i.area.left = position.x;
					i.area.top = position.y;
					i.area.right = i.area.left + i.extents.x;
					i.area.bottom = i.area.top + i.extents.y;
					position.y += i.extents.y;
				}
			}
			virtual void WindowResized(Windows::IWindow * window) noexcept override
			{
				auto & desc = *_windows[window];
				desc.area = window->GetPosition();
				Index2 position(desc.area.left + _visuals->border_size, desc.area.top + _visuals->border_size);
				for (auto & i : desc.items) {
					i.area.left = position.x;
					i.area.top = position.y;
					i.area.right = i.area.left + i.extents.x;
					i.area.bottom = i.area.top + i.extents.y;
					position.y += i.extents.y;
				}
			}
			virtual void MouseLeft(Windows::IWindow * window, uint button_state) noexcept override
			{
				auto pos = Windows::GetWindowSystem()->GetCursorPosition();
				for (auto & w : _windows) if (w.key->PerformHitTest(pos)) {
					bool locked = false;
					int current_selection = -1;
					for (uintptr i = 0; i < w.value.items.GetLength(); i++) {
						if (w.value.items[i].item->IsSeparator() || !w.value.items[i].item->IsEnabled()) continue;
						if (w.value.items[i].state) current_selection = i;
						if (w.value.items[i].state & 2) locked = true;
					}
					if (!locked && current_selection != -1) {
						for (uintptr i = 0; i < w.value.items.GetLength(); i++) {
							w.value.items[i].state = 0;
							if (w.value.items[i].submenu_host && w.value.items[i].item->GetSubmenu()) CloseMenu(w.value.items[i].item->GetSubmenu());
						}
						w.key->Invalidate();
					}
					return;
				}
			}
			virtual void MouseMoved(Windows::IWindow * window, const Index2 & at, uint button_state) noexcept override
			{
				auto pos = window->ConvertClientToGlobal(at);
				for (auto & w : _windows) if (w.key->PerformHitTest(pos)) {
					pos.x = (w.value.area.right + w.value.area.left) / 2;
					bool locked = false;
					int current_selection = -1, new_selection = -1;
					for (uintptr i = 0; i < w.value.items.GetLength(); i++) {
						if (w.value.items[i].item->IsSeparator() || !w.value.items[i].item->IsEnabled()) continue;
						if (w.value.items[i].area.IsInside(pos)) new_selection = i;
						if (w.value.items[i].state) current_selection = i;
						if (w.value.items[i].state & 2) locked = true;
					}
					if (!locked && current_selection != new_selection) {
						for (uintptr i = 0; i < w.value.items.GetLength(); i++) {
							if (i == new_selection) {
								w.value.items[i].state = 1;
								if (!w.value.items[i].submenu_host && w.value.items[i].item->GetSubmenu()) OpenMenu(w.value.items[i].item->GetSubmenu(), w.value.items[i].area);
							} else {
								w.value.items[i].state = 0;
								if (w.value.items[i].submenu_host && w.value.items[i].item->GetSubmenu()) CloseMenu(w.value.items[i].item->GetSubmenu());
							}
						}
						w.key->Invalidate();
					}
					return;
				}
			}
			virtual void LeftButtonIsDown(Windows::IWindow * window, const Index2 & at, bool double_click) noexcept override
			{
				auto pos = window->ConvertClientToGlobal(at);
				for (auto & w : _windows) if (w.key->PerformHitTest(pos)) {
					for (auto & i : w.value.items) if (i.state && i.item->GetSubmenu() && i.submenu_host) i.state ^= 2;
					MouseMoved(window, at, 0);
					return;
				}
			}
			virtual void LeftButtonIsUp(Windows::IWindow * window, const Index2 & at) noexcept override
			{
				auto pos = window->ConvertClientToGlobal(at);
				for (auto & w : _windows) if (w.key->PerformHitTest(pos)) {
					for (auto & i : w.value.items) if (i.state && !i.item->GetSubmenu()) {
						_result = i.item->GetID();
						_exiting = 1;
						CloseMenu(_root_menu);
						return;
					}
				}
			}
			virtual void FocusChanged(Windows::IWindow * window, bool got) noexcept override { if (!got) EndPopup(window); }
			virtual void EndPopup(Windows::IWindow * window) noexcept override
			{
				if (_exiting) return;
				_exiting = 1;
				try { Windows::GetWindowSystem()->SubmitTask(CreateFunctionalTask([this]() { CloseMenu(_root_menu); })); } catch (...) {}
			}
		};
		class MenuItem : public Windows::IMenuItem
		{
			friend class Menu;
		private:
			IMenuService * _system;
			Windows::IMenuItemCallback * _callback;
			union { void * _user_data; Object * _user_object; };
			oref<Windows::IMenu> _submenu;
			int _id;
			string _text, _alt_text;
			bool _separator, _enabled, _checked, _has_object;
		public:
			MenuItem(IMenuService * system) : _system(system), _callback(0), _user_data(0), _id(0), _separator(false), _enabled(false), _checked(false), _has_object(false) {}
			virtual ~MenuItem(void) override { if (_callback) _callback->MenuItemDisposed(this); if (_has_object && _user_object) _user_object->Release(); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Linux Menu Item"; ESSE_TRY_OUTRO(string()) }
			virtual Windows::IMenuItemCallback * GetCallback(void) noexcept override { return _callback; }
			virtual void SetCallback(Windows::IMenuItemCallback * const & callback) noexcept override { _callback = callback; }
			virtual void * GetUserData(void) noexcept override { return _user_data; }
			virtual void SetUserData(void * const & data) noexcept override
			{
				if (_has_object && _user_object) _user_object->Release();
				_has_object = false; _user_data = data;
			}
			virtual Object * GetUserObject(void) noexcept override { return _user_object; }
			virtual void SetUserObject(Object * const & object) noexcept override
			{
				if (_has_object && _user_object) _user_object->Release();
				_has_object = true; _user_object = object;
				if (_user_object) _user_object->Retain();
			}
			virtual Windows::IMenu * GetSubmenu(void) noexcept override { return _submenu; }
			virtual void SetSubmenu(Windows::IMenu * const & menu) noexcept override { _submenu = menu; }
			virtual int GetID(void) noexcept override { return _id; }
			virtual void SetID(const int & id) noexcept override { _id = id; }
			virtual string GetText(void) noexcept override { try { return _text; } catch (...) { return string(); } }
			virtual void SetText(const string & text) noexcept override { try { _text = text; } catch (...) {} }
			virtual string GetSideText(void) noexcept override { try { return _alt_text; } catch (...) { return string(); } }
			virtual void SetSideText(const string & text) noexcept override { try { _alt_text = text; } catch (...) {} }
			virtual bool IsSeparator(void) noexcept override { return _separator; }
			virtual void SetIsSeparator(const bool & separator) noexcept override { _separator = separator; }
			virtual bool IsEnabled(void) noexcept override { return _enabled; }
			virtual void Enable(const bool & enable) noexcept override { _enabled = enable; }
			virtual bool IsChecked(void) noexcept override { return _checked; }
			virtual void Check(const bool & check) noexcept override { _checked = check; }
		};
		class Menu : public Windows::IMenu
		{
			IMenuService * _system;
			object_array<MenuItem> _items;
			union { void * _user_data; Object * _user_object; };
			bool _has_object;
		public:
			Menu(IMenuService * system) : _system(system), _items(0x20), _user_data(0), _has_object(false) {}
			virtual ~Menu(void) override { if (_has_object && _user_object) _user_object->Release(); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Linux Menu"; ESSE_TRY_OUTRO(string()) }
			virtual void * GetUserData(void) noexcept override { return _user_data; }
			virtual void SetUserData(void * const & data) noexcept override
			{
				if (_has_object && _user_object) _user_object->Release();
				_has_object = false; _user_data = data;
			}
			virtual Object * GetUserObject(void) noexcept override { return _user_object; }
			virtual void SetUserObject(Object * const & object) noexcept override
			{
				if (_has_object && _user_object) _user_object->Release();
				_has_object = true; _user_object = object;
				if (_user_object) _user_object->Retain();
			}
			virtual void AppendMenuItem(Windows::IMenuItem * item) noexcept override { try { if (item) _items.Append(static_cast<MenuItem *>(item)); } catch (...) {} }
			virtual void InsertMenuItem(Windows::IMenuItem * item, int at) noexcept override { try { if (item && at >= 0 && at <= _items.GetLength()) _items.Insert(static_cast<MenuItem *>(item), at); } catch (...) {} }
			virtual void RemoveMenuItem(int at) noexcept override { if (at >= 0 && at < _items.GetLength()) _items.Remove(at); }
			virtual Windows::IMenuItem * ElementAt(int at) noexcept override { return at >= 0 && at < _items.GetLength() ? _items(at) : 0; }
			virtual int GetLength(void) noexcept override { return _items.GetLength(); }
			virtual Windows::IMenuItem * FindMenuItem(int id) noexcept override
			{
				for (auto & i : _items) {
					if (i._id && i._id == id) return &i;
					if (i._submenu) {
						auto si = i._submenu->FindMenuItem(id);
						if (si) return si;
					}
				}
				return 0;
			}
			virtual int Perform(Windows::IWindow * owner, const Index2 & at) noexcept override
			{
				if (_system->IsRunningMenu()) return 0;
				auto theme = _system->GetWindowSystem()->GetSystemTheme();
				if (!theme) return 0;
				oref<Windows::IScreen> screen;
				if (owner) screen = owner->GetScreen(); else {
					auto screens = _system->GetWindowSystem()->EnumerateScreens();
					if (!screens) return 0;
					for (auto & s : *screens) if (s.GetScreenRectangle().IsInside(at)) { screen = &s; break; }
					if (!screen) screen = _system->GetWindowSystem()->GetDefaultScreen();
				}
				if (!screen) return 0;
				auto visuals = static_cast<MenuVisuals *>(_system->GetVisuals(screen, theme));
				if (!visuals) return 0;
				_system->SetIsRunningMenu(true);
				RunningMenuCallback callback(visuals);
				callback._screen_box = screen->GetUserRectangle();
				callback._create_relative_to = Rectangle(at.x, at.y, at.x, at.y);
				callback._root_menu = this;
				callback.OpenPrimaryMenu();
				_system->SetIsRunningMenu(false);
				return callback._result;
			}
			virtual handle GetOSHandle(void) noexcept override { return 0; }
		};
		class MenuService : public IMenuService
		{
			Windows::IWindowSystem * _system;
			oref<MenuVisuals> _visuals;
			bool _has_menu;
		public:
			MenuService(Windows::IWindowSystem * system) : _system(system), _has_menu(false) {}
			virtual ~MenuService(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Linux Menu Service"; ESSE_TRY_OUTRO(string()) }
			virtual oref<Windows::IMenu> CreateMenu(void) noexcept override { try { return oref<Windows::IMenu>::CreateOwned(new Menu(this)); } catch (...) { return 0; } }
			virtual oref<Windows::IMenuItem> CreateMenuItem(void) noexcept override { try { return oref<Windows::IMenuItem>::CreateOwned(new MenuItem(this)); } catch (...) { return 0; } }
			virtual Object * GetVisuals(Windows::IScreen * screen, Windows::ITheme * theme) noexcept override
			{
				if (_visuals) {
					auto scale = screen->GetScaleFactor();
					auto accent = theme->GetColor(Windows::ThemeColor::Accent);
					auto scheme = theme->GetColorScheme();
					if (scale != _visuals->scale || accent != _visuals->accent || scheme != _visuals->scheme) _visuals.Clear();
				}
				if (!_visuals) try { _visuals = owrap(new MenuVisuals(screen, theme)); } catch (...) { return 0; }
				return _visuals;
			}
			virtual bool IsRunningMenu(void) noexcept override { return _has_menu; }
			virtual void SetIsRunningMenu(bool set) noexcept override { _has_menu = set; }
			virtual Windows::IWindowSystem * GetWindowSystem(void) noexcept override { return _system; }
		};
		oref<IMenuService> IMenuService::CreateInstance(Windows::IWindowSystem * system) noexcept { try { return oref<IMenuService>::CreateOwned(new MenuService(system)); } catch (...) { return 0; } }
	}
}