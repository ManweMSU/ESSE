#include <Imagines/Imagines.h>
#include <stdio.h>
#include <dlfcn.h>
#include <setjmp.h>

using namespace ESSE::Picturae;

namespace ESSE
{
	namespace Linux
	{
		namespace libpng
		{
			#define PNG_LIBPNG_VER_STRING "1.6.0"

			#define PNG_TRANSFORM_IDENTITY				0x0000    /* read and write */
			#define PNG_TRANSFORM_STRIP_16				0x0001    /* read only */
			#define PNG_TRANSFORM_STRIP_ALPHA			0x0002    /* read only */
			#define PNG_TRANSFORM_PACKING				0x0004    /* read and write */
			#define PNG_TRANSFORM_PACKSWAP				0x0008    /* read and write */
			#define PNG_TRANSFORM_EXPAND				0x0010    /* read only */
			#define PNG_TRANSFORM_INVERT_MONO			0x0020    /* read and write */
			#define PNG_TRANSFORM_SHIFT					0x0040    /* read and write */
			#define PNG_TRANSFORM_BGR					0x0080    /* read and write */
			#define PNG_TRANSFORM_SWAP_ALPHA			0x0100    /* read and write */
			#define PNG_TRANSFORM_SWAP_ENDIAN			0x0200    /* read and write */
			#define PNG_TRANSFORM_INVERT_ALPHA			0x0400    /* read and write */
			#define PNG_TRANSFORM_STRIP_FILLER			0x0800    /* write only */
			#define PNG_TRANSFORM_STRIP_FILLER_BEFORE	PNG_TRANSFORM_STRIP_FILLER
			#define PNG_TRANSFORM_STRIP_FILLER_AFTER	0x1000 /* write only */
			#define PNG_TRANSFORM_GRAY_TO_RGB			0x2000      /* read only */
			#define PNG_TRANSFORM_EXPAND_16				0x4000      /* read only */
			#define PNG_TRANSFORM_SCALE_16				0x8000      /* read only */

			#define PNG_COLOR_MASK_PALETTE		1
			#define PNG_COLOR_MASK_COLOR		2
			#define PNG_COLOR_MASK_ALPHA		4
			#define PNG_COLOR_TYPE_GRAY			0
			#define PNG_COLOR_TYPE_PALETTE		(PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
			#define PNG_COLOR_TYPE_RGB			(PNG_COLOR_MASK_COLOR)
			#define PNG_COLOR_TYPE_RGB_ALPHA	(PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
			#define PNG_COLOR_TYPE_GRAY_ALPHA	(PNG_COLOR_MASK_ALPHA)
			#define PNG_COLOR_TYPE_RGBA			PNG_COLOR_TYPE_RGB_ALPHA
			#define PNG_COLOR_TYPE_GA			PNG_COLOR_TYPE_GRAY_ALPHA

			typedef struct __jmp_buf_tag jmp_buf[1];
			typedef void (* png_error_ptr) (handle, const char *) noexcept;
			typedef void (* png_longjmp_ptr) (jmp_buf, int) noexcept;
			typedef void (* png_rw_ptr) (handle, uint8 *, uintptr) noexcept;
			typedef void (* png_flush_ptr) (handle) noexcept;

			#define DEFINE_PNG_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
			#define DEFINE_PNG_FUNCTION_IMPORT(NAME) libpng::NAME = reinterpret_cast<libpng::func_##NAME>(dlsym(libpng::library, #NAME));

			bool initialized = false;
			handle library;

			DEFINE_PNG_FUNCTION_POINTER(png_sig_cmp, int, (const uint8 *, uintptr, uintptr))
			DEFINE_PNG_FUNCTION_POINTER(png_create_read_struct, handle, (const char *, void *, png_error_ptr, png_error_ptr))
			DEFINE_PNG_FUNCTION_POINTER(png_create_info_struct, handle, (handle))
			DEFINE_PNG_FUNCTION_POINTER(png_destroy_read_struct, void, (handle *, handle *, handle *))
			DEFINE_PNG_FUNCTION_POINTER(png_destroy_info_struct, void, (handle, handle *))
			DEFINE_PNG_FUNCTION_POINTER(png_set_longjmp_fn, jmp_buf *, (handle, png_longjmp_ptr, uintptr))
			DEFINE_PNG_FUNCTION_POINTER(png_set_read_fn, void, (handle, void *, png_rw_ptr))
			DEFINE_PNG_FUNCTION_POINTER(png_get_io_ptr, void *, (handle))
			DEFINE_PNG_FUNCTION_POINTER(png_read_png, void, (handle, handle, int, void *))
			DEFINE_PNG_FUNCTION_POINTER(png_error, void, (handle, const char *))
			DEFINE_PNG_FUNCTION_POINTER(png_get_rows, uint8 **, (handle, handle))
			DEFINE_PNG_FUNCTION_POINTER(png_get_image_width, uint32, (handle, handle))
			DEFINE_PNG_FUNCTION_POINTER(png_get_image_height, uint32, (handle, handle))
			DEFINE_PNG_FUNCTION_POINTER(png_get_bit_depth, uint8, (handle, handle))
			DEFINE_PNG_FUNCTION_POINTER(png_get_color_type, uint8, (handle, handle))
			DEFINE_PNG_FUNCTION_POINTER(png_create_write_struct, handle, (const char *, void *, png_error_ptr, png_error_ptr))
			DEFINE_PNG_FUNCTION_POINTER(png_destroy_write_struct, void, (handle *, handle *))
			DEFINE_PNG_FUNCTION_POINTER(png_set_write_fn, void, (handle, void *, png_rw_ptr, png_flush_ptr))
			DEFINE_PNG_FUNCTION_POINTER(png_set_IHDR, void, (handle, handle, uint32, uint32, int, int, int, int, int))
			DEFINE_PNG_FUNCTION_POINTER(png_set_rows, void, (handle, handle, void **))
			DEFINE_PNG_FUNCTION_POINTER(png_write_png, void, (handle, handle, int, void *))

			jmp_buf * png_jmpbuf(handle ctx) noexcept { return png_set_longjmp_fn(ctx, longjmp, sizeof(jmp_buf)); }
		}
		namespace libjpeg
		{
			#define TJPF_RGB	0
			#define TJPF_RGBX	2
			#define TJPF_RGBA	7
			#define TJPF_BGR	1
			#define TJPF_BGRX	3
			#define TJPF_BGRA	8
			#define TJPF_GRAY	6

			#define TJSAMP_444	0
			#define TJSAMP_422	1
			#define TJSAMP_420	2
			#define TJSAMP_GRAY	3

			#define TJCS_GRAY			2
			#define TJFLAG_BOTTOMUP		2
			#define TJFLAG_PROGRESSIVE	16384

			#define DEFINE_JPEG_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
			#define DEFINE_JPEG_FUNCTION_IMPORT(NAME) libjpeg::NAME = reinterpret_cast<libjpeg::func_##NAME>(dlsym(libjpeg::library, #NAME));

			bool initialized = false;
			handle library;

			DEFINE_JPEG_FUNCTION_POINTER(tjInitCompress, handle, (void))
			DEFINE_JPEG_FUNCTION_POINTER(tjInitDecompress, handle, (void))
			DEFINE_JPEG_FUNCTION_POINTER(tjDestroy, int, (handle));
			DEFINE_JPEG_FUNCTION_POINTER(tjFree, void, (unsigned char *))
			DEFINE_JPEG_FUNCTION_POINTER(tjCompress2, int, (handle, const unsigned char *, int, int, int, int, unsigned char **, unsigned long *, int, int, int))
			DEFINE_JPEG_FUNCTION_POINTER(tjDecompressHeader3, int, (handle, const unsigned char *, unsigned long, int *, int *, int *, int *))
			DEFINE_JPEG_FUNCTION_POINTER(tjDecompress2, int, (handle, const unsigned char *, unsigned long, unsigned char *, int, int, int, int, int))
		}
		namespace libtiff
		{
			#define TIFFTAG_SUBFILETYPE			254
			#define FILETYPE_PAGE				0x2
			#define TIFFTAG_IMAGEWIDTH			256
			#define TIFFTAG_IMAGELENGTH			257
			#define TIFFTAG_BITSPERSAMPLE		258
			#define TIFFTAG_COMPRESSION			259
			#define COMPRESSION_NONE			1
			#define COMPRESSION_LZW				5
			#define COMPRESSION_JPEG			7
			#define COMPRESSION_ADOBE_DEFLATE	8
			#define COMPRESSION_LZMA			34925
			#define TIFFTAG_PHOTOMETRIC			262
			#define PHOTOMETRIC_MINISBLACK		1
			#define PHOTOMETRIC_RGB				2
			#define TIFFTAG_ORIENTATION			274
			#define ORIENTATION_TOPLEFT			1
			#define ORIENTATION_BOTLEFT			4
			#define TIFFTAG_SAMPLESPERPIXEL		277
			#define TIFFTAG_PLANARCONFIG		284
			#define PLANARCONFIG_CONTIG			1
			#define TIFFTAG_PAGENUMBER			297
			#define TIFFTAG_PREDICTOR			317
			#define PREDICTOR_NONE				1
			#define PREDICTOR_HORIZONTAL		2
			#define TIFFTAG_EXTRASAMPLES		338
			#define EXTRASAMPLE_UNSPECIFIED		0
			#define EXTRASAMPLE_ASSOCALPHA		1
			#define EXTRASAMPLE_UNASSALPHA		2
			#define TIFFTAG_SAMPLEFORMAT		339
			#define SAMPLEFORMAT_UINT			1
			#define TIFFTAG_JPEGQUALITY			65537

			typedef void (* TIFFErrorHandler) (const char *, const char *) noexcept;
			typedef intptr (* TIFFReadWriteProc) (void *, void *, intptr) noexcept;
			typedef int64 (* TIFFSeekProc) (void *, int64, int) noexcept;
			typedef int (* TIFFCloseProc) (void *) noexcept;
			typedef int64 (* TIFFSizeProc) (void *) noexcept;
			typedef int (* TIFFMapFileProc) (void *, void **, uint64 *) noexcept;
			typedef void (* TIFFUnmapFileProc) (void *, void *, uint64) noexcept;

			#define DEFINE_TIFF_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
			#define DEFINE_TIFF_FUNCTION_IMPORT(NAME) libtiff::NAME = reinterpret_cast<libtiff::func_##NAME>(dlsym(libtiff::library, #NAME));

			bool initialized = false;
			handle library;

			DEFINE_TIFF_FUNCTION_POINTER(TIFFSetErrorHandler, TIFFErrorHandler, (TIFFErrorHandler))
			DEFINE_TIFF_FUNCTION_POINTER(TIFFSetWarningHandler, TIFFErrorHandler, (TIFFErrorHandler))
			DEFINE_TIFF_FUNCTION_POINTER(TIFFClientOpen, handle, (const char *, const char *, void *, TIFFReadWriteProc, TIFFReadWriteProc, TIFFSeekProc, TIFFCloseProc, TIFFSizeProc, TIFFMapFileProc, TIFFUnmapFileProc))
			DEFINE_TIFF_FUNCTION_POINTER(TIFFGetField, int, (handle, uint32, ...))
			DEFINE_TIFF_FUNCTION_POINTER(TIFFReadRGBAImage, int, (handle, uint32, uint32, uint32 *, int))
			DEFINE_TIFF_FUNCTION_POINTER(TIFFReadDirectory, int, (handle))
			DEFINE_TIFF_FUNCTION_POINTER(TIFFClose, void, (handle))
			DEFINE_TIFF_FUNCTION_POINTER(TIFFSetField, int, (handle, uint32, ...))
			DEFINE_TIFF_FUNCTION_POINTER(TIFFWriteScanline, int, (handle, void *, uint32, uint16))
			DEFINE_TIFF_FUNCTION_POINTER(TIFFWriteDirectory, int, (handle))
		}
		namespace libwebp
		{
			#define DEFINE_WEBP_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
			#define DEFINE_WEBP_FUNCTION_IMPORT(NAME) libwebp::NAME = reinterpret_cast<libwebp::func_##NAME>(dlsym(libwebp::library, #NAME));

			bool initialized = false;
			handle library;

			DEFINE_WEBP_FUNCTION_POINTER(WebPFree, void, (void *))
			DEFINE_WEBP_FUNCTION_POINTER(WebPEncodeRGB, uintptr, (const uint8 *, int, int, int, float, uint8 **))
			DEFINE_WEBP_FUNCTION_POINTER(WebPEncodeBGR, uintptr, (const uint8 *, int, int, int, float, uint8 **))
			DEFINE_WEBP_FUNCTION_POINTER(WebPEncodeRGBA, uintptr, (const uint8 *, int, int, int, float, uint8 **))
			DEFINE_WEBP_FUNCTION_POINTER(WebPEncodeBGRA, uintptr, (const uint8 *, int, int, int, float, uint8 **))
			DEFINE_WEBP_FUNCTION_POINTER(WebPEncodeLosslessRGB, uintptr, (const uint8 *, int, int, int, uint8 **))
			DEFINE_WEBP_FUNCTION_POINTER(WebPEncodeLosslessBGR, uintptr, (const uint8 *, int, int, int, uint8 **))
			DEFINE_WEBP_FUNCTION_POINTER(WebPEncodeLosslessRGBA, uintptr, (const uint8 *, int, int, int, uint8 **))
			DEFINE_WEBP_FUNCTION_POINTER(WebPEncodeLosslessBGRA, uintptr, (const uint8 *, int, int, int, uint8 **))
			DEFINE_WEBP_FUNCTION_POINTER(WebPGetInfo, int, (const uint8 *, uintptr, int *, int *))
			DEFINE_WEBP_FUNCTION_POINTER(WebPDecodeRGBAInto, uint8 *, (const uint8 *, uintptr, uint8 *, uintptr, int))
		}
		namespace libheif
		{
			typedef struct heif_error {
				int code;
				int subcode;
				const char * message;
			} heif_error;
			typedef struct heif_writer {
				int writer_api_version;
				heif_error (* write) (handle ctx, const void* data, uintptr size, void * userdata) noexcept;
			} heif_writer;

			constexpr int heif_compression_HEVC			= 1;
			constexpr int heif_colorspace_monochrome	= 2;
			constexpr int heif_colorspace_RGB			= 1;
			constexpr int heif_chroma_planar			= 0;
			constexpr int heif_chroma_interleaved_RGB	= 10;
			constexpr int heif_chroma_interleaved_RGBA	= 11;
			constexpr int heif_channel_Y				= 0;
			constexpr int heif_channel_interleaved		= 10;

			#define DEFINE_HEIF_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
			#define DEFINE_HEIF_FUNCTION_IMPORT(NAME) libheif::NAME = reinterpret_cast<libheif::func_##NAME>(dlsym(libheif::library, #NAME));

			bool initialized = false;
			handle library;

			DEFINE_HEIF_FUNCTION_POINTER(heif_context_alloc, handle, (void))
			DEFINE_HEIF_FUNCTION_POINTER(heif_context_free, void, (handle))
			DEFINE_HEIF_FUNCTION_POINTER(heif_context_get_encoder_for_format, heif_error, (handle, int, handle *))
			DEFINE_HEIF_FUNCTION_POINTER(heif_encoder_release, void, (handle))
			DEFINE_HEIF_FUNCTION_POINTER(heif_encoder_set_lossy_quality, heif_error, (handle, int))
			DEFINE_HEIF_FUNCTION_POINTER(heif_encoder_set_lossless, heif_error, (handle, int))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_create, heif_error, (int, int, int, int, handle *))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_release, void, (handle))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_set_premultiplied_alpha, void, (handle, int))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_add_plane, heif_error, (handle, int, int, int, int))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_get_plane2, uint8 *, (handle, int, uintptr *))
			DEFINE_HEIF_FUNCTION_POINTER(heif_context_encode_image, heif_error, (handle, handle, handle, void *, void *))
			DEFINE_HEIF_FUNCTION_POINTER(heif_context_write, heif_error, (handle, heif_writer *, void *))
			DEFINE_HEIF_FUNCTION_POINTER(heif_context_read_from_memory_without_copy, heif_error, (handle, const void *, uintptr, void *))
			DEFINE_HEIF_FUNCTION_POINTER(heif_context_get_primary_image_handle, heif_error, (handle, handle *))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_handle_has_alpha_channel, int, (handle))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_handle_release, void, (handle))
			DEFINE_HEIF_FUNCTION_POINTER(heif_decode_image, heif_error, (handle, handle *, int, int, void *))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_is_premultiplied_alpha, int, (handle))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_get_width, int, (handle, int))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_get_height, int, (handle, int))
			DEFINE_HEIF_FUNCTION_POINTER(heif_image_get_plane_readonly2, uint8 *, (handle, int, uintptr *))
		};

		void Linux_PNGLibraryInitialize(void) noexcept
		{
			if (libpng::initialized) return;
			Memory::AcquireRootLock();
			if (libpng::initialized) { Memory::ReleaseRootLock(); return; }
			libpng::library = dlopen("libpng.so", RTLD_NOW);
			if (libpng::library) {
				DEFINE_PNG_FUNCTION_IMPORT(png_sig_cmp)
				DEFINE_PNG_FUNCTION_IMPORT(png_create_read_struct)
				DEFINE_PNG_FUNCTION_IMPORT(png_create_info_struct)
				DEFINE_PNG_FUNCTION_IMPORT(png_destroy_read_struct)
				DEFINE_PNG_FUNCTION_IMPORT(png_destroy_info_struct)
				DEFINE_PNG_FUNCTION_IMPORT(png_set_longjmp_fn)
				DEFINE_PNG_FUNCTION_IMPORT(png_set_read_fn)
				DEFINE_PNG_FUNCTION_IMPORT(png_get_io_ptr)
				DEFINE_PNG_FUNCTION_IMPORT(png_read_png)
				DEFINE_PNG_FUNCTION_IMPORT(png_error)
				DEFINE_PNG_FUNCTION_IMPORT(png_get_rows)
				DEFINE_PNG_FUNCTION_IMPORT(png_get_image_width)
				DEFINE_PNG_FUNCTION_IMPORT(png_get_image_height)
				DEFINE_PNG_FUNCTION_IMPORT(png_get_bit_depth)
				DEFINE_PNG_FUNCTION_IMPORT(png_get_color_type)
				DEFINE_PNG_FUNCTION_IMPORT(png_create_write_struct)
				DEFINE_PNG_FUNCTION_IMPORT(png_destroy_write_struct)
				DEFINE_PNG_FUNCTION_IMPORT(png_set_write_fn)
				DEFINE_PNG_FUNCTION_IMPORT(png_set_IHDR)
				DEFINE_PNG_FUNCTION_IMPORT(png_set_rows)
				DEFINE_PNG_FUNCTION_IMPORT(png_write_png)
			}
			libpng::initialized = true;
			Memory::ReleaseRootLock();
		}
		void Linux_JPEGLibraryInitialize(void) noexcept
		{
			if (libjpeg::initialized) return;
			Memory::AcquireRootLock();
			if (libjpeg::initialized) { Memory::ReleaseRootLock(); return; }
			libjpeg::library = dlopen("libturbojpeg.so", RTLD_NOW);
			if (libjpeg::library) {
				DEFINE_JPEG_FUNCTION_IMPORT(tjInitCompress)
				DEFINE_JPEG_FUNCTION_IMPORT(tjInitDecompress)
				DEFINE_JPEG_FUNCTION_IMPORT(tjDestroy)
				DEFINE_JPEG_FUNCTION_IMPORT(tjFree)
				DEFINE_JPEG_FUNCTION_IMPORT(tjCompress2)
				DEFINE_JPEG_FUNCTION_IMPORT(tjDecompressHeader3)
				DEFINE_JPEG_FUNCTION_IMPORT(tjDecompress2)
			}
			libjpeg::initialized = true;
			Memory::ReleaseRootLock();
		}
		void Linux_TIFFLibraryInitialize(void) noexcept
		{
			if (libtiff::initialized) return;
			Memory::AcquireRootLock();
			if (libtiff::initialized) { Memory::ReleaseRootLock(); return; }
			libtiff::library = dlopen("libtiff.so", RTLD_NOW);
			if (libtiff::library) {
				DEFINE_TIFF_FUNCTION_IMPORT(TIFFSetErrorHandler)
				DEFINE_TIFF_FUNCTION_IMPORT(TIFFSetWarningHandler)
				DEFINE_TIFF_FUNCTION_IMPORT(TIFFClientOpen)
				DEFINE_TIFF_FUNCTION_IMPORT(TIFFGetField)
				DEFINE_TIFF_FUNCTION_IMPORT(TIFFReadRGBAImage)
				DEFINE_TIFF_FUNCTION_IMPORT(TIFFReadDirectory)
				DEFINE_TIFF_FUNCTION_IMPORT(TIFFClose)
				DEFINE_TIFF_FUNCTION_IMPORT(TIFFSetField)
				DEFINE_TIFF_FUNCTION_IMPORT(TIFFWriteScanline)
				DEFINE_TIFF_FUNCTION_IMPORT(TIFFWriteDirectory)
			}
			libtiff::initialized = true;
			Memory::ReleaseRootLock();
		}
		void Linux_WEBPLibraryInitialize(void) noexcept
		{
			if (libwebp::initialized) return;
			Memory::AcquireRootLock();
			if (libwebp::initialized) { Memory::ReleaseRootLock(); return; }
			libwebp::library = dlopen("libwebp.so", RTLD_NOW);
			if (libwebp::library) {
				DEFINE_WEBP_FUNCTION_IMPORT(WebPFree)
				DEFINE_WEBP_FUNCTION_IMPORT(WebPEncodeRGB)
				DEFINE_WEBP_FUNCTION_IMPORT(WebPEncodeBGR)
				DEFINE_WEBP_FUNCTION_IMPORT(WebPEncodeRGBA)
				DEFINE_WEBP_FUNCTION_IMPORT(WebPEncodeBGRA)
				DEFINE_WEBP_FUNCTION_IMPORT(WebPEncodeLosslessRGB)
				DEFINE_WEBP_FUNCTION_IMPORT(WebPEncodeLosslessBGR)
				DEFINE_WEBP_FUNCTION_IMPORT(WebPEncodeLosslessRGBA)
				DEFINE_WEBP_FUNCTION_IMPORT(WebPEncodeLosslessBGRA)
				DEFINE_WEBP_FUNCTION_IMPORT(WebPGetInfo)
				DEFINE_WEBP_FUNCTION_IMPORT(WebPDecodeRGBAInto)
			}
			libwebp::initialized = true;
			Memory::ReleaseRootLock();
		}
		void Linux_HEIFLibraryInitialize(void) noexcept
		{
			if (libheif::initialized) return;
			Memory::AcquireRootLock();
			if (libheif::initialized) { Memory::ReleaseRootLock(); return; }
			libheif::library = dlopen("libheif.so", RTLD_NOW);
			if (libheif::library) {
				DEFINE_HEIF_FUNCTION_IMPORT(heif_context_alloc)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_context_free)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_context_get_encoder_for_format)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_encoder_release)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_encoder_set_lossy_quality)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_encoder_set_lossless)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_create)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_release)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_set_premultiplied_alpha)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_add_plane)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_get_plane2)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_context_encode_image)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_context_write)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_context_read_from_memory_without_copy)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_context_get_primary_image_handle)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_handle_has_alpha_channel)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_handle_release)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_decode_image)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_is_premultiplied_alpha)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_get_width)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_get_height)
				DEFINE_HEIF_FUNCTION_IMPORT(heif_image_get_plane_readonly2)
			}
			libheif::initialized = true;
			Memory::ReleaseRootLock();
		}

		void PngErrorCallback(handle ctx, const char * error) noexcept {}
		void PngWarningCallback(handle ctx, const char * error) noexcept {}
		void PngReadData(handle ctx, uint8 * data, uintptr length) noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto stream = reinterpret_cast<Stream *>(libpng::png_get_io_ptr(ctx));
			auto read = stream->ReadE(data, length, ectx);
			if (ErrorTest(ectx) || read != length) libpng::png_error(ctx, "");
		}
		void PngWriteData(handle ctx, uint8 * data, uintptr length) noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto stream = reinterpret_cast<Stream *>(libpng::png_get_io_ptr(ctx));
			auto written = stream->WriteE(data, length, ectx);
			if (ErrorTest(ectx) || written != length) libpng::png_error(ctx, "");
		}
		void PngFlushData(handle ctx) noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto stream = reinterpret_cast<Stream *>(libpng::png_get_io_ptr(ctx));
			stream->FlushE(ectx);
			if (ErrorTest(ectx)) libpng::png_error(ctx, "");
		}

		intptr TiffStreamRead(void * user, void * pdata, intptr size) noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto stream = reinterpret_cast<Stream *>(user);
			auto read = stream->ReadE(pdata, size, ectx);
			if (ErrorTest(ectx)) return -1;
			return read;
		}
		intptr TiffStreamWrite(void * user, void * pdata, intptr size) noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto stream = reinterpret_cast<Stream *>(user);
			auto written = stream->WriteE(pdata, size, ectx);
			if (ErrorTest(ectx)) return -1;
			return written;
		}
		int64 TiffStreamSeek(void * user, int64 offset, int method) noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto stream = reinterpret_cast<Stream *>(user);
			uint64 newpos;
			if (method == SEEK_SET) newpos = stream->SeekE(offset, SeekOrigin::Begin, ectx);
			else if (method == SEEK_CUR) newpos = stream->SeekE(offset, SeekOrigin::Current, ectx);
			else if (method == SEEK_END) newpos = stream->SeekE(offset, SeekOrigin::End, ectx);
			else return -1;
			if (ErrorTest(ectx)) return -1;
			return newpos;
		}
		int TiffStreamClose(void * user) noexcept { return 0; }
		int64 TiffStreamLength(void * user) noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto stream = reinterpret_cast<Stream *>(user);
			auto result = stream->GetLengthE(ectx);
			if (ErrorTest(ectx)) return -1;
			return result;
		}
		int TiffStreamMap(void * user, void ** ppdata, uint64 * plength) noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto stream = reinterpret_cast<Stream *>(user);
			auto size = stream->GetLengthE(ectx);
			if (ErrorTest(ectx)) return -1;
			auto mem = malloc(size);
			if (!mem) return -1;
			auto org = stream->SeekE(0, SeekOrigin::Current, ectx);
			if (ErrorTest(ectx)) { free(mem); return -1; }
			stream->SeekE(0, SeekOrigin::Begin, ectx);
			if (ErrorTest(ectx)) { free(mem); return -1; }
			auto read = stream->ReadE(mem, size, ectx);
			if (ErrorTest(ectx) || read != size) { free(mem); return -1; }
			stream->SeekE(org, SeekOrigin::Begin, ectx);
			if (ErrorTest(ectx)) { free(mem); return -1; }
			*ppdata = mem;
			*plength = size;
			return 0;
		}
		void TiffStreamUnmap(void * user, void * pdata, uint64 length) noexcept { free(pdata); }
		libheif::heif_error HeicStreamWrite(handle ctx, const void * data, uintptr size, void * userdata) noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto stream = reinterpret_cast<Stream *>(userdata);
			auto written = stream->WriteE(data, size, ectx);
			if (ErrorTest(ectx) || written != size) return libheif::heif_error { .code = 9, .subcode = 0, .message = 0 };
			return libheif::heif_error { .code = 0, .subcode = 0, .message = 0 };
		}
		bool PictureNeedsAlpha(Picture * pict) noexcept
		{
			auto & desc = pict->GetDesc();
			if (PixelFormatHasAlpha(desc.format)) return true;
			if (!NeedsPalette(desc.format)) return false;
			for (uint i = 0; i < desc.palette_size; i++) if (desc.palette[i].a != 0xFF) return true;
			return false;
		}

		void PngCodecEncode(Stream * dest, Picture * src, const uint * arg_names, const uint * arg_values, uint argc)
		{
			if (!libpng::png_create_write_struct || !libpng::png_create_info_struct) throw NotImplementedException();
			if (!libpng::png_destroy_write_struct || !libpng::png_destroy_info_struct) throw NotImplementedException();
			if (!libpng::png_set_longjmp_fn || !libpng::png_set_write_fn || !libpng::png_set_IHDR) throw NotImplementedException();
			if (!libpng::png_set_rows || !libpng::png_write_png || !libpng::png_error) throw NotImplementedException();
			oref<Picture> encode;
			array<void *> scanline(1);
			PixelFormat pxf;
			uint enforce_bpp = 0;
			for (uint i = 0; i < argc; i++) if (arg_names[i] == EncoderOptions::OverrideBitDepth) { enforce_bpp = arg_values[i]; break; }
			int png_color_type;
			int png_transform = PNG_TRANSFORM_IDENTITY;
			if (PictureNeedsAlpha(src) || enforce_bpp == 32) {
				png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;
				pxf = PixelFormat::R8G8B8A8;
			} else {
				png_color_type = PNG_COLOR_TYPE_RGB;
				pxf = PixelFormat::R8G8B8;
			}
			if (src->GetDesc().format == pxf && src->GetDesc().alpha_mode == AlphaMode::Straight) encode = src;
			else encode = src->Convert(pxf, AlphaMode::Straight);
			auto & desc = encode->GetDesc();
			scanline.SetLength(desc.height);
			if (desc.origin == ScanOrigin::TopLeft) {
				for (uint i = 0; i < scanline.GetLength(); i++) scanline[i] = reinterpret_cast<uint8 *>(desc.data) + desc.stride * i;
			} else if (desc.origin == ScanOrigin::BottomLeft) {
				for (uint i = 0; i < scanline.GetLength(); i++) scanline[i] = reinterpret_cast<uint8 *>(desc.data) + desc.stride * (desc.height - i - 1);
			} else throw InvalidStateException();
			auto session = libpng::png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, PngErrorCallback, PngWarningCallback);
			if (!session) throw OutOfMemoryException();
			auto info = libpng::png_create_info_struct(session);
			if (!info) { libpng::png_destroy_write_struct(&session, 0); throw OutOfMemoryException(); }
			if (setjmp(*libpng::png_jmpbuf(session))) {
				libpng::png_destroy_write_struct(&session, &info);
				throw InputOutputException(Errores::SuberrorIO::Unknown);
			}
			libpng::png_set_write_fn(session, dest, PngWriteData, PngFlushData);
			libpng::png_set_IHDR(session, info, desc.width, desc.height, 8, png_color_type, 0, 0, 0);
			libpng::png_set_rows(session, info, scanline);
			libpng::png_write_png(session, info, png_transform, 0);
			libpng::png_destroy_write_struct(&session, &info);
		}
		oref<Picture> PngCodecDecode(Stream * stream)
		{
			if (!libpng::png_create_read_struct || !libpng::png_create_info_struct) throw NotImplementedException();
			if (!libpng::png_destroy_read_struct || !libpng::png_destroy_info_struct) throw NotImplementedException();
			if (!libpng::png_set_longjmp_fn || !libpng::png_set_read_fn) throw NotImplementedException();
			if (!libpng::png_get_io_ptr || !libpng::png_read_png || !libpng::png_error) throw NotImplementedException();
			if (!libpng::png_get_image_width || !libpng::png_get_image_height) throw NotImplementedException();
			if (!libpng::png_get_rows || !libpng::png_get_color_type || !libpng::png_get_bit_depth) throw NotImplementedException();
			oref<Picture> result;
			stream->Seek(0, SeekOrigin::Begin);
			auto session = libpng::png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, PngErrorCallback, PngWarningCallback);
			if (!session) throw OutOfMemoryException();
			auto info = libpng::png_create_info_struct(session);
			if (!info) { libpng::png_destroy_read_struct(&session, 0, 0); throw OutOfMemoryException(); }
			auto end = libpng::png_create_info_struct(session);
			if (!end) { libpng::png_destroy_read_struct(&session, &info, 0); throw OutOfMemoryException(); }
			if (setjmp(*libpng::png_jmpbuf(session))) {
				libpng::png_destroy_read_struct(&session, &info, &end);
				throw InvalidFormatException();
			}
			libpng::png_set_read_fn(session, stream, PngReadData);
			libpng::png_read_png(session, info, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_GRAY_TO_RGB, 0);
			auto rows = libpng::png_get_rows(session, info);
			auto width = libpng::png_get_image_width(session, info);
			auto height = libpng::png_get_image_height(session, info);
			auto color_type = libpng::png_get_color_type(session, info);
			auto bps = libpng::png_get_bit_depth(session, info);
			PictureDesc desc;
			uint valid_scan;
			try {
				desc.width = width;
				desc.height = height;
				if (bps == 8 && color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
					desc.stride = 4 * width;
					desc.format = PixelFormat::R8G8B8A8;
					valid_scan = 4 * width;
				} else if (bps == 8 && color_type == PNG_COLOR_TYPE_RGB) {
					desc.stride = (3 * width + 3) & ~3;
					desc.format = PixelFormat::R8G8B8;
					valid_scan = 3 * width;
				} else throw InvalidFormatException();
				desc.alpha_mode = AlphaMode::Straight;
				desc.origin = ScanOrigin::TopLeft;
				desc.palette_size = 0;
				result = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
				desc = result->GetDesc();
			} catch (...) { libpng::png_destroy_read_struct(&session, &info, &end); throw; }
			for (uint y = 0; y < desc.height; y++) Memory::MemoryCopy(reinterpret_cast<uint8 *>(desc.data) + (y * desc.stride), rows[y], valid_scan);
			libpng::png_destroy_read_struct(&session, &info, &end);
			return result;
		}
		bool PngCodecProbe(Codices::CodecIOProbe & prob)
		{
			if (prob.file_title_size >= 8 && libpng::png_sig_cmp && !libpng::png_sig_cmp(prob.file_title.bytes, 0, 8)) return true;
			return false;
		}

		void JpegCodecEncode(Stream * dest, Picture * src, const uint * arg_names, const uint * arg_values, uint argc)
		{
			if (!libjpeg::tjInitCompress || !libjpeg::tjDestroy || !libjpeg::tjCompress2 || !libjpeg::tjFree) throw NotImplementedException();
			uint quality = 50, chrss = 1, override_bpp = 0;
			bool progressive = false;
			for (uint i = 0; i < argc; i++) {
				if (arg_names[i] == EncoderOptions::OverrideBitDepth) override_bpp = arg_values[i];
				if (arg_names[i] == EncoderOptions::CompressionQuality) quality = min(max(arg_values[i], 1U), 100U);
				if (arg_names[i] == EncoderOptions::CompressionMode && arg_values[i]) progressive = true;
				if (arg_names[i] == EncoderOptions::CompressionChrominanceSubsample) chrss = min(max(arg_values[i], 1U), 4U);
			}
			oref<Picture> encode;
			PixelFormat pxf = src->GetDesc().format;
			int jpeg_pixel_format, chrome_format;
			if (override_bpp && override_bpp < 24) pxf = PixelFormat::R8;
			if (pxf == PixelFormat::R8G8B8) {
				jpeg_pixel_format = TJPF_RGB;
				chrome_format = TJSAMP_444;
			} else if (pxf == PixelFormat::R8G8B8X8) {
				jpeg_pixel_format = TJPF_RGBX;
				chrome_format = TJSAMP_444;
			} else if (pxf == PixelFormat::R8G8B8A8) {
				jpeg_pixel_format = TJPF_RGBA;
				chrome_format = TJSAMP_444;
			} else if (pxf == PixelFormat::B8G8R8) {
				jpeg_pixel_format = TJPF_BGR;
				chrome_format = TJSAMP_444;
			} else if (pxf == PixelFormat::B8G8R8X8) {
				jpeg_pixel_format = TJPF_BGRX;
				chrome_format = TJSAMP_444;
			} else if (pxf == PixelFormat::B8G8R8A8) {
				jpeg_pixel_format = TJPF_BGRA;
				chrome_format = TJSAMP_444;
			} else if (pxf == PixelFormat::R8 || pxf == PixelFormat::R4 || pxf == PixelFormat::R2 || pxf == PixelFormat::R1) {
				pxf = PixelFormat::R8;
				jpeg_pixel_format = TJPF_GRAY;
				chrome_format = TJSAMP_GRAY;
			} else {
				pxf = PixelFormat::R8G8B8;
				jpeg_pixel_format = TJPF_RGB;
				chrome_format = TJSAMP_444;
			}
			if (chrome_format == TJSAMP_444) {
				if (chrss >= 4) chrome_format = TJSAMP_420;
				else if (chrss >= 2) chrome_format = TJSAMP_422;
			}
			if (src->GetDesc().format == pxf) encode = src; else encode = src->Convert(pxf);
			auto & desc = encode->GetDesc();
			auto context = libjpeg::tjInitCompress();
			if (!context) throw OutOfMemoryException();
			unsigned char * result = 0;
			unsigned long result_length = 0;
			int flags = 0;
			if (desc.origin == ScanOrigin::BottomLeft) flags |= TJFLAG_BOTTOMUP;
			if (progressive) flags |= TJFLAG_PROGRESSIVE;
			auto status = libjpeg::tjCompress2(context, reinterpret_cast<const uint8 *>(desc.data), desc.width, desc.stride, desc.height,
				jpeg_pixel_format, &result, &result_length, chrome_format, quality, flags);
			libjpeg::tjDestroy(context);
			if (status < 0) throw OutOfMemoryException();
			ErrorContext ectx; ErrorClear(ectx);
			auto written = dest->WriteE(result, result_length, ectx);
			libjpeg::tjFree(result);
			ErrorThrow(ectx);
			if (written != result_length) throw InputOutputException(Errores::SuberrorIO::WriteFailure);
		}
		oref<Picture> JpegCodecDecode(Stream * stream)
		{
			if (!libjpeg::tjInitDecompress || !libjpeg::tjDestroy || !libjpeg::tjDecompressHeader3 || !libjpeg::tjDecompress2) throw NotImplementedException();
			oref<Picture> decode;
			stream->Seek(0, SeekOrigin::Begin);
			auto data = stream->ReadAll();
			auto context = libjpeg::tjInitDecompress();
			if (!context) throw OutOfMemoryException();
			int width, height, chrss, clrs, format;
			auto status = libjpeg::tjDecompressHeader3(context, data->GetBuffer(), data->GetLength(), &width, &height, &chrss, &clrs);
			if (status < 0) { libjpeg::tjDestroy(context); throw InvalidFormatException(); }
			PictureDesc desc;
			desc.width = width;
			desc.height = height;
			desc.alpha_mode = AlphaMode::Undefined;
			desc.origin = ScanOrigin::TopLeft;
			desc.palette_size = 0;
			if (chrss == TJSAMP_GRAY || clrs == TJCS_GRAY) { desc.format = PixelFormat::R8; format = TJPF_GRAY; }
			else { desc.format = PixelFormat::R8G8B8; format = TJPF_RGB; }
			desc.stride = ((desc.width * GetBitsPerPixel(desc.format) + 31) & ~31) / 8;
			try { decode = owrap(new Picture(desc, PictureInit::AllocateUninitialized)); } catch (...) { libjpeg::tjDestroy(context); throw InvalidFormatException(); }
			desc = decode->GetDesc();
			status = libjpeg::tjDecompress2(context, data->GetBuffer(), data->GetLength(), reinterpret_cast<unsigned char *>(desc.data),
				desc.width, desc.stride, desc.height, format, 0);
			libjpeg::tjDestroy(context);
			if (status < 0) throw InvalidFormatException();
			return decode;
		}
		bool JpegCodecProbe(Codices::CodecIOProbe & prob)
		{
			uint32 signature = 0;
			if (prob.file_title_size >= 3) signature = prob.file_title.dwords[0] & 0xFFFFFFU;
			if (libjpeg::library && signature == 0xFFD8FF) return true;
			return false;
		}

		void TiffCodecEncode(Stream * dest, Image * src, const uint * arg_names, const uint * arg_values, uint argc)
		{
			if (!libtiff::TIFFSetErrorHandler || !libtiff::TIFFSetWarningHandler || !libtiff::TIFFClientOpen || !libtiff::TIFFClose) throw NotImplementedException();
			if (!libtiff::TIFFSetField || !libtiff::TIFFWriteScanline || !libtiff::TIFFWriteDirectory) throw NotImplementedException();
			uint bpp_override = 0, compression_mode = 1, compression_quality = 50;
			for (uint i = 0; i < argc; i++) {
				if (arg_names[i] == EncoderOptions::OverrideBitDepth) bpp_override = arg_values[i];
				else if (arg_names[i] == EncoderOptions::CompressionMode) compression_mode = arg_values[i];
				else if (arg_names[i] == EncoderOptions::CompressionQuality) compression_quality = arg_values[i];
			}
			libtiff::TIFFSetErrorHandler(0);
			libtiff::TIFFSetWarningHandler(0);
			auto context = libtiff::TIFFClientOpen("", "w", dest, TiffStreamRead, TiffStreamWrite, TiffStreamSeek, TiffStreamClose, TiffStreamLength, TiffStreamMap, TiffStreamUnmap);
			if (!context) throw InvalidFormatException();
			uint page_number = 1;
			try { for (auto & f : *src) {
				int nchannels, color_format, alpha_format;
				auto pxf = f.GetDesc().format;
				auto am = f.GetDesc().alpha_mode;
				if (bpp_override) {
					if (bpp_override <= 8) pxf = PixelFormat::R8;
					else if (bpp_override <= 16) pxf = PixelFormat::R8A8;
					else if (bpp_override <= 24) pxf = PixelFormat::R8G8B8;
					else pxf = PixelFormat::R8G8B8A8;
				}
				if (pxf == PixelFormat::R1 || pxf == PixelFormat::R2 || pxf == PixelFormat::R4 || pxf == PixelFormat::R8) {
					nchannels = 1;
					color_format = PHOTOMETRIC_MINISBLACK;
					alpha_format = EXTRASAMPLE_UNSPECIFIED;
					pxf = PixelFormat::R8;
					am = AlphaMode::Undefined;
				} else if (pxf == PixelFormat::R1A1 || pxf == PixelFormat::R2A2 || pxf == PixelFormat::R4A4 || pxf == PixelFormat::R8A8) {
					nchannels = 2;
					color_format = PHOTOMETRIC_MINISBLACK;
					alpha_format = am == AlphaMode::Premultiplied ? EXTRASAMPLE_ASSOCALPHA : EXTRASAMPLE_UNASSALPHA;
					pxf = PixelFormat::R8A8;
					if (am != AlphaMode::Premultiplied) am = AlphaMode::Straight;
				} else if (PictureNeedsAlpha(&f)) {
					nchannels = 4;
					color_format = PHOTOMETRIC_RGB;
					alpha_format = am == AlphaMode::Premultiplied ? EXTRASAMPLE_ASSOCALPHA : EXTRASAMPLE_UNASSALPHA;
					pxf = PixelFormat::R8G8B8A8;
					if (am != AlphaMode::Premultiplied) am = AlphaMode::Straight;
				} else {
					nchannels = 3;
					color_format = PHOTOMETRIC_RGB;
					alpha_format = EXTRASAMPLE_UNSPECIFIED;
					pxf = PixelFormat::R8G8B8;
					am = AlphaMode::Undefined;
				}
				oref<Picture> encode;
				if (f.GetDesc().format == pxf && f.GetDesc().alpha_mode == am) encode = &f; else encode = f.Convert(pxf, am);
				auto & desc = encode->GetDesc();
				if (!libtiff::TIFFSetField(context, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE)) throw OutOfMemoryException();
				if (!libtiff::TIFFSetField(context, TIFFTAG_IMAGEWIDTH, desc.width)) throw OutOfMemoryException();
				if (!libtiff::TIFFSetField(context, TIFFTAG_IMAGELENGTH, desc.height)) throw OutOfMemoryException();
				if (!libtiff::TIFFSetField(context, TIFFTAG_BITSPERSAMPLE, 8)) throw OutOfMemoryException();
				if (compression_mode == 0) {
					if (!libtiff::TIFFSetField(context, TIFFTAG_COMPRESSION, COMPRESSION_NONE)) throw OutOfMemoryException();
				} else if (compression_mode == 1) {
					if (compression_quality <= 50) {
						if (!libtiff::TIFFSetField(context, TIFFTAG_COMPRESSION, COMPRESSION_LZW)) throw OutOfMemoryException();
						if (!libtiff::TIFFSetField(context, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL)) throw OutOfMemoryException();
					} else if (compression_quality <= 75) {
						if (!libtiff::TIFFSetField(context, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE)) throw OutOfMemoryException();
					} else {
						if (!libtiff::TIFFSetField(context, TIFFTAG_COMPRESSION, COMPRESSION_LZMA)) throw OutOfMemoryException();
					}
				} else {
					if (!libtiff::TIFFSetField(context, TIFFTAG_COMPRESSION, COMPRESSION_JPEG)) throw OutOfMemoryException();
					int level = min(max(compression_quality, 1U), 100U);
					if (!libtiff::TIFFSetField(context, TIFFTAG_JPEGQUALITY, level)) throw OutOfMemoryException();
				}
				if (desc.origin == ScanOrigin::BottomLeft) {
					if (!libtiff::TIFFSetField(context, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT)) throw OutOfMemoryException();
				} else {
					if (!libtiff::TIFFSetField(context, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT)) throw OutOfMemoryException();
				}
				if (!libtiff::TIFFSetField(context, TIFFTAG_PHOTOMETRIC, color_format)) throw OutOfMemoryException();
				if (!libtiff::TIFFSetField(context, TIFFTAG_SAMPLESPERPIXEL, nchannels)) throw OutOfMemoryException();
				if (alpha_format != EXTRASAMPLE_UNSPECIFIED) if (!libtiff::TIFFSetField(context, TIFFTAG_EXTRASAMPLES, 1, &alpha_format)) throw OutOfMemoryException();
				if (!libtiff::TIFFSetField(context, TIFFTAG_PAGENUMBER, page_number++)) throw OutOfMemoryException();
				if (!libtiff::TIFFSetField(context, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT)) throw OutOfMemoryException();
				if (!libtiff::TIFFSetField(context, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)) throw OutOfMemoryException();
				auto data = reinterpret_cast<uint8 *>(desc.data);
				for (uint i = 0; i < desc.height; i++) {
					if (!libtiff::TIFFWriteScanline(context, data + desc.stride * i, i, 0)) throw OutOfMemoryException();
				}
				if (!libtiff::TIFFWriteDirectory(context)) throw OutOfMemoryException();
			} } catch (...) { libtiff::TIFFClose(context); throw; }
			libtiff::TIFFClose(context);
		}
		oref<Image> TiffCodecDecode(Stream * stream)
		{
			if (!libtiff::TIFFSetErrorHandler || !libtiff::TIFFSetWarningHandler || !libtiff::TIFFClientOpen || !libtiff::TIFFClose) throw NotImplementedException();
			if (!libtiff::TIFFReadRGBAImage || !libtiff::TIFFReadDirectory || !libtiff::TIFFGetField) throw NotImplementedException();
			auto result = owrap(new Image);
			libtiff::TIFFSetErrorHandler(0);
			libtiff::TIFFSetWarningHandler(0);
			stream->Seek(0, SeekOrigin::Begin);
			auto context = libtiff::TIFFClientOpen("", "r", stream, TiffStreamRead, TiffStreamWrite, TiffStreamSeek, TiffStreamClose, TiffStreamLength, TiffStreamMap, TiffStreamUnmap);
			if (!context) throw InvalidFormatException();
			try { do {
				uint width, height;
				libtiff::TIFFGetField(context, TIFFTAG_IMAGEWIDTH, &width);
				libtiff::TIFFGetField(context, TIFFTAG_IMAGELENGTH, &height);
				PictureDesc desc;
				desc.width = width;
				desc.height = height;
				desc.stride = 4 * width;
				desc.palette_size = 0;
				desc.format = PixelFormat::R8G8B8A8;
				desc.alpha_mode = AlphaMode::Straight;
				desc.origin = ScanOrigin::BottomLeft;
				auto pict = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
				desc = pict->GetDesc();
				if (libtiff::TIFFReadRGBAImage(context, width, height, reinterpret_cast<uint32 *>(desc.data), 0)) result->Append(pict);
			} while (libtiff::TIFFReadDirectory(context)); } catch (...) { libtiff::TIFFClose(context); throw; }
			libtiff::TIFFClose(context);
			if (result->GetLength() < 1) throw InvalidFormatException();
			return result;
		}
		bool TiffCodecProbe(Codices::CodecIOProbe & prob)
		{
			if (libtiff::library && prob.file_title_size >= 4 && (prob.file_title.dwords[0] == 0x2A004D4D || prob.file_title.dwords[0] == 0x002A4949)) return true;
			return false;
		}

		void WebpCodecEncode(Stream * dest, Picture * src, const uint * arg_names, const uint * arg_values, uint argc)
		{
			uint quality = 50, mode = 0, override_bpp = 0;
			for (uint i = 0; i < argc; i++) {
				if (arg_names[i] == EncoderOptions::OverrideBitDepth) override_bpp = arg_values[i];
				if (arg_names[i] == EncoderOptions::CompressionQuality) quality = min(max(arg_values[i], 1U), 100U);
				if (arg_names[i] == EncoderOptions::CompressionMode) mode = arg_values[i];
			}
			oref<Picture> encode;
			PixelFormat pxf = src->GetDesc().format;
			if (override_bpp) {
				if (override_bpp <= 24) pxf = PixelFormat::R8G8B8;
				else pxf = PixelFormat::R8G8B8A8;
			}
			if (pxf == PixelFormat::R8G8B8) {
				pxf = PixelFormat::R8G8B8;
			} else if (pxf == PixelFormat::R8G8B8A8) {
				pxf = PixelFormat::R8G8B8A8;
			} else if (pxf == PixelFormat::B8G8R8) {
				pxf = PixelFormat::B8G8R8;
			} else if (pxf == PixelFormat::B8G8R8A8) {
				pxf = PixelFormat::B8G8R8A8;
			} else if (PictureNeedsAlpha(src)) {
				pxf = PixelFormat::R8G8B8A8;
			} else {
				pxf = PixelFormat::R8G8B8;
			}
			if (src->GetDesc().format == pxf && src->GetDesc().alpha_mode == AlphaMode::Straight && src->GetDesc().origin == ScanOrigin::TopLeft) encode = src;
			else encode = src->Convert(pxf, AlphaMode::Straight, ScanOrigin::TopLeft);
			auto & desc = encode->GetDesc();
			auto data = reinterpret_cast<const uint8 *>(desc.data);
			uint8 * result;
			uintptr length;
			if (pxf == PixelFormat::R8G8B8) {
				if (!libwebp::WebPFree || !libwebp::WebPEncodeRGB || !libwebp::WebPEncodeLosslessRGB) throw NotImplementedException();
				if (mode) length = libwebp::WebPEncodeRGB(data, desc.width, desc.height, desc.stride, quality, &result);
				else length = libwebp::WebPEncodeLosslessRGB(data, desc.width, desc.height, desc.stride, &result);
			} else if (pxf == PixelFormat::R8G8B8A8) {
				if (!libwebp::WebPFree || !libwebp::WebPEncodeRGBA || !libwebp::WebPEncodeLosslessRGBA) throw NotImplementedException();
				if (mode) length = libwebp::WebPEncodeRGBA(data, desc.width, desc.height, desc.stride, quality, &result);
				else length = libwebp::WebPEncodeLosslessRGBA(data, desc.width, desc.height, desc.stride, &result);
			} else if (pxf == PixelFormat::B8G8R8) {
				if (!libwebp::WebPFree || !libwebp::WebPEncodeBGR || !libwebp::WebPEncodeLosslessBGR) throw NotImplementedException();
				if (mode) length = libwebp::WebPEncodeBGR(data, desc.width, desc.height, desc.stride, quality, &result);
				else length = libwebp::WebPEncodeLosslessBGR(data, desc.width, desc.height, desc.stride, &result);
			} else if (pxf == PixelFormat::B8G8R8A8) {
				if (!libwebp::WebPFree || !libwebp::WebPEncodeBGRA || !libwebp::WebPEncodeLosslessBGRA) throw NotImplementedException();
				if (mode) length = libwebp::WebPEncodeBGRA(data, desc.width, desc.height, desc.stride, quality, &result);
				else length = libwebp::WebPEncodeLosslessBGRA(data, desc.width, desc.height, desc.stride, &result);
			}
			if (!length) throw OutOfMemoryException();
			ErrorContext ectx; ErrorClear(ectx);
			auto written = dest->WriteE(result, length, ectx);
			libwebp::WebPFree(result);
			ErrorThrow(ectx);
			if (written != length) throw InputOutputException(Errores::SuberrorIO::WriteFailure);
		}
		oref<Picture> WebpCodecDecode(Stream * stream)
		{
			if (!libwebp::WebPGetInfo || !libwebp::WebPDecodeRGBAInto) throw NotImplementedException();
			oref<Picture> decode;
			stream->Seek(0, SeekOrigin::Begin);
			auto data = stream->ReadAll();
			int width, height;
			if (!libwebp::WebPGetInfo(data->GetBuffer(), data->GetLength(), &width, &height)) throw InvalidFormatException();
			PictureDesc desc;
			desc.width = width;
			desc.height = height;
			desc.stride = 4 * desc.width;
			desc.format = PixelFormat::R8G8B8A8;
			desc.alpha_mode = AlphaMode::Straight;
			desc.origin = ScanOrigin::TopLeft;
			desc.palette_size = 0;
			decode = owrap(new Picture(desc, PictureInit::AllocateUninitialized));
			desc = decode->GetDesc();
			if (!libwebp::WebPDecodeRGBAInto(data->GetBuffer(), data->GetLength(), reinterpret_cast<uint8 *>(desc.data), desc.stride * desc.height, desc.stride)) throw InvalidFormatException();
			return decode;
		}
		bool WebpCodecProbe(Codices::CodecIOProbe & prob)
		{
			if (libwebp::library && prob.file_title_size >= 12 && Memory::MemoryCompare(&prob.file_title.dwords[0], "RIFF", 4) == 0 && Memory::MemoryCompare(&prob.file_title.dwords[2], "WEBP", 4) == 0) return true;
			return false;
		}

		void HeicCodecEncode(Stream * dest, Picture * src, const uint * arg_names, const uint * arg_values, uint argc)
		{
			if (!libheif::heif_context_alloc || !libheif::heif_context_free || !libheif::heif_context_get_encoder_for_format) throw NotImplementedException();
			if (!libheif::heif_encoder_release || !libheif::heif_encoder_set_lossy_quality || !libheif::heif_encoder_set_lossless) throw NotImplementedException();
			if (!libheif::heif_image_create || !libheif::heif_image_release || !libheif::heif_image_set_premultiplied_alpha) throw NotImplementedException();
			if (!libheif::heif_image_add_plane || !libheif::heif_image_get_plane2) throw NotImplementedException();
			if (!libheif::heif_context_encode_image || !libheif::heif_context_write) throw NotImplementedException();
			uint quality = 50, mode = 1, override_bpp = 0;
			for (uint i = 0; i < argc; i++) {
				if (arg_names[i] == EncoderOptions::OverrideBitDepth) override_bpp = arg_values[i];
				if (arg_names[i] == EncoderOptions::CompressionQuality) quality = min(max(arg_values[i], 1U), 100U);
				if (arg_names[i] == EncoderOptions::CompressionMode) mode = arg_values[i];
			}
			oref<Picture> encode;
			PixelFormat pxf = src->GetDesc().format;
			if (override_bpp) {
				if (override_bpp <= 8) pxf = PixelFormat::R8;
				else if (override_bpp <= 24) pxf = PixelFormat::R8G8B8;
				else pxf = PixelFormat::R8G8B8A8;
			}
			if (pxf == PixelFormat::R1 || pxf == PixelFormat::R2 || pxf == PixelFormat::R4 || pxf == PixelFormat::R8) {
				pxf = PixelFormat::R8;
			} else if (PictureNeedsAlpha(src)) {
				pxf = PixelFormat::R8G8B8A8;
			} else {
				pxf = PixelFormat::R8G8B8;
			}
			if (src->GetDesc().format == pxf) encode = src;
			else encode = src->Convert(pxf, AlphaMode::Straight, ScanOrigin::TopLeft);
			auto & desc = encode->GetDesc();
			auto data = reinterpret_cast<const uint8 *>(desc.data);
			handle image, encoder, context = libheif::heif_context_alloc();
			if (!context) throw OutOfMemoryException();
			if (libheif::heif_context_get_encoder_for_format(context, libheif::heif_compression_HEVC, &encoder).code) {
				libheif::heif_context_free(context);
				throw NotImplementedException();
			}
			if (libheif::heif_encoder_set_lossless(encoder, mode == 0).code || (mode && libheif::heif_encoder_set_lossy_quality(encoder, quality).code)) {
				libheif::heif_encoder_release(encoder);
				libheif::heif_context_free(context);
				throw NotImplementedException();
			}
			int channel_name;
			if (desc.format == PixelFormat::R8) {
				channel_name = libheif::heif_channel_Y;
				if (libheif::heif_image_create(desc.width, desc.height, libheif::heif_colorspace_monochrome, libheif::heif_chroma_planar, &image).code) {
					libheif::heif_encoder_release(encoder);
					libheif::heif_context_free(context);
					throw OutOfMemoryException();
				}
			} else if (desc.format == PixelFormat::R8G8B8) {
				channel_name = libheif::heif_channel_interleaved;
				if (libheif::heif_image_create(desc.width, desc.height, libheif::heif_colorspace_RGB, libheif::heif_chroma_interleaved_RGB, &image).code) {
					libheif::heif_encoder_release(encoder);
					libheif::heif_context_free(context);
					throw OutOfMemoryException();
				}
			} else if (desc.format == PixelFormat::R8G8B8A8) {
				channel_name = libheif::heif_channel_interleaved;
				if (libheif::heif_image_create(desc.width, desc.height, libheif::heif_colorspace_RGB, libheif::heif_chroma_interleaved_RGBA, &image).code) {
					libheif::heif_encoder_release(encoder);
					libheif::heif_context_free(context);
					throw OutOfMemoryException();
				}
				libheif::heif_image_set_premultiplied_alpha(image, desc.alpha_mode == AlphaMode::Premultiplied);
			}
			if (libheif::heif_image_add_plane(image, channel_name, desc.width, desc.height, 8).code) {
				libheif::heif_image_release(image);
				libheif::heif_encoder_release(encoder);
				libheif::heif_context_free(context);
				throw OutOfMemoryException();
			}
			uintptr dest_stride;
			auto dest_buffer = libheif::heif_image_get_plane2(image, channel_name, &dest_stride);
			for (uint y = 0; y < desc.height; y++) {
				uint i = (desc.origin == ScanOrigin::BottomLeft) ? desc.height - y - 1 : y;
				Memory::MemoryCopy(dest_buffer + y * dest_stride, data + i * desc.stride, min(desc.stride, uint(dest_stride)));
			}
			if (libheif::heif_context_encode_image(context, image, encoder, 0, 0).code) {
				libheif::heif_image_release(image);
				libheif::heif_encoder_release(encoder);
				libheif::heif_context_free(context);
				throw OutOfMemoryException();
			}
			libheif::heif_image_release(image);
			libheif::heif_encoder_release(encoder);
			libheif::heif_writer wr;
			wr.writer_api_version = 1;
			wr.write = HeicStreamWrite;
			if (libheif::heif_context_write(context, &wr, dest).code) {
				libheif::heif_context_free(context);
				throw InputOutputException(Errores::SuberrorIO::Unknown);
			}
			libheif::heif_context_free(context);
		}
		oref<Picture> HeicCodecDecode(Stream * stream)
		{
			if (!libheif::heif_context_alloc || !libheif::heif_context_free || !libheif::heif_context_read_from_memory_without_copy) throw NotImplementedException();
			if (!libheif::heif_context_get_primary_image_handle || !libheif::heif_image_handle_release || !libheif::heif_image_handle_has_alpha_channel) throw NotImplementedException();
			if (!libheif::heif_decode_image || !libheif::heif_image_release || !libheif::heif_image_is_premultiplied_alpha) throw NotImplementedException();
			if (!libheif::heif_image_get_width || !libheif::heif_image_get_height || !libheif::heif_image_get_plane_readonly2) throw NotImplementedException();
			oref<Picture> decode;
			stream->Seek(0, SeekOrigin::Begin);
			auto data = stream->ReadAll();
			handle image, image_handle, context = libheif::heif_context_alloc();
			if (!context) throw OutOfMemoryException();
			if (libheif::heif_context_read_from_memory_without_copy(context, data->GetBuffer(), data->GetLength(), 0).code) {
				libheif::heif_context_free(context);
				throw InvalidFormatException();
			}
			if (libheif::heif_context_get_primary_image_handle(context, &image_handle).code) {
				libheif::heif_context_free(context);
				throw InvalidFormatException();
			}
			auto has_alpha = libheif::heif_image_handle_has_alpha_channel(image_handle);
			if (has_alpha) {
				if (libheif::heif_decode_image(image_handle, &image, libheif::heif_colorspace_RGB, libheif::heif_chroma_interleaved_RGBA, 0).code) {
					libheif::heif_image_handle_release(image_handle);
					libheif::heif_context_free(context);
					throw OutOfMemoryException();
				}
			} else {
				if (libheif::heif_decode_image(image_handle, &image, libheif::heif_colorspace_RGB, libheif::heif_chroma_interleaved_RGB, 0).code) {
					libheif::heif_image_handle_release(image_handle);
					libheif::heif_context_free(context);
					throw OutOfMemoryException();
				}
			}
			auto premultiplied_alpha = libheif::heif_image_is_premultiplied_alpha(image);
			uintptr stride;
			PictureDesc desc;
			desc.width = libheif::heif_image_get_width(image, libheif::heif_channel_interleaved);
			desc.height = libheif::heif_image_get_height(image, libheif::heif_channel_interleaved);
			desc.data = libheif::heif_image_get_plane_readonly2(image, libheif::heif_channel_interleaved, &stride);
			desc.stride = stride;
			desc.origin = ScanOrigin::TopLeft;
			desc.palette = 0;
			desc.palette_size = 0;
			if (has_alpha) {
				desc.format = PixelFormat::R8G8B8A8;
				desc.alpha_mode = premultiplied_alpha ? AlphaMode::Premultiplied : AlphaMode::Straight;
			} else {
				desc.format = PixelFormat::R8G8B8;
				desc.alpha_mode = AlphaMode::Undefined;
			}
			try { decode = owrap(new Picture(desc, PictureInit::AllocateCopy)); } catch (...) {
				libheif::heif_image_release(image);
				libheif::heif_image_handle_release(image_handle);
				libheif::heif_context_free(context);
				throw;
			}
			libheif::heif_image_release(image);
			libheif::heif_image_handle_release(image_handle);
			libheif::heif_context_free(context);
			return decode;
		}
		bool HeicCodecProbe(Codices::CodecIOProbe & prob)
		{
			if (libheif::library && prob.file_title_size >= 12 && Memory::MemoryCompare(&prob.file_title.dwords[1], "ftyp", 4) == 0) {
				if (Memory::MemoryCompare(&prob.file_title.dwords[2], "heic", 4) == 0) return true;
				if (Memory::MemoryCompare(&prob.file_title.dwords[2], "heix", 4) == 0) return true;
				if (Memory::MemoryCompare(&prob.file_title.dwords[2], "mif1", 4) == 0) return true;
			}
			return false;
		}

		bool PngCodecI(Codices::CodecIO mode, void * io, ErrorContext & ectx) noexcept
		{
			Linux_PNGLibraryInitialize();
			ESSE_TRY_INTRO
				if (mode == Codices::CodecIO::Encode) {
					auto & enc = *reinterpret_cast<Codices::CodecIOEncode *>(io);
					if (Memory::StringCompare(enc.format, ImageFormatPNG)) return false;
					if (!enc.stream || !enc.encode || !enc.encode->GetLength()) throw InvalidArgumentException();
					PngCodecEncode(enc.stream, enc.encode->FirstReference(), enc.option_names, enc.option_values, enc.option_number);
					return true;
				} else if (mode == Codices::CodecIO::Decode) {
					auto & dec = *reinterpret_cast<Codices::CodecIODecode *>(io);
					auto result = PngCodecDecode(dec.stream);
					if (result) {
						dec.decode = owrap(new Image);
						dec.decode->Append(result);
						dec.format = ImageFormatPNG;
						return true;
					} else return false;
				} else if (mode == Codices::CodecIO::Probe) {
					auto & prob = *reinterpret_cast<Codices::CodecIOProbe *>(io);
					if (PngCodecProbe(prob)) { prob.format = ImageFormatPNG; return true; }
					else return false;
				} else if (mode == Codices::CodecIO::EncodeFormats) {
					auto & efl = *reinterpret_cast<Codices::CodecIOEncodeFormats *>(io);
					if (libpng::library) {
						efl.name = "libpng";
						efl.caps.Append(ImageFormatPNG, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode);
						return true;
					} else return false;
				} else if (mode == Codices::CodecIO::EncodeModes) {
					auto & eml = *reinterpret_cast<Codices::CodecIOEncodeModes *>(io);
					if (Memory::StringCompare(eml.format, ImageFormatPNG)) return false;
					if (libpng::library) {
						eml.pixel_formats.AddElement(PixelFormat::R8G8B8A8);
						eml.pixel_formats.AddElement(PixelFormat::R8G8B8);
						eml.options.Append(EncoderOptions::OverrideBitDepth, KeyValuePair<uint, uint>(24, 32));
						return true;
					} else return false;
				} else ErrorSet(ectx, Errores::ErrorNotImplemented);
				return false;
			ESSE_TRY_OUTRO(false)
		}
		bool JpegCodecI(Codices::CodecIO mode, void * io, ErrorContext & ectx) noexcept
		{
			Linux_JPEGLibraryInitialize();
			ESSE_TRY_INTRO
				if (mode == Codices::CodecIO::Encode) {
					auto & enc = *reinterpret_cast<Codices::CodecIOEncode *>(io);
					if (Memory::StringCompare(enc.format, ImageFormatJPEG)) return false;
					if (!enc.stream || !enc.encode || !enc.encode->GetLength()) throw InvalidArgumentException();
					JpegCodecEncode(enc.stream, enc.encode->FirstReference(), enc.option_names, enc.option_values, enc.option_number);
					return true;
				} else if (mode == Codices::CodecIO::Decode) {
					auto & dec = *reinterpret_cast<Codices::CodecIODecode *>(io);
					auto result = JpegCodecDecode(dec.stream);
					if (result) {
						dec.decode = owrap(new Image);
						dec.decode->Append(result);
						dec.format = ImageFormatJPEG;
						return true;
					} else return false;
				} else if (mode == Codices::CodecIO::Probe) {
					auto & prob = *reinterpret_cast<Codices::CodecIOProbe *>(io);
					if (JpegCodecProbe(prob)) { prob.format = ImageFormatJPEG; return true; }
					else return false;
				} else if (mode == Codices::CodecIO::EncodeFormats) {
					auto & efl = *reinterpret_cast<Codices::CodecIOEncodeFormats *>(io);
					if (libjpeg::library) {
						efl.name = "libturbojpeg";
						efl.caps.Append(ImageFormatJPEG, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode);
						return true;
					} else return false;
				} else if (mode == Codices::CodecIO::EncodeModes) {
					auto & eml = *reinterpret_cast<Codices::CodecIOEncodeModes *>(io);
					if (Memory::StringCompare(eml.format, ImageFormatJPEG)) return false;
					if (libjpeg::library) {
						eml.pixel_formats.AddElement(PixelFormat::R8G8B8);
						eml.pixel_formats.AddElement(PixelFormat::R8);
						eml.options.Append(EncoderOptions::OverrideBitDepth, KeyValuePair<uint, uint>(8, 24));
						eml.options.Append(EncoderOptions::CompressionMode, KeyValuePair<uint, uint>(0, 1));
						eml.options.Append(EncoderOptions::CompressionQuality, KeyValuePair<uint, uint>(1, 100));
						eml.options.Append(EncoderOptions::CompressionChrominanceSubsample, KeyValuePair<uint, uint>(1, 4));
						return true;
					} else return false;
				} else ErrorSet(ectx, Errores::ErrorNotImplemented);
				return false;
			ESSE_TRY_OUTRO(false)
		}
		bool TiffCodecI(Codices::CodecIO mode, void * io, ErrorContext & ectx) noexcept
		{
			Linux_TIFFLibraryInitialize();
			ESSE_TRY_INTRO
				if (mode == Codices::CodecIO::Encode) {
					auto & enc = *reinterpret_cast<Codices::CodecIOEncode *>(io);
					if (Memory::StringCompare(enc.format, ImageFormatTIFF)) return false;
					if (!enc.stream || !enc.encode || !enc.encode->GetLength()) throw InvalidArgumentException();
					TiffCodecEncode(enc.stream, enc.encode, enc.option_names, enc.option_values, enc.option_number);
					return true;
				} else if (mode == Codices::CodecIO::Decode) {
					auto & dec = *reinterpret_cast<Codices::CodecIODecode *>(io);
					auto result = TiffCodecDecode(dec.stream);
					if (result) { dec.decode = result; dec.format = ImageFormatTIFF; return true; } else return false;
				} else if (mode == Codices::CodecIO::Probe) {
					auto & prob = *reinterpret_cast<Codices::CodecIOProbe *>(io);
					if (TiffCodecProbe(prob)) { prob.format = ImageFormatTIFF; return true; }
					else return false;
				} else if (mode == Codices::CodecIO::EncodeFormats) {
					auto & efl = *reinterpret_cast<Codices::CodecIOEncodeFormats *>(io);
					if (libtiff::library) {
						efl.name = "libtiff";
						efl.caps.Append(ImageFormatTIFF, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode | Codices::CodecIOMode::Multiframe);
						return true;
					} else return false;
				} else if (mode == Codices::CodecIO::EncodeModes) {
					auto & eml = *reinterpret_cast<Codices::CodecIOEncodeModes *>(io);
					if (Memory::StringCompare(eml.format, ImageFormatTIFF)) return false;
					if (libtiff::library) {
						eml.pixel_formats.AddElement(PixelFormat::R8G8B8A8);
						eml.pixel_formats.AddElement(PixelFormat::R8G8B8);
						eml.pixel_formats.AddElement(PixelFormat::R8A8);
						eml.pixel_formats.AddElement(PixelFormat::R8);
						eml.options.Append(EncoderOptions::OverrideBitDepth, KeyValuePair<uint, uint>(8, 32));
						eml.options.Append(EncoderOptions::CompressionMode, KeyValuePair<uint, uint>(0, 2));
						eml.options.Append(EncoderOptions::CompressionQuality, KeyValuePair<uint, uint>(1, 100));
						return true;
					} else return false;
				} else ErrorSet(ectx, Errores::ErrorNotImplemented);
				return false;
			ESSE_TRY_OUTRO(false)
		}
		bool WebpCodecI(Codices::CodecIO mode, void * io, ErrorContext & ectx) noexcept
		{
			Linux_WEBPLibraryInitialize();
			ESSE_TRY_INTRO
				if (mode == Codices::CodecIO::Encode) {
					auto & enc = *reinterpret_cast<Codices::CodecIOEncode *>(io);
					if (Memory::StringCompare(enc.format, ImageFormatWEBP)) return false;
					if (!enc.stream || !enc.encode || !enc.encode->GetLength()) throw InvalidArgumentException();
					WebpCodecEncode(enc.stream, enc.encode->FirstReference(), enc.option_names, enc.option_values, enc.option_number);
					return true;
				} else if (mode == Codices::CodecIO::Decode) {
					auto & dec = *reinterpret_cast<Codices::CodecIODecode *>(io);
					auto result = WebpCodecDecode(dec.stream);
					if (result) {
						dec.decode = owrap(new Image);
						dec.decode->Append(result);
						dec.format = ImageFormatWEBP;
						return true;
					} else return false;
				} else if (mode == Codices::CodecIO::Probe) {
					auto & prob = *reinterpret_cast<Codices::CodecIOProbe *>(io);
					if (WebpCodecProbe(prob)) { prob.format = ImageFormatWEBP; return true; }
					else return false;
				} else if (mode == Codices::CodecIO::EncodeFormats) {
					auto & efl = *reinterpret_cast<Codices::CodecIOEncodeFormats *>(io);
					if (libwebp::library) {
						efl.name = "libwebp";
						efl.caps.Append(ImageFormatWEBP, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode);
						return true;
					} else return false;
				} else if (mode == Codices::CodecIO::EncodeModes) {
					auto & eml = *reinterpret_cast<Codices::CodecIOEncodeModes *>(io);
					if (Memory::StringCompare(eml.format, ImageFormatWEBP)) return false;
					if (libwebp::library) {
						eml.pixel_formats.AddElement(PixelFormat::R8G8B8A8);
						eml.pixel_formats.AddElement(PixelFormat::R8G8B8);
						eml.pixel_formats.AddElement(PixelFormat::B8G8R8A8);
						eml.pixel_formats.AddElement(PixelFormat::B8G8R8);
						eml.options.Append(EncoderOptions::OverrideBitDepth, KeyValuePair<uint, uint>(24, 32));
						eml.options.Append(EncoderOptions::CompressionMode, KeyValuePair<uint, uint>(0, 1));
						eml.options.Append(EncoderOptions::CompressionQuality, KeyValuePair<uint, uint>(1, 100));
						return true;
					} else return false;
				} else ErrorSet(ectx, Errores::ErrorNotImplemented);
				return false;
			ESSE_TRY_OUTRO(false)
		}
		bool HeicCodecI(Codices::CodecIO mode, void * io, ErrorContext & ectx) noexcept
		{
			Linux_HEIFLibraryInitialize();
			ESSE_TRY_INTRO
				if (mode == Codices::CodecIO::Encode) {
					auto & enc = *reinterpret_cast<Codices::CodecIOEncode *>(io);
					if (Memory::StringCompare(enc.format, ImageFormatHEIC)) return false;
					if (!enc.stream || !enc.encode || !enc.encode->GetLength()) throw InvalidArgumentException();
					HeicCodecEncode(enc.stream, enc.encode->FirstReference(), enc.option_names, enc.option_values, enc.option_number);
					return true;
				} else if (mode == Codices::CodecIO::Decode) {
					auto & dec = *reinterpret_cast<Codices::CodecIODecode *>(io);
					auto result = HeicCodecDecode(dec.stream);
					if (result) {
						dec.decode = owrap(new Image);
						dec.decode->Append(result);
						dec.format = ImageFormatHEIC;
						return true;
					} else return false;
				} else if (mode == Codices::CodecIO::Probe) {
					auto & prob = *reinterpret_cast<Codices::CodecIOProbe *>(io);
					if (HeicCodecProbe(prob)) { prob.format = ImageFormatHEIC; return true; }
					else return false;
				} else if (mode == Codices::CodecIO::EncodeFormats) {
					auto & efl = *reinterpret_cast<Codices::CodecIOEncodeFormats *>(io);
					if (libheif::library) {
						efl.name = "libheif";
						efl.caps.Append(ImageFormatHEIC, Codices::CodecIOMode::Encode | Codices::CodecIOMode::Decode);
						return true;
					} else return false;
				} else if (mode == Codices::CodecIO::EncodeModes) {
					auto & eml = *reinterpret_cast<Codices::CodecIOEncodeModes *>(io);
					if (Memory::StringCompare(eml.format, ImageFormatHEIC)) return false;
					if (libheif::library) {
						eml.pixel_formats.AddElement(PixelFormat::R8G8B8A8);
						eml.pixel_formats.AddElement(PixelFormat::R8G8B8);
						eml.pixel_formats.AddElement(PixelFormat::R8);
						eml.options.Append(EncoderOptions::OverrideBitDepth, KeyValuePair<uint, uint>(8, 32));
						eml.options.Append(EncoderOptions::CompressionMode, KeyValuePair<uint, uint>(0, 1));
						eml.options.Append(EncoderOptions::CompressionQuality, KeyValuePair<uint, uint>(1, 100));
						return true;
					} else return false;
				} else ErrorSet(ectx, Errores::ErrorNotImplemented);
				return false;
			ESSE_TRY_OUTRO(false)
		}
		Codices::CodecIOFunction libpng_f = PngCodecI, libjpeg_f = JpegCodecI, libtiff_f = TiffCodecI;
		Codices::CodecIOFunction libwebp_f = WebpCodecI, libheif_f = HeicCodecI;
	}
}