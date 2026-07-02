#include "../Interfaces/Socket.h"
#include "../Network/Punycode.h"
#include "../Miscellaneous/DynamicString.h"

#include <Communicatio/Communicatio.h>

namespace Engine
{
	namespace Network
	{
		class XSocket : public Socket
		{
			ESSE::oref<ESSE::Communicatio::INetworkChannel> _channel;
			ESSE::oref<ESSE::Communicatio::INetworkListener> _listener;
			ESSE::oref<ESSE::Signal> _read_event, _write_event;
			bool _ipv6;
		public:
			XSocket(ESSE::Communicatio::INetworkChannel * object, bool is_ipv6) : _channel(object), _ipv6(is_ipv6)
			{
				_read_event = ESSE::CreateSignal(false);
				_write_event = ESSE::CreateSignal(false);
				if (!_read_event || !_write_event) throw OutOfMemoryException();
			}
			XSocket(bool is_ipv6) : _ipv6(is_ipv6)
			{
				_read_event = ESSE::CreateSignal(false);
				_write_event = ESSE::CreateSignal(false);
				if (!_read_event || !_write_event) throw OutOfMemoryException();
			}
			~XSocket(void) override { if (_channel) _channel->Close(true); if (_listener) _listener->Close(); }
			virtual void Read(void * buffer, uint32 length) override
			{
				if (!_channel) throw InvalidStateException();
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				ESSE::oref<ESSE::DataBlock> result;
				_read_event->Reset();
				_channel->Receive(length, &ectx, &result, ESSE::CreateFunctionalTask([&]() {
					_read_event->Raise();
				}));
				_read_event->Wait();
				ESSE::ErrorThrow(ectx);
				MemoryCopy(buffer, result->GetBuffer(), result->GetLength());
			}
			virtual void Write(const void * data, uint32 length) override
			{
				if (!_channel) throw InvalidStateException();
				auto message = ESSE::owrap(new ESSE::DataBlock(1));
				message->Append(reinterpret_cast<const uint8 *>(data), length);
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				_write_event->Reset();
				_channel->Send(message, &ectx, 0, ESSE::CreateFunctionalTask([&]() {
					_write_event->Raise();
				}));
				_write_event->Wait();
				ESSE::ErrorThrow(ectx);
			}
			virtual int64 Seek(int64 position, Streaming::SeekOrigin origin) override { throw ESSE::NotImplementedException(); }
			virtual uint64 Length(void) override { throw ESSE::NotImplementedException(); }
			virtual void SetLength(uint64 length) override { throw ESSE::NotImplementedException(); }
			virtual void Flush(void) override {}
			virtual void Connect(const Address & address, uint16 port) override
			{
				if (_channel || _listener) throw InvalidStateException();
				ESSE::oref<ESSE::Communicatio::NetworkAddress> addr;
				if (_ipv6) {
					addr = ESSE::oref<ESSE::Communicatio::NetworkAddress>::CreateOwned(new ESSE::Communicatio::NetworkAddressIPv6(
						address.DWord4 >> 16, address.DWord4, address.DWord3 >> 16, address.DWord3,
						address.DWord2 >> 16, address.DWord2, address.DWord1 >> 16, address.DWord1,
						port
					));
				} else {
					addr = ESSE::oref<ESSE::Communicatio::NetworkAddress>::CreateOwned(new ESSE::Communicatio::NetworkAddressIPv4(
						address.DWord1 >> 24, address.DWord1 >> 16, address.DWord1 >> 8, address.DWord1,
						port
					));
				}
				_channel = ESSE::Communicatio::CreateChannel();
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				_write_event->Reset();
				_channel->Connect(addr, &ectx, ESSE::CreateFunctionalTask([&]() {
					_write_event->Raise();
				}));
				_write_event->Wait();
				ESSE::ErrorThrow(ectx);
			}
			virtual void Bind(const Address & address, uint16 port) override
			{
				if (_channel || _listener) throw InvalidStateException();
				ESSE::oref<ESSE::Communicatio::NetworkAddress> addr;
				if (_ipv6) {
					addr = ESSE::oref<ESSE::Communicatio::NetworkAddress>::CreateOwned(new ESSE::Communicatio::NetworkAddressIPv6(
						address.DWord4 >> 16, address.DWord4, address.DWord3 >> 16, address.DWord3,
						address.DWord2 >> 16, address.DWord2, address.DWord1 >> 16, address.DWord1,
						port
					));
				} else {
					addr = ESSE::oref<ESSE::Communicatio::NetworkAddress>::CreateOwned(new ESSE::Communicatio::NetworkAddressIPv4(
						address.DWord1 >> 24, address.DWord1 >> 16, address.DWord1 >> 8, address.DWord1,
						port
					));
				}
				_listener = ESSE::Communicatio::CreateListener();
				_listener->Bind(addr);
			}
			virtual void Bind(uint16 port) override { Bind(Address::CreateAny(), port); }
			virtual void Listen(void) override {}
			virtual Socket * Accept() override { Address addr; uint16 port; return Accept(addr, port); }
			virtual Socket * Accept(Address & address, uint16 & port) override
			{
				if (!_listener) throw InvalidStateException();
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				ESSE::oref<ESSE::Communicatio::NetworkAddress> addr;
				ESSE::oref<ESSE::Communicatio::INetworkChannel> channel;
				_read_event->Reset();
				_listener->Accept(0, &ectx, &channel, &addr, ESSE::CreateFunctionalTask([&]() {
					_read_event->Raise();
				}));
				_read_event->Wait();
				ESSE::ErrorThrow(ectx);
				if (addr->GetDomain() == ESSE::Communicatio::NetworkAddressDomain::IPv4) {
					MemoryCopy(&address.DWord1, &static_cast<ESSE::Communicatio::NetworkAddressIPv4 *>(addr.Inner())->address, 4);
					address.DWord1 = InverseEndianess(address.DWord1);
				} else if (addr->GetDomain() == ESSE::Communicatio::NetworkAddressDomain::IPv6) {
					auto a6 = static_cast<ESSE::Communicatio::NetworkAddressIPv6 *>(addr.Inner());
					address.DWord1 = (uint(a6->address[6]) << 16) | uint(a6->address[7]);
					address.DWord2 = (uint(a6->address[4]) << 16) | uint(a6->address[5]);
					address.DWord3 = (uint(a6->address[2]) << 16) | uint(a6->address[3]);
					address.DWord4 = (uint(a6->address[0]) << 16) | uint(a6->address[1]);
				}
				return new XSocket(channel, _ipv6);
			}
			virtual void Shutdown(bool close_read, bool close_write) override
			{
				if (_channel) _channel->Close(close_read && close_write);
				if (_listener) _listener->Close();
			}
			virtual bool Wait(int time) override { return true; }
			virtual void SetReadTimeout(int time) override {}
			virtual int GetReadTimeout(void) override { return -1; }
		};
		Socket * CreateSocket(SocketAddressDomain domain, SocketProtocol protocol)
		{
			if (domain == SocketAddressDomain::Unknown) throw InvalidArgumentException();
			if (protocol == SocketProtocol::Unknown || protocol == SocketProtocol::UDP) throw InvalidArgumentException();	
			return new XSocket(domain == SocketAddressDomain::IPv6);
		}
		Address::Address(void) {}
		bool Address::IsIPv4(void) const { return DWord2 == 0x0000FFFF && DWord3 == 0 && DWord4 == 0; }
		Address::operator string(void) const
		{
			if (IsIPv4()) {
				return string((IPv4 & 0xFF000000) >> 24) + U"." + string((IPv4 & 0xFF0000) >> 16) + U"." +
					string((IPv4 & 0xFF00) >> 8) + U"." + string(IPv4 & 0xFF);
			} else {
				DynamicString result;
				result += string((DWord4 & 0xFFFF0000) >> 16, U"0123456789abcdef", 4) + U":";
				result += string(DWord4 & 0xFFFF, U"0123456789abcdef", 4) + U":";
				result += string((DWord3 & 0xFFFF0000) >> 16, U"0123456789abcdef", 4) + U":";
				result += string(DWord3 & 0xFFFF, U"0123456789abcdef", 4) + U":";
				result += string((DWord2 & 0xFFFF0000) >> 16, U"0123456789abcdef", 4) + U":";
				result += string(DWord2 & 0xFFFF, U"0123456789abcdef", 4) + U":";
				result += string((DWord1 & 0xFFFF0000) >> 16, U"0123456789abcdef", 4) + U":";
				result += string(DWord1 & 0xFFFF, U"0123456789abcdef", 4);
				return result.ToString();
			}
		}
		Address Address::CreateIPv4(uint32 address)
		{
			Address result;
			result.IPv4 = address;
			result.DWord2 = 0x0000FFFF;
			result.DWord3 = 0;
			result.DWord4 = 0;
			return result;
		}
		Address Address::CreateIPv6(uint32 dword_1, uint32 dword_2, uint32 dword_3, uint32 dword_4)
		{
			Address result;
			result.DWord1 = dword_1;
			result.DWord2 = dword_2;
			result.DWord3 = dword_3;
			result.DWord4 = dword_4;
			return result;
		}
		Address Address::CreateLoopBackIPv4(void) { return CreateIPv4(0x7F000001); }
		Address Address::CreateLoopBackIPv6(void) { return CreateIPv6(1, 0, 0, 0); }
		Address Address::CreateAny(void) { return CreateIPv6(0, 0, 0, 0); }
		uint32 InverseEndianess(uint32 value) { return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24); }
		uint16 InverseEndianess(uint16 value) { return ((value & 0xFF00) >> 8) | ((value & 0xFF) << 8); }
		Array<AddressEntity>* GetAddressByHost(const string & host_name, uint16 host_port, SocketAddressDomain domain, SocketProtocol protocol)
		{
			string wname = DomainNameToPunycode(host_name);
			auto query = ESSE::Communicatio::GetNetworkAddresses(static_cast<const widechar *>(host_name), host_port,
				domain == SocketAddressDomain::IPv6 ? ESSE::Communicatio::NetworkAddressDomain::IPv6 : ESSE::Communicatio::NetworkAddressDomain::IPv4);
			SafePointer< Array<AddressEntity> > result = new Array<AddressEntity>(0x10);
			for (auto & a : *query) {
				AddressEntity ae;
				if (a.GetDomain() == ESSE::Communicatio::NetworkAddressDomain::IPv4) {
					ae.EntityAddress = Address::CreateIPv4(0);
					MemoryCopy(&ae.EntityAddress.DWord1, &static_cast<ESSE::Communicatio::NetworkAddressIPv4 *>(&a)->address, 4);
					ae.EntityAddress.DWord1 = InverseEndianess(ae.EntityAddress.DWord1);
					ae.EntityName = host_name;
					ae.EntityDomain = SocketAddressDomain::IPv4;
					result->Append(ae);
				} else if (a.GetDomain() == ESSE::Communicatio::NetworkAddressDomain::IPv6) {
					auto address = static_cast<ESSE::Communicatio::NetworkAddressIPv6 *>(&a);
					ae.EntityAddress.DWord1 = (uint(address->address[6]) << 16) | uint(address->address[7]);
					ae.EntityAddress.DWord2 = (uint(address->address[4]) << 16) | uint(address->address[5]);
					ae.EntityAddress.DWord3 = (uint(address->address[2]) << 16) | uint(address->address[3]);
					ae.EntityAddress.DWord4 = (uint(address->address[0]) << 16) | uint(address->address[1]);
					ae.EntityName = host_name;
					ae.EntityDomain = SocketAddressDomain::IPv6;
					result->Append(ae);
				}
			}
			result->Retain();
			return result;
		}
		AddressEntity::operator string(void) const
		{
			return string(EntityAddress) + U" \"" + EntityName + U"\" " + (EntityDomain == SocketAddressDomain::IPv4 ? U"IPv4" : U"IPv6");
		}
	}
}