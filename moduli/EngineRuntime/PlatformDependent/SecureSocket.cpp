#include "../Interfaces/SecureSocket.h"

#include <Communicatio/Communicatio.h>

namespace Engine
{
	namespace Network
	{
		class XSSocket : public Socket
		{
			ESSE::oref<ESSE::Communicatio::INetworkChannel> _channel;
			ESSE::oref<ESSE::Signal> _read_event, _write_event;
			string _domain;
			bool _ipv6;
		public:
			XSSocket(SocketAddressDomain address_domain, const string & domain) : _domain(domain)
			{
				_ipv6 = address_domain == SocketAddressDomain::IPv6;
				_read_event = ESSE::CreateSignal(false);
				_write_event = ESSE::CreateSignal(false);
				if (!_read_event || !_write_event) throw OutOfMemoryException();
			}
			~XSSocket(void) override { if (_channel) _channel->Close(true); }
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
				if (_channel) throw InvalidStateException();
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
				ESSE::Communicatio::NetworkSecurityDesc sec;
				sec.domain = static_cast<const ESSE::unichar32 *>(_domain);
				sec.ignore_security = false;
				_write_event->Reset();
				_channel->Connect(addr, sec, &ectx, ESSE::CreateFunctionalTask([&]() {
					_write_event->Raise();
				}));
				_write_event->Wait();
				ESSE::ErrorThrow(ectx);
			}
			virtual void Bind(const Address & address, uint16 port) override { throw ESSE::NotImplementedException(); }
			virtual void Bind(uint16 port) override { Bind(Address::CreateAny(), port); }
			virtual void Listen(void) override {}
			virtual Socket * Accept() override { Address addr; uint16 port; return Accept(addr, port); }
			virtual Socket * Accept(Address & address, uint16 & port) override { throw ESSE::NotImplementedException(); }
			virtual void Shutdown(bool close_read, bool close_write) override { throw ESSE::NotImplementedException(); }
			virtual bool Wait(int time) override { return true; }
			virtual void SetReadTimeout(int time) override {}
			virtual int GetReadTimeout(void) override { return -1; }
		};

		SecurityAuthenticationFailedException::SecurityAuthenticationFailedException(void) : ESSE::CustomException(ESSE::ErrorMake(ESSE::Errores::ErrorNetwork, ESSE::Errores::SuberrorNetwork::SecurityLayerError)) {}
		Socket * CreateSecureSocket(SocketAddressDomain domain, const string & verify_host) { return new XSSocket(domain, verify_host); }
	}
}