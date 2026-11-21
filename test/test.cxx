#include <Cor/Cor.h>
#include <Auxilia/Auxilia.h>
#include <Consolatorium/Consolatorium.h>
#include <Imagines/Imagines.h>
#include <Graphica/Graphica.h>
#include <Graphica-Linux/Vulkan.h>

#include <Formationes/Formationes.h>
#include <Fenestrae/Fenestrae.h>
#include <Auxilia/Auxilia.h>

#include <unistd.h>

using namespace ESSE;
using namespace ESSE::Picturae;

class HDLR : public Object, public ESSE::Windows::IApplicationCallback, public ESSE::Windows::IWindowCallback, public ESSE::Windows::IStatusCallback, public ESSE::Windows::IDragCallback
{
	oref<Graphica::IDeviceContextFactory2D> factory;
	oref<Graphica::IBitmap> bitmap;
	oref<Graphica::IDeviceContext2D> context;
	oref<Graphica::IBitmapBrush> brush;
	oref<Graphica::AggregateFont> font;
	oref<Graphica::Typesetter> ts;
public:
	void wnew(Windows::IWindow * parent, uint style)
	{
		auto subhdlr = new HDLR;
		auto ws = Windows::GetWindowSystem();
		subhdlr->con = con;
		Windows::CreateWindowDesc wdesc;
		wdesc.desc_type = Windows::CreateWindowDescType::CreateWindowDesc;
		wdesc.next_desc = 0;
		wdesc.style = Windows::WindowStyleHasTitle | Windows::WindowStyleResizeble | Windows::WindowStyleCloseButton | Windows::WindowStyleHelpButton | Windows::WindowStyleSetBlurBehind | style;
		wdesc.title = U"Валера Пиздюк";
		wdesc.callback = subhdlr;
		wdesc.parent_window = parent;
		wdesc.position = Rectangle(200, 200, 1000, 1000);
		wdesc.minimal_constraints = Index2(400, 400);
		wdesc.maximal_constraints = Index2(0, 0);
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
		auto f2 = Graphica::CreateDeviceFactory();
		auto dev = f2->CreateDefaultDevice();
		context = factory->CreatePresentationContext(window, dev);
		if (!context) {
			con->WriteLine(U"FAILED TO CREATE A HARDWARE CONTEXT, FALLING BACK TO SOFTWARE");
			context = factory->CreatePresentationContext(window, 0);
			if (!context) {
				con->WriteLine(U"THE SOFTWARE CONTEXT CREATION FAILED TOO");
				ExitProcess(0);
			}
		}
		try {
			font = owrap(new Graphica::AggregateFont(factory, U"", Graphica::CreateFontSystemDefault | Graphica::CreateFontWeight700, 100));
			ts = owrap(new Graphica::Typesetter(font, U"Валера пиздюк 🩶❤️ 🌹", Graphica::TypesetterFlags::HorizontalAlignmentCenter | Graphica::TypesetterFlags::VerticalAlignmentCenter | Graphica::TypesetterFlags::Underlined, Color(255, 255, 255)));
		} catch (...) {}
	}
	virtual void Destroyed(Windows::IWindow * window) noexcept
	{
		con->WriteLine(U"WINDOW DESTROYED");
		Release();
	}
	virtual void Shown(Windows::IWindow * window, bool show) noexcept { con->WriteLine(U"WINDOW SHOWN: " + string(show)); }
	virtual void RenderWindow(Windows::IWindow * window) noexcept
	{
		con->WriteLine(U"WINDOW RENDER");
		if (!brush) {
			brush = context->CreateBitmapBrush(bitmap, Rectangle(0, 0, bitmap->GetWidth(), bitmap->GetHeight()));
		}
		auto size = window->GetClientSize();
		context->BeginRendering(Graphica::TextureLoadAction::Clear, Color(128, 0, 255, 64));
		double ai = double(bitmap->GetWidth()) / double(bitmap->GetHeight());
		double as = double(size.x) / double(size.y);
		int cx = size.x / 2;
		int cy = size.y / 2;
		if (ai > as) {
			int w = size.x;
			int h = w / ai;
			context->Render(brush, Rectangle(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2));
		} else {
			int h = size.y;
			int w = h * ai;
			context->Render(brush, Rectangle(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2));
		}
		if (ts) ts->Render(context, 0, Rectangle(0, 0, size.x, size.y));
		context->EndRendering();
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
	virtual void WindowMoved(Windows::IWindow * window) noexcept
	{
		auto rect = window->GetPosition();
		con->WriteLine(FormatString(U"WINDOW MOVED: %0, %1, %2, %3", rect.left, rect.top, rect.right, rect.bottom));
	}
	virtual void WindowResized(Windows::IWindow * window) noexcept { con->WriteLine(U"WINDOW RESIZED: " + string(window->GetClientSize())); window->Invalidate(); }
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
		if (vkc == VirtualKeyCodes::R) {
			Windows::ClipboardDataDesc cdesc;
			Windows::GetWindowSystem()->GetClipboardManager()->ReadClipboard(0xFFFF, cdesc);
			if (cdesc.format_mask & Windows::ClipboardDataFormatText) {
				con->WriteLine(U"Text on the clipboard: " + cdesc.text);
			} else if (cdesc.format_mask & Windows::ClipboardDataFormatFiles) {
				con->WriteLine(U"Files on the clipboard:");
				for (auto & f : *cdesc.files) con->WriteLine(f);
			} else if (cdesc.format_mask & Windows::ClipboardDataFormatImage) {
				con->WriteLine(U"Image on the clipboard (see to the window)");
				bitmap = factory->LoadBitmap(cdesc.image);
				brush.Clear();
				window->Invalidate();
			} else if (cdesc.format_mask & Windows::ClipboardDataFormatData) {
				for (auto & f : cdesc.data) {
					con->WriteLine(U"Data on the clipboard:");
					con->WriteLine(f.key);
					con->WriteLine(HexStringFromData(f.value, 0, true));
				}
			}
		} else if (vkc == VirtualKeyCodes::T) {
			Windows::ClipboardDataDesc cdesc;
			cdesc.format_mask = Windows::ClipboardDataFormatText;
			cdesc.text = U"Валера пиздюк";
			Windows::GetWindowSystem()->GetClipboardManager()->WriteClipboard(cdesc);
		} else if (vkc == VirtualKeyCodes::Y) {
			Windows::ClipboardDataDesc cdesc;
			cdesc.format_mask = Windows::ClipboardDataFormatImage;
			cdesc.image = bitmap->QueryContents();
			Windows::GetWindowSystem()->GetClipboardManager()->WriteClipboard(cdesc);
		} else if (vkc == VirtualKeyCodes::U) {
			Windows::ClipboardDataDesc cdesc;
			cdesc.format_mask = Windows::ClipboardDataFormatFiles;
			cdesc.files = owrap(new array<string>(1));
			cdesc.files->Append(IO::GetExecutablePath());
			Windows::GetWindowSystem()->GetClipboardManager()->WriteClipboard(cdesc);
		} else if (vkc == VirtualKeyCodes::I) {
			uint word1 = GetThisProcessPID();
			uint word2 = 666;
			auto data1 = owrap(new DataBlock(1));
			auto data2 = owrap(new DataBlock(1));
			data1->Append(reinterpret_cast<const uint8 *>(&word1), 4);
			data2->Append(reinterpret_cast<const uint8 *>(&word2), 4);
			Windows::ClipboardDataDesc cdesc;
			cdesc.format_mask = Windows::ClipboardDataFormatData;
			cdesc.data.Append(U"Custom.Format.1", data1);
			cdesc.data.Append(U"Custom.Format.2", data2);
			Windows::GetWindowSystem()->GetClipboardManager()->WriteClipboard(cdesc);
		} else if (vkc == VirtualKeyCodes::M) {
			Windows::GetWindowSystem()->SetCursorPosition(window->ConvertClientToGlobal(window->GetClientSize() / Index2(2, 2)));
		} else if (vkc == VirtualKeyCodes::N) {
			window->SetPosition(Rectangle(200, 200, 800, 800));
		}
		return false;
	}
	virtual void KeyIsUp(Windows::IWindow * window, uint vkc, uint vkm) noexcept { con->WriteLine(U"WINDOW KEY UP: " + string(vkc) + U", " + string(vkm)); }
	virtual void CharacterIsDown(Windows::IWindow * window, unichar32 ucs) noexcept { con->WriteLine(U"WINDOW CHAR DOWN: " + string(ucs)); }
	virtual void MouseEntered(Windows::IWindow * window, uint button_state) noexcept { con->WriteLine(U"WINDOW MOUSE ENTERED: " + string(button_state)); }
	virtual void MouseLeft(Windows::IWindow * window, uint button_state) noexcept { con->WriteLine(U"WINDOW MOUSE LEAVED: " + string(button_state)); }
	virtual void MouseMoved(Windows::IWindow * window, const Index2 & at, uint button_state) noexcept { con->WriteLine(U"WINDOW MOUSE MOVED: " + string(at) + U", " + string(button_state)); }
	virtual void LeftButtonIsDown(Windows::IWindow * window, const Index2 & at, bool double_click) noexcept {
		con->WriteLine(U"WINDOW MOUSE LEFT DOWN: " + string(at) + U", " + string(double_click));
		Windows::DragDataDesc cdesc;
		auto img = bitmap->QueryContents();
		cdesc.data.format_mask = Windows::ClipboardDataFormatText | Windows::ClipboardDataFormatImage | Windows::ClipboardDataFormatFiles;
		cdesc.data.text = U"Валера пиздюк";
		cdesc.data.image = bitmap->QueryContents();
		cdesc.data.files = owrap(new array<string>(0x10));
		cdesc.data.files->Append(IO::GetExecutablePath());
		cdesc.actions = Windows::DragActionCopy | Windows::DragActionMove;
		cdesc.icon = img;
		cdesc.user_data = reinterpret_cast<handle>(uintptr(666));
		con->WriteLine(U"DRAG INIT: " + string(window->LaunchDragSession(cdesc, this)));
	}
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
	virtual void DragStarted(Windows::IWindow * origin, const Windows::DragDataDesc & desc) noexcept
	{
		con->WriteLine(U"DRAG BEGIN");
	}
	virtual void DragFinished(uint drag_action) noexcept
	{
		con->WriteLine(U"DRAG END: " + string(uint(drag_action)));
	}
	virtual uint DragEntered(Windows::IWindow * window, Windows::IDragSession * drag, const Index2 & at) noexcept
	{
		con->WriteLine(FormatString(U"DRAG ENTER: (%2, %3); %0; %1", drag->GetActionsSupported(), drag->ProbeDataFormats(0xFFFF), at.x, at.y));
		return Windows::DragActionCopy;
	}
	virtual uint DragMoved(Windows::IWindow * window, Windows::IDragSession * drag, const Index2 & at) noexcept
	{
		con->WriteLine(FormatString(U"DRAG MOVE: (%2, %3); %0; %1", drag->GetActionsSupported(), drag->ProbeDataFormats(0xFFFF), at.x, at.y));
		return Windows::DragActionCopy;
	}
	virtual uint DragDropped(Windows::IWindow * window, Windows::IDragSession * drag, const Index2 & at) noexcept
	{
		con->WriteLine(FormatString(U"DRAG DROP: (%2, %3); %0; %1", drag->GetActionsSupported(), drag->ProbeDataFormats(0xFFFF), at.x, at.y));
		Windows::ClipboardDataDesc cdesc;
		drag->ReadData(0xFFFF, cdesc);
		if (cdesc.format_mask & Windows::ClipboardDataFormatText) {
			con->WriteLine(U"Text on the clipboard: " + cdesc.text);
		}
		if (cdesc.format_mask & Windows::ClipboardDataFormatFiles) {
			con->WriteLine(U"Files on the clipboard:");
			for (auto & f : *cdesc.files) con->WriteLine(f);
		}
		if (cdesc.format_mask & Windows::ClipboardDataFormatImage) {
			con->WriteLine(U"Image on the clipboard (see to the window)");
			bitmap = factory->LoadBitmap(cdesc.image);
			brush.Clear();
			window->Invalidate();
		}
		if (cdesc.format_mask & Windows::ClipboardDataFormatData) {
			for (auto & f : cdesc.data) {
				con->WriteLine(U"Data on the clipboard:");
				con->WriteLine(f.key);
				con->WriteLine(HexStringFromData(f.value, 0, true));
			}
		}
		return Windows::DragActionCopy;
	}
	virtual void DragLeft(Windows::IWindow * window) noexcept
	{
		con->WriteLine(U"DRAG LEAVE");
	}
};

oref<Windows::IMenuItem> CreateTestMenuItem(int i, int j)
{
	auto ws = Windows::GetWindowSystem();
	auto mi = ws->CreateMenuItem();
	auto id = 100 * i + j;
	mi->SetText(FormatString(U"Элемент меню %0.%1", i, j));
	mi->SetSideText(id);
	mi->SetID(id);
	mi->Check(j % 2);
	mi->Enable(j % 3);
	return mi;
}
oref<Windows::IMenuItem> CreateTestMenuSeparator(void)
{
	auto ws = Windows::GetWindowSystem();
	auto mi = ws->CreateMenuItem();
	mi->SetIsSeparator(true);
	return mi;
}
oref<Windows::IMenu> CreateTestMenu(void)
{
	auto ws = Windows::GetWindowSystem();
	auto mi11 = CreateTestMenuItem(1, 1);
	auto mi12 = CreateTestMenuItem(1, 2);
	auto mi13 = CreateTestMenuItem(1, 3);
	auto mi14 = CreateTestMenuItem(1, 4);
	auto mi15 = CreateTestMenuItem(1, 5);
	auto mi21 = CreateTestMenuItem(2, 1);
	auto mi22 = CreateTestMenuItem(2, 2);
	auto mi23 = CreateTestMenuItem(2, 3);
	auto mi24 = CreateTestMenuItem(2, 4);
	auto mi31 = CreateTestMenuItem(3, 1);
	auto mi32 = CreateTestMenuItem(3, 2);
	auto mi33 = CreateTestMenuItem(3, 3);
	auto mi34 = CreateTestMenuItem(3, 4);
	auto mi41 = CreateTestMenuItem(4, 1);
	auto mi42 = CreateTestMenuItem(4, 2);
	auto mi43 = CreateTestMenuItem(4, 3);
	auto mi44 = CreateTestMenuItem(4, 4);
	auto s1 = CreateTestMenuSeparator();
	auto s2 = CreateTestMenuSeparator();
	auto m1 = ws->CreateMenu();
	auto m2 = ws->CreateMenu();
	auto m3 = ws->CreateMenu();
	auto m4 = ws->CreateMenu();
	m1->AppendMenuItem(mi11);
	m1->AppendMenuItem(mi12);
	m1->AppendMenuItem(s1);
	m1->AppendMenuItem(mi13);
	m1->AppendMenuItem(mi14);
	m1->AppendMenuItem(s2);
	m1->AppendMenuItem(mi15);
	m2->AppendMenuItem(mi21);
	m2->AppendMenuItem(mi22);
	m2->AppendMenuItem(mi23);
	m2->AppendMenuItem(mi24);
	m3->AppendMenuItem(mi31);
	m3->AppendMenuItem(mi32);
	m3->AppendMenuItem(mi33);
	m3->AppendMenuItem(mi34);
	m4->AppendMenuItem(mi41);
	m4->AppendMenuItem(mi42);
	m4->AppendMenuItem(mi43);
	m4->AppendMenuItem(mi44);
	mi11->SetSubmenu(m2);
	mi14->SetSubmenu(m3);
	mi22->SetSubmenu(m4);
	return m1;
}

int Main(void)
{
	auto con = ESSE::IO::CreateConsole();
	auto & console = *con;

	try {
		auto hdlr = ESSE::owrap(new HDLR);
		hdlr->con = con;
		auto ws = Windows::CreateWindowSystem();
		ws->SetCallback(hdlr);
		auto s4 = ws->GetKeyboardManager()->RegisterHotKey(666, VirtualKeyCodes::E, VirtualKeyModifiers::Control);
		console.WriteLine(s4);

		auto capt = ws->GetDefaultScreen()->Capture();
		if (capt) Encode(FileStream::Create(U"capture.png", FileAccess::AccessReadWrite, FileCreationMode::CreateAlways), capt, ImageFormatPNG);

		const void * picon;
		uintptr licon;
		QueryResource(&picon, &licon, "1", "ICON");
		auto iicon = DecodeImage(StaticMemoryStream::Create(picon, licon));
		auto sicon = ws->CreateStatusBarIcon();
		sicon->SetEventID(777);
		sicon->SetMenu(CreateTestMenu());
		sicon->SetCallback(hdlr);
		sicon->SetIcon(iicon);
		sicon->SetIconColorUsage(Windows::StatusBarIconColorUsage::Monochromic);
		sicon->SetTooltip(U"VALERA");
		sicon->PresentIcon(true);

		ws->PushUserNotification("VALERA", "PIZDUK", iicon);
		ws->LaunchIPCServer(U"test", U"EngineSoftware");
		ws->Beep();
		hdlr->wnew(0, 0);

		ws->RunMainLoop(true);
		console.WriteLine(U"EXITING");
	} catch (ESSE::Exception & e) {
		console.WriteLine(ESSE::FormatString(U"CRITICAL ERROR: %0, %1", e.GetError().error_code, e.GetError().error_subcode));
	}
	return 0;
}