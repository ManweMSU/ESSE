#include "../Interfaces/Assembly.h"

#include <Auxilia/Auxilia.h>

namespace Engine
{
	namespace Assembly
	{
		string CurrentLocale;
		SafePointer<Storage::StringTable> CommonStrings;

		string GetCurrentUserLocale(void)
		{
			ESSE::unichar32 buffer[3];
			ESSE::System::GetUserLocale(buffer, 3);
			return string(&buffer[0]);
		}
		Streaming::Stream * QueryResource(const widechar * identifier, bool local_first)
		{
			ESSE::ucs1_string rsrc_name(identifier);
			ESSE::ucs1_string rsrc_locale(CurrentLocale);
			const void * rsrc_pdata = 0;
			uintptr rsrc_length = 0;
			if (local_first) {
				if (!ESSE::QueryResource(&rsrc_pdata, &rsrc_length, rsrc_name, rsrc_locale))
					if (!ESSE::QueryResource(&rsrc_pdata, &rsrc_length, rsrc_name, "")) return 0;
			} else {
				if (!ESSE::QueryResource(&rsrc_pdata, &rsrc_length, rsrc_name, ""))
					if (!ESSE::QueryResource(&rsrc_pdata, &rsrc_length, rsrc_name, rsrc_locale)) return 0;
			}
			return new Streaming::MemoryStream(rsrc_pdata, rsrc_length);
		}
		Streaming::Stream * QueryResource(const widechar * identifier) { try { return QueryResource(identifier, false); } catch (...) { return 0; } }
		Streaming::Stream * QueryLocalizedResource(const widechar * identifier) { try { return QueryResource(identifier, true); } catch (...) { return 0; } }
		void SetLocalizedCommonStrings(Storage::StringTable * table) { CommonStrings.SetRetain(table); }
		Storage::StringTable * GetLocalizedCommonStrings(void) { return CommonStrings; }
		const widechar * GetLocalizedCommonString(int ID, const widechar * alternate) { if (CommonStrings) return CommonStrings->GetString(ID); else return alternate; }
	}
}