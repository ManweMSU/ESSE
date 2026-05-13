#include <Cor/Cor.h>
#include <EngineRuntime.h>

#include <stdio.h>

using namespace Engine;

int Main(void)
{
	ESSE::ErrorContext ectx;
	ESSE::ErrorClear(ectx);

	SystemDesc desc;
	GetSystemInformation(desc);
	
	auto str = string(ENGINE_VI_APPIDENT) + L" " + Time::GetCurrentTime().ToLocal().ToString() + L"\n";
	auto enc = str.EncodeSequence(Encoding::UTF8, true);
	printf("%s", enc->GetBuffer());
	enc->Release();

	return 0;
}