#include "Vulkan.h"
#include "Vulkan2D.h"
#include "DeviceCairo.h"
#include <Cor/CorVirtualMemory.h>
#include <Cor-Linux/CorLinuxClasses.h>
#include <Graphica/Graphica.h>
#include <Formationes/Archive.h>

#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 1
#define VULKAN_HPP_NO_DEFAULT_DISPATCHER
#define VULKAN_HPP_NO_EXCEPTIONS
#include <glslang/glslang_c_interface.h>
#include <vulkan/vulkan.h>
#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_WAYLAND
	// TODO: IMPLEMENT WAYLAND
#endif
#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
	#define VK_USE_PLATFORM_XLIB_KHR
	#include <Fenestrae/Fenestrae.h>
	#include <Fenestrae-Linux-X11/X11WindowSystem.h>
	typedef ESSE::X11::Display Display;
	typedef ESSE::X11::Window Window;
	typedef ESSE::X11::VisualID VisualID;
	#include <vulkan/vulkan_xlib.h>
#endif
#include <vulkan/vulkan.hpp>
#include <unistd.h>
#include <semaphore.h>
#include <atomic>
#include <math.h>

using namespace ESSE::Graphica;

namespace ESSE
{
	namespace Vulkan
	{
		constexpr uintptr _vk_extensions_slots = 0x10;
		constexpr uintptr _vk_submission_slots = 0x10;
		constexpr uintptr _vk_descriptor_pool_size = 0x200;
		constexpr uintptr _vk_constant_buffer_size = 0x10000;

		const char * _vk_desired_interface_extensions[] {
			#if defined(ESSE_MODULUS_FENESTRARUM_LINUX_WAYLAND) || defined(ESSE_MODULUS_FENESTRARUM_LINUX_X11)
				#define ESSE_VULKAN_PRESENTATION
				VK_KHR_SURFACE_EXTENSION_NAME,
			#endif
			#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_WAYLAND
				// TODO: IMPLEMENT WAYLAND
				// VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
			#endif
			#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
				VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
			#endif
			VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		};
		const char * _vk_debug_interface_extensions[] {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		};
		const char * _vk_desired_device_extensions[] {
			#ifdef ESSE_VULKAN_PRESENTATION
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			#endif
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_1_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_2_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_3_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
			VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
			VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
			VK_KHR_MULTIVIEW_EXTENSION_NAME,
			VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
			VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME,
			VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
			VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
			VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
			VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
			VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,
		};

		typedef ESSE::oref<ESSE::Object> ResourceHandle;
		void VKValidationOutput(const char * format, const char * arg = 0) noexcept
		{
			int i = 0, s = 0;
			while (format[i]) {
				if (format[i] == '%' && format[i + 1] == 's') {
					if (i > s) write(1, format + s, i - s);
					if (arg) write(1, arg, strlen(arg));
					i += 2; s = i;
				} else if (format[i] == '%' && format[i + 1] == 'i') {
					char num[48];
					int value = sintptr(arg);
					sprintf(num, "%i", value);
					if (i > s) write(1, format + s, i - s);
					write(1, num, strlen(num));
					i += 2; s = i;
				} else i++;
			}
			if (i > s) write(1, format + s, i - s);
		}
		class VKAPI : public Object
		{
			static void * _vk_mem_alloc(void * user, size_t size, size_t align, VkSystemAllocationScope scope) noexcept { return aligned_alloc(align, size); }
			static void _vk_mem_release(void * user, void * memory) noexcept { return free(memory); }
			static void * _vk_mem_realloc(void * user, void * memory, size_t size, size_t align, VkSystemAllocationScope scope) noexcept { return realloc(memory, size); }
		public:
			VkAllocationCallbacks Allocator;
			VkInstance Instance;
			vk::detail::DynamicLoader Loader;
			vk::detail::DispatchLoaderDynamic Dispatch;
			uint32 Version;
		public:
			VKAPI(void)
			{
				bool validation_layer;
				auto validation_layer_name = "VK_LAYER_KHRONOS_validation";
				GetDeviceValidationLayer(validation_layer);
				Allocator.pUserData = 0;
				Allocator.pfnAllocation = _vk_mem_alloc;
				Allocator.pfnReallocation = _vk_mem_realloc;
				Allocator.pfnFree = _vk_mem_release;
				Allocator.pfnInternalAllocation = 0;
				Allocator.pfnInternalFree = 0;
				if (!Loader.success()) {
					if (validation_layer) VKValidationOutput("Vulkan API: Protocollum nullum.\n");
					throw NotImplementedException();
				}
				Dispatch.init(Loader);
				Dispatch.vkEnumerateInstanceVersion(&Version);
				array<const char *> desired_extensions(_vk_extensions_slots);
				array<VkExtensionProperties> extensions(1);
				uint32 extensions_count;
				uint32 desired_extensions_count = sizeof(_vk_desired_interface_extensions) / sizeof(_vk_desired_interface_extensions[0]);
				if (Dispatch.vkEnumerateInstanceExtensionProperties(0, &extensions_count, 0) < 0) throw NotImplementedException();
				extensions.SetLength(extensions_count);
				if (Dispatch.vkEnumerateInstanceExtensionProperties(0, &extensions_count, extensions) < 0) throw NotImplementedException();
				if (extensions_count < extensions.GetLength()) extensions.SetLength(extensions_count);
				for (uint i = 0; i < desired_extensions_count; i++) {
					bool supported = false;
					for (uint j = 0; j < extensions.GetLength(); j++) if (strcmp(_vk_desired_interface_extensions[i], extensions[j].extensionName) == 0) {
						supported = true;
						break;
					}
					if (supported) desired_extensions.Append(_vk_desired_interface_extensions[i]);
					else if (validation_layer) VKValidationOutput("Vulkan API: Extensio protocolli nulla: %s\n", _vk_desired_interface_extensions[i]);
				}
				if (validation_layer) {
					desired_extensions_count = sizeof(_vk_debug_interface_extensions) / sizeof(_vk_debug_interface_extensions[0]);
					for (uint i = 0; i < desired_extensions_count; i++) {
						bool supported = false;
						for (uint j = 0; j < extensions.GetLength(); j++) if (strcmp(_vk_debug_interface_extensions[i], extensions[j].extensionName) == 0) {
							supported = true;
							break;
						}
						if (supported) desired_extensions.Append(_vk_debug_interface_extensions[i]);
						else VKValidationOutput("Vulkan API: Extensio protocolli nulla: %s\n", _vk_debug_interface_extensions[i]);
					}
				}
				VkApplicationInfo app;
				app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				app.pNext = 0;
				app.apiVersion = VK_API_VERSION_1_3;
				app.pApplicationName = app.pEngineName = "";
				app.applicationVersion = app.engineVersion = 0;
				VkInstanceCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				info.pNext = 0;
				info.flags = 0;
				info.pApplicationInfo = &app;
				if (validation_layer) {
					info.enabledLayerCount = 1;
					info.ppEnabledLayerNames = &validation_layer_name;
				} else {
					info.enabledLayerCount = 0;
					info.ppEnabledLayerNames = 0;
				}
				info.enabledExtensionCount = desired_extensions.GetLength();
				info.ppEnabledExtensionNames = desired_extensions;
				if (Dispatch.vkCreateInstance(&info, &Allocator, &Instance) != VK_SUCCESS) throw NotImplementedException();
				Dispatch.init(Instance, Dispatch.vkGetInstanceProcAddr);
				if (validation_layer) VKValidationOutput("Vulkan API: Creatum est.\n");
			}
			virtual ~VKAPI(void) override
			{
				Dispatch.vkDestroyInstance(Instance, &Allocator);
				bool validation_layer;
				GetDeviceValidationLayer(validation_layer);
				if (validation_layer) VKValidationOutput("Vulkan API: Demissum est.\n");
			}
		};
		class VKDeviceAPI : public Object
		{
		public:
			VkDevice Device;
			vk::detail::DispatchLoaderDynamic Dispatch;
			oref<VKAPI> Base;
		public:
			VKDeviceAPI(void) : Device(0) {}
			virtual ~VKDeviceAPI(void) override { if (Device) Base->Dispatch.vkDestroyDevice(Device, &Base->Allocator); }
			void Initialize(VKAPI * api, VkDevice device) noexcept { Device = device; Base.SetRetain(api); Dispatch.init(Base->Instance, Base->Dispatch.vkGetInstanceProcAddr, Device, Base->Dispatch.vkGetDeviceProcAddr); }
		};
		class VKCompilerAPI : public Object
		{
		public:
			typedef int (* func_glslang_initialize_process) (void);
			typedef void (* func_glslang_finalize_process) (void);
			typedef glslang_shader_t* (* func_glslang_shader_create) (const glslang_input_t* input);
			typedef void (* func_glslang_shader_delete) (glslang_shader_t* shader);
			typedef void (* func_glslang_shader_set_preamble) (glslang_shader_t* shader, const char* s);
			typedef void (* func_glslang_shader_shift_binding) (glslang_shader_t* shader, glslang_resource_type_t res, unsigned int base);
			typedef void (* func_glslang_shader_shift_binding_for_set) (glslang_shader_t* shader, glslang_resource_type_t res, unsigned int base, unsigned int set);
			typedef void (* func_glslang_shader_set_options) (glslang_shader_t* shader, int options);
			typedef void (* func_glslang_shader_set_glsl_version) (glslang_shader_t* shader, int version);
			typedef int (* func_glslang_shader_preprocess) (glslang_shader_t* shader, const glslang_input_t* input);
			typedef int (* func_glslang_shader_parse) (glslang_shader_t* shader, const glslang_input_t* input);
			typedef const char* (* func_glslang_shader_get_preprocessed_code) (glslang_shader_t* shader);
			typedef const char* (* func_glslang_shader_get_info_log) (glslang_shader_t* shader);
			typedef const char* (* func_glslang_shader_get_info_debug_log) (glslang_shader_t* shader);
			typedef glslang_program_t* (* func_glslang_program_create) (void);
			typedef void (* func_glslang_program_delete) (glslang_program_t* program);
			typedef void (* func_glslang_program_add_shader) (glslang_program_t* program, glslang_shader_t* shader);
			typedef int (* func_glslang_program_link) (glslang_program_t* program, int messages);
			typedef void (* func_glslang_program_add_source_text) (glslang_program_t* program, glslang_stage_t stage, const char* text, size_t len);
			typedef void (* func_glslang_program_set_source_file) (glslang_program_t* program, glslang_stage_t stage, const char* file);
			typedef int (* func_glslang_program_map_io) (glslang_program_t* program);
			typedef void (* func_glslang_program_SPIRV_generate) (glslang_program_t* program, glslang_stage_t stage);
			typedef void (* func_glslang_program_SPIRV_generate_with_options) (glslang_program_t* program, glslang_stage_t stage, glslang_spv_options_t* spv_options);
			typedef size_t (* func_glslang_program_SPIRV_get_size) (glslang_program_t* program);
			typedef void (* func_glslang_program_SPIRV_get) (glslang_program_t* program, unsigned int*);
			typedef unsigned int* (* func_glslang_program_SPIRV_get_ptr) (glslang_program_t* program);
			typedef const char* (* func_glslang_program_SPIRV_get_messages) (glslang_program_t* program);
			typedef const char* (* func_glslang_program_get_info_log) (glslang_program_t* program);
			typedef const char* (* func_glslang_program_get_info_debug_log) (glslang_program_t* program);
			typedef const glslang_resource_t* (* func_glslang_default_resource) (void);
		private:
			handle _dl, _res_dl, _spirv_dl;
		public:
			func_glslang_initialize_process glslang_initialize_process;
			func_glslang_finalize_process glslang_finalize_process;
			func_glslang_shader_create glslang_shader_create;
			func_glslang_shader_delete glslang_shader_delete;
			func_glslang_shader_set_preamble glslang_shader_set_preamble;
			func_glslang_shader_shift_binding glslang_shader_shift_binding;
			func_glslang_shader_shift_binding_for_set glslang_shader_shift_binding_for_set;
			func_glslang_shader_set_options glslang_shader_set_options;
			func_glslang_shader_set_glsl_version glslang_shader_set_glsl_version;
			func_glslang_shader_preprocess glslang_shader_preprocess;
			func_glslang_shader_parse glslang_shader_parse;
			func_glslang_shader_get_preprocessed_code glslang_shader_get_preprocessed_code;
			func_glslang_shader_get_info_log glslang_shader_get_info_log;
			func_glslang_shader_get_info_debug_log glslang_shader_get_info_debug_log;
			func_glslang_program_create glslang_program_create;
			func_glslang_program_delete glslang_program_delete;
			func_glslang_program_add_shader glslang_program_add_shader;
			func_glslang_program_link glslang_program_link;
			func_glslang_program_add_source_text glslang_program_add_source_text;
			func_glslang_program_set_source_file glslang_program_set_source_file;
			func_glslang_program_map_io glslang_program_map_io;
			func_glslang_program_SPIRV_generate glslang_program_SPIRV_generate;
			func_glslang_program_SPIRV_generate_with_options glslang_program_SPIRV_generate_with_options;
			func_glslang_program_SPIRV_get_size glslang_program_SPIRV_get_size;
			func_glslang_program_SPIRV_get glslang_program_SPIRV_get;
			func_glslang_program_SPIRV_get_ptr glslang_program_SPIRV_get_ptr;
			func_glslang_program_SPIRV_get_messages glslang_program_SPIRV_get_messages;
			func_glslang_program_get_info_log glslang_program_get_info_log;
			func_glslang_program_get_info_debug_log glslang_program_get_info_debug_log;
			func_glslang_default_resource glslang_default_resource;
		public:
			VKCompilerAPI(void)
			{
				_dl = dlopen("libglslang.so", RTLD_NOW | RTLD_LOCAL);
				if (!_dl) throw NotImplementedException();
				_res_dl = dlopen("libglslang-default-resource-limits.so", RTLD_NOW | RTLD_LOCAL);
				if (!_res_dl) { dlclose(_dl); throw NotImplementedException(); }
				_spirv_dl = dlopen("libSPIRV.so", RTLD_NOW | RTLD_LOCAL);
				glslang_initialize_process = reinterpret_cast<func_glslang_initialize_process>(dlsym(_dl, "glslang_initialize_process"));
				glslang_finalize_process = reinterpret_cast<func_glslang_finalize_process>(dlsym(_dl, "glslang_finalize_process"));
				glslang_shader_create = reinterpret_cast<func_glslang_shader_create>(dlsym(_dl, "glslang_shader_create"));
				glslang_shader_delete = reinterpret_cast<func_glslang_shader_delete>(dlsym(_dl, "glslang_shader_delete"));
				glslang_shader_set_preamble = reinterpret_cast<func_glslang_shader_set_preamble>(dlsym(_dl, "glslang_shader_set_preamble"));
				glslang_shader_shift_binding = reinterpret_cast<func_glslang_shader_shift_binding>(dlsym(_dl, "glslang_shader_shift_binding"));
				glslang_shader_shift_binding_for_set = reinterpret_cast<func_glslang_shader_shift_binding_for_set>(dlsym(_dl, "glslang_shader_shift_binding_for_set"));
				glslang_shader_set_options = reinterpret_cast<func_glslang_shader_set_options>(dlsym(_dl, "glslang_shader_set_options"));
				glslang_shader_set_glsl_version = reinterpret_cast<func_glslang_shader_set_glsl_version>(dlsym(_dl, "glslang_shader_set_glsl_version"));
				glslang_shader_preprocess = reinterpret_cast<func_glslang_shader_preprocess>(dlsym(_dl, "glslang_shader_preprocess"));
				glslang_shader_parse = reinterpret_cast<func_glslang_shader_parse>(dlsym(_dl, "glslang_shader_parse"));
				glslang_shader_get_preprocessed_code = reinterpret_cast<func_glslang_shader_get_preprocessed_code>(dlsym(_dl, "glslang_shader_get_preprocessed_code"));
				glslang_shader_get_info_log = reinterpret_cast<func_glslang_shader_get_info_log>(dlsym(_dl, "glslang_shader_get_info_log"));
				glslang_shader_get_info_debug_log = reinterpret_cast<func_glslang_shader_get_info_debug_log>(dlsym(_dl, "glslang_shader_get_info_debug_log"));
				glslang_program_create = reinterpret_cast<func_glslang_program_create>(dlsym(_dl, "glslang_program_create"));
				glslang_program_delete = reinterpret_cast<func_glslang_program_delete>(dlsym(_dl, "glslang_program_delete"));
				glslang_program_add_shader = reinterpret_cast<func_glslang_program_add_shader>(dlsym(_dl, "glslang_program_add_shader"));
				glslang_program_link = reinterpret_cast<func_glslang_program_link>(dlsym(_dl, "glslang_program_link"));
				glslang_program_add_source_text = reinterpret_cast<func_glslang_program_add_source_text>(dlsym(_dl, "glslang_program_add_source_text"));
				glslang_program_set_source_file = reinterpret_cast<func_glslang_program_set_source_file>(dlsym(_dl, "glslang_program_set_source_file"));
				glslang_program_map_io = reinterpret_cast<func_glslang_program_map_io>(dlsym(_dl, "glslang_program_map_io"));
				glslang_program_get_info_log = reinterpret_cast<func_glslang_program_get_info_log>(dlsym(_dl, "glslang_program_get_info_log"));
				glslang_program_get_info_debug_log = reinterpret_cast<func_glslang_program_get_info_debug_log>(dlsym(_dl, "glslang_program_get_info_debug_log"));
				glslang_default_resource = reinterpret_cast<func_glslang_default_resource>(dlsym(_res_dl, "glslang_default_resource"));
				glslang_program_SPIRV_generate = reinterpret_cast<func_glslang_program_SPIRV_generate>(dlsym(_dl, "glslang_program_SPIRV_generate"));
				if (!glslang_program_SPIRV_generate && _spirv_dl) glslang_program_SPIRV_generate = reinterpret_cast<func_glslang_program_SPIRV_generate>(dlsym(_spirv_dl, "glslang_program_SPIRV_generate"));
				glslang_program_SPIRV_generate_with_options = reinterpret_cast<func_glslang_program_SPIRV_generate_with_options>(dlsym(_dl, "glslang_program_SPIRV_generate_with_options"));
				if (!glslang_program_SPIRV_generate_with_options && _spirv_dl) glslang_program_SPIRV_generate_with_options = reinterpret_cast<func_glslang_program_SPIRV_generate_with_options>(dlsym(_spirv_dl, "glslang_program_SPIRV_generate_with_options"));
				glslang_program_SPIRV_get_size = reinterpret_cast<func_glslang_program_SPIRV_get_size>(dlsym(_dl, "glslang_program_SPIRV_get_size"));
				if (!glslang_program_SPIRV_get_size && _spirv_dl) glslang_program_SPIRV_get_size = reinterpret_cast<func_glslang_program_SPIRV_get_size>(dlsym(_spirv_dl, "glslang_program_SPIRV_get_size"));
				glslang_program_SPIRV_get = reinterpret_cast<func_glslang_program_SPIRV_get>(dlsym(_dl, "glslang_program_SPIRV_get"));
				if (!glslang_program_SPIRV_get && _spirv_dl) glslang_program_SPIRV_get = reinterpret_cast<func_glslang_program_SPIRV_get>(dlsym(_spirv_dl, "glslang_program_SPIRV_get"));
				glslang_program_SPIRV_get_ptr = reinterpret_cast<func_glslang_program_SPIRV_get_ptr>(dlsym(_dl, "glslang_program_SPIRV_get_ptr"));
				if (!glslang_program_SPIRV_get_ptr && _spirv_dl) glslang_program_SPIRV_get_ptr = reinterpret_cast<func_glslang_program_SPIRV_get_ptr>(dlsym(_spirv_dl, "glslang_program_SPIRV_get_ptr"));
				glslang_program_SPIRV_get_messages = reinterpret_cast<func_glslang_program_SPIRV_get_messages>(dlsym(_dl, "glslang_program_SPIRV_get_messages"));
				if (!glslang_program_SPIRV_get_messages && _spirv_dl) glslang_program_SPIRV_get_messages = reinterpret_cast<func_glslang_program_SPIRV_get_messages>(dlsym(_spirv_dl, "glslang_program_SPIRV_get_messages"));
				if (!glslang_initialize_process || !glslang_finalize_process || !glslang_shader_create || !glslang_shader_delete ||
					!glslang_shader_set_preamble || !glslang_shader_shift_binding || !glslang_shader_shift_binding_for_set || !glslang_shader_set_options ||
					!glslang_shader_set_glsl_version || !glslang_shader_preprocess || !glslang_shader_parse || !glslang_shader_get_preprocessed_code ||
					!glslang_shader_get_info_log || !glslang_shader_get_info_debug_log || !glslang_program_create || !glslang_program_delete ||
					!glslang_program_add_shader || !glslang_program_link || !glslang_program_add_source_text || !glslang_program_set_source_file ||
					!glslang_program_map_io || !glslang_program_SPIRV_generate || !glslang_program_SPIRV_generate_with_options || !glslang_program_SPIRV_get_size ||
					!glslang_program_SPIRV_get || !glslang_program_SPIRV_get_ptr || !glslang_program_SPIRV_get_messages || !glslang_program_get_info_log ||
					!glslang_program_get_info_debug_log || !glslang_default_resource) { dlclose(_dl); dlclose(_res_dl); if (_spirv_dl) dlclose(_spirv_dl); throw NotImplementedException(); }
				if (!glslang_initialize_process()) { dlclose(_dl); dlclose(_res_dl); if (_spirv_dl) dlclose(_spirv_dl); throw NotImplementedException(); }
			}
			virtual ~VKCompilerAPI(void) override { glslang_finalize_process(); dlclose(_dl); dlclose(_res_dl); if (_spirv_dl) dlclose(_spirv_dl); }
		};
		VkFormat CreateVkFormat(PixelFormat pxf) noexcept
		{
			if (IsColorFormat(pxf)) {
				auto bpp = GetFormatBitsPerPixel(pxf);
				if (bpp == 8) {
					if (pxf == PixelFormat::A8_unorm) return VK_FORMAT_A8_UNORM_KHR;
					else if (pxf == PixelFormat::R8_unorm) return VK_FORMAT_R8_UNORM;
					else if (pxf == PixelFormat::R8_snorm) return VK_FORMAT_R8_SNORM;
					else if (pxf == PixelFormat::R8_uint) return VK_FORMAT_R8_UINT;
					else if (pxf == PixelFormat::R8_sint) return VK_FORMAT_R8_SINT;
					else return VK_FORMAT_UNDEFINED;
				} else if (bpp == 16) {
					if (pxf == PixelFormat::R16_unorm) return VK_FORMAT_R16_UNORM;
					else if (pxf == PixelFormat::R16_snorm) return VK_FORMAT_R16_SNORM;
					else if (pxf == PixelFormat::R16_uint) return VK_FORMAT_R16_UINT;
					else if (pxf == PixelFormat::R16_sint) return VK_FORMAT_R16_SINT;
					else if (pxf == PixelFormat::R16_float) return VK_FORMAT_R16_SFLOAT;
					else if (pxf == PixelFormat::R8G8_unorm) return VK_FORMAT_R8G8_UNORM;
					else if (pxf == PixelFormat::R8G8_snorm) return VK_FORMAT_R8G8_SNORM;
					else if (pxf == PixelFormat::R8G8_uint) return VK_FORMAT_R8G8_UINT;
					else if (pxf == PixelFormat::R8G8_sint) return VK_FORMAT_R8G8_SINT;
					else if (pxf == PixelFormat::B5G6R5_unorm) return VK_FORMAT_B5G6R5_UNORM_PACK16;
					else if (pxf == PixelFormat::R5G6B5_unorm) return VK_FORMAT_R5G6B5_UNORM_PACK16;
					else if (pxf == PixelFormat::B5G5R5A1_unorm) return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
					else if (pxf == PixelFormat::R5G5B5A1_unorm) return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
					else if (pxf == PixelFormat::A1R5G5B5_unorm) return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
					else if (pxf == PixelFormat::B4G4R4A4_unorm) return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
					else if (pxf == PixelFormat::R4G4B4A4_unorm) return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
					else if (pxf == PixelFormat::A4B4G4R4_unorm) return VK_FORMAT_A4B4G4R4_UNORM_PACK16;
					else if (pxf == PixelFormat::A4R4G4B4_unorm) return VK_FORMAT_A4R4G4B4_UNORM_PACK16;
					else return VK_FORMAT_UNDEFINED;
				} else if (bpp == 32) {
					if (pxf == PixelFormat::R32_uint) return VK_FORMAT_R32_UINT;
					else if (pxf == PixelFormat::R32_sint) return VK_FORMAT_R32_SINT;
					else if (pxf == PixelFormat::R32_float) return VK_FORMAT_R32_SFLOAT;
					else if (pxf == PixelFormat::R16G16_unorm) return VK_FORMAT_R16G16_UNORM;
					else if (pxf == PixelFormat::R16G16_snorm) return VK_FORMAT_R16G16_SNORM;
					else if (pxf == PixelFormat::R16G16_uint) return VK_FORMAT_R16G16_UINT;
					else if (pxf == PixelFormat::R16G16_sint) return VK_FORMAT_R16G16_SINT;
					else if (pxf == PixelFormat::R16G16_float) return VK_FORMAT_R16G16_SFLOAT;
					else if (pxf == PixelFormat::B8G8R8A8_unorm) return VK_FORMAT_B8G8R8A8_UNORM;
					else if (pxf == PixelFormat::R8G8B8A8_unorm) return VK_FORMAT_R8G8B8A8_UNORM;
					else if (pxf == PixelFormat::R8G8B8A8_snorm) return VK_FORMAT_R8G8B8A8_SNORM;
					else if (pxf == PixelFormat::R8G8B8A8_uint) return VK_FORMAT_R8G8B8A8_UINT;
					else if (pxf == PixelFormat::R8G8B8A8_sint) return VK_FORMAT_R8G8B8A8_SINT;
					else if (pxf == PixelFormat::A2R10G10B10_unorm) return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
					else if (pxf == PixelFormat::A2R10G10B10_uint) return VK_FORMAT_A2R10G10B10_UINT_PACK32;
					else if (pxf == PixelFormat::B10G11R11_float) return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
					else if (pxf == PixelFormat::E5B9G9R9_float) return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
					else return VK_FORMAT_UNDEFINED;
				} else if (bpp == 64) {
					if (pxf == PixelFormat::R32G32_uint) return VK_FORMAT_R32G32_UINT;
					else if (pxf == PixelFormat::R32G32_sint) return VK_FORMAT_R32G32_SINT;
					else if (pxf == PixelFormat::R32G32_float) return VK_FORMAT_R32G32_SFLOAT;
					else if (pxf == PixelFormat::R16G16B16A16_unorm) return VK_FORMAT_R16G16B16A16_UNORM;
					else if (pxf == PixelFormat::R16G16B16A16_snorm) return VK_FORMAT_R16G16B16A16_SNORM;
					else if (pxf == PixelFormat::R16G16B16A16_uint) return VK_FORMAT_R16G16B16A16_UINT;
					else if (pxf == PixelFormat::R16G16B16A16_sint) return VK_FORMAT_R16G16B16A16_SINT;
					else if (pxf == PixelFormat::R16G16B16A16_float) return VK_FORMAT_R16G16B16A16_SFLOAT;
					else return VK_FORMAT_UNDEFINED;
				} else if (bpp == 128) {
					if (pxf == PixelFormat::R32G32B32A32_uint) return VK_FORMAT_R32G32B32A32_UINT;
					else if (pxf == PixelFormat::R32G32B32A32_sint) return VK_FORMAT_R32G32B32A32_SINT;
					else if (pxf == PixelFormat::R32G32B32A32_float) return VK_FORMAT_R32G32B32A32_SFLOAT;
					else return VK_FORMAT_UNDEFINED;
				} else return VK_FORMAT_UNDEFINED;
			} else if (IsDepthStencilFormat(pxf)) {
				if (pxf == PixelFormat::D16_unorm) return VK_FORMAT_D16_UNORM;
				else if (pxf == PixelFormat::D24_unorm) return VK_FORMAT_X8_D24_UNORM_PACK32;
				else if (pxf == PixelFormat::D32_float) return VK_FORMAT_D32_SFLOAT;
				else if (pxf == PixelFormat::D16S8_unorm) return VK_FORMAT_D16_UNORM_S8_UINT;
				else if (pxf == PixelFormat::D24S8_unorm) return VK_FORMAT_D24_UNORM_S8_UINT;
				else if (pxf == PixelFormat::D32S8_float) return VK_FORMAT_D32_SFLOAT_S8_UINT;
				else return VK_FORMAT_UNDEFINED;
			} else return VK_FORMAT_UNDEFINED;
		}
		VkCompareOp CreateVkCompare(CompareFunction func) noexcept
		{
			if (func == CompareFunction::Equal) return VK_COMPARE_OP_EQUAL;
			else if (func == CompareFunction::Lesser) return VK_COMPARE_OP_LESS;
			else if (func == CompareFunction::LesserEqual) return VK_COMPARE_OP_LESS_OR_EQUAL;
			else if (func == CompareFunction::Greater) return VK_COMPARE_OP_GREATER;
			else if (func == CompareFunction::GreaterEqual) return VK_COMPARE_OP_GREATER_OR_EQUAL;
			else if (func == CompareFunction::NotEqual) return VK_COMPARE_OP_NOT_EQUAL;
			else if (func == CompareFunction::Always) return VK_COMPARE_OP_ALWAYS;
			else if (func == CompareFunction::Never) return VK_COMPARE_OP_NEVER;
			else return VK_COMPARE_OP_NEVER;
		}
		VkStencilOp CreateVkStencil(StencilFunction func) noexcept
		{
			if (func == StencilFunction::Keep) return VK_STENCIL_OP_KEEP;
			else if (func == StencilFunction::IncrementWrap) return VK_STENCIL_OP_INCREMENT_AND_WRAP;
			else if (func == StencilFunction::IncrementClamp) return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			else if (func == StencilFunction::DecrementWrap) return VK_STENCIL_OP_DECREMENT_AND_WRAP;
			else if (func == StencilFunction::DecrementClamp) return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			else if (func == StencilFunction::Invert) return VK_STENCIL_OP_INVERT;
			else if (func == StencilFunction::Replace) return VK_STENCIL_OP_REPLACE;
			else if (func == StencilFunction::SetZero) return VK_STENCIL_OP_ZERO;
			else return VK_STENCIL_OP_ZERO;
		}
		VkBlendOp CreateVkBlend(BlendingFunction func) noexcept
		{
			if (func == BlendingFunction::Add) return VK_BLEND_OP_ADD;
			else if (func == BlendingFunction::SubtractBaseFromOver) return VK_BLEND_OP_SUBTRACT;
			else if (func == BlendingFunction::SubtractOverFromBase) return VK_BLEND_OP_REVERSE_SUBTRACT;
			else if (func == BlendingFunction::Max) return VK_BLEND_OP_MAX;
			else if (func == BlendingFunction::Min) return VK_BLEND_OP_MIN;
			else return VK_BLEND_OP_ADD;
		}
		VkBlendFactor CreateVkFactor(BlendingFactor fact) noexcept
		{
			if (fact == BlendingFactor::Zero) return VK_BLEND_FACTOR_ZERO;
			else if (fact == BlendingFactor::One) return VK_BLEND_FACTOR_ONE;
			else if (fact == BlendingFactor::BaseAlpha) return VK_BLEND_FACTOR_DST_ALPHA;
			else if (fact == BlendingFactor::BaseColor) return VK_BLEND_FACTOR_DST_COLOR;
			else if (fact == BlendingFactor::OverAlpha) return VK_BLEND_FACTOR_SRC_ALPHA;
			else if (fact == BlendingFactor::OverAlphaSaturated) return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			else if (fact == BlendingFactor::OverColor) return VK_BLEND_FACTOR_SRC_COLOR;
			else if (fact == BlendingFactor::SecondaryAlpha) return VK_BLEND_FACTOR_SRC1_ALPHA;
			else if (fact == BlendingFactor::SecondaryColor) return VK_BLEND_FACTOR_SRC1_COLOR;
			else if (fact == BlendingFactor::InvertedBaseAlpha) return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			else if (fact == BlendingFactor::InvertedBaseColor) return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			else if (fact == BlendingFactor::InvertedOverAlpha) return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			else if (fact == BlendingFactor::InvertedOverColor) return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			else if (fact == BlendingFactor::InvertedSecondaryAlpha) return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
			else if (fact == BlendingFactor::InvertedSecondaryColor) return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
			else return VK_BLEND_FACTOR_ZERO;
		}
		VkFilter CreateVkFilter(SamplerFilter filter) noexcept
		{
			if (filter == SamplerFilter::Point) return VK_FILTER_NEAREST;
			else return VK_FILTER_LINEAR;
		}
		VkSamplerMipmapMode CreateVkMipmapMode(SamplerFilter filter) noexcept
		{
			if (filter == SamplerFilter::Point) return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			else return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}
		VkSamplerAddressMode CreateVkAddressMode(SamplerAddressMode mode) noexcept
		{
			if (mode == SamplerAddressMode::Wrap) return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			else if (mode == SamplerAddressMode::Clamp) return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			else if (mode == SamplerAddressMode::Mirror) return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			else if (mode == SamplerAddressMode::Border) return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			else return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}
		void EvaluateMipMapSize(uint level, Index3 & index) noexcept
		{
			index.x >>= level; index.y >>= level; index.z >>= level;
			if (!index.x) index.x = 1;
			if (!index.y) index.y = 1;
			if (!index.z) index.z = 1;
		}
		uint EvaluateMipMapLevels(uint x, uint y = 1, uint z = 1) noexcept
		{
			uint l = 1;
			uint s = max(max(x, y), z);
			while (s > 1) { s /= 2; l++; }
			return l;
		}
		uint EvaluateNumberOfDimensions(TextureType type) noexcept
		{
			if (type == TextureType::Type1D || type == TextureType::TypeArray1D) return 1;
			else if (type == TextureType::Type2D || type == TextureType::TypeArray2D) return 2;
			else if (type == TextureType::TypeCube || type == TextureType::TypeArrayCube) return 2;
			else if (type == TextureType::Type3D) return 3;
			else return 0;
		}
		void ReadShaderResourceMapping(const void * spirv, uintptr length, array<uint> & mapping, uintptr & offset, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				auto data = reinterpret_cast<const uint *>(spirv);
				uintptr length4 = length >> 2U, i = 0;
				while (i < length4 && data[i]) i++;
				mapping.SetLength(i);
				for (uintptr j = 0; j < i; j++) mapping[j] = data[j];
				offset = i + 1;
			ESSE_TRY_OUTRO()
		}
		void ReadShaderResourceMapping(const char * glsl, array<uint> & mapping, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				mapping.Clear();
				uintptr length = Memory::StringLength(glsl), i = 0, e;
				while (i + 8 <= length) { if (Memory::MemoryCompare(glsl + i, "// XWSM:", 8) == 0) break; i++; }
				i += 8;
				if (i >= length) return;
				e = i;
				while (e < length && glsl[e] != '\n') e++;
				auto tmap = SplitString(string(glsl + i, e - i), U':');
				mapping.SetLength(tmap.GetLength());
				for (uintptr j = 0; j < tmap.GetLength(); j++) mapping[j] = tmap[j].ToUInt32(HexadecimalBase);
			ESSE_TRY_OUTRO()
		}

		oref<IDeviceFactory> _common_device_factory;
		uint _default_device_desired	= 0;
		uint _default_device_allowed	= VulkanDeviceAny;
		uint _device_enumeration_mask	= VulkanDeviceAny;
		bool _device_validation_layer	= false;
		#ifdef ESSE_VULKAN_PRESENTATION
		oref<Windows::IWindowExtensionClass> _common_vk_surface_class;
		#endif

		class VKDeviceResourceHandle : public IDeviceResourceHandle
		{
			friend class VKSharedObject;
			friend class VKDevice;
		private:
			uint64 _device_id;
			handle _resource_fd;
			oref<IPC::ISharedMemory> _memory;
		public:
			VKDeviceResourceHandle(uint64 device_id, handle rsrc, IPC::ISharedMemory * mem) : _device_id(device_id), _resource_fd(rsrc), _memory(mem) {}
			virtual ~VKDeviceResourceHandle(void) override { IO::CloseHandle(_resource_fd); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKDeviceResourceHandle"; ESSE_TRY_OUTRO(string()) }
			virtual uint64 GetDeviceIdentifier(void) noexcept override { return _device_id; }
			virtual void Send(IPC::IConnection * con, ErrorContext & ectx) noexcept override
			{
				if (!con) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return; }
				con->SendData(&_device_id, sizeof(_device_id), ectx);
				if (ErrorTest(ectx)) return;
				con->SendHandle(_resource_fd, ectx);
				if (ErrorTest(ectx)) return;
				con->SendHandle(_memory->GetIOHandle(), ectx);
			}
		};
		class VKSharedObject : public Object
		{
			friend class VKTexture;
			friend class VKQueue;
			friend class VKPass;
			friend class VKDeviceDeferredContext;
			friend class VKDeviceImmediateContext;
			friend class VKDevice;
			friend class VKDeviceFactory;
		private:
			struct _shared_object_desc
			{
				sem_t lock;
				struct {
					uint8 device_uuid[16];
					uint8 driver_uuid[16];
					uint64 memory_size;
					uint32 current_layout;
					uint32 reference_count;
				} sharing;
				struct {
					uint32 type, format, usage, size;
					uint32 width, height, depth, mip_count;
				} meta;
				std::atomic_flag init_guard;
			};
		private:
			oref<VKDeviceResourceHandle> _handle;
			_shared_object_desc * _data;
		public:
			VKSharedObject(VKDeviceAPI * api, ITexture * texture, VkPhysicalDevice physical, VkDeviceMemory memory, uint64 memory_size)
			{
				if (!api->Dispatch.vkGetMemoryFdKHR) throw NotImplementedException();
				auto shmem = IPC::CreateSharedMemory(U"", sizeof(_shared_object_desc), FileCreationMode::CreateNew);
				_data = reinterpret_cast<_shared_object_desc *>(shmem->Map(Memory::VirtualMemoryMapRead | Memory::VirtualMemoryMapWrite));
				int devmem_fd;
				VkMemoryGetFdInfoKHR get_fd;
				get_fd.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
				get_fd.pNext = 0;
				get_fd.memory = memory;
				get_fd.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
				if (api->Dispatch.vkGetMemoryFdKHR(api->Device, &get_fd, &devmem_fd) != VK_SUCCESS) {
					shmem->Unmap();
					throw OutOfMemoryException();
				}
				VkPhysicalDeviceProperties2 prop;
				VkPhysicalDeviceIDProperties prop_id;
				prop.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
				prop.pNext = &prop_id;
				prop_id.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
				prop_id.pNext = 0;
				api->Dispatch.vkGetPhysicalDeviceProperties2(physical, &prop);
				uint64 devid = (uint64(prop.properties.vendorID) << 32) | uint64(prop.properties.deviceID);
				Memory::MemoryCopy(&_data->sharing.device_uuid, &prop_id.deviceUUID, 16);
				Memory::MemoryCopy(&_data->sharing.driver_uuid, &prop_id.driverUUID, 16);
				if (sem_init(&_data->lock, 1, 1) < 0) {
					shmem->Unmap();
					close(devmem_fd);
					throw NotImplementedException();
				}
				_data->sharing.memory_size = memory_size;
				_data->sharing.current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
				_data->sharing.reference_count = 1;
				_data->meta.type = uint32(texture->GetTextureType());
				_data->meta.format = uint32(texture->GetPixelFormat());
				_data->meta.usage = texture->GetResourceUsage();
				_data->meta.size = texture->GetArraySize();
				_data->meta.width = texture->GetWidth();
				_data->meta.height = texture->GetHeight();
				_data->meta.depth = texture->GetDepth();
				_data->meta.mip_count = texture->GetMipmapCount();
				_data->init_guard.clear();
				_handle = owrap(new (std::nothrow) VKDeviceResourceHandle(devid, reinterpret_cast<handle>(intptr(devmem_fd)), shmem));
				if (!_handle) {
					sem_destroy(&_data->lock);
					shmem->Unmap();
					close(devmem_fd);
					throw OutOfMemoryException();
				}
			}
			VKSharedObject(VKDeviceResourceHandle * rsrc)
			{
				ErrorContext ectx; ErrorClear(ectx);
				auto hres = IO::DuplicateHandle(rsrc->_resource_fd, ectx);
				ErrorThrow(ectx);
				auto hmem = IO::DuplicateHandle(rsrc->_memory->GetIOHandle(), ectx);
				if (ErrorTest(ectx)) { IO::CloseHandle(hres); ErrorThrow(ectx); }
				auto shmem = IPC::OpenSharedMemory(hmem, sizeof(_shared_object_desc), ectx);
				if (ErrorTest(ectx)) { IO::CloseHandle(hres); IO::CloseHandle(hmem); ErrorThrow(ectx); }
				_handle = owrap(new (std::nothrow) VKDeviceResourceHandle(rsrc->_device_id, hres, shmem));
				if (!_handle) { IO::CloseHandle(hres); throw OutOfMemoryException(); }
				_data = reinterpret_cast<_shared_object_desc *>(shmem->Map(Memory::VirtualMemoryMapRead | Memory::VirtualMemoryMapWrite));
				while (_data->init_guard.test_and_set(std::memory_order_acquire));
				_data->sharing.reference_count++;
				if (_data->sharing.reference_count == 1 && sem_init(&_data->lock, 1, 1) < 0) {
					_data->init_guard.clear(std::memory_order_release);
					shmem->Unmap();
					throw NotImplementedException();
				}
				_data->init_guard.clear(std::memory_order_release);
			}
			virtual ~VKSharedObject(void) override
			{
				while (_data->init_guard.test_and_set(std::memory_order_acquire));
				_data->sharing.reference_count--;
				if (!_data->sharing.reference_count) sem_destroy(&_data->lock);
				_data->init_guard.clear(std::memory_order_release);
				_handle->_memory->Unmap();
			}
		};
		class VKShader : public IShader
		{
			friend class VKDevice;
		private:
			oref<VKDeviceAPI> _api;
			IDevice * _parent;
			VkShaderModule _module;
			ShaderType _type;
			ucs1_string _name, _entry;
			array<uint> _rmapping;
		public:
			VKShader(VKDeviceAPI * api, IDevice * parent) : _api(api), _parent(parent), _module(0), _rmapping(1) {}
			virtual ~VKShader(void) override { if (_module) _api->Dispatch.vkDestroyShaderModule(_api->Device, _module, &_api->Base->Allocator); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKShader"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent; }
			virtual const ucs1_string & GetName(void) noexcept override { return _name; }
			virtual ShaderType GetType(void) noexcept override { return _type; }
			array<uint> & GetResourceMapping(void) noexcept { return _rmapping; }
		};
		class VKShaderLibrary : public IShaderLibrary
		{
			friend class VKDevice;
		private:
			oref<VKDeviceAPI> _api;
			IDevice * _parent;
			ObjectDictionary<ucs1_string, VKShader> _shaders;
		public:
			VKShaderLibrary(VKDeviceAPI * api, IDevice * parent) : _api(api), _parent(parent) {}
			virtual ~VKShaderLibrary(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKShaderLibrary"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent; }
			virtual oref<array<ucs1_string>> EnumerateShaderNames(void) noexcept override
			{
				try {
					auto result = owrap(new array<ucs1_string>(_shaders.Count()));
					for (auto & s : _shaders) result->Append(s.key);
					return result;
				} catch (...) { return 0; }
			}
			virtual oref<IShader> CreateShader(const ucs1_string & name) noexcept override { return _shaders[name]; }
		};
		class VKDeviceStats : public Object
		{
		public:
			oref<VKDeviceAPI> api;
			VkPhysicalDevice physical_device;
			uint max_constants, max_samplers, max_buffers, max_textures, max_per_stage, constant_alignment;
		public:
			VKDeviceStats(VKDeviceAPI * device, VkPhysicalDevice physical) : api(device), physical_device(physical)
			{
				VkPhysicalDeviceProperties props;
				api->Dispatch.vkGetPhysicalDeviceProperties(physical_device, &props);
				constant_alignment = props.limits.nonCoherentAtomSize;
				max_per_stage = props.limits.maxPerStageResources;
				max_constants = min(props.limits.maxPerStageDescriptorUniformBuffers, props.limits.maxDescriptorSetUniformBuffers / 2);
				max_samplers = min(props.limits.maxPerStageDescriptorSamplers, props.limits.maxDescriptorSetSamplers / 2);
				max_buffers = min(props.limits.maxPerStageDescriptorStorageBuffers, props.limits.maxDescriptorSetStorageBuffers / 2);
				max_textures = min(props.limits.maxPerStageDescriptorSampledImages, props.limits.maxDescriptorSetSampledImages / 2);
				if (max_constants > MaxRuntimePerShaderConstants) max_constants = MaxRuntimePerShaderConstants;
				if (max_samplers > MaxRuntimePerShaderSamplers) max_samplers = MaxRuntimePerShaderSamplers;
				if (max_buffers > MaxRuntimePerShaderBuffers) max_buffers = MaxRuntimePerShaderBuffers;
				if (max_textures > MaxRuntimePerShaderTextures) max_textures = MaxRuntimePerShaderTextures;
				if (_device_validation_layer) {
					VKValidationOutput("Vulkan API: numerus maximus selectorum constatorum: %i.\n", reinterpret_cast<char *>(intptr(max_constants)));
					VKValidationOutput("Vulkan API: numerus maximus selectorum exceptorum: %i.\n", reinterpret_cast<char *>(intptr(max_samplers)));
					VKValidationOutput("Vulkan API: numerus maximus selectorum serierum: %i.\n", reinterpret_cast<char *>(intptr(max_buffers)));
					VKValidationOutput("Vulkan API: numerus maximus selectorum texturarum: %i.\n", reinterpret_cast<char *>(intptr(max_textures)));
					VKValidationOutput("Vulkan API: numerus maximus selectorum per stadio: %i.\n", reinterpret_cast<char *>(intptr(max_per_stage)));
					VKValidationOutput("Vulkan API: politio constatorum: %i.\n", reinterpret_cast<char *>(intptr(constant_alignment)));
				}
			}
			virtual ~VKDeviceStats(void) override {}
		};
		class VKDescriptorAllocator : public Object
		{
		public:
			oref<VKDeviceAPI> api;
			VkDescriptorPool pool;
			uint allocations, state; // 0 - usable, 1 - needs reset
		public:
			VKDescriptorAllocator(VKDeviceAPI * device) : api(device), pool(0), allocations(0), state(0) {}
			virtual ~VKDescriptorAllocator(void) override { if (pool) api->Dispatch.vkDestroyDescriptorPool(api->Device, pool, &api->Base->Allocator); }
			bool Initialize(const VkDescriptorPoolSize * sizes, uint sizes_count) noexcept
			{
				if (pool) return true;
				VkDescriptorPoolCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				info.pNext = 0;
				info.flags = 0;
				info.maxSets = _vk_descriptor_pool_size;
				info.poolSizeCount = sizes_count;
				info.pPoolSizes = sizes;
				if (api->Dispatch.vkCreateDescriptorPool(api->Device, &info, &api->Base->Allocator, &pool) != VK_SUCCESS) return false;
				return true;
			}
		};
		class VKPipelineLayout : public Object
		{
		public:
			static constexpr uint selector_mapping_esse_stage_vertex	= 0x00000000;
			static constexpr uint selector_mapping_esse_stage_pixel		= 0x10000000;
			static constexpr uint selector_mapping_esse_stage_mask		= 0xF0000000;
			static constexpr uint selector_mapping_esse_type_constant	= 0x01000000;
			static constexpr uint selector_mapping_esse_type_buffer		= 0x02000000;
			static constexpr uint selector_mapping_esse_type_texture	= 0x03000000;
			static constexpr uint selector_mapping_esse_type_sampler	= 0x04000000;
			static constexpr uint selector_mapping_esse_type_mask		= 0x0F000000;
			static constexpr uint selector_mapping_esse_index_mask		= 0x00FF0000;
			static constexpr uint selector_mapping_esse_mask			= 0xFFFF0000;
			static constexpr uint selector_mapping_vulkan_mask			= 0x0000FFFF;
		public:
			oref<VKDeviceStats> stats;
			VkDescriptorSetLayout descriptor_layout;
			VkPipelineLayout pipeline_layout;
			string symbol;
			Dictionary<uint, uint> smap; // esse selector -> vulkan selector
			uint constants, buffers, textures, samplers, allocate_descriptor_sizes_length;
			object_array<VKDescriptorAllocator> allocators;
			VkDescriptorPoolSize allocate_descriptor_sizes[4];
		public:
			VKPipelineLayout(VKDeviceStats * devstat) : stats(devstat), descriptor_layout(0), pipeline_layout(0), allocators(0x40) {}
			virtual ~VKPipelineLayout(void) override
			{
				auto api = stats->api.Inner();
				if (pipeline_layout) api->Dispatch.vkDestroyPipelineLayout(api->Device, pipeline_layout, &api->Base->Allocator);
				if (descriptor_layout) api->Dispatch.vkDestroyDescriptorSetLayout(api->Device, descriptor_layout, &api->Base->Allocator);
			}
			static string MakeLayoutSymbol(VKShader * vertex, VKShader * pixel)
			{
				array<uint> linear(vertex->GetResourceMapping().GetLength() + pixel->GetResourceMapping().GetLength());
				linear.Append(vertex->GetResourceMapping());
				linear.Append(pixel->GetResourceMapping());
				SortArray(linear);
				dynamic_string_ucs4 result;
				for (auto & i : linear) result += string(i, HexadecimalBase, 8);
				return result;
			}
			bool Initialize(VKShader * vertex, VKShader * pixel) noexcept
			{
				auto api = stats->api.Inner();
				array<VkDescriptorSetLayoutBinding> binding_desc(1);
				try {
					symbol = MakeLayoutSymbol(vertex, pixel);
					for (auto & m : vertex->GetResourceMapping()) if (!smap.Append(m & selector_mapping_esse_mask, m & selector_mapping_vulkan_mask));
					for (auto & m : pixel->GetResourceMapping()) if (!smap.Append(m & selector_mapping_esse_mask, m & selector_mapping_vulkan_mask));
					uint vulkan_desc_max = 0;
					uint per_stage_constants[2] = { 0, 0 };
					uint per_stage_buffers[2] = { 0, 0 };
					uint per_stage_textures[2] = { 0, 0 };
					uint per_stage_samplers[2] = { 0, 0 };
					for (auto & m : smap) {
						if (m.value > vulkan_desc_max) vulkan_desc_max = m.value;
						if ((m.key & selector_mapping_esse_stage_mask) == selector_mapping_esse_stage_vertex) {
							if ((m.key & selector_mapping_esse_type_mask) == selector_mapping_esse_type_constant) per_stage_constants[0]++;
							else if ((m.key & selector_mapping_esse_type_mask) == selector_mapping_esse_type_buffer) per_stage_buffers[0]++;
							else if ((m.key & selector_mapping_esse_type_mask) == selector_mapping_esse_type_texture) per_stage_textures[0]++;
							else if ((m.key & selector_mapping_esse_type_mask) == selector_mapping_esse_type_sampler) per_stage_samplers[0]++;
							else return false;
						} else if ((m.key & selector_mapping_esse_stage_mask) == selector_mapping_esse_stage_pixel) {
							if ((m.key & selector_mapping_esse_type_mask) == selector_mapping_esse_type_constant) per_stage_constants[1]++;
							else if ((m.key & selector_mapping_esse_type_mask) == selector_mapping_esse_type_buffer) per_stage_buffers[1]++;
							else if ((m.key & selector_mapping_esse_type_mask) == selector_mapping_esse_type_texture) per_stage_textures[1]++;
							else if ((m.key & selector_mapping_esse_type_mask) == selector_mapping_esse_type_sampler) per_stage_samplers[1]++;
							else return false;
						} else return false;
					}
					if (per_stage_constants[0] + per_stage_buffers[0] + per_stage_textures[0] + per_stage_samplers[0] > stats->max_per_stage) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: (error) numerus selectorum auxiliorum per stadio verticis (%i) grandis nimium.\n", reinterpret_cast<char *>(intptr(per_stage_constants[0] + per_stage_buffers[0] + per_stage_textures[0] + per_stage_samplers[0])));
						return false;
					}
					if (per_stage_constants[1] + per_stage_buffers[1] + per_stage_textures[1] + per_stage_samplers[1] > stats->max_per_stage) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: (error) numerus selectorum auxiliorum per stadio puncti (%i) grandis nimium.\n", reinterpret_cast<char *>(intptr(per_stage_constants[1] + per_stage_buffers[1] + per_stage_textures[1] + per_stage_samplers[1])));
						return false;
					}
					if (per_stage_constants[0] > stats->max_constants) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: (error) numerus selectorum constatorum per stadio verticis (%i) grandis nimium.\n", reinterpret_cast<char *>(intptr(per_stage_constants[0])));
						return false;
					}
					if (per_stage_buffers[0] > stats->max_buffers) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: (error) numerus selectorum serierum per stadio verticis (%i) grandis nimium.\n", reinterpret_cast<char *>(intptr(per_stage_buffers[0])));
						return false;
					}
					if (per_stage_textures[0] > stats->max_textures) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: (error) numerus selectorum texturarum per stadio verticis (%i) grandis nimium.\n", reinterpret_cast<char *>(intptr(per_stage_textures[0])));
						return false;
					}
					if (per_stage_samplers[0] > stats->max_samplers) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: (error) numerus selectorum exceptorum per stadio verticis (%i) grandis nimium.\n", reinterpret_cast<char *>(intptr(per_stage_samplers[0])));
						return false;
					}
					if (per_stage_constants[1] > stats->max_constants) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: (error) numerus selectorum constatorum per stadio puncti (%i) grandis nimium.\n", reinterpret_cast<char *>(intptr(per_stage_constants[1])));
						return false;
					}
					if (per_stage_buffers[1] > stats->max_buffers) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: (error) numerus selectorum serierum per stadio puncti (%i) grandis nimium.\n", reinterpret_cast<char *>(intptr(per_stage_buffers[1])));
						return false;
					}
					if (per_stage_textures[1] > stats->max_textures) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: (error) numerus selectorum texturarum per stadio puncti (%i) grandis nimium.\n", reinterpret_cast<char *>(intptr(per_stage_textures[1])));
						return false;
					}
					if (per_stage_samplers[1] > stats->max_samplers) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: (error) numerus selectorum exceptorum per stadio puncti (%i) grandis nimium.\n", reinterpret_cast<char *>(intptr(per_stage_samplers[1])));
						return false;
					}
					constants = per_stage_constants[0] + per_stage_constants[1];
					buffers = per_stage_buffers[0] + per_stage_buffers[1];
					textures = per_stage_textures[0] + per_stage_textures[1];
					samplers = per_stage_samplers[0] + per_stage_samplers[1];
					binding_desc.SetLength(vulkan_desc_max + 1);
				} catch (...) { return false; }
				VkDescriptorSetLayoutCreateInfo descriptor_layout_info;
				VkPipelineLayoutCreateInfo pipeline_layout_info;
				for (uintptr i = 0; i < binding_desc.GetLength(); i++) {
					auto & bind = binding_desc[i];
					uint target = 0;
					bind.pImmutableSamplers = 0;
					bind.binding = i;
					for (auto & m : smap) if (m.value == i) { target = m.key; break; }
					if (target) {
						bind.descriptorCount = 1;
						if ((target & selector_mapping_esse_type_mask) == selector_mapping_esse_type_constant) bind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						else if ((target & selector_mapping_esse_type_mask) == selector_mapping_esse_type_buffer) bind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						else if ((target & selector_mapping_esse_type_mask) == selector_mapping_esse_type_texture) bind.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
						else if ((target & selector_mapping_esse_type_mask) == selector_mapping_esse_type_sampler) bind.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
						if ((target & selector_mapping_esse_stage_mask) == selector_mapping_esse_stage_vertex) bind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
						else if ((target & selector_mapping_esse_stage_mask) == selector_mapping_esse_stage_pixel) bind.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
					} else {
						bind.descriptorCount = 0;
						bind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						bind.stageFlags = 0;
					}
				}
				descriptor_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				descriptor_layout_info.pNext = 0;
				descriptor_layout_info.flags = 0;
				descriptor_layout_info.bindingCount = binding_desc.GetLength();
				descriptor_layout_info.pBindings = binding_desc;
				pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipeline_layout_info.pNext = 0;
				pipeline_layout_info.flags = 0;
				pipeline_layout_info.setLayoutCount = 1;
				pipeline_layout_info.pSetLayouts = &descriptor_layout;
				pipeline_layout_info.pushConstantRangeCount = 0;
				pipeline_layout_info.pPushConstantRanges = 0;
				if (api->Dispatch.vkCreateDescriptorSetLayout(api->Device, &descriptor_layout_info, &api->Base->Allocator, &descriptor_layout) != VK_SUCCESS) return false;
				if (api->Dispatch.vkCreatePipelineLayout(api->Device, &pipeline_layout_info, &api->Base->Allocator, &pipeline_layout) != VK_SUCCESS) return false;
				allocate_descriptor_sizes_length = 0;
				Memory::ZeroMemory(&allocate_descriptor_sizes, sizeof(allocate_descriptor_sizes));
				if (constants) {
					allocate_descriptor_sizes[allocate_descriptor_sizes_length].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					allocate_descriptor_sizes[allocate_descriptor_sizes_length].descriptorCount = _vk_descriptor_pool_size * constants;
					allocate_descriptor_sizes_length++;
				}
				if (buffers) {
					allocate_descriptor_sizes[allocate_descriptor_sizes_length].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					allocate_descriptor_sizes[allocate_descriptor_sizes_length].descriptorCount = _vk_descriptor_pool_size * buffers;
					allocate_descriptor_sizes_length++;
				}
				if (textures) {
					allocate_descriptor_sizes[allocate_descriptor_sizes_length].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					allocate_descriptor_sizes[allocate_descriptor_sizes_length].descriptorCount = _vk_descriptor_pool_size * textures;
					allocate_descriptor_sizes_length++;
				}
				if (samplers) {
					allocate_descriptor_sizes[allocate_descriptor_sizes_length].type = VK_DESCRIPTOR_TYPE_SAMPLER;
					allocate_descriptor_sizes[allocate_descriptor_sizes_length].descriptorCount = _vk_descriptor_pool_size * samplers;
					allocate_descriptor_sizes_length++;
				}
				try {
					auto pool = owrap(new VKDescriptorAllocator(api));
					if (!pool->Initialize(allocate_descriptor_sizes, allocate_descriptor_sizes_length)) return false;
					allocators.Append(pool);
				} catch (...) { return false; }
				return true;
			}
			bool AllocateDescriptorSet(VkDescriptorSet & set, oref<VKDescriptorAllocator> & pool) noexcept
			{
				auto api = stats->api.Inner();
				for (auto & p : allocators) if (p.state == 1 && p.GetReferenceCount() == 1) {
					api->Dispatch.vkResetDescriptorPool(api->Device, p.pool, 0);
					p.allocations = p.state = 0;
				}
				VkDescriptorSetAllocateInfo info;
				info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				info.pNext = 0;
				info.pSetLayouts = &descriptor_layout;
				info.descriptorSetCount = 1;
				VKDescriptorAllocator * pool_used = 0;
				VkDescriptorSet result;
				for (auto & p : allocators) if (p.state == 0) {
					info.descriptorPool = p.pool;
					if (api->Dispatch.vkAllocateDescriptorSets(api->Device, &info, &result) == VK_SUCCESS) {
						pool_used = &p;
						p.allocations++;
						if (p.allocations >= _vk_descriptor_pool_size) p.state = 1;
						break;
					} else p.state = 1;
				}
				if (pool_used) { set = result; pool = pool_used; return true; }
				oref<VKDescriptorAllocator> new_pool;
				try {
					new_pool = owrap(new VKDescriptorAllocator(api));
					if (!new_pool->Initialize(allocate_descriptor_sizes, allocate_descriptor_sizes_length)) return false;
					allocators.Append(new_pool);
				} catch (...) { return false; }
				info.descriptorPool = new_pool->pool;
				if (api->Dispatch.vkAllocateDescriptorSets(api->Device, &info, &result) == VK_SUCCESS) { new_pool->allocations++; set = result; pool = new_pool; return true; } else return false;
			}
		};
		class VKPipelineState : public IPipelineState
		{
			friend class VKPass;
			friend class VKQueue;
			friend class VKDevice;
		private:
			oref<VKDeviceAPI> _api;
			oref<VKPipelineLayout> _layout;
			IDevice * _parent;
			VkPipeline _pipeline;
		public:
			VKPipelineState(VKDeviceAPI * api, IDevice * parent, VKPipelineLayout * layout) : _api(api), _parent(parent), _layout(layout), _pipeline(0) {}
			virtual ~VKPipelineState(void) override { if (_pipeline) _api->Dispatch.vkDestroyPipeline(_api->Device, _pipeline, &_api->Base->Allocator); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKPipelineState"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent; }
			VKPipelineLayout * GetLayout(void) noexcept { return _layout; }
			VkPipeline GetPipeline(void) noexcept { return _pipeline; }
		};
		class VKSamplerState : public ISamplerState
		{
			friend class VKPass;
			friend class VKQueue;
			friend class VKDevice;
			friend class VKDeviceDeferredContext;
			friend class VKDeviceImmediateContext;
			friend class VKDeviceContext2D;
		private:
			oref<VKDeviceAPI> _api;
			IDevice * _parent;
			VkSampler _sampler;
		public:
			VKSamplerState(VKDeviceAPI * api, IDevice * parent) : _api(api), _parent(parent), _sampler(0) {}
			virtual ~VKSamplerState(void) override { if (_sampler) _api->Dispatch.vkDestroySampler(_api->Device, _sampler, &_api->Base->Allocator); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKSamplerState"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent; }
			VkSampler GetSampler(void) noexcept { return _sampler; }
		};
		class VKBuffer : public IBuffer
		{
			friend class VKPass;
			friend class VKQueue;
			friend class VKDevice;
			friend class VKDeviceDeferredContext;
			friend class VKDeviceImmediateContext;
			friend class VKDeviceContext2D;
		protected:
			oref<VKDeviceAPI> _api;
			IDevice * _parent;
			BufferDesc _desc;
			oref<VKBuffer> _transit;
			VkBuffer _buffer;
			VkDeviceMemory _memory;
		public:
			VKBuffer(VKDeviceAPI * api, IDevice * parent) : _api(api), _parent(parent), _buffer(0), _memory(0) {}
			virtual ~VKBuffer(void) override
			{
				if (_buffer) _api->Dispatch.vkDestroyBuffer(_api->Device, _buffer, &_api->Base->Allocator);
				if (_memory) _api->Dispatch.vkFreeMemory(_api->Device, _memory, &_api->Base->Allocator);
			}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKBuffer"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent; }
			virtual ResourceType GetResourceType(void) noexcept override { return ResourceType::Buffer; }
			virtual ResourceMemoryPool GetMemoryPool(void) noexcept override { return _desc.MemoryPool; }
			virtual uint32 GetResourceUsage(void) noexcept override { return _desc.Usage; }
			virtual uint32 GetLength(void) noexcept override { return _desc.Length; }
			VkBuffer GetBuffer(void) noexcept { return _buffer; }
		};
		class VKTexture : public ITexture
		{
			friend class VKPass;
			friend class VKQueue;
			friend class VKPresentationLayer;
			friend class VKDevice;
			friend class VKDeviceImmediateContext;
			friend class VKDeviceContext2D;
			friend class VKDeviceFactory;
		private:
			oref<VKDeviceAPI> _api;
			IDevice * _parent;
			TextureDesc _desc;
			oref<Object> _dependency;
			oref<VKTexture> _transit;
			oref<VKSharedObject> _shared;
			VkImage _image;
			VkImageView _view, _view_stencil;
			VkDeviceMemory _memory;
			VkImageLayout _current_layout;
		public:
			VKTexture(VKDeviceAPI * api, IDevice * parent) : _api(api), _parent(parent), _image(0), _view(0), _view_stencil(0), _memory(0) {}
			virtual ~VKTexture(void) override
			{
				if (_view_stencil) _api->Dispatch.vkDestroyImageView(_api->Device, _view_stencil, &_api->Base->Allocator);
				if (_view) _api->Dispatch.vkDestroyImageView(_api->Device, _view, &_api->Base->Allocator);
				if (!_dependency) {
					if (_image) _api->Dispatch.vkDestroyImage(_api->Device, _image, &_api->Base->Allocator);
					if (_memory) _api->Dispatch.vkFreeMemory(_api->Device, _memory, &_api->Base->Allocator);
				}
			}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKTexture"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent; }
			virtual ResourceType GetResourceType(void) noexcept override { return ResourceType::Texture; }
			virtual ResourceMemoryPool GetMemoryPool(void) noexcept override { return _desc.MemoryPool; }
			virtual uint32 GetResourceUsage(void) noexcept override { return _desc.Usage; }
			virtual TextureType GetTextureType(void) noexcept override { return _desc.Type; }
			virtual PixelFormat GetPixelFormat(void) noexcept override { return _desc.Format; }
			virtual uint32 GetWidth(void) noexcept override { return _desc.Width; }
			virtual uint32 GetHeight(void) noexcept override { return _desc.Height; }
			virtual uint32 GetDepth(void) noexcept override { return _desc.Type == TextureType::Type3D ? _desc.Depth : 1; }
			virtual uint32 GetMipmapCount(void) noexcept override { return _desc.MipmapCount; }
			virtual uint32 GetArraySize(void) noexcept override { return _desc.Type == TextureType::Type3D ? _desc.ArraySize : 1; }
			VkImageView GetView(void) noexcept { return _view; }
			VkImageLayout GetLayout(void) noexcept { return _current_layout; }
		};
		class VKConstantPool : public VKBuffer
		{
		public:
			uint allocated, used, offset;
			uint8 * memory_mapping;
		public:
			VKConstantPool(VKDeviceAPI * api, IDevice * parent) : VKBuffer(api, parent), allocated(0), used(0), offset(0), memory_mapping(0) {}
			virtual ~VKConstantPool(void) override { if (memory_mapping) _api->Dispatch.vkUnmapMemory(_api->Device, _memory); }
			bool MapMemory(void) noexcept
			{
				if (memory_mapping) return true;
				void * address;
				if (_api->Dispatch.vkMapMemory(_api->Device, _memory, 0, allocated, 0, &address) != VK_SUCCESS) return false;
				memory_mapping = reinterpret_cast<uint8 *>(address);
				return true;
			}
			static oref<VKConstantPool> Allocate(IDevice * parent_device) noexcept;
		};
		class VKSelectorState : public Object
		{
		public:
			oref<VKPipelineState> state;
			oref<VKPipelineLayout> layout;
			VKBuffer		* vertex_constant_buffers		[MaxRuntimePerShaderConstants];
			VKSamplerState	* vertex_samplers				[MaxRuntimePerShaderSamplers];
			VKBuffer		* vertex_buffers				[MaxRuntimePerShaderBuffers];
			VKTexture		* vertex_textures				[MaxRuntimePerShaderTextures];
			VKBuffer		* pixel_constant_buffers		[MaxRuntimePerShaderConstants];
			VKSamplerState	* pixel_samplers				[MaxRuntimePerShaderSamplers];
			VKBuffer		* pixel_buffers					[MaxRuntimePerShaderBuffers];
			VKTexture		* pixel_textures				[MaxRuntimePerShaderTextures];
			VkDescriptorBufferInfo vertex_constant_ranges	[MaxRuntimePerShaderConstants];
			VkDescriptorBufferInfo pixel_constant_ranges	[MaxRuntimePerShaderConstants];
			uint32 vc_set_mask, vs_set_mask, vb_set_mask[4], vt_set_mask[4];
			uint32 pc_set_mask, ps_set_mask, pb_set_mask[4], pt_set_mask[4];
			uint32 update_pipeline_layout;
		public:
			VKSelectorState(void) : vc_set_mask(0), vs_set_mask(0), pc_set_mask(0), ps_set_mask(0), update_pipeline_layout(0) { for (uint i = 0; i < 4; i++) vb_set_mask[i] = vt_set_mask[i] = pb_set_mask[i] = pt_set_mask[i] = 0; }
			virtual ~VKSelectorState(void) override
			{
				for (uint i = 0; i < MaxRuntimePerShaderConstants; i++) if (vc_set_mask & (1U << i)) if (vertex_constant_buffers[i]) vertex_constant_buffers[i]->Release();
				for (uint i = 0; i < MaxRuntimePerShaderSamplers; i++) if (vs_set_mask & (1U << i)) if (vertex_samplers[i]) vertex_samplers[i]->Release();
				for (uint i = 0; i < MaxRuntimePerShaderBuffers; i++) if (vb_set_mask[i >> 5] & (1U << (i & 0x1F))) if (vertex_buffers[i]) vertex_buffers[i]->Release();
				for (uint i = 0; i < MaxRuntimePerShaderTextures; i++) if (vt_set_mask[i >> 5] & (1U << (i & 0x1F))) if (vertex_textures[i]) vertex_textures[i]->Release();
				for (uint i = 0; i < MaxRuntimePerShaderConstants; i++) if (pc_set_mask & (1U << i)) if (pixel_constant_buffers[i]) pixel_constant_buffers[i]->Release();
				for (uint i = 0; i < MaxRuntimePerShaderSamplers; i++) if (ps_set_mask & (1U << i)) if (pixel_samplers[i]) pixel_samplers[i]->Release();
				for (uint i = 0; i < MaxRuntimePerShaderBuffers; i++) if (pb_set_mask[i >> 5] & (1U << (i & 0x1F))) if (pixel_buffers[i]) pixel_buffers[i]->Release();
				for (uint i = 0; i < MaxRuntimePerShaderTextures; i++) if (pt_set_mask[i >> 5] & (1U << (i & 0x1F))) if (pixel_textures[i]) pixel_textures[i]->Release();
			}
			void UpdatePipeline(IPipelineState * pstate) noexcept
			{
				if (state != pstate) update_pipeline_layout |= 1;
				state = pstate ? static_cast<VKPipelineState *>(pstate) : 0;
				if (state) {
					if (layout.Inner() != state->GetLayout()) {
						layout = state->GetLayout();
						update_pipeline_layout |= 2;
					}
				} else { layout.Clear(); update_pipeline_layout |= 2; }
			}
			void UpdateSelector(uint domain, uint index, Object * object, VkDeviceSize origin = 0, VkDeviceSize size = VK_WHOLE_SIZE) noexcept
			{
				if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant)) {
					if (index >= MaxRuntimePerShaderConstants) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Index #%i selectoris constati falsa.\n", reinterpret_cast<char *>(intptr(object)));
						return;
					}
					if ((vc_set_mask & (1U << index)) && vertex_constant_buffers[index]) vertex_constant_buffers[index]->Release();
					vc_set_mask |= (1U << index);
					vertex_constant_buffers[index] = static_cast<VKBuffer *>(object);
					vertex_constant_ranges[index].buffer = 0;
					vertex_constant_ranges[index].offset = origin;
					vertex_constant_ranges[index].range = size;
					if (object) object->Retain();
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_sampler)) {
					if (index >= MaxRuntimePerShaderSamplers) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Index #%i selectoris exceptoris falsa.\n", reinterpret_cast<char *>(intptr(object)));
						return;
					}
					if ((vs_set_mask & (1U << index)) && vertex_samplers[index]) vertex_samplers[index]->Release();
					vs_set_mask |= (1U << index);
					vertex_samplers[index] = static_cast<VKSamplerState *>(object);
					if (object) object->Retain();
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer)) {
					if (index >= MaxRuntimePerShaderBuffers) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Index #%i selectoris seriei falsa.\n", reinterpret_cast<char *>(intptr(object)));
						return;
					}
					if ((vb_set_mask[index >> 5] & (1U << (index & 0x1F))) && vertex_buffers[index]) vertex_buffers[index]->Release();
					vb_set_mask[index >> 5] |= (1U << (index & 0x1F));
					vertex_buffers[index] = static_cast<VKBuffer *>(object);
					if (object) object->Retain();
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_texture)) {
					if (index >= MaxRuntimePerShaderTextures) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Index #%i selectoris texturae falsa.\n", reinterpret_cast<char *>(intptr(object)));
						return;
					}
					if ((vt_set_mask[index >> 5] & (1U << (index & 0x1F))) && vertex_textures[index]) vertex_textures[index]->Release();
					vt_set_mask[index >> 5] |= (1U << (index & 0x1F));
					vertex_textures[index] = static_cast<VKTexture *>(object);
					if (object) object->Retain();
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_constant)) {
					if (index >= MaxRuntimePerShaderConstants) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Index #%i selectoris constati falsa.\n", reinterpret_cast<char *>(intptr(object)));
						return;
					}
					if ((pc_set_mask & (1U << index)) && pixel_constant_buffers[index]) pixel_constant_buffers[index]->Release();
					pc_set_mask |= (1U << index);
					pixel_constant_buffers[index] = static_cast<VKBuffer *>(object);
					pixel_constant_ranges[index].buffer = 0;
					pixel_constant_ranges[index].offset = origin;
					pixel_constant_ranges[index].range = size;
					if (object) object->Retain();
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_sampler)) {
					if (index >= MaxRuntimePerShaderSamplers) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Index #%i selectoris exceptoris falsa.\n", reinterpret_cast<char *>(intptr(object)));
						return;
					}
					if ((ps_set_mask & (1U << index)) && pixel_samplers[index]) pixel_samplers[index]->Release();
					ps_set_mask |= (1U << index);
					pixel_samplers[index] = static_cast<VKSamplerState *>(object);
					if (object) object->Retain();
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_buffer)) {
					if (index >= MaxRuntimePerShaderBuffers) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Index #%i selectoris seriei falsa.\n", reinterpret_cast<char *>(intptr(object)));
						return;
					}
					if ((pb_set_mask[index >> 5] & (1U << (index & 0x1F))) && pixel_buffers[index]) pixel_buffers[index]->Release();
					pb_set_mask[index >> 5] |= (1U << (index & 0x1F));
					pixel_buffers[index] = static_cast<VKBuffer *>(object);
					if (object) object->Retain();
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture)) {
					if (index >= MaxRuntimePerShaderTextures) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Index #%i selectoris texturae falsa.\n", reinterpret_cast<char *>(intptr(object)));
						return;
					}
					if ((pt_set_mask[index >> 5] & (1U << (index & 0x1F))) && pixel_textures[index]) pixel_textures[index]->Release();
					pt_set_mask[index >> 5] |= (1U << (index & 0x1F));
					pixel_textures[index] = static_cast<VKTexture *>(object);
					if (object) object->Retain();
				} else abort();
				update_pipeline_layout |= 2;
			}
			void WriteSelectors(VKDeviceAPI * api, uint & index, VkWriteDescriptorSet * write) noexcept { api->Dispatch.vkUpdateDescriptorSets(api->Device, index, write, 0, 0); index = 0; }
			bool PushSelector(VkDescriptorSet set, Set<ResourceHandle> & retain, uint & index, VkDescriptorImageInfo * image, VkDescriptorBufferInfo * buffer, VkWriteDescriptorSet * write, uint position, uint bind, VKSamplerState * stub_sampler)
			{
				uint domain = position & (VKPipelineLayout::selector_mapping_esse_stage_mask | VKPipelineLayout::selector_mapping_esse_type_mask);
				uint at = (position & VKPipelineLayout::selector_mapping_esse_index_mask) >> 16U;
				write[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write[index].pNext = 0;
				write[index].dstSet = set;
				write[index].dstArrayElement = 0;
				write[index].descriptorCount = 1;
				write[index].pTexelBufferView = 0;
				write[index].dstBinding = bind;
				if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant)) {
					if (!(vc_set_mask & (1U << at)) || !vertex_constant_buffers[at]) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Selector #%i constati stadii verticum requisitum, sed nullum est.\n", reinterpret_cast<char *>(intptr(at)));
						return false;
					}
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					write[index].pBufferInfo = buffer + index;
					write[index].pImageInfo = 0;
					buffer[index].buffer = vertex_constant_buffers[at]->GetBuffer();
					buffer[index].offset = vertex_constant_ranges[at].offset;
					buffer[index].range = vertex_constant_ranges[at].range;
					retain.AddElement(vertex_constant_buffers[at]);
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_constant)) {
					if (!(pc_set_mask & (1U << at)) || !pixel_constant_buffers[at]) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Selector #%i constati stadii punctorum requisitum, sed nullum est.\n", reinterpret_cast<char *>(intptr(at)));
						return false;
					}
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					write[index].pBufferInfo = buffer + index;
					write[index].pImageInfo = 0;
					buffer[index].buffer = pixel_constant_buffers[at]->GetBuffer();
					buffer[index].offset = pixel_constant_ranges[at].offset;
					buffer[index].range = pixel_constant_ranges[at].range;
					retain.AddElement(pixel_constant_buffers[at]);
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer)) {
					if (!(vb_set_mask[at >> 5] & (1U << (at & 0x1F))) || !vertex_buffers[at]) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Selector #%i seriei stadii verticum requisitum, sed nullum est.\n", reinterpret_cast<char *>(intptr(at)));
						return false;
					}
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					write[index].pBufferInfo = buffer + index;
					write[index].pImageInfo = 0;
					buffer[index].buffer = vertex_buffers[at]->GetBuffer();
					buffer[index].offset = 0;
					buffer[index].range = VK_WHOLE_SIZE;
					retain.AddElement(vertex_buffers[at]);
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_buffer)) {
					if (!(pb_set_mask[at >> 5] & (1U << (at & 0x1F))) || !pixel_buffers[at]) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Selector #%i seriei stadii punctorum requisitum, sed nullum est.\n", reinterpret_cast<char *>(intptr(at)));
						return false;
					}
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					write[index].pBufferInfo = buffer + index;
					write[index].pImageInfo = 0;
					buffer[index].buffer = pixel_buffers[at]->GetBuffer();
					buffer[index].offset = 0;
					buffer[index].range = VK_WHOLE_SIZE;
					retain.AddElement(pixel_buffers[at]);
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_texture)) {
					if (!(vt_set_mask[at >> 5] & (1U << (at & 0x1F))) || !vertex_textures[at]) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Selector #%i texturae stadii verticum requisitum, sed nullum est.\n", reinterpret_cast<char *>(intptr(at)));
						return false;
					}
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					write[index].pBufferInfo = 0;
					write[index].pImageInfo = image + index;
					image[index].sampler = 0;
					image[index].imageView = vertex_textures[at]->GetView();
					image[index].imageLayout = vertex_textures[at]->GetLayout();
					retain.AddElement(vertex_textures[at]);
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture)) {
					if (!(pt_set_mask[at >> 5] & (1U << (at & 0x1F))) || !pixel_textures[at]) {
						if (_device_validation_layer) VKValidationOutput("Vulkan API: Selector #%i texturae stadii punctorum requisitum, sed nullum est.\n", reinterpret_cast<char *>(intptr(at)));
						return false;
					}
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					write[index].pBufferInfo = 0;
					write[index].pImageInfo = image + index;
					image[index].sampler = 0;
					image[index].imageView = pixel_textures[at]->GetView();
					image[index].imageLayout = pixel_textures[at]->GetLayout();
					retain.AddElement(pixel_textures[at]);
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_sampler)) {
					VKSamplerState * sampler;
					if (!(vs_set_mask & (1U << at)) || !vertex_samplers[at]) sampler = stub_sampler; else sampler = vertex_samplers[at];
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
					write[index].pBufferInfo = 0;
					write[index].pImageInfo = image + index;
					image[index].sampler = sampler->GetSampler();
					image[index].imageView = 0;
					image[index].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					retain.AddElement(sampler);
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_sampler)) {
					VKSamplerState * sampler;
					if (!(ps_set_mask & (1U << at)) || !pixel_samplers[at]) sampler = stub_sampler; else sampler = pixel_samplers[at];
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
					write[index].pBufferInfo = 0;
					write[index].pImageInfo = image + index;
					image[index].sampler = sampler->GetSampler();
					image[index].imageView = 0;
					image[index].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					retain.AddElement(sampler);
				} else abort();
				index++;
				return true;
			}
			bool PushState(VkCommandBuffer command, Set<ResourceHandle> & retain, VKSamplerState * stub_sampler) noexcept
			{
				try {
					auto api = layout->stats->api.Inner();
					if (update_pipeline_layout & 1) {
						if (!state) {
							if (_device_validation_layer) VKValidationOutput("Vulkan API: Requisitum reddendi admissum est, sed status oleiductus nullus est.\n");
							return false;
						}
						retain.AddElement(state.Inner());
						api->Dispatch.vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, state->GetPipeline());
					}
					if (update_pipeline_layout & 2) {
						if (!layout) {
							if (_device_validation_layer) VKValidationOutput("Vulkan API: Requisitum reddendi admissum est, sed status oleiductus nullus est.\n");
							return false;
						}
						VkDescriptorSet set;
						oref<VKDescriptorAllocator> alloc;
						if (!layout->AllocateDescriptorSet(set, alloc)) {
							if (_device_validation_layer) VKValidationOutput("Vulkan API: Error allocationis memoriae pro selectoribus.\n");
							return false;
						}
						uint index = 0;
						VkDescriptorImageInfo image[8];
						VkDescriptorBufferInfo buffer[8];
						VkWriteDescriptorSet write[8];
						for (auto & m : layout->smap) {
							if (!PushSelector(set, retain, index, image, buffer, write, m.key, m.value, stub_sampler)) return false;
							if (index == 8) WriteSelectors(api, index, write);
						}
						if (index) WriteSelectors(api, index, write);
						retain.AddElement(alloc.Inner());
						api->Dispatch.vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, layout->pipeline_layout, 0, 1, &set, 0, 0);
					}
					update_pipeline_layout = 0;
					return true;
				} catch (...) { return false; }
			}
		};
		class VKSelectorState2D : public Object
		{
		public:
			static constexpr uint selector_constant_global	= 0;
			static constexpr uint selector_constant_local	= 1;
			static constexpr uint selector_vertex_buffer	= 0;
			static constexpr uint selector_color_surface	= 0;
			static constexpr uint selector_mask_surface		= 1;
		public:
			oref<VKPipelineState> state;
			oref<VKPipelineLayout> layout;
			oref<VKBuffer> constant_global, constant_local, vertex;
			oref<VKTexture> surface, mask;
			VkDescriptorBufferInfo constant_global_range, constant_local_range;
			uint32 update_pipeline_layout;
		public:
			VKSelectorState2D(void) : update_pipeline_layout(0) {}
			virtual ~VKSelectorState2D(void) override {}
			void UpdatePipeline(IPipelineState * pstate) noexcept
			{
				if (state != pstate) update_pipeline_layout |= 1;
				state = pstate ? static_cast<VKPipelineState *>(pstate) : 0;
				if (state) {
					if (layout.Inner() != state->GetLayout()) {
						layout = state->GetLayout();
						update_pipeline_layout |= 2;
					}
				} else { layout.Clear(); update_pipeline_layout |= 2; }
			}
			void UpdateSelector(uint domain, uint index, Object * object, VkDeviceSize origin = 0, VkDeviceSize size = VK_WHOLE_SIZE) noexcept
			{
				if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant)) {
					if (index == selector_constant_global) {
						constant_global = static_cast<VKBuffer *>(object);
						constant_global_range.buffer = 0;
						constant_global_range.offset = origin;
						constant_global_range.range = size;
					} else if (index == selector_constant_local) {
						constant_local = static_cast<VKBuffer *>(object);
						constant_local_range.buffer = 0;
						constant_local_range.offset = origin;
						constant_local_range.range = size;
					} else abort();
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer)) {
					if (index == selector_vertex_buffer) vertex = static_cast<VKBuffer *>(object);
					else abort();
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture)) {
					if (index == selector_color_surface) surface = static_cast<VKTexture *>(object);
					else if (index == selector_mask_surface) mask = static_cast<VKTexture *>(object);
					else abort();
				} else abort();
				update_pipeline_layout |= 2;
			}
			void WriteSelectors(VKDeviceAPI * api, uint & index, VkWriteDescriptorSet * write) noexcept { api->Dispatch.vkUpdateDescriptorSets(api->Device, index, write, 0, 0); index = 0; }
			bool PushSelector(VkDescriptorSet set, Set<ResourceHandle> & retain, uint & index, VkDescriptorImageInfo * image, VkDescriptorBufferInfo * buffer, VkWriteDescriptorSet * write, uint position, uint bind, VKSamplerState * stub_sampler)
			{
				uint domain = position & (VKPipelineLayout::selector_mapping_esse_stage_mask | VKPipelineLayout::selector_mapping_esse_type_mask);
				uint at = (position & VKPipelineLayout::selector_mapping_esse_index_mask) >> 16U;
				write[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write[index].pNext = 0;
				write[index].dstSet = set;
				write[index].dstArrayElement = 0;
				write[index].descriptorCount = 1;
				write[index].pTexelBufferView = 0;
				write[index].dstBinding = bind;
				if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant)) {
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					write[index].pBufferInfo = buffer + index;
					write[index].pImageInfo = 0;
					if (at) {
						buffer[index].buffer = constant_local->GetBuffer();
						buffer[index].offset = constant_local_range.offset;
						buffer[index].range = constant_local_range.range;
						retain.AddElement(constant_local.Inner());
					} else {
						buffer[index].buffer = constant_global->GetBuffer();
						buffer[index].offset = constant_global_range.offset;
						buffer[index].range = constant_global_range.range;
						retain.AddElement(constant_global.Inner());
					}
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer)) {
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					write[index].pBufferInfo = buffer + index;
					write[index].pImageInfo = 0;
					buffer[index].buffer = vertex->GetBuffer();
					buffer[index].offset = 0;
					buffer[index].range = VK_WHOLE_SIZE;
					retain.AddElement(vertex.Inner());
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture)) {
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					write[index].pBufferInfo = 0;
					write[index].pImageInfo = image + index;
					if (at) {
						image[index].sampler = 0;
						image[index].imageView = mask->GetView();
						image[index].imageLayout = mask->GetLayout();
						retain.AddElement(mask.Inner());
					} else {
						image[index].sampler = 0;
						image[index].imageView = surface->GetView();
						image[index].imageLayout = surface->GetLayout();
						retain.AddElement(surface.Inner());
					}
				} else if (domain == (VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_sampler)) {
					write[index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
					write[index].pBufferInfo = 0;
					write[index].pImageInfo = image + index;
					image[index].sampler = stub_sampler->GetSampler();
					image[index].imageView = 0;
					image[index].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					retain.AddElement(stub_sampler);
				} else abort();
				index++;
				return true;
			}
			bool PushState(VkCommandBuffer command, Set<ResourceHandle> & retain, VKSamplerState * stub_sampler) noexcept
			{
				try {
					auto api = layout->stats->api.Inner();
					if (update_pipeline_layout & 1) {
						if (!state) {
							if (_device_validation_layer) VKValidationOutput("Vulkan API: Requisitum reddendi admissum est, sed status oleiductus nullus est.\n");
							return false;
						}
						retain.AddElement(state.Inner());
						api->Dispatch.vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, state->GetPipeline());
					}
					if (update_pipeline_layout & 2) {
						if (!layout) {
							if (_device_validation_layer) VKValidationOutput("Vulkan API: Requisitum reddendi admissum est, sed status oleiductus nullus est.\n");
							return false;
						}
						VkDescriptorSet set;
						oref<VKDescriptorAllocator> alloc;
						if (!layout->AllocateDescriptorSet(set, alloc)) {
							if (_device_validation_layer) VKValidationOutput("Vulkan API: Error allocationis memoriae pro selectoribus.\n");
							return false;
						}
						uint index = 0;
						VkDescriptorImageInfo image[8];
						VkDescriptorBufferInfo buffer[8];
						VkWriteDescriptorSet write[8];
						for (auto & m : layout->smap) {
							if (!PushSelector(set, retain, index, image, buffer, write, m.key, m.value, stub_sampler)) return false;
							if (index == 8) WriteSelectors(api, index, write);
						}
						if (index) WriteSelectors(api, index, write);
						retain.AddElement(alloc.Inner());
						api->Dispatch.vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, layout->pipeline_layout, 0, 1, &set, 0, 0);
					}
					update_pipeline_layout = 0;
					return true;
				} catch (...) { return false; }
			}
		};
		class VKPass : public Object
		{
			friend class VKQueue;
			static bool _validate_subresource(VKTexture * texture, const Index2 & subres) noexcept
			{
				if (subres.x < 0 || subres.y < 0) return false;
				if (texture->_desc.Type == TextureType::TypeArray1D || texture->_desc.Type == TextureType::TypeArray2D) {
					return subres.x < texture->_desc.MipmapCount && subres.y < texture->_desc.ArraySize;
				} else if (texture->_desc.Type == TextureType::TypeCube || texture->_desc.Type == TextureType::TypeArrayCube) {
					return subres.x < texture->_desc.MipmapCount && subres.y < texture->_desc.ArraySize * 6;
				} else {
					return subres.x < texture->_desc.MipmapCount && subres.y == 0;
				}
			}
			static bool _validate_region(VKTexture * texture, const Index2 & subres, const Index3 & origin, Index3 & size) noexcept
			{
				if (texture->_desc.Type == TextureType::Type1D || texture->_desc.Type == TextureType::TypeArray1D) {
					Index3 dim(texture->_desc.Width, 1, 1);
					EvaluateMipMapSize(subres.x, dim);
					if (origin.x >= dim.x || origin.y >= dim.y || origin.z >= dim.z || !size.x) return false;
					size.y = size.z = 1;
					if (dim.x - origin.x < size.x) size.x = dim.x - origin.x;
					if (dim.y - origin.y < size.y) size.y = dim.y - origin.y;
					if (dim.z - origin.z < size.z) size.z = dim.z - origin.z;
					return true;
				} else if (texture->_desc.Type == TextureType::Type3D) {
					Index3 dim(texture->_desc.Width, texture->_desc.Height, texture->_desc.Depth);
					EvaluateMipMapSize(subres.x, dim);
					if (origin.x >= dim.x || origin.y >= dim.y || origin.z >= dim.z || !size.x || !size.y || !size.z) return false;
					if (dim.x - origin.x < size.x) size.x = dim.x - origin.x;
					if (dim.y - origin.y < size.y) size.y = dim.y - origin.y;
					if (dim.z - origin.z < size.z) size.z = dim.z - origin.z;
					return true;
				} else {
					Index3 dim(texture->_desc.Width, texture->_desc.Height, 1);
					EvaluateMipMapSize(subres.x, dim);
					if (origin.x >= dim.x || origin.y >= dim.y || origin.z >= dim.z || !size.x || !size.y) return false;
					size.z = 1;
					if (dim.x - origin.x < size.x) size.x = dim.x - origin.x;
					if (dim.y - origin.y < size.y) size.y = dim.y - origin.y;
					if (dim.z - origin.z < size.z) size.z = dim.z - origin.z;
					return true;
				}
			}
			static bool _validate_fragment(VKBuffer * buffer, uint origin, uint & size) noexcept
			{
				uint length = buffer->_desc.Length;
				if (origin >= length || !size) return false;
				if (length - origin < size) size = length - origin;
				return true;
			}
		public:
			Set<ResourceHandle> retain;
			oref<VKDeviceAPI> api;
			oref<VKDeviceStats> stats;
			oref<VKSelectorState> state;
			oref<VKConstantPool> & constant_pool;
			oref<VKSamplerState> stub_sampler;
			IDevice * parent_device;
			VkCommandPool pool;
			VkCommandBuffer buffer;
			uint queue;
			int mode, slot;
		public:
			VKPass(oref<VKConstantPool> & constant, VKDeviceStats * devstat) : constant_pool(constant), stats(devstat), parent_device(0), pool(0), buffer(0), mode(0), slot(-1) {}
			virtual ~VKPass(void) override { if (buffer) api->Dispatch.vkFreeCommandBuffers(api->Device, pool, 1, &buffer); }
			bool Initialize(VKDeviceAPI * device_api, IDevice * device, VkCommandPool command_pool, uint queue_index) noexcept
			{
				api.SetRetain(device_api);
				parent_device = device;
				pool = command_pool;
				queue = queue_index;
				VkCommandBufferAllocateInfo info;
				VkCommandBufferBeginInfo begin;
				info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				info.pNext = 0;
				info.commandPool = pool;
				info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				info.commandBufferCount = 1;
				begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin.pNext = 0;
				begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				begin.pInheritanceInfo = 0;
				if (api->Dispatch.vkAllocateCommandBuffers(api->Device, &info, &buffer) != VK_SUCCESS) return false;
				if (api->Dispatch.vkBeginCommandBuffer(buffer, &begin) != VK_SUCCESS) return false;
				return true;
			}
			bool AllocateConstantBuffer(uint length) noexcept
			{
				if (!constant_pool || constant_pool->used + length > constant_pool->allocated) {
					constant_pool = VKConstantPool::Allocate(parent_device);
					if (!constant_pool) return false;
				}
				constant_pool->offset = constant_pool->used;
				constant_pool->used += length;
				return true;
			}
			void UpdateSelectorConstant(uint domain, uint index, const void * data, int length) noexcept
			{
				if (!state) return;
				uint align = length;
				if (align & (stats->constant_alignment - 1)) { align &= ~(stats->constant_alignment - 1); align += stats->constant_alignment; }
				if (length <= _vk_constant_buffer_size && AllocateConstantBuffer(align)) {
					uint origin = constant_pool->offset;
					Memory::MemoryCopy(constant_pool->memory_mapping + origin, data, length);
					VkMappedMemoryRange range;
					range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
					range.pNext = 0;
					range.memory = constant_pool->_memory;
					range.offset = origin;
					range.size = align;
					api->Dispatch.vkFlushMappedMemoryRanges(api->Device, 1, &range);
					state->UpdateSelector(domain, index, constant_pool, origin, align);
				} else {
					BufferDesc desc;
					desc.Usage = ResourceUsageConstantBuffer;
					desc.Length = desc.Stride = length;
					desc.MemoryPool = ResourceMemoryPool::Immutable;
					ResourceInitDesc init;
					init.Data = data;
					auto buffer = parent_device->CreateBufferWithData(desc, init);
					if (buffer) state->UpdateSelector(domain, index, buffer);
					else if (_device_validation_layer) VKValidationOutput("Vulkan API: allocatio constatorum falsa.\n");
				}
			}
			void MakeDestinationLayoutTransition(VKTexture * texture, VkImageMemoryBarrier & barrier, VkImageLayout layout) noexcept
			{
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.pNext = 0;
				barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
				barrier.oldLayout = texture->_current_layout;
				barrier.newLayout = layout;
				barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = queue;
				barrier.image = texture->_image;
				if (IsColorFormat(texture->_desc.Format)) barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				else if (IsDepthStencilFormat(texture->_desc.Format)) {
					if (GetFormatChannelCount(texture->_desc.Format) == 1) barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					else barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				} else barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_NONE;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.layerCount = texture->GetArraySize();
				barrier.subresourceRange.levelCount = texture->GetMipmapCount();
				texture->_current_layout = layout;
			}
			void MakeSharedLayoutTransition(VKTexture * texture, VkImageMemoryBarrier & barrier, bool input) noexcept
			{
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.pNext = 0;
				barrier.srcAccessMask = barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				if (input) {
					auto global_layout = static_cast<VkImageLayout>(texture->_shared->_data->sharing.current_layout);
					if (global_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
						barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier.srcQueueFamilyIndex = queue;
						barrier.dstQueueFamilyIndex = queue;
						texture->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
					} else {
						if (texture->_current_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
							barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
							barrier.newLayout = global_layout;
						} else {
							barrier.oldLayout = global_layout;
							barrier.newLayout = global_layout;
						}
						barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
						barrier.dstQueueFamilyIndex = queue;
						texture->_current_layout = global_layout;
					}
				} else {
					barrier.oldLayout = texture->_current_layout;
					barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
					barrier.dstQueueFamilyIndex = queue;
					barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
					texture->_shared->_data->sharing.current_layout = texture->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
				}
				barrier.image = texture->_image;
				if (IsColorFormat(texture->_desc.Format)) {
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				} else if (IsDepthStencilFormat(texture->_desc.Format)) {
					if (GetFormatChannelCount(texture->_desc.Format) > 1) barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
					else barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				} else barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_NONE;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.layerCount = texture->GetArraySize();
				barrier.subresourceRange.levelCount = texture->GetMipmapCount();
			}
			bool BeginRenderingPass(uint32 rtc, const RenderTargetViewDesc * rtv, const DepthStencilViewDesc * dsv) noexcept
			{
				auto subpass_selectors = owrap(new (std::nothrow) VKSelectorState);
				if (!subpass_selectors) return false;
				VkMemoryBarrier barrier;
				VkImageMemoryBarrier image_barrier[9];
				uint image_barrier_count = 0;
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrier.pNext = 0;
				barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				for (uint i = 0; i < rtc; i++) if (static_cast<VKTexture *>(rtv[i].Texture)->_current_layout != VK_IMAGE_LAYOUT_GENERAL) {
					MakeDestinationLayoutTransition(static_cast<VKTexture *>(rtv[i].Texture), image_barrier[image_barrier_count], VK_IMAGE_LAYOUT_GENERAL);
					image_barrier_count++;
				}
				if (dsv && dsv->Texture && static_cast<VKTexture *>(dsv->Texture)->_current_layout != VK_IMAGE_LAYOUT_GENERAL) {
					MakeDestinationLayoutTransition(static_cast<VKTexture *>(dsv->Texture), image_barrier[image_barrier_count], VK_IMAGE_LAYOUT_GENERAL);
					image_barrier_count++;
				}
				api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, 0, image_barrier_count, image_barrier);
				VkRenderingAttachmentInfoKHR color[8];
				VkRenderingAttachmentInfoKHR depth;
				VkRenderingAttachmentInfoKHR stencil;
				VkRenderingInfoKHR rendering;
				rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
				rendering.pNext = 0;
				rendering.flags = 0;
				rendering.renderArea.offset.x = rendering.renderArea.offset.y = 0;
				rendering.renderArea.extent.width = rtv->Texture->GetWidth();
				rendering.renderArea.extent.height = rtv->Texture->GetHeight();
				rendering.layerCount = 1;
				rendering.viewMask = 0;
				rendering.colorAttachmentCount = rtc;
				rendering.pColorAttachments = color;
				for (uint i = 0; i < rtc; i++) {
					auto & c = color[i];
					auto t = static_cast<VKTexture *>(rtv[i].Texture);
					try { retain.AddElement(t); } catch (...) { return false; }
					if (!(t->_desc.Usage & ResourceUsageRenderTarget)) return false;
					c.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
					c.pNext = 0;
					c.imageView = t->_view;
					c.imageLayout = t->_current_layout;
					c.resolveMode = VK_RESOLVE_MODE_NONE_KHR;
					c.resolveImageView = 0;
					c.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					c.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					if (rtv[i].LoadAction == TextureLoadAction::Load) c.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
					else if (rtv[i].LoadAction == TextureLoadAction::Clear) {
						c.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
						Memory::MemoryCopy(&c.clearValue, &rtv[i].ClearValue, sizeof(c.clearValue));
					} else c.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				}
				depth.sType = stencil.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				depth.pNext = stencil.pNext = 0;
				if (dsv) {
					if (!dsv->Texture) return false;
					auto t = static_cast<VKTexture *>(dsv->Texture);
					try { retain.AddElement(t); } catch (...) { return false; }
					if (!(t->_desc.Usage & ResourceUsageDepthStencil)) return false;
					depth.resolveMode = stencil.resolveMode = VK_RESOLVE_MODE_NONE_KHR;
					depth.resolveImageView = stencil.resolveImageView = 0;
					depth.resolveImageLayout = stencil.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					depth.imageView = t->_view;
					depth.imageLayout = t->_current_layout;
					depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					if (dsv->DepthLoadAction == TextureLoadAction::Load) depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
					else if (dsv->DepthLoadAction == TextureLoadAction::Clear) {
						depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
						depth.clearValue.depthStencil.depth = dsv->DepthClearValue;
					} else depth.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					if (t->_view_stencil) {
						stencil.imageView = t->_view_stencil;
						stencil.imageLayout = t->_current_layout;
						stencil.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
						if (dsv->StencilLoadAction == TextureLoadAction::Load) stencil.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
						else if (dsv->StencilLoadAction == TextureLoadAction::Clear) {
							stencil.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
							stencil.clearValue.depthStencil.stencil = dsv->StencilClearValue;
						} else stencil.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					} else {
						stencil.imageView = 0;
						stencil.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						stencil.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
						stencil.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					}
					rendering.pDepthAttachment = &depth;
					rendering.pStencilAttachment = &stencil;
				} else {
					depth.imageView = stencil.imageView = 0;
					depth.imageLayout = stencil.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					depth.resolveMode = stencil.resolveMode = VK_RESOLVE_MODE_NONE_KHR;
					depth.resolveImageView = stencil.resolveImageView = 0;
					depth.resolveImageLayout = stencil.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					depth.storeOp = stencil.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
					depth.loadOp = stencil.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					rendering.pDepthAttachment = 0;
					rendering.pStencilAttachment = 0;
				}
				api->Dispatch.vkCmdBeginRenderingKHR(buffer, &rendering);
				mode = 1;
				state = subpass_selectors;
				return true;
			}
			void EndRenderingPass(void) noexcept { api->Dispatch.vkCmdEndRenderingKHR(buffer); state.Clear(); }
			void SetRenderingPipelineState(IPipelineState * pstate) noexcept { if (state) state->UpdatePipeline(pstate); }
			void SetViewport(float top_left_x, float top_left_y, float width, float height, float min_depth, float max_depth) noexcept
			{
				VkRect2D scissors;
				VkViewport viewport;
				viewport.x = top_left_x;
				viewport.y = top_left_y + height;
				viewport.width = width;
				viewport.height = -height;
				viewport.minDepth = min_depth;
				viewport.maxDepth = max_depth;
				scissors.offset.x = top_left_x;
				scissors.offset.y = top_left_y;
				scissors.extent.width = width;
				scissors.extent.height = height;
				api->Dispatch.vkCmdSetViewportWithCountEXT(buffer, 1, &viewport);
				api->Dispatch.vkCmdSetScissorWithCountEXT(buffer, 1, &scissors);
			}
			void SetVertexShaderResource(uint32 at, IDeviceResource * resource) noexcept
			{
				if (!state) return;
				if (resource) {
					#ifdef ESSE_DEBUG
					if (!(resource->GetResourceUsage() & ResourceUsageShaderRead) && _device_validation_layer) VKValidationOutput("Vulkan API: adnectio auxilii falsa - permissio legendi nulla.\n");
					#endif
					if (resource->GetResourceType() == ResourceType::Buffer) state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer, at, resource);
					else if (resource->GetResourceType() == ResourceType::Texture) state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_texture, at, resource);
				} else {
					state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer, at, 0);
					state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_texture, at, 0);
				}
			}
			void SetVertexShaderConstant(uint32 at, IBuffer * buffer) noexcept
			{
				if (!state) return;
				#ifdef ESSE_DEBUG
				if (!(buffer->GetResourceUsage() & ResourceUsageConstantBuffer) && _device_validation_layer) VKValidationOutput("Vulkan API: adnectio constati falsa.\n");
				#endif
				state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, at, buffer);
			}
			void SetVertexShaderConstant(uint32 at, const void * data, int length) noexcept { UpdateSelectorConstant(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, at, data, length); }
			void SetVertexShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept { if (!state) return; state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_sampler, at, sampler); }
			void SetPixelShaderResource(uint32 at, IDeviceResource * resource) noexcept
			{
				if (!state) return;
				if (resource) {
					#ifdef ESSE_DEBUG
					if (!(resource->GetResourceUsage() & ResourceUsageShaderRead) && _device_validation_layer) VKValidationOutput("Vulkan API: adnectio auxilii falsa - permissio legendi nulla.\n");
					#endif
					if (resource->GetResourceType() == ResourceType::Buffer) state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_buffer, at, resource);
					else if (resource->GetResourceType() == ResourceType::Texture) state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture, at, resource);
				} else {
					state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_buffer, at, 0);
					state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture, at, 0);
				}
			}
			void SetPixelShaderConstant(uint32 at, IBuffer * buffer) noexcept
			{
				if (!state) return;
				#ifdef ESSE_DEBUG
				if (!(buffer->GetResourceUsage() & ResourceUsageConstantBuffer) && _device_validation_layer) VKValidationOutput("Vulkan API: adnectio constati falsa.\n");
				#endif
				state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_constant, at, buffer);
			}
			void SetPixelShaderConstant(uint32 at, const void * data, int length) noexcept { UpdateSelectorConstant(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_constant, at, data, length); }
			void SetPixelShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept { if (!state) return; state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_sampler, at, sampler); }
			void SetIndexBuffer(IBuffer * index, IndexBufferFormat format) noexcept
			{
				#ifdef ESSE_DEBUG
				if (!(index->GetResourceUsage() & ResourceUsageIndexBuffer) && _device_validation_layer) VKValidationOutput("Vulkan API: adnectio ordinis indicum falsa.\n");
				#endif
				VkIndexType index_format;
				if (format == IndexBufferFormat::UInt32) index_format = VK_INDEX_TYPE_UINT32;
				else if (format == IndexBufferFormat::UInt16) index_format = VK_INDEX_TYPE_UINT16;
				#ifdef ESSE_DEBUG
				else { if (_device_validation_layer) VKValidationOutput("Vulkan API: efformatio indicis falsa.\n"); return; }
				#else
				else return;
				#endif
				auto index_buffer = static_cast<VKBuffer *>(index);
				if (!(index_buffer->_desc.Usage & ResourceUsageIndexBuffer)) return;
				try { retain.AddElement(index); } catch (...) { return; }
				api->Dispatch.vkCmdBindIndexBuffer(buffer, index_buffer->_buffer, 0, index_format);
			}
			void SetStencilReferenceValue(uint8 ref) noexcept { api->Dispatch.vkCmdSetStencilReference(buffer, VK_STENCIL_FACE_FRONT_AND_BACK, ref); }
			void DrawPrimitives(uint32 vertex_count, uint32 first_vertex) noexcept
			{
				if (!state || !state->PushState(buffer, retain, stub_sampler)) {
					if (_device_validation_layer) VKValidationOutput("Vulkan API: Error emissionis selectorum: cancello reddendum.\n");
					return;
				}
				api->Dispatch.vkCmdDraw(buffer, vertex_count, 1, first_vertex, 0);
			}
			void DrawInstancedPrimitives(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance) noexcept
			{
				if (!state || !state->PushState(buffer, retain, stub_sampler)) {
					if (_device_validation_layer) VKValidationOutput("Vulkan API: Error emissionis selectorum: cancello reddendum.\n");
					return;
				}
				api->Dispatch.vkCmdDraw(buffer, vertex_count, instance_count, first_vertex, first_instance);
			}
			void DrawIndexedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex) noexcept
			{
				if (!state || !state->PushState(buffer, retain, stub_sampler)) {
					if (_device_validation_layer) VKValidationOutput("Vulkan API: Error emissionis selectorum: cancello reddendum.\n");
					return;
				}
				api->Dispatch.vkCmdDrawIndexed(buffer, index_count, 1, first_index, base_vertex, 0);
			}
			void DrawIndexedInstancedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex, uint32 instance_count, uint32 first_instance) noexcept
			{
				if (!state || !state->PushState(buffer, retain, stub_sampler)) {
					if (_device_validation_layer) VKValidationOutput("Vulkan API: Error emissionis selectorum: cancello reddendum.\n");
					return;
				}
				api->Dispatch.vkCmdDrawIndexed(buffer, index_count, instance_count, first_index, base_vertex, first_instance);
			}
			void GenerateMipmaps(ITexture * texture) noexcept
			{
				try { retain.AddElement(texture); } catch (...) { return; }
				bool is_cube = texture->GetTextureType() == TextureType::TypeCube || texture->GetTextureType() == TextureType::TypeArrayCube;
				auto mip_count = texture->GetMipmapCount();
				auto array_count = is_cube ? 6 * texture->GetArraySize() : texture->GetArraySize();
				auto object = static_cast<VKTexture *>(texture);
				if (mip_count <= 1) return;
				Index3 size(texture->GetWidth(), texture->GetHeight(), texture->GetDepth());
				auto old_layout = object->_current_layout;
				for (uint i = 0; i < mip_count - 1; i++) {
					VkImageMemoryBarrier barrier[2];
					barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier[0].pNext = barrier[1].pNext = 0;
					barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
					barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
					barrier[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier[0].oldLayout = i ? VK_IMAGE_LAYOUT_GENERAL : old_layout;
					barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
					barrier[1].oldLayout = old_layout;
					barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
					barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = queue;
					barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = queue;
					barrier[0].image = barrier[1].image = object->_image;
					barrier[0].subresourceRange.aspectMask = barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					barrier[0].subresourceRange.baseArrayLayer = 0;
					barrier[0].subresourceRange.baseMipLevel = i;
					barrier[1].subresourceRange.baseArrayLayer = 0;
					barrier[1].subresourceRange.baseMipLevel = i + 1;
					barrier[0].subresourceRange.layerCount = array_count;
					barrier[0].subresourceRange.levelCount = 1;
					barrier[1].subresourceRange.layerCount = array_count;
					barrier[1].subresourceRange.levelCount = 1;
					api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
					auto size_src = size;
					auto size_dest = size;
					EvaluateMipMapSize(i, size_src);
					EvaluateMipMapSize(i + 1, size_dest);
					VkImageBlit blt;
					blt.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blt.srcSubresource.baseArrayLayer = 0;
					blt.srcSubresource.layerCount = array_count;
					blt.srcSubresource.mipLevel = i;
					blt.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					blt.dstSubresource.baseArrayLayer = 0;
					blt.dstSubresource.layerCount = array_count;
					blt.dstSubresource.mipLevel = i + 1;
					blt.srcOffsets[0].x = blt.srcOffsets[0].y = blt.srcOffsets[0].z = blt.dstOffsets[0].x = blt.dstOffsets[0].y = blt.dstOffsets[0].z = 0;
					blt.srcOffsets[1].x = size_src.x;
					blt.srcOffsets[1].y = size_src.y;
					blt.srcOffsets[1].z = size_src.z;
					blt.dstOffsets[1].x = size_dest.x;
					blt.dstOffsets[1].y = size_dest.y;
					blt.dstOffsets[1].z = size_dest.z;
					api->Dispatch.vkCmdBlitImage(buffer, object->_image, VK_IMAGE_LAYOUT_GENERAL, object->_image, VK_IMAGE_LAYOUT_GENERAL, 1, &blt, VK_FILTER_LINEAR);
				}
				object->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
			}
			void CopyResourceData(IDeviceResource * dest, IDeviceResource * src) noexcept
			{
				try { retain.AddElement(src); retain.AddElement(dest); } catch (...) { return; }
				auto dtype = dest->GetResourceType();
				auto stype = src->GetResourceType();
				if (dtype == ResourceType::Buffer) {
					auto destination = static_cast<VKBuffer *>(dest);
					if (!destination->_buffer) return;
					if (stype == ResourceType::Buffer) {
						auto source = static_cast<VKBuffer *>(src);
						if (!source->_buffer) return;
						auto & sdesc = source->_desc;
						auto & ddesc = destination->_desc;
						if (sdesc.Length != ddesc.Length) return;
						uint size = sdesc.Length;
						VkBufferMemoryBarrier barrier[2];
						barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
						barrier[0].pNext = barrier[1].pNext = 0;
						barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
						barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = queue;
						barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = queue;
						barrier[0].buffer = source->_buffer;
						barrier[1].buffer = destination->_buffer;
						barrier[0].offset = barrier[1].offset = 0;
						barrier[0].size = barrier[1].size = size;
						api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 2, barrier, 0, 0);
						VkBufferCopy copy;
						copy.srcOffset = copy.dstOffset = 0;
						copy.size = size;
						api->Dispatch.vkCmdCopyBuffer(buffer, source->_buffer, destination->_buffer, 1, &copy);
					} else return;
				} else if (dtype == ResourceType::Texture) {
					auto destination = static_cast<VKTexture *>(dest);
					if (!destination->_image) return;
					if (stype == ResourceType::Texture) {
						auto source = static_cast<VKTexture *>(src);
						if (!source->_image) return;
						auto & sdesc = source->_desc;
						auto & ddesc = destination->_desc;
						if (sdesc.Type != ddesc.Type || sdesc.Width != ddesc.Width || sdesc.MipmapCount != ddesc.MipmapCount) return;
						if (sdesc.Height != ddesc.Height || sdesc.Depth != ddesc.Depth) return;
						bool is_cube = source->GetTextureType() == TextureType::TypeCube || source->GetTextureType() == TextureType::TypeArrayCube;
						Index3 size(source->GetWidth(), source->GetHeight(), source->GetDepth());
						VkImageAspectFlags src_copy, dest_copy;
						if (IsDepthStencilFormat(source->_desc.Format)) {
							if (GetFormatChannelCount(source->_desc.Format) == 1) src_copy = VK_IMAGE_ASPECT_DEPTH_BIT;
							else src_copy = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
						} else if (IsColorFormat(source->_desc.Format)) src_copy = VK_IMAGE_ASPECT_COLOR_BIT;
						else src_copy = VK_IMAGE_ASPECT_NONE;
						if (IsDepthStencilFormat(destination->_desc.Format)) {
							if (GetFormatChannelCount(destination->_desc.Format) == 1) dest_copy = VK_IMAGE_ASPECT_DEPTH_BIT;
							else dest_copy = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
						} else if (IsColorFormat(destination->_desc.Format)) dest_copy = VK_IMAGE_ASPECT_COLOR_BIT;
						else dest_copy = VK_IMAGE_ASPECT_NONE;
						if (src_copy == VK_IMAGE_ASPECT_COLOR_BIT && dest_copy & VK_IMAGE_ASPECT_DEPTH_BIT) dest_copy = VK_IMAGE_ASPECT_DEPTH_BIT;
						if (dest_copy == VK_IMAGE_ASPECT_COLOR_BIT && src_copy & VK_IMAGE_ASPECT_DEPTH_BIT) src_copy = VK_IMAGE_ASPECT_DEPTH_BIT;
						VkImageMemoryBarrier barrier[2];
						barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						barrier[0].pNext = barrier[1].pNext = 0;
						barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
						barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].oldLayout = source->_current_layout;
						barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[1].oldLayout = destination->_current_layout;
						barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = queue;
						barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = queue;
						barrier[0].image = source->_image;
						barrier[1].image = destination->_image;
						barrier[0].subresourceRange.aspectMask = src_copy;
						barrier[1].subresourceRange.aspectMask = dest_copy;
						barrier[0].subresourceRange.baseArrayLayer = 0;
						barrier[0].subresourceRange.baseMipLevel = 0;
						barrier[1].subresourceRange.baseArrayLayer = 0;
						barrier[1].subresourceRange.baseMipLevel = 0;
						barrier[0].subresourceRange.layerCount = is_cube ? 6 * source->GetArraySize() : source->GetArraySize();
						barrier[0].subresourceRange.levelCount = source->GetMipmapCount();
						barrier[1].subresourceRange.layerCount = is_cube ? 6 * destination->GetArraySize() : destination->GetArraySize();
						barrier[1].subresourceRange.levelCount = destination->GetMipmapCount();
						api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
						source->_current_layout = destination->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
						array<VkImageCopy> copy(1);
						try { copy.SetLength(sdesc.MipmapCount); } catch (...) { return; }
						for (uint i = 0; i < sdesc.MipmapCount; i++) {
							auto msize = size;
							EvaluateMipMapSize(i, size);
							copy[i].srcSubresource.aspectMask = src_copy;
							copy[i].srcSubresource.baseArrayLayer = 0;
							copy[i].srcSubresource.layerCount = is_cube ? 6 * source->GetArraySize() : source->GetArraySize();
							copy[i].srcSubresource.mipLevel = i;
							copy[i].srcOffset.x = copy[i].srcOffset.y = copy[i].srcOffset.z = 0;
							copy[i].dstSubresource.aspectMask = dest_copy;
							copy[i].dstSubresource.baseArrayLayer = 0;
							copy[i].dstSubresource.layerCount = is_cube ? 6 * destination->GetArraySize() : destination->GetArraySize();
							copy[i].dstSubresource.mipLevel = i;
							copy[i].dstOffset.x = copy[i].dstOffset.y = copy[i].dstOffset.z = 0;
							copy[i].extent.width = size.x;
							copy[i].extent.height = size.y;
							copy[i].extent.depth = size.z;
						}
						api->Dispatch.vkCmdCopyImage(buffer, source->_image, source->_current_layout, destination->_image, destination->_current_layout, copy.GetLength(), copy);
					} else return;
				} else return;
			}
			void CopySubresourceData(IDeviceResource * dest, Index2 dest_subres, Index3 dest_origin, IDeviceResource * src, Index2 src_subres, Index3 src_origin, Index3 size) noexcept
			{
				try { retain.AddElement(src); retain.AddElement(dest); } catch (...) { return; }
				auto dtype = dest->GetResourceType();
				auto stype = src->GetResourceType();
				if (dtype == ResourceType::Buffer) {
					auto destination = static_cast<VKBuffer *>(dest);
					if (!destination->_buffer) return;
					if (stype == ResourceType::Buffer) {
						auto source = static_cast<VKBuffer *>(src);
						if (!source->_buffer) return;
						uint effective_size = size.x;
						if (!_validate_fragment(source, src_origin.x, effective_size)) return;
						if (!_validate_fragment(destination, dest_origin.x, effective_size)) return;
						VkBufferMemoryBarrier barrier[2];
						barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
						barrier[0].pNext = barrier[1].pNext = 0;
						barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
						barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = queue;
						barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = queue;
						barrier[0].buffer = source->_buffer;
						barrier[1].buffer = destination->_buffer;
						barrier[0].offset = src_origin.x;
						barrier[1].offset = dest_origin.x;
						barrier[0].size = barrier[1].size = effective_size;
						api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 2, barrier, 0, 0);
						VkBufferCopy copy;
						copy.srcOffset = src_origin.x;
						copy.dstOffset = dest_origin.x;
						copy.size = effective_size;
						api->Dispatch.vkCmdCopyBuffer(buffer, source->_buffer, destination->_buffer, 1, &copy);
					} else return;
				} else if (dtype == ResourceType::Texture) {
					auto destination = static_cast<VKTexture *>(dest);
					if (!destination->_image) return;
					if (stype == ResourceType::Texture) {
						auto source = static_cast<VKTexture *>(src);
						if (!source->_image) return;
						Index3 effective_size = size;
						if (!_validate_subresource(source, src_subres)) return;
						if (!_validate_subresource(destination, dest_subres)) return;
						if (!_validate_region(source, src_subres, src_origin, effective_size)) return;
						if (!_validate_region(destination, dest_subres, dest_origin, effective_size)) return;
						VkImageAspectFlags src_copy, dest_copy;
						if (IsDepthStencilFormat(source->_desc.Format)) {
							if (GetFormatChannelCount(source->_desc.Format) == 1) src_copy = VK_IMAGE_ASPECT_DEPTH_BIT;
							else src_copy = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
						} else if (IsColorFormat(source->_desc.Format)) src_copy = VK_IMAGE_ASPECT_COLOR_BIT;
						else src_copy = VK_IMAGE_ASPECT_NONE;
						if (IsDepthStencilFormat(destination->_desc.Format)) {
							if (GetFormatChannelCount(destination->_desc.Format) == 1) dest_copy = VK_IMAGE_ASPECT_DEPTH_BIT;
							else dest_copy = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
						} else if (IsColorFormat(destination->_desc.Format)) dest_copy = VK_IMAGE_ASPECT_COLOR_BIT;
						else dest_copy = VK_IMAGE_ASPECT_NONE;
						if (src_copy == VK_IMAGE_ASPECT_COLOR_BIT && dest_copy & VK_IMAGE_ASPECT_DEPTH_BIT) dest_copy = VK_IMAGE_ASPECT_DEPTH_BIT;
						if (dest_copy == VK_IMAGE_ASPECT_COLOR_BIT && src_copy & VK_IMAGE_ASPECT_DEPTH_BIT) src_copy = VK_IMAGE_ASPECT_DEPTH_BIT;
						VkImageMemoryBarrier barrier[2];
						barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						barrier[0].pNext = barrier[1].pNext = 0;
						barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
						barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].oldLayout = source->_current_layout;
						barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[1].oldLayout = destination->_current_layout;
						barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = queue;
						barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = queue;
						barrier[0].image = source->_image;
						barrier[1].image = destination->_image;
						barrier[0].subresourceRange.aspectMask = src_copy;
						barrier[1].subresourceRange.aspectMask = dest_copy;
						if (source->_current_layout != VK_IMAGE_LAYOUT_GENERAL || destination->_current_layout != VK_IMAGE_LAYOUT_GENERAL) {
							bool s_is_cube = source->GetTextureType() == TextureType::TypeCube || source->GetTextureType() == TextureType::TypeArrayCube;
							bool d_is_cube = destination->GetTextureType() == TextureType::TypeCube || destination->GetTextureType() == TextureType::TypeArrayCube;
							barrier[0].subresourceRange.baseArrayLayer = barrier[0].subresourceRange.baseMipLevel = 0;
							barrier[1].subresourceRange.baseArrayLayer = barrier[1].subresourceRange.baseMipLevel = 0;
							barrier[0].subresourceRange.layerCount = s_is_cube ? 6 * source->GetArraySize() : source->GetArraySize();
							barrier[0].subresourceRange.levelCount = source->GetMipmapCount();
							barrier[1].subresourceRange.layerCount = d_is_cube ? 6 * destination->GetArraySize() : destination->GetArraySize();
							barrier[1].subresourceRange.levelCount = destination->GetMipmapCount();
						} else {
							barrier[0].subresourceRange.baseArrayLayer = src_subres.y;
							barrier[0].subresourceRange.baseMipLevel = src_subres.x;
							barrier[1].subresourceRange.baseArrayLayer = dest_subres.y;
							barrier[1].subresourceRange.baseMipLevel = dest_subres.x;
							barrier[0].subresourceRange.layerCount = barrier[0].subresourceRange.levelCount = 1;
							barrier[1].subresourceRange.layerCount = barrier[1].subresourceRange.levelCount = 1;
						}
						api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
						source->_current_layout = destination->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
						VkImageCopy copy;
						copy.srcSubresource.aspectMask = src_copy;
						copy.srcSubresource.baseArrayLayer = src_subres.y;
						copy.srcSubresource.layerCount = 1;
						copy.srcSubresource.mipLevel = src_subres.x;
						copy.srcOffset.x = src_origin.x;
						copy.srcOffset.y = src_origin.y;
						copy.srcOffset.z = src_origin.z;
						copy.dstSubresource.aspectMask = dest_copy;
						copy.dstSubresource.baseArrayLayer = dest_subres.y;
						copy.dstSubresource.layerCount = 1;
						copy.dstSubresource.mipLevel = dest_subres.x;
						copy.dstOffset.x = dest_origin.x;
						copy.dstOffset.y = dest_origin.y;
						copy.dstOffset.z = dest_origin.z;
						copy.extent.width = effective_size.x;
						copy.extent.height = effective_size.y;
						copy.extent.depth = effective_size.z;
						api->Dispatch.vkCmdCopyImage(buffer, source->_image, source->_current_layout, destination->_image, destination->_current_layout, 1, &copy);
					} else return;
				} else return;
			}
		};
		class VKQueue : public Object
		{
			friend class VKDevice;
			friend class VKDeviceDeferredContext;
			friend class VKDeviceImmediateContext;
			friend class VKDeviceContext2D;
		private:
			IDevice * _parent_device;
			VkQueue _queue;
			VkCommandPool _pool;
			oref<VKDeviceAPI> _api;
			oref<VKDeviceStats> _stats;
			oref<VKSamplerState> _stub_sampler;
			oref<VKConstantPool> _constant_pool;
			volatile bool * _valid;
			uint _queue_index;
			uintptr _circular;
			array<VkFence> _completion;
			object_array<VKPass> _submitted;
			ObjectDictionary<Object *, Object> _swapchains_retained;
		private:
			oref<VKSamplerState> _internal_create_sampler_stub(void) noexcept
			{
				auto state = owrap(new (std::nothrow) VKSamplerState(_api, _parent_device));
				if (!state) return 0;
				VkSamplerCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				info.pNext = 0;
				info.flags = 0;
				info.magFilter = info.minFilter = VK_FILTER_NEAREST;
				info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				info.addressModeU = info.addressModeV = info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				info.mipLodBias = 0.0f;
				info.anisotropyEnable = VK_FALSE;
				info.maxAnisotropy = 0.0f;
				info.compareEnable = VK_FALSE;
				info.compareOp = VK_COMPARE_OP_NEVER;
				info.minLod = 0.0f;
				info.maxLod = VK_LOD_CLAMP_NONE;
				info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
				info.unnormalizedCoordinates = VK_FALSE;
				if (_api->Dispatch.vkCreateSampler(_api->Device, &info, &_api->Base->Allocator, &state->_sampler) != VK_SUCCESS) return 0;
				return state;
			}
		public:
			VKQueue(IDevice * parent, VKDeviceStats * stats) : _parent_device(parent), _stats(stats), _queue(0), _pool(0), _valid(0), _circular(0), _completion(_vk_submission_slots), _submitted(_vk_submission_slots) {}
			virtual ~VKQueue(void) override { for (auto & f : _completion) if (f) _api->Dispatch.vkDestroyFence(_api->Device, f, &_api->Base->Allocator); }
			bool Initialize(VKDeviceAPI * api, VkQueue queue, VkCommandPool pool, uint queue_index, volatile bool * valid) noexcept
			{
				_api = api; _valid = valid; _queue_index = queue_index;
				_queue = queue; _pool = pool;
				try { for (int i = 0; i < _vk_submission_slots; i++) { _submitted.Append(0); _completion.Append(0); } } catch (...) { return false; }
				for (uintptr i = 0; i < _completion.GetLength(); i++) {
					VkFence fence;
					VkFenceCreateInfo fence_info;
					fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
					fence_info.pNext = 0;
					fence_info.flags = 0;
					if (api->Dispatch.vkCreateFence(api->Device, &fence_info, &api->Base->Allocator, &fence) != VK_SUCCESS) return false;
					_completion[i] = fence;
				}
				_stub_sampler = _internal_create_sampler_stub();
				if (!_stub_sampler) return false;
				return true;
			}
			void Finalize(void) noexcept
			{
				_api->Dispatch.vkQueueWaitIdle(_queue);
				_constant_pool.Clear(); _submitted.Clear();
				_swapchains_retained.Clear();
			}
			oref<VKPass> CreatePass(void) noexcept
			{
				auto pass = owrap(new (std::nothrow) VKPass(_constant_pool, _stats));
				if (!pass || !pass->Initialize(_api, _parent_device, _pool, _queue_index)) return 0;
				pass->stub_sampler = _stub_sampler;
				return pass;
			}
			bool SubmitPass(VKPass * submit, VkSemaphore sem_open = 0) noexcept
			{
				for (uintptr i = 0; i < _submitted.GetLength(); i++) {
					auto pass = _submitted(i);
					if (pass) {
						auto status = _api->Dispatch.vkGetFenceStatus(_api->Device, _completion[i]);
						if (status == VK_SUCCESS) _submitted.SetElement(0, i); else if (status < 0) *_valid = false;
					}
				}
				if (!*_valid) return false;
				if (_submitted(_circular)) {
					while (true) {
						auto status = _api->Dispatch.vkWaitForFences(_api->Device, 1, &_completion[_circular], VK_TRUE, 1000000000UL);
						if (status == VK_SUCCESS) { _submitted.SetElement(0, _circular); break; }
						else if (status < 0) { *_valid = false; break; }
					}
					if (!*_valid) return false;
				}
				if (_api->Dispatch.vkResetFences(_api->Device, 1, &_completion[_circular]) != VK_SUCCESS) return false;
				submit->slot = _circular;
				if (_api->Dispatch.vkEndCommandBuffer(submit->buffer) != VK_SUCCESS) return false;
				VkSubmitInfo submit_info;
				VkSemaphore sem_open_internal;
				submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info.pNext = 0;
				submit_info.waitSemaphoreCount = 0;
				submit_info.pWaitSemaphores = 0;
				submit_info.pWaitDstStageMask = 0;
				submit_info.commandBufferCount = 1;
				submit_info.pCommandBuffers = &submit->buffer;
				if (sem_open) {
					sem_open_internal = sem_open;
					submit_info.signalSemaphoreCount = 1;
					submit_info.pSignalSemaphores = &sem_open_internal;
				} else {
					submit_info.signalSemaphoreCount = 0;
					submit_info.pSignalSemaphores = 0;
				}
				if (_api->Dispatch.vkQueueSubmit(_queue, 1, &submit_info, _completion[_circular]) != VK_SUCCESS) return false;
				_submitted.SetElement(submit, _circular);
				_circular = (_circular + 1) % _vk_submission_slots;
				return true;
			}
			void WaitForCompletion(void) noexcept
			{
				for (uintptr i = 0; i < _submitted.GetLength(); i++) {
					auto pass = _submitted(i);
					if (pass) {
						while (true) {
							auto status = _api->Dispatch.vkWaitForFences(_api->Device, 1, &_completion[_circular], VK_TRUE, 1000000000UL);
							if (status == VK_SUCCESS) break;
							else if (status < 0) { *_valid = false; break; }
						}
						_submitted.SetElement(0, _circular);
					}
				}
			}
			VkCommandBuffer BeginPrivatePass(void) noexcept
			{
				VkCommandBuffer buffer;
				VkCommandBufferAllocateInfo info;
				VkCommandBufferBeginInfo begin;
				info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				info.pNext = 0;
				info.commandPool = _pool;
				info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				info.commandBufferCount = 1;
				begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin.pNext = 0;
				begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				begin.pInheritanceInfo = 0;
				if (_api->Dispatch.vkAllocateCommandBuffers(_api->Device, &info, &buffer) != VK_SUCCESS) return 0;
				if (_api->Dispatch.vkBeginCommandBuffer(buffer, &begin) != VK_SUCCESS) {
					_api->Dispatch.vkFreeCommandBuffers(_api->Device, _pool, 1, &buffer);
					return 0;
				}
				return buffer;
			}
			bool EndPrivatePass(VkCommandBuffer buffer) noexcept
			{
				if (_api->Dispatch.vkEndCommandBuffer(buffer) != VK_SUCCESS) {
					_api->Dispatch.vkFreeCommandBuffers(_api->Device, _pool, 1, &buffer);
					return false;
				}
				VkFence fence;
				VkFenceCreateInfo fence_info;
				fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fence_info.pNext = 0;
				fence_info.flags = 0;
				if (_api->Dispatch.vkCreateFence(_api->Device, &fence_info, &_api->Base->Allocator, &fence) != VK_SUCCESS) {
					_api->Dispatch.vkFreeCommandBuffers(_api->Device, _pool, 1, &buffer);
					return false;
				}
				VkSubmitInfo submit;
				submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit.pNext = 0;
				submit.waitSemaphoreCount = 0;
				submit.pWaitSemaphores = 0;
				submit.pWaitDstStageMask = 0;
				submit.commandBufferCount = 1;
				submit.pCommandBuffers = &buffer;
				submit.signalSemaphoreCount = 0;
				submit.pSignalSemaphores = 0;
				if (_api->Dispatch.vkQueueSubmit(_queue, 1, &submit, fence) != VK_SUCCESS) {
					_api->Dispatch.vkFreeCommandBuffers(_api->Device, _pool, 1, &buffer);
					_api->Dispatch.vkDestroyFence(_api->Device, fence, &_api->Base->Allocator);
					return false;
				}
				while (true) {
					auto status = _api->Dispatch.vkWaitForFences(_api->Device, 1, &fence, VK_TRUE, 1000000000UL);
					if (status == VK_SUCCESS) break;
					if (status == VK_ERROR_DEVICE_LOST) *_valid = false;
					if (status != VK_TIMEOUT) {
						_api->Dispatch.vkFreeCommandBuffers(_api->Device, _pool, 1, &buffer);
						_api->Dispatch.vkDestroyFence(_api->Device, fence, &_api->Base->Allocator);
						return false;
					}
				}
				_api->Dispatch.vkFreeCommandBuffers(_api->Device, _pool, 1, &buffer);
				_api->Dispatch.vkDestroyFence(_api->Device, fence, &_api->Base->Allocator);
				return true;
			}
			bool InternalUpdateResourceData(VkCommandBuffer & buffer, IDeviceResource * dest, Index2 subres, Index3 origin, Index3 size, const ResourceInitDesc & src) noexcept
			{
				if (!dest) return true;
				if (dest->GetResourceType() == ResourceType::Buffer) {
					auto object = static_cast<VKBuffer *>(dest);
					if (!object->_transit || !object->_buffer) return true;
					uint effective_size = size.x;
					if (!VKPass::_validate_fragment(object, origin.x, effective_size)) return true;
					void * memory;
					if (_api->Dispatch.vkMapMemory(_api->Device, object->_transit->_memory, 0, effective_size, 0, &memory) != VK_SUCCESS) {
						_api->Dispatch.vkFreeCommandBuffers(_api->Device, _pool, 1, &buffer);
						return false;
					}
					Memory::MemoryCopy(memory, src.Data, effective_size);
					VkMappedMemoryRange range;
					range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
					range.pNext = 0;
					range.memory = object->_transit->_memory;
					range.offset = 0;
					range.size = VK_WHOLE_SIZE;
					_api->Dispatch.vkFlushMappedMemoryRanges(_api->Device, 1, &range);
					_api->Dispatch.vkUnmapMemory(_api->Device, object->_transit->_memory);
					VkBufferMemoryBarrier barrier[2];
					barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
					barrier[0].pNext = barrier[1].pNext = 0;
					barrier[0].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
					barrier[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue_index;
					barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue_index;
					barrier[0].buffer = object->_transit->_buffer;
					barrier[1].buffer = object->_buffer;
					barrier[0].offset = 0;
					barrier[1].offset = origin.x;
					barrier[0].size = barrier[1].size = effective_size;
					_api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 2, barrier, 0, 0);
					VkBufferCopy copy;
					copy.srcOffset = 0;
					copy.dstOffset = origin.x;
					copy.size = effective_size;
					_api->Dispatch.vkCmdCopyBuffer(buffer, object->_transit->_buffer, object->_buffer, 1, &copy);
					if (!EndPrivatePass(buffer)) return false;
					buffer = BeginPrivatePass();
					if (!buffer) return false;
					return true;
				} else if (dest->GetResourceType() == ResourceType::Texture) {
					auto texture = static_cast<VKTexture *>(dest);
					if (!texture->_transit || !texture->_image) return true;
					Index3 effective_size = size;
					if (!VKPass::_validate_subresource(texture, subres)) return true;
					if (!VKPass::_validate_region(texture, subres, origin, effective_size)) return true;
					if (texture->GetTextureType() == TextureType::Type3D) {
						VkImageSubresource subresource;
						VkSubresourceLayout layout;
						subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						subresource.mipLevel = subresource.arrayLayer = 0;
						_api->Dispatch.vkGetImageSubresourceLayout(_api->Device, texture->_transit->_image, &subresource, &layout);
						for (uint z = 0; z < effective_size.z; z++) {
							void * memory;
							if (_api->Dispatch.vkMapMemory(_api->Device, texture->_transit->_memory, layout.offset, layout.size, 0, &memory) != VK_SUCCESS) {
								_api->Dispatch.vkFreeCommandBuffers(_api->Device, _pool, 1, &buffer);
								return false;
							}
							auto scanline = uint64(GetFormatBitsPerPixel(texture->_desc.Format) / 8 * effective_size.x);
							for (uint row = 0; row < effective_size.y; row++) {
								auto base_dest = reinterpret_cast<void *>(uintptr(memory) + layout.rowPitch * row);
								auto base_src = reinterpret_cast<void *>(uintptr(src.Data) + src.DataPitch * row + src.DataSlicePitch * z);
								Memory::MemoryCopy(base_dest, base_src, scanline);
							}
							VkMappedMemoryRange range;
							range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
							range.pNext = 0;
							range.memory = texture->_transit->_memory;
							range.offset = layout.offset;
							range.size = layout.size;
							_api->Dispatch.vkFlushMappedMemoryRanges(_api->Device, 1, &range);
							_api->Dispatch.vkUnmapMemory(_api->Device, texture->_transit->_memory);
							VkImageMemoryBarrier barrier[2];
							barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
							barrier[0].pNext = barrier[1].pNext = 0;
							barrier[0].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
							barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
							barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
							barrier[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
							barrier[0].oldLayout = texture->_transit->_current_layout;
							barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
							barrier[1].oldLayout = texture->_current_layout;
							barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
							barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue_index;
							barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue_index;
							barrier[0].image = texture->_transit->_image;
							barrier[1].image = texture->_image;
							barrier[0].subresourceRange.aspectMask = barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
							barrier[0].subresourceRange.baseArrayLayer = barrier[0].subresourceRange.baseMipLevel = 0;
							barrier[1].subresourceRange.baseArrayLayer = 0;
							barrier[1].subresourceRange.baseMipLevel = 0;
							barrier[0].subresourceRange.layerCount = barrier[0].subresourceRange.levelCount = 1;
							barrier[1].subresourceRange.layerCount = 1;
							barrier[1].subresourceRange.levelCount = texture->_desc.MipmapCount;
							_api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
							texture->_transit->_current_layout = texture->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
							VkImageCopy copy;
							copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
							copy.srcSubresource.baseArrayLayer = 0;
							copy.srcSubresource.layerCount = 1;
							copy.srcSubresource.mipLevel = 0;
							copy.srcOffset.x = copy.srcOffset.y = copy.srcOffset.z = 0;
							copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
							copy.dstSubresource.baseArrayLayer = 0;
							copy.dstSubresource.layerCount = 1;
							copy.dstSubresource.mipLevel = subres.x;
							copy.dstOffset.x = origin.x;
							copy.dstOffset.y = origin.y;
							copy.dstOffset.z = origin.z + z;
							copy.extent.width = effective_size.x;
							copy.extent.height = effective_size.y;
							copy.extent.depth = 1;
							_api->Dispatch.vkCmdCopyImage(buffer,
								texture->_transit->_image, texture->_transit->_current_layout,
								texture->_image, texture->_current_layout, 1, &copy);
							if (!EndPrivatePass(buffer)) return false;
							buffer = BeginPrivatePass();
							if (!buffer) return false;
						}
						return true;
					} else {
						bool is_cube = texture->GetTextureType() == TextureType::TypeCube || texture->GetTextureType() == TextureType::TypeArrayCube;
						VkImageSubresource subresource;
						VkSubresourceLayout layout;
						subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						subresource.mipLevel = subresource.arrayLayer = 0;
						_api->Dispatch.vkGetImageSubresourceLayout(_api->Device, texture->_transit->_image, &subresource, &layout);
						void * memory;
						if (_api->Dispatch.vkMapMemory(_api->Device, texture->_transit->_memory, layout.offset, layout.size, 0, &memory) != VK_SUCCESS) {
							_api->Dispatch.vkFreeCommandBuffers(_api->Device, _pool, 1, &buffer);
							return false;
						}
						auto scanline = uint64(GetFormatBitsPerPixel(texture->_desc.Format) / 8 * effective_size.x);
						for (uint row = 0; row < effective_size.y; row++) {
							auto base_dest = reinterpret_cast<void *>(uintptr(memory) + layout.rowPitch * row);
							auto base_src = reinterpret_cast<void *>(uintptr(src.Data) + src.DataPitch * row);
							Memory::MemoryCopy(base_dest, base_src, scanline);
						}
						VkMappedMemoryRange range;
						range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
						range.pNext = 0;
						range.memory = texture->_transit->_memory;
						range.offset = layout.offset;
						range.size = layout.size;
						_api->Dispatch.vkFlushMappedMemoryRanges(_api->Device, 1, &range);
						_api->Dispatch.vkUnmapMemory(_api->Device, texture->_transit->_memory);
						VkImageMemoryBarrier barrier[2];
						barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						barrier[0].pNext = barrier[1].pNext = 0;
						barrier[0].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
						barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						barrier[0].oldLayout = texture->_transit->_current_layout;
						barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[1].oldLayout = texture->_current_layout;
						barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue_index;
						barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue_index;
						barrier[0].image = texture->_transit->_image;
						barrier[1].image = texture->_image;
						barrier[0].subresourceRange.aspectMask = barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						barrier[0].subresourceRange.baseArrayLayer = barrier[0].subresourceRange.baseMipLevel = 0;
						barrier[1].subresourceRange.baseArrayLayer = 0;
						barrier[1].subresourceRange.baseMipLevel = 0;
						barrier[0].subresourceRange.layerCount = barrier[0].subresourceRange.levelCount = 1;
						barrier[1].subresourceRange.layerCount = is_cube ? 6 * texture->_desc.ArraySize : texture->_desc.ArraySize;
						barrier[1].subresourceRange.levelCount = texture->_desc.MipmapCount;
						_api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
						texture->_transit->_current_layout = texture->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
						VkImageCopy copy;
						copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copy.srcSubresource.baseArrayLayer = 0;
						copy.srcSubresource.layerCount = 1;
						copy.srcSubresource.mipLevel = 0;
						copy.srcOffset.x = copy.srcOffset.y = copy.srcOffset.z = 0;
						copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copy.dstSubresource.baseArrayLayer = subres.y;
						copy.dstSubresource.layerCount = 1;
						copy.dstSubresource.mipLevel = subres.x;
						copy.dstOffset.x = origin.x;
						copy.dstOffset.y = origin.y;
						copy.dstOffset.z = origin.z;
						copy.extent.width = effective_size.x;
						copy.extent.height = effective_size.y;
						copy.extent.depth = effective_size.z;
						_api->Dispatch.vkCmdCopyImage(buffer,
							texture->_transit->_image, texture->_transit->_current_layout,
							texture->_image, texture->_current_layout, 1, &copy);
						if (!EndPrivatePass(buffer)) return false;
						buffer = BeginPrivatePass();
						if (!buffer) return false;
						return true;
					}
				} else return true;
			}
			bool InternalQueryResourceData(VkCommandBuffer & buffer, const ResourceDataDesc & dest, IDeviceResource * src, Index2 subres, Index3 origin, Index3 size) noexcept
			{
				if (!src) return true;
				if (src->GetResourceType() == ResourceType::Buffer) {
					auto object = static_cast<VKBuffer *>(src);
					if (!object->_transit || !object->_buffer) return true;
					uint effective_size = size.x;
					if (!VKPass::_validate_fragment(object, origin.x, effective_size)) return true;
					VkBufferMemoryBarrier barrier[2];
					barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
					barrier[0].pNext = barrier[1].pNext = 0;
					barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
					barrier[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue_index;
					barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue_index;
					barrier[0].buffer = object->_buffer;
					barrier[1].buffer = object->_transit->_buffer;
					barrier[0].offset = origin.x;
					barrier[1].offset = 0;
					barrier[0].size = barrier[1].size = effective_size;
					_api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 2, barrier, 0, 0);
					VkBufferCopy copy;
					copy.srcOffset = origin.x;
					copy.dstOffset = 0;
					copy.size = effective_size;
					_api->Dispatch.vkCmdCopyBuffer(buffer, object->_buffer, object->_transit->_buffer, 1, &copy);
					barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier[0].dstAccessMask = VK_ACCESS_HOST_READ_BIT;
					barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue_index;
					barrier[0].buffer = object->_transit->_buffer;
					barrier[0].offset = 0;
					barrier[0].size = effective_size;
					_api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, 0, 1, barrier, 0, 0);
					auto submission_state = EndPrivatePass(buffer);
					buffer = 0;
					if (!submission_state) return false;
					VkMappedMemoryRange range;
					range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
					range.pNext = 0;
					range.memory = object->_transit->_memory;
					range.offset = 0;
					range.size = VK_WHOLE_SIZE;
					void * memory;
					if (_api->Dispatch.vkMapMemory(_api->Device, object->_transit->_memory, 0, effective_size, 0, &memory) != VK_SUCCESS) return false;
					_api->Dispatch.vkInvalidateMappedMemoryRanges(_api->Device, 1, &range);
					Memory::MemoryCopy(dest.Data, memory, effective_size);
					_api->Dispatch.vkUnmapMemory(_api->Device, object->_transit->_memory);
					buffer = BeginPrivatePass();
					if (!buffer) return false;
					return true;
				} else if (src->GetResourceType() == ResourceType::Texture) {
					auto texture = static_cast<VKTexture *>(src);
					if (!texture->_transit || !texture->_image) return true;
					Index3 effective_size = size;
					if (!VKPass::_validate_subresource(texture, subres)) return true;
					if (!VKPass::_validate_region(texture, subres, origin, effective_size)) return true;
					if (texture->GetTextureType() == TextureType::Type3D) {
						VkImageSubresource subresource;
						VkSubresourceLayout layout;
						subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						subresource.mipLevel = subresource.arrayLayer = 0;
						_api->Dispatch.vkGetImageSubresourceLayout(_api->Device, texture->_transit->_image, &subresource, &layout);
						for (uint z = 0; z < effective_size.z; z++) {
							VkImageMemoryBarrier barrier[2];
							barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
							barrier[0].pNext = barrier[1].pNext = 0;
							barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
							barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
							barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
							barrier[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
							barrier[0].oldLayout = texture->_current_layout;
							barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
							barrier[1].oldLayout = texture->_transit->_current_layout;
							barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
							barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue_index;
							barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue_index;
							barrier[0].image = texture->_image;
							barrier[1].image = texture->_transit->_image;
							barrier[0].subresourceRange.aspectMask = barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
							barrier[0].subresourceRange.baseArrayLayer = 0;
							barrier[0].subresourceRange.baseMipLevel = 0;
							barrier[1].subresourceRange.baseArrayLayer = barrier[1].subresourceRange.baseMipLevel = 0;
							barrier[0].subresourceRange.layerCount = 1;
							barrier[0].subresourceRange.levelCount = texture->_desc.MipmapCount;
							barrier[1].subresourceRange.layerCount = barrier[1].subresourceRange.levelCount = 1;
							_api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
							texture->_transit->_current_layout = texture->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
							VkImageCopy copy;
							copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
							copy.srcSubresource.baseArrayLayer = 0;
							copy.srcSubresource.layerCount = 1;
							copy.srcSubresource.mipLevel = subres.x;
							copy.srcOffset.x = origin.x;
							copy.srcOffset.y = origin.y;
							copy.srcOffset.z = origin.z + z;
							copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
							copy.dstSubresource.baseArrayLayer = 0;
							copy.dstSubresource.layerCount = 1;
							copy.dstSubresource.mipLevel = 0;
							copy.dstOffset.x = copy.dstOffset.y = copy.dstOffset.z = 0;
							copy.extent.width = effective_size.x;
							copy.extent.height = effective_size.y;
							copy.extent.depth = 1;
							_api->Dispatch.vkCmdCopyImage(buffer, texture->_image, texture->_current_layout,
								texture->_transit->_image, texture->_transit->_current_layout, 1, &copy);
							barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
							barrier[0].dstAccessMask = VK_ACCESS_HOST_READ_BIT;
							barrier[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
							barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
							barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue_index;
							barrier[0].image = texture->_transit->_image;
							barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
							barrier[0].subresourceRange.baseArrayLayer = barrier[0].subresourceRange.baseMipLevel = 0;
							barrier[0].subresourceRange.layerCount = barrier[0].subresourceRange.levelCount = 1;
							_api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, 0, 0, 0, 1, barrier);
							auto submission_state = EndPrivatePass(buffer);
							buffer = 0;
							if (!submission_state) return false;
							VkMappedMemoryRange range;
							range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
							range.pNext = 0;
							range.memory = texture->_transit->_memory;
							range.offset = layout.offset;
							range.size = layout.size;
							void * memory;
							if (_api->Dispatch.vkMapMemory(_api->Device, texture->_transit->_memory, layout.offset, layout.size, 0, &memory) != VK_SUCCESS) return false;
							_api->Dispatch.vkInvalidateMappedMemoryRanges(_api->Device, 1, &range);
							auto scanline = uint64(GetFormatBitsPerPixel(texture->_desc.Format) / 8 * effective_size.x);
							for (uint row = 0; row < effective_size.y; row++) {
								auto base_dest = reinterpret_cast<void *>(uintptr(dest.Data) + dest.DataPitch * row + dest.DataSlicePitch * z);
								auto base_src = reinterpret_cast<void *>(uintptr(memory) + layout.rowPitch * row);
								Memory::MemoryCopy(base_dest, base_src, scanline);
							}
							_api->Dispatch.vkUnmapMemory(_api->Device, texture->_transit->_memory);
							buffer = BeginPrivatePass();
							if (!buffer) return false;
						}
						return true;
					} else {
						bool is_cube = texture->GetTextureType() == TextureType::TypeCube || texture->GetTextureType() == TextureType::TypeArrayCube;
						VkImageMemoryBarrier barrier[2];
						barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						barrier[0].pNext = barrier[1].pNext = 0;
						barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						barrier[0].oldLayout = texture->_current_layout;
						barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[1].oldLayout = texture->_transit->_current_layout;
						barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue_index;
						barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue_index;
						barrier[0].image = texture->_image;
						barrier[1].image = texture->_transit->_image;
						barrier[0].subresourceRange.aspectMask = barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						barrier[0].subresourceRange.baseArrayLayer = 0;
						barrier[0].subresourceRange.baseMipLevel = 0;
						barrier[1].subresourceRange.baseArrayLayer = barrier[1].subresourceRange.baseMipLevel = 0;
						barrier[0].subresourceRange.layerCount = is_cube ? 6 * texture->_desc.ArraySize : texture->_desc.ArraySize;
						barrier[0].subresourceRange.levelCount = texture->_desc.MipmapCount;
						barrier[1].subresourceRange.layerCount = barrier[1].subresourceRange.levelCount = 1;
						_api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
						texture->_transit->_current_layout = texture->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
						VkImageCopy copy;
						copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copy.srcSubresource.baseArrayLayer = subres.y;
						copy.srcSubresource.layerCount = 1;
						copy.srcSubresource.mipLevel = subres.x;
						copy.srcOffset.x = origin.x;
						copy.srcOffset.y = origin.y;
						copy.srcOffset.z = origin.z;
						copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copy.dstSubresource.baseArrayLayer = 0;
						copy.dstSubresource.layerCount = 1;
						copy.dstSubresource.mipLevel = 0;
						copy.dstOffset.x = copy.dstOffset.y = copy.dstOffset.z = 0;
						copy.extent.width = effective_size.x;
						copy.extent.height = effective_size.y;
						copy.extent.depth = effective_size.z;
						_api->Dispatch.vkCmdCopyImage(buffer, texture->_image, texture->_current_layout,
							texture->_transit->_image, texture->_transit->_current_layout, 1, &copy);
						barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						barrier[0].dstAccessMask = VK_ACCESS_HOST_READ_BIT;
						barrier[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue_index;
						barrier[0].image = texture->_transit->_image;
						barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						barrier[0].subresourceRange.baseArrayLayer = barrier[0].subresourceRange.baseMipLevel = 0;
						barrier[0].subresourceRange.layerCount = barrier[0].subresourceRange.levelCount = 1;
						_api->Dispatch.vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, 0, 0, 0, 1, barrier);
						VkImageSubresource subresource;
						VkSubresourceLayout layout;
						subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						subresource.mipLevel = subresource.arrayLayer = 0;
						_api->Dispatch.vkGetImageSubresourceLayout(_api->Device, texture->_transit->_image, &subresource, &layout);
						auto submission_state = EndPrivatePass(buffer);
						buffer = 0;
						if (!submission_state) return false;
						VkMappedMemoryRange range;
						range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
						range.pNext = 0;
						range.memory = texture->_transit->_memory;
						range.offset = layout.offset;
						range.size = layout.size;
						void * memory;
						if (_api->Dispatch.vkMapMemory(_api->Device, texture->_transit->_memory, layout.offset, layout.size, 0, &memory) != VK_SUCCESS) return false;
						_api->Dispatch.vkInvalidateMappedMemoryRanges(_api->Device, 1, &range);
						auto scanline = uint64(GetFormatBitsPerPixel(texture->_desc.Format) / 8 * size.x);
						for (uint row = 0; row < effective_size.y; row++) {
							auto base_dest = reinterpret_cast<void *>(uintptr(dest.Data) + dest.DataPitch * row);
							auto base_src = reinterpret_cast<void *>(uintptr(memory) + layout.rowPitch * row);
							Memory::MemoryCopy(base_dest, base_src, scanline);
						}
						_api->Dispatch.vkUnmapMemory(_api->Device, texture->_transit->_memory);
						buffer = BeginPrivatePass();
						if (!buffer) return false;
						return true;
					}
				} else return true;
			}
			bool CommitSwapChain(Object * host_layer, Object * swapchain) noexcept { try { _swapchains_retained.Update(host_layer, swapchain); return true; } catch (...) { return false; } }
			void RemoveSwapChain(Object * host_layer) noexcept { _api->Dispatch.vkQueueWaitIdle(_queue); _swapchains_retained.Remove(host_layer); }
			VKDeviceAPI * GetAPI(void) noexcept { return _api; }
			uint GetQueueIndex(void) noexcept { return _queue_index; }
			VkQueue GetQueue(void) noexcept { return _queue; }
		};
		class VKLayerBacking : public ILayerBacking
		{
			friend class VKDeviceContext2D;
		private:
			IDevice * _parent_device;
			IDeviceContext2D * _parent_context;
			oref<ITexture> _swap;
			oref<ITexture> _surface, _surface_mask;
			Rectangle _position;
			float _blend_alpha;
			uint _mode;
		public:
			VKLayerBacking(IDevice * parent_device, IDeviceContext2D * parent_context) : _parent_device(parent_device), _parent_context(parent_context), _position(0, 0, 0, 0), _blend_alpha(0.0f), _mode(0) {}
			virtual ~VKLayerBacking(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKLayerBacking"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent_device; }
			virtual IDeviceContext2D * GetParentContext(void) noexcept override { return _parent_context; }
		};
		class VKTextureHeapPage : public Object
		{
			friend class VKTextureHeap;
			friend class VKTextureHeapAllocation;
		private:
			oref<IBitmap> _surface;
			oref<IDeviceContext2D> _surface_context;
			oref<ITexture> _video_surface;
			uint _width, _height;
			uint _area_free, _area_allocated, _area_discarded;
			uint _needs_synchronization, _needs_defragmentation, _exhausted, _close_session;
			Rectangle _synchronization_rectangle;
			uint _allocator_x, _allocator_y, _allocator_line;
		public:
			VKTextureHeapPage(void) noexcept {}
			virtual ~VKTextureHeapPage(void) override {}
		};
		class VKTextureHeapAllocation : public Object
		{
			friend class VKTextureHeap;
			friend class VKDeviceContext2D;
		private:
			oref<VKTextureHeapPage> _page;
			oref<ITexture> _surface;
			uint _x, _y, _width, _height;
		public:
			VKTextureHeapAllocation(void) noexcept {}
			virtual ~VKTextureHeapAllocation(void) override { if (_page) { auto area = _width * _height; _page->_area_allocated -= area; _page->_area_discarded += area; } }
			bool EndPopulate(IDeviceContext2D * context) noexcept { context->PopClip(); return true; }
		};
		class VKTextureHeap : public Object
		{
			List<oref<VKTextureHeapPage>> _pages;
			List<oref<VKTextureHeapAllocation>> _allocations;
			oref<Graphica::IDeviceContextFactory2D> _factory;
			oref<VKQueue> _queue;
			Graphica::IDevice * _device;
		public:
			VKTextureHeap(Graphica::IDeviceContextFactory2D * factory, Graphica::IDevice * device, VKQueue * queue) : _factory(factory), _device(device), _queue(queue) {}
			virtual ~VKTextureHeap(void) override {}
			bool AllocateRegion(uint width, uint height, uint default_page_width, uint default_page_height, oref<VKTextureHeapPage> & surface_page, oref<Graphica::ITexture> & surface, Rectangle & rect) noexcept
			{
				VKTextureHeapPage * page = 0;
				for (auto & p : _pages) {
					if (p->_needs_defragmentation) continue;
					if (p->_width - p->_allocator_x >= width && p->_height - p->_allocator_y >= height) { page = p.Inner(); break; }
					if (p->_width >= width && p->_height - p->_allocator_y - p->_allocator_line >= height) { p->_allocator_y += p->_allocator_line; p->_allocator_x = p->_allocator_line = 0; page = p.Inner(); break; }
					p->_exhausted = 1;
				}
				if (!page && default_page_width && default_page_height) {
					auto new_page = owrap(new (std::nothrow) VKTextureHeapPage);
					if (!new_page) return false;
					new_page->_width = max(default_page_width, width);
					new_page->_height = max(default_page_height, height);
					if (new_page->_width > 16384 || new_page->_height > 16384) return false;
					new_page->_area_free = new_page->_width * new_page->_height;
					new_page->_area_allocated = new_page->_area_discarded = 0;
					new_page->_needs_synchronization = new_page->_needs_defragmentation = new_page->_exhausted = new_page->_close_session = 0;
					new_page->_allocator_x = new_page->_allocator_y = new_page->_allocator_line = 0;
					new_page->_surface = _factory->CreateBitmap(new_page->_width, new_page->_height, 0);
					if (!new_page->_surface) return false;
					new_page->_surface_context = _factory->CreateBitmapContext(new_page->_surface);
					if (!new_page->_surface_context) return false;
					Graphica::TextureDesc desc;
					desc.Type = Graphica::TextureType::Type2D;
					desc.Format = Graphica::PixelFormat::B8G8R8A8_unorm;
					desc.Width = new_page->_width;
					desc.Height = new_page->_height;
					desc.MipmapCount = 1;
					desc.MemoryPool = Graphica::ResourceMemoryPool::Regular;
					desc.Usage = Graphica::ResourceUsageCPUWrite | Graphica::ResourceUsageShaderRead;
					new_page->_video_surface = _device->CreateTexture(desc);
					if (!new_page->_video_surface) return false;
					try { _pages.InsertLast(new_page); } catch (...) { return false; }
					page = new_page;
				}
				if (!page) return false;
				surface_page = page;
				surface = page->_video_surface;
				rect.left = page->_allocator_x;
				rect.top = page->_allocator_y;
				rect.right = rect.left + width;
				rect.bottom = rect.top + height;
				page->_allocator_x += width;
				page->_allocator_line = max(page->_allocator_line, height);
				page->_area_allocated += width * height;
				page->_area_free = (page->_height - page->_allocator_y - page->_allocator_line) * page->_width + (page->_width - page->_allocator_x) * page->_allocator_line;
				return true;
			}
			oref<VKTextureHeapAllocation> Allocate(uint width, uint height, uint default_page_width, uint default_page_height, oref<IDeviceContext2D> & populate, uint & populate_at_x, uint & populate_at_y) noexcept
			{
				auto result = owrap(new (std::nothrow) VKTextureHeapAllocation);
				if (!result) return 0;
				Rectangle vrect;
				if (!AllocateRegion(width, height, default_page_width, default_page_height, result->_page, result->_surface, vrect)) return 0;
				result->_x = vrect.left;
				result->_y = vrect.top;
				result->_width = width;
				result->_height = height;
				if (result->_page->_needs_synchronization) {
					result->_page->_synchronization_rectangle = Rectangle::OuterRectangle(result->_page->_synchronization_rectangle, vrect);
				} else {
					result->_page->_needs_synchronization = 1;
					result->_page->_synchronization_rectangle = vrect;
				}
				try { _allocations.InsertLast(result); } catch (...) { return 0; }
				populate_at_x = vrect.left;
				populate_at_y = vrect.top;
				populate = result->_page->_surface_context;
				if (!result->_page->_close_session && !populate->BeginRendering(Graphica::TextureLoadAction::Load, 0)) return 0;
				result->_page->_close_session = 1;
				populate->PushClip(vrect);
				return result;
			}
			bool SynchronizeIfNeeded(void) noexcept
			{
				VkCommandBuffer buffer = 0;
				for (auto & p : _pages) {
					if (p->_close_session) {
						p->_surface_context->EndRendering();
						p->_close_session = 0;
					}
					if (p->_needs_synchronization) {
						if (!buffer) {
							buffer = _queue->BeginPrivatePass();
							if (!buffer) return false;
						}
						auto & desc = static_cast<Cairo::CairoBitmap *>(p->_surface.Inner())->GetData()->GetDesc();
						Graphica::ResourceInitDesc data;
						data.Data = reinterpret_cast<const uint8 *>(desc.data) + desc.stride * p->_synchronization_rectangle.top + 4 * p->_synchronization_rectangle.left;
						data.DataPitch = desc.stride;
						if (!_queue->InternalUpdateResourceData(buffer, p->_video_surface, Index2(0, 0), Index3(p->_synchronization_rectangle.left, p->_synchronization_rectangle.top, 0),
							Index3(p->_synchronization_rectangle.right - p->_synchronization_rectangle.left, p->_synchronization_rectangle.bottom - p->_synchronization_rectangle.top, 1), data)) return false;
						p->_needs_synchronization = 0;
					}
				}
				return buffer ? _queue->EndPrivatePass(buffer) : true;
			}
			bool DefragmentIfNeeded(VKPass * pass) noexcept
			{
				bool defragment = false;
				auto current_alloc = _allocations.GetFirst();
				while (current_alloc) {
					auto next = current_alloc->GetNext();
					if (current_alloc->GetValue()->GetReferenceCount() == 1) _allocations.Remove(current_alloc);
					current_alloc = next;
				}
				for (auto & p : _pages) if (p->_exhausted && (p->_area_allocated << 1U) < p->_area_discarded) { p->_needs_defragmentation = 1; if (p->_area_allocated) defragment = true; }
				if (defragment) {
					current_alloc = _allocations.GetFirst();
					while (current_alloc) {
						auto & a = current_alloc->GetValue();
						if (a->_page->_needs_defragmentation) {
							oref<VKTextureHeapPage> page;
							oref<Graphica::ITexture> surface;
							Rectangle rect;
							if (AllocateRegion(a->_width, a->_height, a->_page->_width, a->_page->_height, page, surface, rect)) {
								pass->CopySubresourceData(surface, Index2(0, 0), Index3(rect.left, rect.top, 0), a->_surface, Index2(0, 0), Index3(a->_x, a->_y, 0), Index3(a->_width, a->_height, 1));
								auto & dest = static_cast<Cairo::CairoBitmap *>(page->_surface.Inner())->GetData()->GetDesc();
								auto & src = static_cast<Cairo::CairoBitmap *>(a->_page->_surface.Inner())->GetData()->GetDesc();
								uint scan = 4 * a->_width;
								for (uint y = 0; y < a->_height; y++) Memory::MemoryCopy(
									reinterpret_cast<uint8 *>(dest.data) + (rect.top + y) * dest.stride + 4 * rect.left,
									reinterpret_cast<const uint8 *>(src.data) + (a->_y + y) * src.stride + 4 * a->_x, scan);
								a->_x = rect.left;
								a->_y = rect.top;
								a->_page = page;
								a->_surface = surface;
							}
						}
						current_alloc = current_alloc->GetNext();
					}
				}
				auto current_page = _pages.GetFirst();
				while (current_page) {
					auto next = current_page->GetNext();
					if (current_page->GetValue()->GetReferenceCount() == 1) _pages.Remove(current_page);
					current_page = next;
				}
				return true;
			}
		};
		class VKGlyphRun : public IGlyphRun
		{
			friend class VKDeviceContext2D;
		private:
			IDevice * _parent_device;
			IDeviceContext2D * _parent_context;
			oref<IGlyphRun> _inner;
			uint _width, _height;
			oref<VKTextureHeapAllocation> _surface;
			Rectangle _aabb, _current_view;
		public:
			VKGlyphRun(IDevice * parent_device, IDeviceContext2D * parent_context) : _parent_device(parent_device), _parent_context(parent_context) {}
			virtual ~VKGlyphRun(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKGlyphRun"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent_device; }
			virtual IDeviceContext2D * GetParentContext(void) noexcept override { return _parent_context; }
		};
		class VKColorBrush : public IColorBrush
		{
			friend class VKDeviceContext2D;
		private:
			IDevice * _parent_device;
			IDeviceContext2D * _parent_context;
			oref<IBuffer> _area;
			int _vertex_count;
			Index2 _from, _to;
			bool _gradient;
		public:
			VKColorBrush(IDevice * parent_device, IDeviceContext2D * parent_context) : _parent_device(parent_device), _parent_context(parent_context), _vertex_count(0), _from(0, 0), _to(0, 0), _gradient(false) {}
			virtual ~VKColorBrush(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKColorBrush"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent_device; }
			virtual IDeviceContext2D * GetParentContext(void) noexcept override { return _parent_context; }
			virtual BrushType GetBrushType(void) noexcept override { return BrushType::Color; }
			virtual void OverrideGradientPoints(const Index2 & from, const Index2 & to) noexcept override { _from = from; _to = to; }
		};
		class VKBitmapBrush : public IBitmapBrush
		{
			friend class VKDeviceContext2D;
		private:
			IDevice * _parent_device;
			IDeviceContext2D * _parent_context;
			oref<ITexture> _surface;
			oref<IBuffer> _area;
			int _vertex_count;
			Rectangle _tile_ref_box, _tile_image_box;
			bool _tile, _alpha;
		public:
			VKBitmapBrush(IDevice * parent_device, IDeviceContext2D * parent_context) : _parent_device(parent_device), _parent_context(parent_context), _vertex_count(0), _tile_ref_box(0, 0, 0, 0), _tile_image_box(0, 0, 0, 0), _tile(false), _alpha(false) {}
			virtual ~VKBitmapBrush(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKBitmapBrush"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent_device; }
			virtual IDeviceContext2D * GetParentContext(void) noexcept override { return _parent_context; }
			virtual BrushType GetBrushType(void) noexcept override { return BrushType::Bitmap; }
			virtual void OverrideTileReferenceRectangle(const Rectangle & rect) noexcept override
			{
				_tile_ref_box = rect;
				auto w = rect.right - rect.left;
				auto h = rect.bottom - rect.top;
				if (w <= 0 || h <= 0) return;
				while (_tile_ref_box.left > 0) { _tile_ref_box.left -= w; _tile_ref_box.right -= w; }
				while (_tile_ref_box.top > 0) { _tile_ref_box.top -= h; _tile_ref_box.bottom -= h; }
			}
		};
		class VKBlurEffectBrush : public IBlurEffectBrush
		{
			friend class VKDeviceContext2D;
		private:
			IDevice * _parent_device;
			IDeviceContext2D * _parent_context;
			float _sigma;
		public:
			VKBlurEffectBrush(IDevice * parent_device, IDeviceContext2D * parent_context) : _parent_device(parent_device), _parent_context(parent_context), _sigma(0.0f) {}
			virtual ~VKBlurEffectBrush(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKBlurEffectBrush"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent_device; }
			virtual IDeviceContext2D * GetParentContext(void) noexcept override { return _parent_context; }
			virtual BrushType GetBrushType(void) noexcept override { return BrushType::Blur; }
		};
		class VKInversionEffectBrush : public IInversionEffectBrush
		{
			IDevice * _parent_device;
			IDeviceContext2D * _parent_context;
		public:
			VKInversionEffectBrush(IDevice * parent_device, IDeviceContext2D * parent_context) : _parent_device(parent_device), _parent_context(parent_context) {}
			virtual ~VKInversionEffectBrush(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKInversionEffectBrush"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent_device; }
			virtual IDeviceContext2D * GetParentContext(void) noexcept override { return _parent_context; }
			virtual BrushType GetBrushType(void) noexcept override { return BrushType::Inversion; }
		};
		class VK2DCommonResources : public Object
		{
		public:
			oref<IShaderLibrary> library;
			oref<ITexture> white;
			oref<IBuffer> area;
			oref<IPipelineState> main_alpha_state, main_double_alpha_state, main_opaque_state;
			oref<IPipelineState> gradient_state, tile_state, invert_state, blur_state, layer_blend_state;
		public:
			static oref<VK2DCommonResources> GetCommonResources(IDevice * device) noexcept;
		};
		class VKDeviceContext2D : public IDeviceContext2D
		{
			friend class VK2DCommonResources;
			friend class VKDeviceDeferredContext;
			friend class VKDeviceImmediateContext;
		private:
			struct VKViewportDesc { int offset_x, offset_y, width, height; };
			struct VKDrawDesc { int left, top, right, bottom; int du, dv, index; float alpha; };
			struct VKGradientDesc { float from_x, from_y, to_x, to_y, side_x, side_y, extent_x, extent_y; };
			struct VKTileDesc { Rectangle at; Rectangle tref; Rectangle irect; };
			struct VKVertexDesc { float color[4]; float position[2], uv[2]; };
		private:
			oref<VKQueue> _queue;
			oref<IDeviceContextFactory2D> _parent_factory;
			IDevice * _parent_device;
			IDeviceContext * _parent_context;
			VKPass * _current_pass;
			Stack<Rectangle> _clipboxes;
			Stack<VKViewportDesc> _viewports;
			oref<VK2DCommonResources> _common;
			oref<ITexture> _main_destination, _current_destination, _blur_backstage;
			oref<VKSelectorState2D> _state;
			oref<IDeviceContext2D> _measure_context;
			oref<VKTextureHeap> _heap;
		private:
			void _finalize(void) noexcept
			{
				_current_pass = 0;
				_state.Clear();
				_common.Clear();
				_main_destination.Clear();
				_current_destination.Clear();
				_blur_backstage.Clear();
				_measure_context.Clear();
			}
			static float _saturate(float v) noexcept { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }
			static void _create_color(float * dest, Color c) noexcept
			{
				dest[3] = float(c.a) / 255.0f;
				dest[0] = float(c.r) * dest[3] / 255.0f;
				dest[1] = float(c.g) * dest[3] / 255.0f;
				dest[2] = float(c.b) * dest[3] / 255.0f;
			}
			static void _create_color(float * dest, float r, float g, float b, float a) noexcept { dest[0] = r * a; dest[1] = g * a; dest[2] = b * a; dest[3] = a; }
			static void _create_point(float * dest, float x, float y) noexcept { dest[0] = x; dest[1] = y; }
			static oref<IBuffer> _create_area_buffer(IDevice * device, const Rectangle & area) noexcept
			{
				if (!device) return 0;
				VKVertexDesc data[6];
				_create_point(data[0].position, 0.0f, 0.0f);
				_create_color(data[0].color, 1.0f, 1.0f, 1.0f, 1.0f);
				_create_point(data[0].uv, area.left, area.top);
				_create_point(data[1].position, 1.0f, 0.0f);
				_create_color(data[1].color, 1.0f, 1.0f, 1.0f, 1.0f);
				_create_point(data[1].uv, area.right, area.top);
				_create_point(data[2].position, 0.0f, 1.0f);
				_create_color(data[2].color, 1.0f, 1.0f, 1.0f, 1.0f);
				_create_point(data[2].uv, area.left, area.bottom);
				_create_point(data[5].position, 1.0f, 1.0f);
				_create_color(data[5].color, 1.0f, 1.0f, 1.0f, 1.0f);
				_create_point(data[5].uv, area.right, area.bottom);
				data[3] = data[1];
				data[4] = data[2];
				BufferDesc desc;
				ResourceInitDesc init;
				desc.Length = sizeof(data);
				desc.Stride = sizeof(VKVertexDesc);
				desc.Usage = ResourceUsageShaderRead;
				desc.MemoryPool = ResourceMemoryPool::Immutable;
				init.Data = data;
				return device->CreateBufferWithData(desc, init);
			}
			static oref<IBuffer> _create_area_buffer(IDevice * device, Color color) noexcept
			{
				if (!device) return 0;
				VKVertexDesc data[6];
				_create_point(data[0].position, 0.0f, 0.0f);
				_create_color(data[0].color, color);
				_create_point(data[0].uv, 0.0f, 0.0f);
				_create_point(data[1].position, 1.0f, 0.0f);
				_create_color(data[1].color, color);
				_create_point(data[1].uv, 1.0f, 0.0f);
				_create_point(data[2].position, 0.0f, 1.0f);
				_create_color(data[2].color, color);
				_create_point(data[2].uv, 0.0f, 1.0f);
				_create_point(data[5].position, 1.0f, 1.0f);
				_create_color(data[5].color, color);
				_create_point(data[5].uv, 1.0f, 1.0f);
				data[3] = data[1];
				data[4] = data[2];
				BufferDesc desc;
				ResourceInitDesc init;
				desc.Length = sizeof(data);
				desc.Stride = sizeof(VKVertexDesc);
				desc.Usage = ResourceUsageShaderRead;
				desc.MemoryPool = ResourceMemoryPool::Immutable;
				init.Data = data;
				return device->CreateBufferWithData(desc, init);
			}
			bool _update_selector_constant(uint domain, uint index, const void * data, int length) noexcept
			{
				uint align = length;
				if (align & (_current_pass->stats->constant_alignment - 1)) { align &= ~(_current_pass->stats->constant_alignment - 1); align += _current_pass->stats->constant_alignment; }
				if (_current_pass->AllocateConstantBuffer(align)) {
					uint origin = _current_pass->constant_pool->offset;
					Memory::MemoryCopy(_current_pass->constant_pool->memory_mapping + origin, data, length);
					VkMappedMemoryRange range;
					range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
					range.pNext = 0;
					range.memory = _current_pass->constant_pool->_memory;
					range.offset = origin;
					range.size = align;
					auto api = _queue->GetAPI();
					api->Dispatch.vkFlushMappedMemoryRanges(api->Device, 1, &range);
					_state->UpdateSelector(domain, index, _current_pass->constant_pool, origin, align);
					return true;
				} else return false;
			}
			bool _resume_rendering(TextureLoadAction load, const float * clear_color) noexcept
			{
				auto api = _queue->GetAPI();
				VkMemoryBarrier barrier;
				VkImageMemoryBarrier image_barrier;
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrier.pNext = 0;
				barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				image_barrier.pNext = 0;
				image_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				image_barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
				image_barrier.oldLayout = static_cast<VKTexture *>(_current_destination.Inner())->_current_layout;
				image_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				image_barrier.image = static_cast<VKTexture *>(_current_destination.Inner())->_image;
				image_barrier.srcQueueFamilyIndex = image_barrier.dstQueueFamilyIndex = _queue->GetQueueIndex();
				image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				image_barrier.subresourceRange.baseArrayLayer = 0;
				image_barrier.subresourceRange.baseMipLevel = 0;
				image_barrier.subresourceRange.layerCount = _current_destination.Inner()->GetArraySize();
				image_barrier.subresourceRange.levelCount = _current_destination.Inner()->GetMipmapCount();
				static_cast<VKTexture *>(_current_destination.Inner())->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
				api->Dispatch.vkCmdPipelineBarrier(_current_pass->buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, 0, 1, &image_barrier);
				VkRenderingAttachmentInfoKHR color;
				VkRenderingInfoKHR rendering;
				rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
				rendering.pNext = 0;
				rendering.flags = 0;
				rendering.renderArea.offset.x = rendering.renderArea.offset.y = 0;
				rendering.renderArea.extent.width = _current_destination.Inner()->GetWidth();
				rendering.renderArea.extent.height = _current_destination.Inner()->GetHeight();
				rendering.layerCount = 1;
				rendering.viewMask = 0;
				rendering.colorAttachmentCount = 1;
				rendering.pColorAttachments = &color;
				rendering.pDepthAttachment = 0;
				rendering.pStencilAttachment = 0;
				try { _current_pass->retain.AddElement(_current_destination.Inner()); } catch (...) { return false; }
				color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				color.pNext = 0;
				color.imageView = static_cast<VKTexture *>(_current_destination.Inner())->_view;
				color.imageLayout = static_cast<VKTexture *>(_current_destination.Inner())->_current_layout;
				color.resolveMode = VK_RESOLVE_MODE_NONE_KHR;
				color.resolveImageView = 0;
				color.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				if (load == TextureLoadAction::Clear) {
					color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					if (clear_color) {
						color.clearValue.color.float32[3] = clear_color[3];
						color.clearValue.color.float32[0] = clear_color[0] * clear_color[3];
						color.clearValue.color.float32[1] = clear_color[1] * clear_color[3];
						color.clearValue.color.float32[2] = clear_color[2] * clear_color[3];
					} else Memory::ZeroMemory(&color.clearValue.color, sizeof(color.clearValue.color));
				} else if (load == TextureLoadAction::Load) color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				else color.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				api->Dispatch.vkCmdBeginRenderingKHR(_current_pass->buffer, &rendering);
				auto & current_clipping = _clipboxes.GetLast()->GetValue();
				auto & current_viewport = _viewports.GetLast()->GetValue();
				VkRect2D scissors;
				VkViewport viewport;
				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = _current_destination->GetWidth();
				viewport.height = _current_destination->GetHeight();
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				scissors.offset.x = current_clipping.left - current_viewport.offset_x;
				scissors.offset.y = current_clipping.top - current_viewport.offset_y;
				scissors.extent.width = current_clipping.right - current_clipping.left;
				scissors.extent.height = current_clipping.bottom - current_clipping.top;
				api->Dispatch.vkCmdSetViewportWithCountEXT(_current_pass->buffer, 1, &viewport);
				api->Dispatch.vkCmdSetScissorWithCountEXT(_current_pass->buffer, 1, &scissors);
				_state->update_pipeline_layout = 3;
				_update_selector_constant(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, VKSelectorState2D::selector_constant_global, &current_viewport, sizeof(current_viewport));
				return true;
			}
			void _stop_rendering(void) noexcept
			{
				auto api = _queue->GetAPI();
				api->Dispatch.vkCmdEndRenderingKHR(_current_pass->buffer);
			}
			bool _internal_begin_layer_alpha(ILayerBacking * layer, const Rectangle & rect, bool inherit) noexcept
			{
				if (!_current_pass || !layer) return false;
				auto l = static_cast<VKLayerBacking *>(layer);
				if (l->_mode) return false;
				auto fxr = Rectangle::Intersect(_clipboxes.GetLast()->GetValue(), rect);
				if (fxr.left >= fxr.right || fxr.top >= fxr.bottom) return false;
				l->_position = fxr;
				auto w = fxr.right - fxr.left, h = fxr.bottom - fxr.top;
				auto & primary_viewport = _viewports.GetLast()->GetValue();
				VKViewportDesc viewport;
				viewport.offset_x = fxr.left;
				viewport.offset_y = fxr.top;
				viewport.width = fxr.right - fxr.left;
				viewport.height = fxr.bottom - fxr.top;
				try { _clipboxes.Push(fxr); _viewports.Push(viewport); } catch (...) { return false; }
				if (!l->_surface || l->_surface->GetWidth() < w || l->_surface->GetHeight() < h) {
					TextureDesc desc;
					desc.Type = TextureType::Type2D;
					desc.Format = PixelFormat::B8G8R8A8_unorm;
					desc.Width = w;
					desc.Height = h;
					desc.MipmapCount = 1;
					desc.Usage = ResourceUsageShaderRead | ResourceUsageRenderTarget;
					desc.MemoryPool = ResourceMemoryPool::Regular;
					l->_surface = _parent_device->CreateTexture(desc);
					if (!l->_surface) return false;
				}
				if (!l->_surface_mask || l->_surface_mask->GetWidth() < w || l->_surface_mask->GetHeight() < h) {
					TextureDesc desc;
					desc.Type = TextureType::Type2D;
					desc.Format = PixelFormat::B8G8R8A8_unorm;
					desc.Width = w;
					desc.Height = h;
					desc.MipmapCount = 1;
					desc.Usage = ResourceUsageShaderRead | ResourceUsageRenderTarget;
					desc.MemoryPool = ResourceMemoryPool::Regular;
					l->_surface_mask = _parent_device->CreateTexture(desc);
					if (!l->_surface_mask) return false;
				}
				_stop_rendering();
				l->_swap = _current_destination;
				l->_mode = 1;
				_current_destination = l->_surface_mask;
				if (inherit) {
					auto api = _queue->GetAPI();
					auto source = static_cast<VKTexture *>(l->_swap.Inner());
					auto dest = static_cast<VKTexture *>(l->_surface.Inner());
					VkImageMemoryBarrier barrier[2];
					barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier[0].pNext = barrier[1].pNext = 0;
					barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
					barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
					barrier[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier[0].oldLayout = source->_current_layout;
					barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
					barrier[1].oldLayout = dest->_current_layout;
					barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
					barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue->GetQueueIndex();
					barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue->GetQueueIndex();
					barrier[0].image = source->_image;
					barrier[1].image = dest->_image;
					barrier[0].subresourceRange.aspectMask = barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					barrier[0].subresourceRange.baseArrayLayer = barrier[0].subresourceRange.baseMipLevel = 0;
					barrier[1].subresourceRange.baseArrayLayer = barrier[1].subresourceRange.baseMipLevel = 0;
					barrier[0].subresourceRange.layerCount = barrier[0].subresourceRange.levelCount = 1;
					barrier[1].subresourceRange.layerCount = barrier[1].subresourceRange.levelCount = 1;
					api->Dispatch.vkCmdPipelineBarrier(_current_pass->buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
					dest->_current_layout = source->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
					VkImageCopy copy;
					copy.srcSubresource.aspectMask = copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					copy.srcSubresource.baseArrayLayer = copy.srcSubresource.mipLevel = copy.dstSubresource.baseArrayLayer = copy.dstSubresource.mipLevel = 0;
					copy.srcSubresource.layerCount = copy.dstSubresource.layerCount = 1;
					copy.srcOffset.x = l->_position.left - primary_viewport.offset_x;
					copy.srcOffset.y = l->_position.top - primary_viewport.offset_y;
					copy.srcOffset.z = 0;
					copy.dstOffset.x = copy.dstOffset.y = copy.dstOffset.z = 0;
					copy.extent.width = l->_position.right - l->_position.left;
					copy.extent.height = l->_position.bottom - l->_position.top;
					copy.extent.depth = 1;
					api->Dispatch.vkCmdCopyImage(_current_pass->buffer, source->_image, source->_current_layout, dest->_image, dest->_current_layout, 1, &copy);
				}
				return _resume_rendering(TextureLoadAction::Clear, 0);
			}
			bool _internal_begin_layer(ILayerBacking * layer, const Rectangle & rect, double opacity, bool inherit) noexcept
			{
				if (!_current_pass || !layer) return false;
				auto l = static_cast<VKLayerBacking *>(layer);
				if (l->_mode == 1) {
					_stop_rendering();
					l->_mode = 3;
					l->_blend_alpha = _saturate(opacity);
					_current_destination = l->_surface;
					if (inherit) return _resume_rendering(TextureLoadAction::Load, 0);
					else return _resume_rendering(TextureLoadAction::Clear, 0);
				} else if (l->_mode == 0) {
					auto fxr = Rectangle::Intersect(_clipboxes.GetLast()->GetValue(), rect);
					if (fxr.left >= fxr.right || fxr.top >= fxr.bottom) return false;
					l->_position = fxr;
					l->_blend_alpha = _saturate(opacity);
					auto w = fxr.right - fxr.left, h = fxr.bottom - fxr.top;
					VKViewportDesc viewport;
					viewport.offset_x = fxr.left;
					viewport.offset_y = fxr.top;
					viewport.width = fxr.right - fxr.left;
					viewport.height = fxr.bottom - fxr.top;
					try { _clipboxes.Push(fxr); _viewports.Push(viewport); } catch (...) { return false; }
					if (!l->_surface || l->_surface->GetWidth() < w || l->_surface->GetHeight() < h) {
						TextureDesc desc;
						desc.Type = TextureType::Type2D;
						desc.Format = PixelFormat::B8G8R8A8_unorm;
						desc.Width = w;
						desc.Height = h;
						desc.MipmapCount = 1;
						desc.Usage = ResourceUsageShaderRead | ResourceUsageRenderTarget;
						desc.MemoryPool = ResourceMemoryPool::Regular;
						l->_surface = _parent_device->CreateTexture(desc);
						if (!l->_surface) return false;
					}
					_stop_rendering();
					l->_swap = _current_destination;
					l->_mode = 2;
					_current_destination = l->_surface;
					return _resume_rendering(TextureLoadAction::Clear, 0);
				} else return false;
			}
		public:
			VKDeviceContext2D(VKQueue * queue, IDevice * parent_device, IDeviceContext * parent_context) : _queue(queue), _parent_device(parent_device), _parent_context(parent_context), _current_pass(0)
			{
				_parent_factory = CreateDeviceContextFactory2D();
				_common = VK2DCommonResources::GetCommonResources(parent_device);
				if (!_common) throw InvalidStateException();
			}
			virtual ~VKDeviceContext2D(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKDeviceContext2D"; ESSE_TRY_OUTRO(string()) }
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept override
			{
				if (cls == Classes.Object || cls == Classes.DynamicObject || cls == Classes.IDeviceContext2D) {
					Retain(); return this;
				} else if (cls == Classes.IDevice) {
					if (!_parent_device) { ErrorSet(ectx, Errores::ErrorInvalidState); return 0; }
					_parent_device->Retain(); return _parent_device;
				} else if (cls == Classes.IDeviceContext) {
					if (!_parent_context) { ErrorSet(ectx, Errores::ErrorInvalidState); return 0; }
					_parent_context->Retain(); return _parent_context;
				} else if (cls == Classes.IDeviceContextFactory2D) {
					_parent_factory->Retain(); return _parent_factory;
				} else { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			}
			virtual const void * GetType(void) noexcept override { return Classes.IDeviceContext2D; }
			virtual void GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) noexcept override { _parent_device->GetImplementationInfo(tech, version_major, version_minor); }
			virtual uint32 GetImplementationFeatures(void) noexcept override { return DeviceContextSupportsBlurEffect | DeviceContextSupportsInversionEffect | DeviceContextSupportsLayers | DeviceContextHardwareAccelerated | DeviceContextHasControllingDevice; }
			virtual oref<IColorBrush> CreateSolidColorBrush(const Color & color) noexcept override
			{
				auto result = owrap(new (std::nothrow) VKColorBrush(_parent_device, this));
				if (!result) return 0;
				result->_area = _create_area_buffer(_parent_device, color);
				result->_vertex_count = 6;
				if (!result->_area) return 0;
				return oref<IColorBrush>(result);
			}
			virtual oref<IColorBrush> CreateGradientBrush(const Index2 & from, const Index2 & to, const Color * colors, const double * positions, uint count) noexcept override
			{
				if (count < 1) return 0;
				if (count == 1) return CreateSolidColorBrush(colors[0]);
				auto result = owrap(new (std::nothrow) VKColorBrush(_parent_device, this));
				if (!result) return 0;
				try {
					array<VKVertexDesc> data(0x80);
					VKVertexDesc v;
					_create_point(v.uv, 0.0f, 0.0f);
					_create_point(v.position, -1.0f, -1.0f);
					_create_color(v.color, colors[0]);
					data << v;
					_create_point(v.position, -1.0f, 1.0f);
					data << v;
					_create_point(v.position, _saturate(positions[0]), -1.0f);
					data << v;
					data << data[data.GetLength() - 2];
					data << data[data.GetLength() - 2];
					_create_point(v.position, _saturate(positions[0]), 1.0f);
					data << v;
					for (int i = 1; i < count; i++) {
						data << data[data.GetLength() - 2];
						data << data[data.GetLength() - 2];
						_create_point(v.position, _saturate(positions[i]), -1.0f);
						_create_color(v.color, colors[i]);
						data << v;
						data << data[data.GetLength() - 2];
						data << data[data.GetLength() - 2];
						_create_point(v.position, _saturate(positions[i]), 1.0f);
						data << v;
					}
					data << data[data.GetLength() - 2];
					data << data[data.GetLength() - 2];
					_create_point(v.position, 2.0f, -1.0f);
					data << v;
					data << data[data.GetLength() - 2];
					data << data[data.GetLength() - 2];
					_create_point(v.position, 2.0f, 1.0f);
					data << v;
					BufferDesc desc;
					ResourceInitDesc init;
					desc.Length = data.GetLength() * sizeof(VKVertexDesc);
					desc.Stride = sizeof(VKVertexDesc);
					desc.Usage = ResourceUsageShaderRead;
					desc.MemoryPool = ResourceMemoryPool::Immutable;
					init.Data = data.GetBuffer();
					result->_area = _parent_device->CreateBufferWithData(desc, init);
					result->_vertex_count = data.GetLength();
					if (!result->_area) return 0;
				} catch (...) { return 0; }
				result->_from = from;
				result->_to = to;
				result->_gradient = true;
				return oref<IColorBrush>(result);
			}
			virtual oref<IBitmapBrush> CreateBitmapBrush(IBitmap * bitmap, const Rectangle & area) noexcept override
			{
				if (!bitmap) return 0;
				auto entire = Rectangle(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
				auto fx = Rectangle::Intersect(area, entire);
				if (fx.left >= fx.right || fx.top >= fx.bottom) return 0;
				auto result = owrap(new (std::nothrow) VKBitmapBrush(_parent_device, this));
				auto storage = static_cast<Cairo::CairoBitmap *>(bitmap)->GetData();
				ResourceInitDesc init;
				TextureDesc desc;
				desc.Type = TextureType::Type2D;
				desc.Format = PixelFormat::B8G8R8A8_unorm;
				desc.Width = bitmap->GetWidth();
				desc.Height = bitmap->GetHeight();
				desc.MipmapCount = 1;
				desc.Usage = ResourceUsageShaderRead;
				desc.MemoryPool = ResourceMemoryPool::Immutable;
				init.Data = storage->GetDesc().data;
				init.DataPitch = storage->GetDesc().stride;
				result->_surface = _parent_device->CreateTextureWithData(desc, &init);
				if (!result->_surface) return 0;
				result->_area = _create_area_buffer(_parent_device, fx);
				if (!result->_area) return 0;
				result->_vertex_count = 6;
				result->_alpha = true;
				return oref<IBitmapBrush>(result);
			}
			virtual oref<IBitmapBrush> CreateBitmapBrushCopy(IBitmapBrush * bitmap, const Rectangle & area) noexcept override
			{
				if (!bitmap) return 0;
				auto texture = static_cast<VKBitmapBrush *>(bitmap)->_surface;
				auto entire = Rectangle(0, 0, texture->GetWidth(), texture->GetHeight());
				auto fx = Rectangle::Intersect(area, entire);
				if (fx.left >= fx.right || fx.top >= fx.bottom) return 0;
				auto result = owrap(new (std::nothrow) VKBitmapBrush(_parent_device, this));
				result->_surface = texture;
				if (!result->_surface) return 0;
				result->_area = _create_area_buffer(_parent_device, fx);
				if (!result->_area) return 0;
				result->_vertex_count = 6;
				result->_alpha = true;
				return oref<IBitmapBrush>(result);
			}
			virtual oref<IBitmapBrush> CreateTileBrush(IBitmap * bitmap, const Rectangle & area) noexcept override
			{
				if (!bitmap) return 0;
				auto entire = Rectangle(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
				auto fx = Rectangle::Intersect(area, entire);
				if (fx.left >= fx.right || fx.top >= fx.bottom) return 0;
				auto result = owrap(new (std::nothrow) VKBitmapBrush(_parent_device, this));
				auto storage = static_cast<Cairo::CairoBitmap *>(bitmap)->GetData();
				ResourceInitDesc init;
				TextureDesc desc;
				desc.Type = TextureType::Type2D;
				desc.Format = PixelFormat::B8G8R8A8_unorm;
				desc.Width = bitmap->GetWidth();
				desc.Height = bitmap->GetHeight();
				desc.MipmapCount = 1;
				desc.Usage = ResourceUsageShaderRead;
				desc.MemoryPool = ResourceMemoryPool::Immutable;
				init.Data = storage->GetDesc().data;
				init.DataPitch = storage->GetDesc().stride;
				result->_surface = _parent_device->CreateTextureWithData(desc, &init);
				if (!result->_surface) return 0;
				result->_area = _common->area;
				result->_vertex_count = 6;
				result->_tile_ref_box = Rectangle(0, 0, fx.right - fx.left, fx.bottom - fx.top);
				result->_tile_image_box = fx;
				result->_tile = true;
				result->_alpha = true;
				return oref<IBitmapBrush>(result);
			}
			virtual oref<IBitmapBrush> CreateTileBrushCopy(IBitmapBrush * bitmap, const Rectangle & area) noexcept override
			{
				if (!bitmap) return 0;
				auto texture = static_cast<VKBitmapBrush *>(bitmap)->_surface;
				auto entire = Rectangle(0, 0, texture->GetWidth(), texture->GetHeight());
				auto fx = Rectangle::Intersect(area, entire);
				if (fx.left >= fx.right || fx.top >= fx.bottom) return 0;
				auto result = owrap(new (std::nothrow) VKBitmapBrush(_parent_device, this));
				result->_surface = texture;
				if (!result->_surface) return 0;
				result->_area = _common->area;
				result->_vertex_count = 6;
				result->_tile_ref_box = Rectangle(0, 0, fx.right - fx.left, fx.bottom - fx.top);
				result->_tile_image_box = fx;
				result->_tile = true;
				result->_alpha = true;
				return oref<IBitmapBrush>(result);
			}
			virtual oref<IBitmapBrush> CreateTextureBrush(ITexture * texture, TextureAlphaMode mode) noexcept override
			{
				if (!texture || mode == TextureAlphaMode::Straight) return 0;
				auto entire = Rectangle(0, 0, texture->GetWidth(), texture->GetHeight());
				auto result = owrap(new (std::nothrow) VKBitmapBrush(_parent_device, this));
				result->_surface = texture;
				if (!result->_surface) return 0;
				result->_area = _create_area_buffer(_parent_device, entire);
				if (!result->_area) return 0;
				result->_vertex_count = 6;
				result->_alpha = mode == TextureAlphaMode::Premultiplied;
				return oref<IBitmapBrush>(result);
			}
			virtual oref<IBlurEffectBrush> CreateBlurEffectBrush(double sigma) noexcept override
			{
				if (sigma <= 0.0) return 0;
				auto effective_sigma = min(sigma, 1.0E+3);
				auto result = owrap(new (std::nothrow) VKBlurEffectBrush(_parent_device, this));
				if (!result) return 0;
				result->_sigma = effective_sigma;
				return oref<IBlurEffectBrush>(result);
			}
			virtual oref<IInversionEffectBrush> CreateInversionEffectBrush(void) noexcept override { return oref<IInversionEffectBrush>::CreateOwned(new (std::nothrow) VKInversionEffectBrush(_parent_device, this)); }
			virtual oref<ILayerBacking> CreateLayerBackingStorage(void) noexcept override { return oref<ILayerBacking>::CreateOwned(new (std::nothrow) VKLayerBacking(_parent_device, this)); }
			virtual oref<IGlyphRun> CreateGlyphRun(IFont ** fonts, const uint * glyphs, const double * px, const double * py, const Color * colors, uint count, const double * transform) noexcept override
			{
				auto result = owrap(new VKGlyphRun(_parent_device, this));
				if (!result) return 0;
				if (!_measure_context) {
					auto measure_bitmap = _parent_factory->CreateBitmap(1, 1, 0);
					if (!measure_bitmap) return 0;
					_measure_context = _parent_factory->CreateBitmapContext(measure_bitmap);
					if (!_measure_context) return 0;
				}
				result->_inner = _measure_context->CreateGlyphRun(fonts, glyphs, px, py, colors, count, transform);
				if (!result->_inner) return 0;
				bool undefined = true;
				double bl, bt, br, bb;
				for (uint i = 0; i < count; i++) if (fonts[i] && glyphs[i] != InvalidGlyph) {
					FontMetrics fm;
					FontGlyphMetrics gm;
					fonts[i]->GetFontMetrics(fm);
					fonts[i]->GetGlyphMetrics(glyphs + i, &gm, 1);
					double threshold = (fm.Ascent - fm.Descent) * 0.25;
					double mnx, mxx, mny, mxy;
					auto left = px[i] + gm.HorizontalLeftBearing - threshold;
					auto right = px[i] + gm.HorizontalAdvance - gm.HorizontalRightBearing + threshold;
					auto top = py[i] - gm.HorizontalTopBearing - threshold;
					auto bottom = py[i] - gm.HorizontalBottomBearing + threshold;
					if (transform) {
						double x[4], y[4];
						x[0] = transform[0] * left + transform[1] * top + transform[2];
						y[0] = transform[3] * left + transform[4] * top + transform[5];
						x[1] = transform[0] * right + transform[1] * top + transform[2];
						y[1] = transform[3] * right + transform[4] * top + transform[5];
						x[2] = transform[0] * right + transform[1] * bottom + transform[2];
						y[2] = transform[3] * right + transform[4] * bottom + transform[5];
						x[3] = transform[0] * left + transform[1] * bottom + transform[2];
						y[3] = transform[3] * left + transform[4] * bottom + transform[5];
						mnx = min(min(x[0], x[1]), min(x[2], x[3]));
						mxx = max(max(x[0], x[1]), max(x[2], x[3]));
						mny = min(min(y[0], y[1]), min(y[2], y[3]));
						mxy = max(max(y[0], y[1]), max(y[2], y[3]));
					} else {
						mnx = min(left, right);
						mxx = max(left, right);
						mny = min(top, bottom);
						mxy = max(top, bottom);
					}
					if (undefined) {
						undefined = false;
						bl = mnx; br = mxx;
						bt = mny; bb = mxy;
					} else {
						if (mnx < bl) bl = mnx;
						if (mny < bt) bt = mny;
						if (mxx > br) br = mxx;
						if (mxy > bb) bb = mxy;
					}
				}
				if (undefined) return 0;
				result->_aabb = Rectangle(floor(bl), floor(bt), ceil(br), ceil(bb));
				result->_width = min<int>(result->_aabb.right - result->_aabb.left, 16384);
				result->_height = min<int>(result->_aabb.bottom - result->_aabb.top, 16384);
				return oref<IGlyphRun>(result);
			}
			virtual void PushClip(const Rectangle & rect) noexcept override
			{
				if (!_current_pass || _clipboxes.IsEmpty()) return;
				try {
					auto api = _queue->GetAPI();
					auto at = Rectangle::Intersect(_clipboxes.GetLast()->GetValue(), rect);
					_clipboxes.Push(at);
					auto & viewport = _viewports.GetLast()->GetValue();
					VkRect2D scissor;
					scissor.offset.x = at.left - viewport.offset_x;
					scissor.offset.y = at.top - viewport.offset_y;
					scissor.extent.width = at.right - at.left;
					scissor.extent.height = at.bottom - at.top;
					api->Dispatch.vkCmdSetScissorWithCountEXT(_current_pass->buffer, 1, &scissor);
				} catch (...) {}
			}
			virtual void PopClip(void) noexcept override
			{
				if (_current_pass && !_clipboxes.IsEmpty()) {
					auto api = _queue->GetAPI();
					_clipboxes.RemoveLast();
					auto & box = _clipboxes.GetLast()->GetValue();
					auto & viewport = _viewports.GetLast()->GetValue();
					VkRect2D scissor;
					scissor.offset.x = box.left - viewport.offset_x;
					scissor.offset.y = box.top - viewport.offset_y;
					scissor.extent.width = box.right - box.left;
					scissor.extent.height = box.bottom - box.top;
					api->Dispatch.vkCmdSetScissorWithCountEXT(_current_pass->buffer, 1, &scissor);
				}
			}
			virtual bool BeginLayerAlpha(ILayerBacking * layer, const Rectangle & rect) noexcept override { return _internal_begin_layer_alpha(layer, rect, false); }
			virtual bool BeginLayer(ILayerBacking * layer, const Rectangle & rect, double opacity) noexcept override { return _internal_begin_layer(layer, rect, opacity, false); }
			virtual void EndLayer(ILayerBacking * layer) noexcept override
			{
				if (!_current_pass || !layer) return;
				auto l = static_cast<VKLayerBacking *>(layer);
				if (l->_mode == 2 || l->_mode == 3) {
					auto mode = l->_mode;
					_stop_rendering();
					_clipboxes.Pop();
					_viewports.Pop();
					_current_destination = l->_swap;
					l->_mode = 0;
					l->_swap.Clear();
					_resume_rendering(TextureLoadAction::Load, 0);
					if (l->_blend_alpha) {
						auto api = _queue->GetAPI();
						IPipelineState * pipeline;
						if (mode == 3) pipeline = _common->main_double_alpha_state; else pipeline = _common->main_alpha_state;
						_state->UpdatePipeline(pipeline);
						VKDrawDesc draw;
						draw.left = l->_position.left;
						draw.top = l->_position.top;
						draw.right = l->_position.right;
						draw.bottom = l->_position.bottom;
						draw.du = l->_position.right - l->_position.left;
						draw.dv = l->_position.bottom - l->_position.top;
						draw.index = 0;
						draw.alpha = l->_blend_alpha;
						if (!_update_selector_constant(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, VKSelectorState2D::selector_constant_local, &draw, sizeof(draw))) return;
						_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer, VKSelectorState2D::selector_vertex_buffer, _common->area);
						_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture, VKSelectorState2D::selector_color_surface, l->_surface);
						if (mode == 3) _state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture, VKSelectorState2D::selector_mask_surface, l->_surface_mask);
						if (!_state->PushState(_current_pass->buffer, _current_pass->retain, _current_pass->stub_sampler)) return;
						api->Dispatch.vkCmdDraw(_current_pass->buffer, 6, 1, 0, 0);
					}
				}
			}
			virtual void Render(IBrush * brush, const Rectangle & at) noexcept override
			{
				if (!_current_pass || !brush) return;
				auto vrect = Rectangle::Intersect(_clipboxes.GetLast()->GetValue(), at);
				if (vrect.left >= vrect.right || vrect.top >= vrect.bottom) return;
				auto api = _queue->GetAPI();
				auto type = brush->GetBrushType();
				if (type == BrushType::Color) {
					auto b = static_cast<VKColorBrush *>(brush);
					if (b->_gradient) {
						PushClip(at);
						float dx = b->_to.x - b->_from.x, dy = b->_to.y - b->_from.y;
						float sx = dy, sy = -dx;
						float ox = b->_from.x, oy = b->_from.y;
						float n = dx * dx + dy * dy;
						float xc1 = ((at.left - ox) * dx + (at.top - oy) * dy) / n;
						float xc2 = ((at.right - ox) * dx + (at.top - oy) * dy) / n;
						float xc3 = ((at.left - ox) * dx + (at.bottom - oy) * dy) / n;
						float xc4 = ((at.right - ox) * dx + (at.bottom - oy) * dy) / n;
						float xmx = max(max(xc1, xc2), max(xc3, xc4));
						float xmn = min(min(xc1, xc2), min(xc3, xc4));
						xmx = abs(max(xmx - 1.0f, -xmx));
						xmn = abs(max(xmn - 1.0f, -xmn));
						float yc1 = ((at.left - ox) * sx + (at.top - oy) * sy) / n;
						float yc2 = ((at.right - ox) * sx + (at.top - oy) * sy) / n;
						float yc3 = ((at.left - ox) * sx + (at.bottom - oy) * sy) / n;
						float yc4 = ((at.right - ox) * sx + (at.bottom - oy) * sy) / n;
						float ymx = abs(max(max(yc1, yc2), max(yc3, yc4)));
						float ymn = abs(min(min(yc1, yc2), min(yc3, yc4)));
						_state->UpdatePipeline(_common->gradient_state);
						VKGradientDesc grad;
						grad.from_x = b->_from.x;
						grad.from_y = b->_from.y;
						grad.to_x = b->_to.x;
						grad.to_y = b->_to.y;
						grad.side_x = sx;
						grad.side_y = sy;
						grad.extent_x = max(xmx, xmn) + 0.01f;
						grad.extent_y = max(ymx, ymn) + 0.01f;
						if (!_update_selector_constant(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, VKSelectorState2D::selector_constant_local, &grad, sizeof(grad))) { PopClip(); return; }
						_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer, VKSelectorState2D::selector_vertex_buffer, b->_area);
						_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture, VKSelectorState2D::selector_color_surface, _common->white);
						if (!_state->PushState(_current_pass->buffer, _current_pass->retain, _current_pass->stub_sampler)) { PopClip(); return; }
						api->Dispatch.vkCmdDraw(_current_pass->buffer, b->_vertex_count, 1, 0, 0);
						PopClip();
					} else {
						_state->UpdatePipeline(_common->main_alpha_state);
						VKDrawDesc draw;
						draw.left = at.left;
						draw.top = at.top;
						draw.right = at.right;
						draw.bottom = at.bottom;
						draw.du = draw.dv = draw.index = 0;
						draw.alpha = 1.0f;
						if (!_update_selector_constant(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, VKSelectorState2D::selector_constant_local, &draw, sizeof(draw))) return;
						_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer, VKSelectorState2D::selector_vertex_buffer, b->_area);
						_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture, VKSelectorState2D::selector_color_surface, _common->white);
						if (!_state->PushState(_current_pass->buffer, _current_pass->retain, _current_pass->stub_sampler)) return;
						api->Dispatch.vkCmdDraw(_current_pass->buffer, b->_vertex_count, 1, 0, 0);
					}
				} else if (type == BrushType::Bitmap) {
					auto b = static_cast<VKBitmapBrush *>(brush);
					if (b->_tile) {
						_state->UpdatePipeline(_common->tile_state);
						VKTileDesc draw;
						draw.at = at;
						draw.tref = b->_tile_ref_box;
						draw.irect = b->_tile_image_box;
						if (!_update_selector_constant(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, VKSelectorState2D::selector_constant_local, &draw, sizeof(draw))) return;
					} else {
						_state->UpdatePipeline(b->_alpha ? _common->main_alpha_state : _common->main_opaque_state);
						VKDrawDesc draw;
						draw.left = at.left;
						draw.top = at.top;
						draw.right = at.right;
						draw.bottom = at.bottom;
						draw.du = 1;
						draw.dv = 1;
						draw.index = 0;
						draw.alpha = 1.0f;
						if (!_update_selector_constant(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, VKSelectorState2D::selector_constant_local, &draw, sizeof(draw))) return;
					}
					_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer, VKSelectorState2D::selector_vertex_buffer, b->_area);
					_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture, VKSelectorState2D::selector_color_surface, b->_surface);
					if (!_state->PushState(_current_pass->buffer, _current_pass->retain, _current_pass->stub_sampler)) return;
					api->Dispatch.vkCmdDraw(_current_pass->buffer, b->_vertex_count, 1, 0, 0);
				} else if (type == BrushType::Blur) {
					auto b = static_cast<VKBlurEffectBrush *>(brush);
					auto blur_box = Rectangle::Intersect(_clipboxes.GetLast()->GetValue(), at);
					if (blur_box.right <= blur_box.left || blur_box.bottom <= blur_box.top) return;
					auto size = Index2(blur_box.right - blur_box.left, blur_box.bottom - blur_box.top);
					if (!_blur_backstage || _blur_backstage->GetWidth() < size.x || _blur_backstage->GetHeight() < size.y) {
						TextureDesc desc;
						desc.Type = TextureType::Type2D;
						desc.Format = PixelFormat::B8G8R8A8_unorm;
						desc.Width = size.x;
						desc.Height = size.y;
						desc.MipmapCount = 2;
						desc.Usage = ResourceUsageShaderRead;
						desc.MemoryPool = ResourceMemoryPool::Regular;
						_blur_backstage = _parent_device->CreateTexture(desc);
						if (!_blur_backstage) return;
					}
					_stop_rendering();
					uint effective_lod = 0;
					float effective_sigma = b->_sigma;
					Index2 mip_size = size;
					auto & viewport = _viewports.GetLast()->GetValue();
					if (effective_sigma <= 2.0f) {
						VkImageMemoryBarrier barrier[2];
						barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						barrier[0].pNext = barrier[1].pNext = 0;
						barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
						barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
						barrier[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].oldLayout = static_cast<VKTexture *>(_current_destination.Inner())->_current_layout;
						barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[1].oldLayout = static_cast<VKTexture *>(_blur_backstage.Inner())->_current_layout;
						barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue->GetQueueIndex();
						barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue->GetQueueIndex();
						barrier[0].image = static_cast<VKTexture *>(_current_destination.Inner())->_image;
						barrier[1].image = static_cast<VKTexture *>(_blur_backstage.Inner())->_image;
						barrier[0].subresourceRange.aspectMask = barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						barrier[0].subresourceRange.baseArrayLayer = barrier[0].subresourceRange.baseMipLevel = 0;
						barrier[1].subresourceRange.baseArrayLayer = barrier[1].subresourceRange.baseMipLevel = 0;
						barrier[0].subresourceRange.layerCount = 1;
						barrier[0].subresourceRange.levelCount = _current_destination->GetMipmapCount();
						barrier[1].subresourceRange.layerCount = 1;
						barrier[1].subresourceRange.levelCount = 1;
						api->Dispatch.vkCmdPipelineBarrier(_current_pass->buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
						static_cast<VKTexture *>(_current_destination.Inner())->_current_layout = static_cast<VKTexture *>(_blur_backstage.Inner())->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
						VkImageCopy copy;
						copy.srcSubresource.aspectMask = copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copy.srcSubresource.baseArrayLayer = copy.srcSubresource.mipLevel = copy.dstSubresource.baseArrayLayer = copy.dstSubresource.mipLevel = 0;
						copy.srcSubresource.layerCount = copy.dstSubresource.layerCount = 1;
						copy.srcOffset.x = blur_box.left - viewport.offset_x;
						copy.srcOffset.y = blur_box.top - viewport.offset_y;
						copy.srcOffset.z = 0;
						copy.dstOffset.x = copy.dstOffset.y = copy.dstOffset.z = 0;
						copy.extent.width = size.x;
						copy.extent.height = size.y;
						copy.extent.depth = 1;
						api->Dispatch.vkCmdCopyImage(_current_pass->buffer, static_cast<VKTexture *>(_current_destination.Inner())->_image, static_cast<VKTexture *>(_current_destination.Inner())->_current_layout, static_cast<VKTexture *>(_blur_backstage.Inner())->_image, static_cast<VKTexture *>(_blur_backstage.Inner())->_current_layout, 1, &copy);
					} else {
						VkImageMemoryBarrier barrier[2];
						barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						barrier[0].pNext = barrier[1].pNext = 0;
						barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
						barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
						barrier[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
						barrier[0].oldLayout = static_cast<VKTexture *>(_current_destination.Inner())->_current_layout;
						barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[1].oldLayout = static_cast<VKTexture *>(_blur_backstage.Inner())->_current_layout;
						barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
						barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue->GetQueueIndex();
						barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue->GetQueueIndex();
						barrier[0].image = static_cast<VKTexture *>(_current_destination.Inner())->_image;
						barrier[1].image = static_cast<VKTexture *>(_blur_backstage.Inner())->_image;
						barrier[0].subresourceRange.aspectMask = barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						barrier[0].subresourceRange.baseArrayLayer = barrier[0].subresourceRange.baseMipLevel = 0;
						barrier[1].subresourceRange.baseArrayLayer = barrier[1].subresourceRange.baseMipLevel = 0;
						barrier[0].subresourceRange.layerCount = 1;
						barrier[0].subresourceRange.levelCount = _current_destination->GetMipmapCount();
						barrier[1].subresourceRange.layerCount = 1;
						barrier[1].subresourceRange.levelCount = _blur_backstage->GetMipmapCount();
						api->Dispatch.vkCmdPipelineBarrier(_current_pass->buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
						static_cast<VKTexture *>(_current_destination.Inner())->_current_layout = static_cast<VKTexture *>(_blur_backstage.Inner())->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
						VkImageBlit blt;
						blt.srcSubresource.aspectMask = blt.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						blt.srcSubresource.baseArrayLayer = blt.dstSubresource.baseArrayLayer = 0;
						blt.srcSubresource.layerCount = blt.dstSubresource.layerCount = 1;
						blt.srcSubresource.mipLevel = blt.dstSubresource.mipLevel = 0;
						blt.srcOffsets[0].x = blur_box.left - viewport.offset_x;
						blt.srcOffsets[0].y = blur_box.top - viewport.offset_y;
						blt.srcOffsets[0].z = 0;
						blt.dstOffsets[0].x = blt.dstOffsets[0].y = blt.dstOffsets[0].z = 0;
						blt.srcOffsets[1].x = blur_box.right - viewport.offset_x;
						blt.srcOffsets[1].y = blur_box.bottom - viewport.offset_y;
						blt.srcOffsets[1].z = blt.dstOffsets[1].z = 1;
						blt.dstOffsets[1].x = mip_size.x / 2;
						blt.dstOffsets[1].y = mip_size.y / 2;
						api->Dispatch.vkCmdBlitImage(_current_pass->buffer, static_cast<VKTexture *>(_current_destination.Inner())->_image, static_cast<VKTexture *>(_current_destination.Inner())->_current_layout, static_cast<VKTexture *>(_blur_backstage.Inner())->_image, static_cast<VKTexture *>(_blur_backstage.Inner())->_current_layout, 1, &blt, VK_FILTER_LINEAR);
						effective_sigma /= 2.0f; mip_size.x >>= 1; mip_size.y >>= 1;
						while (effective_sigma > 2.0f) {
							barrier[0].oldLayout = barrier[1].oldLayout = barrier[0].newLayout = barrier[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
							barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue->GetQueueIndex();
							barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue->GetQueueIndex();
							barrier[0].image = barrier[1].image = static_cast<VKTexture *>(_blur_backstage.Inner())->_image;
							barrier[0].subresourceRange.aspectMask = barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
							barrier[0].subresourceRange.baseArrayLayer = 0;
							barrier[0].subresourceRange.baseMipLevel = effective_lod;
							barrier[1].subresourceRange.baseArrayLayer = 0;
							barrier[1].subresourceRange.baseMipLevel = 1 - effective_lod;
							barrier[0].subresourceRange.layerCount = barrier[0].subresourceRange.levelCount = 1;
							barrier[1].subresourceRange.layerCount = barrier[1].subresourceRange.levelCount = 1;
							api->Dispatch.vkCmdPipelineBarrier(_current_pass->buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
							blt.srcSubresource.baseArrayLayer = blt.dstSubresource.baseArrayLayer = 0;
							blt.srcSubresource.layerCount = blt.dstSubresource.layerCount = 1;
							blt.srcSubresource.mipLevel = effective_lod;
							blt.dstSubresource.mipLevel = 1 - effective_lod;
							blt.srcOffsets[0].x = blt.srcOffsets[0].y = blt.srcOffsets[0].z = blt.dstOffsets[0].x = blt.dstOffsets[0].y = blt.dstOffsets[0].z = 0;
							blt.srcOffsets[1].x = mip_size.x;
							blt.srcOffsets[1].y = mip_size.y;
							blt.dstOffsets[1].x = mip_size.x / 2;
							blt.dstOffsets[1].y = mip_size.y / 2;
							blt.srcOffsets[1].z = blt.dstOffsets[1].z = 1;
							api->Dispatch.vkCmdBlitImage(_current_pass->buffer, barrier[0].image, VK_IMAGE_LAYOUT_GENERAL, barrier[1].image, VK_IMAGE_LAYOUT_GENERAL, 1, &blt, VK_FILTER_LINEAR);
							effective_lod = 1 - effective_lod; effective_sigma /= 2.0f; mip_size.x >>= 1; mip_size.y >>= 1;
						}
					}
					_resume_rendering(TextureLoadAction::Load, 0);
					_state->UpdatePipeline(_common->blur_state);
					VKDrawDesc draw;
					draw.left = at.left;
					draw.top = at.top;
					draw.right = at.right;
					draw.bottom = at.bottom;
					draw.du = mip_size.x;
					draw.dv = mip_size.y;
					draw.index = effective_lod;
					draw.alpha = effective_sigma;
					if (!_update_selector_constant(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, VKSelectorState2D::selector_constant_local, &draw, sizeof(draw))) return;
					_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer, VKSelectorState2D::selector_vertex_buffer, _common->area);
					_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture, VKSelectorState2D::selector_color_surface, _blur_backstage);
					if (!_state->PushState(_current_pass->buffer, _current_pass->retain, _current_pass->stub_sampler)) return;
					api->Dispatch.vkCmdDraw(_current_pass->buffer, 6, 1, 0, 0);
				} else if (type == BrushType::Inversion) {
					_state->UpdatePipeline(_common->invert_state);
					VKDrawDesc draw;
					draw.left = at.left;
					draw.top = at.top;
					draw.right = at.right;
					draw.bottom = at.bottom;
					draw.du = draw.dv = draw.index = 0;
					draw.alpha = 1.0f;
					if (!_update_selector_constant(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, VKSelectorState2D::selector_constant_local, &draw, sizeof(draw))) return;
					_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer, VKSelectorState2D::selector_vertex_buffer, _common->area);
					_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture, VKSelectorState2D::selector_color_surface, _common->white);
					if (!_state->PushState(_current_pass->buffer, _current_pass->retain, _current_pass->stub_sampler)) return;
					api->Dispatch.vkCmdDraw(_current_pass->buffer, 6, 1, 0, 0);
				}
			}
			virtual void RenderPolyline(const double * px, const double * py, uint count, bool closed, IBrush * brush, double width) noexcept override
			{
				if (count < 2 || !brush) return;
				Rectangle aabb(floor(px[0]), floor(py[0]), ceil(px[0]) + 1, ceil(py[0]) + 1);
				for (uint i = 1; i < count; i++) {
					if (floor(px[i]) < aabb.left) aabb.left = floor(px[i]);
					if (ceil(px[i]) > aabb.right - 1) aabb.right = ceil(px[i]) + 1;
					if (floor(py[i]) < aabb.top) aabb.top = floor(py[i]);
					if (ceil(py[i]) > aabb.bottom - 1) aabb.bottom = ceil(py[i]) + 1;
				}
				auto ext = int(ceil(width / 2.0));
				aabb.left -= ext; aabb.top -= ext; aabb.right += ext; aabb.bottom += ext;
				auto fxbrush = brush->GetBrushType() == BrushType::Inversion || brush->GetBrushType() == BrushType::Blur;
				auto bitmap = _parent_factory->CreateBitmap(_main_destination->GetWidth(), _main_destination->GetHeight(), 0);
				auto subctx = _parent_factory->CreateBitmapContext(bitmap);
				auto subbrs = subctx->CreateSolidColorBrush(Color(0, 0, 0));
				subctx->BeginRendering(TextureLoadAction::Load, 0);
				subctx->RenderPolyline(px, py, count, closed, subbrs, width);
				subctx->EndRendering();
				auto bitmap_brush = CreateBitmapBrush(bitmap, Rectangle(0, 0, bitmap->GetWidth(), bitmap->GetHeight()));
				auto layer = CreateLayerBackingStorage();
				_internal_begin_layer_alpha(layer, aabb, fxbrush);
				Render(bitmap_brush, Rectangle(0, 0, bitmap->GetWidth(), bitmap->GetHeight()));
				_internal_begin_layer(layer, aabb, 1.0, fxbrush);
				Render(brush, aabb);
				EndLayer(layer);
			}
			virtual void RenderPolygon(const double * px, const double * py, uint count, IBrush * brush) noexcept override
			{
				if (count < 2 || !brush) return;
				Rectangle aabb(floor(px[0]), floor(py[0]), ceil(px[0]) + 1, ceil(py[0]) + 1);
				for (uint i = 1; i < count; i++) {
					if (floor(px[i]) < aabb.left) aabb.left = floor(px[i]);
					if (ceil(px[i]) > aabb.right - 1) aabb.right = ceil(px[i]) + 1;
					if (floor(py[i]) < aabb.top) aabb.top = floor(py[i]);
					if (ceil(py[i]) > aabb.bottom - 1) aabb.bottom = ceil(py[i]) + 1;
				}
				auto fxbrush = brush->GetBrushType() == BrushType::Inversion || brush->GetBrushType() == BrushType::Blur;
				auto bitmap = _parent_factory->CreateBitmap(_main_destination->GetWidth(), _main_destination->GetHeight(), 0);
				auto subctx = _parent_factory->CreateBitmapContext(bitmap);
				auto subbrs = subctx->CreateSolidColorBrush(Color(0, 0, 0));
				subctx->BeginRendering(TextureLoadAction::Load, 0);
				subctx->RenderPolygon(px, py, count, subbrs);
				subctx->EndRendering();
				auto bitmap_brush = CreateBitmapBrush(bitmap, Rectangle(0, 0, bitmap->GetWidth(), bitmap->GetHeight()));
				auto layer = CreateLayerBackingStorage();
				_internal_begin_layer_alpha(layer, aabb, fxbrush);
				Render(bitmap_brush, Rectangle(0, 0, bitmap->GetWidth(), bitmap->GetHeight()));
				_internal_begin_layer(layer, aabb, 1.0, fxbrush);
				Render(brush, aabb);
				EndLayer(layer);
			}
			virtual void RenderGlyphRun(IGlyphRun * run, const Index2 & at) noexcept override
			{
				if (!run) return;
				auto r = static_cast<VKGlyphRun *>(run);
				auto & clipping = _clipboxes.GetLast()->GetValue();
				auto new_view = Rectangle::Intersect(Rectangle(clipping.left - at.x, clipping.top - at.y, clipping.right - at.x, clipping.bottom - at.y), r->_aabb);
				if (!r->_surface || Rectangle::Intersect(new_view, r->_current_view) != new_view) {
					if (new_view.right - new_view.left <= r->_width) r->_current_view.left = r->_aabb.left;
					else r->_current_view.left = new_view.left;
					if (new_view.bottom - new_view.top <= r->_height) r->_current_view.top = r->_aabb.top;
					else r->_current_view.top = new_view.top;
					r->_current_view.right = r->_current_view.left + r->_width;
					r->_current_view.bottom = r->_current_view.top + r->_height;
					if (!_heap) {
						_heap = owrap(new (std::nothrow) VKTextureHeap(_parent_factory, _parent_device, _queue));
						if (!_heap) return;
					}
					oref<Graphica::IDeviceContext2D> subctx;
					uint x, y;
					r->_surface = _heap->Allocate(r->_width, r->_height, _main_destination->GetWidth(), _main_destination->GetHeight(), subctx, x, y);
					if (!r->_surface) return;
					subctx->RenderGlyphRun(r->_inner, Index2(x - r->_current_view.left, y - r->_current_view.top));
					if (!r->_surface->EndPopulate(subctx)) r->_surface.Clear();
				}
				if (r->_surface && new_view.right > new_view.left && new_view.bottom > new_view.top) {
					auto api = _queue->GetAPI();
					_state->UpdatePipeline(_common->tile_state);
					VKTileDesc draw;
					draw.at = draw.tref = Rectangle(r->_current_view.left + at.x, r->_current_view.top + at.y, r->_current_view.right + at.x, r->_current_view.bottom + at.y);
					draw.irect = Rectangle(r->_surface->_x, r->_surface->_y, r->_surface->_x + r->_surface->_width, r->_surface->_y + r->_surface->_height);
					if (!_update_selector_constant(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_constant, VKSelectorState2D::selector_constant_local, &draw, sizeof(draw))) return;
					_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_vertex | VKPipelineLayout::selector_mapping_esse_type_buffer, VKSelectorState2D::selector_vertex_buffer, _common->area);
					_state->UpdateSelector(VKPipelineLayout::selector_mapping_esse_stage_pixel | VKPipelineLayout::selector_mapping_esse_type_texture, VKSelectorState2D::selector_color_surface, r->_surface->_surface);
					if (!_state->PushState(_current_pass->buffer, _current_pass->retain, _current_pass->stub_sampler))return;
					api->Dispatch.vkCmdDraw(_current_pass->buffer, 6, 1, 0, 0);
				}
			}
			virtual bool BeginRendering(TextureLoadAction load, const Color & clear_color) noexcept override { return false; }
			virtual bool EndRendering(void) noexcept override { return false; }
			bool BeginPass(const RenderTargetViewDesc & rtv, VKPass * pass) noexcept
			{
				_current_destination = _main_destination = rtv.Texture;
				_current_pass = pass;
				VKViewportDesc viewport;
				viewport.offset_x = 0; viewport.offset_y = 0;
				viewport.width = rtv.Texture->GetWidth(); viewport.height = rtv.Texture->GetHeight();
				try {
					_state = owrap(new VKSelectorState2D);
					_clipboxes.Clear();
					_clipboxes.Push(Rectangle(0, 0, rtv.Texture->GetWidth(), rtv.Texture->GetHeight()));
					_viewports.Clear();
					_viewports.Push(viewport);
				} catch (...) { return false; }
				if (_heap) _heap->DefragmentIfNeeded(_current_pass);
				if (!_resume_rendering(rtv.LoadAction, rtv.ClearValue)) {
					_current_pass = 0;
					_current_destination.Clear();
					_main_destination.Clear();
				}
				_current_pass->mode = 2;
				return true;
			}
			bool EndPass(void) noexcept
			{
				_stop_rendering();
				_state.Clear();
				_current_destination.Clear();
				_main_destination.Clear();
				_current_pass = 0;
				if (_heap) _heap->SynchronizeIfNeeded();
				return true;
			}
		};

		#ifdef ESSE_VULKAN_PRESENTATION
		class VKSurface : public Object
		{
		public:
			virtual VkSurfaceKHR GetSurface(void) noexcept = 0;
			virtual Windows::IWindow * GetWindow(void) noexcept = 0;
			virtual bool EnableEDR(void) noexcept = 0;
			virtual bool GetFullscreenState(void) noexcept = 0;
			virtual void SetFullscreenState(bool set) noexcept = 0;
			virtual void Invalidate(void) noexcept = 0;
		};
		class VKSurfaceClass : public Windows::IWindowExtensionClass
		{
		public:
			virtual bool ExtensionAttached(Windows::IWindow * window, Object * extension) noexcept override { return true; }
			virtual void ExtensionDetached(Windows::IWindow * window, Object * extension) noexcept override { static_cast<VKSurface *>(extension)->Invalidate(); }
		};
		class VKSwapChain : public Object
		{
			oref<VKDeviceAPI> _api;
			oref<VKSurface> _surface;
			array<VkImage> _images;
			array<VkImageLayout> _image_layouts;
			array<VkSemaphore> _sync;
			VkSwapchainKHR _swapchain;
			VkFence _fence;
			uint _width, _height;
			bool _allocated;
		public:
			VKSwapChain(VKDeviceAPI * api, VKSurface * surface) : _api(api), _surface(surface), _images(1), _image_layouts(1), _sync(1), _swapchain(0), _fence(0), _allocated(false) {}
			virtual ~VKSwapChain(void) override
			{
				for (auto & s : _sync) if (s) _api->Dispatch.vkDestroySemaphore(_api->Device, s, &_api->Base->Allocator);
				if (_swapchain) _api->Dispatch.vkDestroySwapchainKHR(_api->Device, _swapchain, &_api->Base->Allocator);
				if (_fence) _api->Dispatch.vkDestroyFence(_api->Device, _fence, &_api->Base->Allocator);
			}
			bool Initialize(VKSwapChain * previous, VkPhysicalDevice device, VkFormat format, uint window_style, uint desired_attribute, uint & effective_attribute) noexcept
			{
				VkFenceCreateInfo fence_info;
				fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fence_info.pNext = 0;
				fence_info.flags = 0;
				if (_api->Dispatch.vkCreateFence(_api->Device, &fence_info, &_api->Base->Allocator, &_fence) != VK_SUCCESS) return false;
				VkSurfaceCapabilitiesKHR capabilities;
				array<VkSurfaceFormatKHR> formats(1);
				array<VkPresentModeKHR> modes(1);
				auto surface = _surface->GetSurface();
				try {
					uint count;
					if (_api->Dispatch.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities) < 0) return false;
					if (_api->Dispatch.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, 0) < 0) return false;
					formats.SetLength(count);
					if (_api->Dispatch.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats) < 0) return false;
					if (count < formats.GetLength()) formats.SetLength(count);
					if (_api->Dispatch.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, 0) < 0) return false;
					modes.SetLength(count);
					if (_api->Dispatch.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, modes) < 0) return false;
					if (count < modes.GetLength()) modes.SetLength(count);
					SortArray(modes);
				} catch (...) { return false; }
				bool window_needs_alpha = (window_style & (Windows::WindowStyleTransparent | Windows::WindowStyleSetBlurBehind)) != 0;
				if (!formats.GetLength() || !modes.GetLength()) return false;
				if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) return false;
				bool ffound = false;
				VkSwapchainCreateInfoKHR swapchain_info;
				swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
				swapchain_info.pNext = 0;
				swapchain_info.flags = 0;
				swapchain_info.surface = surface;
				swapchain_info.minImageCount = 2;
				if (capabilities.maxImageCount && swapchain_info.minImageCount > capabilities.maxImageCount) swapchain_info.minImageCount = capabilities.maxImageCount;
				if (capabilities.minImageCount > swapchain_info.minImageCount) swapchain_info.minImageCount = capabilities.minImageCount;
				for (auto & f : formats) if (f.format == format) {
					swapchain_info.imageFormat = f.format;
					swapchain_info.imageColorSpace = f.colorSpace;
					ffound = true; break;
				}
				if (!ffound) return false;
				swapchain_info.imageExtent = capabilities.currentExtent;
				swapchain_info.imageArrayLayers = 1;
				swapchain_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				swapchain_info.queueFamilyIndexCount = 0;
				swapchain_info.pQueueFamilyIndices = 0;
				swapchain_info.preTransform = capabilities.currentTransform;
				effective_attribute = 0;
				if (desired_attribute & WindowLayerAttributeAlphaChannelIgnore) {
					if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
						swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
						effective_attribute |= WindowLayerAttributeAlphaChannelIgnore;
					} else return false;
				} else if (desired_attribute & WindowLayerAttributeAlphaChannelPremultiplied) {
					if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
						swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
						effective_attribute |= WindowLayerAttributeAlphaChannelPremultiplied;
					} else return false;
				} else if (desired_attribute & WindowLayerAttributeAlphaChannelStraight) {
					if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
						swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
						effective_attribute |= WindowLayerAttributeAlphaChannelStraight;
					} else return false;
				} else {
					if (window_needs_alpha) {
						if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
							swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
							effective_attribute |= WindowLayerAttributeAlphaChannelPremultiplied;
						} else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
							swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
						} else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
							swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
							effective_attribute |= WindowLayerAttributeAlphaChannelIgnore;
						} else return false;
					} else {
						if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
							swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
							effective_attribute |= WindowLayerAttributeAlphaChannelIgnore;
						} else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
							swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
						} else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
							swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
							effective_attribute |= WindowLayerAttributeAlphaChannelPremultiplied;
						} else return false;
					}
				}
				bool supports_present_immediate = false, supports_present_sequential = false, supports_present_synchronous = false;
				for (auto & m : modes) {
					if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) supports_present_immediate = true;
					else if (m == VK_PRESENT_MODE_FIFO_KHR) supports_present_sequential = true;
					else if (m == VK_PRESENT_MODE_MAILBOX_KHR) supports_present_synchronous = true;
				}
				uint present_desired = desired_attribute & WindowLayerAttributePresentModeMask;
				if (present_desired == WindowLayerAttributePresentModeImmediate) {
					if (supports_present_immediate) {
						swapchain_info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
						effective_attribute |= WindowLayerAttributePresentModeImmediate;
					} else return false;
				} else if (present_desired == WindowLayerAttributePresentModeSequential) {
					if (supports_present_sequential) {
						swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
						effective_attribute |= WindowLayerAttributePresentModeSequential;
					} else return false;
				} else if (present_desired == WindowLayerAttributePresentModeSynchronous) {
					if (supports_present_synchronous) {
						swapchain_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
						effective_attribute |= WindowLayerAttributePresentModeSynchronous;
					} else return false;
				} else {
					if (supports_present_synchronous) {
						swapchain_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
						effective_attribute |= WindowLayerAttributePresentModeSynchronous;
					} else if (supports_present_sequential) {
						swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
						effective_attribute |= WindowLayerAttributePresentModeSequential;
					} else swapchain_info.presentMode = modes[0];
				}
				swapchain_info.clipped = VK_TRUE;
				swapchain_info.oldSwapchain = previous ? previous->_swapchain : 0;
				if (_api->Dispatch.vkCreateSwapchainKHR(_api->Device, &swapchain_info, &_api->Base->Allocator, &_swapchain) != VK_SUCCESS) return false;
				_width = swapchain_info.imageExtent.width;
				_height = swapchain_info.imageExtent.height;
				try {
					uint32 count;
					if (_api->Dispatch.vkGetSwapchainImagesKHR(_api->Device, _swapchain, &count, 0) < 0) return false;
					_images.SetLength(count);
					if (_api->Dispatch.vkGetSwapchainImagesKHR(_api->Device, _swapchain, &count, _images) < 0) return false;
					if (count < _images.GetLength()) _images.SetLength(count);
					_image_layouts.SetLength(_images.GetLength());
					_sync.SetLength(_images.GetLength());
					for (auto & l : _image_layouts) l = VK_IMAGE_LAYOUT_UNDEFINED;
					for (auto & s : _sync) s = 0;
					for (auto & s : _sync) {
						VkSemaphoreCreateInfo sem;
						sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
						sem.pNext = 0;
						sem.flags = 0;
						if (_api->Dispatch.vkCreateSemaphore(_api->Device, &sem, &_api->Base->Allocator, &s) != VK_SUCCESS) { s = 0; return false; }
					}
				} catch (...) { return false; }
				return true;
			}
			VkSwapchainKHR & GetSwapchain(void) noexcept { return _swapchain; }
			VkResult AcquireNextImage(uint & index) noexcept { return _api->Dispatch.vkAcquireNextImageKHR(_api->Device, _swapchain, 1000000000UL, 0, _fence, &index); }
			void QueryImage(uint index, VkImage & image, VkImageLayout *& layout, VkSemaphore & sync) noexcept { image = _images[index]; layout = &_image_layouts[index]; sync = _sync[index]; }
			bool ResetFence(void) noexcept { if (_api->Dispatch.vkResetFences(_api->Device, 1, &_fence) < 0) return false; else return true; }
			bool WaitFence(void) noexcept { return _api->Dispatch.vkWaitForFences(_api->Device, 1, &_fence, VK_TRUE, 1000000000UL) == VK_SUCCESS; }
			bool IsAllocated(void) noexcept { return _allocated; }
			void SetAllocated(bool allocated) noexcept { _allocated = allocated; }
			uint GetWidth(void) noexcept { return _width; }
			uint GetHeight(void) noexcept { return _height; }
		};
		class VKPresentationLayer : public IPresentationLayer
		{
			oref<VKQueue> _queue;
			Graphica::IDevice * _parent;
			uint _window_style;
			oref<VKSurface> _surface;
			oref<VKSwapChain> _swapchain;
			oref<VKTexture> _intermediate;
			uint _usage, _desired_attributes, _effective_attributes, _size_desync_counter;
			PixelFormat _esse_format;
			VkPhysicalDevice _device;
			VkFormat _vk_format;
		private:
			bool _reallocate_swapchain(void) noexcept
			{
				auto swapchain = owrap(new (std::nothrow) VKSwapChain(_queue->GetAPI(), _surface));
				if (!swapchain || !swapchain->Initialize(_swapchain, _device, _vk_format, _window_style, _desired_attributes, _effective_attributes)) return false;
				if (_desired_attributes & WindowLayerAttributeExtendedDynamicRange) {
					if (!_surface->EnableEDR()) return false;
					_effective_attributes |= WindowLayerAttributeExtendedDynamicRange;
				}
				_swapchain = swapchain;
				return true;
			}
		public:
			VKPresentationLayer(VKQueue * queue, IDevice * parent_device, VkPhysicalDevice physical) : _queue(queue), _parent(parent_device), _device(physical), _size_desync_counter(0) {}
			virtual ~VKPresentationLayer(void) override { if (_swapchain) _queue->RemoveSwapChain(this); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKPresentationLayer"; ESSE_TRY_OUTRO(string()) }
			virtual IDevice * GetParentDevice(void) noexcept override { return _parent; }
			virtual bool Present(void) noexcept override
			{
				uint num_retr = 0;
				VkImage image;
				VkImageLayout * layout;
				VkSemaphore sync;
				uint image_index;
				while (true) {
					num_retr++;
					if (_swapchain->IsAllocated() || !_swapchain->ResetFence()) return false;
					auto status = _swapchain->AcquireNextImage(image_index);
					if (status < 0 || status == VK_TIMEOUT) return false;
					if (status == VK_SUBOPTIMAL_KHR) {
						if (num_retr > 3) return false;
						_queue->RemoveSwapChain(this);
						if (!_reallocate_swapchain()) return false;
						continue;
					}
					_swapchain->SetAllocated(true);
					_swapchain->QueryImage(image_index, image, layout, sync);
					if (!_swapchain->WaitFence()) return false;
					break;
				}
				_queue->CommitSwapChain(this, _swapchain);
				_parent->GetPrimaryDeviceContext()->Flush();
				auto window = _surface->GetWindow();
				auto size = window ? window->GetClientSize() : Index2(0, 0);
				auto pass = _queue->CreatePass();
				if (!pass) return false;
				auto api = _queue->GetAPI();
				try { pass->retain.AddElement(_intermediate.Inner()); pass->retain.AddElement(_swapchain.Inner()); } catch (...) {}
				VkImageMemoryBarrier barrier[2];
				barrier[0].sType = barrier[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier[0].pNext = barrier[1].pNext = 0;
				barrier[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
				barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				barrier[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier[0].oldLayout = _intermediate->_current_layout;
				barrier[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier[1].oldLayout = *layout;
				barrier[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue->GetQueueIndex();
				barrier[1].srcQueueFamilyIndex = barrier[1].dstQueueFamilyIndex = _queue->GetQueueIndex();
				barrier[0].image = _intermediate->_image;
				barrier[1].image = image;
				barrier[0].subresourceRange.aspectMask = barrier[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier[0].subresourceRange.baseArrayLayer = barrier[0].subresourceRange.baseMipLevel = 0;
				barrier[1].subresourceRange.baseArrayLayer = barrier[1].subresourceRange.baseMipLevel = 0;
				barrier[0].subresourceRange.layerCount = barrier[0].subresourceRange.levelCount = 1;
				barrier[1].subresourceRange.layerCount = barrier[1].subresourceRange.levelCount = 1;
				api->Dispatch.vkCmdPipelineBarrier(pass->buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 2, barrier);
				_intermediate->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
				VkImageBlit blt;
				blt.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blt.srcSubresource.baseArrayLayer = 0;
				blt.srcSubresource.layerCount = 1;
				blt.srcSubresource.mipLevel = 0;
				blt.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blt.dstSubresource.baseArrayLayer = 0;
				blt.dstSubresource.layerCount = 1;
				blt.dstSubresource.mipLevel = 0;
				blt.srcOffsets[0].x = blt.srcOffsets[0].y = blt.srcOffsets[0].z = blt.dstOffsets[0].x = blt.dstOffsets[0].y = blt.dstOffsets[0].z = 0;
				blt.srcOffsets[1].x = _intermediate->_desc.Width;
				blt.srcOffsets[1].y = _intermediate->_desc.Height;
				blt.srcOffsets[1].z = 1;
				blt.dstOffsets[1].x = window ? min<uint>(_swapchain->GetWidth(), size.x) : _swapchain->GetWidth();
				blt.dstOffsets[1].y = window ? min<uint>(_swapchain->GetHeight(), size.y) : _swapchain->GetHeight();
				blt.dstOffsets[1].z = 1;
				api->Dispatch.vkCmdBlitImage(pass->buffer, _intermediate->_image, VK_IMAGE_LAYOUT_GENERAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blt, VK_FILTER_LINEAR);
				barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier[0].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				barrier[0].srcQueueFamilyIndex = barrier[0].dstQueueFamilyIndex = _queue->GetQueueIndex();
				barrier[0].image = image;
				barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier[0].subresourceRange.baseArrayLayer = barrier[0].subresourceRange.baseMipLevel = 0;
				barrier[0].subresourceRange.layerCount = barrier[0].subresourceRange.levelCount = 1;
				api->Dispatch.vkCmdPipelineBarrier(pass->buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1, barrier);
				*layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				if (!_queue->SubmitPass(pass, sync)) return false;
				VkPresentInfoKHR present;
				present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				present.pNext = 0;
				present.waitSemaphoreCount = 1;
				present.pWaitSemaphores = &sync;
				present.swapchainCount = 1;
				present.pSwapchains = &_swapchain->GetSwapchain();
				present.pImageIndices = &image_index;
				present.pResults = 0;
				auto status = api->Dispatch.vkQueuePresentKHR(_queue->GetQueue(), &present);
				_swapchain->SetAllocated(false);
				if (window && (_swapchain->GetWidth() != size.x || _swapchain->GetHeight() != size.y)) {
					_size_desync_counter++;
					auto ws = Windows::GetWindowSystem();
					if (ws && _size_desync_counter <= 5) try { oref<Windows::IWindow> wnd = window; ws->SubmitTask(CreateFunctionalTask([wnd]() { wnd->Invalidate(); })); } catch (...) {}
				} else _size_desync_counter = 0;
				return (status >= 0);
			}
			virtual oref<ITexture> QuerySurface(void) noexcept override { return _intermediate.Inner(); }
			virtual bool ResizeSurface(uint32 width, uint32 height) noexcept override
			{
				auto w = max(width, 1U), h = max(height, 1U);
				if (_intermediate->GetWidth() == w && _intermediate->GetHeight() == h) return true;
				TextureDesc desc;
				desc.Type = TextureType::Type2D;
				desc.Format = _esse_format;
				desc.Width = w;
				desc.Height = h;
				desc.MipmapCount = 1;
				desc.Usage = _usage;
				desc.MemoryPool = ResourceMemoryPool::Regular;
				auto texture = _parent->CreateTexture(desc);
				if (!texture) return false;
				_intermediate = static_cast<VKTexture *>(texture.Inner());
				return true;
			}
			virtual bool SwitchToFullscreen(void) noexcept override { _surface->SetFullscreenState(true); return _surface->GetFullscreenState(); }
			virtual bool SwitchToWindow(void) noexcept override { _surface->SetFullscreenState(false); return !_surface->GetFullscreenState(); }
			virtual bool IsFullscreen(void) noexcept override { return _surface->GetFullscreenState(); }
			virtual uint GetLayerAttributes(void) noexcept override { return _effective_attributes; }
			bool Initialize(Windows::IWindow * window, VKSurface * surface, const PresentationLayerDesc & desc) noexcept
			{
				_window_style = window->GetEffectiveStyle(Windows::CreateWindowDescType::CreateWindowDesc);
				_surface = surface;
				_usage = desc.Usage & ~WindowLayerAttributeMask;
				_desired_attributes = desc.Usage & WindowLayerAttributeMask;
				_esse_format = desc.Format;
				_vk_format = CreateVkFormat(_esse_format);
				if (_vk_format == VK_FORMAT_UNDEFINED) return false;
				TextureDesc tdesc;
				tdesc.Type = TextureType::Type2D;
				tdesc.Format = desc.Format;
				tdesc.Width = max(desc.Width, 1U);
				tdesc.Height = max(desc.Height, 1U);
				tdesc.MipmapCount = 1;
				tdesc.Usage = desc.Usage;
				tdesc.MemoryPool = ResourceMemoryPool::Regular;
				auto texture = _parent->CreateTexture(tdesc);
				if (!texture) return false;
				_intermediate = static_cast<VKTexture *>(texture.Inner());
				if (!_reallocate_swapchain()) return false;
				return true;
			}
		};
		#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
		class VKX11Surface : public VKSurface
		{
			oref<VKDeviceAPI> _api;
			VkSurfaceKHR _surface;
			Windows::IWindow * _window;
			X11::IX11Window * _window_x11;
		public:
			VKX11Surface(VKDeviceAPI * api) : _api(api), _surface(0), _window(0), _window_x11(0) {}
			virtual ~VKX11Surface(void) override { if (_surface) _api->Base->Dispatch.vkDestroySurfaceKHR(_api->Base->Instance, _surface, &_api->Base->Allocator); }
			virtual VkSurfaceKHR GetSurface(void) noexcept override { return _surface; }
			virtual Windows::IWindow * GetWindow(void) noexcept override { return _window; }
			virtual bool EnableEDR(void) noexcept override { return false; }
			virtual bool GetFullscreenState(void) noexcept override { return _window_x11 ? _window_x11->GetFullscreenState() : false; }
			virtual void SetFullscreenState(bool set) noexcept override { if (_window_x11) _window_x11->SetFullscreenState(set); }
			virtual void Invalidate(void) noexcept override { _window = 0; _window_x11 = 0; }
			bool Initialize(VkPhysicalDevice device, VKQueue * queue, Windows::IWindow * window, X11::IX11Window * window_x11) noexcept
			{
				if (!_api->Dispatch.vkCreateXlibSurfaceKHR || !_api->Dispatch.vkCreateSwapchainKHR || !_api->Dispatch.vkGetPhysicalDeviceXlibPresentationSupportKHR) return false;
				ErrorContext ectx; ErrorClear(ectx);
				auto ws_x11 = reinterpret_cast<X11::IX11WindowSystem *>(window_x11->GetWindowSystem()->DynamicCast(Linux::Classes::X11_WindowSystem, ectx));
				if (ErrorTest(ectx)) return false;
				if (!_api->Dispatch.vkGetPhysicalDeviceXlibPresentationSupportKHR(device, queue->GetQueueIndex(), ws_x11->GetConnection()->GetXDisplay(), window_x11->GetVisual()->visualid)) return false;
				VkXlibSurfaceCreateInfoKHR info;
				info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
				info.pNext = 0;
				info.flags = 0;
				info.dpy = ws_x11->GetConnection()->GetXDisplay();
				info.window = window_x11->GetHandle();
				if (_api->Dispatch.vkCreateXlibSurfaceKHR(_api->Base->Instance, &info, &_api->Base->Allocator, &_surface) != VK_SUCCESS) return 0;
				_window = window;
				_window_x11 = window_x11;
				return true;
			}
		};
		#endif
		#endif

		class VKDeviceDeferredContext : public IDeviceContext
		{
			friend class VKDeviceImmediateContext;
			friend class VKDevice;
		private:
			IDevice * _parent_device;
			oref<VKQueue> _queue;
			oref<VKDeviceContext2D> _plain_context;
			oref<VKPass> _pass;
		private:
			bool _pass_create(void) noexcept
			{
				if (_pass) return false;
				_pass = _queue->CreatePass();
				if (!_pass) return false;
				return true;
			}
		public:
			VKDeviceDeferredContext(IDevice * parent_device, VKQueue * queue) : _parent_device(parent_device), _queue(queue) {}
			virtual ~VKDeviceDeferredContext(void) override { if (_plain_context) { _plain_context->_parent_context = 0; _plain_context->_parent_device = 0; _plain_context->_current_pass = 0; } }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKDeviceDeferredContext"; ESSE_TRY_OUTRO(string()) }
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept override
			{
				if (cls == Classes.Object || cls == Classes.DynamicObject || cls == Classes.IDeviceContext) {
					Retain(); return this;
				} else if (cls == Classes.IDevice) {
					_parent_device->Retain(); return _parent_device;
				} else if (cls == Classes.IDeviceContext2D) {
					if (!_plain_context) { ErrorSet(ectx, Errores::ErrorInvalidState); return 0; }
					_plain_context->Retain(); return _plain_context;
				} else { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			}
			virtual const void * GetType(void) noexcept override { return Classes.IDeviceContext; }
			virtual bool BeginRenderingPass(uint32 rtc, const RenderTargetViewDesc * rtv, const DepthStencilViewDesc * dsv) noexcept override
			{
				if (!rtc || rtc > 8 || !rtv || !_queue->_api->Dispatch.vkCmdBeginRenderingKHR) return false;
				for (uint i = 0; i < rtc; i++) if (!rtv->Texture) return false;
				if (_pass && _pass->mode) return false;
				if (!_pass && !_pass_create()) return false;
				return _pass->BeginRenderingPass(rtc, rtv, dsv);
			}
			virtual bool BeginRenderingPass2D(const RenderTargetViewDesc & rtv) noexcept override
			{
				if (!rtv.Texture || !(rtv.Texture->GetResourceUsage() & ResourceUsageRenderTarget) || rtv.Texture->GetPixelFormat() != PixelFormat::B8G8R8A8_unorm || rtv.Texture->GetTextureType() != TextureType::Type2D) return false;
				if (_pass && _pass->mode) return false;
				if (!_pass && !_pass_create()) return false;
				if (!_plain_context) {
					try { _plain_context = owrap(new VKDeviceContext2D(_queue, _parent_device, this)); } catch (...) {}
					if (!_plain_context) return false;
				}
				return _plain_context->BeginPass(rtv, _pass);
			}
			virtual bool BeginMemoryManagementPass(void) noexcept override
			{
				if (_pass && _pass->mode) return false;
				if (!_pass && !_pass_create()) return false;
				_pass->mode = 3;
				return true;
			}
			virtual bool EndCurrentPass(void) noexcept override
			{
				if (_pass && _pass->mode) {
					bool status = true;
					if (_pass->mode == 1) {
						_pass->EndRenderingPass();
					} else if (_pass->mode == 2) {
						status = _plain_context->EndPass();
					}
					_pass->mode = 0;
					return status;
				} else return false;
			}
			virtual bool SubmitDeferredContext(IDeviceContext * context) noexcept override { return false; }
			virtual void Flush(void) noexcept override {}
			virtual bool IsDeferred(void) noexcept override { return true; }
			virtual void SetRenderingPipelineState(IPipelineState * state) noexcept override
			{
				if (!state || !_pass || _pass->mode != 1) return;
				_pass->SetRenderingPipelineState(state);
			}
			virtual void SetViewport(float top_left_x, float top_left_y, float width, float height, float min_depth, float max_depth) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->SetViewport(top_left_x, top_left_y, width, height, min_depth, max_depth);
			}
			virtual void SetVertexShaderResource(uint32 at, IDeviceResource * resource) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->SetVertexShaderResource(at, resource);
			}
			virtual void SetVertexShaderConstant(uint32 at, IBuffer * buffer) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->SetVertexShaderConstant(at, buffer);
			}
			virtual void SetVertexShaderConstantImmediate(uint32 at, const void * data, uint length) noexcept override
			{
				if (!_pass || _pass->mode != 1 || !data || length <= 0) return;
				_pass->SetVertexShaderConstant(at, data, length);
			}
			virtual void SetVertexShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->SetVertexShaderSamplerState(at, sampler);
			}
			virtual void SetPixelShaderResource(uint32 at, IDeviceResource * resource) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->SetPixelShaderResource(at, resource);
			}
			virtual void SetPixelShaderConstant(uint32 at, IBuffer * buffer) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->SetPixelShaderConstant(at, buffer);
			}
			virtual void SetPixelShaderConstantImmediate(uint32 at, const void * data, uint length) noexcept override
			{
				if (!_pass || _pass->mode != 1 || !data || length <= 0) return;
				_pass->SetPixelShaderConstant(at, data, length);
			}
			virtual void SetPixelShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->SetPixelShaderSamplerState(at, sampler);
			}
			virtual void SetIndexBuffer(IBuffer * index, IndexBufferFormat format) noexcept override
			{
				if (!index || !_pass || _pass->mode != 1) return;
				_pass->SetIndexBuffer(index, format);
			}
			virtual void SetStencilReferenceValue(uint8 ref) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->SetStencilReferenceValue(ref);
			}
			virtual void DrawPrimitives(uint32 vertex_count, uint32 first_vertex) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->DrawPrimitives(vertex_count, first_vertex);
			}
			virtual void DrawInstancedPrimitives(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->DrawInstancedPrimitives(vertex_count, first_vertex, instance_count, first_instance);
			}
			virtual void DrawIndexedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->DrawIndexedPrimitives(index_count, first_index, base_vertex);
			}
			virtual void DrawIndexedInstancedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex, uint32 instance_count, uint32 first_instance) noexcept override
			{
				if (!_pass || _pass->mode != 1) return;
				_pass->DrawIndexedInstancedPrimitives(index_count, first_index, base_vertex, instance_count, first_instance);
			}
			virtual void GenerateMipmaps(ITexture * texture) noexcept override
			{
				if (!texture || !_pass || _pass->mode != 3) return;
				_pass->GenerateMipmaps(texture);
			}
			virtual void CopyResourceData(IDeviceResource * dest, IDeviceResource * src) noexcept override
			{
				if (!dest || !src || !_pass || _pass->mode != 3) return;
				_pass->CopyResourceData(dest, src);
			}
			virtual void CopySubresourceData(IDeviceResource * dest, const Index2 & dest_subres, const Index3 & dest_origin, IDeviceResource * src, const Index2 & src_subres, const Index3 & src_origin, const Index3 & size) noexcept override
			{
				if (!dest || !src || !_pass || _pass->mode != 3) return;
				_pass->CopySubresourceData(dest, dest_subres, dest_origin, src, src_subres, src_origin, size);
			}
			virtual void UpdateResourceData(IDeviceResource * dest, const Index2 & subres, const Index3 & origin, const Index3 & size, const ResourceInitDesc & src) noexcept override {}
			virtual void QueryResourceData(const ResourceDataDesc & dest, IDeviceResource * src, const Index2 & subres, const Index3 & origin, const Index3 & size) noexcept override {}
			virtual bool AcquireSharedResource(IDeviceResource * rsrc) noexcept override { return false; }
			virtual bool TryAcquireSharedResource(IDeviceResource * rsrc, uint32 timeout) noexcept override { return false; }
			virtual bool ReleaseSharedResource(IDeviceResource * rsrc) noexcept override { return false; }
			void PrecreateContext2D(void) noexcept { if (!_plain_context) try { _plain_context = owrap(new VKDeviceContext2D(_queue, _parent_device, this)); } catch (...) {} }
		};
		class VKDeviceImmediateContext : public IDeviceContext
		{
			friend class VKDevice;
		private:
			IDevice * _parent_device;
			oref<VKQueue> _queue;
			oref<VKDeviceContext2D> _plain_context;
			oref<VKPass> _current_pass;
		private:
			void _current_pass_cancel(void) noexcept { _current_pass.Clear(); }
			bool _current_pass_create(void) noexcept
			{
				if (_current_pass) return false;
				_current_pass = _queue->CreatePass();
				if (!_current_pass) return false;
				return true;
			}
			bool _current_pass_submit(void) noexcept
			{
				if (!_current_pass) return false;
				auto status = _queue->SubmitPass(_current_pass);
				_current_pass.Clear();
				return status;
			}
		public:
			VKDeviceImmediateContext(IDevice * parent_device, VKQueue * queue) : _parent_device(parent_device), _queue(queue) {}
			virtual ~VKDeviceImmediateContext(void) override { if (_plain_context) { _plain_context->_parent_context = 0; _plain_context->_parent_device = 0; _plain_context->_current_pass = 0; } }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKDeviceImmediateContext"; ESSE_TRY_OUTRO(string()) }
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept override
			{
				if (cls == Classes.Object || cls == Classes.DynamicObject || cls == Classes.IDeviceContext) {
					Retain(); return this;
				} else if (cls == Classes.IDevice) {
					_parent_device->Retain(); return _parent_device;
				} else if (cls == Classes.IDeviceContext2D) {
					if (!_plain_context) { ErrorSet(ectx, Errores::ErrorInvalidState); return 0; }
					_plain_context->Retain(); return _plain_context;
				} else { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			}
			virtual const void * GetType(void) noexcept override { return Classes.IDeviceContext; }
			virtual bool BeginRenderingPass(uint32 rtc, const RenderTargetViewDesc * rtv, const DepthStencilViewDesc * dsv) noexcept override
			{
				if (!rtc || rtc > 8 || !rtv || !_queue->_api->Dispatch.vkCmdBeginRenderingKHR) return false;
				for (uint i = 0; i < rtc; i++) if (!rtv->Texture) return false;
				if (_current_pass && _current_pass->mode) return false;
				if (!_current_pass && !_current_pass_create()) return false;
				return _current_pass->BeginRenderingPass(rtc, rtv, dsv);
			}
			virtual bool BeginRenderingPass2D(const RenderTargetViewDesc & rtv) noexcept override
			{
				if (!rtv.Texture || !(rtv.Texture->GetResourceUsage() & ResourceUsageRenderTarget) || rtv.Texture->GetPixelFormat() != PixelFormat::B8G8R8A8_unorm || rtv.Texture->GetTextureType() != TextureType::Type2D) return false;
				if (_current_pass && _current_pass->mode) return false;
				if (!_current_pass && !_current_pass_create()) return false;
				if (!_plain_context) {
					try { _plain_context = owrap(new VKDeviceContext2D(_queue, _parent_device, this)); } catch (...) {}
					if (!_plain_context) return false;
				}
				return _plain_context->BeginPass(rtv, _current_pass);
			}
			virtual bool BeginMemoryManagementPass(void) noexcept override
			{
				if (_current_pass && _current_pass->mode) return false;
				if (!_current_pass && !_current_pass_create()) return false;
				_current_pass->mode = 3;
				return true;
			}
			virtual bool EndCurrentPass(void) noexcept override
			{
				if (_current_pass && _current_pass->mode) {
					bool status = true;
					if (_current_pass->mode == 1) {
						_current_pass->EndRenderingPass();
					} else if (_current_pass->mode == 2) {
						status = _plain_context->EndPass();
					}
					_current_pass->mode = 0;
					return status;
				} else return false;
			}
			virtual bool SubmitDeferredContext(IDeviceContext * context) noexcept override
			{
				if (!context || !context->IsDeferred()) return false;
				auto ctx = static_cast<VKDeviceDeferredContext *>(context);
				if (!ctx->_pass || ctx->_pass->mode) return false;
				auto status = _queue->SubmitPass(ctx->_pass);
				ctx->_pass.Clear();
				return status;
			}
			virtual void Flush(void) noexcept override { if (_current_pass && !_current_pass->mode && !_current_pass_submit()) *_queue->_valid = false; }
			virtual bool IsDeferred(void) noexcept override { return false; }
			virtual void SetRenderingPipelineState(IPipelineState * state) noexcept override
			{
				if (!state || !_current_pass || _current_pass->mode != 1) return;
				_current_pass->SetRenderingPipelineState(state);
			}
			virtual void SetViewport(float top_left_x, float top_left_y, float width, float height, float min_depth, float max_depth) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->SetViewport(top_left_x, top_left_y, width, height, min_depth, max_depth);
			}
			virtual void SetVertexShaderResource(uint32 at, IDeviceResource * resource) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->SetVertexShaderResource(at, resource);
			}
			virtual void SetVertexShaderConstant(uint32 at, IBuffer * buffer) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->SetVertexShaderConstant(at, buffer);
			}
			virtual void SetVertexShaderConstantImmediate(uint32 at, const void * data, uint length) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1 || !data || length <= 0) return;
				_current_pass->SetVertexShaderConstant(at, data, length);
			}
			virtual void SetVertexShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->SetVertexShaderSamplerState(at, sampler);
			}
			virtual void SetPixelShaderResource(uint32 at, IDeviceResource * resource) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->SetPixelShaderResource(at, resource);
			}
			virtual void SetPixelShaderConstant(uint32 at, IBuffer * buffer) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->SetPixelShaderConstant(at, buffer);
			}
			virtual void SetPixelShaderConstantImmediate(uint32 at, const void * data, uint length) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1 || !data || length <= 0) return;
				_current_pass->SetPixelShaderConstant(at, data, length);
			}
			virtual void SetPixelShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->SetPixelShaderSamplerState(at, sampler);
			}
			virtual void SetIndexBuffer(IBuffer * index, IndexBufferFormat format) noexcept override
			{
				if (!index || !_current_pass || _current_pass->mode != 1) return;
				_current_pass->SetIndexBuffer(index, format);
			}
			virtual void SetStencilReferenceValue(uint8 ref) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->SetStencilReferenceValue(ref);
			}
			virtual void DrawPrimitives(uint32 vertex_count, uint32 first_vertex) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->DrawPrimitives(vertex_count, first_vertex);
			}
			virtual void DrawInstancedPrimitives(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->DrawInstancedPrimitives(vertex_count, first_vertex, instance_count, first_instance);
			}
			virtual void DrawIndexedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->DrawIndexedPrimitives(index_count, first_index, base_vertex);
			}
			virtual void DrawIndexedInstancedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex, uint32 instance_count, uint32 first_instance) noexcept override
			{
				if (!_current_pass || _current_pass->mode != 1) return;
				_current_pass->DrawIndexedInstancedPrimitives(index_count, first_index, base_vertex, instance_count, first_instance);
			}
			virtual void GenerateMipmaps(ITexture * texture) noexcept override
			{
				if (!texture || !_current_pass || _current_pass->mode != 3) return;
				_current_pass->GenerateMipmaps(texture);
			}
			virtual void CopyResourceData(IDeviceResource * dest, IDeviceResource * src) noexcept override
			{
				if (!dest || !src || !_current_pass || _current_pass->mode != 3) return;
				_current_pass->CopyResourceData(dest, src);
			}
			virtual void CopySubresourceData(IDeviceResource * dest, const Index2 & dest_subres, const Index3 & dest_origin, IDeviceResource * src, const Index2 & src_subres, const Index3 & src_origin, const Index3 & size) noexcept override
			{
				if (!dest || !src || !_current_pass || _current_pass->mode != 3) return;
				_current_pass->CopySubresourceData(dest, dest_subres, dest_origin, src, src_subres, src_origin, size);
			}
			virtual void UpdateResourceData(IDeviceResource * dest, const Index2 & subres, const Index3 & origin, const Index3 & size, const ResourceInitDesc & src) noexcept override
			{
				if (!dest || !_current_pass || _current_pass->mode != 3) return;
				try { _current_pass->retain.AddElement(dest); } catch (...) { return; }
				if (!_queue->InternalUpdateResourceData(_current_pass->buffer, dest, subres, origin, size, src)) _current_pass_cancel();
			}
			virtual void QueryResourceData(const ResourceDataDesc & dest, IDeviceResource * src, const Index2 & subres, const Index3 & origin, const Index3 & size) noexcept override
			{
				if (!src || !_current_pass || _current_pass->mode != 3) return;
				try { _current_pass->retain.AddElement(src); } catch (...) { return; }
				if (!_queue->InternalQueryResourceData(_current_pass->buffer, dest, src, subres, origin, size)) _current_pass_cancel();
			}
			virtual bool AcquireSharedResource(IDeviceResource * rsrc) noexcept override
			{
				if (!rsrc || rsrc->GetMemoryPool() != ResourceMemoryPool::Shared) return false;
				if (!_current_pass && !_current_pass_create()) return false;
				if (_current_pass->mode) return false;
				auto sync = static_cast<VKTexture *>(rsrc)->_shared->_data;
				while (true) {
					auto io = sem_wait(&sync->lock);
					if (io >= 0) break;
					else if (io < 0 && errno != EINTR) return false;
				}
				VkImageMemoryBarrier barrier;
				_current_pass->MakeSharedLayoutTransition(static_cast<VKTexture *>(rsrc), barrier, true);
				_queue->_api->Dispatch.vkCmdPipelineBarrier(_current_pass->buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 1, &barrier);
				return true;
			}
			virtual bool TryAcquireSharedResource(IDeviceResource * rsrc, uint32 timeout) noexcept override
			{
				if (!rsrc || rsrc->GetMemoryPool() != ResourceMemoryPool::Shared) return false;
				if (!_current_pass && !_current_pass_create()) return false;
				if (_current_pass->mode) return false;
				auto sync = static_cast<VKTexture *>(rsrc)->_shared->_data;
				if (timeout) {
					struct timespec date;
					if (clock_gettime(CLOCK_REALTIME, &date) == -1) abort();
					uint64 new_ns = date.tv_nsec + uint64(timeout) * 1000000UL;
					uint64 sec_carry = new_ns / 1000000000UL;
					date.tv_nsec = new_ns % 1000000000UL;
					date.tv_sec += sec_carry;
					while (true) {
						auto io = sem_timedwait(&sync->lock, &date);
						if (io >= 0) break;
						else if (errno != EINTR) return false;
					}
				} else {
					auto io = sem_trywait(&sync->lock);
					if (io < 0) return false;
				}
				VkImageMemoryBarrier barrier;
				_current_pass->MakeSharedLayoutTransition(static_cast<VKTexture *>(rsrc), barrier, true);
				_queue->_api->Dispatch.vkCmdPipelineBarrier(_current_pass->buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 1, &barrier);
				return true;
			}
			virtual bool ReleaseSharedResource(IDeviceResource * rsrc) noexcept override
			{
				if (!rsrc || rsrc->GetMemoryPool() != ResourceMemoryPool::Shared) return false;
				if (!_current_pass && !_current_pass_create()) return false;
				if (_current_pass->mode) return false;
				VkImageMemoryBarrier barrier;
				_current_pass->MakeSharedLayoutTransition(static_cast<VKTexture *>(rsrc), barrier, false);
				_queue->_api->Dispatch.vkCmdPipelineBarrier(_current_pass->buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 1, &barrier);
				if (!_current_pass_submit()) *_queue->_valid = false;
				_queue->_api->Dispatch.vkQueueWaitIdle(_queue->_queue);
				auto sync = static_cast<VKTexture *>(rsrc)->_shared->_data;
				sem_post(&sync->lock);
				return true;
			}
			void Finalize(void) noexcept
			{
				_current_pass_cancel();
				if (_plain_context) _plain_context->_finalize();
			}
			void PrecreateContext2D(void) noexcept { if (!_plain_context) try { _plain_context = owrap(new VKDeviceContext2D(_queue, _parent_device, this)); } catch (...) {} }
		};
		class VKDevice : public IDevice
		{
			friend class VK2DCommonResources;
		private:
			VkPhysicalDevice _physical;
			VkQueue _queue;
			VkCommandPool _pool;
			oref<VKDeviceAPI> _api;
			oref<VKQueue> _dispatcher;
			oref<VKDeviceImmediateContext> _immediate_context;
			oref<VKDeviceStats> _stats;
			oref<VK2DCommonResources> _common;
			ObjectDictionary<string, VKPipelineLayout> _layouts;
			VkPhysicalDeviceMemoryProperties _memory_info;
			int _queue_family_index;
			volatile bool _valid;
			string _name;
		private:
			bool _texture_init_views(VKTexture * texture, bool entire, uint32 mip_level, uint32 array_offset_or_depth) noexcept
			{
				if (IsDepthStencilFormat(texture->_desc.Format)) {
					VkImageViewCreateInfo view;
					view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					view.pNext = 0;
					view.flags = 0;
					view.image = texture->_image;
					view.format = CreateVkFormat(texture->_desc.Format);
					view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
					view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
					view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
					view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
					if (texture->_desc.Type == TextureType::Type1D) {
						view.viewType = VK_IMAGE_VIEW_TYPE_1D;
						view.subresourceRange.baseArrayLayer = 0;
						view.subresourceRange.layerCount = 1;
						if (entire) {
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::TypeArray1D) {
						if (entire) {
							view.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = texture->_desc.ArraySize;
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.viewType = VK_IMAGE_VIEW_TYPE_1D;
							view.subresourceRange.baseArrayLayer = array_offset_or_depth;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::Type2D) {
						view.viewType = VK_IMAGE_VIEW_TYPE_2D;
						view.subresourceRange.baseArrayLayer = 0;
						view.subresourceRange.layerCount = 1;
						if (entire) {
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::TypeArray2D) {
						if (entire) {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = texture->_desc.ArraySize;
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D;
							view.subresourceRange.baseArrayLayer = array_offset_or_depth;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::TypeCube) {
						if (entire) {
							view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = 6;
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D;
							view.subresourceRange.baseArrayLayer = array_offset_or_depth;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::TypeArrayCube) {
						if (entire) {
							view.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = 6 * texture->_desc.ArraySize;
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D;
							view.subresourceRange.baseArrayLayer = array_offset_or_depth;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::Type3D) {
						if (entire) {
							view.viewType = VK_IMAGE_VIEW_TYPE_3D;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D;
							view.subresourceRange.baseArrayLayer = array_offset_or_depth;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else return false;
					if (_api->Dispatch.vkCreateImageView(_api->Device, &view, &_api->Base->Allocator, &texture->_view) != VK_SUCCESS) return false;
					if (GetFormatChannelCount(texture->_desc.Format) > 1) {
						VkImageViewCreateInfo view;
						view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
						view.pNext = 0;
						view.flags = 0;
						view.image = texture->_image;
						view.format = CreateVkFormat(texture->_desc.Format);
						view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
						view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
						view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
						view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
						view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
						if (texture->_desc.Type == TextureType::Type1D) {
							view.viewType = VK_IMAGE_VIEW_TYPE_1D;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = 1;
							if (entire) {
								view.subresourceRange.baseMipLevel = 0;
								view.subresourceRange.levelCount = texture->_desc.MipmapCount;
							} else {
								view.subresourceRange.baseMipLevel = mip_level;
								view.subresourceRange.levelCount = 1;
							}
						} else if (texture->_desc.Type == TextureType::TypeArray1D) {
							if (entire) {
								view.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
								view.subresourceRange.baseArrayLayer = 0;
								view.subresourceRange.layerCount = texture->_desc.ArraySize;
								view.subresourceRange.baseMipLevel = 0;
								view.subresourceRange.levelCount = texture->_desc.MipmapCount;
							} else {
								view.viewType = VK_IMAGE_VIEW_TYPE_1D;
								view.subresourceRange.baseArrayLayer = array_offset_or_depth;
								view.subresourceRange.layerCount = 1;
								view.subresourceRange.baseMipLevel = mip_level;
								view.subresourceRange.levelCount = 1;
							}
						} else if (texture->_desc.Type == TextureType::Type2D) {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = 1;
							if (entire) {
								view.subresourceRange.baseMipLevel = 0;
								view.subresourceRange.levelCount = texture->_desc.MipmapCount;
							} else {
								view.subresourceRange.baseMipLevel = mip_level;
								view.subresourceRange.levelCount = 1;
							}
						} else if (texture->_desc.Type == TextureType::TypeArray2D) {
							if (entire) {
								view.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
								view.subresourceRange.baseArrayLayer = 0;
								view.subresourceRange.layerCount = texture->_desc.ArraySize;
								view.subresourceRange.baseMipLevel = 0;
								view.subresourceRange.levelCount = texture->_desc.MipmapCount;
							} else {
								view.viewType = VK_IMAGE_VIEW_TYPE_2D;
								view.subresourceRange.baseArrayLayer = array_offset_or_depth;
								view.subresourceRange.layerCount = 1;
								view.subresourceRange.baseMipLevel = mip_level;
								view.subresourceRange.levelCount = 1;
							}
						} else if (texture->_desc.Type == TextureType::TypeCube) {
							if (entire) {
								view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
								view.subresourceRange.baseArrayLayer = 0;
								view.subresourceRange.layerCount = 6;
								view.subresourceRange.baseMipLevel = 0;
								view.subresourceRange.levelCount = texture->_desc.MipmapCount;
							} else {
								view.viewType = VK_IMAGE_VIEW_TYPE_2D;
								view.subresourceRange.baseArrayLayer = array_offset_or_depth;
								view.subresourceRange.layerCount = 1;
								view.subresourceRange.baseMipLevel = mip_level;
								view.subresourceRange.levelCount = 1;
							}
						} else if (texture->_desc.Type == TextureType::TypeArrayCube) {
							if (entire) {
								view.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
								view.subresourceRange.baseArrayLayer = 0;
								view.subresourceRange.layerCount = 6 * texture->_desc.ArraySize;
								view.subresourceRange.baseMipLevel = 0;
								view.subresourceRange.levelCount = texture->_desc.MipmapCount;
							} else {
								view.viewType = VK_IMAGE_VIEW_TYPE_2D;
								view.subresourceRange.baseArrayLayer = array_offset_or_depth;
								view.subresourceRange.layerCount = 1;
								view.subresourceRange.baseMipLevel = mip_level;
								view.subresourceRange.levelCount = 1;
							}
						} else if (texture->_desc.Type == TextureType::Type3D) {
							if (entire) {
								view.viewType = VK_IMAGE_VIEW_TYPE_3D;
								view.subresourceRange.baseArrayLayer = 0;
								view.subresourceRange.layerCount = 1;
								view.subresourceRange.baseMipLevel = 0;
								view.subresourceRange.levelCount = texture->_desc.MipmapCount;
							} else {
								view.viewType = VK_IMAGE_VIEW_TYPE_2D;
								view.subresourceRange.baseArrayLayer = array_offset_or_depth;
								view.subresourceRange.layerCount = 1;
								view.subresourceRange.baseMipLevel = mip_level;
								view.subresourceRange.levelCount = 1;
							}
						} else return false;
						if (_api->Dispatch.vkCreateImageView(_api->Device, &view, &_api->Base->Allocator, &texture->_view) != VK_SUCCESS) return false;
					}
					return true;
				} else if (IsColorFormat(texture->_desc.Format)) {
					VkImageViewCreateInfo view;
					view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					view.pNext = 0;
					view.flags = 0;
					view.image = texture->_image;
					view.format = CreateVkFormat(texture->_desc.Format);
					view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
					view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
					view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
					view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
					if (texture->_desc.Type == TextureType::Type1D) {
						view.viewType = VK_IMAGE_VIEW_TYPE_1D;
						view.subresourceRange.baseArrayLayer = 0;
						view.subresourceRange.layerCount = 1;
						if (entire) {
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::TypeArray1D) {
						if (entire) {
							view.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = texture->_desc.ArraySize;
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.viewType = VK_IMAGE_VIEW_TYPE_1D;
							view.subresourceRange.baseArrayLayer = array_offset_or_depth;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::Type2D) {
						view.viewType = VK_IMAGE_VIEW_TYPE_2D;
						view.subresourceRange.baseArrayLayer = 0;
						view.subresourceRange.layerCount = 1;
						if (entire) {
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::TypeArray2D) {
						if (entire) {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = texture->_desc.ArraySize;
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D;
							view.subresourceRange.baseArrayLayer = array_offset_or_depth;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::TypeCube) {
						if (entire) {
							view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = 6;
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D;
							view.subresourceRange.baseArrayLayer = array_offset_or_depth;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::TypeArrayCube) {
						if (entire) {
							view.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = 6 * texture->_desc.ArraySize;
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D;
							view.subresourceRange.baseArrayLayer = array_offset_or_depth;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else if (texture->_desc.Type == TextureType::Type3D) {
						if (entire) {
							view.viewType = VK_IMAGE_VIEW_TYPE_3D;
							view.subresourceRange.baseArrayLayer = 0;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = 0;
							view.subresourceRange.levelCount = texture->_desc.MipmapCount;
						} else {
							view.viewType = VK_IMAGE_VIEW_TYPE_2D;
							view.subresourceRange.baseArrayLayer = array_offset_or_depth;
							view.subresourceRange.layerCount = 1;
							view.subresourceRange.baseMipLevel = mip_level;
							view.subresourceRange.levelCount = 1;
						}
					} else return false;
					if (_api->Dispatch.vkCreateImageView(_api->Device, &view, &_api->Base->Allocator, &texture->_view) != VK_SUCCESS) return false;
					return true;
				} else return false;
			}
			oref<VKBuffer> _internal_create_transfer_buffer(const BufferDesc & desc) noexcept
			{
				auto result = owrap(new (std::nothrow) VKBuffer(_api, this));
				if (!result) return 0;
				result->_desc = desc;
				result->_desc.Usage = ResourceUsageCPUAll;
				VkBufferCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				info.pNext = 0;
				info.flags = 0;
				info.size = desc.Length;
				if (!info.size) return 0;
				info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				info.queueFamilyIndexCount = 0;
				info.pQueueFamilyIndices = 0;
				if (_api->Dispatch.vkCreateBuffer(_api->Device, &info, &_api->Base->Allocator, &result->_buffer) != VK_SUCCESS) return 0;
				VkMemoryRequirements memreq;
				VkMemoryAllocateInfo memalloc;
				_api->Dispatch.vkGetBufferMemoryRequirements(_api->Device, result->_buffer, &memreq);
				memalloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memalloc.pNext = 0;
				memalloc.allocationSize = memreq.size;
				for (uint i = 0; i < _memory_info.memoryTypeCount; i++) if (memreq.memoryTypeBits & (1 << i)) {
					if (_memory_info.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
						memalloc.memoryTypeIndex = i;
						VkDeviceMemory memory;
						if (_api->Dispatch.vkAllocateMemory(_api->Device, &memalloc, &_api->Base->Allocator, &memory) == VK_SUCCESS) {
							result->_memory = memory;
							break;
						}
					}
				}
				if (!result->_memory) return 0;
				if (_api->Dispatch.vkBindBufferMemory(_api->Device, result->_buffer, result->_memory, 0) != VK_SUCCESS) return 0;
				return result;
			}
			oref<VKBuffer> _internal_create_buffer(const BufferDesc & desc, const ResourceInitDesc * init) noexcept
			{
				if (desc.Usage & ~ResourceUsageBufferMask) return 0;
				if (desc.MemoryPool == ResourceMemoryPool::Shared) return 0;
				bool create_read_only = desc.MemoryPool == ResourceMemoryPool::Immutable;
				bool create_cpu_io = false;
				if (desc.Usage & ResourceUsageCPUAll) create_cpu_io = true;
				if (create_read_only) {
					if (!init) return 0;
					if (desc.Usage & ResourceUsageShaderWrite) return 0;
					if (desc.Usage & ResourceUsageCPUWrite) return 0;
				}
				auto result = owrap(new (std::nothrow) VKBuffer(_api, this));
				if (!result) return 0;
				result->_desc = desc;
				result->_desc.Usage = 0;
				VkBufferCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				info.pNext = 0;
				info.flags = 0;
				info.size = desc.Length;
				if (!info.size) return 0;
				info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				if (desc.Usage & ResourceUsageShaderAll) {
					info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
					result->_desc.Usage |= ResourceUsageShaderAll;
				}
				if (desc.Usage & ResourceUsageConstantBuffer) {
					info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
					result->_desc.Usage |= ResourceUsageConstantBuffer;
				}
				if (desc.Usage & ResourceUsageIndexBuffer) {
					info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
					result->_desc.Usage |= ResourceUsageIndexBuffer;
				}
				if (create_cpu_io) result->_desc.Usage |= ResourceUsageCPUAll;
				info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				info.queueFamilyIndexCount = 0;
				info.pQueueFamilyIndices = 0;
				if (_api->Dispatch.vkCreateBuffer(_api->Device, &info, &_api->Base->Allocator, &result->_buffer) != VK_SUCCESS) return 0;
				VkMemoryRequirements memreq;
				VkMemoryAllocateInfo memalloc;
				_api->Dispatch.vkGetBufferMemoryRequirements(_api->Device, result->_buffer, &memreq);
				memalloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memalloc.pNext = 0;
				memalloc.allocationSize = memreq.size;
				for (uint i = 0; i < _memory_info.memoryTypeCount; i++) if (memreq.memoryTypeBits & (1 << i)) {
					if (_memory_info.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
						memalloc.memoryTypeIndex = i;
						VkDeviceMemory memory;
						if (_api->Dispatch.vkAllocateMemory(_api->Device, &memalloc, &_api->Base->Allocator, &memory) == VK_SUCCESS) {
							result->_memory = memory;
							break;
						}
					}
				}
				if (!result->_memory) for (uint i = 0; i < _memory_info.memoryTypeCount; i++) if (memreq.memoryTypeBits & (1 << i)) {
					memalloc.memoryTypeIndex = i;
					VkDeviceMemory memory;
					if (_api->Dispatch.vkAllocateMemory(_api->Device, &memalloc, &_api->Base->Allocator, &memory) == VK_SUCCESS) {
						result->_memory = memory;
						break;
					}
				}
				if (!result->_memory) return 0;
				if (_api->Dispatch.vkBindBufferMemory(_api->Device, result->_buffer, result->_memory, 0) != VK_SUCCESS) return 0;
				if (init || create_cpu_io) {
					auto transit = _internal_create_transfer_buffer(desc);
					if (!transit) return 0;
					if (init) {
						result->_transit = transit;
						auto pass = _dispatcher->BeginPrivatePass();
						if (!pass) return 0;
						if (!_dispatcher->InternalUpdateResourceData(pass, result, Index2(0, 0), Index3(0, 0, 0), Index3(desc.Length, 0, 0), *init)) return 0;
						if (!_dispatcher->EndPrivatePass(pass)) return 0;
						result->_transit.Clear();
					}
					if (create_cpu_io) result->_transit = transit;
				}
				return result;
			}
			oref<VKTexture> _internal_create_transfer_texture(const TextureDesc & desc) noexcept
			{
				if (IsDepthStencilFormat(desc.Format)) return 0;
				auto result = owrap(new (std::nothrow) VKTexture(_api, this));
				if (!result) return 0;
				result->_desc = desc;
				VkImageCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				info.pNext = 0;
				info.flags = 0;
				info.imageType = VK_IMAGE_TYPE_2D;
				if (desc.Type == TextureType::Type1D || desc.Type == TextureType::TypeArray1D) {
					info.extent.width = desc.Width;
					info.extent.height = info.extent.depth = 1;
				} else {
					info.extent.width = desc.Width;
					info.extent.height = desc.Height;
					info.extent.depth = 1;
				}
				info.arrayLayers = 1;
				info.mipLevels = 1;
				if (!info.extent.width || !info.extent.height || !info.extent.depth || !info.arrayLayers) return 0;
				info.format = CreateVkFormat(desc.Format);
				if (info.format == VK_FORMAT_UNDEFINED) return 0;
				result->_desc.MipmapCount = info.mipLevels;
				result->_desc.Usage = ResourceUsageCPUAll | ResourceUsageVideoAll;
				info.samples = VK_SAMPLE_COUNT_1_BIT;
				info.tiling = VK_IMAGE_TILING_LINEAR;
				info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				info.queueFamilyIndexCount = 0;
				info.pQueueFamilyIndices = 0;
				result->_current_layout = info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
				if (_api->Dispatch.vkCreateImage(_api->Device, &info, &_api->Base->Allocator, &result->_image) != VK_SUCCESS) return 0;
				VkMemoryRequirements memreq;
				VkMemoryAllocateInfo memalloc;
				_api->Dispatch.vkGetImageMemoryRequirements(_api->Device, result->_image, &memreq);
				memalloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memalloc.pNext = 0;
				memalloc.allocationSize = memreq.size;
				for (uint i = 0; i < _memory_info.memoryTypeCount; i++) if (memreq.memoryTypeBits & (1 << i)) {
					if (_memory_info.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
						memalloc.memoryTypeIndex = i;
						VkDeviceMemory memory;
						if (_api->Dispatch.vkAllocateMemory(_api->Device, &memalloc, &_api->Base->Allocator, &memory) == VK_SUCCESS) {
							result->_memory = memory;
							break;
						}
					}
				}
				if (!result->_memory) return 0;
				if (_api->Dispatch.vkBindImageMemory(_api->Device, result->_image, result->_memory, 0) != VK_SUCCESS) return 0;
				return result;
			}
			oref<VKTexture> _internal_create_texture(const TextureDesc & desc, const ResourceInitDesc * init, int * import) noexcept
			{
				if (desc.Usage & ~ResourceUsageTextureMask) return 0;
				bool create_read_only = desc.MemoryPool == ResourceMemoryPool::Immutable;
				bool create_cpu_io = false;
				bool create_shared_io = desc.MemoryPool == ResourceMemoryPool::Shared;
				if (desc.Usage & ResourceUsageCPUAll) create_cpu_io = true;
				if (desc.Usage & ResourceUsageVideoAll) create_cpu_io = true;
				if (create_read_only) {
					if (!init) return 0;
					if (desc.Usage & ResourceUsageShaderWrite) return 0;
					if (desc.Usage & ResourceUsageCPUWrite) return 0;
					if (desc.Usage & ResourceUsageRenderTarget) return 0;
					if (desc.Usage & ResourceUsageDepthStencil) return 0;
					if (desc.Usage & ResourceUsageVideoWrite) return 0;
				}
				if (create_shared_io) {
					if (init) return 0;
					if (desc.Usage & ResourceUsageCPUAll) return 0;
					if (desc.Usage & ResourceUsageVideoAll) return 0;
				}
				if (desc.Usage & ResourceUsageRenderTarget) {
					if (!IsColorFormat(desc.Format)) return 0;
				}
				if (desc.Usage & ResourceUsageDepthStencil) {
					if (!IsDepthStencilFormat(desc.Format)) return 0;
				}
				auto result = owrap(new (std::nothrow) VKTexture(_api, this));
				if (!result) return 0;
				result->_desc = desc;
				VkImageCreateInfo info;
				VkExternalMemoryImageCreateInfo info_share;
				info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				info.pNext = 0;
				info.flags = 0;
				if (desc.Type == TextureType::TypeCube || desc.Type == TextureType::TypeArrayCube) info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
				if (desc.Type == TextureType::Type3D && desc.Usage & (ResourceUsageRenderTarget | ResourceUsageDepthStencil)) info.flags |= VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT;
				if (desc.Type == TextureType::Type1D || desc.Type == TextureType::TypeArray1D) {
					info.imageType = VK_IMAGE_TYPE_1D;
					info.extent.width = desc.Width;
					info.extent.height = info.extent.depth = 1;
					if (desc.Type == TextureType::TypeArray1D) info.arrayLayers = desc.ArraySize;
					else info.arrayLayers = 1;
					if (desc.MipmapCount) info.mipLevels = desc.MipmapCount;
					else info.mipLevels = EvaluateMipMapLevels(desc.Width);
				} else if (desc.Type == TextureType::Type2D || desc.Type == TextureType::TypeArray2D || desc.Type == TextureType::TypeCube || desc.Type == TextureType::TypeArrayCube) {
					info.imageType = VK_IMAGE_TYPE_2D;
					info.extent.width = desc.Width;
					info.extent.height = desc.Height;
					info.extent.depth = 1;
					if (desc.Type == TextureType::TypeArray1D) info.arrayLayers = desc.ArraySize;
					else if (desc.Type == TextureType::TypeCube) info.arrayLayers = 6;
					else if (desc.Type == TextureType::TypeArrayCube) info.arrayLayers = 6 * desc.ArraySize;
					else info.arrayLayers = 1;
					if (desc.MipmapCount) info.mipLevels = desc.MipmapCount;
					else info.mipLevels = EvaluateMipMapLevels(desc.Width, desc.Height);
				} else if (desc.Type == TextureType::Type3D) {
					info.imageType = VK_IMAGE_TYPE_3D;
					info.extent.width = desc.Width;
					info.extent.height = desc.Height;
					info.extent.depth = desc.Depth;
					info.arrayLayers = 1;
					if (desc.MipmapCount) info.mipLevels = desc.MipmapCount;
					else info.mipLevels = EvaluateMipMapLevels(desc.Width, desc.Height, desc.Depth);
				} else return 0;
				if (!info.extent.width || !info.extent.height || !info.extent.depth || !info.arrayLayers) return 0;
				info.format = CreateVkFormat(desc.Format);
				if (info.format == VK_FORMAT_UNDEFINED) return 0;
				result->_desc.Width = info.extent.width;
				result->_desc.Height = info.extent.height;
				if (desc.Type == TextureType::Type3D) result->_desc.Depth = info.extent.depth;
				else if (desc.Type == TextureType::TypeCube || desc.Type == TextureType::TypeArrayCube) result->_desc.ArraySize = info.arrayLayers / 6;
				else result->_desc.ArraySize = info.arrayLayers;
				result->_desc.MipmapCount = info.mipLevels;
				result->_desc.Usage = 0;
				info.samples = VK_SAMPLE_COUNT_1_BIT;
				info.tiling = VK_IMAGE_TILING_OPTIMAL;
				info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				if (desc.Usage & ResourceUsageShaderRead) {
					info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
					result->_desc.Usage |= ResourceUsageShaderRead;
				}
				if (desc.Usage & ResourceUsageShaderWrite) {
					info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
					result->_desc.Usage |= ResourceUsageShaderWrite;
				}
				if (desc.Usage & ResourceUsageRenderTarget) {
					info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
					result->_desc.Usage |= ResourceUsageRenderTarget;
				}
				if (desc.Usage & ResourceUsageDepthStencil) {
					info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
					result->_desc.Usage |= ResourceUsageDepthStencil;
				}
				if (create_cpu_io) {
					result->_desc.Usage |= ResourceUsageCPUAll | ResourceUsageVideoAll;
				}
				info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				info.queueFamilyIndexCount = 0;
				info.pQueueFamilyIndices = 0;
				info.initialLayout = result->_current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
				if (create_shared_io) {
					info.pNext = &info_share;
					info_share.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
					info_share.pNext = 0;
					info_share.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
				}
				if (_api->Dispatch.vkCreateImage(_api->Device, &info, &_api->Base->Allocator, &result->_image) != VK_SUCCESS) return 0;
				VkMemoryRequirements memreq;
				VkMemoryAllocateInfo memalloc;
				VkExportMemoryAllocateInfo mem_export;
				VkImportMemoryFdInfoKHR mem_import;
				_api->Dispatch.vkGetImageMemoryRequirements(_api->Device, result->_image, &memreq);
				memalloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memalloc.pNext = 0;
				memalloc.allocationSize = memreq.size;
				if (create_shared_io) {
					if (import) {
						memalloc.pNext = &mem_import;
						mem_import.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
						mem_import.pNext = &mem_export;
						mem_import.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
						mem_import.fd = *import;
					} else memalloc.pNext = &mem_export;
					mem_export.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
					mem_export.pNext = 0;
					mem_export.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
				}
				for (uint i = 0; i < _memory_info.memoryTypeCount; i++) if (memreq.memoryTypeBits & (1 << i)) {
					if (_memory_info.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
						memalloc.memoryTypeIndex = i;
						VkDeviceMemory memory;
						if (_api->Dispatch.vkAllocateMemory(_api->Device, &memalloc, &_api->Base->Allocator, &memory) == VK_SUCCESS) {
							result->_memory = memory;
							break;
						}
					}
				}
				if (!result->_memory) for (uint i = 0; i < _memory_info.memoryTypeCount; i++) if (memreq.memoryTypeBits & (1 << i)) {
					memalloc.memoryTypeIndex = i;
					VkDeviceMemory memory;
					if (_api->Dispatch.vkAllocateMemory(_api->Device, &memalloc, &_api->Base->Allocator, &memory) == VK_SUCCESS) {
						result->_memory = memory;
						break;
					}
				}
				if (!result->_memory) return 0;
				if (import) *import = -1;
				if (_api->Dispatch.vkBindImageMemory(_api->Device, result->_image, result->_memory, 0) != VK_SUCCESS) return 0;
				if (init || create_cpu_io) {
					auto transit = _internal_create_transfer_texture(desc);
					if (!transit) return 0;
					if (init) {
						result->_transit = transit;
						auto pass = _dispatcher->BeginPrivatePass();
						if (!pass) return 0;
						if (desc.Type == TextureType::Type1D || desc.Type == TextureType::TypeArray1D) {
							for (uint j = 0; j < result->_desc.ArraySize; j++) for (uint i = 0; i < result->_desc.MipmapCount; i++) {
								uint subres = i + j * result->_desc.MipmapCount;
								Index3 size(result->_desc.Width, 1, 1);
								EvaluateMipMapSize(i, size);
								if (!_dispatcher->InternalUpdateResourceData(pass, result, Index2(i, j), Index3(0, 0, 0), size, init[subres])) return 0;
							}
						} else if (desc.Type == TextureType::Type2D || desc.Type == TextureType::TypeArray2D) {
							for (uint j = 0; j < result->_desc.ArraySize; j++) for (uint i = 0; i < result->_desc.MipmapCount; i++) {
								uint subres = i + j * result->_desc.MipmapCount;
								Index3 size(result->_desc.Width, result->_desc.Height, 1);
								EvaluateMipMapSize(i, size);
								if (!_dispatcher->InternalUpdateResourceData(pass, result, Index2(i, j), Index3(0, 0, 0), size, init[subres])) return 0;
							}
						} else if (desc.Type == TextureType::TypeCube || desc.Type == TextureType::TypeArrayCube) {
							for (uint j = 0; j < result->_desc.ArraySize * 6; j++) for (uint i = 0; i < result->_desc.MipmapCount; i++) {
								uint subres = i + j * result->_desc.MipmapCount;
								Index3 size(result->_desc.Width, result->_desc.Height, 1);
								EvaluateMipMapSize(i, size);
								if (!_dispatcher->InternalUpdateResourceData(pass, result, Index2(i, j), Index3(0, 0, 0), size, init[subres])) return 0;
							}
						} else if (desc.Type == TextureType::Type3D) {
							for (uint i = 0; i < result->_desc.MipmapCount; i++) {
								Index3 size(result->_desc.Width, result->_desc.Height, result->_desc.Depth);
								EvaluateMipMapSize(i, size);
								if (!_dispatcher->InternalUpdateResourceData(pass, result, Index2(i, 0), Index3(0, 0, 0), size, init[i])) return 0;
							}
						}
						if (!_dispatcher->EndPrivatePass(pass)) return 0;
						result->_transit.Clear();
					}
					if (create_cpu_io) result->_transit = transit;
				}
				if (!init && !create_shared_io) {
					auto pass = _dispatcher->BeginPrivatePass();
					if (!pass) return 0;
					VkImageMemoryBarrier barrier;
					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.pNext = 0;
					barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
					barrier.oldLayout = result->_current_layout;
					barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
					barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = _queue_family_index;
					barrier.image = result->_image;
					if (IsDepthStencilFormat(result->_desc.Format)) {
						if (GetFormatChannelCount(result->_desc.Format) == 1) barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
						else barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
					} else if (IsColorFormat(result->_desc.Format)) barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					barrier.subresourceRange.baseArrayLayer = barrier.subresourceRange.baseMipLevel = 0;
					barrier.subresourceRange.layerCount = result->GetArraySize();
					barrier.subresourceRange.levelCount = result->GetMipmapCount();
					_api->Dispatch.vkCmdPipelineBarrier(pass, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, 0, 0, 0, 1, &barrier);
					result->_current_layout = VK_IMAGE_LAYOUT_GENERAL;
					if (!_dispatcher->EndPrivatePass(pass)) return 0;
				}
				if (desc.Usage & (ResourceUsageShaderAll | ResourceUsageRenderTarget | ResourceUsageDepthStencil)) {
					if (!_texture_init_views(result, true, 0, 0)) return 0;
				}
				if (create_shared_io && !import) try { result->_shared = owrap(new VKSharedObject(_api, result, _physical, result->_memory, memreq.size)); } catch (...) { return 0; }
				return result;
			}
		public:
			VKDevice(void) : _physical(0), _queue(0), _pool(0), _queue_family_index(-1), _valid(true) {}
			virtual ~VKDevice(void) override
			{
				if (_immediate_context) _immediate_context->Finalize();
				if (_dispatcher) _dispatcher->Finalize();
				if (_pool) _api->Dispatch.vkDestroyCommandPool(_api->Device, _pool, &_api->Base->Allocator);
			}
			bool Initialize(VKDeviceAPI * api, VkPhysicalDevice physical, int queue_family_index) noexcept
			{
				_api.SetRetain(api);
				_physical = physical;
				_queue_family_index = queue_family_index;
				_api->Dispatch.vkGetDeviceQueue(_api->Device, queue_family_index, 0, &_queue);
				VkCommandPoolCreateInfo pool_info;
				pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				pool_info.pNext = 0;
				pool_info.flags = 0;
				pool_info.queueFamilyIndex = queue_family_index;
				if (_api->Dispatch.vkCreateCommandPool(_api->Device, &pool_info, &_api->Base->Allocator, &_pool) != VK_SUCCESS) return false;
				_stats = owrap(new (std::nothrow) VKDeviceStats(_api, _physical));
				if (!_stats) return false;
				_dispatcher = owrap(new (std::nothrow) VKQueue(this, _stats));
				if (!_dispatcher) return false;
				if (!_dispatcher->Initialize(_api, _queue, _pool, _queue_family_index, &_valid)) return false;
				_immediate_context = owrap(new (std::nothrow) VKDeviceImmediateContext(this, _dispatcher));
				if (!_immediate_context) return false;
				_api->Base->Dispatch.vkGetPhysicalDeviceMemoryProperties(_physical, &_memory_info);
				VkPhysicalDeviceProperties prop;
				_api->Dispatch.vkGetPhysicalDeviceProperties(_physical, &prop);
				try { _name = prop.deviceName; } catch (...) {}
				return true;
			}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKDevice"; ESSE_TRY_OUTRO(string()) }
			virtual const string & GetDeviceName(void) noexcept override { return _name; }
			virtual uint64 GetDeviceIdentifier(void) noexcept override
			{
				VkPhysicalDeviceProperties prop;
				_api->Dispatch.vkGetPhysicalDeviceProperties(_physical, &prop);
				return (uint64(prop.vendorID) << 32) | uint64(prop.deviceID);
			}
			virtual bool DeviceIsValid(void) noexcept override { return _valid; }
			virtual void GetImplementationInfo(string & tech, uint32 & version_major, uint32 & version_minor) noexcept override
			{
				try { tech = U"Vulkan"; } catch (...) {}
				version_major = VK_API_VERSION_MAJOR(_api->Base->Version);
				version_minor = VK_API_VERSION_MINOR(_api->Base->Version);
			}
			virtual DeviceClass GetDeviceClass(void) noexcept override
			{
				VkPhysicalDeviceProperties prop;
				_api->Dispatch.vkGetPhysicalDeviceProperties(_physical, &prop);
				if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return DeviceClass::Discrete;
				else if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) return DeviceClass::Integrated;
				else if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) return DeviceClass::Software;
				else return DeviceClass::Unknown;
			}
			virtual uint64 GetDeviceMemory(void) noexcept override
			{
				uint64 total = 0;
				for (uint i = 0; i < _memory_info.memoryHeapCount; i++) if (_memory_info.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) total += _memory_info.memoryHeaps[i].size;
				return total;
			}
			virtual bool GetDevicePixelFormatSupport(PixelFormat format, PixelFormatUsage usage) noexcept override
			{
				if (usage == PixelFormatUsage::ShaderRead || usage == PixelFormatUsage::ShaderSample || usage == PixelFormatUsage::BitmapSource || usage == PixelFormatUsage::RenderTarget || usage == PixelFormatUsage::BlendRenderTarget || usage == PixelFormatUsage::DepthStencil) {
					auto vk_format = CreateVkFormat(format);
					if (vk_format == VK_FORMAT_UNDEFINED) return false;
					VkFormatProperties prop;
					_api->Dispatch.vkGetPhysicalDeviceFormatProperties(_physical, vk_format, &prop);
					if (usage == PixelFormatUsage::ShaderRead) {
						return prop.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
					} else if (usage == PixelFormatUsage::ShaderSample) {
						return (prop.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && (prop.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
					} else if (usage == PixelFormatUsage::BitmapSource) {
						return (prop.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && IsColorFormat(format);
					} else if (usage == PixelFormatUsage::RenderTarget) {
						return prop.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
					} else if (usage == PixelFormatUsage::BlendRenderTarget) {
						return prop.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
					} else if (usage == PixelFormatUsage::DepthStencil) {
						return prop.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
					} else return false;
				} else if (usage == PixelFormatUsage::WindowSurface) {
					#ifdef ESSE_VULKAN_PRESENTATION
						auto vk_format = CreateVkFormat(format);
						if (vk_format == VK_FORMAT_UNDEFINED) return false;
						auto ws = Windows::GetWindowSystem();
						if (!ws) return false;
						ErrorContext ectx;
						// TODO: IMPLEMENT WAYLAND
						#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
							ErrorClear(ectx);
							auto ws_x11 = reinterpret_cast<X11::IX11WindowSystem *>(ws->DynamicCast(Linux::Classes::X11_WindowSystem, ectx));
							if (!ErrorTest(ectx) && _api->Dispatch.vkCreateXlibSurfaceKHR && _api->Dispatch.vkCreateSwapchainKHR) {
								auto xapi = ws_x11->GetConnection()->GetAPI();
								auto display = ws_x11->GetConnection()->GetXDisplay();
								VkSurfaceKHR probe;
								VkXlibSurfaceCreateInfoKHR info;
								info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
								info.pNext = 0;
								info.flags = 0;
								info.dpy = display;
								info.window = xapi->XDefaultRootWindow(display);
								if (_api->Dispatch.vkCreateXlibSurfaceKHR(_api->Base->Instance, &info, &_api->Base->Allocator, &probe) != VK_SUCCESS) return false;
								bool result = false;
								try {
									array<VkSurfaceFormatKHR> formats(1);
									uint count;
									if (_api->Dispatch.vkGetPhysicalDeviceSurfaceFormatsKHR(_physical, probe, &count, 0) < 0) throw Exception();
									formats.SetLength(count);
									if (_api->Dispatch.vkGetPhysicalDeviceSurfaceFormatsKHR(_physical, probe, &count, formats) < 0) throw Exception();
									if (count < formats.GetLength()) formats.SetLength(count);
									for (auto & fmt : formats) if (fmt.format == vk_format) { result = true; break; }
								} catch (...) {}
								_api->Dispatch.vkDestroySurfaceKHR(_api->Base->Instance, probe, &_api->Base->Allocator);
								return result;
							}
						#endif
					#endif
					return false;
				} else if (usage == PixelFormatUsage::RenderTarget2D || usage == PixelFormatUsage::VideoIO) {
					return format == PixelFormat::B8G8R8A8_unorm;
				} else return false;
			}
			virtual IDeviceContext * GetPrimaryDeviceContext(void) noexcept override { return _immediate_context; }
			virtual oref<IDeviceContext> CreateDeferredDeviceContext(void) noexcept override { return oref<IDeviceContext>::CreateOwned(new (std::nothrow) VKDeviceDeferredContext(this, _dispatcher)); }
			virtual oref<IShaderLibrary> LoadShaderLibraryFromData(const void * data, uintptr length, ErrorContext & ectx) noexcept override
			{
				if (!data || !length) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
				ESSE_TRY_INTRO return LoadShaderLibrary(StaticMemoryStream::Create(data, length), ectx); ESSE_TRY_OUTRO(0)
			}
			virtual oref<IShaderLibrary> LoadShaderLibrary(Stream * stream, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!stream) throw InvalidArgumentException();
					auto arc = Formationes::Archive::Open(stream);
					auto result = owrap(new (std::nothrow) VKShaderLibrary(_api, this));
					if (!result) throw OutOfMemoryException();
					auto file_count = arc->GetFileCount();
					for (int file = 1; file <= file_count; file++) {
						auto user = arc->GetFileUserData(file);
						ShaderType shader_type = ShaderType::Unknown;
						string shader_name;
						string shader_entry;
						if (user == 0x0001) shader_type = ShaderType::Vertex;
						else if (user == 0x0002) shader_type = ShaderType::Pixel;
						else return 0;
						shader_name = arc->GetFileName(file);
						auto del = shader_name.FindFirst(L'!');
						if (del < 0) return 0;
						shader_entry = shader_name.Substring(del + 1, -1);
						shader_name = shader_name.Substring(0, del);
						auto shader_stream = arc->QueryFileStream(file, Formationes::ArchiveStream::Native);
						array<uint32> shader_spirv(1);
						auto shader_length = shader_stream->GetLength();
						if (shader_length & 0x3) return 0;
						shader_spirv.SetLength(shader_length >> 2U);
						shader_stream->Read(shader_spirv.GetBuffer(), shader_length);
						auto shader = owrap(new (std::nothrow) VKShader(_api, this));
						if (!shader) throw OutOfMemoryException();
						uintptr shader_spirv_offset;
						ReadShaderResourceMapping(shader_spirv.GetBuffer(), shader_spirv.GetLength() << 2U, shader->GetResourceMapping(), shader_spirv_offset, ectx);
						if (ErrorTest(ectx)) return 0;
						VkShaderModuleCreateInfo info;
						info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
						info.pNext = 0;
						info.flags = 0;
						info.pCode = shader_spirv.GetBuffer() + shader_spirv_offset;
						info.codeSize = (shader_spirv.GetLength() - shader_spirv_offset) << 2U;
						if (_api->Dispatch.vkCreateShaderModule(_api->Device, &info, &_api->Base->Allocator, &shader->_module) != VK_SUCCESS) return 0;
						shader->_name = shader_name;
						shader->_type = shader_type;
						shader->_entry = shader_entry;
						result->_shaders.Append(shader->_name, shader);
					}
					if (result->_shaders.IsEmpty()) throw InvalidFormatException();
					return oref<IShaderLibrary>(result);
				ESSE_TRY_OUTRO(0)
			}
			virtual oref<IShaderLibrary> CompileShaderLibraryFromData(const void * data, uintptr length, ErrorContext & ectx) noexcept override
			{
				if (!data || !length) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
				ESSE_TRY_INTRO return CompileShaderLibrary(StaticMemoryStream::Create(data, length), ectx); ESSE_TRY_OUTRO(0)
			}
			virtual oref<IShaderLibrary> CompileShaderLibrary(Stream * stream, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!stream) throw InvalidArgumentException();
					auto arc = Formationes::Archive::Open(stream);
					oref<VKCompilerAPI> compiler = owrap(new VKCompilerAPI);
					auto result = owrap(new (std::nothrow) VKShaderLibrary(_api, this));
					if (!result) throw OutOfMemoryException();
					auto file_count = arc->GetFileCount();
					for (uint file = 1; file <= file_count; file++) {
						auto user = arc->GetFileUserData(file);
						if ((user & 0xFF0000) != 0x30000) continue;
						ShaderType shader_type = ShaderType::Unknown;
						string shader_name;
						string shader_entry;
						if ((user & 0xFFFF) == 0x0001) shader_type = ShaderType::Vertex;
						else if ((user & 0xFFFF) == 0x0002) shader_type = ShaderType::Pixel;
						else throw InvalidFormatException();
						shader_name = arc->GetFileName(file);
						auto del = shader_name.FindFirst(U'!');
						if (del < 0) throw InvalidFormatException();
						shader_entry = shader_name.Substring(del + 1, -1);
						shader_name = shader_name.Substring(0, del);
						auto shader_stream = arc->QueryFileStream(file, Formationes::ArchiveStream::Native);
						array<char> shader_data(1);
						array<uint32> shader_spirv(1);
						auto shader_length = shader_stream->GetLength();
						shader_data.SetLength(shader_length + 1);
						if (shader_stream->Read(shader_data.GetBuffer(), shader_length) != shader_length) throw InvalidFormatException();
						shader_data[shader_length] = 0;
						glslang_input_t input;
						input.language = GLSLANG_SOURCE_GLSL;
						if (shader_type == ShaderType::Vertex) input.stage = GLSLANG_STAGE_VERTEX;
						else if (shader_type == ShaderType::Pixel) input.stage = GLSLANG_STAGE_FRAGMENT;
						input.client = GLSLANG_CLIENT_VULKAN;
						input.client_version = GLSLANG_TARGET_VULKAN_1_3;
						input.target_language = GLSLANG_TARGET_SPV;
						input.target_language_version = GLSLANG_TARGET_SPV_1_6;
						input.code = shader_data;
						input.default_version = 450;
						input.default_profile = GLSLANG_NO_PROFILE;
						input.force_default_version_and_profile = false;
						input.forward_compatible = false;
						input.messages = GLSLANG_MSG_DEFAULT_BIT;
						input.resource = compiler->glslang_default_resource();
						input.callbacks.free_include_result = 0;
						input.callbacks.include_local = 0;
						input.callbacks.include_system = 0;
						input.callbacks_ctx = 0;
						auto context = compiler->glslang_shader_create(&input);
						if (!context) throw NotImplementedException();
						if (!compiler->glslang_shader_preprocess(context, &input)) {
							compiler->glslang_shader_delete(context);
							throw CustomException(ErrorMake(Errores::ErrorDynamicLinkage, Errores::SuberrorDL::InvalidFunctionFormat));
						}
						if (!compiler->glslang_shader_parse(context, &input)) {
							compiler->glslang_shader_delete(context);
							throw CustomException(ErrorMake(Errores::ErrorDynamicLinkage, Errores::SuberrorDL::InvalidFunctionFormat));
						}
						auto module_context = compiler->glslang_program_create();
						if (!module_context) {
							compiler->glslang_shader_delete(context);
							throw NotImplementedException();
						}
						compiler->glslang_program_add_shader(module_context, context);
						if (!compiler->glslang_program_link(module_context, GLSLANG_MSG_DEFAULT_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
							compiler->glslang_program_delete(module_context);
							compiler->glslang_shader_delete(context);
							throw CustomException(ErrorMake(Errores::ErrorDynamicLinkage, Errores::SuberrorDL::LinkageFailure));
						}
						compiler->glslang_program_SPIRV_generate(module_context, input.stage);
						try {
							shader_spirv.SetLength(compiler->glslang_program_SPIRV_get_size(module_context));
							compiler->glslang_program_SPIRV_get(module_context, shader_spirv);
						} catch (...) {
							compiler->glslang_program_delete(module_context);
							compiler->glslang_shader_delete(context);
							throw OutOfMemoryException();
						}
						compiler->glslang_program_delete(module_context);
						compiler->glslang_shader_delete(context);
						auto shader = owrap(new (std::nothrow) VKShader(_api, this));
						if (!shader) throw OutOfMemoryException();
						VkShaderModuleCreateInfo info;
						info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
						info.pNext = 0;
						info.flags = 0;
						info.pCode = shader_spirv.GetBuffer();
						info.codeSize = shader_spirv.GetLength() << 2U;
						if (_api->Dispatch.vkCreateShaderModule(_api->Device, &info, &_api->Base->Allocator, &shader->_module) != VK_SUCCESS) throw InvalidFormatException();
						shader->_name = shader_name;
						shader->_type = shader_type;
						shader->_entry = shader_entry;
						result->_shaders.Append(shader_name, shader);
						ReadShaderResourceMapping(shader_data, shader->GetResourceMapping(), ectx);
						if (ErrorTest(ectx)) return 0;
					}
					if (result->_shaders.IsEmpty()) throw CustomException(ErrorMake(Errores::ErrorDynamicLinkage, Errores::SuberrorDL::NoDedicatedVersion));
					return oref<IShaderLibrary>(result);
				ESSE_TRY_OUTRO(0)
			}
			virtual oref<IPipelineState> CreateRenderingPipelineState(const PipelineStateDesc & desc) noexcept override
			{
				if (!desc.RenderTargetCount || desc.RenderTargetCount > 8 || !desc.VertexShader || !desc.PixelShader || !_api->Dispatch.vkCmdBeginRenderingKHR) return 0;
				if (desc.VertexShader->GetType() != ShaderType::Vertex) return 0;
				if (desc.PixelShader->GetType() != ShaderType::Pixel) return 0;
				oref<VKPipelineLayout> pipeline_layout;
				try {
					auto layout_symbol = VKPipelineLayout::MakeLayoutSymbol(static_cast<VKShader *>(desc.VertexShader), static_cast<VKShader *>(desc.PixelShader));
					auto cached_layout = _layouts[layout_symbol];
					if (!cached_layout) {
						pipeline_layout = owrap(new (std::nothrow) VKPipelineLayout(_stats));
						if (!pipeline_layout || !pipeline_layout->Initialize(static_cast<VKShader *>(desc.VertexShader), static_cast<VKShader *>(desc.PixelShader))) return 0;
						_layouts.Append(layout_symbol, pipeline_layout);
					} else pipeline_layout = cached_layout;
					auto current = _layouts.GetFirst();
					while (current) {
						auto next = current->GetNext();
						if (current->GetValue().value->GetReferenceCount() == 1) _layouts.BinaryTree::Remove(current);
						current = next;
					}
				} catch (...) { return 0; }
				auto state = owrap(new (std::nothrow) VKPipelineState(_api, this, pipeline_layout));
				if (!state) return 0;
				VkPipelineRenderingCreateInfo attachments;
				VkFormat attachment_formats[8];
				VkPipelineColorBlendAttachmentState blend_modes[8];
				VkPipelineShaderStageCreateInfo stages_info[2];
				VkGraphicsPipelineCreateInfo pipeline_info;
				attachments.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
				attachments.pNext = 0;
				attachments.viewMask = 0;
				attachments.colorAttachmentCount = desc.RenderTargetCount;
				attachments.pColorAttachmentFormats = attachment_formats;
				for (uint i = 0; i < desc.RenderTargetCount; i++) {
					auto & rt = desc.RenderTarget[i];
					if (!IsColorFormat(rt.Format)) return 0;
					attachment_formats[i] = CreateVkFormat(rt.Format);
					if (rt.Flags & RenderTargetFlagBlendingEnabled) {
						blend_modes[i].blendEnable = VK_TRUE;
						blend_modes[i].srcColorBlendFactor = CreateVkFactor(rt.OverFactorRGB);
						blend_modes[i].dstColorBlendFactor = CreateVkFactor(rt.BaseFactorRGB);
						blend_modes[i].colorBlendOp = CreateVkBlend(rt.BlendRGB);
						blend_modes[i].srcAlphaBlendFactor = CreateVkFactor(rt.OverFactorAlpha);
						blend_modes[i].dstAlphaBlendFactor = CreateVkFactor(rt.BaseFactorAlpha);
						blend_modes[i].alphaBlendOp = CreateVkBlend(rt.BlendAlpha);
					} else {
						blend_modes[i].blendEnable = VK_FALSE;
						blend_modes[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
						blend_modes[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
						blend_modes[i].colorBlendOp = VK_BLEND_OP_ADD;
						blend_modes[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
						blend_modes[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
						blend_modes[i].alphaBlendOp = VK_BLEND_OP_ADD;
					}
					blend_modes[i].colorWriteMask = 0;
					if (!(rt.Flags & RenderTargetFlagRestrictWriteRed)) blend_modes[i].colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
					if (!(rt.Flags & RenderTargetFlagRestrictWriteGreen)) blend_modes[i].colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
					if (!(rt.Flags & RenderTargetFlagRestrictWriteBlue)) blend_modes[i].colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
					if (!(rt.Flags & RenderTargetFlagRestrictWriteAlpha)) blend_modes[i].colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
				}
				if (desc.DepthStencil.Flags & (DepthStencilFlagDepthTestEnabled | DepthStencilFlagStencilTestEnabled)) {
					if (!IsDepthStencilFormat(desc.DepthStencil.Format)) return 0;
					attachments.depthAttachmentFormat = CreateVkFormat(desc.DepthStencil.Format);
					if (desc.DepthStencil.Format == PixelFormat::D24S8_unorm || desc.DepthStencil.Format == PixelFormat::D32S8_float) {
						attachments.stencilAttachmentFormat = attachments.depthAttachmentFormat;
					} else {
						attachments.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
					}
				} else {
					attachments.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
					attachments.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
				}
				VkPipelineVertexInputStateCreateInfo vertex_input;
				VkPipelineInputAssemblyStateCreateInfo input_assembler;
				VkPipelineTessellationStateCreateInfo tesselation;
				VkPipelineViewportStateCreateInfo viewport;
				VkPipelineRasterizationStateCreateInfo rasterization;
				VkPipelineRasterizationDepthClipStateCreateInfoEXT rasterization_depth_clip;
				VkPipelineMultisampleStateCreateInfo multisample;
				VkPipelineDepthStencilStateCreateInfo depth_stencil;
				VkPipelineColorBlendStateCreateInfo color_blend;
				VkPipelineDynamicStateCreateInfo dynamic_state;
				VkDynamicState dynamic_state_list[] = {
					VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
					VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
					VK_DYNAMIC_STATE_STENCIL_REFERENCE
				};
				stages_info[0].sType = stages_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stages_info[0].pNext = stages_info[1].pNext = 0;
				stages_info[0].flags = stages_info[1].flags = 0;
				stages_info[0].pSpecializationInfo = stages_info[1].pSpecializationInfo = 0;
				stages_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
				stages_info[0].module = static_cast<VKShader *>(desc.VertexShader)->_module;
				stages_info[0].pName = static_cast<VKShader *>(desc.VertexShader)->_entry;
				stages_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				stages_info[1].module = static_cast<VKShader *>(desc.PixelShader)->_module;
				stages_info[1].pName = static_cast<VKShader *>(desc.PixelShader)->_entry;
				vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertex_input.pNext = 0;
				vertex_input.flags = 0;
				vertex_input.vertexBindingDescriptionCount = 0;
				vertex_input.pVertexBindingDescriptions = 0;
				vertex_input.vertexAttributeDescriptionCount = 0;
				vertex_input.pVertexAttributeDescriptions = 0;
				input_assembler.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				input_assembler.pNext = 0;
				input_assembler.flags = 0;
				if (desc.Topology == PrimitiveTopology::PointList) input_assembler.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
				else if (desc.Topology == PrimitiveTopology::LineList) input_assembler.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				else if (desc.Topology == PrimitiveTopology::LineStrip) input_assembler.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
				else if (desc.Topology == PrimitiveTopology::TriangleList) input_assembler.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				else if (desc.Topology == PrimitiveTopology::TriangleStrip) input_assembler.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
				else return 0;
				input_assembler.primitiveRestartEnable = VK_FALSE;
				tesselation.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
				tesselation.pNext = 0;
				tesselation.flags = 0;
				tesselation.patchControlPoints = 1;
				viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewport.pNext = 0;
				viewport.flags = 0;
				viewport.viewportCount = 0;
				viewport.pViewports = 0;
				viewport.scissorCount = 0;
				viewport.pScissors = 0;
				rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterization.pNext = 0;
				rasterization.flags = 0;
				rasterization.depthClampEnable = VK_FALSE;
				rasterization.rasterizerDiscardEnable = VK_FALSE;
				if (desc.Rasterization.DepthClipEnable) {
					rasterization_depth_clip.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
					rasterization_depth_clip.pNext = 0;
					rasterization_depth_clip.flags = 0;
					rasterization_depth_clip.depthClipEnable = VK_TRUE;
					rasterization.pNext = &rasterization_depth_clip;
				}
				if (desc.Rasterization.Fill == FillMode::Solid) rasterization.polygonMode = VK_POLYGON_MODE_FILL;
				else if (desc.Rasterization.Fill == FillMode::Wireframe) rasterization.polygonMode = VK_POLYGON_MODE_LINE;
				else return 0;
				if (desc.Rasterization.Cull == CullMode::None) rasterization.cullMode = VK_CULL_MODE_NONE;
				else if (desc.Rasterization.Cull == CullMode::Front) rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
				else if (desc.Rasterization.Cull == CullMode::Back) rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
				else return 0;
				if (desc.Rasterization.FrontIsCounterClockwise) rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				else rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
				if (desc.Rasterization.DepthBias || desc.Rasterization.DepthBiasClamp || desc.Rasterization.SlopeScaledDepthBias) {
					rasterization.depthBiasEnable = VK_TRUE;
					rasterization.depthBiasConstantFactor = desc.Rasterization.DepthBias;
					rasterization.depthBiasClamp = desc.Rasterization.DepthBiasClamp;
					rasterization.depthBiasSlopeFactor = desc.Rasterization.SlopeScaledDepthBias;
				} else {
					rasterization.depthBiasEnable = VK_FALSE;
					rasterization.depthBiasConstantFactor = 0.0f;
					rasterization.depthBiasClamp = 0.0f;
					rasterization.depthBiasSlopeFactor = 0.0f;
				}
				rasterization.lineWidth = 1.0f;
				multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisample.pNext = 0;
				multisample.flags = 0;
				multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				multisample.sampleShadingEnable = VK_FALSE;
				multisample.minSampleShading = 1.0f;
				multisample.pSampleMask = 0;
				multisample.alphaToCoverageEnable = VK_FALSE;
				multisample.alphaToOneEnable = VK_FALSE;
				depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				depth_stencil.pNext = 0;
				depth_stencil.flags = 0;
				depth_stencil.depthTestEnable = (desc.DepthStencil.Flags & DepthStencilFlagDepthTestEnabled) ? VK_TRUE : VK_FALSE;
				depth_stencil.depthWriteEnable = (desc.DepthStencil.Flags & DepthStencilFlagDepthWriteEnabled) ? VK_TRUE : VK_FALSE;
				depth_stencil.depthCompareOp = CreateVkCompare(desc.DepthStencil.DepthTestFunction);
				depth_stencil.depthBoundsTestEnable = VK_FALSE;
				depth_stencil.stencilTestEnable = (desc.DepthStencil.Flags & DepthStencilFlagStencilTestEnabled) ? VK_TRUE : VK_FALSE;
				depth_stencil.front.failOp = CreateVkStencil(desc.DepthStencil.FrontStencil.OnStencilTestFailed);
				depth_stencil.front.passOp = CreateVkStencil(desc.DepthStencil.FrontStencil.OnTestsPassed);
				depth_stencil.front.depthFailOp = CreateVkStencil(desc.DepthStencil.FrontStencil.OnDepthTestFailed);
				depth_stencil.front.compareOp = CreateVkCompare(desc.DepthStencil.FrontStencil.TestFunction);
				depth_stencil.front.compareMask = desc.DepthStencil.StencilReadMask;
				depth_stencil.front.writeMask = desc.DepthStencil.StencilWriteMask;
				depth_stencil.front.reference = 0;
				depth_stencil.back.failOp = CreateVkStencil(desc.DepthStencil.BackStencil.OnStencilTestFailed);
				depth_stencil.back.passOp = CreateVkStencil(desc.DepthStencil.BackStencil.OnTestsPassed);
				depth_stencil.back.depthFailOp = CreateVkStencil(desc.DepthStencil.BackStencil.OnDepthTestFailed);
				depth_stencil.back.compareOp = CreateVkCompare(desc.DepthStencil.BackStencil.TestFunction);
				depth_stencil.back.compareMask = desc.DepthStencil.StencilReadMask;
				depth_stencil.back.writeMask = desc.DepthStencil.StencilWriteMask;
				depth_stencil.back.reference = 0;
				depth_stencil.minDepthBounds = depth_stencil.maxDepthBounds = 0.0f;
				color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				color_blend.pNext = 0;
				color_blend.flags = 0;
				color_blend.logicOpEnable = VK_FALSE;
				color_blend.logicOp = VK_LOGIC_OP_COPY;
				color_blend.attachmentCount = desc.RenderTargetCount;
				color_blend.pAttachments = blend_modes;
				color_blend.blendConstants[0] = color_blend.blendConstants[1] = 0.0f;
				color_blend.blendConstants[2] = color_blend.blendConstants[3] = 0.0f;
				dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamic_state.pNext = 0;
				dynamic_state.flags = 0;
				dynamic_state.dynamicStateCount = sizeof(dynamic_state_list) / sizeof(*dynamic_state_list);
				dynamic_state.pDynamicStates = dynamic_state_list;
				pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipeline_info.pNext = &attachments;
				pipeline_info.flags = 0;
				pipeline_info.stageCount = 2;
				pipeline_info.pStages = stages_info;
				pipeline_info.pVertexInputState = &vertex_input;
				pipeline_info.pInputAssemblyState = &input_assembler;
				pipeline_info.pTessellationState = &tesselation;
				pipeline_info.pViewportState = &viewport;
				pipeline_info.pRasterizationState = &rasterization;
				pipeline_info.pMultisampleState = &multisample;
				pipeline_info.pDepthStencilState = &depth_stencil;
				pipeline_info.pColorBlendState = &color_blend;
				pipeline_info.pDynamicState = &dynamic_state;
				pipeline_info.layout = pipeline_layout->pipeline_layout;
				pipeline_info.renderPass = 0;
				pipeline_info.subpass = 0;
				pipeline_info.basePipelineHandle = 0;
				pipeline_info.basePipelineIndex = -1;
				if (_api->Dispatch.vkCreateGraphicsPipelines(_api->Device, 0, 1, &pipeline_info, &_api->Base->Allocator, &state->_pipeline) != VK_SUCCESS) return 0;
				return oref<IPipelineState>(state);
			}
			virtual oref<ISamplerState> CreateSamplerState(const SamplerDesc & desc) noexcept override
			{
				auto state = owrap(new (std::nothrow) VKSamplerState(_api, this));
				if (!state) return 0;
				VkSamplerCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				info.pNext = 0;
				info.flags = 0;
				info.magFilter = CreateVkFilter(desc.MagnificationFilter);
				info.minFilter = CreateVkFilter(desc.MinificationFilter);
				info.mipmapMode = CreateVkMipmapMode(desc.MipFilter);
				info.addressModeU = CreateVkAddressMode(desc.AddressU);
				info.addressModeV = CreateVkAddressMode(desc.AddressV);
				info.addressModeW = CreateVkAddressMode(desc.AddressW);
				info.mipLodBias = 0.0f;
				if (desc.MagnificationFilter == SamplerFilter::Anisotropic && desc.MinificationFilter == SamplerFilter::Anisotropic && desc.MipFilter == SamplerFilter::Anisotropic) {
					info.anisotropyEnable = VK_TRUE;
					info.maxAnisotropy = desc.MaximalAnisotropy;
				} else {
					info.anisotropyEnable = VK_FALSE;
					info.maxAnisotropy = 0.0f;
				}
				info.compareEnable = VK_FALSE;
				info.compareOp = VK_COMPARE_OP_NEVER;
				info.minLod = desc.MinimalLOD;
				info.maxLod = desc.MaximalLOD;
				if (desc.BorderColor[3] < 0.5f) info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
				else if (desc.BorderColor[0] < 0.5f && desc.BorderColor[1] < 0.5f && desc.BorderColor[2] < 0.5f) info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
				else info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
				info.unnormalizedCoordinates = VK_FALSE;
				if (_api->Dispatch.vkCreateSampler(_api->Device, &info, &_api->Base->Allocator, &state->_sampler) != VK_SUCCESS) return 0;
				return oref<ISamplerState>(state);
			}
			virtual oref<IBuffer> CreateBuffer(const BufferDesc & desc) noexcept override { return oref<IBuffer>(_internal_create_buffer(desc, 0)); }
			virtual oref<IBuffer> CreateBufferWithData(const BufferDesc & desc, const ResourceInitDesc & init) noexcept override { return oref<IBuffer>(_internal_create_buffer(desc, &init)); }
			virtual oref<ITexture> CreateTexture(const TextureDesc & desc) noexcept override { return oref<ITexture>(_internal_create_texture(desc, 0, 0)); }
			virtual oref<ITexture> CreateTextureWithData(const TextureDesc & desc, const ResourceInitDesc * init) noexcept override { if (!init) return 0; return oref<ITexture>(_internal_create_texture(desc, init, 0)); }
			virtual oref<ITexture> CreateRenderTargetView(ITexture * texture, uint32 mip_level, uint32 array_offset_or_depth) noexcept override
			{
				if (!texture) return 0;
				auto result = owrap(new (std::nothrow) VKTexture(_api, this));
				if (!result) return 0;
				auto inner = static_cast<VKTexture *>(texture);
				if (!inner->_image) return 0;
				result->_desc = inner->_desc;
				result->_dependency.SetRetain(texture);
				if (mip_level >= inner->_desc.MipmapCount) return 0;
				if (result->_desc.Type == TextureType::Type1D) {
					result->_desc.MipmapCount = 1;
				} else if (result->_desc.Type == TextureType::TypeArray1D) {
					if (array_offset_or_depth >= inner->_desc.ArraySize) return 0;
					result->_desc.Type = TextureType::Type1D;
					result->_desc.MipmapCount = 1;
					result->_desc.ArraySize = 1;
				} else if (result->_desc.Type == TextureType::Type2D) {
					result->_desc.MipmapCount = 1;
				} else if (result->_desc.Type == TextureType::TypeArray2D) {
					if (array_offset_or_depth >= inner->_desc.ArraySize) return 0;
					result->_desc.Type = TextureType::Type2D;
					result->_desc.MipmapCount = 1;
					result->_desc.ArraySize = 1;
				} else if (result->_desc.Type == TextureType::TypeCube || result->_desc.Type == TextureType::TypeArrayCube) {
					if (array_offset_or_depth >= inner->_desc.ArraySize * 6) return 0;
					result->_desc.Type = TextureType::Type2D;
					result->_desc.MipmapCount = 1;
					result->_desc.ArraySize = 1;
				} else if (result->_desc.Type == TextureType::Type3D) {
					if (array_offset_or_depth >= inner->_desc.Depth) return 0;
					result->_desc.Type = TextureType::Type2D;
					result->_desc.MipmapCount = 1;
					result->_desc.ArraySize = 1;
				} else return 0;
				result->_image = inner->_image;
				if (!_texture_init_views(result, false, mip_level, array_offset_or_depth)) return 0;
				result->_image = 0;
				return oref<ITexture>(result);
			}
			virtual oref<IDeviceResource> OpenResource(IDeviceResourceHandle * handle) noexcept override
			{
				if (!handle || handle->GetDeviceIdentifier() != GetDeviceIdentifier()) return 0;
				oref<VKSharedObject> shared;
				try { shared = owrap(new VKSharedObject(static_cast<VKDeviceResourceHandle *>(handle))); } catch (...) { return 0; }
				VkPhysicalDeviceProperties2 prop;
				VkPhysicalDeviceIDProperties prop_id;
				prop.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
				prop.pNext = &prop_id;
				prop_id.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
				prop_id.pNext = 0;
				_api->Dispatch.vkGetPhysicalDeviceProperties2(_physical, &prop);
				if (Memory::MemoryCompare(&shared->_data->sharing.device_uuid, &prop_id.deviceUUID, 16) != 0) return 0;
				if (Memory::MemoryCompare(&shared->_data->sharing.driver_uuid, &prop_id.driverUUID, 16) != 0) return 0;
				TextureDesc desc;
				desc.Type = static_cast<TextureType>(shared->_data->meta.type);
				desc.Format = static_cast<PixelFormat>(shared->_data->meta.format);
				desc.Width = shared->_data->meta.width;
				desc.Height = shared->_data->meta.height;
				if (desc.Type == TextureType::Type3D) desc.Depth = shared->_data->meta.depth;
				else desc.ArraySize = shared->_data->meta.size;
				desc.MipmapCount = shared->_data->meta.mip_count;
				desc.Usage = shared->_data->meta.usage;
				desc.MemoryPool = ResourceMemoryPool::Shared;
				int devmem_fd = reinterpret_cast<intptr>(shared->_handle->_resource_fd);
				auto result = _internal_create_texture(desc, 0, &devmem_fd);
				if (result) result->_shared = shared;
				return oref<IDeviceResource>(result);
			}
			virtual oref<IPresentationLayer> CreatePresentationLayer(DynamicObject * presentor, const PresentationLayerDesc & desc) noexcept override
			{
				#ifdef ESSE_VULKAN_PRESENTATION
					if (!presentor || !IsColorFormat(desc.Format)) return 0;
					if (desc.Usage & ~(ResourceUsageRenderTarget | ResourceUsageShaderRead)) return 0;
					ErrorContext ectx; ErrorClear(ectx);
					auto window = owrap(reinterpret_cast<Windows::IWindow *>(presentor->DynamicCast(Classes.IWindow, ectx)));
					if (ErrorTest(ectx)) return 0;
					VKSurfaceClass * vk_surface_class;
					Memory::AcquireRootLock();
					if (!_common_vk_surface_class) _common_vk_surface_class = oref<Windows::IWindowExtensionClass>::CreateOwned(new (std::nothrow) VKSurfaceClass);
					vk_surface_class = static_cast<VKSurfaceClass *>(_common_vk_surface_class.Inner());
					Memory::ReleaseRootLock();
					if (!vk_surface_class) return 0;
					oref<VKSurface> vk_surface;
					// TODO: IMPLEMENT WAYLAND
					#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
						ErrorClear(ectx);
						auto window_x11 = reinterpret_cast<X11::IX11Window *>(presentor->DynamicCast(Linux::Classes::X11_Window, ectx));
						if (!ErrorTest(ectx)) {
							auto vk_x11_surface = owrap(new (std::nothrow) VKX11Surface(_api));
							if (!vk_x11_surface || !vk_x11_surface->Initialize(_physical, _dispatcher, window, window_x11)) return 0;
							vk_surface = vk_x11_surface.Inner();
						}
					#endif
					if (!vk_surface) return 0;
					window->RemoveExtension(vk_surface_class);
					if (!window->AddExtension(vk_surface, vk_surface_class)) return 0;
					auto layer = owrap(new (std::nothrow) VKPresentationLayer(_dispatcher, this, _physical));
					if (!layer) { window->RemoveExtension(vk_surface_class); return 0; }
					if (!layer->Initialize(window, vk_surface, desc)) { window->RemoveExtension(vk_surface_class); return 0; }
					return oref<IPresentationLayer>(layer);
				#else
					return 0;
				#endif
			}
			VkPhysicalDevice GetPhysicalDevice(void) noexcept { return _physical; }
			uint32 GetQueueFamilyIndex(void) noexcept { return _queue_family_index; }
			oref<VKConstantPool> AllocateConstantPool(void) noexcept
			{
				auto constant = owrap(new (std::nothrow) VKConstantPool(_api, this));
				if (!constant) return 0;
				constant->_desc.Usage = ResourceUsageConstantBuffer;
				constant->_desc.Stride = constant->_desc.Length = _vk_constant_buffer_size;
				constant->_desc.MemoryPool = ResourceMemoryPool::Regular;
				VkBufferCreateInfo info;
				info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				info.pNext = 0;
				info.flags = 0;
				info.size = _vk_constant_buffer_size;
				info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				info.queueFamilyIndexCount = 0;
				info.pQueueFamilyIndices = 0;
				if (_api->Dispatch.vkCreateBuffer(_api->Device, &info, &_api->Base->Allocator, &constant->_buffer) != VK_SUCCESS) return 0;
				VkMemoryRequirements memreq;
				VkMemoryAllocateInfo memalloc;
				_api->Dispatch.vkGetBufferMemoryRequirements(_api->Device, constant->_buffer, &memreq);
				memalloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memalloc.pNext = 0;
				memalloc.allocationSize = memreq.size;
				for (uint i = 0; i < _memory_info.memoryTypeCount; i++) if (memreq.memoryTypeBits & (1 << i)) {
					auto memprop = _memory_info.memoryTypes[i].propertyFlags;
					if ((memprop & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && (memprop & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
						memalloc.memoryTypeIndex = i;
						VkDeviceMemory memory;
						if (_api->Dispatch.vkAllocateMemory(_api->Device, &memalloc, &_api->Base->Allocator, &memory) == VK_SUCCESS) {
							constant->_memory = memory;
							break;
						}
					}
				}
				if (!constant->_memory) for (uint i = 0; i < _memory_info.memoryTypeCount; i++) if (memreq.memoryTypeBits & (1 << i)) {
					auto memprop = _memory_info.memoryTypes[i].propertyFlags;
					if (memprop & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
						memalloc.memoryTypeIndex = i;
						VkDeviceMemory memory;
						if (_api->Dispatch.vkAllocateMemory(_api->Device, &memalloc, &_api->Base->Allocator, &memory) == VK_SUCCESS) {
							constant->_memory = memory;
							break;
						}
					}
				}
				if (!constant->_memory) return 0;
				if (_api->Dispatch.vkBindBufferMemory(_api->Device, constant->_buffer, constant->_memory, 0) != VK_SUCCESS) return 0;
				constant->allocated = _vk_constant_buffer_size;
				constant->used = constant->offset = 0;
				if (!constant->MapMemory()) return 0;
				return constant;
			}
			VulkanDeviceClass GetDeviceClassVK(void) noexcept
			{
				VkPhysicalDeviceProperties prop;
				_api->Dispatch.vkGetPhysicalDeviceProperties(_physical, &prop);
				if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return VulkanDeviceDiscrete;
				else if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) return VulkanDeviceIntegrated;
				else if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) return VulkanDeviceVirtual;
				else if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) return VulkanDeviceSoftware;
				else return VulkanDeviceOther;
			}
		};
		oref<VKConstantPool> VKConstantPool::Allocate(IDevice * parent_device) noexcept { return static_cast<VKDevice *>(parent_device)->AllocateConstantPool(); }
		oref<VK2DCommonResources> VK2DCommonResources::GetCommonResources(IDevice * device) noexcept
		{
			auto dev = static_cast<VKDevice *>(device);
			if (!dev->_common) try {
				auto result = owrap(new VK2DCommonResources);
				const void * sdd;
				uintptr sdl;
				GetShaders(&sdd, &sdl);
				result->library = device->LoadShaderLibraryFromData(sdd, sdl);
				auto main_vertex = result->library->CreateShader("v_primus");
				auto main_vertex_gradient = result->library->CreateShader("v_gradi");
				auto tile_vertex = result->library->CreateShader("v_taleae");
				auto blur_vertex = result->library->CreateShader("v_maculae");
				auto main_pixel = result->library->CreateShader("p_primus");
				auto main_pixel_no_alpha = result->library->CreateShader("p_primus_alpha_nulla");
				auto main_pixel_double_alpha = result->library->CreateShader("p_primus_dua_alpha");
				auto tile_pixel = result->library->CreateShader("p_taleae");
				auto blur_pixel = result->library->CreateShader("p_maculae");
				if (!main_vertex || !main_pixel || !main_vertex_gradient || !main_pixel_no_alpha || !main_pixel_double_alpha || !tile_vertex || !tile_pixel || !blur_vertex || !blur_pixel) throw Exception();
				PipelineStateDesc pipeline;
				pipeline.VertexShader = main_vertex;
				pipeline.PixelShader = main_pixel;
				pipeline.RenderTargetCount = 1;
				pipeline.RenderTarget[0].Format = PixelFormat::B8G8R8A8_unorm;
				pipeline.RenderTarget[0].Flags = RenderTargetFlagBlendingEnabled;
				pipeline.RenderTarget[0].BlendRGB = pipeline.RenderTarget[0].BlendAlpha = BlendingFunction::Add;
				pipeline.RenderTarget[0].BaseFactorRGB = pipeline.RenderTarget[0].BaseFactorAlpha = BlendingFactor::InvertedOverAlpha;
				pipeline.RenderTarget[0].OverFactorRGB = pipeline.RenderTarget[0].OverFactorAlpha = BlendingFactor::One;
				pipeline.DepthStencil.Flags = 0;
				pipeline.Rasterization.Cull = CullMode::None;
				pipeline.Rasterization.Fill = FillMode::Solid;
				pipeline.Rasterization.FrontIsCounterClockwise = true;
				pipeline.Rasterization.DepthBias = 0.0f;
				pipeline.Rasterization.DepthBiasClamp = 0.0f;
				pipeline.Rasterization.DepthClipEnable = false;
				pipeline.Rasterization.SlopeScaledDepthBias = 0.0f;
				pipeline.Topology = PrimitiveTopology::TriangleList;
				result->main_alpha_state = device->CreateRenderingPipelineState(pipeline);
				if (!result->main_alpha_state) return 0;
				pipeline.PixelShader = main_pixel_double_alpha;
				result->main_double_alpha_state = device->CreateRenderingPipelineState(pipeline);
				if (!result->main_double_alpha_state) return 0;
				pipeline.VertexShader = main_vertex_gradient;
				pipeline.PixelShader = main_pixel;
				result->gradient_state = device->CreateRenderingPipelineState(pipeline);
				if (!result->gradient_state) return 0;
				pipeline.VertexShader = main_vertex;
				pipeline.PixelShader = main_pixel_no_alpha;
				pipeline.RenderTarget[0].Flags = 0;
				result->main_opaque_state = device->CreateRenderingPipelineState(pipeline);
				if (!result->main_opaque_state) return 0;
				pipeline.VertexShader = tile_vertex;
				pipeline.PixelShader = tile_pixel;
				pipeline.RenderTarget[0].Flags = RenderTargetFlagBlendingEnabled;
				result->tile_state = device->CreateRenderingPipelineState(pipeline);
				if (!result->tile_state) return 0;
				pipeline.VertexShader = main_vertex;
				pipeline.PixelShader = main_pixel;
				pipeline.RenderTarget[0].BlendRGB = BlendingFunction::SubtractBaseFromOver;
				pipeline.RenderTarget[0].BaseFactorAlpha = BlendingFactor::One;
				pipeline.RenderTarget[0].OverFactorAlpha = BlendingFactor::Zero;
				pipeline.RenderTarget[0].BaseFactorRGB = BlendingFactor::One;
				pipeline.RenderTarget[0].OverFactorRGB = BlendingFactor::BaseAlpha;
				result->invert_state = device->CreateRenderingPipelineState(pipeline);
				if (!result->invert_state) return 0;
				pipeline.VertexShader = blur_vertex;
				pipeline.PixelShader = blur_pixel;
				pipeline.RenderTarget[0].Flags = 0;
				result->blur_state = device->CreateRenderingPipelineState(pipeline);
				if (!result->blur_state) return 0;
				Color white = 0xFFFFFFFF;
				ResourceInitDesc init;
				TextureDesc tdesc;
				tdesc.Type = TextureType::Type2D;
				tdesc.Format = PixelFormat::B8G8R8A8_unorm;
				tdesc.Width = tdesc.Height = tdesc.MipmapCount = 1;
				tdesc.Usage = ResourceUsageShaderRead;
				tdesc.MemoryPool = ResourceMemoryPool::Immutable;
				init.Data = &white;
				init.DataPitch = sizeof(white);
				result->white = device->CreateTextureWithData(tdesc, &init);
				result->area = VKDeviceContext2D::_create_area_buffer(device, white);
				if (!result->white || !result->area) throw Exception();
				dev->_common = result;
			} catch (...) { return 0; }
			return dev->_common;
		}

		class VKDeviceFactory : public IDeviceFactory
		{
			oref<VKAPI> _common_vk_api;
		private:
			void _set_up_required_features(VkPhysicalDeviceFeatures & f10, VkPhysicalDeviceVulkan11Features & f11, VkPhysicalDeviceVulkan12Features & f12, VkPhysicalDeviceVulkan13Features & f13, VkPhysicalDeviceExtendedDynamicStateFeaturesEXT & fds) noexcept
			{
				if (_device_validation_layer) {
					if (!f10.robustBufferAccess) VKValidationOutput("Vulkan API: functio nulla: \"robustBufferAccess\".\n");
					if (!f10.fullDrawIndexUint32) VKValidationOutput("Vulkan API: functio nulla: \"fullDrawIndexUint32\".\n");
					if (!f10.imageCubeArray) VKValidationOutput("Vulkan API: functio nulla: \"imageCubeArray\".\n");
					if (!f10.independentBlend) VKValidationOutput("Vulkan API: functio nulla: \"independentBlend\".\n");
					if (!f10.dualSrcBlend) VKValidationOutput("Vulkan API: functio nulla: \"dualSrcBlend\".\n");
					if (!f10.depthClamp) VKValidationOutput("Vulkan API: functio nulla: \"depthClamp\".\n");
					if (!f10.depthBiasClamp) VKValidationOutput("Vulkan API: functio nulla: \"depthBiasClamp\".\n");
					if (!f10.fillModeNonSolid) VKValidationOutput("Vulkan API: functio nulla: \"fillModeNonSolid\".\n");
					if (!f10.multiViewport) VKValidationOutput("Vulkan API: functio necessaria nulla: \"multiViewport\".\n");
					if (!f10.samplerAnisotropy) VKValidationOutput("Vulkan API: functio nulla: \"samplerAnisotropy\".\n");
					if (!f12.descriptorBindingPartiallyBound) VKValidationOutput("Vulkan API: functio nulla: \"descriptorBindingPartiallyBound\".\n");
					if (!f12.scalarBlockLayout) VKValidationOutput("Vulkan API: functio necessaria nulla: \"scalarBlockLayout\".\n");
					if (!f13.robustImageAccess) VKValidationOutput("Vulkan API: functio nulla: \"robustImageAccess\".\n");
					if (!f13.synchronization2) VKValidationOutput("Vulkan API: functio necessaria nulla: \"synchronization2\".\n");
					if (!f13.dynamicRendering) VKValidationOutput("Vulkan API: functio necessaria nulla: \"dynamicRendering\".\n");
					if (!fds.extendedDynamicState) VKValidationOutput("Vulkan API: functio necessaria nulla: \"extendedDynamicState\".\n");
				}
				VkPhysicalDeviceFeatures new_f10;
				Memory::ZeroMemory(&new_f10, sizeof(new_f10));
				new_f10.robustBufferAccess = f10.robustBufferAccess;
				new_f10.fullDrawIndexUint32 = f10.fullDrawIndexUint32;
				new_f10.imageCubeArray = f10.imageCubeArray;
				new_f10.independentBlend = f10.independentBlend;
				new_f10.dualSrcBlend = f10.dualSrcBlend;
				new_f10.depthClamp = f10.depthClamp;
				new_f10.depthBiasClamp = f10.depthBiasClamp;
				new_f10.fillModeNonSolid = f10.fillModeNonSolid;
				new_f10.multiViewport = true;
				new_f10.samplerAnisotropy = f10.samplerAnisotropy;
				Memory::MemoryCopy(&f10, &new_f10, sizeof(f10));
				f11.storageBuffer16BitAccess = f11.uniformAndStorageBuffer16BitAccess = f11.storagePushConstant16 = f11.storageInputOutput16 = false;
				f11.multiview = f11.multiviewGeometryShader = f11.multiviewTessellationShader = false;
				f11.variablePointersStorageBuffer = f11.variablePointers = f11.protectedMemory = false;
				f11.samplerYcbcrConversion = f11.shaderDrawParameters = false;
				VkPhysicalDeviceVulkan12Features new_f12;
				Memory::ZeroMemory(&new_f12, sizeof(new_f12));
				new_f12.sType = f12.sType; new_f12.pNext = f12.pNext;
				new_f12.descriptorBindingPartiallyBound = f12.descriptorBindingPartiallyBound;
				new_f12.scalarBlockLayout = true;
				Memory::MemoryCopy(&f12, &new_f12, sizeof(f12));
				VkPhysicalDeviceVulkan13Features new_f13;
				Memory::ZeroMemory(&new_f13, sizeof(new_f13));
				new_f13.sType = f13.sType; new_f13.pNext = f13.pNext;
				new_f13.robustImageAccess = f13.robustImageAccess;
				new_f13.synchronization2 = true;
				new_f13.dynamicRendering = true;
				Memory::MemoryCopy(&f13, &new_f13, sizeof(f13));
				fds.extendedDynamicState = true;
			}
			oref<IDevice> _internal_create_device(VkPhysicalDevice phys) noexcept
			{
				auto api = owrap(new (std::nothrow) VKDeviceAPI);
				auto result = owrap(new (std::nothrow) VKDevice);
				if (!api || !result) return 0;
				int queue_index = -1;
				float queue_priority = 1.0f;
				VkDevice device;
				VkDeviceCreateInfo device_info;
				VkPhysicalDeviceFeatures2 device_features_2;
				VkPhysicalDeviceVulkan11Features device_features_11;
				VkPhysicalDeviceVulkan12Features device_features_12;
				VkPhysicalDeviceVulkan13Features device_features_13;
				VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamic_state;
				VkDeviceQueueCreateInfo device_queue;
				try {
					array<VkQueueFamilyProperties> queues(1);
					uint32 count;
					_common_vk_api->Dispatch.vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, 0);
					queues.SetLength(count);
					_common_vk_api->Dispatch.vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, queues);
					if (count < queues.GetLength()) queues.SetLength(count);
					for (int i = 0; i < queues.GetLength(); i++) {
						auto & q = queues[i];
						auto & e = q.minImageTransferGranularity;
						if (q.queueFlags & VK_QUEUE_GRAPHICS_BIT && e.width == 1 && e.height == 1 && e.depth == 1) { queue_index = i; break; }
					}
					if (queue_index < 0) return 0;
					device_queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					device_queue.pNext = 0;
					device_queue.flags = 0;
					device_queue.queueFamilyIndex = queue_index;
					device_queue.queueCount = 1;
					device_queue.pQueuePriorities = &queue_priority;
				} catch (...) { return 0; }
				array<const char *> desired_extensions(_vk_extensions_slots);
				try {
					array<VkExtensionProperties> extensions(1);
					uint32 extensions_count;
					uint32 desired_extensions_count = sizeof(_vk_desired_device_extensions) / sizeof(_vk_desired_device_extensions[0]);
					if (_common_vk_api->Dispatch.vkEnumerateDeviceExtensionProperties(phys, 0, &extensions_count, 0) < 0) return 0;
					extensions.SetLength(extensions_count);
					if (_common_vk_api->Dispatch.vkEnumerateDeviceExtensionProperties(phys, 0, &extensions_count, extensions) < 0) return 0;
					if (extensions_count < extensions.GetLength()) extensions.SetLength(extensions_count);
					for (uint i = 0; i < desired_extensions_count; i++) {
						bool supported = false;
						for (uint j = 0; j < extensions.GetLength(); j++) if (strcmp(_vk_desired_device_extensions[i], extensions[j].extensionName) == 0) {
							supported = true;
							break;
						}
						if (supported) desired_extensions.Append(_vk_desired_device_extensions[i]);
						else if (_device_validation_layer) VKValidationOutput("Vulkan API: Extensio machinationis nulla: %s\n", _vk_desired_device_extensions[i]);
					}
				} catch (...) { return 0; }
				device_features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				device_features_2.pNext = &device_features_11;
				device_features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
				device_features_11.pNext = &device_features_12;
				device_features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
				device_features_12.pNext = &device_features_13;
				device_features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
				device_features_13.pNext = &dynamic_state;
				dynamic_state.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
				dynamic_state.pNext = 0;
				_common_vk_api->Dispatch.vkGetPhysicalDeviceFeatures2(phys, &device_features_2);
				_set_up_required_features(device_features_2.features, device_features_11, device_features_12, device_features_13, dynamic_state);
				device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				device_info.pNext = &device_features_2;
				device_info.flags = 0;
				device_info.queueCreateInfoCount = 1;
				device_info.pQueueCreateInfos = &device_queue;
				device_info.enabledLayerCount = 0;
				device_info.ppEnabledLayerNames = 0;
				device_info.enabledExtensionCount = desired_extensions.GetLength();
				device_info.ppEnabledExtensionNames = desired_extensions;
				device_info.pEnabledFeatures = 0;
				if (_common_vk_api->Dispatch.vkCreateDevice(phys, &device_info, &_common_vk_api->Allocator, &device) != VK_SUCCESS) return 0;
				api->Initialize(_common_vk_api, device);
				if (!result->Initialize(api, phys, queue_index)) return 0;
				return oref<IDevice>(result);
			}
			static bool _mask_match(VkPhysicalDeviceType type, uint mask) noexcept
			{
				if (type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return (mask & VulkanDeviceDiscrete) != 0;
				else if (type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) return (mask & VulkanDeviceIntegrated) != 0;
				else if (type == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) return (mask & VulkanDeviceVirtual) != 0;
				else if (type == VK_PHYSICAL_DEVICE_TYPE_CPU) return (mask & VulkanDeviceSoftware) != 0;
				else return (mask & VulkanDeviceOther) != 0;
			}
			static bool _version_match(const VkPhysicalDeviceProperties & prop) noexcept
			{
				if (VK_API_VERSION_MAJOR(prop.apiVersion) > 1) return true;
				if (VK_API_VERSION_MAJOR(prop.apiVersion) == 1 && VK_API_VERSION_MINOR(prop.apiVersion) >= 3) return true;
				return false;
			}
		public:
			VKDeviceFactory(void) { _common_vk_api = owrap(new VKAPI); }
			virtual ~VKDeviceFactory(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"VKDeviceFactory"; ESSE_TRY_OUTRO(string()) }
			virtual oref<Dictionary<uint64, string>> EnumerateDevices(void) noexcept override
			{
				try {
					auto result = owrap(new Dictionary<uint64, string>);
					array<VkPhysicalDevice> devices(1);
					uint32 count;
					if (_common_vk_api->Dispatch.vkEnumeratePhysicalDevices(_common_vk_api->Instance, &count, 0) < 0) return 0;
					devices.SetLength(count);
					if (_common_vk_api->Dispatch.vkEnumeratePhysicalDevices(_common_vk_api->Instance, &count, devices) < 0) return 0;
					if (count < devices.GetLength()) devices.SetLength(count);
					for (auto & dev : devices) {
						VkPhysicalDeviceProperties prop;
						_common_vk_api->Dispatch.vkGetPhysicalDeviceProperties(dev, &prop);
						if (!_version_match(prop) || !_mask_match(prop.deviceType, _device_enumeration_mask)) continue;
						uint64 devid = (uint64(prop.vendorID) << 32) | uint64(prop.deviceID);
						try { result->Append(devid, string(prop.deviceName)); } catch (...) {}
					}
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual oref<IDevice> CreateDevice(uint64 identifier) noexcept override
			{
				try {
					array<VkPhysicalDevice> devices(1);
					uint32 count;
					if (_common_vk_api->Dispatch.vkEnumeratePhysicalDevices(_common_vk_api->Instance, &count, 0) < 0) return 0;
					devices.SetLength(count);
					if (_common_vk_api->Dispatch.vkEnumeratePhysicalDevices(_common_vk_api->Instance, &count, devices) < 0) return 0;
					if (count < devices.GetLength()) devices.SetLength(count);
					VkPhysicalDevice phys = 0;
					for (auto & dev : devices) {
						VkPhysicalDeviceProperties prop;
						_common_vk_api->Dispatch.vkGetPhysicalDeviceProperties(dev, &prop);
						if (_version_match(prop)) {
							uint64 devid = (uint64(prop.vendorID) << 32) | uint64(prop.deviceID);
							if (devid == identifier) { phys = dev; break; }
						}
					}
					return phys ? _internal_create_device(phys) : oref<IDevice>();
				} catch (...) { return 0; }
			}
			virtual oref<IDevice> CreateDefaultDevice(void) noexcept override
			{
				try {
					array<VkPhysicalDevice> devices(1);
					uint32 count;
					if (_common_vk_api->Dispatch.vkEnumeratePhysicalDevices(_common_vk_api->Instance, &count, 0) < 0) return 0;
					devices.SetLength(count);
					if (_common_vk_api->Dispatch.vkEnumeratePhysicalDevices(_common_vk_api->Instance, &count, devices) < 0) return 0;
					if (count < devices.GetLength()) devices.SetLength(count);
					VkPhysicalDevice phys = 0;
					for (auto & dev : devices) {
						VkPhysicalDeviceProperties prop;
						_common_vk_api->Dispatch.vkGetPhysicalDeviceProperties(dev, &prop);
						if (!_version_match(prop)) continue;
						if (_mask_match(prop.deviceType, _default_device_desired)) { phys = dev; break; }
					}
					if (!phys) for (auto & dev : devices) {
						VkPhysicalDeviceProperties prop;
						_common_vk_api->Dispatch.vkGetPhysicalDeviceProperties(dev, &prop);
						if (!_version_match(prop)) continue;
						if (!_mask_match(prop.deviceType, _default_device_allowed)) continue;
						if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) { phys = dev; break; }
					}
					if (!phys) for (auto & dev : devices) {
						VkPhysicalDeviceProperties prop;
						_common_vk_api->Dispatch.vkGetPhysicalDeviceProperties(dev, &prop);
						if (!_version_match(prop)) continue;
						if (!_mask_match(prop.deviceType, _default_device_allowed)) continue;
						if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { phys = dev; break; }
					}
					if (!phys) for (auto & dev : devices) {
						VkPhysicalDeviceProperties prop;
						_common_vk_api->Dispatch.vkGetPhysicalDeviceProperties(dev, &prop);
						if (!_version_match(prop)) continue;
						if (!_mask_match(prop.deviceType, _default_device_allowed)) continue;
						if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) { phys = dev; break; }
					}
					if (!phys) for (auto & dev : devices) {
						VkPhysicalDeviceProperties prop;
						_common_vk_api->Dispatch.vkGetPhysicalDeviceProperties(dev, &prop);
						if (!_version_match(prop)) continue;
						if (!_mask_match(prop.deviceType, _default_device_allowed)) continue;
						phys = dev;
						break;
					}
					return phys ? _internal_create_device(phys) : oref<IDevice>();
				} catch (...) { return 0; }
			}
			virtual oref<IDeviceResourceHandle> QueryResourceHandle(IDeviceResource * resource) noexcept override
			{
				if (!resource || resource->GetResourceType() != ResourceType::Texture || resource->GetMemoryPool() != ResourceMemoryPool::Shared) return 0;
				VKSharedObject * shared = static_cast<VKTexture *>(resource)->_shared;
				if (!shared || !shared->_handle) return 0;
				return oref<IDeviceResourceHandle>(shared->_handle);
			}
			virtual oref<IDeviceResourceHandle> ReceiveResourceHandle(IPC::IConnection * con, ErrorContext & ectx) noexcept override
			{
				if (!con) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
				uint64 devid;
				con->ReceiveData(&devid, sizeof(devid), ectx);
				if (ErrorTest(ectx)) return 0;
				auto rsrc = con->ReceiveHandle(ectx);
				if (ErrorTest(ectx)) return 0;
				auto hmem = con->ReceiveHandle(ectx);
				if (ErrorTest(ectx)) { IO::CloseHandle(rsrc); return 0; }
				auto mem = IPC::OpenSharedMemory(hmem, sizeof(VKSharedObject::_shared_object_desc), ectx);
				if (ErrorTest(ectx)) { IO::CloseHandle(rsrc); IO::CloseHandle(hmem); return 0; }
				auto result = oref<IDeviceResourceHandle>::CreateOwned(new (std::nothrow) VKDeviceResourceHandle(devid, rsrc, mem));
				if (!result) { IO::CloseHandle(rsrc); ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
				return result;
			}
		};

		void GetDefaultDevicePriority(uint & desired, uint & allowed) noexcept { desired = _default_device_desired; allowed = _default_device_allowed; }
		void SetDefaultDevicePriority(uint desired, uint allowed) noexcept { _default_device_desired = desired; _default_device_allowed = allowed; }
		void GetDeviceEnumerationMask(uint & mask) noexcept { mask = _device_enumeration_mask; }
		void SetDeviceEnumerationMask(uint mask) noexcept { _device_enumeration_mask = mask; }
		void GetDeviceValidationLayer(bool & set) noexcept { set = _device_validation_layer; }
		void SetDeviceValidationLayer(bool set) noexcept { _device_validation_layer = set; }

		void PrecompileMakeLog(VKCompilerAPI * compiler, glslang_shader_t * shader, glslang_program_t * module, oref<DataBlock> & log) noexcept
		{
			const char * shader_log = 0;
			const char * module_log = 0;
			if (shader) shader_log = compiler->glslang_shader_get_info_log(shader);
			if (module) module_log = compiler->glslang_program_get_info_log(module);
			try {
				log = owrap(new DataBlock(0x400));
				if (shader_log) log->Append(reinterpret_cast<const uint8 *>(shader_log), strlen(shader_log));
				if (module_log) log->Append(reinterpret_cast<const uint8 *>(module_log), strlen(module_log));
			} catch (...) {}
		}
		oref<DataBlock> PrecompileShadersSource(VKCompilerAPI * compiler, const void * data, uintptr length, const VulkanInputDesc & desc, oref<DataBlock> & log, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				if (length < 0 || !data) throw InvalidArgumentException();
				array<uint> mapping(1);
				array<char> code(1);
				oref<DataBlock> result;
				code.SetLength(length + 1);
				Memory::MemoryCopy(code.GetBuffer(), data, length);
				code[length] = 0;
				ReadShaderResourceMapping(code, mapping, ectx);
				if (ErrorTest(ectx)) return 0;
				glslang_input_t input;
				input.language = GLSLANG_SOURCE_GLSL;
				if (desc.InputClass == VulkanInputGLSLVertexFunction) input.stage = GLSLANG_STAGE_VERTEX;
				else if (desc.InputClass == VulkanInputGLSLPixelFunction) input.stage = GLSLANG_STAGE_FRAGMENT;
				else throw InvalidFormatException();
				input.client = GLSLANG_CLIENT_VULKAN;
				input.client_version = static_cast<glslang_target_client_version_t>((desc.VulkanVersionMajor << 22) | (desc.VulkanVersionMinor << 12));
				input.target_language = GLSLANG_TARGET_SPV;
				input.target_language_version = static_cast<glslang_target_language_version_t>((desc.SPIRVVersionMajor << 16) | (desc.SPIRVVersionMinor << 8));
				input.code = code;
				input.default_version = (desc.GLSLVersionMajor * 100) + desc.GLSLVersionMinor;
				input.default_profile = GLSLANG_NO_PROFILE;
				input.force_default_version_and_profile = false;
				input.forward_compatible = false;
				input.messages = GLSLANG_MSG_DEFAULT_BIT;
				input.resource = compiler->glslang_default_resource();
				input.callbacks.free_include_result = 0;
				input.callbacks.include_local = 0;
				input.callbacks.include_system = 0;
				input.callbacks_ctx = 0;
				auto context = compiler->glslang_shader_create(&input);
				if (!context) throw NotImplementedException();
				if (!compiler->glslang_shader_preprocess(context, &input)) {
					if (log) PrecompileMakeLog(compiler, context, 0, log);
					compiler->glslang_shader_delete(context);
					throw CustomException(ErrorMake(Errores::ErrorDynamicLinkage, Errores::SuberrorDL::InvalidFunctionFormat));
				}
				if (!compiler->glslang_shader_parse(context, &input)) {
					if (log) PrecompileMakeLog(compiler, context, 0, log);
					compiler->glslang_shader_delete(context);
					throw CustomException(ErrorMake(Errores::ErrorDynamicLinkage, Errores::SuberrorDL::InvalidFunctionFormat));
				}
				auto module_context = compiler->glslang_program_create();
				if (!module_context) {
					compiler->glslang_shader_delete(context);
					throw NotImplementedException();
				}
				compiler->glslang_program_add_shader(module_context, context);
				if (!compiler->glslang_program_link(module_context, GLSLANG_MSG_DEFAULT_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
					if (log) PrecompileMakeLog(compiler, context, module_context, log);
					compiler->glslang_program_delete(module_context);
					compiler->glslang_shader_delete(context);
					throw CustomException(ErrorMake(Errores::ErrorDynamicLinkage, Errores::SuberrorDL::LinkageFailure));
				}
				compiler->glslang_program_SPIRV_generate(module_context, input.stage);
				try {
					auto length = (compiler->glslang_program_SPIRV_get_size(module_context) + mapping.GetLength() + 1) * 4;
					auto mapping_length = mapping.GetLength() * 4;
					result = owrap(new DataBlock(length));
					result->SetLength(length);
					Memory::MemoryCopy(result->GetBuffer(), mapping.GetBuffer(), mapping_length);
					result->ElementAt(mapping_length) = result->ElementAt(mapping_length + 1) = result->ElementAt(mapping_length + 2) = result->ElementAt(mapping_length + 3) = 0;
					compiler->glslang_program_SPIRV_get(module_context, reinterpret_cast<uint32 *>(result->GetBuffer() + mapping_length + 4));
				} catch (...) {
					compiler->glslang_program_delete(module_context);
					compiler->glslang_shader_delete(context);
					throw OutOfMemoryException();
				}
				if (log) PrecompileMakeLog(compiler, context, module_context, log);
				compiler->glslang_program_delete(module_context);
				compiler->glslang_shader_delete(context);
				return result;
			ESSE_TRY_OUTRO(0)
		}
		oref<DataBlock> PrecompileShadersUniversal(VKCompilerAPI * compiler, const void * data, uintptr length, const VulkanInputDesc & desc, oref<DataBlock> & log, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				if (!data || length < 0) throw InvalidArgumentException();
				auto stream_in = StaticMemoryStream::Create(data, length);
				auto stream_out = MemoryStream::Create(0x10000);
				auto arc_in = Formationes::Archive::Open(stream_in);
				int file_count = arc_in->GetFileCount();
				array<string> output_names(0x40);
				array<uint> output_specs(0x40);
				object_array<DataBlock> output_data(0x40);
				for (uint file = 1; file <= file_count; file++) {
					auto user = arc_in->GetFileUserData(file);
					if ((user & 0xFF0000) != 0x30000) continue;
					VulkanInputDesc subdesc = desc;
					string shader_name;
					if ((user & 0xFFFF) == 0x0001) subdesc.InputClass = VulkanInputGLSLVertexFunction;
					else if ((user & 0xFFFF) == 0x0002) subdesc.InputClass = VulkanInputGLSLPixelFunction;
					else throw InvalidFormatException();
					shader_name = arc_in->GetFileName(file);
					auto shader_stream = arc_in->QueryFileStream(file, Formationes::ArchiveStream::Native);
					auto shader_code = shader_stream->ReadAll();
					auto spirv = PrecompileShadersSource(compiler, shader_code->GetBuffer(), shader_code->GetLength(), subdesc, log, ectx);
					if (ErrorTest(ectx)) return 0;
					output_names.Append(shader_name);
					output_specs.Append(user & 0xFFFF);
					output_data.Append(spirv);
				}
				auto arc_out = Formationes::NewArchive::Create(stream_out, output_names.GetLength(), Formationes::ArchiveFlags::Create32bit);
				for (uint i = 0; i < output_names.GetLength(); i++) {
					arc_out->SetFileName(i + 1, output_names[i]);
					arc_out->SetFileUserData(i + 1, output_specs[i]);
					arc_out->SetFileData(i + 1, output_data[i].GetBuffer(), output_data[i].GetLength());
				}
				arc_out->Finalize();
				return stream_out->GetStorage();
			ESSE_TRY_OUTRO(0)
		}
		oref<DataBlock> PrecompileShaders(const void * data, uintptr length, const VulkanInputDesc & desc, oref<DataBlock> & log, ErrorContext & ectx) noexcept
		{
			log.Clear();
			oref<VKCompilerAPI> compiler;
			try { compiler = owrap(new VKCompilerAPI); } catch (...) { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			if (desc.InputClass == VulkanInputUniversalShaderBundle) return PrecompileShadersUniversal(compiler, data, length, desc, log, ectx);
			else return PrecompileShadersSource(compiler, data, length, desc, log, ectx);
		}
		VulkanDeviceClass GetVulkanDeviceClass(Graphica::IDevice * device) noexcept{ return static_cast<VKDevice *>(device)->GetDeviceClassVK(); }
		oref<Graphica::IDeviceContext2D> PrecreateContext2D(Graphica::IDeviceContext * context) noexcept
		{
			if (context->IsDeferred()) static_cast<VKDeviceDeferredContext *>(context)->PrecreateContext2D();
			else static_cast<VKDeviceImmediateContext *>(context)->PrecreateContext2D();
			ErrorContext ectx; ErrorClear(ectx);
			auto result = owrap(reinterpret_cast<Graphica::IDeviceContext2D *>(context->DynamicCast(Classes.IDeviceContext2D, ectx)));
			if (ErrorTest(ectx)) return 0;
			return result;
		}
		oref<Graphica::IDeviceFactory> CreateDeviceFactory(ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				Memory::AcquireRootLock();
				auto result = _common_device_factory;
				if (!result) try { result = _common_device_factory = oref<IDeviceFactory>::CreateOwned(new VKDeviceFactory); }
				catch (...) { Memory::ReleaseRootLock(); throw; }
				Memory::ReleaseRootLock();
				return result;
			ESSE_TRY_OUTRO(0)
		}
	}
}