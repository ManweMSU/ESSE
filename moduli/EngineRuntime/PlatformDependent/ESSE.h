#pragma once

#include "../Streaming.h"
#include "../ImageCodec/CodecBase.h"

namespace Engine
{
	namespace ESSEIO
	{
		ESSE::oref<ESSE::Stream> WrapStream(Streaming::Stream * inner);
		ESSE::oref<ESSE::Picturae::Picture> WrapFrame(Codec::Frame * frame);
		Codec::Frame * WrapFrame(ESSE::Picturae::Picture * frame);
	}
}