#pragma once

#include "DeviceCairo.h"

#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
#include <Fenestrae-Linux-X11/X11WindowSystem.h>
#endif

namespace ESSE
{
	namespace Cairo
	{
		#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
		class CairoDeviceX11 : public CairoDevice
		{
			oref<X11::XServerConnection> _con;
			oref<X11::XRenderAPI> _xrender_api;
			Windows::IWindow * _window_object;
			X11::Window _window;
			X11::Pixmap _backbuffer;
			X11::GC _blt_context;
			Index2 _backbuffer_size;
			cairo_surface_t _surface;
			Picturae::PixelFormat _esse_format;
			X11::XRenderPictFormat * _xrender_format;
			bool _online;
		public:
			CairoDeviceX11(CairoAPI * api, Graphica::IDeviceContextFactory2D * parent, Windows::IWindow * window, X11::IX11Window * wx11);
			virtual ~CairoDeviceX11(void) override;
			virtual string ToStringE(ErrorContext & ectx) const noexcept override;
			virtual uint32 GetImplementationFeatures(void) noexcept override;
			virtual bool BeginRendering(Graphica::TextureLoadAction load, const Color & clear_color) noexcept override;
			virtual bool EndRendering(void) noexcept override;
			void Reset(void) noexcept;
			void Invalidate(void) noexcept;
			bool Resize(Index2 size) noexcept;
		};
		#endif
	}
}