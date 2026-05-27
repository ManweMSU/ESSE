#include <Cor/Cor.h>
#include <Auxilia/Auxilia.h>
#include <Consolatorium/Consolatorium.h>
#include <EngineRuntime.h>
#include <Imagines/Imagines.h>

using namespace Engine;

class HDLR : public ESSE::IO::IConsoleSessionEventHandler
{
public:
	ESSE::IO::Console * con;
	virtual void HandleConsoleSessionEvent(ESSE::IO::ConsoleSessionEvent event) noexcept override
	{
		con->SendEvent(uint(event), 0);
	}
};

int Main(void)
{
	auto con = ESSE::IO::CreateConsole();
	auto & console = *con;
	ESSE::ErrorContext ectx;
	ESSE::ErrorClear(ectx);
	try {
		SystemDesc desc;
		GetSystemInformation(desc);
		auto hdlr = ESSE::owrap(new HDLR);
		hdlr->con = con;

		auto v4 = ESSE::string(ENGINE_VI_APPIDENT) + U" " + static_cast<const widechar *>(Time::GetCurrentTime().ToLocal().ToString()) + U" - Валера \033CFПиздюк\033-- 🥀\n";
		console.WriteLineFormatted(v4);
		console.SetTitle(U"Консоль");
		console.EnableSendEvent();
		console.SetSessionEventMode(0xFFFF, hdlr);

		ESSE::IO::ConsoleEventDesc ev;
		console.SetInputMode(ESSE::IO::ConsoleInputMode::Raw);
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

		console.Write(U"MODE: ");
		auto mode = console.ReadLine();
		if (mode.GetLength()) {
			ESSE::IPC::PurifyConnectionListener(U"pizduk");
			auto ipc_l = ESSE::IPC::CreateConnectionListener(U"pizduk", 0);
			auto ipc = ipc_l->Accept();
			while (true) {
				auto l = console.ReadLine();
				if (!l.GetLength()) break;
				else if (l == U"f") {
					auto file = ESSE::IO::CreateFile(U"test.txt", ESSE::FileAccess::AccessWrite, ESSE::FileCreationMode::CreateAlways);
					ipc->SendData("\33", 1);
					ipc->SendHandle(file);
					ESSE::IO::CloseHandle(file);
				} else {
					auto data = ESSE::EncodeString(l + U"\n", ESSE::Unicode::Encoding::UTF8, false);
					ipc->SendData(data->GetBuffer(), data->GetLength());
				}
			}
		} else {
			auto ipc = ESSE::IPC::Connect(U"pizduk", 0);
			while (true) {
				char chr;
				auto num = ipc->ReceiveData(&chr, 1);
				if (!num) {
					console.WriteLine(U"END-OF-STREAM");
					break;
				} else if (chr != '\33') {
					console.Write(ESSE::ucs1_string(&chr, 1));
				} else {
					console.WriteLine(U"RECEIVE HANDLE MODE");
					auto file = ipc->ReceiveHandle();
					ESSE::IO::WriteFile(file, "valera pizduk", 13);
					ESSE::IO::CloseHandle(file);
					console.WriteLine(U"RECEIVE HANDLE MODE END");
				}
			}
		}
	} catch (ESSE::Exception & e) {
		console.WriteLine(ESSE::FormatString(U"CRITICAL ERROR: %0, %1", e.GetError().error_code, e.GetError().error_subcode));
	}
	return 0;
}