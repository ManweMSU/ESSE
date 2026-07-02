#pragma once

#include <Communicatio/Communicatio.h>

#define REQUEST_ATTRIBUTE_POLL_READ		0x001
#define REQUEST_ATTRIBUTE_POLL_WRITE	0x002
#define REQUEST_ATTRIBUTE_CONNECT_BIT	0x010
#define REQUEST_ATTRIBUTE_ACCEPT_BIT	0x020
#define REQUEST_ATTRIBUTE_RECEIVE_BIT	0x040
#define REQUEST_ATTRIBUTE_SEND_BIT		0x080
#define REQUEST_ATTRIBUTE_REMOVE		0x100
#define REQUEST_ATTRIBUTE_OPENSSL		0x200
#define REQUEST_ATTRIBUTE_PRIORITY		0x400
#define REQUEST_ATTRIBUTE_INCOMPLETE	0x800

#define REQUEST_ATTRIBUTE_POLL_MASK		0x00F
#define REQUEST_ATTRIBUTE_ACTION_MASK	0x0F0

#define REQUEST_ATTRIBUTE_CONNECT	(REQUEST_ATTRIBUTE_CONNECT_BIT	| REQUEST_ATTRIBUTE_POLL_WRITE)
#define REQUEST_ATTRIBUTE_ACCEPT	(REQUEST_ATTRIBUTE_ACCEPT_BIT	| REQUEST_ATTRIBUTE_POLL_READ)
#define REQUEST_ATTRIBUTE_RECEIVE	(REQUEST_ATTRIBUTE_RECEIVE_BIT	| REQUEST_ATTRIBUTE_POLL_READ)
#define REQUEST_ATTRIBUTE_SEND		(REQUEST_ATTRIBUTE_SEND_BIT		| REQUEST_ATTRIBUTE_POLL_WRITE)

namespace ESSE
{
	namespace Communicatio
	{
		struct NetworkRequestInput
		{
			uint attributes;
			intptr length, pointer; // read length, read/write execution pointer
			oref<DataBlock> data; // address structure or data to send
		};
		struct NetworkRequestOutput
		{
			oref<IDispatchTask> handler;
			ErrorContext * status;
			uintptr * length;
			oref<DataBlock> * data;
			oref<NetworkAddress> * address;
			oref<INetworkChannel> * channel;
		};
		struct NetworkRequest
		{
			NetworkRequestInput in;
			NetworkRequestOutput out;
		};
		struct NewNetworkConnection
		{
			int in_socket;
			void * in_address;
			int in_address_length;
			oref<INetworkChannel> out_channel;
			oref<NetworkAddress> out_address;
		};
		enum class OpenSSLEventStatus { None = 0, Failed = 1, PollRenew = 2, CompleteReadRequest = 3, CompleteWriteRequest = 4, CloseChannelWrite = 5, CloseChannel = 6 };
		class IOpenSSLEventHandler : public Object
		{
		public:
			virtual OpenSSLEventStatus ProcessConnection(int pollstat, NetworkRequest * req_read, NetworkRequest * req_write, uint pollattr) noexcept = 0; 
		};

		void SetPosixError(int error, ErrorContext & ectx) noexcept;
		void SetPosixError(ErrorContext & ectx) noexcept;
		void SetDNSError(int error, ErrorContext & ectx) noexcept;

		void SocketAddressInit(DataBlock & dest, NetworkAddress * address, ucs1_string * ulnk);
		void SocketAddressRead(void * src, oref<NetworkAddress> * address);

		void NetworkEngineInit(void);
		void NetworkEngineStop(void) noexcept;
		oref<INetworkChannel> CreateNetworkChannel(NetworkAddress * address);
		oref<INetworkListener> CreateNetworkListener(NetworkAddress * address);
		oref<object_array<NetworkAddress>> GetNetworkDomainAddresses(const string & domain_name, uint16 port, NetworkAddressDomain address_domain, ErrorContext & ectx) noexcept;

		int UnixChannelGetSocket(INetworkChannel * channel) noexcept;
		void UnixChannelEnqueueRequest(INetworkChannel * channel, const NetworkRequest & req);
		void UnixChannelEnterCriticalSection(INetworkChannel * channel) noexcept;
		void UnixChannelLeaveCriticalSection(INetworkChannel * channel) noexcept;
		void UnixChannelSetOpenSSLEventHandler(INetworkChannel * channel, IOpenSSLEventHandler * hdlr) noexcept;
	}
}