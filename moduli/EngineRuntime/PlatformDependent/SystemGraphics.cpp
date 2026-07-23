#include "../Interfaces/SystemGraphics.h"
#include <Imagines/Imagines.h>
#include <Graphica/Graphica.h>
#include "SystemGraphicsEx.h"
#include "SystemWindowsEx.h"
#include "ESSE.h"

namespace Engine
{
	namespace ESSEIO
	{
		ESSE::string _shared_rsrc_io_public_name;
		ESSE::oref<ESSE::Semaphore> _shared_rsrc_sync;
		ESSE::oref<ESSE::Thread> _shared_rsrc_thread;
		ESSE::oref<ESSE::IPC::IConnectionListener> _shared_rsrc_io;
		ESSE::ObjectDictionary<uint64, ESSE::Graphica::IDeviceResourceHandle> _shared_rsrc_registry;
		int _shared_rsrc_thread_proc(void * arg) noexcept
		{
			while (true) {
				try {
					auto client = _shared_rsrc_io->Accept();
					try {
						uint64 serial;
						if (client->ReceiveData(&serial, sizeof(serial)) != sizeof(serial)) throw InvalidFormatException();
						_shared_rsrc_sync->Wait();
						auto current = _shared_rsrc_registry.GetFirst();
						while (current) {
							auto next = current->GetNext();
							if (current->GetValue().value->GetReferenceCount() == 1) _shared_rsrc_registry.BinaryTree::Remove(current);
							current = next;
						}
						auto object = _shared_rsrc_registry[serial];
						_shared_rsrc_sync->Open();
						if (object) object->Send(client);
					} catch (...) {}
				} catch (...) { return 1; }
			}
			return 0;
		}

		class Shader : public Graphics::IShader
		{
			ESSE::oref<ESSE::Graphica::IShader> _inner;
			Graphics::IDevice * _device;
		public:
			Shader(ESSE::Graphica::IShader * inner, Graphics::IDevice * device) noexcept : _inner(inner), _device(device) {}
			virtual ~Shader(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::IDevice * GetParentDevice(void) noexcept override { return _device; }
			virtual string GetName(void) noexcept override { try { return _inner->GetName().GetData(); } catch (...) { return U""; } }
			virtual Graphics::ShaderType GetType(void) noexcept override { return _inner->GetType(); }
			ESSE::Graphica::IShader * Unwrap(void) const noexcept { return _inner; }
		};
		class ShaderLibrary : public Graphics::IShaderLibrary
		{
			ESSE::oref<ESSE::Graphica::IShaderLibrary> _inner;
			Graphics::IDevice * _device;
		public:
			ShaderLibrary(ESSE::Graphica::IShaderLibrary * inner, Graphics::IDevice * device) noexcept : _inner(inner), _device(device) {}
			virtual ~ShaderLibrary(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::IDevice * GetParentDevice(void) noexcept override { return _device; }
			virtual Array<string> * GetShaderNames(void) noexcept override
			{
				try {
					auto shaders = _inner->EnumerateShaderNames();
					if (!shaders) return 0;
					SafePointer<Array<string>> result = new Array<string>(shaders->GetLength());
					for (auto & s : *shaders) result->Append(s.GetData());
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual Graphics::IShader * CreateShader(const string & name) noexcept override
			{
				try {
					auto inner = _inner->CreateShader(ESSE::ucs1_string(static_cast<const ESSE::unichar32 *>(name)));
					if (!inner) return 0;
					return new Shader(inner, _device);
				} catch (...) { return 0; }
			}
			ESSE::Graphica::IShaderLibrary * Unwrap(void) const noexcept { return _inner; }
		};
		class PipelineState : public Graphics::IPipelineState
		{
			ESSE::oref<ESSE::Graphica::IPipelineState> _inner;
			Graphics::IDevice * _device;
		public:
			PipelineState(ESSE::Graphica::IPipelineState * inner, Graphics::IDevice * device) noexcept : _inner(inner), _device(device) {}
			virtual ~PipelineState(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::IDevice * GetParentDevice(void) noexcept override { return _device; }
			ESSE::Graphica::IPipelineState * Unwrap(void) const noexcept { return _inner; }
		};
		class SamplerState : public Graphics::ISamplerState
		{
			ESSE::oref<ESSE::Graphica::ISamplerState> _inner;
			Graphics::IDevice * _device;
		public:
			SamplerState(ESSE::Graphica::ISamplerState * inner, Graphics::IDevice * device) noexcept : _inner(inner), _device(device) {}
			virtual ~SamplerState(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::IDevice * GetParentDevice(void) noexcept override { return _device; }
			ESSE::Graphica::ISamplerState * Unwrap(void) const noexcept { return _inner; }
		};
		class Buffer : public Graphics::IBuffer
		{
			ESSE::oref<ESSE::Graphica::IBuffer> _inner;
			Graphics::IDevice * _device;
		public:
			Buffer(ESSE::Graphica::IBuffer * inner, Graphics::IDevice * device) noexcept : _inner(inner), _device(device) {}
			virtual ~Buffer(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::IDevice * GetParentDevice(void) noexcept override { return _device; }
			virtual Graphics::ResourceType GetResourceType(void) noexcept override { return _inner->GetResourceType(); }
			virtual Graphics::ResourceMemoryPool GetMemoryPool(void) noexcept override { return static_cast<Graphics::ResourceMemoryPool>(_inner->GetMemoryPool()); }
			virtual uint32 GetResourceUsage(void) noexcept override { return _inner->GetResourceUsage(); }
			virtual uint32 GetLength(void) noexcept override { return _inner->GetLength(); }
			ESSE::Graphica::IBuffer * Unwrap(void) const noexcept { return _inner; }
		};
		class Texture : public Graphics::ITexture
		{
			ESSE::oref<ESSE::Graphica::ITexture> _inner;
			Graphics::IDevice * _device;
		public:
			Texture(ESSE::Graphica::ITexture * inner, Graphics::IDevice * device) noexcept : _inner(inner), _device(device) {}
			virtual ~Texture(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::IDevice * GetParentDevice(void) noexcept override { return _device; }
			virtual Graphics::ResourceType GetResourceType(void) noexcept override { return _inner->GetResourceType(); }
			virtual Graphics::ResourceMemoryPool GetMemoryPool(void) noexcept override { return static_cast<Graphics::ResourceMemoryPool>(_inner->GetMemoryPool()); }
			virtual uint32 GetResourceUsage(void) noexcept override { return _inner->GetResourceUsage(); }
			virtual Graphics::TextureType GetTextureType(void) noexcept override { return _inner->GetTextureType(); }
			virtual Graphics::PixelFormat GetPixelFormat(void) noexcept override { return _inner->GetPixelFormat(); }
			virtual uint32 GetWidth(void) noexcept override { return _inner->GetWidth(); }
			virtual uint32 GetHeight(void) noexcept override { return _inner->GetHeight(); }
			virtual uint32 GetDepth(void) noexcept override { return _inner->GetDepth(); }
			virtual uint32 GetMipmapCount(void) noexcept override { return _inner->GetMipmapCount(); }
			virtual uint32 GetArraySize(void) noexcept override { return _inner->GetArraySize(); }
			ESSE::Graphica::ITexture * Unwrap(void) const noexcept { return _inner; }
		};
		class WindowLayer : public Graphics::IWindowLayer
		{
			ESSE::oref<ESSE::Graphica::IPresentationLayer> _inner;
			Graphics::IDevice * _device;
		public:
			WindowLayer(ESSE::Graphica::IPresentationLayer * inner, Graphics::IDevice * device) noexcept : _inner(inner), _device(device) {}
			virtual ~WindowLayer(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::IDevice * GetParentDevice(void) noexcept override { return _device; }
			virtual bool Present(void) noexcept override { return _inner->Present(); }
			virtual Graphics::ITexture * QuerySurface(void) noexcept override
			{
				try {
					auto texture = _inner->QuerySurface();
					if (!texture) return 0;
					return new Texture(texture, _device);
				} catch (...) { return 0; }
			}
			virtual bool ResizeSurface(uint32 width, uint32 height) noexcept override { return _inner->ResizeSurface(width, height); }
			virtual bool SwitchToFullscreen(void) noexcept override { return _inner->SwitchToFullscreen(); }
			virtual bool SwitchToWindow(void) noexcept override { return _inner->SwitchToWindow(); }
			virtual bool IsFullscreen(void) noexcept override { return _inner->IsFullscreen(); }
			virtual uint GetLayerAttributes(void) noexcept override { return _inner->GetLayerAttributes(); }
			ESSE::Graphica::IPresentationLayer * Unwrap(void) const noexcept { return _inner; }
		};
		class DeviceContext : public Graphics::IDeviceContext
		{
			ESSE::oref<ESSE::Graphica::IDeviceContext> _inner;
			SafePointer<Graphics::I2DDeviceContext> _context_2d;
			Graphics::IDevice * _device;
		private:
			static ESSE::Graphica::IDeviceResource * _expose_resource(Graphics::IDeviceResource * rsrc) noexcept
			{
				if (!rsrc) return 0;
				if (rsrc->GetResourceType() == Graphics::ResourceType::Buffer) return static_cast<Buffer *>(rsrc)->Unwrap();
				else if (rsrc->GetResourceType() == Graphics::ResourceType::Texture) return static_cast<Texture *>(rsrc)->Unwrap();
				else return 0;
			}
		public:
			DeviceContext(ESSE::Graphica::IDeviceContext * inner, Graphics::IDevice * device) noexcept : _inner(inner), _device(device) {}
			virtual ~DeviceContext(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::IDevice * GetParentDevice(void) noexcept override { return _device; }
			virtual bool BeginRenderingPass(uint32 rtc, const Graphics::RenderTargetViewDesc * rtv, const Graphics::DepthStencilViewDesc * dsv) noexcept override
			{
				if (rtc > 8) return false;
				ESSE::Graphica::DepthStencilViewDesc dsve;
				ESSE::Graphica::RenderTargetViewDesc rtve[8];
				for (uint i = 0; i < rtc; i++) {
					rtve[i].Texture = rtv[i].Texture ? static_cast<Texture *>(rtv[i].Texture)->Unwrap() : 0;
					rtve[i].LoadAction = rtv[i].LoadAction;
					MemoryCopy(rtve[i].ClearValue, rtv[i].ClearValue, 16);
				}
				if (dsv) {
					dsve.Texture = dsv->Texture ? static_cast<Texture *>(dsv->Texture)->Unwrap() : 0;
					dsve.DepthLoadAction = dsv->DepthLoadAction;
					dsve.DepthClearValue = dsv->DepthClearValue;
					dsve.StencilLoadAction = dsv->StencilLoadAction;
					dsve.StencilClearValue = dsv->StencilClearValue;
				}
				return _inner->BeginRenderingPass(rtc, rtve, dsv ? &dsve : 0);
			}
			virtual bool Begin2DRenderingPass(const Graphics::RenderTargetViewDesc & rtv) noexcept override
			{
				ESSE::Graphica::RenderTargetViewDesc rtve;
				rtve.Texture = rtv.Texture ? static_cast<Texture *>(rtv.Texture)->Unwrap() : 0;
				rtve.LoadAction = rtv.LoadAction;
				MemoryCopy(rtve.ClearValue, rtv.ClearValue, 16);
				return _inner->BeginRenderingPass2D(rtve);
			}
			virtual bool BeginMemoryManagementPass(void) noexcept override { return _inner->BeginMemoryManagementPass(); }
			virtual bool EndCurrentPass(void) noexcept override { return _inner->EndCurrentPass(); }
			virtual void Flush(void) noexcept override { return _inner->Flush(); }
			virtual void SetRenderingPipelineState(Graphics::IPipelineState * state) noexcept override { _inner->SetRenderingPipelineState(state ? static_cast<PipelineState *>(state)->Unwrap() : 0); }
			virtual void SetViewport(float top_left_x, float top_left_y, float width, float height, float min_depth, float max_depth) noexcept override { _inner->SetViewport(top_left_x, top_left_y, width, height, min_depth, max_depth); }
			virtual void SetVertexShaderResource(uint32 at, Graphics::IDeviceResource * resource) noexcept override { _inner->SetVertexShaderResource(at, _expose_resource(resource)); }
			virtual void SetVertexShaderConstant(uint32 at, Graphics::IBuffer * buffer) noexcept override { _inner->SetVertexShaderConstant(at, buffer ? static_cast<Buffer *>(buffer)->Unwrap() : 0); }
			virtual void SetVertexShaderConstant(uint32 at, const void * data, int length) noexcept override { _inner->SetVertexShaderConstantImmediate(at, data, length); }
			virtual void SetVertexShaderSamplerState(uint32 at, Graphics::ISamplerState * sampler) noexcept override { _inner->SetVertexShaderSamplerState(at, sampler ? static_cast<SamplerState *>(sampler)->Unwrap() : 0); }
			virtual void SetPixelShaderResource(uint32 at, Graphics::IDeviceResource * resource) noexcept override { _inner->SetPixelShaderResource(at, _expose_resource(resource)); }
			virtual void SetPixelShaderConstant(uint32 at, Graphics::IBuffer * buffer) noexcept override { _inner->SetPixelShaderConstant(at, buffer ? static_cast<Buffer *>(buffer)->Unwrap() : 0); }
			virtual void SetPixelShaderConstant(uint32 at, const void * data, int length) noexcept override { _inner->SetPixelShaderConstantImmediate(at, data, length); }
			virtual void SetPixelShaderSamplerState(uint32 at, Graphics::ISamplerState * sampler) noexcept override { _inner->SetPixelShaderSamplerState(at, sampler ? static_cast<SamplerState *>(sampler)->Unwrap() : 0); }
			virtual void SetIndexBuffer(Graphics::IBuffer * index, Graphics::IndexBufferFormat format) noexcept override { _inner->SetIndexBuffer(index ? static_cast<Buffer *>(index)->Unwrap() : 0, format); }
			virtual void SetStencilReferenceValue(uint8 ref) noexcept override { _inner->SetStencilReferenceValue(ref); }
			virtual void DrawPrimitives(uint32 vertex_count, uint32 first_vertex) noexcept override { _inner->DrawPrimitives(vertex_count, first_vertex); }
			virtual void DrawInstancedPrimitives(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance) noexcept override { _inner->DrawInstancedPrimitives(vertex_count, first_vertex, instance_count, first_instance); }
			virtual void DrawIndexedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex) noexcept override { _inner->DrawIndexedPrimitives(index_count, first_index, base_vertex); }
			virtual void DrawIndexedInstancedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex, uint32 instance_count, uint32 first_instance) noexcept override { _inner->DrawIndexedInstancedPrimitives(index_count, first_index, base_vertex, instance_count, first_instance); }
			virtual Graphics::I2DDeviceContext * Get2DContext(void) noexcept override
			{
				if (!_context_2d) {
					ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
					auto inner = ESSE::owrap(reinterpret_cast<ESSE::Graphica::IDeviceContext2D *>(_inner->DynamicCast(ESSE::Classes.IDeviceContext2D, ectx)));
					if (ESSE::ErrorTest(ectx)) return 0;
					_context_2d = WrapContext(inner, _device);
				}
				return _context_2d;
			}
			virtual void GenerateMipmaps(Graphics::ITexture * texture) noexcept override { _inner->GenerateMipmaps(texture ? static_cast<Texture *>(texture)->Unwrap() : 0); }
			virtual void CopyResourceData(Graphics::IDeviceResource * dest, Graphics::IDeviceResource * src) noexcept override { _inner->CopyResourceData(_expose_resource(dest), _expose_resource(src)); }
			virtual void CopySubresourceData(Graphics::IDeviceResource * dest, Graphics::SubresourceIndex dest_subres, Graphics::VolumeIndex dest_origin, Graphics::IDeviceResource * src, Graphics::SubresourceIndex src_subres, Graphics::VolumeIndex src_origin, Graphics::VolumeIndex size) noexcept override
			{
				_inner->CopySubresourceData(
					_expose_resource(dest), ESSE::Index2(dest_subres.mip_level, dest_subres.array_index), ESSE::Index3(dest_origin.x, dest_origin.y, dest_origin.z),
					_expose_resource(src), ESSE::Index2(src_subres.mip_level, src_subres.array_index), ESSE::Index3(src_origin.x, src_origin.y, src_origin.z),
					ESSE::Index3(size.x, size.y, size.z));
			}
			virtual void UpdateResourceData(Graphics::IDeviceResource * dest, Graphics::SubresourceIndex subres, Graphics::VolumeIndex origin, Graphics::VolumeIndex size, const Graphics::ResourceInitDesc & src) noexcept override { _inner->UpdateResourceData( _expose_resource(dest), ESSE::Index2(subres.mip_level, subres.array_index), ESSE::Index3(origin.x, origin.y, origin.z), ESSE::Index3(size.x, size.y, size.z), src); }
			virtual void QueryResourceData(const Graphics::ResourceDataDesc & dest, Graphics::IDeviceResource * src, Graphics::SubresourceIndex subres, Graphics::VolumeIndex origin, Graphics::VolumeIndex size) noexcept override { _inner->QueryResourceData(dest, _expose_resource(src), ESSE::Index2(subres.mip_level, subres.array_index), ESSE::Index3(origin.x, origin.y, origin.z), ESSE::Index3(size.x, size.y, size.z)); }
			virtual bool AcquireSharedResource(Graphics::IDeviceResource * rsrc) noexcept override { return _inner->AcquireSharedResource(_expose_resource(rsrc)); }
			virtual bool AcquireSharedResource(Graphics::IDeviceResource * rsrc, uint32 timeout) noexcept override { return _inner->TryAcquireSharedResource(_expose_resource(rsrc), timeout); }
			virtual bool ReleaseSharedResource(Graphics::IDeviceResource * rsrc) noexcept override { return _inner->ReleaseSharedResource(_expose_resource(rsrc)); }
			ESSE::Graphica::IDeviceContext * Unwrap(void) const noexcept { return _inner; }
		};
		class DeviceResourceHandle : public Graphics::IDeviceResourceHandle
		{
			ESSE::oref<ESSE::Graphica::IDeviceResourceHandle> _inner;
			string _public_name;
		public:
			DeviceResourceHandle(ESSE::Graphica::IDeviceResourceHandle * inner, const string & public_name) noexcept : _inner(inner), _public_name(public_name) {}
			virtual ~DeviceResourceHandle(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual uint64 GetDeviceIdentifier(void) noexcept override { return _inner->GetDeviceIdentifier(); }
			virtual DataBlock * Serialize(void) noexcept override { try { return _public_name.EncodeSequence(Encoding::UTF8, false); } catch (...) { return 0; } }
			ESSE::Graphica::IDeviceResourceHandle * Unwrap(void) const noexcept { return _inner; }
		};
		class PresentationEngine : public Windows::IPresentationEngine
		{
			Windows::ICoreWindow * _window;
		public:
			PresentationEngine(void) noexcept : _window(0) {}
			virtual ~PresentationEngine(void) override {}
			virtual string ToString(void) const override { return U"Presentation Engine"; }
			virtual void Attach(Windows::ICoreWindow * window) override { _window = window; SetWindowUserRenderCallback(window); }
			virtual void Detach(void) override { _window = 0; }
			virtual void Invalidate(void) override { if (_window) InvalidateWindow(_window); }
			virtual void Resize(int width, int height) override {}
		};
		class Device : public Graphics::IDevice
		{
			ESSE::oref<ESSE::Graphica::IDevice> _inner;
			SafePointer<Graphics::IDeviceContext> _context;
		private:
			Graphics::IShaderLibrary * _compile_shader_library(ESSE::Stream * stream, Graphics::ShaderError * error) noexcept
			{
				try {
					auto inner = _inner->CompileShaderLibrary(stream);
					SafePointer<Graphics::IShaderLibrary> result = new ShaderLibrary(inner, this);
					if (error) *error = Graphics::ShaderError::Success;
					result->Retain();
					return result;
				} catch (ESSE::Exception & e) {
					if (error) {
						auto ec = e.GetError();
						if (ec.error_code == ESSE::Errores::ErrorInvalidFormat) *error = Graphics::ShaderError::InvalidContainerData;
						else if (ec.error_code == ESSE::Errores::ErrorIO) *error = Graphics::ShaderError::IO;
						else if (ec.error_code == ESSE::Errores::ErrorNotImplemented) *error = Graphics::ShaderError::NoCompiler;
						else if (ec.error_code == ESSE::Errores::ErrorDynamicLinkage) {
							if (ec.error_subcode == ESSE::Errores::SuberrorDL::NoDedicatedVersion) *error = Graphics::ShaderError::NoPlatformVersion;
							else *error = Graphics::ShaderError::Compilation;
						} else *error = Graphics::ShaderError::Unknown;
					}
					return 0;
				} catch (...) {
					if (error) *error = Graphics::ShaderError::Unknown;
					return 0;
				}
			}
		public:
			Device(ESSE::Graphica::IDevice * inner) noexcept : _inner(inner) { _context = new DeviceContext(_inner->GetPrimaryDeviceContext(), this); }
			virtual ~Device(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual string GetDeviceName(void) noexcept override { try { return _inner->GetDeviceName().GetData(); } catch (...) { return U""; } }
			virtual uint64 GetDeviceIdentifier(void) noexcept override { return _inner->GetDeviceIdentifier(); }
			virtual bool DeviceIsValid(void) noexcept override { return _inner->DeviceIsValid(); }
			virtual void GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) noexcept override
			{
				try {
					ESSE::string t;
					_inner->GetImplementationInfo(t, version_major, version_minor);
					tech = t.GetData();
				} catch (...) { tech = U""; version_major = version_minor = 0; }
			}
			virtual Graphics::DeviceClass GetDeviceClass(void) noexcept override { return _inner->GetDeviceClass(); }
			virtual uint64 GetDeviceMemory(void) noexcept override { return _inner->GetDeviceMemory(); }
			virtual bool GetDevicePixelFormatSupport(Graphics::PixelFormat format, Graphics::PixelFormatUsage usage) noexcept override { return _inner->GetDevicePixelFormatSupport(format, usage); }
			virtual Graphics::IShaderLibrary * LoadShaderLibrary(const void * data, int length) noexcept override
			{
				try {
					auto inner = _inner->LoadShaderLibraryFromData(data, length);
					if (!inner) return 0;
					return new ShaderLibrary(inner, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::IShaderLibrary * LoadShaderLibrary(const DataBlock * data) noexcept override
			{
				try {
					if (!data) return 0;
					auto inner = _inner->LoadShaderLibraryFromData(data->GetBuffer(), data->Length());
					if (!inner) return 0;
					return new ShaderLibrary(inner, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::IShaderLibrary * LoadShaderLibrary(Streaming::Stream * stream) noexcept override
			{
				try {
					if (!stream) return 0;
					auto inner = _inner->LoadShaderLibrary(WrapStream(stream));
					if (!inner) return 0;
					return new ShaderLibrary(inner, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::IShaderLibrary * CompileShaderLibrary(const void * data, int length, Graphics::ShaderError * error) noexcept override
			{
				try {
					return _compile_shader_library(ESSE::StaticMemoryStream::Create(data, length), error);
				} catch (...) { if (error) *error = Graphics::ShaderError::Unknown; return 0; }
			}
			virtual Graphics::IShaderLibrary * CompileShaderLibrary(const DataBlock * data, Graphics::ShaderError * error) noexcept override
			{
				if (!data) { if (error) *error = Graphics::ShaderError::Unknown; return 0; }
				return CompileShaderLibrary(data->GetBuffer(), data->Length(), error);
			}
			virtual Graphics::IShaderLibrary * CompileShaderLibrary(Streaming::Stream * stream, Graphics::ShaderError * error) noexcept override
			{
				try {
					if (!stream) throw InvalidArgumentException();
					return _compile_shader_library(WrapStream(stream), error);
				} catch (...) { if (error) *error = Graphics::ShaderError::Unknown; return 0; }
			}
			virtual Graphics::IDeviceContext * GetDeviceContext(void) noexcept override { return _context; }
			virtual Graphics::IPipelineState * CreateRenderingPipelineState(const Graphics::PipelineStateDesc & desc) noexcept override
			{
				try {
					ESSE::Graphica::PipelineStateDesc idesc;
					idesc.VertexShader = desc.VertexShader ? static_cast<Shader *>(desc.VertexShader)->Unwrap() : 0;
					idesc.PixelShader = desc.PixelShader ? static_cast<Shader *>(desc.PixelShader)->Unwrap() : 0;
					idesc.RenderTargetCount = desc.RenderTargetCount;
					for (uint i = 0; i < desc.RenderTargetCount; i++) idesc.RenderTarget[i] = desc.RenderTarget[i];
					idesc.DepthStencil.Format = desc.DepthStencil.Format;
					idesc.DepthStencil.Flags = desc.DepthStencil.Flags;
					idesc.DepthStencil.DepthTestFunction = desc.DepthStencil.DepthTestFunction;
					idesc.DepthStencil.StencilWriteMask = desc.DepthStencil.StencilWriteMask;
					idesc.DepthStencil.StencilReadMask = desc.DepthStencil.StencilReadMask;
					idesc.DepthStencil.FrontStencil = desc.DepthStencil.FrontStencil;
					idesc.DepthStencil.BackStencil = desc.DepthStencil.BackStencil;
					idesc.Rasterization.Fill = desc.Rasterization.Fill;
					idesc.Rasterization.Cull = desc.Rasterization.Cull;
					idesc.Rasterization.FrontIsCounterClockwise = desc.Rasterization.FrontIsCounterClockwise;
					idesc.Rasterization.DepthBias = desc.Rasterization.DepthBias;
					idesc.Rasterization.DepthBiasClamp = desc.Rasterization.DepthBiasClamp;
					idesc.Rasterization.SlopeScaledDepthBias = desc.Rasterization.SlopeScaledDepthBias;
					idesc.Rasterization.DepthClipEnable = desc.Rasterization.DepthClipEnable;
					idesc.Topology = desc.Topology;
					auto inner = _inner->CreateRenderingPipelineState(idesc);
					if (!inner) return 0;
					return new PipelineState(inner, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::ISamplerState * CreateSamplerState(const Graphics::SamplerDesc & desc) noexcept override
			{
				try {
					auto inner = _inner->CreateSamplerState(desc);
					if (!inner) return 0;
					return new SamplerState(inner, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::IBuffer * CreateBuffer(const Graphics::BufferDesc & desc) noexcept override
			{
				try {
					ESSE::Graphica::BufferDesc idesc;
					idesc.Length = desc.Length;
					idesc.Stride = desc.Stride;
					idesc.Usage = desc.Usage;
					idesc.MemoryPool = static_cast<ESSE::Graphica::ResourceMemoryPool>(desc.MemoryPool);
					auto inner = _inner->CreateBuffer(idesc);
					if (!inner) return 0;
					return new Buffer(inner, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::IBuffer * CreateBuffer(const Graphics::BufferDesc & desc, const Graphics::ResourceInitDesc & init) noexcept override
			{
				try {
					ESSE::Graphica::BufferDesc idesc;
					idesc.Length = desc.Length;
					idesc.Stride = desc.Stride;
					idesc.Usage = desc.Usage;
					idesc.MemoryPool = static_cast<ESSE::Graphica::ResourceMemoryPool>(desc.MemoryPool);
					auto inner = _inner->CreateBufferWithData(idesc, init);
					if (!inner) return 0;
					return new Buffer(inner, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::ITexture * CreateTexture(const Graphics::TextureDesc & desc) noexcept override
			{
				try {
					ESSE::Graphica::TextureDesc idesc;
					idesc.Type = desc.Type;
					idesc.Format = desc.Format;
					idesc.Width = desc.Width;
					idesc.Height = desc.Height;
					idesc.Depth = desc.Depth;
					idesc.MipmapCount = desc.MipmapCount;
					idesc.Usage = desc.Usage;
					idesc.MemoryPool = static_cast<ESSE::Graphica::ResourceMemoryPool>(desc.MemoryPool);
					auto inner = _inner->CreateTexture(idesc);
					if (!inner) return 0;
					return new Texture(inner, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::ITexture * CreateTexture(const Graphics::TextureDesc & desc, const Graphics::ResourceInitDesc * init) noexcept override
			{
				try {
					ESSE::Graphica::TextureDesc idesc;
					idesc.Type = desc.Type;
					idesc.Format = desc.Format;
					idesc.Width = desc.Width;
					idesc.Height = desc.Height;
					idesc.Depth = desc.Depth;
					idesc.MipmapCount = desc.MipmapCount;
					idesc.Usage = desc.Usage;
					idesc.MemoryPool = static_cast<ESSE::Graphica::ResourceMemoryPool>(desc.MemoryPool);
					auto inner = _inner->CreateTextureWithData(idesc, init);
					if (!inner) return 0;
					return new Texture(inner, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::ITexture * CreateRenderTargetView(Graphics::ITexture * texture, uint32 mip_level, uint32 array_offset_or_depth) noexcept override
			{
				try {
					auto inner = _inner->CreateRenderTargetView(texture ? static_cast<Texture *>(texture)->Unwrap() : 0, mip_level, array_offset_or_depth);
					if (!inner) return 0;
					return new Texture(inner, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::IDeviceResource * OpenResource(Graphics::IDeviceResourceHandle * handle) noexcept override
			{
				try {
					if (!handle) return 0;
					auto rsrc = _inner->OpenResource(static_cast<DeviceResourceHandle *>(handle)->Unwrap());
					if (!rsrc) return 0;
					if (rsrc->GetResourceType() == Graphics::ResourceType::Buffer) return new Buffer(static_cast<ESSE::Graphica::IBuffer *>(rsrc.Inner()), this);
					else if (rsrc->GetResourceType() == Graphics::ResourceType::Texture) return new Texture(static_cast<ESSE::Graphica::ITexture *>(rsrc.Inner()), this);
					else return 0;
				} catch (...) { return 0; }
			}
			virtual Graphics::IWindowLayer * CreateWindowLayer(Windows::ICoreWindow * window, const Graphics::WindowLayerDesc & desc) noexcept override
			{
				try {
					auto inner = _inner->CreatePresentationLayer(reinterpret_cast<ESSE::Windows::IWindow *>(window->GetOSHandle()), desc);
					if (!inner) return 0;
					SafePointer<Windows::IPresentationEngine> engine = new PresentationEngine;
					window->SetPresentationEngine(engine);
					return new WindowLayer(inner, this);
				} catch (...) { return 0; }
			}
			ESSE::Graphica::IDevice * Unwrap(void) const noexcept { return _inner; }
		};
		class DeviceFactory : public Graphics::IDeviceFactory
		{
			ESSE::oref<ESSE::Graphica::IDeviceFactory> _inner;
		private:
			static ESSE::Graphica::IDeviceResource * _expose_resource(Graphics::IDeviceResource * rsrc) noexcept
			{
				if (!rsrc) return 0;
				if (rsrc->GetResourceType() == Graphics::ResourceType::Buffer) return static_cast<Buffer *>(rsrc)->Unwrap();
				else if (rsrc->GetResourceType() == Graphics::ResourceType::Texture) return static_cast<Texture *>(rsrc)->Unwrap();
				else return 0;
			}
		public:
			DeviceFactory(ESSE::Graphica::IDeviceFactory * inner) noexcept : _inner(inner) {}
			virtual ~DeviceFactory(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Volumes::Dictionary<uint64, string> * GetAvailableDevices(void) noexcept override
			{
				try {
					auto devlist = _inner->EnumerateDevices();
					if (!devlist) return 0;
					SafePointer<Volumes::Dictionary<uint64, string>> result = new Volumes::Dictionary<uint64, string>;
					for (auto & d : *devlist) result->Append(d.key, d.value.GetData());
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual Graphics::IDevice * CreateDevice(uint64 identifier) noexcept override
			{
				try {
					auto inner = _inner->CreateDevice(identifier);
					if (!inner) return 0;
					return new Device(inner);
				} catch (...) { return 0; }
			}
			virtual Graphics::IDevice * CreateDefaultDevice(void) noexcept override
			{
				try {
					auto inner = _inner->CreateDefaultDevice();
					if (!inner) return 0;
					return new Device(inner);
				} catch (...) { return 0; }
			}
			virtual Graphics::IDeviceResourceHandle * QueryResourceHandle(Graphics::IDeviceResource * resource) noexcept override
			{
				try {
					if (!resource) return 0;
					auto inner = _inner->QueryResourceHandle(_expose_resource(resource));
					if (!inner) return 0;
					if (!_shared_rsrc_sync) {
						_shared_rsrc_sync = ESSE::CreateSemaphore(1);
						if (!_shared_rsrc_sync) throw OutOfMemoryException();
					}
					if (!_shared_rsrc_io) {
						uint serial = 0;
						while (true) {
							ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
							_shared_rsrc_io_public_name = ESSE::FormatString(U"ert_shared_resource_%0", serial++);
							_shared_rsrc_io = ESSE::IPC::CreateConnectionListener(_shared_rsrc_io_public_name, ESSE::IPC::ConnectionMode::ConnectionModeRegular, ectx);
							if (ectx.error_code == ESSE::Errores::ErrorIO && ectx.error_subcode == ESSE::Errores::SuberrorIO::FileExists) continue;
							else if (ectx.error_code == ESSE::Errores::ErrorSuccess) break;
							else ESSE::ErrorThrow(ectx);
						}
						if (!_shared_rsrc_io) throw InvalidStateException();
					}
					if (!_shared_rsrc_thread) {
						_shared_rsrc_thread = ESSE::CreateThread(_shared_rsrc_thread_proc);
						if (!_shared_rsrc_thread) throw OutOfMemoryException();
					}
					uint64 registry_serial;
					_shared_rsrc_sync->Wait();
					try {
						auto current = _shared_rsrc_registry.GetFirst();
						while (current) {
							auto next = current->GetNext();
							if (current->GetValue().value->GetReferenceCount() == 1) _shared_rsrc_registry.BinaryTree::Remove(current);
							current = next;
						}
						registry_serial = reinterpret_cast<uintptr>(inner.Inner());
						while (_shared_rsrc_registry.ElementExists(registry_serial)) registry_serial++;
						_shared_rsrc_registry.Append(registry_serial, inner);
					} catch (...) { _shared_rsrc_sync->Open(); throw; }
					_shared_rsrc_sync->Open();
					return new DeviceResourceHandle(inner, string(_shared_rsrc_io_public_name.GetData()) + U"/" + string(registry_serial, HexadecimalBase, 16));
				} catch (...) { return 0; }
			}
			virtual Graphics::IDeviceResourceHandle * OpenResourceHandle(const DataBlock * data) noexcept override
			{
				try {
					if (!data) return 0;
					auto path = ESSE::string(data->GetBuffer(), data->Length(), ESSE::Unicode::Encoding::UTF8);
					auto names = ESSE::SplitString(path, U'/');
					if (names.GetLength() != 2) return 0;
					uint64 serial = names[1].ToUInt64(HexadecimalBase);
					auto io = ESSE::IPC::Connect(names[0], ESSE::IPC::ConnectionMode::ConnectionModeRegular);
					if (io->SendData(&serial, sizeof(serial)) != sizeof(serial)) throw InvalidStateException();
					auto inner = _inner->ReceiveResourceHandle(io);
					return new DeviceResourceHandle(inner, path.GetData());
				} catch (...) { return 0; }
			}
		};

		class Bitmap : public Graphics::IBitmap
		{
			ESSE::oref<ESSE::Graphica::IBitmap> _inner;
		public:
			Bitmap(ESSE::Graphica::IBitmap * inner) noexcept : _inner(inner) {}
			virtual ~Bitmap(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual int GetWidth(void) const noexcept override { return _inner->GetWidth(); }
			virtual int GetHeight(void) const noexcept override { return _inner->GetHeight(); }
			virtual bool Reload(Codec::Frame * source) noexcept override
			{
				try {
					auto factory = ESSE::Graphica::CreateDeviceContextFactory2D();
					_inner = factory->LoadBitmap(WrapFrame(source));
					return true;
				} catch (...) { return false; }
			}
			virtual bool AddDeviceBitmap(Graphics::IDeviceBitmap * bitmap, Graphics::I2DDeviceContext * device_for) noexcept override { return false; }
			virtual bool RemoveDeviceBitmap(Graphics::I2DDeviceContext * device_for) noexcept override { return false; }
			virtual Graphics::IDeviceBitmap * GetDeviceBitmap(Graphics::I2DDeviceContext * device_for) const noexcept override { return 0; }
			virtual Graphics::IBitmapLink * GetLinkObject(void) const noexcept override { return 0; }
			virtual Codec::Frame * QueryFrame(void) const noexcept override { try { return WrapFrame(_inner->QueryContents()); } catch (...) { return 0; } }
			ESSE::Graphica::IBitmap * Unwrap(void) const noexcept { return _inner; }
		};
		class Font : public Graphics::IFont
		{
			ESSE::oref<ESSE::Graphica::AggregateFont> _inner;
			uint _style;
		public:
			Font(ESSE::Graphica::AggregateFont * inner, uint style) noexcept : _inner(inner), _style(style) {}
			virtual ~Font(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual int GetWidth(void) const noexcept override
			{
				uint glyph;
				_inner->GetPrimaryFont()->GetGlyphsForCharacters(U" ", &glyph, 1);
				if (glyph == ESSE::Graphica::InvalidGlyph) return 0;
				ESSE::Graphica::FontGlyphMetrics gm;
				_inner->GetPrimaryFont()->GetGlyphMetrics(&glyph, &gm, 1);
				return gm.HorizontalAdvance;
			}
			virtual int GetHeight(void) const noexcept override { return _inner->GetHeight(); }
			virtual int GetLineSpacing(void) const noexcept override
			{
				ESSE::Graphica::FontMetrics fm;
				_inner->GetFontMetrics(fm);
				return fm.Ascent - fm.Descent + fm.LineSpacing;
			}
			virtual int GetBaselineOffset(void) const noexcept override
			{
				ESSE::Graphica::FontMetrics fm;
				_inner->GetFontMetrics(fm);
				return fm.Ascent;
			}
			ESSE::Graphica::AggregateFont * Unwrap(void) const noexcept { return _inner; }
			uint GetStyle(void) const noexcept { return _style; }
		};
		class ColorBrush : public Graphics::IColorBrush
		{
			ESSE::oref<ESSE::Graphica::IColorBrush> _inner;
			Point _rel_from, _rel_to;
			Graphics::I2DDeviceContext * _parent;
		public:
			ColorBrush(ESSE::Graphica::IColorBrush * inner, Graphics::I2DDeviceContext * parent, Point rel_from, Point rel_to) noexcept : _inner(inner), _parent(parent), _rel_from(rel_from), _rel_to(rel_to) {}
			virtual ~ColorBrush(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::I2DDeviceContext * GetParentDevice(void) const noexcept override { return _parent; }
			ESSE::Graphica::IColorBrush * Unwrap(void) const noexcept { return _inner; }
			Point GetGradientPointA(void) const noexcept { return _rel_from; }
			Point GetGradientPointB(void) const noexcept { return _rel_to; }
		};
		class BlurEffectBrush : public Graphics::IBlurEffectBrush
		{
			ESSE::oref<ESSE::Graphica::IBlurEffectBrush> _inner;
			Graphics::I2DDeviceContext * _parent;
		public:
			BlurEffectBrush(ESSE::Graphica::IBlurEffectBrush * inner, Graphics::I2DDeviceContext * parent) noexcept : _inner(inner), _parent(parent) {}
			virtual ~BlurEffectBrush(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::I2DDeviceContext * GetParentDevice(void) const noexcept override { return _parent; }
			ESSE::Graphica::IBlurEffectBrush * Unwrap(void) const noexcept { return _inner; }
		};
		class InversionEffectBrush : public Graphics::IInversionEffectBrush
		{
			ESSE::oref<ESSE::Graphica::IInversionEffectBrush> _inner;
			Graphics::I2DDeviceContext * _parent;
		public:
			InversionEffectBrush(ESSE::Graphica::IInversionEffectBrush * inner, Graphics::I2DDeviceContext * parent) noexcept : _inner(inner), _parent(parent) {}
			virtual ~InversionEffectBrush(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::I2DDeviceContext * GetParentDevice(void) const noexcept override { return _parent; }
			ESSE::Graphica::IInversionEffectBrush * Unwrap(void) const noexcept { return _inner; }
		};
		class BitmapBrush : public Graphics::IBitmapBrush
		{
			ESSE::oref<ESSE::Graphica::IBitmapBrush> _inner;
			Graphics::I2DDeviceContext * _parent;
		public:
			BitmapBrush(ESSE::Graphica::IBitmapBrush * inner, Graphics::I2DDeviceContext * parent) noexcept : _inner(inner), _parent(parent) {}
			virtual ~BitmapBrush(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::I2DDeviceContext * GetParentDevice(void) const noexcept override { return _parent; }
			ESSE::Graphica::IBitmapBrush * Unwrap(void) const noexcept { return _inner; }
		};
		class TextBrush : public Graphics::ITextBrush
		{
			friend class DeviceContext2D;
		private:
			ESSE::oref<ESSE::Graphica::Typesetter> _inner;
			ESSE::oref<ESSE::Graphica::IColorBrush> _background;
			Graphics::I2DDeviceContext * _parent;
			ESSE::Color _primary, _selection;
			ESSE::array<ESSE::Color> _palette;
			uint _ha, _va;
			int _select_from, _select_to;
		public:
			TextBrush(ESSE::Graphica::Typesetter * inner, Graphics::I2DDeviceContext * parent, Color primary, uint ha, uint va) noexcept : _inner(inner), _parent(parent), _primary(primary.Value), _selection(0), _palette(1), _ha(ha), _va(va), _select_from(-1), _select_to(-1) {}
			virtual ~TextBrush(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::I2DDeviceContext * GetParentDevice(void) const noexcept override { return _parent; }
			virtual void GetExtents(int & width, int & height) noexcept override { auto e = _inner->GetExtents(); width = e.x; height = e.y; }
			virtual void SetHighlightColor(const Color & color) noexcept override { _selection = color.Value; _background.Clear(); }
			virtual void HighlightText(int start, int end) noexcept override { _select_from = start; _select_to = end; }
			virtual int TestPosition(int point) noexcept override
			{
				if (point < 0) return 0;
				for (uint i = 0; i < _inner->GetTextLength(); i++) {
					double a, b, c;
					_inner->GetGlyphRectangles(i, 1, &a, &b, 0, 0);
					c = (a + b) / 2.0;
					if (point <= c) return i;
					else if (point <= b) return i + 1;
				}
				return _inner->GetTextLength();
			}
			virtual int EndOfChar(int index) noexcept override
			{
				if (index < 0) return 0;
				if (index >= _inner->GetTextLength()) return _inner->GetExtents().x;
				double p;
				_inner->GetGlyphRectangles(index, 1, 0, &p, 0, 0);
				return p;
			}
			virtual int GetStringLength(void) noexcept override { return _inner->GetTextLength(); }
			virtual void SetCharPalette(const Color * colors, int count) override { _palette.SetLength(count); for (uint i = 0; i < count; i++) _palette[i] = colors[i].Value; }
			virtual void SetCharColors(const uint8 * indicies, int count) override { for (uint i = 0; i < count; i++) { auto clr = indicies[i] ? _palette[1 + indicies[i]] : _primary; _inner->SetColors(&clr, i, 1); } }
			virtual void SetCharAdvances(const double * advances) override { _inner->SetAdvances(advances, 0, _inner->GetTextLength()); }
			virtual void GetCharAdvances(double * advances) noexcept override
			{
				for (uint i = 0; i < _inner->GetTextLength(); i++) {
					double a, b;
					_inner->GetGlyphRectangles(i, 1, &a, &b, 0, 0);
					advances[i] = b - a;
				}
			}
			ESSE::Graphica::Typesetter * Unwrap(void) const noexcept { return _inner; }
		};
		class DeviceContext2D : public Graphics::IBitmapContext
		{
			ESSE::oref<ESSE::Graphica::IDeviceContextFactory2D> _factory;
			ESSE::oref<ESSE::Graphica::IDeviceContext2D> _inner;
			ESSE::oref<ESSE::Graphica::DeviceCache> _cache;
			ESSE::object_array<ESSE::Graphica::ILayerBacking> _layers;
			SafePointer<Graphics::IDevice> _controlling_device;
			uint _current_time, _caret_ref_time, _caret_blink_time, _caret_blink_time_2, _current_layer_index;
			bool _bitmap;
		private:
			void _init(ESSE::Graphica::IDeviceContext2D * inner, Graphics::IDevice * device, ESSE::Graphica::DeviceCache * cache)
			{
				_inner = inner;
				_controlling_device.SetRetain(device);
				_cache = cache ? ESSE::oref<ESSE::Graphica::DeviceCache>(cache) : ESSE::owrap(new ESSE::Graphica::DeviceCache(_inner));
				_current_time = _caret_ref_time = 0;
				_caret_blink_time = 1000;
				_caret_blink_time_2 = 500;
				_current_layer_index = 0;
			}
		public:
			DeviceContext2D(ESSE::Graphica::IDeviceContextFactory2D * factory, ESSE::Graphica::IDeviceContext2D * inner, Graphics::IDevice * device) : _factory(factory), _layers(0x10), _bitmap(false) { _init(inner, device, 0); }
			DeviceContext2D(ESSE::Graphica::IDeviceContextFactory2D * factory, ESSE::Graphica::IDeviceContext2D * inner, ESSE::Graphica::DeviceCache * cache) : _factory(factory), _layers(0x10), _bitmap(false) { _init(inner, 0, cache); }
			DeviceContext2D(ESSE::Graphica::IDeviceContextFactory2D * factory) : _factory(factory), _layers(0x10), _bitmap(true) { _init(0, 0, 0); }
			virtual ~DeviceContext2D(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual void GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) override
			{
				try {
					if (!_inner) throw InvalidStateException();
					ESSE::string t;
					_inner->GetImplementationInfo(t, version_major, version_minor);
					tech = t.GetData();
				} catch (...) { tech = U""; version_major = version_minor = 0; }
			}
			virtual uint32 GetFeatureList(void) noexcept override
			{
				uint flags = _inner ? _inner->GetImplementationFeatures() : 0;
				uint result = 0;
				if (flags & ESSE::Graphica::DeviceContextSupportsBlurEffect) result |= Graphics::DeviceContextFeatureBlurCapable;
				if (flags & ESSE::Graphica::DeviceContextSupportsInversionEffect) result |= Graphics::DeviceContextFeatureInversionCapable;
				if (flags & ESSE::Graphica::DeviceContextSupportsPolygons) result |= Graphics::DeviceContextFeaturePolygonCapable;
				if (flags & ESSE::Graphica::DeviceContextSupportsLayers) result |= Graphics::DeviceContextFeatureLayersCapable;
				if (flags & ESSE::Graphica::DeviceContextHardwareAccelerated) result |= Graphics::DeviceContextFeatureHardware;
				if (flags & ESSE::Graphica::DeviceContextHasControllingDevice) result |= Graphics::DeviceContextFeatureGraphicsInteropEnabled;
				if (flags & ESSE::Graphica::DeviceContextBitmapContext) result |= Graphics::DeviceContextFeatureBitmapTarget;
				return result;
			}
			virtual Graphics::IColorBrush * CreateSolidColorBrush(Color color) noexcept override
			{
				try {
					if (!_inner) return 0;
					auto brush = _cache->CreateSolidColorBrush(color.Value);
					if (!brush) return 0;
					return new ColorBrush(brush, this, Point(0, 0), Point(0, 0));
				} catch (...) { return 0; }
			}
			virtual Graphics::IColorBrush * CreateGradientBrush(Point rel_from, Point rel_to, const GradientPoint * points, int count) noexcept override
			{
				try {
					if (count == 0) return 0;
					else if (count == 1) return CreateSolidColorBrush(points[0].Value);
					if (!_inner) return 0;
					ESSE::array<ESSE::Color> colors(count);
					ESSE::array<double> positions(count);
					for (uint i = 0; i < count; i++) { colors.Append(points[i].Value.Value); positions.Append(points[i].Position); }
					auto brush = _inner->CreateGradientBrush(ESSE::Index2(0, 0), ESSE::Index2(1, 0), colors, positions, count);
					if (!brush) return 0;
					return new ColorBrush(brush, this, rel_from, rel_to);
				} catch (...) { return 0; }
			}
			virtual Graphics::IBlurEffectBrush * CreateBlurEffectBrush(double power) noexcept override
			{
				try {
					if (!_inner) return 0;
					auto brush = _cache->CreateBlurEffectBrush(power);
					if (!brush) return 0;
					return new BlurEffectBrush(brush, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::IInversionEffectBrush * CreateInversionEffectBrush(void) noexcept override
			{
				try {
					if (!_inner) return 0;
					auto brush = _cache->CreateInversionEffectBrush();
					if (!brush) return 0;
					return new InversionEffectBrush(brush, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::IBitmapBrush * CreateBitmapBrush(Graphics::IBitmap * bitmap, const Box & area, bool tile) noexcept override
			{
				try {
					if (!_inner || !bitmap) return 0;
					auto brush = tile ? _cache->CreateTileBrush(static_cast<Bitmap *>(bitmap)->Unwrap(), ESSE::Rectangle(area.Left, area.Top, area.Right, area.Bottom)) : _cache->CreateBitmapBrush(static_cast<Bitmap *>(bitmap)->Unwrap(), ESSE::Rectangle(area.Left, area.Top, area.Right, area.Bottom));
					if (!brush) return 0;
					return new BitmapBrush(brush, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::IBitmapBrush * CreateTextureBrush(Graphics::ITexture * texture, Graphics::TextureAlphaMode mode) noexcept override
			{
				try {
					if (!_inner || !texture) return 0;
					auto brush = _inner->CreateTextureBrush(static_cast<Texture *>(texture)->Unwrap(), mode == Graphics::TextureAlphaMode::Premultiplied ? ESSE::Graphica::TextureAlphaMode::Premultiplied : ESSE::Graphica::TextureAlphaMode::Ignore);
					if (!brush) return 0;
					return new BitmapBrush(brush, this);
				} catch (...) { return 0; }
			}
			virtual Graphics::ITextBrush * CreateTextBrush(Graphics::IFont * font, const string & text, uint32 horizontal_align, uint32 vertical_align, const Color & color) noexcept override { return CreateTextBrush(font, reinterpret_cast<const uint32 *>(static_cast<const ESSE::unichar32 *>(text)), text.Length(), horizontal_align, vertical_align, color); }
			virtual Graphics::ITextBrush * CreateTextBrush(Graphics::IFont * font, const uint32 * ucs, int length, uint32 horizontal_align, uint32 vertical_align, const Color & color) noexcept override
			{
				try {
					if (!_inner || !font) return 0;
					uint style = static_cast<Font *>(font)->GetStyle();
					auto ts = ESSE::owrap(new ESSE::Graphica::Typesetter(static_cast<Font *>(font)->Unwrap(), ESSE::string(reinterpret_cast<const ESSE::unichar32 *>(ucs), length), style, color.Value));
					return new TextBrush(ts, this, color.Value, horizontal_align, vertical_align);
				} catch (...) { return 0; }
			}
			virtual void ClearInternalCache(void) noexcept override { _layers.Clear(); _current_layer_index = 0; _cache->ResetCache(); }
			virtual void PushClip(const Box & rect) noexcept override { if (_inner) _inner->PushClip(ESSE::Rectangle(rect.Left, rect.Top, rect.Right, rect.Bottom)); }
			virtual void PopClip(void) noexcept override { if (_inner) _inner->PopClip(); }
			virtual void BeginLayer(const Box & rect, double opacity) noexcept override
			{
				if (!_inner || _current_layer_index > _layers.GetLength()) return;
				if (_current_layer_index == _layers.GetLength()) {
					auto layer = _inner->CreateLayerBackingStorage();
					if (!layer) return;
					try { _layers.Append(layer); } catch (...) { return; }
				}
				if (_inner->BeginLayer(_layers(_current_layer_index), ESSE::Rectangle(rect.Left, rect.Top, rect.Right, rect.Bottom), opacity)) _current_layer_index++;
			}
			virtual void EndLayer(void) noexcept override
			{
				if (!_inner || _current_layer_index >= _layers.GetLength() || !_current_layer_index) return;
				_current_layer_index--;
				_inner->EndLayer(_layers(_current_layer_index));
			}
			virtual void Render(Graphics::IColorBrush * brush, const Box & at) noexcept override
			{
				if (!brush || !_inner) return;
				auto b = static_cast<ColorBrush *>(brush);
				auto i = b->Unwrap();
				i->OverrideGradientPoints(ESSE::Index2(at.Left + b->GetGradientPointA().x, at.Top + b->GetGradientPointA().y), ESSE::Index2(at.Left + b->GetGradientPointB().x, at.Top + b->GetGradientPointB().y));
				_inner->Render(i, ESSE::Rectangle(at.Left, at.Top, at.Right, at.Bottom));
			}
			virtual void Render(Graphics::IBitmapBrush * brush, const Box & at) noexcept override
			{
				if (!brush || !_inner) return;
				auto b = static_cast<BitmapBrush *>(brush);
				auto i = b->Unwrap();
				_inner->Render(i, ESSE::Rectangle(at.Left, at.Top, at.Right, at.Bottom));
			}
			virtual void Render(Graphics::ITextBrush * brush, const Box & at, bool clip) noexcept override
			{
				if (!brush || !_inner) return;
				auto b = static_cast<TextBrush *>(brush);
				auto rect = ESSE::Rectangle(at.Left, at.Top, at.Right, at.Bottom);
				if (clip) _inner->PushClip(rect);
				auto e = b->_inner->GetExtents();
				int dx, dy;
				if (b->_ha == 1) dx = (rect.left + rect.right - e.x) / 2;
				else if (b->_ha == 2) dx = rect.right - e.x;
				else dx = rect.left;
				if (b->_va == 1) dy = (rect.top + rect.bottom - e.y) / 2;
				else if (b->_va == 2) dy = rect.bottom - e.y;
				else dy = rect.top;
				if (b->_select_from >= 0 && b->_select_to >= 0) {
					if (!b->_background) b->_background = _cache->CreateSolidColorBrush(b->_selection);
					int left = brush->EndOfChar(b->_select_from - 1), right = brush->EndOfChar(b->_select_to - 1);
					_inner->Render(b->_background, ESSE::Rectangle(dx + left, rect.top, dx + right, rect.bottom));
				}
				b->_inner->Render(_inner, _cache, ESSE::Rectangle(dx, dy, dx, dy));
				if (clip) _inner->PopClip();
			}
			virtual void Render(Graphics::IBlurEffectBrush * brush, const Box & at) noexcept override
			{
				if (!brush || !_inner) return;
				auto b = static_cast<BlurEffectBrush *>(brush);
				auto i = b->Unwrap();
				_inner->Render(i, ESSE::Rectangle(at.Left, at.Top, at.Right, at.Bottom));
			}
			virtual void Render(Graphics::IInversionEffectBrush * brush, const Box & at, bool blink) noexcept override
			{
				if (!brush || !_inner) return;
				if (blink && !IsCaretVisible()) return;
				auto b = static_cast<InversionEffectBrush *>(brush);
				auto i = b->Unwrap();
				_inner->Render(i, ESSE::Rectangle(at.Left, at.Top, at.Right, at.Bottom));
			}
			virtual void RenderPolyline(const Math::Vector2 * points, int count, Color color, double width) noexcept override
			{
				try {
					if (!_inner) return;
					auto brush = _cache->CreateSolidColorBrush(color.Value);
					if (!brush) return;
					ESSE::array<double> x(count + 1), y(count + 1);
					for (uint i = 0; i < count; i++) { x.Append(points[i].x); y.Append(points[i].y); }
					_inner->RenderPolyline(x, y, count, false, brush, width);
				} catch (...) {}
			}
			virtual void RenderPolygon(const Math::Vector2 * points, int count, Color color) noexcept override
			{
				try {
					if (!_inner) return;
					auto brush = _cache->CreateSolidColorBrush(color.Value);
					if (!brush) return;
					ESSE::array<double> x(count + 1), y(count + 1);
					for (uint i = 0; i < count; i++) { x.Append(points[i].x); y.Append(points[i].y); }
					_inner->RenderPolygon(x, y, count, brush);
				} catch (...) {}
			}
			virtual void SetAnimationTime(uint32 value) noexcept override { _current_time = value; }
			virtual uint32 GetAnimationTime(void) noexcept override { return _current_time; }
			virtual void SetCaretReferenceTime(uint32 value) noexcept override { _caret_ref_time = value; }
			virtual uint32 GetCaretReferenceTime(void) noexcept override { return _caret_ref_time; }
			virtual void SetCaretBlinkPeriod(uint32 value) noexcept override { _caret_blink_time = value; _caret_blink_time_2 = _caret_blink_time >> 1U; }
			virtual uint32 GetCaretBlinkPeriod(void) noexcept override { return _caret_blink_time; }
			virtual bool IsCaretVisible(void) noexcept override { return (((_current_time - _caret_ref_time) / _caret_blink_time_2) & 1U) == 0; }
			virtual Graphics::IDevice * GetParentDevice(void) noexcept override { return _controlling_device; }
			virtual Graphics::I2DDeviceContextFactory * GetParentFactory(void) noexcept override { SafePointer<Graphics::I2DDeviceContextFactory> result = Graphics::CreateDeviceContextFactory(); return result; }
			virtual bool BeginRendering(Graphics::IBitmap * dest) noexcept override
			{
				if (!_bitmap || _inner || !dest) return false;
				auto surface = _factory->CreateBitmapContext(static_cast<Bitmap *>(dest)->Unwrap());
				if (!surface) return false;
				try { _init(surface, 0, 0); } catch (...) { _inner.Clear(); _cache.Clear(); return false; }
				return _inner->BeginRendering(ESSE::Graphica::TextureLoadAction::Load, 0);
			}
			virtual bool BeginRendering(Graphics::IBitmap * dest, Color clear_color) noexcept override
			{
				if (!_bitmap || _inner || !dest) return false;
				auto surface = _factory->CreateBitmapContext(static_cast<Bitmap *>(dest)->Unwrap());
				if (!surface) return false;
				try { _init(surface, 0, 0); } catch (...) { _inner.Clear(); _cache.Clear(); return false; }
				return _inner->BeginRendering(ESSE::Graphica::TextureLoadAction::Clear, clear_color.Value);
			}
			virtual bool EndRendering(void) noexcept override
			{
				if (!_bitmap || !_inner) return false;
				auto status = _inner->EndRendering();
				ClearInternalCache();
				_inner.Clear(); _cache.Clear();
				return status;
			}
			ESSE::Graphica::IDeviceContext2D * Unwrap(void) const noexcept { return _inner; }
		};
		class DeviceContextFactory2D : public Graphics::I2DDeviceContextFactory
		{
			ESSE::oref<ESSE::Graphica::IDeviceContextFactory2D> _inner;
		public:
			DeviceContextFactory2D(ESSE::Graphica::IDeviceContextFactory2D * inner) : _inner(inner) {}
			virtual ~DeviceContextFactory2D(void) override {}
			virtual string ToString(void) const override { return _inner->ToString().GetData(); }
			virtual Graphics::IBitmap * CreateBitmap(int width, int height, Color clear_color) noexcept override
			{
				try {
					auto inner = _inner->CreateBitmap(width, height, clear_color.Value);
					if (!inner) return 0;
					return new Bitmap(inner);
				} catch (...) { return 0; }
			}
			virtual Graphics::IBitmap * LoadBitmap(Codec::Frame * source) noexcept override
			{
				try {
					if (!source) return 0;
					auto inner = _inner->LoadBitmap(WrapFrame(source));
					if (!inner) return 0;
					return new Bitmap(inner);
				} catch (...) { return 0; }
			}
			virtual Graphics::IFont * LoadFont(const string & face_name, int height, int weight, bool italic, bool underline, bool strikeout) noexcept override
			{
				try {
					ESSE::string face;
					uint style = 0, style_ex = 0;
					if (face_name[0] == U'@') {
						bool serif = false;
						bool sans_serif = false;
						bool monospace = false;
						auto parts = face_name.Fragment(1, -1).Split(U' ');
						for (auto & p : parts) if (p.Length()) {
							if (string::CompareIgnoreCase(p, Graphics::FontWordSerif) == 0) serif = true;
							else if (string::CompareIgnoreCase(p, Graphics::FontWordSans) == 0) sans_serif = true;
							else if (string::CompareIgnoreCase(p, Graphics::FontWordMono) == 0) monospace = true;
							else return 0;
						}
						if (!serif) return 0;
						style = ESSE::Graphica::CreateFontSystemDefault;
						if (sans_serif) style |= ESSE::Graphica::CreateFontSansSerif;
						if (monospace) style |= ESSE::Graphica::CreateFontMonospace;
					} else face = static_cast<const ESSE::unichar32 *>(face_name);
					if (weight < 150) style |= ESSE::Graphica::CreateFontWeight100;
					else if (weight < 250) style |= ESSE::Graphica::CreateFontWeight200;
					else if (weight < 350) style |= ESSE::Graphica::CreateFontWeight300;
					else if (weight < 450) style |= ESSE::Graphica::CreateFontWeight400;
					else if (weight < 550) style |= ESSE::Graphica::CreateFontWeight500;
					else if (weight < 650) style |= ESSE::Graphica::CreateFontWeight600;
					else if (weight < 750) style |= ESSE::Graphica::CreateFontWeight700;
					else if (weight < 850) style |= ESSE::Graphica::CreateFontWeight800;
					else style |= ESSE::Graphica::CreateFontWeight900;
					if (italic) style |= ESSE::Graphica::CreateFontItalic;
					if (underline) style_ex |= ESSE::Graphica::TypesetterFlags::Underlined;
					if (strikeout) style_ex |= ESSE::Graphica::TypesetterFlags::Strikedout;
					auto inner = _inner->CreateFont(face, style, height);
					if (!inner) return 0;
					auto aggregate = ESSE::owrap(new ESSE::Graphica::AggregateFont(_inner, inner));
					return new Font(aggregate, style_ex);
				} catch (...) { return 0; }
			}
			virtual Array<string> * GetFontFamilies(void) noexcept override
			{
				try {
					auto list = _inner->EnumerateFontFamilies();
					if (!list) return 0;
					SafePointer<Array<string>> result = new Array<string>(list->GetLength());
					for (auto & ff : *list) result->Append(ff.GetData());
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual Graphics::IBitmapContext * CreateBitmapContext(void) noexcept override { try { return new DeviceContext2D(_inner); } catch (...) { return 0; } }
		};

		Graphics::I2DDeviceContext * WrapContext(ESSE::Graphica::IDeviceContext2D * context, Graphics::IDevice * controlling_device) noexcept
		{
			try {
				auto factory = ESSE::Graphica::CreateDeviceContextFactory2D();
				return new DeviceContext2D(factory, context, controlling_device);
			} catch (...) { return 0; }
		}
		Graphics::I2DDeviceContext * WrapContext(ESSE::Graphica::IDeviceContext2D * context, ESSE::Graphica::DeviceCache * cache) noexcept
		{
			try {
				auto factory = ESSE::Graphica::CreateDeviceContextFactory2D();
				return new DeviceContext2D(factory, context, cache);
			} catch (...) { return 0; }
		}
		Graphics::IDevice * WrapDevice(ESSE::Graphica::IDevice * device) noexcept { try { return new Device(device); } catch (...) { return 0; } }
		ESSE::Graphica::IDevice * UnwrapDevice(Graphics::IDevice * device) noexcept { return static_cast<Device *>(device)->Unwrap(); }
		ESSE::Graphica::IDeviceContext2D * UnwrapContext(Graphics::I2DDeviceContext * context) noexcept { return static_cast<DeviceContext2D *>(context)->Unwrap(); }
	}
	namespace Graphics
	{
		SafePointer<IDeviceFactory> _common_factory;
		SafePointer<I2DDeviceContextFactory> _common_factory_2d;
		SafePointer<IDevice> _common_device;

		I2DDeviceContextFactory * CreateDeviceContextFactory(void)
		{
			if (!_common_factory_2d) try {
				auto factory = ESSE::Graphica::CreateDeviceContextFactory2D();
				_common_factory_2d = new ESSEIO::DeviceContextFactory2D(factory);
			} catch (...) { return 0; }
			if (_common_factory_2d) _common_factory_2d->Retain();
			return _common_factory_2d;
		}
		IDeviceFactory * CreateDeviceFactory(void)
		{
			if (!_common_factory) try {
				auto factory = ESSE::Graphica::CreateDeviceFactory();
				_common_factory = new ESSEIO::DeviceFactory(factory);
			} catch (...) { return 0; }
			if (_common_factory) _common_factory->Retain();
			return _common_factory;
		}
		IDevice * GetCommonDevice(void)
		{
			if (!_common_device) {
				SafePointer<IDeviceFactory> factory = CreateDeviceFactory();
				_common_device = factory->CreateDefaultDevice();
			}
			return _common_device;
		}
		void ResetCommonDevice(void)
		{
			_common_device.SetReference(0);
			GetCommonDevice();
		}
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