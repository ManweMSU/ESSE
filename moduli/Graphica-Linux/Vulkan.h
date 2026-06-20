#pragma once

#include <Cor/Images/CorGraphics.h>

namespace ESSE
{
	namespace Vulkan
	{
		enum VulkanDeviceClass : uint {
			VulkanDeviceDiscrete	= 0x01,
			VulkanDeviceIntegrated	= 0x02,
			VulkanDeviceVirtual		= 0x04,
			VulkanDeviceSoftware	= 0x08,
			VulkanDeviceOther		= 0x10,
			VulkanDeviceAny			= 0x1F,
		};
		enum VulkanInputClass : uint {
			VulkanInputUniversalShaderBundle	= 0,
			VulkanInputGLSLVertexFunction		= 1,
			VulkanInputGLSLPixelFunction		= 2,
		};
		struct VulkanInputDesc {
			VulkanInputClass InputClass;
			uint VulkanVersionMajor;
			uint VulkanVersionMinor;
			uint SPIRVVersionMajor;
			uint SPIRVVersionMinor;
			uint GLSLVersionMajor;
			uint GLSLVersionMinor;
		};

		constexpr uint MaxRuntimePerShaderConstants	= 16;
		constexpr uint MaxRuntimePerShaderSamplers	= 16;
		constexpr uint MaxRuntimePerShaderBuffers	= 128;
		constexpr uint MaxRuntimePerShaderTextures	= 128;

		void GetDefaultDevicePriority(uint & desired, uint & allowed) noexcept;
		void SetDefaultDevicePriority(uint desired, uint allowed) noexcept;
		void GetDeviceEnumerationMask(uint & mask) noexcept;
		void SetDeviceEnumerationMask(uint mask) noexcept;
		void GetDeviceValidationLayer(bool & set) noexcept;
		void SetDeviceValidationLayer(bool set) noexcept;

		oref<DataBlock> PrecompileShaders(const void * data, uintptr length, const VulkanInputDesc & desc, oref<DataBlock> & log, ErrorContext & ectx) noexcept;
		VulkanDeviceClass GetVulkanDeviceClass(Graphica::IDevice * device) noexcept;

		oref<Graphica::IDeviceFactory> CreateDeviceFactory(ErrorContext & ectx) noexcept;
	}
}