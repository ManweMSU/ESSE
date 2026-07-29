#pragma once

#include <Cor/Classes/CorObject.h>
#include <Cor/IO/CorDL.h>

namespace ESSE
{
	namespace Cairo
	{
		#define DEFINE_HANDLE_TYPE(NAME) typedef struct __internal_##NAME * NAME;
		#define DEFINE_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
		#define DEFINE_FUNCTION_IMPORT(NAME) NAME = reinterpret_cast<func_##NAME>(ESSE::IO::GetLibraryRoutine(_library, #NAME)); if (!NAME) throw NotImplementedException();

		#define CAIRO_FORMAT_ARGB32 0
		#define CAIRO_STATUS_SUCCESS 0
		#define CAIRO_CONTENT_COLOR 0x1000
		#define CAIRO_CONTENT_ALPHA 0x2000
		#define CAIRO_CONTENT_COLOR_ALPHA 0x3000
		#define CAIRO_FT_SYNTHESIZE_BOLD 1
		#define CAIRO_FT_SYNTHESIZE_OBLIQUE 2

		#define FcResultMatch 0
		#define FcResultNoMatch 1
		#define FcResultTypeMismatch 2
		#define FcResultNoId 3
		#define FcResultOutOfMemory 4
		#define FcMatchPattern 0
		#define FcMatchFont 1
		#define FcMatchScan 2
		#define FcMatchKindEnd 3
		#define FcMatchKindBegin 0

		#define FC_FAMILY               "family"         /* String */
		#define FC_STYLE                "style"          /* String */
		#define FC_SLANT                "slant"          /* Int */
		#define FC_WEIGHT               "weight"         /* Int */
		#define FC_SIZE                 "size"           /* Range (double) */
		#define FC_ASPECT               "aspect"         /* Double */
		#define FC_PIXEL_SIZE           "pixelsize"      /* Double */
		#define FC_SPACING              "spacing"        /* Int */
		#define FC_FOUNDRY              "foundry"        /* String */
		#define FC_ANTIALIAS            "antialias"      /* Bool (depends) */
		#define FC_HINTING              "hinting"        /* Bool (true) */
		#define FC_HINT_STYLE           "hintstyle"      /* Int */
		#define FC_VERTICAL_LAYOUT      "verticallayout" /* Bool (false) */
		#define FC_AUTOHINT             "autohint"       /* Bool (false) */
		#define FC_GLOBAL_ADVANCE       "globaladvance"  /* Bool (true) */
		#define FC_WIDTH                "width"          /* Int */
		#define FC_FILE                 "file"           /* String */
		#define FC_INDEX                "index"          /* Int */
		#define FC_FT_FACE              "ftface"         /* FT_Face */
		#define FC_RASTERIZER           "rasterizer"     /* String (deprecated) */
		#define FC_OUTLINE              "outline"        /* Bool */
		#define FC_SCALABLE             "scalable"       /* Bool */
		#define FC_COLOR                "color"          /* Bool */
		#define FC_VARIABLE             "variable"       /* Bool */
		#define FC_SCALE                "scale"          /* double (deprecated) */
		#define FC_SYMBOL               "symbol"         /* Bool */
		#define FC_DPI                  "dpi"            /* double */
		#define FC_RGBA                 "rgba"           /* Int */
		#define FC_MINSPACE             "minspace"       /* Bool use minimum line spacing */
		#define FC_SOURCE               "source"         /* String (deprecated) */
		#define FC_CHARSET              "charset"        /* CharSet */
		#define FC_LANG                 "lang"           /* LangSet Set of RFC 3066 langs */
		#define FC_FONTVERSION          "fontversion"    /* Int from 'head' table */
		#define FC_FULLNAME             "fullname"       /* String */
		#define FC_FAMILYLANG           "familylang"     /* String RFC 3066 langs */
		#define FC_STYLELANG            "stylelang"      /* String RFC 3066 langs */
		#define FC_FULLNAMELANG         "fullnamelang"   /* String RFC 3066 langs */
		#define FC_CAPABILITY           "capability"     /* String */
		#define FC_FONTFORMAT           "fontformat"     /* String */
		#define FC_EMBOLDEN             "embolden"       /* Bool - true if emboldening needed*/
		#define FC_EMBEDDED_BITMAP      "embeddedbitmap" /* Bool - true to enable embedded bitmaps */
		#define FC_DECORATIVE           "decorative"     /* Bool - true if style is a decorative variant */
		#define FC_LCD_FILTER           "lcdfilter"      /* Int */
		#define FC_FONT_FEATURES        "fontfeatures"   /* String */
		#define FC_FONT_VARIATIONS      "fontvariations" /* String */
		#define FC_NAMELANG             "namelang"       /* String RFC 3866 langs */
		#define FC_PRGNAME              "prgname"        /* String */
		#define FC_HASH                 "hash"           /* String (deprecated) */
		#define FC_POSTSCRIPT_NAME      "postscriptname" /* String */
		#define FC_FONT_HAS_HINT        "fonthashint"    /* Bool - true if font has hinting */
		#define FC_ORDER                "order"          /* Integer */
		#define FC_DESKTOP_NAME         "desktop"        /* String */
		#define FC_NAMED_INSTANCE       "namedinstance"  /* Bool - true if font is named instance */
		#define FC_FONT_WRAPPER         "fontwrapper"    /* String */
		#define FC_SLANT_ROMAN          0
		#define FC_SLANT_ITALIC         100
		#define FC_SLANT_OBLIQUE        110
		#define FC_WEIGHT_THIN          0
		#define FC_WEIGHT_EXTRALIGHT    40
		#define FC_WEIGHT_ULTRALIGHT    FC_WEIGHT_EXTRALIGHT
		#define FC_WEIGHT_LIGHT         50
		#define FC_WEIGHT_DEMILIGHT     55
		#define FC_WEIGHT_SEMILIGHT     FC_WEIGHT_DEMILIGHT
		#define FC_WEIGHT_BOOK          75
		#define FC_WEIGHT_REGULAR       80
		#define FC_WEIGHT_NORMAL        FC_WEIGHT_REGULAR
		#define FC_WEIGHT_MEDIUM        100
		#define FC_WEIGHT_DEMIBOLD      180
		#define FC_WEIGHT_SEMIBOLD      FC_WEIGHT_DEMIBOLD
		#define FC_WEIGHT_BOLD          200
		#define FC_WEIGHT_EXTRABOLD     205
		#define FC_WEIGHT_ULTRABOLD     FC_WEIGHT_EXTRABOLD
		#define FC_WEIGHT_BLACK         210
		#define FC_WEIGHT_HEAVY         FC_WEIGHT_BLACK
		#define FC_WEIGHT_EXTRABLACK    215
		#define FC_WEIGHT_ULTRABLACK    FC_WEIGHT_EXTRABLACK

		#define FT_FACE_FLAG_SCALABLE          ( 1L <<  0 )
		#define FT_FACE_FLAG_FIXED_SIZES       ( 1L <<  1 )
		#define FT_FACE_FLAG_FIXED_WIDTH       ( 1L <<  2 )
		#define FT_FACE_FLAG_SFNT              ( 1L <<  3 )
		#define FT_FACE_FLAG_HORIZONTAL        ( 1L <<  4 )
		#define FT_FACE_FLAG_VERTICAL          ( 1L <<  5 )
		#define FT_FACE_FLAG_KERNING           ( 1L <<  6 )
		#define FT_FACE_FLAG_FAST_GLYPHS       ( 1L <<  7 )
		#define FT_FACE_FLAG_MULTIPLE_MASTERS  ( 1L <<  8 )
		#define FT_FACE_FLAG_GLYPH_NAMES       ( 1L <<  9 )
		#define FT_FACE_FLAG_EXTERNAL_STREAM   ( 1L << 10 )
		#define FT_FACE_FLAG_HINTER            ( 1L << 11 )
		#define FT_FACE_FLAG_CID_KEYED         ( 1L << 12 )
		#define FT_FACE_FLAG_TRICKY            ( 1L << 13 )
		#define FT_FACE_FLAG_COLOR             ( 1L << 14 )
		#define FT_FACE_FLAG_VARIATION         ( 1L << 15 )
		#define FT_FACE_FLAG_SVG               ( 1L << 16 )
		#define FT_FACE_FLAG_SBIX              ( 1L << 17 )
		#define FT_FACE_FLAG_SBIX_OVERLAY      ( 1L << 18 )
		#define FT_STYLE_FLAG_ITALIC           ( 1 << 0 )
		#define FT_STYLE_FLAG_BOLD             ( 1 << 1 )
		#define FT_LOAD_NO_SCALE               ( 1L << 0  )
		#define FT_ENCODING_UNICODE 1970170211

		DEFINE_HANDLE_TYPE(cairo_t)
		DEFINE_HANDLE_TYPE(cairo_surface_t)
		DEFINE_HANDLE_TYPE(cairo_pattern_t)
		DEFINE_HANDLE_TYPE(cairo_font_face_t)
		DEFINE_HANDLE_TYPE(cairo_scaled_font_t)
		DEFINE_HANDLE_TYPE(cairo_font_options_t)
		DEFINE_HANDLE_TYPE(FcConfig)
		DEFINE_HANDLE_TYPE(FcPattern)
		DEFINE_HANDLE_TYPE(FcCharSet)
		DEFINE_HANDLE_TYPE(FcObjectSet)
		DEFINE_HANDLE_TYPE(FT_Library)
		typedef int cairo_format_t;
		typedef int cairo_status_t;
		typedef struct _cairo_matrix {
			double xx; double yx;
			double xy; double yy;
			double x0; double y0;
		} cairo_matrix_t;
		typedef struct {
			unsigned long        index;
			double               x;
			double               y;
		} cairo_glyph_t;
		typedef void (* cairo_destroy_func_t) (void * data);
		typedef struct _FcFontSet {
			int			nfont;
			int			sfont;
			FcPattern	*fonts;
		} FcFontSet;
		typedef int FcBool;
		typedef int FcSetName;
		typedef int FcResult;
		typedef unsigned char FcChar8;
		typedef signed long FT_Long;
		typedef signed long FT_Pos;
		typedef signed long FT_Fixed;
		typedef unsigned long FT_ULong;
		typedef signed int FT_Int;
		typedef unsigned int FT_UInt;
		typedef signed short FT_Short;
		typedef unsigned short FT_UShort;
		typedef char FT_String;
		typedef struct FT_FaceRec_* FT_Face;
		typedef struct FT_GlyphSlotRec_* FT_GlyphSlot;
		typedef struct FT_SizeRec_* FT_Size;
		typedef struct FT_BBox_
		{
			FT_Pos  xMin, yMin;
			FT_Pos  xMax, yMax;
		} FT_BBox;
		typedef struct  FT_Glyph_Metrics_
		{
			FT_Pos  width;
			FT_Pos  height;
			FT_Pos  horiBearingX;
			FT_Pos  horiBearingY;
			FT_Pos  horiAdvance;
			FT_Pos  vertBearingX;
			FT_Pos  vertBearingY;
			FT_Pos  vertAdvance;
		} FT_Glyph_Metrics;
		typedef struct  FT_Vector_
		{
			FT_Pos  x;
			FT_Pos  y;
		} FT_Vector;
		typedef struct  FT_Bitmap_
		{
			unsigned int    rows;
			unsigned int    width;
			int             pitch;
			unsigned char*  buffer;
			unsigned short  num_grays;
			unsigned char   pixel_mode;
			unsigned char   palette_mode;
			void*           palette;
		} FT_Bitmap;
		typedef struct  FT_GlyphSlotRec_
		{
			FT_Library        library;
			FT_Face           face;
			void*             next;
			FT_UInt           glyph_index; /* new in 2.10; was reserved previously */
			void*             generic0;
			void*             generic1;
			FT_Glyph_Metrics  metrics;
			FT_Fixed          linearHoriAdvance;
			FT_Fixed          linearVertAdvance;
			FT_Vector         advance;
			int               format;
			FT_Bitmap         bitmap;
			FT_Int            bitmap_left;
			FT_Int            bitmap_top;
		} FT_GlyphSlotRec;
		typedef struct  FT_Bitmap_Size_
		{
			FT_Short  height;
			FT_Short  width;
			FT_Pos    size;
			FT_Pos    x_ppem;
			FT_Pos    y_ppem;
		} FT_Bitmap_Size;
		typedef struct  FT_Size_Metrics_
		{
			FT_UShort  x_ppem;      /* horizontal pixels per EM               */
			FT_UShort  y_ppem;      /* vertical pixels per EM                 */
			FT_Fixed   x_scale;     /* scaling values used to convert font    */
			FT_Fixed   y_scale;     /* units to 26.6 fractional pixels        */
			FT_Pos     ascender;    /* ascender in 26.6 frac. pixels          */
			FT_Pos     descender;   /* descender in 26.6 frac. pixels         */
			FT_Pos     height;      /* text height in 26.6 frac. pixels       */
			FT_Pos     max_advance; /* max horizontal advance, in 26.6 pixels */
		} FT_Size_Metrics;
		typedef struct  FT_SizeRec_
		{
			FT_Face           face;
			void*             generic0;
			void*             generic1;
			FT_Size_Metrics   metrics;
		} FT_SizeRec;
		typedef struct  FT_FaceRec_
		{
			FT_Long           num_faces;
			FT_Long           face_index;
			FT_Long           face_flags;
			FT_Long           style_flags;
			FT_Long           num_glyphs;
			FT_String*        family_name;
			FT_String*        style_name;
			FT_Int            num_fixed_sizes;
			FT_Bitmap_Size*   available_sizes;
			FT_Int            num_charmaps;
			void*             charmaps;
			void*             generic0;
			void*             generic1;

			FT_BBox           bbox;
			FT_UShort         units_per_EM;
			FT_Short          ascender;
			FT_Short          descender;
			FT_Short          height;
			FT_Short          max_advance_width;
			FT_Short          max_advance_height;
			FT_Short          underline_position;
			FT_Short          underline_thickness;

			FT_GlyphSlot      glyph;
			FT_Size           size;
			void*             charmap;
		} FT_FaceRec;

		class CairoAPI : public Object
		{
			handle _library;
		public:
			DEFINE_FUNCTION_POINTER(cairo_version, int, (void))
			DEFINE_FUNCTION_POINTER(cairo_format_stride_for_width, int, (cairo_format_t, int))
			DEFINE_FUNCTION_POINTER(cairo_image_surface_create_for_data, cairo_surface_t, (uint8 *, cairo_format_t, int, int, int))
			DEFINE_FUNCTION_POINTER(cairo_surface_status, cairo_status_t, (cairo_surface_t))
			DEFINE_FUNCTION_POINTER(cairo_surface_destroy, void, (cairo_surface_t))
			DEFINE_FUNCTION_POINTER(cairo_surface_mark_dirty, void, (cairo_surface_t))
			DEFINE_FUNCTION_POINTER(cairo_surface_flush, void, (cairo_surface_t))
			DEFINE_FUNCTION_POINTER(cairo_create, cairo_t, (cairo_surface_t))
			DEFINE_FUNCTION_POINTER(cairo_status, cairo_status_t, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_destroy, void, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_pattern_create_linear, cairo_pattern_t, (double, double, double, double))
			DEFINE_FUNCTION_POINTER(cairo_pattern_status, cairo_status_t, (cairo_pattern_t))
			DEFINE_FUNCTION_POINTER(cairo_pattern_destroy, void, (cairo_pattern_t))
			DEFINE_FUNCTION_POINTER(cairo_pattern_add_color_stop_rgba, void, (cairo_pattern_t, double, double, double, double, double))
			DEFINE_FUNCTION_POINTER(cairo_pattern_set_matrix, void, (cairo_pattern_t, const cairo_matrix_t &))
			DEFINE_FUNCTION_POINTER(cairo_pattern_get_matrix, void, (cairo_pattern_t, cairo_matrix_t &))
			DEFINE_FUNCTION_POINTER(cairo_set_source_rgba, void, (cairo_t, double, double, double, double))
			DEFINE_FUNCTION_POINTER(cairo_set_source, void, (cairo_t, cairo_pattern_t))
			DEFINE_FUNCTION_POINTER(cairo_rectangle, void, (cairo_t, double, double, double, double))
			DEFINE_FUNCTION_POINTER(cairo_fill, void, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_stroke, void, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_get_matrix, void, (cairo_t, cairo_matrix_t &))
			DEFINE_FUNCTION_POINTER(cairo_set_matrix, void, (cairo_t, const cairo_matrix_t &))
			DEFINE_FUNCTION_POINTER(cairo_matrix_init, void, (cairo_matrix_t &, double, double, double, double, double, double))
			DEFINE_FUNCTION_POINTER(cairo_matrix_init_identity, void, (cairo_matrix_t &))
			DEFINE_FUNCTION_POINTER(cairo_matrix_init_translate, void, (cairo_matrix_t &, double, double))
			DEFINE_FUNCTION_POINTER(cairo_matrix_init_scale, void, (cairo_matrix_t &, double, double))
			DEFINE_FUNCTION_POINTER(cairo_matrix_init_rotate, void, (cairo_matrix_t &, double))
			DEFINE_FUNCTION_POINTER(cairo_matrix_translate, void, (cairo_matrix_t &, double, double))
			DEFINE_FUNCTION_POINTER(cairo_matrix_scale, void, (cairo_matrix_t &, double, double))
			DEFINE_FUNCTION_POINTER(cairo_matrix_rotate, void, (cairo_matrix_t &, double))
			DEFINE_FUNCTION_POINTER(cairo_matrix_invert, cairo_status_t, (cairo_matrix_t &))
			DEFINE_FUNCTION_POINTER(cairo_matrix_multiply, void, (cairo_matrix_t &, const cairo_matrix_t &, const cairo_matrix_t &))
			DEFINE_FUNCTION_POINTER(cairo_move_to, void, (cairo_t, double, double))
			DEFINE_FUNCTION_POINTER(cairo_line_to, void, (cairo_t, double, double))
			DEFINE_FUNCTION_POINTER(cairo_close_path, void, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_set_line_width, void, (cairo_t, double))
			DEFINE_FUNCTION_POINTER(cairo_set_line_join, void, (cairo_t, int))
			DEFINE_FUNCTION_POINTER(cairo_set_line_cap, void, (cairo_t, int))
			DEFINE_FUNCTION_POINTER(cairo_set_source_rgb, void, (cairo_t, double, double, double))
			DEFINE_FUNCTION_POINTER(cairo_set_operator, void, (cairo_t, int))
			DEFINE_FUNCTION_POINTER(cairo_surface_create_for_rectangle, cairo_surface_t, (cairo_surface_t, double, double, double, double))
			DEFINE_FUNCTION_POINTER(cairo_pattern_create_for_surface, cairo_pattern_t, (cairo_surface_t))
			DEFINE_FUNCTION_POINTER(cairo_pattern_set_filter, void, (cairo_pattern_t, int))
			DEFINE_FUNCTION_POINTER(cairo_pattern_set_extend, void, (cairo_pattern_t, int))
			DEFINE_FUNCTION_POINTER(cairo_save, void, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_restore, void, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_clip, void, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_push_group_with_content, void, (cairo_t, int))
			DEFINE_FUNCTION_POINTER(cairo_pop_group, cairo_pattern_t, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_pop_group_to_source, void, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_paint, void, (cairo_t))
			DEFINE_FUNCTION_POINTER(cairo_paint_with_alpha, void, (cairo_t, double))
			DEFINE_FUNCTION_POINTER(cairo_mask, void, (cairo_t, cairo_pattern_t))
			DEFINE_FUNCTION_POINTER(cairo_ft_font_face_create_for_ft_face, cairo_font_face_t, (FT_Face, int))
			DEFINE_FUNCTION_POINTER(cairo_font_face_status, cairo_status_t, (cairo_font_face_t))
			DEFINE_FUNCTION_POINTER(cairo_font_face_destroy, void, (cairo_font_face_t))
			DEFINE_FUNCTION_POINTER(cairo_font_face_get_user_data, void *, (cairo_font_face_t, handle))
			DEFINE_FUNCTION_POINTER(cairo_font_face_set_user_data, cairo_status_t, (cairo_font_face_t, handle, void *, cairo_destroy_func_t))
			DEFINE_FUNCTION_POINTER(cairo_ft_font_face_set_synthesize, void, (cairo_font_face_t, unsigned int))
			DEFINE_FUNCTION_POINTER(cairo_ft_font_face_unset_synthesize, void, (cairo_font_face_t, unsigned int))
			DEFINE_FUNCTION_POINTER(cairo_ft_font_face_get_synthesize, unsigned int, (cairo_font_face_t))
			DEFINE_FUNCTION_POINTER(cairo_scaled_font_create, cairo_scaled_font_t, (cairo_font_face_t, const cairo_matrix_t &, const cairo_matrix_t &, cairo_font_options_t))
			DEFINE_FUNCTION_POINTER(cairo_scaled_font_status, cairo_status_t, (cairo_scaled_font_t))
			DEFINE_FUNCTION_POINTER(cairo_scaled_font_destroy, void, (cairo_scaled_font_t))
			DEFINE_FUNCTION_POINTER(cairo_ft_scaled_font_lock_face, FT_Face, (cairo_scaled_font_t))
			DEFINE_FUNCTION_POINTER(cairo_ft_scaled_font_unlock_face, void, (cairo_scaled_font_t))
			DEFINE_FUNCTION_POINTER(cairo_set_font_face, void, (cairo_t, cairo_font_face_t))
			DEFINE_FUNCTION_POINTER(cairo_set_font_size, void, (cairo_t, double))
			DEFINE_FUNCTION_POINTER(cairo_set_scaled_font, void, (cairo_t, cairo_scaled_font_t))
			DEFINE_FUNCTION_POINTER(cairo_show_glyphs, void, (cairo_t, const cairo_glyph_t *, int))
			DEFINE_FUNCTION_POINTER(cairo_font_options_create, cairo_font_options_t, (void))
			DEFINE_FUNCTION_POINTER(cairo_font_options_status, cairo_status_t, (cairo_font_options_t))
			DEFINE_FUNCTION_POINTER(cairo_font_options_destroy, void, (cairo_font_options_t))
			DEFINE_FUNCTION_POINTER(cairo_debug_reset_static_data, void, (void))
			#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
			DEFINE_FUNCTION_POINTER(cairo_xlib_surface_create_with_xrender_format, cairo_surface_t, (void *, unsigned long, void *, void *, int, int))
			DEFINE_FUNCTION_POINTER(cairo_xlib_surface_set_drawable, void, (cairo_surface_t, unsigned long, int, int))
			DEFINE_FUNCTION_POINTER(cairo_xlib_surface_set_size, void, (cairo_surface_t, int, int))
			#endif
		public:
			CairoAPI(void);
			virtual ~CairoAPI(void) override;
		};
		class FontConfigAPI : public Object
		{
			handle _library;
		public:
			DEFINE_FUNCTION_POINTER(FcInit, FcBool, (void))
			DEFINE_FUNCTION_POINTER(FcFini, void, (void))
			DEFINE_FUNCTION_POINTER(FcInitLoadConfigAndFonts, FcConfig, (void))
			DEFINE_FUNCTION_POINTER(FcConfigGetFonts, FcFontSet *, (FcConfig, FcSetName))
			DEFINE_FUNCTION_POINTER(FcPatternCreate, FcPattern, (void))
			DEFINE_FUNCTION_POINTER(FcPatternDestroy, void, (FcPattern))
			DEFINE_FUNCTION_POINTER(FcPatternAddInteger, FcBool, (FcPattern, const char *, int))
			DEFINE_FUNCTION_POINTER(FcPatternAddDouble, FcBool, (FcPattern, const char *, double))
			DEFINE_FUNCTION_POINTER(FcPatternAddString, FcBool, (FcPattern, const char *, const FcChar8 *))
			DEFINE_FUNCTION_POINTER(FcPatternAddCharSet, FcBool, (FcPattern, const char *, FcCharSet))
			DEFINE_FUNCTION_POINTER(FcPatternGetInteger, FcResult, (FcPattern, const char *, int, int *))
			DEFINE_FUNCTION_POINTER(FcPatternGetDouble, FcResult, (FcPattern, const char *, int, double *))
			DEFINE_FUNCTION_POINTER(FcPatternGetString, FcResult, (FcPattern, const char *, int, FcChar8 **))
			DEFINE_FUNCTION_POINTER(FcPatternGetCharSet, FcResult, (FcPattern, const char *, int, FcCharSet *))
			DEFINE_FUNCTION_POINTER(FcConfigSubstitute, FcBool, (FcConfig, FcPattern, int))
			DEFINE_FUNCTION_POINTER(FcDefaultSubstitute, void, (FcPattern))
			DEFINE_FUNCTION_POINTER(FcFontSetMatch, FcPattern, (FcConfig, FcFontSet **, int, FcPattern, FcResult *))
			DEFINE_FUNCTION_POINTER(FcCharSetCreate, FcCharSet, (void))
			DEFINE_FUNCTION_POINTER(FcCharSetDestroy, void, (FcCharSet))
			DEFINE_FUNCTION_POINTER(FcCharSetAddChar, FcBool, (FcCharSet, unichar32))
			DEFINE_FUNCTION_POINTER(FcObjectSetBuild, FcObjectSet, (const char *, ...))
			DEFINE_FUNCTION_POINTER(FcObjectSetDestroy, void, (FcObjectSet))
			DEFINE_FUNCTION_POINTER(FcFontList, FcFontSet *, (FcConfig, FcPattern, FcObjectSet))
			DEFINE_FUNCTION_POINTER(FcFontSetDestroy, void, (FcFontSet *))
		public:
			FontConfigAPI(void);
			virtual ~FontConfigAPI(void) override;
		};
		class FreeTypeAPI : public Object
		{
			handle _library;
		public:
			DEFINE_FUNCTION_POINTER(FT_Init_FreeType, int, (FT_Library *))
			DEFINE_FUNCTION_POINTER(FT_Done_FreeType, int, (FT_Library))
			DEFINE_FUNCTION_POINTER(FT_New_Face, int, (FT_Library, const char *, FT_Long, FT_Face *))
			DEFINE_FUNCTION_POINTER(FT_New_Memory_Face, int, (FT_Library, const void *, FT_Long, FT_Long, FT_Face *))
			DEFINE_FUNCTION_POINTER(FT_Done_Face, int, (FT_Face))
			DEFINE_FUNCTION_POINTER(FT_Select_Charmap, int, (FT_Face, int))
			DEFINE_FUNCTION_POINTER(FT_Load_Glyph, int, (FT_Face, FT_UInt, int32))
			DEFINE_FUNCTION_POINTER(FT_Get_Char_Index, FT_UInt, (FT_Face, FT_ULong))
			DEFINE_FUNCTION_POINTER(FT_Select_Size, int, (FT_Face, FT_Int))
		public:
			FreeTypeAPI(void);
			virtual ~FreeTypeAPI(void) override;
		};
	}
}