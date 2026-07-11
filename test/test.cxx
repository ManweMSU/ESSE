#include <Cor/Cor.h>
#include <Auxilia/Auxilia.h>
#include <Consolatorium/Consolatorium.h>
#include <EngineRuntime.h>
#include <Imagines/Imagines.h>
#include <Graphica/Graphica.h>
#include <Graphica-Linux/Vulkan.h>

#include <Formationes/Formationes.h>
#include <Fenestrae/Fenestrae.h>
#include <Auxilia/Auxilia.h>

using namespace ESSE;
using namespace ESSE::Picturae;

class HDLR : public Object, public ESSE::Windows::IApplicationCallback, public ESSE::Windows::IWindowCallback, public ESSE::Windows::IStatusCallback
{
	oref<Graphica::IDeviceContextFactory2D> factory;
	oref<Graphica::IBitmap> bitmap;
	ObjectDictionary<Windows::IWindow *, Graphica::IDeviceContext2D> context;
	ObjectDictionary<Windows::IWindow *, Graphica::IBitmapBrush> brush;
public:
	void wnew(Windows::IWindow * parent, uint style)
	{
		auto ws = Windows::GetWindowSystem();
		Windows::CreateWindowDesc wdesc;
		wdesc.desc_type = Windows::CreateWindowDescType::CreateWindowDesc;
		wdesc.next_desc = 0;
		wdesc.style = Windows::WindowStyleHasTitle | Windows::WindowStyleResizeble | Windows::WindowStyleCloseButton | Windows::WindowStyleHelpButton | Windows::WindowStyleSetBlurBehind | style;
		wdesc.title = U"Валера Пиздюк";
		wdesc.callback = this;
		wdesc.parent_window = parent;
		wdesc.position = Rectangle(100, 100, 1000, 1000);
		wdesc.minimal_constraints = wdesc.maximal_constraints = Index2(0, 0);
		auto window = ws->CreateWindow(&wdesc);
		window->SetVisibility(true);
	}
	ESSE::IO::Console * con;
	virtual bool AcceptsApplicationCommand(Windows::ApplicationCommand command) noexcept override { return true; }
	virtual bool AcceptsWindowCommand(Windows::WindowCommand command) noexcept override { return true; }
	virtual bool HandleApplicationCommand(Windows::ApplicationCommand command, const string & argument) noexcept override
	{
		con->WriteLine(FormatString(U"APP COMMAND: %0, %1", uint(command), argument));
		if (command == Windows::ApplicationCommand::Terminate) Windows::GetWindowSystem()->ExitMainLoop();
		return true;
	}
	virtual void HandleHotKeyEvent(uint event_id) noexcept override
	{
		con->WriteLine(FormatString(U"HOTKEY COMMAND: %0", event_id));
		auto s1 = Windows::GetWindowSystem()->GetKeyboardManager()->IsKeyPressed(VirtualKeyCodes::W);
		auto s2 = Windows::GetWindowSystem()->GetKeyboardManager()->IsKeyPressed(VirtualKeyCodes::CapsLock);
		auto s3 = Windows::GetWindowSystem()->GetKeyboardManager()->IsKeyToggled(VirtualKeyCodes::CapsLock);
		con->WriteLine(FormatString(U"KB STATUS: %0, %1, %2", s1, s2, s3));
	}
	virtual void Created(Windows::IWindow * window) noexcept
	{
		con->WriteLine(U"WINDOW CREATED");
		if (!factory) factory = Graphica::CreateDeviceContextFactory2D();
		if (!bitmap) {
			auto path = IO::Path::GetDirectory(IO::GetExecutablePath()) + U"/../../saryu.png";
			auto image = DecodePicture(FileStream::Create(path, FileAccess::AccessRead, FileCreationMode::OpenExisting));
			bitmap = factory->LoadBitmap(image);
		}
		auto ctx = factory->CreatePresentationContext(window, 0);
		auto brs = ctx->CreateBitmapBrush(bitmap, Rectangle(0, 0, bitmap->GetWidth(), bitmap->GetHeight()));
		context.Append(window, ctx);
		brush.Append(window, brs);
	}
	virtual void Destroyed(Windows::IWindow * window) noexcept
	{
		con->WriteLine(U"WINDOW DESTROYED");
		context.Remove(window);
		brush.Remove(window);
	}
	virtual void Shown(Windows::IWindow * window, bool show) noexcept { con->WriteLine(U"WINDOW SHOWN: " + string(show)); }
	virtual void RenderWindow(Windows::IWindow * window) noexcept
	{
		con->WriteLine(U"WINDOW RENDER");
		auto ctx = context[window];
		auto brs = brush[window];
		auto size = window->GetClientSize();
		ctx->BeginRendering(Graphica::TextureLoadAction::Clear, Color(128, 0, 255, 128));
		double ai = double(bitmap->GetWidth()) / double(bitmap->GetHeight());
		double as = double(size.x) / double(size.y);
		int cx = size.x / 2;
		int cy = size.y / 2;
		if (ai > as) {
			int w = size.x;
			int h = w / ai;
			ctx->Render(brs, Rectangle(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2));
		} else {
			int h = size.y;
			int w = h * ai;
			ctx->Render(brs, Rectangle(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2));
		}
		ctx->EndRendering();
	}
	virtual void WindowClosed(Windows::IWindow * window) noexcept
	{
		con->WriteLine(U"WINDOW CLOSE");
		window->Destroy();
	}
	virtual void WindowMaximized(Windows::IWindow * window) noexcept { con->WriteLine(U"WINDOW MAXIMIZE"); }
	virtual void WindowMinimized(Windows::IWindow * window) noexcept { con->WriteLine(U"WINDOW MINIMIZE"); }
	virtual void WindowRestored(Windows::IWindow * window) noexcept { con->WriteLine(U"WINDOW RESTORE"); }
	virtual void WindowHelpRequired(Windows::IWindow * window) noexcept { con->WriteLine(U"WINDOW HELP"); }
	virtual void WindowActivated(Windows::IWindow * window) noexcept { con->WriteLine(U"WINDOW ACTIVATE"); }
	virtual void WindowDeactivated(Windows::IWindow * window) noexcept { con->WriteLine(U"WINDOW DEACTIVATE"); }
	virtual void WindowMoved(Windows::IWindow * window) noexcept { con->WriteLine(U"WINDOW MOVED"); }
	virtual void WindowResized(Windows::IWindow * window) noexcept { con->WriteLine(U"WINDOW RESIZED"); window->Invalidate(); }
	virtual void FocusChanged(Windows::IWindow * window, bool got) noexcept { con->WriteLine(U"WINDOW FOCUS: " + string(got)); }
	virtual bool KeyIsDown(Windows::IWindow * window, uint vkc, uint vkm) noexcept
	{
		con->WriteLine(U"WINDOW KEY DOWN: " + string(vkc) + U", " + string(vkm));
		auto s1 = Windows::GetWindowSystem()->GetKeyboardManager()->IsKeyPressed(VirtualKeyCodes::W);
		auto s2 = Windows::GetWindowSystem()->GetKeyboardManager()->IsKeyPressed(VirtualKeyCodes::CapsLock);
		auto s3 = Windows::GetWindowSystem()->GetKeyboardManager()->IsKeyToggled(VirtualKeyCodes::CapsLock);
		con->WriteLine(FormatString(U"KB STATUS: %0, %1, %2", s1, s2, s3));
		if (vkc == VirtualKeyCodes::Q) wnew(0, 0);
		if (vkc == VirtualKeyCodes::W) wnew(0, Windows::WindowStyleModal);
		if (vkc == VirtualKeyCodes::A) wnew(window, 0);
		if (vkc == VirtualKeyCodes::S) wnew(window, Windows::WindowStyleModal);
		return false;
	}
	virtual void KeyIsUp(Windows::IWindow * window, uint vkc, uint vkm) noexcept { con->WriteLine(U"WINDOW KEY UP: " + string(vkc) + U", " + string(vkm)); }
	virtual void CharacterIsDown(Windows::IWindow * window, unichar32 ucs) noexcept { con->WriteLine(U"WINDOW CHAR DOWN: " + string(ucs)); }
	virtual void MouseEntered(Windows::IWindow * window, uint button_state) noexcept { con->WriteLine(U"WINDOW MOUSE ENTERED: " + string(button_state)); }
	virtual void MouseLeft(Windows::IWindow * window, uint button_state) noexcept { con->WriteLine(U"WINDOW MOUSE LEAVED: " + string(button_state)); }
	virtual void MouseMoved(Windows::IWindow * window, const Index2 & at, uint button_state) noexcept { con->WriteLine(U"WINDOW MOUSE MOVED: " + string(at) + U", " + string(button_state)); }
	virtual void LeftButtonIsDown(Windows::IWindow * window, const Index2 & at, bool double_click) noexcept { con->WriteLine(U"WINDOW MOUSE LEFT DOWN: " + string(at) + U", " + string(double_click)); }
	virtual void LeftButtonIsUp(Windows::IWindow * window, const Index2 & at) noexcept { con->WriteLine(U"WINDOW MOUSE LEFT UP: " + string(at)); }
	virtual void RightButtonIsDown(Windows::IWindow * window, const Index2 & at, bool double_click) noexcept { con->WriteLine(U"WINDOW MOUSE RIGHT DOWN: " + string(at) + U", " + string(double_click)); }
	virtual void RightButtonIsUp(Windows::IWindow * window, const Index2 & at) noexcept { con->WriteLine(U"WINDOW MOUSE RIGHT UP: " + string(at)); }
	virtual void ScrollVertically(Windows::IWindow * window, const Index2 & at, double delta) noexcept { con->WriteLine(U"WINDOW VSCROLL: " + string(at) + U", " + string(delta)); }
	virtual void ScrollHorizontally(Windows::IWindow * window, const Index2 & at, double delta) noexcept { con->WriteLine(U"WINDOW HSCROLL: " + string(at) + U", " + string(delta)); }
	virtual void Timer(Windows::IWindow * window, int timer_id) noexcept { con->WriteLine(U"WINDOW TIMER: " + string(timer_id)); }
	virtual void ThemeChanged(Windows::IWindow * window) noexcept { con->WriteLine(U"WINDOW THEME CHANGE"); }
	virtual bool IsWindowCommandEnabled(Windows::IWindow * window, Windows::WindowCommand command) noexcept { con->WriteLine(U"WINDOW COMMAND TEST: " + string(uint(command))); return true; }
	virtual void HandleWindowCommand(Windows::IWindow * window, Windows::WindowCommand command) noexcept { con->WriteLine(U"WINDOW COMMAND: " + string(uint(command))); }
	virtual void HandleStatusIconCommand(Windows::IStatusBarIcon * icon, int id) noexcept { con->WriteLine(U"STATUS COMMAND: " + string(uint(id))); Windows::GetWindowSystem()->EnumerateTopLevelWindows()->ElementAt(0)->RequireAttention(); }
};

int Main(void)
{
	auto con = ESSE::IO::CreateConsole();
	auto & console = *con;
	ESSE::ErrorContext ectx;
	ESSE::ErrorClear(ectx);
	try {
		Engine::SystemDesc desc;
		Engine::GetSystemInformation(desc);
		auto hdlr = ESSE::owrap(new HDLR);
		hdlr->con = con;
		auto ws = Windows::CreateWindowSystem();
		ws->SetCallback(hdlr);
		auto s4 = ws->GetKeyboardManager()->RegisterHotKey(666, VirtualKeyCodes::Q, VirtualKeyModifiers::Control);
		console.WriteLine(s4);

		auto capt = ws->GetDefaultScreen()->Capture();
		if (capt) Encode(FileStream::Create(U"capture.png", FileAccess::AccessReadWrite, FileCreationMode::CreateAlways), capt, ImageFormatPNG);

		Windows::ClipboardDataDesc cdesc;
		cdesc.format_mask = Windows::ClipboardDataFormatText | Windows::ClipboardDataFormatFiles;
		cdesc.text = U"VALERA";
		cdesc.files = owrap(new array<string>(1));
		cdesc.files->Append(IO::GetExecutablePath());

		const void * picon;
		uintptr licon;
		QueryResource(&picon, &licon, "1", "ICON");
		auto sicon = ws->CreateStatusBarIcon();
		auto iicon = DecodeImage(StaticMemoryStream::Create(picon, licon));
		sicon->SetEventID(777);
		sicon->SetCallback(hdlr);
		sicon->SetIcon(iicon);
		sicon->SetIconColorUsage(Windows::StatusBarIconColorUsage::Monochromic);
		sicon->SetTooltip(U"VALERA");
		sicon->PresentIcon(true);

		ws->PushUserNotification("VALERA", "PIZDUK", iicon);
		ws->Beep();
		hdlr->wnew(0, 0);

		// auto devf = ESSE::Graphica::CreateDeviceFactory();
		// auto dev = devf->CreateDefaultDevice();
		// auto ctx = dev->GetPrimaryDeviceContext();
		// Graphica::TextureDesc tdesc;
		// Graphica::ResourceInitDesc tinit;
		// tdesc.Type = Graphica::TextureType::Type2D;
		// tdesc.Format = Graphica::PixelFormat::B8G8R8A8_unorm;
		// tdesc.MipmapCount = 1;
		// tdesc.Usage = Graphica::ResourceUsageShaderAll | Graphica::ResourceUsageCPUAll | Graphica::ResourceUsageRenderTarget;
		// tdesc.MemoryPool = Graphica::ResourceMemoryPool::Regular;
		// auto frame1 = DecodePicture(FileStream::Create(U"/home/aemilianus/Pictures/Elaina.png", FileAccess::AccessRead, FileCreationMode::OpenExisting))->Convert(PixelFormat::B8G8R8A8, ScanOrigin::TopLeft);
		// auto frame2 = DecodePicture(FileStream::Create(U"/home/aemilianus/Pictures/Elaina - 2.jpg", FileAccess::AccessRead, FileCreationMode::OpenExisting))->Convert(PixelFormat::B8G8R8A8, ScanOrigin::TopLeft);
		// tdesc.Width = frame2->GetDesc().width;
		// tdesc.Height = frame2->GetDesc().height;
		// tinit.Data = frame2->GetDesc().data;
		// tinit.DataPitch = frame2->GetDesc().stride;
		// auto tex2 = dev->CreateTextureWithData(tdesc, &tinit);
		// tdesc.Width = 2000;
		// tdesc.Height = 1000;
		// tdesc.Usage = Graphica::ResourceUsageShaderAll | Graphica::ResourceUsageCPUAll | Graphica::ResourceUsageRenderTarget;
		// tdesc.MemoryPool = Graphica::ResourceMemoryPool::Regular;
		// auto tex3 = dev->CreateTexture(tdesc);
		// Graphica::RenderTargetViewDesc rtvd;
		// rtvd.Texture = tex3;
		// rtvd.LoadAction = Graphica::TextureLoadAction::Clear;
		// rtvd.ClearValue[0] = 0.5;
		// rtvd.ClearValue[1] = 0.0;
		// rtvd.ClearValue[2] = 1.0;
		// rtvd.ClearValue[3] = 0.5;
		// ctx->BeginRenderingPass2D(rtvd);
		// auto ctx2d = reinterpret_cast<Graphica::IDeviceContext2D *>(ctx->DynamicCast(Classes.IDeviceContext2D, ectx));
		// ErrorThrow(ectx);
		// double g[] = { 0.0, 1.0 / 6.0, 2.0 / 6.0, 3.0 / 6.0, 4.0 / 6.0, 5.0 / 6.0, 1.0 };
		// Color c[] = { 0xFF0000FF, 0xFF00FFFF, 0xFF00FF00, 0xFFFFFF00, 0xFFFF0000, 0xFFFF00FF, 0xFF0000FF };
		// auto b1 = ctx2d->CreateSolidColorBrush(Color(255, 128, 64, 128));
		// auto b2 = ctx2d->CreateGradientBrush(Index2(100, 0), Index2(200, 100), c, g, 7);
		// ctx2d->Render(b1, Rectangle(0, 100, 100, 300));
		// ctx2d->Render(b2, Rectangle(100, 0, 300, 100));
		// auto bmp = Graphica::CreateDeviceContextFactory2D()->LoadBitmap(frame1);
		// auto b3 = ctx2d->CreateTileBrush(bmp, Rectangle(0, 0, 0x10000, 0x10000));
		// auto b4 = ctx2d->CreateTileBrushCopy(b3, Rectangle(500, 500, 800, 800));
		// auto b5 = ctx2d->CreateTextureBrush(tex2, Graphica::TextureAlphaMode::Ignore);
		// auto blur = ctx2d->CreateBlurEffectBrush(25.0);
		// b3->OverrideTileReferenceRectangle(Rectangle(150, 150, 250, 250));
		// b4->OverrideTileReferenceRectangle(Rectangle(150, 150, 250, 250));
		// ctx2d->Render(b2, Rectangle(100, 100, 400, 400));
		// ctx2d->Render(b3, Rectangle(400, 100, 700, 400));
		// ctx2d->Render(b4, Rectangle(100, 400, 400, 700));
		// ctx2d->Render(b5, Rectangle(400, 400, 700, 700));
		// double pp[] = { 400, 700, 400, 100, 100, 400, 700, 400 };
		// ctx2d->RenderPolygon(pp, pp + 4, 4, blur);
		// auto font = Graphica::CreateDeviceContextFactory2D()->CreateFont(U"", Graphica::CreateFontSystemDefault | Graphica::CreateFontWeight900, 100);
		// uint glyphs[6];
		// font->GetGlyphsForCharacters(U"PIZDUK", glyphs, 6);
		// double px[6] = { 100, 180, 260, 340, 420, 500 };
		// double py[6] = { 400, 400, 400, 400, 400, 400 };
		// Color clr[6] = { Color(128, 0, 255), Color(128, 0, 255), Color(128, 0, 255), Color(128, 0, 255), Color(128, 0, 255), Color(128, 0, 255) };
		// Graphica::IFont * fnt[6] = { font, font, font, font, font, font };
		// double m[6] = { 0.5, -0.5, 0.0, 0.5, 0.5, 0.0 };
		// auto run = ctx2d->CreateGlyphRun(fnt, glyphs, px, py, clr, 6, m);
		// ctx2d->RenderGlyphRun(run, Index2(0, 0));
		// ctx->EndCurrentPass();
		// PictureDesc pdesc;
		// Graphica::ResourceDataDesc pdata;
		// pdesc.width = tex3->GetWidth();
		// pdesc.height = tex3->GetHeight();
		// pdesc.stride = pdata.DataPitch = 4 * pdesc.width;
		// pdesc.format = PixelFormat::B8G8R8A8;
		// pdesc.alpha_mode = AlphaMode::Straight;
		// pdesc.origin = ScanOrigin::TopLeft;
		// pdesc.palette_size = 0;
		// auto pict = owrap(new Picture(pdesc, PictureInit::AllocateUninitialized));
		// pdata.Data = pict->GetDesc().data;
		// ctx->BeginMemoryManagementPass();
		// ctx->QueryResourceData(pdata, tex3, Index2(0, 0), Index3(0, 0, 0), Index3(tex3->GetWidth(), tex3->GetHeight(), 1));
		// ctx->EndCurrentPass();
		// Encode(FileStream::Create(U"test3.png", FileAccess::AccessReadWrite, FileCreationMode::CreateAlways), pict, ImageFormatPNG);
		
		ws->RunMainLoop(true);
		console.WriteLine(U"EXITING");
	} catch (ESSE::Exception & e) {
		console.WriteLine(ESSE::FormatString(U"CRITICAL ERROR: %0, %1", e.GetError().error_code, e.GetError().error_subcode));
	}
	return 0;
}