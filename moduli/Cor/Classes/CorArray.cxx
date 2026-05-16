#include "CorArray.hxx"

namespace ESSE
{
	oref<DataBlock> EncodeString(const string & str, Unicode::Encoding enc, bool include_terminator)
	{
		auto data = owrap(new DataBlock(1));
		data->SetLength(str.GetByteLengthInEncoding(enc, include_terminator));
		str.Encode(data->GetBuffer(), data->GetLength(), enc, include_terminator);
		return data;
	}
	oref<DataBlock> EncodeString(const string & str, const Unicode::EncodingCodepage & cp, bool include_terminator)
	{
		auto data = owrap(new DataBlock(1));
		data->SetLength(str.GetByteLengthInEncoding(cp, include_terminator));
		str.Encode(data->GetBuffer(), data->GetLength(), cp, include_terminator);
		return data;
	}
	array<string> SplitString(const string & str, unichar32 div)
	{
		array<string> result(0x40);
		uintptr prev = 0;
		while (true) {
			auto p = str.FindFirst(div, prev);
			if (p < 0) break;
			result << str.Substring(prev, p - prev);
			prev = p + 1;
		}
		result << str.Substring(prev, -1);
		return result;
	}
	string GatherString(const array<string> & str, unichar32 div)
	{
		if (!str.GetLength()) return string();
		array<const unichar32 *> addr(2 * str.GetLength() - 1);
		array<uintptr> len(2 * str.GetLength() - 1);
		for (uintptr i = 0; i < str.GetLength(); i++) {
			if (i) { addr << &div; len << 1; }
			addr << str[i].GetData();
			len << str[i].GetLength();
		}
		return string::Concatenate(addr, len, addr.GetLength());
	}
	string HexStringFromData(const DataBlock * data, uintptr max_length, bool byte_spaces)
	{
		auto bytes_take = (max_length > 0) ? min(max_length, data->GetLength()) : data->GetLength();
		auto string_size = bytes_take * 2;
		if (byte_spaces && bytes_take > 1) string_size += bytes_take - 1;
		auto str = reinterpret_cast<char *>(malloc(string_size));
		if (!str) throw OutOfMemoryException();
		uintptr pos = 0;
		for (uintptr i = 0; i < bytes_take; i++) {
			if (i && byte_spaces) { str[pos] = ' '; pos++; }
			auto byte = data->ElementAt(i);
			str[pos] = char(HexadecimalBase[(byte & 0xF0) >> 4]);
			str[pos + 1] = char(HexadecimalBase[byte & 0x0F]);
			pos += 2;
		}
		string result;
		try { result = string(str, string_size); } catch (...) { free(str); throw; }
		free(str);
		return result;
	}
	oref<DataBlock> DataFromHexString(const string & str)
	{
		auto data = owrap(new DataBlock(str.GetLength() / 2));
		uint8 a = 0;
		uintptr n = 0;
		for (uintptr i = 0; i < str.GetLength(); i++) {
			auto d = str[i];
			uint8 v;
			if (d == U' ') continue;
			if (d >= U'0' && d <= U'9') v = d - U'0';
			else if (d >= U'A' && d <= U'F') v = 10 + d - U'A';
			else if (d >= U'a' && d <= U'f') v = 10 + d - U'a';
			else throw InvalidFormatException();
			if (n) { a = 16 * a + v; data->Append(a); n = 0; }
			else { a = v; n = 1; }
		}
		if (n) throw InvalidFormatException();
		return data;
	}

	dynamic_string_ucs1::dynamic_string_ucs1(void) : _data(0x200), _length(0) { _data << 0; }
	dynamic_string_ucs1::dynamic_string_ucs1(uintptr block) : _data(block), _length(0) { _data << 0; }
	dynamic_string_ucs1::dynamic_string_ucs1(const ucs1_string & src) : _data(0x200), _length(src.GetLength()) { _data.Append(src.GetData(), src.GetLength()); }
	dynamic_string_ucs1::dynamic_string_ucs1(const ucs1_string & src, uintptr block) : _data(block), _length(src.GetLength()) { _data.Append(src.GetData(), src.GetLength()); }
	dynamic_string_ucs1::operator ucs1_string (void) const { return ucs1_string(_data, _length); }
	dynamic_string_ucs1::operator unichar8 * (void) noexcept { return _data; }
	dynamic_string_ucs1::operator const unichar8 * (void) const noexcept { return _data; }
	unichar8 & dynamic_string_ucs1::operator [] (uintptr index) noexcept { return _data[index]; }
	unichar8 dynamic_string_ucs1::operator [] (uintptr index) const noexcept { return _data[index]; }
	string dynamic_string_ucs1::ToString(void) const { return string(_data, _length); }
	uintptr dynamic_string_ucs1::GetLength(void) const noexcept { return _length; }
	unichar8 * dynamic_string_ucs1::GetData(void) noexcept { return _data; }
	const unichar8 * dynamic_string_ucs1::GetData(void) const noexcept { return _data; }
	unichar8 & dynamic_string_ucs1::GetCharacterAt(uintptr index) noexcept { return _data[index]; }
	unichar8 dynamic_string_ucs1::GetCharacterAt(uintptr index) const noexcept { return _data[index]; }
	void dynamic_string_ucs1::Append(const ucs1_string & str) { _data.SetLength(_length + str.GetLength() + 1); Memory::MemoryCopy(_data.GetBuffer() + _length, str.GetData(), (str.GetLength() + 1) * sizeof(*str.GetData())); _length += str.GetLength(); }
	void dynamic_string_ucs1::Append(unichar8 chr) { _data.SetLength(_length + 2); _data[_length] = chr; _data[_length + 1] = 0; _length++; }
	dynamic_string_ucs1 & dynamic_string_ucs1::operator += (const ucs1_string & str) { Append(str); return *this; }
	dynamic_string_ucs1 & dynamic_string_ucs1::operator += (unichar8 chr) { Append(chr); return *this; }
	dynamic_string_ucs1 & dynamic_string_ucs1::operator << (const ucs1_string & str) { Append(str); return *this; }
	dynamic_string_ucs1 & dynamic_string_ucs1::operator << (unichar8 chr) { Append(chr); return *this; }
	void dynamic_string_ucs1::Insert(const ucs1_string & str, uintptr at) { _data.Insert(str.GetData(), str.GetLength(), at); _length += str.GetLength(); }
	void dynamic_string_ucs1::RemoveRange(uintptr from, uintptr length) { _data.RemoveRange(from, length); _length -= length; }
	void dynamic_string_ucs1::Clear(void) { _data.SetLength(1); _data[0] = 0; _length = 0; }
	void dynamic_string_ucs1::Reserve(uintptr units) { _data.SetLength(units + 1); }
	void dynamic_string_ucs1::ReevaluateLength(void) noexcept { _length = Memory::StringLength(_data); }
	uintptr dynamic_string_ucs1::GetReservedLength(void) const noexcept { return _data.GetLength(); }

	dynamic_string_ucs2::dynamic_string_ucs2(void) : _data(0x200), _length(0) { _data << 0; }
	dynamic_string_ucs2::dynamic_string_ucs2(uintptr block) : _data(block), _length(0) { _data << 0; }
	dynamic_string_ucs2::dynamic_string_ucs2(const ucs2_string & src) : _data(0x200), _length(src.GetLength()) { _data.Append(src.GetData(), src.GetLength()); }
	dynamic_string_ucs2::dynamic_string_ucs2(const ucs2_string & src, uintptr block) : _data(block), _length(src.GetLength()) { _data.Append(src.GetData(), src.GetLength()); }
	dynamic_string_ucs2::operator ucs2_string (void) const { return ucs2_string(_data, _length); }
	dynamic_string_ucs2::operator unichar16 * (void) noexcept { return _data; }
	dynamic_string_ucs2::operator const unichar16 * (void) const noexcept { return _data; }
	unichar16 & dynamic_string_ucs2::operator [] (uintptr index) noexcept { return _data[index]; }
	unichar16 dynamic_string_ucs2::operator [] (uintptr index) const noexcept { return _data[index]; }
	string dynamic_string_ucs2::ToString(void) const { return string(_data, _length); }
	uintptr dynamic_string_ucs2::GetLength(void) const noexcept { return _length; }
	unichar16 * dynamic_string_ucs2::GetData(void) noexcept { return _data; }
	const unichar16 * dynamic_string_ucs2::GetData(void) const noexcept { return _data; }
	unichar16 & dynamic_string_ucs2::GetCharacterAt(uintptr index) noexcept { return _data[index]; }
	unichar16 dynamic_string_ucs2::GetCharacterAt(uintptr index) const noexcept { return _data[index]; }
	void dynamic_string_ucs2::Append(const ucs2_string & str) { _data.SetLength(_length + str.GetLength() + 1); Memory::MemoryCopy(_data.GetBuffer() + _length, str.GetData(), (str.GetLength() + 1) * sizeof(*str.GetData())); _length += str.GetLength(); }
	void dynamic_string_ucs2::Append(unichar16 chr) { _data.SetLength(_length + 2); _data[_length] = chr; _data[_length + 1] = 0; _length++; }
	dynamic_string_ucs2 & dynamic_string_ucs2::operator += (const ucs2_string & str) { Append(str); return *this; }
	dynamic_string_ucs2 & dynamic_string_ucs2::operator += (unichar16 chr) { Append(chr); return *this; }
	dynamic_string_ucs2 & dynamic_string_ucs2::operator << (const ucs2_string & str) { Append(str); return *this; }
	dynamic_string_ucs2 & dynamic_string_ucs2::operator << (unichar16 chr) { Append(chr); return *this; }
	void dynamic_string_ucs2::Insert(const ucs2_string & str, uintptr at) { _data.Insert(str.GetData(), str.GetLength(), at); _length += str.GetLength(); }
	void dynamic_string_ucs2::RemoveRange(uintptr from, uintptr length) { _data.RemoveRange(from, length); _length -= length; }
	void dynamic_string_ucs2::Clear(void) { _data.SetLength(1); _data[0] = 0; _length = 0; }
	void dynamic_string_ucs2::Reserve(uintptr units) { _data.SetLength(units + 1); }
	void dynamic_string_ucs2::ReevaluateLength(void) noexcept { _length = Memory::StringLength(_data); }
	uintptr dynamic_string_ucs2::GetReservedLength(void) const noexcept { return _data.GetLength(); }

	dynamic_string_ucs4::dynamic_string_ucs4(void) : _data(0x200), _length(0) { _data << 0; }
	dynamic_string_ucs4::dynamic_string_ucs4(uintptr block) : _data(block), _length(0) { _data << 0; }
	dynamic_string_ucs4::dynamic_string_ucs4(const ucs4_string & src) : _data(0x200), _length(src.GetLength()) { _data.Append(src.GetData(), src.GetLength()); }
	dynamic_string_ucs4::dynamic_string_ucs4(const ucs4_string & src, uintptr block) : _data(block), _length(src.GetLength()) { _data.Append(src.GetData(), src.GetLength()); }
	dynamic_string_ucs4::operator ucs4_string (void) const { return ucs4_string(_data, _length); }
	dynamic_string_ucs4::operator unichar32 * (void) noexcept { return _data; }
	dynamic_string_ucs4::operator const unichar32 * (void) const noexcept { return _data; }
	unichar32 & dynamic_string_ucs4::operator [] (uintptr index) noexcept { return _data[index]; }
	unichar32 dynamic_string_ucs4::operator [] (uintptr index) const noexcept { return _data[index]; }
	string dynamic_string_ucs4::ToString(void) const { return string(_data, _length); }
	uintptr dynamic_string_ucs4::GetLength(void) const noexcept { return _length; }
	unichar32 * dynamic_string_ucs4::GetData(void) noexcept { return _data; }
	const unichar32 * dynamic_string_ucs4::GetData(void) const noexcept { return _data; }
	unichar32 & dynamic_string_ucs4::GetCharacterAt(uintptr index) noexcept { return _data[index]; }
	unichar32 dynamic_string_ucs4::GetCharacterAt(uintptr index) const noexcept { return _data[index]; }
	void dynamic_string_ucs4::Append(const ucs4_string & str) { _data.SetLength(_length + str.GetLength() + 1); Memory::MemoryCopy(_data.GetBuffer() + _length, str.GetData(), (str.GetLength() + 1) * sizeof(*str.GetData())); _length += str.GetLength(); }
	void dynamic_string_ucs4::Append(unichar32 chr) { _data.SetLength(_length + 2); _data[_length] = chr; _data[_length + 1] = 0; _length++; }
	dynamic_string_ucs4 & dynamic_string_ucs4::operator += (const ucs4_string & str) { Append(str); return *this; }
	dynamic_string_ucs4 & dynamic_string_ucs4::operator += (unichar32 chr) { Append(chr); return *this; }
	dynamic_string_ucs4 & dynamic_string_ucs4::operator << (const ucs4_string & str) { Append(str); return *this; }
	dynamic_string_ucs4 & dynamic_string_ucs4::operator << (unichar32 chr) { Append(chr); return *this; }
	void dynamic_string_ucs4::Insert(const ucs4_string & str, uintptr at) { _data.Insert(str.GetData(), str.GetLength(), at); _length += str.GetLength(); }
	void dynamic_string_ucs4::RemoveRange(uintptr from, uintptr length) { _data.RemoveRange(from, length); _length -= length; }
	void dynamic_string_ucs4::Clear(void) { _data.SetLength(1); _data[0] = 0; _length = 0; }
	void dynamic_string_ucs4::Reserve(uintptr units) { _data.SetLength(units + 1); }
	void dynamic_string_ucs4::ReevaluateLength(void) noexcept { _length = Memory::StringLength(_data); }
	uintptr dynamic_string_ucs4::GetReservedLength(void) const noexcept { return _data.GetLength(); }
}