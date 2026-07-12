#include "../Interfaces/Clipboard.h"
#include "ESSE.h"
#include <Fenestrae/Fenestrae.h>

namespace Engine
{
	namespace Clipboard
	{
		bool IsFormatAvailable(Format format)
		{
			auto ws = ESSE::Windows::GetWindowSystem();
			if (!ws) return false;
			if (format == Format::Text) return ws->GetClipboardManager()->ProbeClipboardFormats(ESSE::Windows::ClipboardDataFormatText) != 0;
			else if (format == Format::Image) return ws->GetClipboardManager()->ProbeClipboardFormats(ESSE::Windows::ClipboardDataFormatImage) != 0;
			else if (format == Format::RichText) {
				ESSE::Windows::ClipboardDataDesc desc;
				if (!ws->GetClipboardManager()->ReadClipboard(ESSE::Windows::ClipboardDataFormatDataFormat, desc)) return false;
				return desc.data_format == U"EngineRuntime.RichText";
			} else if (format == Format::Custom) return ws->GetClipboardManager()->ProbeClipboardFormats(ESSE::Windows::ClipboardDataFormatData) != 0;
			else return false;
		}
		bool GetData(string & value)
		{
			auto ws = ESSE::Windows::GetWindowSystem();
			if (!ws) return false;
			ESSE::Windows::ClipboardDataDesc desc;
			if (!ws->GetClipboardManager()->ReadClipboard(ESSE::Windows::ClipboardDataFormatText, desc)) return false;
			value = desc.text.GetData();
			return true;
		}
		bool GetData(Codec::Frame ** value)
		{
			auto ws = ESSE::Windows::GetWindowSystem();
			if (!ws) return false;
			ESSE::Windows::ClipboardDataDesc desc;
			if (!ws->GetClipboardManager()->ReadClipboard(ESSE::Windows::ClipboardDataFormatImage, desc)) return false;
			*value = ESSEIO::WrapFrame(desc.image);
			return true;
		}
		bool GetData(string & value, bool attributed)
		{
			if (!attributed) return GetData(value);
			auto ws = ESSE::Windows::GetWindowSystem();
			if (!ws) return false;
			ESSE::Windows::ClipboardDataDesc desc;
			if (!ws->GetClipboardManager()->ReadClipboard(ESSE::Windows::ClipboardDataFormatData, desc)) return false;
			if ((desc.format_mask & ESSE::Windows::ClipboardDataFormatData) == ESSE::Windows::ClipboardDataFormatData && desc.data_format == U"EngineRuntime.RichText") {
				value = string(desc.data->GetBuffer(), desc.data->GetLength(), Encoding::UTF8);
				return true;
			} else return false;
		}
		bool GetData(const string & subclass, Array<uint8> ** value)
		{
			auto ws = ESSE::Windows::GetWindowSystem();
			if (!ws) return false;
			ESSE::Windows::ClipboardDataDesc desc;
			if (!ws->GetClipboardManager()->ReadClipboard(ESSE::Windows::ClipboardDataFormatData, desc)) return false;
			if ((desc.format_mask & ESSE::Windows::ClipboardDataFormatData) == ESSE::Windows::ClipboardDataFormatData && StringCompare(desc.data_format, subclass)) {
				*value = new Array<uint8>(1);
				(*value)->SetLength(desc.data->GetLength());
				MemoryCopy((*value)->GetBuffer(), desc.data->GetBuffer(), desc.data->GetLength());
				return true;
			} else return false;
		}
		bool SetData(const string & value)
		{
			auto ws = ESSE::Windows::GetWindowSystem();
			if (!ws) return false;
			ESSE::Windows::ClipboardDataDesc desc;
			desc.format_mask = ESSE::Windows::ClipboardDataFormatText;
			desc.text = static_cast<const widechar *>(value);
			return ws->GetClipboardManager()->WriteClipboard(desc);
		}
		bool SetData(Codec::Frame * value)
		{
			auto ws = ESSE::Windows::GetWindowSystem();
			if (!ws) return false;
			ESSE::Windows::ClipboardDataDesc desc;
			desc.format_mask = ESSE::Windows::ClipboardDataFormatImage;
			desc.image = ESSEIO::WrapFrame(value);
			return ws->GetClipboardManager()->WriteClipboard(desc);
		}
		bool SetData(const string & plain, const string & attributed)
		{
			auto ws = ESSE::Windows::GetWindowSystem();
			if (!ws) return false;
			ESSE::Windows::ClipboardDataDesc desc;
			desc.format_mask = ESSE::Windows::ClipboardDataFormatText | ESSE::Windows::ClipboardDataFormatData;
			desc.text = static_cast<const widechar *>(plain);
			desc.data_format = U"EngineRuntime.RichText";
			desc.data = ESSE::EncodeString(ESSE::string(static_cast<const widechar *>(attributed)), ESSE::Unicode::Encoding::UTF8, false);
			return ws->GetClipboardManager()->WriteClipboard(desc);
		}
		bool SetData(const string & subclass, const void * data, int size)
		{
			auto ws = ESSE::Windows::GetWindowSystem();
			if (!ws) return false;
			ESSE::Windows::ClipboardDataDesc desc;
			desc.format_mask = ESSE::Windows::ClipboardDataFormatData;
			desc.data_format = static_cast<const widechar *>(subclass);
			desc.data = ESSE::owrap(new ESSE::DataBlock(1));
			desc.data->SetLength(size);
			MemoryCopy(desc.data->GetBuffer(), data, size);
			return ws->GetClipboardManager()->WriteClipboard(desc);
		}
		string GetCustomSubclass(void)
		{
			auto ws = ESSE::Windows::GetWindowSystem();
			if (!ws) return U"";
			ESSE::Windows::ClipboardDataDesc desc;
			if (!ws->GetClipboardManager()->ReadClipboard(ESSE::Windows::ClipboardDataFormatDataFormat, desc)) return false;
			return desc.data_format.GetData();
		}
	}
}