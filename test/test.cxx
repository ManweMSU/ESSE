#include <Cor/Cor.h>
#include <Auxilia/Auxilia.h>
#include <EngineRuntime.h>

using namespace Engine;

int Main(void)
{
	auto stream_in = ESSE::FileStream::CreateWrapper(ESSE::IO::GetStandardHandle(ESSE::IO::StandardHandleType::Input));
	auto stream_out = ESSE::FileStream::CreateWrapper(ESSE::IO::GetStandardHandle(ESSE::IO::StandardHandleType::Output));
	ESSE::TextDecoder reader(stream_in, ESSE::Unicode::Encoding::UTF8);
	ESSE::TextEncoder writer(stream_out, ESSE::Unicode::Encoding::UTF8);
	ESSE::ErrorContext ectx;
	ESSE::ErrorClear(ectx);
	try {
		SystemDesc desc;
		GetSystemInformation(desc);

		auto v4 = ESSE::string(ENGINE_VI_APPIDENT) + U" " + static_cast<const widechar *>(Time::GetCurrentTime().ToLocal().ToString()) + U" - Валера Пиздюк 🥀\n";
		writer.WriteLine(v4);
		writer.Write(U"MODE: ");
		auto mode = reader.ReadLine();
		if (mode.GetLength()) {
			ESSE::IPC::PurifyConnectionListener(U"pizduk");
			auto ipc_l = ESSE::IPC::CreateConnectionListener(U"pizduk", 0);
			auto ipc = ipc_l->Accept();
			while (true) {
				auto l = reader.ReadLine();
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
					writer.WriteLine(U"END-OF-STREAM");
					break;
				} else if (chr != '\33') {
					writer.Write(ESSE::ucs1_string(&chr, 1));
				} else {
					writer.WriteLine(U"RECEIVE HANDLE MODE");
					auto file = ipc->ReceiveHandle();
					ESSE::IO::WriteFile(file, "valera pizduk", 13);
					ESSE::IO::CloseHandle(file);
					writer.WriteLine(U"RECEIVE HANDLE MODE END");
				}
			}
		}
	} catch (ESSE::Exception & e) {
		writer.WriteLine(ESSE::FormatString(U"CRITICAL ERROR: %0, %1", e.GetError().error_code, e.GetError().error_subcode));
	}
	return 0;
}