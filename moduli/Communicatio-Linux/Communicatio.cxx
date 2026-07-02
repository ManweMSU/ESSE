#include <Communicatio/Communicatio.h>

#include "CommUnix.h"
#include "CommOpenSSL.h"

namespace ESSE
{
	namespace Communicatio
	{
		class NetworkChannel : public INetworkChannel
		{
			oref<INetworkChannel> _interface;
		public:
			NetworkChannel(void) {}
			virtual ~NetworkChannel(void) noexcept {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO _interface ? _interface->ToStringE(ectx) : U"Null Channel"; ESSE_TRY_OUTRO(string()) }
			virtual void Connect(NetworkAddress * address, ErrorContext * error, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override
			{
				if (_interface) _interface->Connect(address, error, hdlr, ectx); else {
					ESSE_TRY_INTRO
					_interface = CreateNetworkChannel(address);
					ESSE_TRY_OUTRO()
					_interface->Connect(address, error, hdlr, ectx);
				}
			}
			virtual void Connect(NetworkAddress * address, NetworkSecurityDesc & sec, ErrorContext * error, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override
			{
				if (_interface) _interface->Connect(address, sec, error, hdlr, ectx); else {
					ESSE_TRY_INTRO
					_interface = CreateNetworkChannelS(address);
					ESSE_TRY_OUTRO()
					_interface->Connect(address, sec, error, hdlr, ectx);
				}
			}
			virtual void Send(DataBlock * data, ErrorContext * error, uintptr * sent, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override
			{
				if (_interface) _interface->Send(data, error, sent, hdlr, ectx);
				else ErrorSet(ectx, 5, 0);
			}
			virtual void Receive(uintptr length, ErrorContext * error, oref<DataBlock> * data, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override
			{
				if (_interface) _interface->Receive(length, error, data, hdlr, ectx);
				else ErrorSet(ectx, 5, 0);
			}
			virtual void Close(bool ultimately, ErrorContext & ectx) noexcept override
			{
				if (_interface) _interface->Close(ultimately, ectx);
				else ErrorSet(ectx, 5, 0);
			}
		};
		class NetworkListener : public INetworkListener
		{
			oref<INetworkListener> _interface;
		public:
			NetworkListener(void) {}
			virtual ~NetworkListener(void) noexcept {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO _interface ? _interface->ToStringE(ectx) : U"Null Listener"; ESSE_TRY_OUTRO(string()) }
			virtual void Bind(NetworkAddress * address, ErrorContext & ectx) noexcept override
			{
				if (_interface) _interface->Bind(address, ectx); else {
					ESSE_TRY_INTRO
					_interface = CreateNetworkListener(address);
					ESSE_TRY_OUTRO()
					_interface->Bind(address, ectx);
				}
			}
			virtual void Bind(NetworkAddress * address, NetworkIdentityDesc & idesc, ErrorContext & ectx) noexcept override
			{
				if (_interface) _interface->Bind(address, idesc, ectx); else {
					ESSE_TRY_INTRO
					_interface = CreateNetworkListenerS(address);
					ESSE_TRY_OUTRO()
					_interface->Bind(address, idesc, ectx);
				}
			}
			virtual void Accept(uintptr limit, ErrorContext * error, oref<INetworkChannel> * channel, oref<NetworkAddress> * address, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override
			{
				if (_interface) _interface->Accept(limit, error, channel, address, hdlr, ectx);
				else ErrorSet(ectx, 5, 0);
			}
			virtual void Close(ErrorContext & ectx) noexcept override
			{
				if (_interface) _interface->Close(ectx);
				else ErrorSet(ectx, 5, 0);
			}
		};
		oref<INetworkChannel> CreateChannel(ErrorContext & ectx) noexcept { ESSE_TRY_INTRO return oref<INetworkChannel>::CreateOwned(new NetworkChannel); ESSE_TRY_OUTRO(0) }
		oref<INetworkListener> CreateListener(ErrorContext & ectx) noexcept { ESSE_TRY_INTRO return oref<INetworkListener>::CreateOwned(new NetworkListener); ESSE_TRY_OUTRO(0) }
		oref<object_array<NetworkAddress>> GetNetworkAddresses(const string & domain_name, uint16 port, NetworkAddressDomain address_domain, ErrorContext & ectx) noexcept { return GetNetworkDomainAddresses(domain_name, port, address_domain, ectx); }
	}
}