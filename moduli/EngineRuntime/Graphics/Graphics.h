#pragma once

#include "PresentationCore.h"
#include "../Streaming.h"
#include "../Miscellaneous/Volumes.h"

namespace Engine
{
	namespace Graphics
	{
		class IDevice;
		class IDeviceChild;
		class IShader;
		class IShaderLibrary;
		class IPipelineState;
		class ISamplerState;
		class IDeviceResource;
		class IBuffer;
		class ITexture;
		class IWindowLayer;
		class IDeviceContext;
		class IDeviceResourceHandle;
		class IDeviceFactory;
		class I2DDeviceContext;

		typedef ESSE::Graphica::DeviceClass DeviceClass;
		typedef ESSE::Graphica::PixelFormatUsage PixelFormatUsage;
		typedef ESSE::Graphica::PixelFormat PixelFormat;
		typedef ESSE::Graphica::SamplerFilter SamplerFilter;
		typedef ESSE::Graphica::SamplerAddressMode SamplerAddressMode;
		typedef ESSE::Graphica::ShaderType ShaderType;
		enum class ShaderError { Success = 0, Unknown = 1, IO = 2, InvalidContainerData = 3, NoCompiler = 4, NoPlatformVersion = 5, Compilation = 6 };
		typedef ESSE::Graphica::BlendingFactor BlendingFactor;
		typedef ESSE::Graphica::BlendingFunction BlendingFunction;
		typedef ESSE::Graphica::CompareFunction CompareFunction;
		typedef ESSE::Graphica::StencilFunction StencilFunction;
		typedef ESSE::Graphica::FillMode FillMode;
		typedef ESSE::Graphica::CullMode CullMode;
		typedef ESSE::Graphica::ResourceType ResourceType;
		enum class ResourceMemoryPool { Default = 0, Immutable = 1, Shared = 2 };
		typedef ESSE::Graphica::TextureType TextureType;
		typedef ESSE::Graphica::PrimitiveTopology PrimitiveTopology;
	 	typedef ESSE::Graphica::TextureLoadAction TextureLoadAction;
		typedef ESSE::Graphica::IndexBufferFormat IndexBufferFormat;
		
		enum RenderTargetFlags : uint {
			RenderTargetFlagBlendingEnabled = 0x00000001,
			RenderTargetFlagRestrictWriteRed = 0x00000002,
			RenderTargetFlagRestrictWriteGreen = 0x00000004,
			RenderTargetFlagRestrictWriteBlue = 0x00000008,
			RenderTargetFlagRestrictWriteAlpha = 0x00000010
		};
		enum DepthStencilFlags : uint {
			DepthStencilFlagDepthTestEnabled = 0x00000001,
			DepthStencilFlagStencilTestEnabled = 0x00000004,
			DepthStencilFlagDepthWriteEnabled = 0x00000002
		};
		enum ResourceUsage : uint {
			ResourceUsageShaderRead = 0x00000001,
			ResourceUsageShaderWrite = 0x00000002,
			ResourceUsageConstantBuffer = 0x00000004,
			ResourceUsageIndexBuffer = 0x00000008,
			ResourceUsageRenderTarget = 0x00000010,
			ResourceUsageDepthStencil = 0x00000020,
			ResourceUsageCPURead = 0x00000040,
			ResourceUsageCPUWrite = 0x00000080,
			ResourceUsageVideoRead = 0x00000100,
			ResourceUsageVideoWrite = 0x00000200,
			ResourceUsageShaderAll = ResourceUsageShaderRead | ResourceUsageShaderWrite,
			ResourceUsageCPUAll = ResourceUsageCPURead | ResourceUsageCPUWrite,
			ResourceUsageVideoAll = ResourceUsageVideoRead | ResourceUsageVideoWrite,
			ResourceUsageBufferMask = ResourceUsageShaderAll | ResourceUsageConstantBuffer | ResourceUsageIndexBuffer | ResourceUsageCPUAll,
			ResourceUsageTextureMask = ResourceUsageShaderAll | ResourceUsageRenderTarget | ResourceUsageDepthStencil | ResourceUsageCPUAll | ResourceUsageVideoAll
		};
		enum WindowLayerAttributes : uint {
			WindowLayerAttributeAlphaChannelIgnore			= 0x01000000,
			WindowLayerAttributeAlphaChannelStraight		= 0x02000000,
			WindowLayerAttributeAlphaChannelPremultiplied	= 0x04000000,
			WindowLayerAttributeExtendedDynamicRange		= 0x08000000,
			WindowLayerAttributeMask						= 0xFF000000
		};

		typedef ESSE::Graphica::SamplerDesc SamplerDesc;
		typedef ESSE::Graphica::RenderTargetDesc RenderTargetDesc;
		typedef ESSE::Graphica::StencilDesc StencilDesc;
		struct DepthStencilDesc
		{
			PixelFormat Format;
			uint32 Flags;
			CompareFunction DepthTestFunction;
			uint8 StencilWriteMask;
			uint8 StencilReadMask;
			StencilDesc FrontStencil;
			StencilDesc BackStencil;
		};
		struct RasterizationDesc
		{
			FillMode Fill;
			CullMode Cull;
			bool FrontIsCounterClockwise;
			int DepthBias;
			float DepthBiasClamp;
			float SlopeScaledDepthBias;
			bool DepthClipEnable;
		};
		struct PipelineStateDesc
		{
			IShader * VertexShader;
			IShader * PixelShader;
			uint32 RenderTargetCount;
			RenderTargetDesc RenderTarget[8];
			DepthStencilDesc DepthStencil;
			RasterizationDesc Rasterization;
			PrimitiveTopology Topology;
		};
		struct BufferDesc
		{
			uint32 Length;
			uint32 Stride;
			uint32 Usage;
			ResourceMemoryPool MemoryPool;
		};
		struct TextureDesc
		{
			TextureType Type;
			PixelFormat Format;
			uint32 Width;
			uint32 Height;
			union { uint32 Depth; uint32 ArraySize; };
			uint32 MipmapCount;
			uint32 Usage;
			ResourceMemoryPool MemoryPool;
		};
		typedef ESSE::Graphica::PresentationLayerDesc WindowLayerDesc;
		typedef ESSE::Graphica::ResourceInitDesc ResourceInitDesc;
		typedef ESSE::Graphica::ResourceDataDesc ResourceDataDesc;
		struct RenderTargetViewDesc
		{
			ITexture * Texture;
			TextureLoadAction LoadAction;
			float ClearValue[4];
		};
		struct DepthStencilViewDesc
		{
			ITexture * Texture;
			TextureLoadAction DepthLoadAction;
			TextureLoadAction StencilLoadAction;
			float DepthClearValue;
			uint8 StencilClearValue;
		};

		class VolumeIndex
		{
		public:
			uint32 x, y, z;
			VolumeIndex(void);
			VolumeIndex(uint32 sx);
			VolumeIndex(uint32 sx, uint32 sy);
			VolumeIndex(uint32 sx, uint32 sy, uint32 sz);
		};
		class SubresourceIndex
		{
		public:
			uint32 mip_level, array_index;
			SubresourceIndex(void);
			SubresourceIndex(uint32 mip, uint32 index);
		};

		class IDevice : public Object
		{
		public:
			virtual string GetDeviceName(void) noexcept = 0;
			virtual uint64 GetDeviceIdentifier(void) noexcept = 0;
			virtual bool DeviceIsValid(void) noexcept = 0;
			virtual void GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) noexcept = 0;
			virtual DeviceClass GetDeviceClass(void) noexcept = 0;
			virtual uint64 GetDeviceMemory(void) noexcept = 0;
			virtual bool GetDevicePixelFormatSupport(PixelFormat format, PixelFormatUsage usage) noexcept = 0;
			virtual IShaderLibrary * LoadShaderLibrary(const void * data, int length) noexcept = 0;
			virtual IShaderLibrary * LoadShaderLibrary(const DataBlock * data) noexcept = 0;
			virtual IShaderLibrary * LoadShaderLibrary(Streaming::Stream * stream) noexcept = 0;
			virtual IShaderLibrary * CompileShaderLibrary(const void * data, int length, ShaderError * error) noexcept = 0;
			virtual IShaderLibrary * CompileShaderLibrary(const DataBlock * data, ShaderError * error) noexcept = 0;
			virtual IShaderLibrary * CompileShaderLibrary(Streaming::Stream * stream, ShaderError * error) noexcept = 0;
			virtual IDeviceContext * GetDeviceContext(void) noexcept = 0;
			virtual IPipelineState * CreateRenderingPipelineState(const PipelineStateDesc & desc) noexcept = 0;
			virtual ISamplerState * CreateSamplerState(const SamplerDesc & desc) noexcept = 0;
			virtual IBuffer * CreateBuffer(const BufferDesc & desc) noexcept = 0;
			virtual IBuffer * CreateBuffer(const BufferDesc & desc, const ResourceInitDesc & init) noexcept = 0;
			virtual ITexture * CreateTexture(const TextureDesc & desc) noexcept = 0;
			virtual ITexture * CreateTexture(const TextureDesc & desc, const ResourceInitDesc * init) noexcept = 0;
			virtual ITexture * CreateRenderTargetView(ITexture * texture, uint32 mip_level, uint32 array_offset_or_depth) noexcept = 0;
			virtual IDeviceResource * OpenResource(IDeviceResourceHandle * handle) noexcept = 0;
			virtual IWindowLayer * CreateWindowLayer(Windows::ICoreWindow * window, const WindowLayerDesc & desc) noexcept = 0;
		};
		class IDeviceChild : public Object
		{
		public:
			virtual IDevice * GetParentDevice(void) noexcept = 0;
		};
		class IShader : public IDeviceChild
		{
		public:
			virtual string GetName(void) noexcept = 0;
			virtual ShaderType GetType(void) noexcept = 0;
		};
		class IShaderLibrary : public IDeviceChild
		{
		public:
			virtual Array<string> * GetShaderNames(void) noexcept = 0;
			virtual IShader * CreateShader(const string & name) noexcept = 0;
		};
		class IPipelineState : public IDeviceChild {};
		class ISamplerState : public IDeviceChild {};
		class IDeviceResource : public IDeviceChild
		{
		public:
			virtual ResourceType GetResourceType(void) noexcept = 0;
			virtual ResourceMemoryPool GetMemoryPool(void) noexcept = 0;
			virtual uint32 GetResourceUsage(void) noexcept = 0;
		};
		class IBuffer : public IDeviceResource
		{
		public:
			virtual uint32 GetLength(void) noexcept = 0;
		};
		class ITexture : public IDeviceResource
		{
		public:
			virtual TextureType GetTextureType(void) noexcept = 0;
			virtual PixelFormat GetPixelFormat(void) noexcept = 0;
			virtual uint32 GetWidth(void) noexcept = 0;
			virtual uint32 GetHeight(void) noexcept = 0;
			virtual uint32 GetDepth(void) noexcept = 0;
			virtual uint32 GetMipmapCount(void) noexcept = 0;
			virtual uint32 GetArraySize(void) noexcept = 0;
		};
		class IWindowLayer : public IDeviceChild
		{
		public:
			virtual bool Present(void) noexcept = 0;
			virtual ITexture * QuerySurface(void) noexcept = 0;
			virtual bool ResizeSurface(uint32 width, uint32 height) noexcept = 0;
			virtual bool SwitchToFullscreen(void) noexcept = 0;
			virtual bool SwitchToWindow(void) noexcept = 0;
			virtual bool IsFullscreen(void) noexcept = 0;
			virtual uint GetLayerAttributes(void) noexcept = 0;
		};
		class IDeviceContext : public IDeviceChild
		{
		public:
			virtual bool BeginRenderingPass(uint32 rtc, const RenderTargetViewDesc * rtv, const DepthStencilViewDesc * dsv) noexcept = 0;
			virtual bool Begin2DRenderingPass(const RenderTargetViewDesc & rtv) noexcept = 0;
			virtual bool BeginMemoryManagementPass(void) noexcept = 0;
			virtual bool EndCurrentPass(void) noexcept = 0;
			virtual void Flush(void) noexcept = 0;

			virtual void SetRenderingPipelineState(IPipelineState * state) noexcept = 0;
			virtual void SetViewport(float top_left_x, float top_left_y, float width, float height, float min_depth, float max_depth) noexcept = 0;
			virtual void SetVertexShaderResource(uint32 at, IDeviceResource * resource) noexcept = 0;
			virtual void SetVertexShaderConstant(uint32 at, IBuffer * buffer) noexcept = 0;
			virtual void SetVertexShaderConstant(uint32 at, const void * data, int length) noexcept = 0;
			virtual void SetVertexShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept = 0;
			virtual void SetPixelShaderResource(uint32 at, IDeviceResource * resource) noexcept = 0;
			virtual void SetPixelShaderConstant(uint32 at, IBuffer * buffer) noexcept = 0;
			virtual void SetPixelShaderConstant(uint32 at, const void * data, int length) noexcept = 0;
			virtual void SetPixelShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept = 0;
			virtual void SetIndexBuffer(IBuffer * index, IndexBufferFormat format) noexcept = 0;
			virtual void SetStencilReferenceValue(uint8 ref) noexcept = 0;
			virtual void DrawPrimitives(uint32 vertex_count, uint32 first_vertex) noexcept = 0;
			virtual void DrawInstancedPrimitives(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance) noexcept = 0;
			virtual void DrawIndexedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex) noexcept = 0;
			virtual void DrawIndexedInstancedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex, uint32 instance_count, uint32 first_instance) noexcept = 0;

			virtual I2DDeviceContext * Get2DContext(void) noexcept = 0;

			virtual void GenerateMipmaps(ITexture * texture) noexcept = 0;
			virtual void CopyResourceData(IDeviceResource * dest, IDeviceResource * src) noexcept = 0;
			virtual void CopySubresourceData(IDeviceResource * dest, SubresourceIndex dest_subres, VolumeIndex dest_origin, IDeviceResource * src, SubresourceIndex src_subres, VolumeIndex src_origin, VolumeIndex size) noexcept = 0;
			virtual void UpdateResourceData(IDeviceResource * dest, SubresourceIndex subres, VolumeIndex origin, VolumeIndex size, const ResourceInitDesc & src) noexcept = 0;
			virtual void QueryResourceData(const ResourceDataDesc & dest, IDeviceResource * src, SubresourceIndex subres, VolumeIndex origin, VolumeIndex size) noexcept = 0;

			virtual bool AcquireSharedResource(IDeviceResource * rsrc) noexcept = 0;
			virtual bool AcquireSharedResource(IDeviceResource * rsrc, uint32 timeout) noexcept = 0;
			virtual bool ReleaseSharedResource(IDeviceResource * rsrc) noexcept = 0;
		};
		class IDeviceResourceHandle : public Object
		{
		public:
			virtual uint64 GetDeviceIdentifier(void) noexcept = 0;
			virtual DataBlock * Serialize(void) noexcept = 0;
		};
		class IDeviceFactory : public Object
		{
		public:
			virtual Volumes::Dictionary<uint64, string> * GetAvailableDevices(void) noexcept = 0;
			virtual IDevice * CreateDevice(uint64 identifier) noexcept = 0;
			virtual IDevice * CreateDefaultDevice(void) noexcept = 0;
			virtual IDeviceResourceHandle * QueryResourceHandle(IDeviceResource * resource) noexcept = 0;
			virtual IDeviceResourceHandle * OpenResourceHandle(const DataBlock * data) noexcept = 0;
		};

		bool IsColorFormat(PixelFormat format);
		bool IsDepthStencilFormat(PixelFormat format);
		int GetFormatChannelCount(PixelFormat format);
		int GetFormatBitsPerPixel(PixelFormat format);
	}
}

#include "GraphicsBase.h"