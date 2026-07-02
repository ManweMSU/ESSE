#pragma once

#include <Communicatio/Communicatio.h>

namespace ESSE
{
	namespace Communicatio
	{
		oref<INetworkChannel> CreateNetworkChannelS(NetworkAddress * address);
		oref<INetworkListener> CreateNetworkListenerS(NetworkAddress * address);
	}
}