#pragma once

#include <Cor/Images/CorGraphics.h>

namespace ESSE
{
	namespace Graphica
	{
		namespace PDF
		{
			enum PageFlags : uint {
				PageFlagColorMonochrome	= 0x0000,
				PageFlagColorGrayscale	= 0x0001,
				PageFlagColorFull		= 0x0002,
				PageFlagDontCompress	= 0x0000,
				PageFlagCompress		= 0x4000,
				PageFlagDontInterpolate	= 0x0000,
				PageFlagInterpolate		= 0x8000
			};
			enum class MetadataKey {
				CreatorSoftware = 0x10001,
				EncoderSoftware = 0x10002,
				Title			= 0x10003,
				Author			= 0x10004,
				Subject			= 0x10005,
				Keywords		= 0x10006,
				CreationDate	= 0x20001,
				AlternationDate	= 0x20002,
			};

			class IEncoderContext : public Object
			{
			public:
				virtual bool AddPage(uint physical_width, uint physical_height, Picturae::Picture * data = 0, uint flags = 0) noexcept = 0;
				virtual bool SetMetadata(MetadataKey key, const string & value) noexcept = 0;
				virtual bool SetMetadata(MetadataKey key, const Time & value) noexcept = 0;
				virtual bool FinalizeDocument(void) noexcept = 0;
			};

			oref<IEncoderContext> CreateEncoder(Stream * stream, ErrorContext & ectx) noexcept;
			oref<IEncoderContext> CreateEncoder(Stream * stream);
		}
	}
}