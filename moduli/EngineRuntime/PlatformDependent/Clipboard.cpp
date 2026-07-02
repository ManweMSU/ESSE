#include "../Interfaces/Clipboard.h"

namespace Engine
{
	namespace Clipboard
	{
		bool IsFormatAvailable(Format format) { return false; }
		bool GetData(string & value) { return false; }
		bool GetData(Codec::Frame ** value) { return false; }
		bool GetData(string & value, bool attributed) { return false; }
		bool GetData(const string & subclass, Array<uint8> ** value) { return false; }
		bool SetData(const string & value) { return false; }
		bool SetData(Codec::Frame * value) { return false; }
		bool SetData(const string & plain, const string & attributed) { return false; }
		bool SetData(const string & subclass, const void * data, int size) { return false; }
		string GetCustomSubclass(void) { return L""; }
	}
}