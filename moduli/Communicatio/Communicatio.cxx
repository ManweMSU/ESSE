#include "Communicatio.h"

namespace ESSE
{
	namespace Communicatio
	{
		string NetworkAddress::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return GetAddressString(); ESSE_TRY_OUTRO(string()) }

		NetworkAddressLocal::NetworkAddressLocal(const string & n) : name(n) {}
		NetworkAddressLocal::~NetworkAddressLocal(void) {}
		NetworkAddressDomain NetworkAddressLocal::GetDomain(void) const noexcept { return NetworkAddressDomain::Local; }
		string NetworkAddressLocal::GetAddressString(void) const { return name; }
		void NetworkAddressLocal::SetAddressString(const string & addr) { name = addr; }
		oref<DataBlock> NetworkAddressLocal::GetAddressData(void) const { return EncodeString(name, Unicode::Encoding::ASCII, false); }
		void NetworkAddressLocal::SetAddressData(const void * data, uintptr length) { name = string(data, length, Unicode::Encoding::ASCII); }

		NetworkAddressIPv4::NetworkAddressIPv4(const string & text) { SetAddressString(text); }
		NetworkAddressIPv4::NetworkAddressIPv4(uint8 a0, uint8 a1, uint8 a2, uint8 a3, uint16 p) noexcept { address[0] = a0; address[1] = a1; address[2] = a2; address[3] = a3; port = p; }
		NetworkAddressIPv4::~NetworkAddressIPv4(void) {}
		NetworkAddressDomain NetworkAddressIPv4::GetDomain(void) const noexcept { return NetworkAddressDomain::IPv4; }
		string NetworkAddressIPv4::GetAddressString(void) const
		{
			if (port) return FormatString(U"%0.%1.%2.%3:%4", address[0], address[1], address[2], address[3], port);
			else return FormatString(U"%0.%1.%2.%3", address[0], address[1], address[2], address[3]);
		}
		void NetworkAddressIPv4::SetAddressString(const string & addr)
		{
			intptr p;
			array<string> ip;
			if ((p = addr.FindFirst(U':')) >= 0) {
				ip = SplitString(addr.Substring(0, p), U'.');
				port = addr.Substring(p + 1, -1).ToUInt32();
			} else {
				ip = SplitString(addr, U'.');
				port = 0;
			}
			if (ip.GetLength() != 4) throw InvalidFormatException();
			for (uint i = 0; i < 4; i++) address[i] = ip[i].ToUInt32();
		}
		oref<DataBlock> NetworkAddressIPv4::GetAddressData(void) const
		{
			auto result = owrap(new DataBlock(6));
			result->SetLength(6);
			Memory::MemoryCopy(result->GetBuffer(), &address, 4);
			Memory::MemoryCopy(result->GetBuffer() + 4, &port, 2);
			return result;
		}
		void NetworkAddressIPv4::SetAddressData(const void * data, uintptr length)
		{
			if (length != 6) throw InvalidFormatException();
			Memory::MemoryCopy(&address, data, 4);
			Memory::MemoryCopy(&port, reinterpret_cast<const uint8 *>(data) + 4, 2);
		}
		NetworkAddressIPv4 NetworkAddressIPv4::CreateLoopback(uint16 p) noexcept { return NetworkAddressIPv4(127, 0, 0, 1, p); }
		NetworkAddressIPv4 NetworkAddressIPv4::CreateAnycast(uint16 p) noexcept { return NetworkAddressIPv4(0, 0, 0, 0, p); }

		NetworkAddressIPv6::NetworkAddressIPv6(const string & text) { SetAddressString(text); }
		NetworkAddressIPv6::NetworkAddressIPv6(uint16 a0, uint16 a1, uint16 a2, uint16 a3, uint16 a4, uint16 a5, uint16 a6, uint16 a7, uint16 p) noexcept { address[0] = a0; address[1] = a1; address[2] = a2; address[3] = a3; address[4] = a4; address[5] = a5; address[6] = a6; address[7] = a7; port = p; }
		NetworkAddressIPv6::~NetworkAddressIPv6(void) {}
		NetworkAddressDomain NetworkAddressIPv6::GetDomain(void) const noexcept { return NetworkAddressDomain::IPv6; }
		string NetworkAddressIPv6::GetAddressString(void) const
		{
			auto ip = FormatString(U"[%0:%1:%2:%3:%4:%5:%6:%7]",
				string(address[0], HexadecimalBaseLowerCase, 4),
				string(address[1], HexadecimalBaseLowerCase, 4),
				string(address[2], HexadecimalBaseLowerCase, 4),
				string(address[3], HexadecimalBaseLowerCase, 4),
				string(address[4], HexadecimalBaseLowerCase, 4),
				string(address[5], HexadecimalBaseLowerCase, 4),
				string(address[6], HexadecimalBaseLowerCase, 4),
				string(address[7], HexadecimalBaseLowerCase, 4));
			if (port) return ip + U":" + string(port); else return ip;
		}
		void NetworkAddressIPv6::SetAddressString(const string & addr)
		{
			auto ipf = addr.FindFirst(U'[');
			auto ipl = addr.FindLast(U']');
			if (ipf != 0 || ipl < 0) throw InvalidFormatException();
			auto ip = SplitString(addr.Substring(ipf + 1, ipl - ipf - 1), U':');
			if (addr.GetLength() > ipl + 1) {
				if (addr[ipl + 1] == U':') {
					port = addr.Substring(ipl + 2, -1).ToUInt32();
				} else throw InvalidFormatException();
			} else port = 0;
			Memory::ZeroMemory(&address, sizeof(address));
			int s = 0;
			for (uintptr i = 0; i < ip.GetLength(); i++) {
				if (!ip[i].GetLength()) break;
				if (ip[i].FindFirst(U'.') >= 0) {
					if (s > 6) throw InvalidFormatException();
					NetworkAddressIPv4 v4(ip[i]);
					address[s] = (uint(v4.address[0]) << 8) | uint(v4.address[1]);
					address[s + 1] = (uint(v4.address[2]) << 8) | uint(v4.address[3]);
					s += 2;
				} else {
					if (s > 7) throw InvalidFormatException();
					address[s] = ip[i].ToUInt32(HexadecimalBase);
					s++;
				}
			}
			s = 7;
			for (uintptr i = ip.GetLength() - 1; i < ip.GetLength(); i--) {
				if (!ip[i].GetLength()) break;
				if (ip[i].FindFirst(U'.') >= 0) {
					if (s < 1) throw InvalidFormatException();
					NetworkAddressIPv4 v4(ip[i]);
					address[s - 1] = (uint(v4.address[0]) << 8) | uint(v4.address[1]);
					address[s] = (uint(v4.address[2]) << 8) | uint(v4.address[3]);
					s -= 2;
				} else {
					if (s < 0) throw InvalidFormatException();
					address[s] = ip[i].ToUInt32(HexadecimalBase);
					s--;
				}
			}
		}
		oref<DataBlock> NetworkAddressIPv6::GetAddressData(void) const
		{
			auto result = owrap(new DataBlock(18));
			result->SetLength(18);
			Memory::MemoryCopy(result->GetBuffer(), &address, 16);
			Memory::MemoryCopy(result->GetBuffer() + 16, &port, 2);
			return result;
		}
		void NetworkAddressIPv6::SetAddressData(const void * data, uintptr length)
		{
			if (length != 18) throw InvalidFormatException();
			Memory::MemoryCopy(&address, data, 16);
			Memory::MemoryCopy(&port, reinterpret_cast<const uint8 *>(data) + 16, 2);
		}
		NetworkAddressIPv6 NetworkAddressIPv6::CreateLoopback(uint16 p) noexcept { return NetworkAddressIPv6(0, 0, 0, 0, 0, 0, 0, 1, p); }
		NetworkAddressIPv6 NetworkAddressIPv6::CreateAnycast(uint16 p) noexcept { return NetworkAddressIPv6(0, 0, 0, 0, 0, 0, 0, 0, p); }

		void INetworkChannel::Connect(NetworkAddress * address, ErrorContext * error, IDispatchTask * hdlr)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Connect(address, error, hdlr, ectx);
			ErrorThrow(ectx);
		}
		void INetworkChannel::Connect(NetworkAddress * address, NetworkSecurityDesc & sec, ErrorContext * error, IDispatchTask * hdlr)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Connect(address, sec, error, hdlr, ectx);
			ErrorThrow(ectx);
		}
		void INetworkChannel::Send(DataBlock * data, ErrorContext * error, uintptr * sent, IDispatchTask * hdlr)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Send(data, error, sent, hdlr, ectx);
			ErrorThrow(ectx);
		}
		void INetworkChannel::Receive(uintptr length, ErrorContext * error, oref<DataBlock> * data, IDispatchTask * hdlr)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Receive(length, error, data, hdlr, ectx);
			ErrorThrow(ectx);
		}
		void INetworkChannel::Close(bool ultimately)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Close(ultimately, ectx);
			ErrorThrow(ectx);
		}

		void INetworkListener::Bind(NetworkAddress * address)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Bind(address, ectx);
			ErrorThrow(ectx);
		}
		void INetworkListener::Bind(NetworkAddress * address, NetworkIdentityDesc & idesc)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Bind(address, idesc, ectx);
			ErrorThrow(ectx);
		}
		void INetworkListener::Accept(uintptr limit, ErrorContext * error, oref<INetworkChannel> * channel, oref<NetworkAddress> * address, IDispatchTask * hdlr)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Accept(limit, error, channel, address, hdlr, ectx);
			ErrorThrow(ectx);
		}
		void INetworkListener::Close(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			Close(ectx);
			ErrorThrow(ectx);
		}

		oref<INetworkChannel> CreateChannel(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreateChannel(ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<INetworkListener> CreateListener(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreateListener(ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<object_array<NetworkAddress>> GetNetworkAddresses(const string & domain_name, uint16 port, NetworkAddressDomain address_domain)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = GetNetworkAddresses(domain_name, port, address_domain, ectx);
			ErrorThrow(ectx);
			return result;
		}

		string ConvertStringToPunycode(const string & text)
		{
			bool regular = true;
			for (uintptr i = 0; i < text.GetLength(); i++) if (text[i] & 0xFFFFFF80) { regular = false; break; }
			if (regular) return text;
			dynamic_string_ucs4 result;
			uintptr run_length = 0;
			unichar32 max_char = 0;
			for (uintptr i = 0; i < text.GetLength(); i++) {
				if (text[i] > max_char) max_char = text[i];
				if (text[i] < 128) { run_length++; result += text[i]; }
			}
			if (run_length) result += U'-';
			run_length++;
			unichar32 cur_char = 128;
			int state = 0, last_state = -1, bias = 72;
			while (cur_char <= max_char) {
				unichar32 min_char = max_char;
				for (uintptr i = 0; i < text.GetLength(); i++) if (text[i] >= cur_char && text[i] < min_char) min_char = text[i];
				if (min_char > cur_char) {
					for (uintptr i = 0; i < text.GetLength(); i++) if (text[i] < min_char) state += min_char - cur_char;
					cur_char = min_char;
				}
				for (uintptr i = 0; i < text.GetLength(); i++) {
					if (text[i] == cur_char) {
						int ds = state - last_state - 1;
						if (ds) {
							int ids = ds, div = 0;
							while (true) {
								div += 36;
								int edge = max(min(div - bias, 26), 1);
								if (ds < edge) break;
								result += U"abcdefghijklmnopqrstuvwxyz0123456789"[edge + ((ds - edge) % (36 - edge))];
								ds = (ds - edge) / (36 - edge);
							}
							result += U"abcdefghijklmnopqrstuvwxyz0123456789"[ds];
							if (last_state == -1) ids /= 700;
							else ids /= 2;
							ids += (ids / run_length);
							int k = 0;
							while (ids > 35 * 13) { ids /= 35; k += 36; }
							bias = k + (36 * ids) / (ids + 38);

						} else result += U'a';
						last_state = state;
						run_length++;
					}
					if (text[i] <= cur_char) state++;
				}
				cur_char++;
			}
			return string(U"xn--") + string(result);
		}
		string ConvertStringFromPunycode(const string & text)
		{
			if (text.GetLength() >= 4 && text.Substring(0, 4) == U"xn--") {
				intptr xs = text.FindLast(U'-'), dp;
				array<unichar32> ucs(0x40);
				if (xs >= 4) {
					for (intptr i = 4; i < xs; i++) ucs << text[i];
					dp = xs + 1;
				} else dp = 4;
				intptr bias = 72;
				unichar32 char_dec = 128;
				intptr pos_at = 0;
				bool first = true;
				while (dp < text.GetLength()) {
					intptr delta = 0, reg = 36, weight = 1;
					while (true) {
						if (dp >= text.GetLength()) break;
						int digit = 0;
						if (text[dp] >= U'A' && text[dp] <= U'Z') digit = text[dp] - U'A';
						else if (text[dp] >= U'a' && text[dp] <= U'z') digit = text[dp] - U'a';
						else if (text[dp] >= U'0' && text[dp] <= U'9') digit = text[dp] - U'0' + 26;
						delta += digit * weight;
						intptr edge = max<intptr>(min<intptr>(reg - bias, 26), 1);
						dp++;
						if (digit < edge) break;
						weight *= 36 - edge;
						reg += 36;
					}
					intptr ds = delta;
					if (first) { ds /= 700; first = false; } else ds /= 2;
					ds += (ds / (ucs.GetLength() + 1));
					intptr k = 0;
					while (ds > 35 * 13) { ds /= 35; k += 36; }
					bias = k + (36 * ds) / (ds + 38);
					for (intptr i = 0; i < delta; i++) {
						pos_at++;
						if (pos_at > ucs.GetLength()) { pos_at = 0; char_dec++; }
					}
					ucs.Insert(char_dec, pos_at);
					pos_at++;
					if (pos_at > ucs.GetLength()) { pos_at = 0; char_dec++; }
				}
				return string(ucs.GetBuffer(), ucs.GetLength());
			} else return text;
		}
		string ConvertDomainToPunycode(const string & text)
		{
			auto domains = SplitString(text, U'.');
			for (auto & d : domains) d = ConvertStringToPunycode(d);
			return GatherString(domains, U'.');
		}
		string ConvertDomainFromPunycode(const string & text)
		{
			auto domains = SplitString(text, U'.');
			for (auto & d : domains) d = ConvertStringFromPunycode(d);
			return GatherString(domains, U'.');
		}
	}
}