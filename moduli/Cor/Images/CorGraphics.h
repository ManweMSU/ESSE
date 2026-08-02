#pragma once

#include "CorImages.h"
#include "../IO/CorStreams.h"
#include "../IO/CorIPC.h"
#include "../Classes/CorVolume.hxx"

namespace ESSE
{
	class Index2
	{
	public:
		int x, y;
	public:
		Index2(void) noexcept;
		Index2(int sx, int sy) noexcept;
		~Index2(void);
		operator string (void) const;

		Index2 operator - (void) const noexcept;
		Index2 & operator += (const Index2 & a) noexcept;
		Index2 & operator -= (const Index2 & a) noexcept;
		Index2 & operator *= (const Index2 & a) noexcept;
		Index2 & operator /= (const Index2 & a) noexcept;
		Index2 & operator %= (const Index2 & a) noexcept;

		friend Index2 operator + (const Index2 & a, const Index2 & b) noexcept;
		friend Index2 operator - (const Index2 & a, const Index2 & b) noexcept;
		friend Index2 operator * (const Index2 & a, const Index2 & b) noexcept;
		friend Index2 operator / (const Index2 & a, const Index2 & b) noexcept;
		friend Index2 operator % (const Index2 & a, const Index2 & b) noexcept;

		friend bool operator == (const Index2 & a, const Index2 & b) noexcept;
		friend bool operator != (const Index2 & a, const Index2 & b) noexcept;
		friend bool operator <= (const Index2 & a, const Index2 & b) noexcept;
		friend bool operator >= (const Index2 & a, const Index2 & b) noexcept;
		friend bool operator < (const Index2 & a, const Index2 & b) noexcept;
		friend bool operator > (const Index2 & a, const Index2 & b) noexcept;
	};
	class Index3
	{
	public:
		int x, y, z;
	public:
		Index3(void) noexcept;
		Index3(int sx, int sy, int sz) noexcept;
		Index3(const Index2 & sxy, int sz) noexcept;
		Index3(int sx, const Index2 & syz) noexcept;
		~Index3(void);
		operator string (void) const;

		Index3 operator - (void) const noexcept;
		Index3 & operator += (const Index3 & a) noexcept;
		Index3 & operator -= (const Index3 & a) noexcept;
		Index3 & operator *= (const Index3 & a) noexcept;
		Index3 & operator /= (const Index3 & a) noexcept;
		Index3 & operator %= (const Index3 & a) noexcept;

		friend Index3 operator + (const Index3 & a, const Index3 & b) noexcept;
		friend Index3 operator - (const Index3 & a, const Index3 & b) noexcept;
		friend Index3 operator * (const Index3 & a, const Index3 & b) noexcept;
		friend Index3 operator / (const Index3 & a, const Index3 & b) noexcept;
		friend Index3 operator % (const Index3 & a, const Index3 & b) noexcept;

		friend bool operator == (const Index3 & a, const Index3 & b) noexcept;
		friend bool operator != (const Index3 & a, const Index3 & b) noexcept;
		friend bool operator <= (const Index3 & a, const Index3 & b) noexcept;
		friend bool operator >= (const Index3 & a, const Index3 & b) noexcept;
		friend bool operator < (const Index3 & a, const Index3 & b) noexcept;
		friend bool operator > (const Index3 & a, const Index3 & b) noexcept;
	};
	class Rectangle
	{
	public:
		int left, top, right, bottom;
	public:
		Rectangle(void) noexcept;
		Rectangle(int l, int t, int r, int b) noexcept;
		Rectangle(const Index2 & lt, const Index2 & rb) noexcept;
		~Rectangle(void);
		operator string (void) const;

		bool IsInside(const Index2 & i) const noexcept;

		bool friend operator == (const Rectangle & a, const Rectangle & b) noexcept;
		bool friend operator != (const Rectangle & a, const Rectangle & b) noexcept;

		static Rectangle Intersect(const Rectangle & a, const Rectangle & b) noexcept;
		static Rectangle OuterRectangle(const Rectangle & a, const Rectangle & b) noexcept;
	};

	namespace Graphica
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
		class IPresentationLayer;
		class IDeviceContext;
		class IDeviceResourceHandle;
		class IDeviceFactory;
		
		class IDeviceContext2D;
		class IDeviceContextFactory2D;
		class IBitmap;
		class IFont;

		enum class DeviceClass : uint { Unknown = 0, Integrated = 1, Discrete = 2, Software = 3 };
		enum class PixelFormatUsage : uint { ShaderRead = 0, ShaderSample = 1, RenderTarget = 2, BlendRenderTarget = 3, DepthStencil = 4, WindowSurface = 5, RenderTarget2D = 6, BitmapSource = 7, VideoIO = 8 };
		enum class PixelFormat : uint {
			Invalid = 0x00000000,
			// Color formats
			// 8 bpp
			A8_unorm = 0x81100001,
			R8_unorm = 0x81100002, R8_snorm = 0x81100003, R8_uint = 0x81100004, R8_sint = 0x81100005,

			// 16 bpp
			R16_unorm = 0x82100001, R16_snorm = 0x82100002, R16_uint = 0x82100003, R16_sint = 0x82100004, R16_float = 0x82100005,
			R8G8_unorm = 0x82200001, R8G8_snorm = 0x82200002, R8G8_uint = 0x82200003, R8G8_sint = 0x82200004,
			B5G6R5_unorm = 0x82300001, R5G6B5_unorm = 0x82300002,
			B5G5R5A1_unorm = 0x82400001, R5G5B5A1_unorm = 0x82400002, A1B5G5R5_unorm = 0x82400003, A1R5G5B5_unorm = 0x82400004,
			B4G4R4A4_unorm = 0x82400005, R4G4B4A4_unorm = 0x82400006, A4B4G4R4_unorm = 0x82400007, A4R4G4B4_unorm = 0x82400008,

			// 32 bpp
			R32_uint = 0x83100001, R32_sint = 0x83100002, R32_float = 0x83100003,
			R16G16_unorm = 0x83200001, R16G16_snorm = 0x83200002, R16G16_uint = 0x83200003, R16G16_sint = 0x83200004, R16G16_float = 0x83200005,
			B8G8R8A8_unorm = 0x83400001,
			R8G8B8A8_unorm = 0x83400002, R8G8B8A8_snorm = 0x83400003, R8G8B8A8_uint = 0x83400004, R8G8B8A8_sint = 0x83400005,
			R10G10B10A2_unorm = 0x83400006, R10G10B10A2_uint = 0x83400007, A2R10G10B10_unorm = 0x83400008, A2R10G10B10_uint = 0x83400009,
			R11G11B10_float = 0x83300001, B10G11R11_float = 0x83300003,
			R9G9B9E5_float = 0x83300002, E5B9G9R9_float = 0x83300004,

			// 64 bpp
			R32G32_uint = 0x84200001, R32G32_sint = 0x84200002, R32G32_float = 0x84200003,
			R16G16B16A16_unorm = 0x84400001, R16G16B16A16_snorm = 0x84400002, R16G16B16A16_uint = 0x84400003, R16G16B16A16_sint = 0x84400004, R16G16B16A16_float = 0x84400005,

			// 128 bpp
			R32G32B32A32_uint = 0x85400001, R32G32B32A32_sint = 0x85400002, R32G32B32A32_float = 0x85400003,

			// Depth/Stencil formats
			D16_unorm = 0x42100001, D24_unorm = 0x42100002, D32_float = 0x43100001, D16S8_unorm = 0x42200001, D24S8_unorm = 0x43200001, D32S8_float = 0x44200001
		};
		enum class SamplerFilter : uint { Point = 0, Linear = 1, Anisotropic = 2 };
		enum class SamplerAddressMode : uint { Wrap = 0, Mirror = 1, Clamp = 2, Border = 3 };
		enum class ShaderType : uint { Unknown = 0, Vertex = 1, Pixel = 2 };
		enum class BlendingFactor : uint {
			Zero = 0, One = 1,
			OverColor = 2, InvertedOverColor = 3, OverAlpha = 4, InvertedOverAlpha = 5,
			BaseColor = 6, InvertedBaseColor = 7, BaseAlpha = 8, InvertedBaseAlpha = 9,
			SecondaryColor = 10, InvertedSecondaryColor = 11, SecondaryAlpha = 12, InvertedSecondaryAlpha = 13,
			OverAlphaSaturated = 14
		};
		enum class BlendingFunction : uint { Add = 0, SubtractOverFromBase = 1, SubtractBaseFromOver = 2, Min = 3, Max = 4 };
		enum class CompareFunction : uint { Always = 0, Lesser = 1, Greater = 2, Equal = 3, LesserEqual = 4, GreaterEqual = 5, NotEqual = 6, Never = 7 };
		enum class StencilFunction : uint { Keep = 0, SetZero = 1, Replace = 2, IncrementWrap = 3, DecrementWrap = 4, IncrementClamp = 5, DecrementClamp = 6, Invert = 7 };
		enum class FillMode : uint { Solid = 0, Wireframe = 1 };
		enum class CullMode : uint { None = 0, Front = 1, Back = 2 };
		enum class ResourceType : uint { Buffer = 0, Texture = 1 };
		enum class ResourceMemoryPool : uint { Regular = 0, Immutable = 1, Shared = 2 };
		enum class TextureType : uint { Type1D = 0, TypeArray1D = 1, Type2D = 2, TypeArray2D = 3, TypeCube = 4, TypeArrayCube = 5, Type3D = 6 };
		enum class PrimitiveTopology : uint { PointList = 0, LineList = 1, LineStrip = 2, TriangleList = 3, TriangleStrip = 4 };
		enum class IndexBufferFormat : uint { UInt16 = 0, UInt32 = 1 };
		enum class TextureLoadAction : uint { DontCare = 0, Load = 1, Clear = 2 };
		enum class TextureAlphaMode : uint { Ignore = 0, Straight = 1, Premultiplied = 2 };
		enum class BrushType : uint { Unknown = 0, Color = 1, Bitmap = 2, Blur = 3, Inversion = 4 };
		
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
			WindowLayerAttributePresentModeDontCare			= 0x00000000,
			WindowLayerAttributePresentModeImmediate		= 0x10000000,
			WindowLayerAttributePresentModeSequential		= 0x20000000,
			WindowLayerAttributePresentModeSynchronous		= 0x30000000,
			WindowLayerAttributePresentModeMask				= 0x30000000,
			WindowLayerAttributeMask						= 0xFF000000
		};
		enum CreateFontFlags : uint {
			CreateFontWeight100		= 0x001,
			CreateFontWeight200		= 0x002,
			CreateFontWeight300		= 0x003,
			CreateFontWeight400		= 0x004,
			CreateFontWeight500		= 0x005,
			CreateFontWeight600		= 0x006,
			CreateFontWeight700		= 0x007,
			CreateFontWeight800		= 0x008,
			CreateFontWeight900		= 0x009,
			CreateFontWeightMask	= 0x00F,
			CreateFontItalic		= 0x010,
			CreateFontOblique		= 0x020,
			CreateFontSystemDefault	= 0x100,
			CreateFontSansSerif		= 0x200,
			CreateFontMonospace		= 0x400,
		};
		enum DeviceContextFeature : uint {
			DeviceContextSupportsBlurEffect			= 0x00000001,
			DeviceContextSupportsInversionEffect	= 0x00000002,
			DeviceContextSupportsPolygons			= 0x00000004,
			DeviceContextSupportsLayers				= 0x00000008,
			DeviceContextPresentationContext		= 0x10000000,
			DeviceContextHardwareAccelerated		= 0x20000000,
			DeviceContextHasControllingDevice		= 0x40000000,
			DeviceContextBitmapContext				= 0x80000000,
		};

		struct SamplerDesc
		{
			SamplerFilter MinificationFilter;
			SamplerFilter MagnificationFilter;
			SamplerFilter MipFilter;
			SamplerAddressMode AddressU;
			SamplerAddressMode AddressV;
			SamplerAddressMode AddressW;
			uint32 MaximalAnisotropy;
			float MinimalLOD;
			float MaximalLOD;
			float BorderColor[4];
		};
		struct RenderTargetDesc
		{
			PixelFormat Format;
			uint32 Flags;
			BlendingFunction BlendRGB;
			BlendingFunction BlendAlpha;
			BlendingFactor BaseFactorRGB;
			BlendingFactor BaseFactorAlpha;
			BlendingFactor OverFactorRGB;
			BlendingFactor OverFactorAlpha;
		};
		struct StencilDesc
		{
			CompareFunction TestFunction;
			StencilFunction OnStencilTestFailed;
			StencilFunction OnDepthTestFailed;
			StencilFunction OnTestsPassed;
		};
		struct DepthStencilDesc
		{
			PixelFormat Format;
			uint32 Flags;
			CompareFunction DepthTestFunction;
			uint32 StencilWriteMask;
			uint32 StencilReadMask;
			StencilDesc FrontStencil;
			StencilDesc BackStencil;
		};
		struct RasterizationDesc
		{
			FillMode Fill;
			CullMode Cull;
			uint FrontIsCounterClockwise;
			int DepthBias;
			float DepthBiasClamp;
			float SlopeScaledDepthBias;
			uint DepthClipEnable;
		};
		struct PipelineStateDesc
		{
			IShader * VertexShader;
			IShader * PixelShader;
			uint32 RenderTargetCount;
			PrimitiveTopology Topology;
			RenderTargetDesc RenderTarget[8];
			DepthStencilDesc DepthStencil;
			RasterizationDesc Rasterization;
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
		struct PresentationLayerDesc
		{
			PixelFormat Format;
			uint32 Width;
			uint32 Height;
			uint32 Usage;
		};
		struct ResourceInitDesc
		{
			const void * Data;
			uintptr DataPitch;
			uintptr DataSlicePitch;
		};
		struct ResourceDataDesc
		{
			void * Data;
			uintptr DataPitch;
			uintptr DataSlicePitch;
		};
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
			uint32 StencilClearValue;
		};
		struct FontMetrics
		{
			double Ascent;
			double Descent;
			double LineSpacing;
			double UnderlinePosition;
			double UnderlineWidth;
			double StrikeoutPosition;
			double StrikeoutWidth;
		};
		struct FontGlyphMetrics
		{
			double HorizontalAdvance;
			double HorizontalLeftBearing;
			double HorizontalRightBearing;
			double HorizontalTopBearing;
			double HorizontalBottomBearing;
		};

		class IDevice : public Object
		{
		public:
			virtual const string & GetDeviceName(void) noexcept = 0;
			virtual uint64 GetDeviceIdentifier(void) noexcept = 0;
			virtual bool DeviceIsValid(void) noexcept = 0;
			virtual void GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) noexcept = 0;
			virtual DeviceClass GetDeviceClass(void) noexcept = 0;
			virtual uint64 GetDeviceMemory(void) noexcept = 0;
			virtual bool GetDevicePixelFormatSupport(PixelFormat format, PixelFormatUsage usage) noexcept = 0;
			virtual IDeviceContext * GetPrimaryDeviceContext(void) noexcept = 0;
			virtual oref<IDeviceContext> CreateDeferredDeviceContext(void) noexcept = 0;
			virtual oref<IShaderLibrary> LoadShaderLibraryFromData(const void * data, uintptr length, ErrorContext & ectx) noexcept = 0;
			virtual oref<IShaderLibrary> LoadShaderLibrary(Stream * stream, ErrorContext & ectx) noexcept = 0;
			virtual oref<IShaderLibrary> CompileShaderLibraryFromData(const void * data, uintptr length, ErrorContext & ectx) noexcept = 0;
			virtual oref<IShaderLibrary> CompileShaderLibrary(Stream * stream, ErrorContext & ectx) noexcept = 0;
			virtual oref<IPipelineState> CreateRenderingPipelineState(const PipelineStateDesc & desc) noexcept = 0;
			virtual oref<ISamplerState> CreateSamplerState(const SamplerDesc & desc) noexcept = 0;
			virtual oref<IBuffer> CreateBuffer(const BufferDesc & desc) noexcept = 0;
			virtual oref<IBuffer> CreateBufferWithData(const BufferDesc & desc, const ResourceInitDesc & init) noexcept = 0;
			virtual oref<ITexture> CreateTexture(const TextureDesc & desc) noexcept = 0;
			virtual oref<ITexture> CreateTextureWithData(const TextureDesc & desc, const ResourceInitDesc * init) noexcept = 0;
			virtual oref<ITexture> CreateRenderTargetView(ITexture * texture, uint32 mip_level, uint32 array_offset_or_depth) noexcept = 0;
			virtual oref<IDeviceResource> OpenResource(IDeviceResourceHandle * handle) noexcept = 0;
			virtual oref<IPresentationLayer> CreatePresentationLayer(DynamicObject * presentor, const PresentationLayerDesc & desc) noexcept = 0;

			oref<IShaderLibrary> LoadShaderLibraryFromData(const void * data, uintptr length);
			oref<IShaderLibrary> LoadShaderLibrary(Stream * stream);
			oref<IShaderLibrary> CompileShaderLibraryFromData(const void * data, uintptr length);
			oref<IShaderLibrary> CompileShaderLibrary(Stream * stream);
		};
		class IDeviceChild : public Object
		{
		public:
			virtual IDevice * GetParentDevice(void) noexcept = 0;
		};
		class IShader : public IDeviceChild
		{
		public:
			virtual const ucs1_string & GetName(void) noexcept = 0;
			virtual ShaderType GetType(void) noexcept = 0;
		};
		class IShaderLibrary : public IDeviceChild
		{
		public:
			virtual oref<array<ucs1_string>> EnumerateShaderNames(void) noexcept = 0;
			virtual oref<IShader> CreateShader(const ucs1_string & name) noexcept = 0;
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
		class IPresentationLayer : public IDeviceChild
		{
		public:
			virtual bool Present(void) noexcept = 0;
			virtual oref<ITexture> QuerySurface(void) noexcept = 0;
			virtual bool ResizeSurface(uint32 width, uint32 height) noexcept = 0;
			virtual bool SwitchToFullscreen(void) noexcept = 0;
			virtual bool SwitchToWindow(void) noexcept = 0;
			virtual bool IsFullscreen(void) noexcept = 0;
			virtual uint GetLayerAttributes(void) noexcept = 0;
		};
		class IDeviceContext : public DynamicObject
		{
		public:
			virtual bool BeginRenderingPass(uint32 rtc, const RenderTargetViewDesc * rtv, const DepthStencilViewDesc * dsv) noexcept = 0;
			virtual bool BeginRenderingPass2D(const RenderTargetViewDesc & rtv) noexcept = 0;
			virtual bool BeginMemoryManagementPass(void) noexcept = 0;
			virtual bool EndCurrentPass(void) noexcept = 0;
			virtual bool SubmitDeferredContext(IDeviceContext * context) noexcept = 0;
			virtual void Flush(void) noexcept = 0;
			virtual bool IsDeferred(void) noexcept = 0;

			virtual void SetRenderingPipelineState(IPipelineState * state) noexcept = 0;
			virtual void SetViewport(float top_left_x, float top_left_y, float width, float height, float min_depth, float max_depth) noexcept = 0;
			virtual void SetVertexShaderResource(uint32 at, IDeviceResource * resource) noexcept = 0;
			virtual void SetVertexShaderConstant(uint32 at, IBuffer * buffer) noexcept = 0;
			virtual void SetVertexShaderConstantImmediate(uint32 at, const void * data, uint length) noexcept = 0;
			virtual void SetVertexShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept = 0;
			virtual void SetPixelShaderResource(uint32 at, IDeviceResource * resource) noexcept = 0;
			virtual void SetPixelShaderConstant(uint32 at, IBuffer * buffer) noexcept = 0;
			virtual void SetPixelShaderConstantImmediate(uint32 at, const void * data, uint length) noexcept = 0;
			virtual void SetPixelShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept = 0;
			virtual void SetIndexBuffer(IBuffer * index, IndexBufferFormat format) noexcept = 0;
			virtual void SetStencilReferenceValue(uint8 ref) noexcept = 0;
			virtual void DrawPrimitives(uint32 vertex_count, uint32 first_vertex) noexcept = 0;
			virtual void DrawInstancedPrimitives(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance) noexcept = 0;
			virtual void DrawIndexedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex) noexcept = 0;
			virtual void DrawIndexedInstancedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex, uint32 instance_count, uint32 first_instance) noexcept = 0;

			virtual void GenerateMipmaps(ITexture * texture) noexcept = 0;
			virtual void CopyResourceData(IDeviceResource * dest, IDeviceResource * src) noexcept = 0;
			virtual void CopySubresourceData(IDeviceResource * dest, const Index2 & dest_subres, const Index3 & dest_origin, IDeviceResource * src, const Index2 & src_subres, const Index3 & src_origin, const Index3 & size) noexcept = 0;
			virtual void UpdateResourceData(IDeviceResource * dest, const Index2 & subres, const Index3 & origin, const Index3 & size, const ResourceInitDesc & src) noexcept = 0;
			virtual void QueryResourceData(const ResourceDataDesc & dest, IDeviceResource * src, const Index2 & subres, const Index3 & origin, const Index3 & size) noexcept = 0;

			virtual bool AcquireSharedResource(IDeviceResource * rsrc) noexcept = 0;
			virtual bool TryAcquireSharedResource(IDeviceResource * rsrc, uint32 timeout) noexcept = 0;
			virtual bool ReleaseSharedResource(IDeviceResource * rsrc) noexcept = 0;
		};

		class IBitmap : public Object
		{
		public:
			virtual uint GetWidth(void) noexcept = 0;
			virtual uint GetHeight(void) noexcept = 0;
			virtual bool Update(Picturae::Picture * source) noexcept = 0;
			virtual oref<Picturae::Picture> QueryContents(void) noexcept = 0;
		};
		class IFont : public Object
		{
		public:
			virtual string GetFontFace(void) noexcept = 0;
			virtual uint GetFontStyle(void) noexcept = 0;
			virtual uint GetHeight(void) noexcept = 0;
			virtual void GetFontMetrics(FontMetrics & metrics) noexcept = 0;
			virtual void GetGlyphMetrics(const uint * glyph, FontGlyphMetrics * metrics, uintptr length) noexcept = 0;
			virtual void GetGlyphsForCharacters(const unichar32 * chr, uint * glyph, uintptr length) noexcept = 0;
		};

		class ILayerBacking : public IDeviceChild
		{
		public:
			virtual IDeviceContext2D * GetParentContext(void) noexcept = 0;
		};
		class IGlyphRun : public IDeviceChild
		{
		public:
			virtual IDeviceContext2D * GetParentContext(void) noexcept = 0;
		};
		class IBrush : public IDeviceChild
		{
		public:
			virtual IDeviceContext2D * GetParentContext(void) noexcept = 0;
			virtual BrushType GetBrushType(void) noexcept = 0;
		};
		class IColorBrush : public IBrush
		{
		public:
			virtual void OverrideGradientPoints(const Index2 & from, const Index2 & to) noexcept = 0;
		};
		class IBitmapBrush : public IBrush
		{
		public:
			virtual void OverrideTileReferenceRectangle(const Rectangle & rect) noexcept = 0;
		};
		class IBlurEffectBrush : public IBrush {};
		class IInversionEffectBrush : public IBrush {};

		class IDeviceContext2D : public DynamicObject
		{
		public:
			virtual void GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) noexcept = 0;
			virtual uint32 GetImplementationFeatures(void) noexcept = 0;

			virtual oref<IColorBrush> CreateSolidColorBrush(const Color & color) noexcept = 0;
			virtual oref<IColorBrush> CreateGradientBrush(const Index2 & from, const Index2 & to, const Color * colors, const double * positions, uint count) noexcept = 0;
			virtual oref<IBitmapBrush> CreateBitmapBrush(IBitmap * bitmap, const Rectangle & area) noexcept = 0;
			virtual oref<IBitmapBrush> CreateBitmapBrushCopy(IBitmapBrush * bitmap, const Rectangle & area) noexcept = 0;
			virtual oref<IBitmapBrush> CreateTileBrush(IBitmap * bitmap, const Rectangle & area) noexcept = 0;
			virtual oref<IBitmapBrush> CreateTileBrushCopy(IBitmapBrush * bitmap, const Rectangle & area) noexcept = 0;
			virtual oref<IBitmapBrush> CreateTextureBrush(ITexture * texture, TextureAlphaMode mode) noexcept = 0;
			virtual oref<IBlurEffectBrush> CreateBlurEffectBrush(double sigma) noexcept = 0;
			virtual oref<IInversionEffectBrush> CreateInversionEffectBrush(void) noexcept = 0;
			virtual oref<ILayerBacking> CreateLayerBackingStorage(void) noexcept = 0;
			virtual oref<IGlyphRun> CreateGlyphRun(IFont ** fonts, const uint * glyphs, const double * px, const double * py, const Color * colors, uint count, const double * transform) noexcept = 0;

			virtual void PushClip(const Rectangle & rect) noexcept = 0;
			virtual void PopClip(void) noexcept = 0;
			virtual bool BeginLayerAlpha(ILayerBacking * layer, const Rectangle & rect) noexcept = 0;
			virtual bool BeginLayer(ILayerBacking * layer, const Rectangle & rect, double opacity) noexcept = 0;
			virtual void EndLayer(ILayerBacking * layer) noexcept = 0;
			virtual void Render(IBrush * brush, const Rectangle & at) noexcept = 0;
			virtual void RenderPolyline(const double * px, const double * py, uint count, bool closed, IBrush * brush, double width) noexcept = 0;
			virtual void RenderPolygon(const double * px, const double * py, uint count, IBrush * brush) noexcept = 0;
			virtual void RenderGlyphRun(IGlyphRun * run, const Index2 & at) noexcept = 0;

			virtual bool BeginRendering(TextureLoadAction load, const Color & clear_color) noexcept = 0;
			virtual bool EndRendering(void) noexcept = 0;
		};

		class IDeviceResourceHandle : public Object
		{
		public:
			virtual uint64 GetDeviceIdentifier(void) noexcept = 0;
			virtual void Send(IPC::IConnection * con, ErrorContext & ectx) noexcept = 0;

			void Send(IPC::IConnection * con);
		};
		class IDeviceFactory : public Object
		{
		public:
			virtual oref<Dictionary<uint64, string>> EnumerateDevices(void) noexcept = 0;
			virtual oref<IDevice> CreateDevice(uint64 identifier) noexcept = 0;
			virtual oref<IDevice> CreateDefaultDevice(void) noexcept = 0;
			virtual oref<IDeviceResourceHandle> QueryResourceHandle(IDeviceResource * resource) noexcept = 0;
			virtual oref<IDeviceResourceHandle> ReceiveResourceHandle(IPC::IConnection * con, ErrorContext & ectx) noexcept = 0;

			oref<IDeviceResourceHandle> ReceiveResourceHandle(IPC::IConnection * con);
		};
		class IDeviceContextFactory2D : public Object
		{
		public:
			virtual oref<IBitmap> CreateBitmap(uint width, uint height, const Color & clear_color) noexcept = 0;
			virtual oref<IBitmap> LoadBitmap(Picturae::Picture * source) noexcept = 0;
			virtual oref<IDeviceContext2D> CreateBitmapContext(IBitmap * bitmap) noexcept = 0;
			virtual oref<IDeviceContext2D> CreatePresentationContext(DynamicObject * presentor, IDevice * device) noexcept = 0;
			virtual oref<IFont> CreateFont(const string & font_face, uint style, uint height, ErrorContext & ectx) noexcept = 0;
			virtual oref<IFont> LoadFont(Stream * stream, uint height, ErrorContext & ectx) noexcept = 0;
			virtual oref<IFont> SearchFont(IFont * base_font, const unichar32 * chars, uint count, ErrorContext & ectx) noexcept = 0;
			virtual oref<array<string>> EnumerateFontFamilies(void) noexcept = 0;

			oref<IFont> CreateFont(const string & font_face, uint style, uint height);
			oref<IFont> LoadFont(Stream * stream, uint height);
			oref<IFont> SearchFont(IFont * base_font, const unichar32 * chars, uint count);
		};

		constexpr uint InvalidGlyph = uint(int(-1));
		bool IsColorFormat(PixelFormat format) noexcept;
		bool IsDepthStencilFormat(PixelFormat format) noexcept;
		uint GetFormatChannelCount(PixelFormat format) noexcept;
		uint GetFormatBitsPerPixel(PixelFormat format) noexcept;
	}
}