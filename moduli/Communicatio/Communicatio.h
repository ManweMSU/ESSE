#pragma once

#include <Cor/Classes/CorArray.hxx>
#include <Cor/Tasks/CorTasks.h>

namespace ESSE
{
	namespace Communicatio
	{
		enum class NetworkAddressDomain : uint { Local = 0, IPv4 = 4, IP = IPv4, IPv6 = 6 };
		class NetworkAddress : public Object
		{
		public:
			virtual string ToStringE(ErrorContext & ectx) const noexcept override;
			virtual NetworkAddressDomain GetDomain(void) const noexcept = 0;
			virtual string GetAddressString(void) const = 0;
			virtual void SetAddressString(const string & addr) = 0;
			virtual oref<DataBlock> GetAddressData(void) const = 0;
			virtual void SetAddressData(const void * data, uintptr length) = 0;
		};
		class NetworkAddressLocal : public NetworkAddress
		{
		public:
			string name;
		public:
			NetworkAddressLocal(const string & n);
			virtual ~NetworkAddressLocal(void) override;
			virtual NetworkAddressDomain GetDomain(void) const noexcept override;
			virtual string GetAddressString(void) const override;
			virtual void SetAddressString(const string & addr) override;
			virtual oref<DataBlock> GetAddressData(void) const override;
			virtual void SetAddressData(const void * data, uintptr length) override;
		};
		class NetworkAddressIPv4 : public NetworkAddress
		{
		public:
			uint8 address[4];
			uint16 port;
		public:
			NetworkAddressIPv4(const string & text);
			NetworkAddressIPv4(uint8 a0, uint8 a1, uint8 a2, uint8 a3, uint16 p) noexcept;
			virtual ~NetworkAddressIPv4(void) override;
			virtual NetworkAddressDomain GetDomain(void) const noexcept override;
			virtual string GetAddressString(void) const override;
			virtual void SetAddressString(const string & addr) override;
			virtual oref<DataBlock> GetAddressData(void) const override;
			virtual void SetAddressData(const void * data, uintptr length) override;
			static NetworkAddressIPv4 CreateLoopback(uint16 p) noexcept;
			static NetworkAddressIPv4 CreateAnycast(uint16 p) noexcept;
		};
		class NetworkAddressIPv6 : public NetworkAddress
		{
		public:
			uint16 address[8];
			uint16 port;
		public:
			NetworkAddressIPv6(const string & text);
			NetworkAddressIPv6(uint16 a0, uint16 a1, uint16 a2, uint16 a3, uint16 a4, uint16 a5, uint16 a6, uint16 a7, uint16 p) noexcept;
			virtual ~NetworkAddressIPv6(void) override;
			virtual NetworkAddressDomain GetDomain(void) const noexcept override;
			virtual string GetAddressString(void) const override;
			virtual void SetAddressString(const string & addr) override;
			virtual oref<DataBlock> GetAddressData(void) const override;
			virtual void SetAddressData(const void * data, uintptr length) override;
			static NetworkAddressIPv6 CreateLoopback(uint16 p) noexcept;
			static NetworkAddressIPv6 CreateAnycast(uint16 p) noexcept;
		};
		struct NetworkSecurityDesc
		{
			oref<DataBlock> certificate;
			string domain;
			bool ignore_security;
		};
		struct NetworkIdentityDesc
		{
			oref<DataBlock> data;
			string password;
		};

		class INetworkChannel : public Object
		{
		public:
			virtual void Connect(NetworkAddress * address, ErrorContext * error, IDispatchTask * hdlr, ErrorContext & ectx) noexcept = 0;
			virtual void Connect(NetworkAddress * address, NetworkSecurityDesc & sec, ErrorContext * error, IDispatchTask * hdlr, ErrorContext & ectx) noexcept = 0;
			virtual void Send(DataBlock * data, ErrorContext * error, uintptr * sent, IDispatchTask * hdlr, ErrorContext & ectx) noexcept = 0;
			virtual void Receive(uintptr length, ErrorContext * error, oref<DataBlock> * data, IDispatchTask * hdlr, ErrorContext & ectx) noexcept = 0;
			virtual void Close(bool ultimately, ErrorContext & ectx) noexcept = 0;

			void Connect(NetworkAddress * address, ErrorContext * error, IDispatchTask * hdlr);
			void Connect(NetworkAddress * address, NetworkSecurityDesc & sec, ErrorContext * error, IDispatchTask * hdlr);
			void Send(DataBlock * data, ErrorContext * error, uintptr * sent, IDispatchTask * hdlr);
			void Receive(uintptr length, ErrorContext * error, oref<DataBlock> * data, IDispatchTask * hdlr);
			void Close(bool ultimately);
		};
		class INetworkListener : public Object
		{
		public:
			virtual void Bind(NetworkAddress * address, ErrorContext & ectx) noexcept = 0;
			virtual void Bind(NetworkAddress * address, NetworkIdentityDesc & idesc, ErrorContext & ectx) noexcept = 0;
			virtual void Accept(uintptr limit, ErrorContext * error, oref<INetworkChannel> * channel, oref<NetworkAddress> * address, IDispatchTask * hdlr, ErrorContext & ectx) noexcept = 0;
			virtual void Close(ErrorContext & ectx) noexcept = 0;

			void Bind(NetworkAddress * address);
			void Bind(NetworkAddress * address, NetworkIdentityDesc & idesc);
			void Accept(uintptr limit, ErrorContext * error, oref<INetworkChannel> * channel, oref<NetworkAddress> * address, IDispatchTask * hdlr);
			void Close(void);
		};

		oref<INetworkChannel> CreateChannel(ErrorContext & ectx) noexcept;
		oref<INetworkListener> CreateListener(ErrorContext & ectx) noexcept;
		oref<object_array<NetworkAddress>> GetNetworkAddresses(const string & domain_name, uint16 port, NetworkAddressDomain address_domain, ErrorContext & ectx) noexcept;

		oref<INetworkChannel> CreateChannel(void);
		oref<INetworkListener> CreateListener(void);
		oref<object_array<NetworkAddress>> GetNetworkAddresses(const string & domain_name, uint16 port, NetworkAddressDomain address_domain);

		string ConvertStringToPunycode(const string & text);
		string ConvertStringFromPunycode(const string & text);
		string ConvertDomainToPunycode(const string & text);
		string ConvertDomainFromPunycode(const string & text);
	}
}