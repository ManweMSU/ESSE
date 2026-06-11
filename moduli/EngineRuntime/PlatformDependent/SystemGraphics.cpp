#include "../Interfaces/SystemGraphics.h"
#include <Imagines/Imagines.h>
#include "ESSE.h"

namespace Engine
{
	namespace Graphics
	{
		// TODO: REWORK
		// #ifdef ENGINE_LINUX_FULL
		// I2DDeviceContextFactory * CreateDeviceContextFactory(void)
		// {
		// 	auto factory = Cairo::GetCommonDeviceContextFactory();
		// 	if (factory) { factory->Retain(); return factory; } else return 0;
		// }
		// IDeviceFactory * CreateDeviceFactory(void) { return Vulkan::CreateDeviceFactory(); }
		// IDevice * GetCommonDevice(void) { return Vulkan::GetCommonDevice(); }
		// void ResetCommonDevice(void) { return Vulkan::ResetCommonDevice(); }
		// #else
		I2DDeviceContextFactory * CreateDeviceContextFactory(void) { return 0; }
		IDeviceFactory * CreateDeviceFactory(void) { return 0; }
		IDevice * GetCommonDevice(void) { return 0; }
		void ResetCommonDevice(void) {}
		// #endif
		// TODO: END REWORK
	}
	namespace Codec
	{
		class ESSECodecMarshaller : public Codec::ICodec
		{
			static ESSE::ucs1_string _get_esse_format(const string & fmt)
			{
				if (fmt == Codec::ImageFormatDIB) return ESSE::Picturae::ImageFormatDIB;
				else if (fmt == Codec::ImageFormatPNG) return ESSE::Picturae::ImageFormatPNG;
				else if (fmt == Codec::ImageFormatJPEG) return ESSE::Picturae::ImageFormatJPEG;
				else if (fmt == Codec::ImageFormatGIF) return ESSE::Picturae::ImageFormatGIF;
				else if (fmt == Codec::ImageFormatTIFF) return ESSE::Picturae::ImageFormatTIFF;
				else if (fmt == Codec::ImageFormatDDS) return ESSE::Picturae::ImageFormatDDS;
				else if (fmt == Codec::ImageFormatHEIF) return ESSE::Picturae::ImageFormatHEIC;
				else if (fmt == Codec::ImageFormatWindowsIcon) return ESSE::Picturae::ImageFormatWindowsIcon;
				else if (fmt == Codec::ImageFormatWindowsCursor) return ESSE::Picturae::ImageFormatWindowsCursor;
				else if (fmt == Codec::ImageFormatAppleIcon) return ESSE::Picturae::ImageFormatAppleIcon;
				else if (fmt == Codec::ImageFormatEngine) return ESSE::Picturae::ImageFormatESSE;
				else return static_cast<const ESSE::unichar32 *>(fmt);
			}
			static string _get_ert_format(const ESSE::ucs1_string & fmt)
			{
				if (fmt == ESSE::Picturae::ImageFormatDIB) return Codec::ImageFormatDIB;
				else if (fmt == ESSE::Picturae::ImageFormatPNG) return Codec::ImageFormatPNG;
				else if (fmt == ESSE::Picturae::ImageFormatJPEG) return Codec::ImageFormatJPEG;
				else if (fmt == ESSE::Picturae::ImageFormatGIF) return Codec::ImageFormatGIF;
				else if (fmt == ESSE::Picturae::ImageFormatTIFF) return Codec::ImageFormatTIFF;
				else if (fmt == ESSE::Picturae::ImageFormatDDS) return Codec::ImageFormatDDS;
				else if (fmt == ESSE::Picturae::ImageFormatHEIC) return Codec::ImageFormatHEIF;
				else if (fmt == ESSE::Picturae::ImageFormatWindowsIcon) return Codec::ImageFormatWindowsIcon;
				else if (fmt == ESSE::Picturae::ImageFormatWindowsCursor) return Codec::ImageFormatWindowsCursor;
				else if (fmt == ESSE::Picturae::ImageFormatAppleIcon) return Codec::ImageFormatAppleIcon;
				else if (fmt == ESSE::Picturae::ImageFormatESSE) return Codec::ImageFormatEngine;
				else return static_cast<const ESSE::unichar8 *>(fmt);
			}
		public:
			ESSECodecMarshaller(void) {}
			virtual ~ESSECodecMarshaller(void) override {}
			virtual void EncodeFrame(Streaming::Stream * stream, Codec::Frame * frame, const string & format) override
			{
				if (!stream || !frame) throw InvalidArgumentException();
				auto swrp = ESSEIO::WrapStream(stream);
				auto fwrp = ESSEIO::WrapFrame(frame);
				auto fmt = _get_esse_format(format);
				ESSE::Picturae::Encode(swrp, fwrp, fmt);
			}
			virtual void EncodeImage(Streaming::Stream * stream, Codec::Image * image, const string & format) override
			{
				if (!stream || !image) throw InvalidArgumentException();
				auto swrp = ESSEIO::WrapStream(stream);
				auto iwrp = ESSE::owrap(new ESSE::Picturae::Image);
				auto fmt = _get_esse_format(format);
				for (auto & i : image->Frames) iwrp->Append(ESSEIO::WrapFrame(&i));
				ESSE::Picturae::Encode(swrp, iwrp, fmt);
			}
			virtual Codec::Frame * DecodeFrame(Streaming::Stream * stream) override
			{
				if (!stream) throw InvalidArgumentException();
				auto swrp = ESSEIO::WrapStream(stream);
				auto wres = ESSE::Picturae::DecodePicture(swrp);
				SafePointer<Codec::Frame> result = ESSEIO::WrapFrame(wres);
				result->Retain();
				return result;
			}
			virtual Codec::Image * DecodeImage(Streaming::Stream * stream) override
			{
				if (!stream) throw InvalidArgumentException();
				auto swrp = ESSEIO::WrapStream(stream);
				auto wres = ESSE::Picturae::DecodeImage(swrp);
				SafePointer<Codec::Image> result = new Codec::Image;
				for (auto & fw : *wres) {
					SafePointer<Codec::Frame> f = ESSEIO::WrapFrame(&fw);
					result->Frames.Append(f);
				}
				result->Retain();
				return result;
			}
			virtual bool IsImageCodec(void) override { return true; }
			virtual bool IsFrameCodec(void) override { return true; }
			virtual string GetCodecName(void) override { return L"Engine ESSE Codec Marshaller"; }
			virtual string ExamineData(Streaming::Stream * stream) override
			{
				if (!stream) throw InvalidArgumentException();
				auto wrp = ESSEIO::WrapStream(stream);
				return _get_ert_format(ESSE::Picturae::ProbeImageFileFormat(wrp));
			}
			virtual bool CanEncode(const string & format) override
			{
				auto fmt = _get_esse_format(format);
				auto clist = ESSE::Picturae::GetEncodeFormats();
				for (auto & c : *clist) for (auto & f : c.caps) if (f.key == fmt) return (f.value & ESSE::Picturae::Codices::CodecIOMode::Encode) != 0;
				return false;
			}
			virtual bool CanDecode(const string & format) override
			{
				auto fmt = _get_esse_format(format);
				auto clist = ESSE::Picturae::GetEncodeFormats();
				for (auto & c : *clist) for (auto & f : c.caps) if (f.key == fmt) return (f.value & ESSE::Picturae::Codices::CodecIOMode::Decode) != 0;
				return false;
			}
		};
		ESSECodecMarshaller * _esse_codec = 0;
		void InitializeDefaultCodecs(void)
		{
			if (!_esse_codec) {
				SafePointer<ESSECodecMarshaller> codec = new ESSECodecMarshaller;
				_esse_codec = codec.Inner();
			}
		}
	}
}