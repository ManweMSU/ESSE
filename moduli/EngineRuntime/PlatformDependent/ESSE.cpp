#include "ESSE.h"

namespace Engine
{
	namespace ESSEIO
	{
		class WrappedStream : public ESSE::Stream
		{
			SafePointer<Streaming::Stream> _inner;
		public:
			WrappedStream(Streaming::Stream * inner) { _inner.SetRetain(inner); }
			virtual ~WrappedStream(void) override {}
			virtual uintptr ReadE(void * data, uintptr size, ESSE::ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
				try {
					_inner->Read(data, size);
					return size;
				} catch (IO::FileReadEndOfFileException & e) {
					return e.DataRead;
				}
				ESSE_TRY_OUTRO(0)
			}
			virtual uintptr WriteE(const void * data, uintptr size, ESSE::ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
				_inner->Write(data, size);
				return size;
				ESSE_TRY_OUTRO(0)
			}
			virtual uint64 SeekE(int64 position, ESSE::SeekOrigin org, ESSE::ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
				if (org == ESSE::SeekOrigin::Begin) return _inner->Seek(position, Engine::Streaming::Begin);
				else if (org == ESSE::SeekOrigin::Current) return _inner->Seek(position, Engine::Streaming::Current);
				else if (org == ESSE::SeekOrigin::End) return _inner->Seek(position, Engine::Streaming::End);
				else throw InvalidArgumentException();
				ESSE_TRY_OUTRO(0)
			}
			virtual uint64 GetLengthE(ESSE::ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
				return _inner->Length();
				ESSE_TRY_OUTRO(0)
			}
			virtual void SetLengthE(const uint64 & length, ESSE::ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
				_inner->SetLength(length);
				ESSE_TRY_OUTRO()
			}
			virtual void FlushE(ESSE::ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
				_inner->Flush();
				ESSE_TRY_OUTRO()
			}
		};
		ESSE::oref<ESSE::Stream> WrapStream(Streaming::Stream * inner) { return ESSE::oref<ESSE::Stream>::CreateOwned(new WrappedStream(inner)); }
		ESSE::oref<ESSE::Picturae::Picture> WrapFrame(Codec::Frame * frame)
		{
			ESSE::Picturae::PictureDesc desc;
			desc.width = frame->GetWidth();
			desc.height = frame->GetHeight();
			desc.stride = frame->GetScanLineLength();
			desc.format = static_cast<ESSE::Picturae::PixelFormat>(frame->GetPixelFormat());
			if (frame->GetAlphaMode() == Codec::AlphaMode::Premultiplied) desc.alpha_mode = ESSE::Picturae::AlphaMode::Premultiplied;
			else desc.alpha_mode = ESSE::Picturae::AlphaMode::Straight;
			if (frame->GetScanOrigin() == Codec::ScanOrigin::BottomUp) desc.origin = ESSE::Picturae::ScanOrigin::BottomLeft;
			else desc.origin = ESSE::Picturae::ScanOrigin::TopLeft;
			desc.data = frame->GetData();
			desc.palette_size = frame->GetPaletteVolume();
			desc.palette = reinterpret_cast<ESSE::Color *>(frame->GetPalette());
			auto result = ESSE::owrap(new ESSE::Picturae::Picture(desc, ESSE::Picturae::PictureInit::AllocateCopy));
			desc = result->GetDesc();
			for (uint i = 0; i < desc.palette_size; i++) ESSE::swap(desc.palette[i].r, desc.palette[i].b);
			result->GetAttributes().plane = static_cast<uint>(frame->Usage);
			result->GetAttributes().scale_factor = frame->DpiUsage;
			result->GetAttributes().animation_duration = frame->Duration;
			result->GetAttributes().pointer_offset_x = frame->HotPointX;
			result->GetAttributes().pointer_offset_y = frame->HotPointY;
			return result;
		}
		Codec::Frame * WrapFrame(::ESSE::Picturae::Picture * frame)
		{
			auto & desc = frame->GetDesc();
			auto pxf = static_cast<Codec::PixelFormat>(desc.format);
			Codec::AlphaMode am;
			Codec::ScanOrigin org;
			if (desc.alpha_mode == ESSE::Picturae::AlphaMode::Premultiplied) am = Codec::AlphaMode::Premultiplied;
			else am = Codec::AlphaMode::Straight;
			if (desc.origin == ESSE::Picturae::ScanOrigin::BottomLeft) org = Codec::ScanOrigin::BottomUp;
			else org = Codec::ScanOrigin::TopDown;
			SafePointer<Codec::Frame> result = new Codec::Frame(desc.width, desc.height, desc.stride, pxf, am, org);
			MemoryCopy(result->GetData(), desc.data, desc.stride * desc.height);
			if (desc.palette_size) {
				result->SetPaletteVolume(desc.palette_size);
				for (uint i = 0; i < desc.palette_size; i++) result->WritePalette(i, desc.palette[i]);
			}
			result->Usage = static_cast<Codec::FrameUsage>(frame->GetAttributes().plane);
			result->DpiUsage = frame->GetAttributes().scale_factor;
			result->Duration = frame->GetAttributes().animation_duration;
			result->HotPointX = frame->GetAttributes().pointer_offset_x;
			result->HotPointY = frame->GetAttributes().pointer_offset_y;
			result->Retain();
			return result;
		}
	}
}