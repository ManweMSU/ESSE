#include <Cor/Cor.h>
#include <Auxilia/Auxilia.h>
#include <Consolatorium/Consolatorium.h>
#include <EngineRuntime.h>
#include <Imagines/Imagines.h>
#include <Graphica/Graphica.h>
#include <Graphica-Linux/Vulkan.h>

#include <Formationes/Formationes.h>

using namespace ESSE;
using namespace ESSE::Picturae;

class HDLR : public ESSE::IO::IConsoleSessionEventHandler
{
public:
	ESSE::IO::Console * con;
	virtual void HandleConsoleSessionEvent(ESSE::IO::ConsoleSessionEvent event) noexcept override
	{
		con->SendEvent(uint(event), 0);
	}
};

void ListRegistry(ESSE::Formationes::RegistryNode * node, ESSE::IO::Console & console, const ESSE::string & prefix)
{
	for (auto & n : node->GetSubnodes()) {
		auto sn = node->OpenNode(n);
		console.WriteLineFormatted(prefix + n + U":");
		ListRegistry(sn, console, prefix + U"  ");
	}
	for (auto & v : node->GetValues()) {
		auto vv = node->GetValueString(v);
		console.WriteLineFormatted(prefix + v + U" = " + vv);
	}
}

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

		auto v4 = ESSE::string(ENGINE_VI_APPIDENT) + U" " + Time::GetCurrentTime().ToString() + U" - Валера \033CFПиздюк\033-- 🥀\n";
		console.WriteLineFormatted(v4);
		console.SetTitle(U"Консоль");

		auto devf = ESSE::Graphica::CreateDeviceFactory();
		auto dev = devf->CreateDefaultDevice();
		auto ctx = dev->GetPrimaryDeviceContext();
		
		Graphica::TextureDesc tdesc;
		Graphica::ResourceInitDesc tinit;
		tdesc.Type = Graphica::TextureType::Type2D;
		tdesc.Format = Graphica::PixelFormat::B8G8R8A8_unorm;
		tdesc.MipmapCount = 1;
		tdesc.Usage = Graphica::ResourceUsageShaderAll | Graphica::ResourceUsageCPUAll | Graphica::ResourceUsageRenderTarget;
		tdesc.MemoryPool = Graphica::ResourceMemoryPool::Regular;
		auto frame1 = DecodePicture(FileStream::Create(U"/home/aemilianus/Pictures/Elaina.png", FileAccess::AccessRead, FileCreationMode::OpenExisting))->Convert(PixelFormat::B8G8R8A8, ScanOrigin::TopLeft);
		auto frame2 = DecodePicture(FileStream::Create(U"/home/aemilianus/Pictures/Elaina - 2.jpg", FileAccess::AccessRead, FileCreationMode::OpenExisting))->Convert(PixelFormat::B8G8R8A8, ScanOrigin::TopLeft);
		tdesc.Width = frame2->GetDesc().width;
		tdesc.Height = frame2->GetDesc().height;
		tinit.Data = frame2->GetDesc().data;
		tinit.DataPitch = frame2->GetDesc().stride;
		auto tex2 = dev->CreateTextureWithData(tdesc, &tinit);
		tdesc.Width = 2000;
		tdesc.Height = 1000;
		tdesc.Usage = Graphica::ResourceUsageShaderAll | Graphica::ResourceUsageCPUAll | Graphica::ResourceUsageRenderTarget;
		tdesc.MemoryPool = Graphica::ResourceMemoryPool::Regular;
		auto tex3 = dev->CreateTexture(tdesc);
		Graphica::RenderTargetViewDesc rtvd;
		rtvd.Texture = tex3;
		rtvd.LoadAction = Graphica::TextureLoadAction::Clear;
		rtvd.ClearValue[0] = 0.5;
		rtvd.ClearValue[1] = 0.0;
		rtvd.ClearValue[2] = 1.0;
		rtvd.ClearValue[3] = 0.5;
		ctx->BeginRenderingPass2D(rtvd);
		auto ctx2d = reinterpret_cast<Graphica::IDeviceContext2D *>(ctx->DynamicCast(Classes.IDeviceContext2D, ectx));
		ErrorThrow(ectx);
		double g[] = { 0.0, 1.0 / 6.0, 2.0 / 6.0, 3.0 / 6.0, 4.0 / 6.0, 5.0 / 6.0, 1.0 };
		Color c[] = { 0xFF0000FF, 0xFF00FFFF, 0xFF00FF00, 0xFFFFFF00, 0xFFFF0000, 0xFFFF00FF, 0xFF0000FF };
		auto b1 = ctx2d->CreateSolidColorBrush(Color(255, 128, 64, 128));
		auto b2 = ctx2d->CreateGradientBrush(Index2(100, 0), Index2(200, 100), c, g, 7);
		ctx2d->Render(b1, Rectangle(0, 100, 100, 300));
		ctx2d->Render(b2, Rectangle(100, 0, 300, 100));
		auto bmp = Graphica::CreateDeviceContextFactory2D()->LoadBitmap(frame1);
		auto b3 = ctx2d->CreateTileBrush(bmp, Rectangle(0, 0, 0x10000, 0x10000));
		auto b4 = ctx2d->CreateTileBrushCopy(b3, Rectangle(500, 500, 800, 800));
		auto b5 = ctx2d->CreateTextureBrush(tex2, Graphica::TextureAlphaMode::Ignore);
		auto blur = ctx2d->CreateBlurEffectBrush(25.0);
		b3->OverrideTileReferenceRectangle(Rectangle(150, 150, 250, 250));
		b4->OverrideTileReferenceRectangle(Rectangle(150, 150, 250, 250));

		ctx2d->Render(b2, Rectangle(100, 100, 400, 400));
		ctx2d->Render(b3, Rectangle(400, 100, 700, 400));
		ctx2d->Render(b4, Rectangle(100, 400, 400, 700));
		ctx2d->Render(b5, Rectangle(400, 400, 700, 700));

		double pp[] = { 400, 700, 400, 100, 100, 400, 700, 400 };
		ctx2d->RenderPolygon(pp, pp + 4, 4, blur);

		auto font = Graphica::CreateDeviceContextFactory2D()->CreateFont(U"", Graphica::CreateFontSystemDefault | Graphica::CreateFontWeight900, 100);
		uint glyphs[6];
		font->GetGlyphsForCharacters(U"PIZDUK", glyphs, 6);
		double px[6] = { 100, 180, 260, 340, 420, 500 };
		double py[6] = { 400, 400, 400, 400, 400, 400 };
		Color clr[6] = { Color(128, 0, 255), Color(128, 0, 255), Color(128, 0, 255), Color(128, 0, 255), Color(128, 0, 255), Color(128, 0, 255) };
		Graphica::IFont * fnt[6] = { font, font, font, font, font, font };
		double m[6] = { 0.5, -0.5, 0.0, 0.5, 0.5, 0.0 };
		auto run = ctx2d->CreateGlyphRun(fnt, glyphs, px, py, clr, 6, m);
		ctx2d->RenderGlyphRun(run, Index2(0, 0));

		ctx->EndCurrentPass();

		PictureDesc pdesc;
		Graphica::ResourceDataDesc pdata;
		pdesc.width = tex3->GetWidth();
		pdesc.height = tex3->GetHeight();
		pdesc.stride = pdata.DataPitch = 4 * pdesc.width;
		pdesc.format = PixelFormat::B8G8R8A8;
		pdesc.alpha_mode = AlphaMode::Straight;
		pdesc.origin = ScanOrigin::TopLeft;
		pdesc.palette_size = 0;
		auto pict = owrap(new Picture(pdesc, PictureInit::AllocateUninitialized));
		pdata.Data = pict->GetDesc().data;
		ctx->BeginMemoryManagementPass();
		ctx->QueryResourceData(pdata, tex3, Index2(0, 0), Index3(0, 0, 0), Index3(tex3->GetWidth(), tex3->GetHeight(), 1));
		ctx->EndCurrentPass();
		Encode(FileStream::Create(U"test3.png", FileAccess::AccessReadWrite, FileCreationMode::CreateAlways), pict, ImageFormatPNG);
		

		ESSE::IO::ConsoleEventDesc ev;
		console.EnableSendEvent();
		console.SetInputMode(ESSE::IO::ConsoleInputMode::Raw);
		console.SetSessionEventMode(0xFFFF, hdlr);
		while (true) {
			if (!console.WaitEvent(ev, 1000)) {
				console.WriteLineFormatted(U"\0330CNO EVENT\033--");
				continue;
			}
			auto cp = console.GetCaretPosition();
			console.WriteLineFormatted(ESSE::FormatString(U"\03309CARET\033--: \033A*%0\033-*, \033E*%1\033-*", cp.x, cp.y));
			if (ev.event == ESSE::IO::ConsoleInputEvent::CharacterInput) {
				console.WriteLineFormatted(ESSE::FormatString(U"\0330FCHAR\033--: %0", ESSE::string(ev.character, 1)));
			} else if (ev.event == ESSE::IO::ConsoleInputEvent::KeyInput) {
				console.WriteLineFormatted(ESSE::FormatString(U"\0330FKEY \033--: %0, %1", ev.virtual_key_code, ev.virtual_key_modifiers));
				if (ev.virtual_key_code == ESSE::VirtualKeyCodes::Escape) {
					console.SetInputMode(ESSE::IO::ConsoleInputMode::Echo);
					return 0;
				}
			} else if (ev.event == ESSE::IO::ConsoleInputEvent::ConsoleResized) {
				console.WriteLineFormatted(ESSE::FormatString(U"\0330FRES \033--: %0 x %1", ev.width, ev.height));
			} else if (ev.event == ESSE::IO::ConsoleInputEvent::CharacterInput) {
				console.WriteLineFormatted(U"\0330FEOS \033--");
				console.SetInputMode(ESSE::IO::ConsoleInputMode::Echo);
				return 0;
			} else if (ev.event == ESSE::IO::ConsoleInputEvent::SendEvent) {
				auto event = static_cast<ESSE::IO::ConsoleSessionEvent>(ev.user1);
				if (event == ESSE::IO::ConsoleSessionEvent::ControlC)
					con->WriteLineFormatted(U"\033E4 Ctrl+C \033--");
				else if (event == ESSE::IO::ConsoleSessionEvent::Quit)
					con->WriteLineFormatted(U"\033E4 Quit \033--");
				else if (event == ESSE::IO::ConsoleSessionEvent::ConsoleClosed)
					con->WriteLineFormatted(U"\033E4 Close Console \033--");
				else if (event == ESSE::IO::ConsoleSessionEvent::Terminate)
					con->WriteLineFormatted(U"\033E4 Terminate \033--");
				console.SetInputMode(ESSE::IO::ConsoleInputMode::Echo);
				return 0;
			}
		}
	} catch (ESSE::Exception & e) {
		console.WriteLine(ESSE::FormatString(U"CRITICAL ERROR: %0, %1", e.GetError().error_code, e.GetError().error_subcode));
	}
	return 0;
}