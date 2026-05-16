#include <Cor/Cor.h>
#include <EngineRuntime.h>

#include <stdio.h>
#include <unistd.h>

using namespace Engine;

int Main(void)
{
	ESSE::ErrorContext ectx;
	ESSE::ErrorClear(ectx);
	try {
		SystemDesc desc;
		GetSystemInformation(desc);

		auto v4 = ESSE::string(ENGINE_VI_APPIDENT) + U" " + static_cast<const widechar *>(Time::GetCurrentTime().ToLocal().ToString()) + U" - Валера Пиздюк 🥀\n";

		auto s = ESSE::SplitString(v4, U' ');
		for (auto & ss : s) printf("%s\n", ESSE::ucs1_string(ss).GetData());
		auto v5 = ESSE::GatherString(s, U'|');

	} catch (ESSE::Exception & e) {
		printf("CRITICAL ERROR: %i, %i\n", int(e.GetError().error_code), int(e.GetError().error_subcode));
	}
	return 0;
}