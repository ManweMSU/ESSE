#pragma once

#include <Cor/Classes/CorObject.h>
#include <Cor/IO/CorDL.h>

namespace ESSE
{
	namespace CUPS
	{
		#define DEFINE_HANDLE_TYPE(NAME) typedef struct __internal_##NAME * NAME;
		#define DEFINE_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
		#define DEFINE_FUNCTION_IMPORT(NAME) NAME = reinterpret_cast<func_##NAME>(ESSE::IO::GetLibraryRoutine(_library, #NAME)); if (!NAME) throw NotImplementedException();

		#define CUPS_DEST_FLAGS_NONE			0x00
		#define CUPS_DEST_FLAGS_REMOVED			0x04
		#define CUPS_DEST_FLAGS_ERROR			0x08
		#define CUPS_MEDIA						"media"
		#define CUPS_COPIES						"copies"
		#define CUPS_ORIENTATION				"orientation-requested"
		#define CUPS_ORIENTATION_PORTRAIT		"3"
		#define CUPS_ORIENTATION_LANDSCAPE		"4"
		#define CUPS_SIDES						"sides"
		#define CUPS_SIDES_ONE_SIDED			"one-sided"
		#define CUPS_SIDES_TWO_SIDED_PORTRAIT	"two-sided-long-edge"
		#define CUPS_SIDES_TWO_SIDED_LANDSCAPE	"two-sided-short-edge"
		#define CUPS_FORMAT_PDF					"application/pdf"
		#define IPP_STATUS_OK					0x0000
		#define HTTP_STATUS_CONTINUE			100

		DEFINE_HANDLE_TYPE(http_t)
		DEFINE_HANDLE_TYPE(cups_dinfo_t)
		DEFINE_HANDLE_TYPE(ipp_attribute_t)
		typedef struct cups_option_s
		{
			char * name;
			char * value;
		} cups_option_t;
		typedef struct cups_dest_s
		{
			char * name, * instance;
			int is_default, num_options;
			cups_option_t * options;
		} cups_dest_t;
		typedef struct cups_size_s
		{
			char media[128];
			int width, length, bottom, left, right, top;
		} cups_size_t;
		typedef int (* cups_dest_cb_t) (void * user_data, unsigned flags, cups_dest_t * dest);

		class CUPSAPI : public Object
		{
			handle _library;
		public:
			DEFINE_FUNCTION_POINTER(cupsEnumDests, int, (unsigned, int, int *, unsigned int, unsigned int, cups_dest_cb_t, void *))
			DEFINE_FUNCTION_POINTER(cupsCopyDest, int, (cups_dest_t *, int, cups_dest_t **))
			DEFINE_FUNCTION_POINTER(cupsFreeDests, void, (int,  cups_dest_t *))
			DEFINE_FUNCTION_POINTER(cupsCopyDestInfo, cups_dinfo_t, (http_t, cups_dest_t *))
			DEFINE_FUNCTION_POINTER(cupsFreeDestInfo, void, (cups_dinfo_t))
			DEFINE_FUNCTION_POINTER(cupsAddOption, int, (const char *, const char *, int, cups_option_t **))
			DEFINE_FUNCTION_POINTER(cupsFreeOptions, void, (int, cups_option_t *))
			DEFINE_FUNCTION_POINTER(cupsFindDestDefault, ipp_attribute_t, (http_t, cups_dest_t *, cups_dinfo_t, const char *))
			DEFINE_FUNCTION_POINTER(cupsGetOption, const char *, (const char *, int, cups_option_t *))
			DEFINE_FUNCTION_POINTER(ippGetCount, int, (ipp_attribute_t))
			DEFINE_FUNCTION_POINTER(ippGetInteger, int, (ipp_attribute_t, int))
			DEFINE_FUNCTION_POINTER(ippGetString, const char *, (ipp_attribute_t, int, const char **))
			DEFINE_FUNCTION_POINTER(ippGetResolution, int, (ipp_attribute_t, int, int *, int *))
			DEFINE_FUNCTION_POINTER(ippGetBoolean, int, (ipp_attribute_t, int))
			DEFINE_FUNCTION_POINTER(cupsGetDestMediaByIndex, int, (http_t, cups_dest_t *, cups_dinfo_t, int, unsigned, cups_size_t *))
			DEFINE_FUNCTION_POINTER(cupsGetDestMediaCount, int, (http_t, cups_dest_t *, cups_dinfo_t, unsigned))
			DEFINE_FUNCTION_POINTER(cupsGetDestMediaDefault, int, (http_t, cups_dest_t *, cups_dinfo_t, unsigned, cups_size_t *))
			DEFINE_FUNCTION_POINTER(cupsCheckDestSupported, int, (http_t, cups_dest_t *, cups_dinfo_t, const char *, const char *))
			DEFINE_FUNCTION_POINTER(cupsCreateDestJob, int, (http_t, cups_dest_t *, cups_dinfo_t, int *, const char *, int, cups_option_t *))
			DEFINE_FUNCTION_POINTER(cupsCancelDestJob, int, (http_t, cups_dest_t *, int))
			DEFINE_FUNCTION_POINTER(cupsStartDestDocument, int, (http_t, cups_dest_t *, cups_dinfo_t, int, const char *, const char *, int, cups_option_t *, int))
			DEFINE_FUNCTION_POINTER(cupsWriteRequestData, int, (http_t, const char *, uintptr))
			DEFINE_FUNCTION_POINTER(cupsFinishDestDocument, int, (http_t, cups_dest_t *, cups_dinfo_t))
		public:
			CUPSAPI(void);
			virtual ~CUPSAPI(void) override;
		};
	}
}