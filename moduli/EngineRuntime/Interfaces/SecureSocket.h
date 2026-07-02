#pragma once

#include "Socket.h"

namespace Engine
{
	namespace Network
	{
		class SecurityAuthenticationFailedException : public ESSE::CustomException { public: SecurityAuthenticationFailedException(void); };

		Socket * CreateSecureSocket(SocketAddressDomain domain, const string & verify_host);
	}
}