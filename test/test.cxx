#include <Cor/Cor.h>
#include <EngineRuntime.h>

#include <stdio.h>
#include <unistd.h>

using namespace Engine;

int Main(void)
{
	ESSE::ErrorContext ectx;
	ESSE::ErrorClear(ectx);

	SystemDesc desc;
	GetSystemInformation(desc);

	auto v4 = ESSE::string(ENGINE_VI_APPIDENT) + U" " + static_cast<const widechar *>(Time::GetCurrentTime().ToLocal().ToString()) + U" - Валера Пиздюк 🥀\n";

	v4 = v4.Uppercased() + ESSE::string(-123);

	printf("%i %i %s#\n", int(v4.GetLength()), int(ESSE::ucs1_string(v4).GetLength()), ESSE::ucs1_string(v4).GetData());

	ESSE::ucs4_string f1 = U"bc", f2 = U"a";
	ESSE::ucs4_string r1 = U"a", r2 = U"Z";

	const ESSE::ucs4_string * sa[] = { &f1, &f2, &r1, &r2 };

	auto v5 = ESSE::string(U"abcabcabcabcabc").Replace(sa + 0, sa + 2, 2);
	auto v6 = ESSE::string(U"abcabcabcabcabc").Replace(U'a', U'Z');
	printf("%s#\n%s#\n", ESSE::ucs1_string(v5).GetData(), ESSE::ucs1_string(v6).GetData());

	auto f = ESSE::string(U"1234.5678").ToDouble();
	auto vf = ESSE::string(f);
	printf("%s#\n", ESSE::ucs1_string(vf).GetData());

	f = ESSE::string(U"123456780000").ToDouble();
	vf = ESSE::string(f);
	printf("%s#\n", ESSE::ucs1_string(vf).GetData());

	f = ESSE::string(U"0.000012345678").ToDouble();
	vf = ESSE::string(f);
	printf("%s#\n", ESSE::ucs1_string(vf).GetData());

	return 0;
}