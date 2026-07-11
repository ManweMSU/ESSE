#include "CairoDL.h"
#include <dlfcn.h>

namespace ESSE
{
	namespace Cairo
	{
		CairoAPI::CairoAPI(void)
		{
			_library = dlopen("libcairo.so", RTLD_NOW);
			if (!_library) throw NotImplementedException();
			try {
				DEFINE_FUNCTION_IMPORT(cairo_version)
				DEFINE_FUNCTION_IMPORT(cairo_format_stride_for_width)
				DEFINE_FUNCTION_IMPORT(cairo_image_surface_create_for_data)
				DEFINE_FUNCTION_IMPORT(cairo_surface_status)
				DEFINE_FUNCTION_IMPORT(cairo_surface_destroy)
				DEFINE_FUNCTION_IMPORT(cairo_surface_mark_dirty)
				DEFINE_FUNCTION_IMPORT(cairo_surface_flush)
				DEFINE_FUNCTION_IMPORT(cairo_create)
				DEFINE_FUNCTION_IMPORT(cairo_status)
				DEFINE_FUNCTION_IMPORT(cairo_destroy)
				DEFINE_FUNCTION_IMPORT(cairo_pattern_create_linear)
				DEFINE_FUNCTION_IMPORT(cairo_pattern_status)
				DEFINE_FUNCTION_IMPORT(cairo_pattern_destroy)
				DEFINE_FUNCTION_IMPORT(cairo_pattern_add_color_stop_rgba)
				DEFINE_FUNCTION_IMPORT(cairo_pattern_set_matrix)
				DEFINE_FUNCTION_IMPORT(cairo_pattern_get_matrix)
				DEFINE_FUNCTION_IMPORT(cairo_set_source_rgba)
				DEFINE_FUNCTION_IMPORT(cairo_set_source)
				DEFINE_FUNCTION_IMPORT(cairo_rectangle)
				DEFINE_FUNCTION_IMPORT(cairo_fill)
				DEFINE_FUNCTION_IMPORT(cairo_stroke)
				DEFINE_FUNCTION_IMPORT(cairo_get_matrix)
				DEFINE_FUNCTION_IMPORT(cairo_set_matrix)
				DEFINE_FUNCTION_IMPORT(cairo_matrix_init)
				DEFINE_FUNCTION_IMPORT(cairo_matrix_init_identity)
				DEFINE_FUNCTION_IMPORT(cairo_matrix_init_translate)
				DEFINE_FUNCTION_IMPORT(cairo_matrix_init_scale)
				DEFINE_FUNCTION_IMPORT(cairo_matrix_init_rotate)
				DEFINE_FUNCTION_IMPORT(cairo_matrix_translate)
				DEFINE_FUNCTION_IMPORT(cairo_matrix_scale)
				DEFINE_FUNCTION_IMPORT(cairo_matrix_rotate)
				DEFINE_FUNCTION_IMPORT(cairo_matrix_invert)
				DEFINE_FUNCTION_IMPORT(cairo_matrix_multiply)
				DEFINE_FUNCTION_IMPORT(cairo_move_to)
				DEFINE_FUNCTION_IMPORT(cairo_line_to)
				DEFINE_FUNCTION_IMPORT(cairo_close_path)
				DEFINE_FUNCTION_IMPORT(cairo_set_line_width)
				DEFINE_FUNCTION_IMPORT(cairo_set_line_join)
				DEFINE_FUNCTION_IMPORT(cairo_set_line_cap)
				DEFINE_FUNCTION_IMPORT(cairo_set_source_rgb)
				DEFINE_FUNCTION_IMPORT(cairo_set_operator)
				DEFINE_FUNCTION_IMPORT(cairo_surface_create_for_rectangle)
				DEFINE_FUNCTION_IMPORT(cairo_pattern_create_for_surface)
				DEFINE_FUNCTION_IMPORT(cairo_pattern_set_filter)
				DEFINE_FUNCTION_IMPORT(cairo_pattern_set_extend)
				DEFINE_FUNCTION_IMPORT(cairo_save)
				DEFINE_FUNCTION_IMPORT(cairo_restore)
				DEFINE_FUNCTION_IMPORT(cairo_clip)
				DEFINE_FUNCTION_IMPORT(cairo_push_group_with_content)
				DEFINE_FUNCTION_IMPORT(cairo_pop_group)
				DEFINE_FUNCTION_IMPORT(cairo_pop_group_to_source)
				DEFINE_FUNCTION_IMPORT(cairo_paint)
				DEFINE_FUNCTION_IMPORT(cairo_paint_with_alpha)
				DEFINE_FUNCTION_IMPORT(cairo_mask)
				DEFINE_FUNCTION_IMPORT(cairo_ft_font_face_create_for_ft_face)
				DEFINE_FUNCTION_IMPORT(cairo_font_face_status)
				DEFINE_FUNCTION_IMPORT(cairo_font_face_destroy)
				DEFINE_FUNCTION_IMPORT(cairo_ft_font_face_set_synthesize)
				DEFINE_FUNCTION_IMPORT(cairo_ft_font_face_unset_synthesize)
				DEFINE_FUNCTION_IMPORT(cairo_ft_font_face_get_synthesize)
				DEFINE_FUNCTION_IMPORT(cairo_set_font_face)
				DEFINE_FUNCTION_IMPORT(cairo_set_font_size)
				DEFINE_FUNCTION_IMPORT(cairo_show_glyphs)
				#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
				DEFINE_FUNCTION_IMPORT(cairo_xlib_surface_create_with_xrender_format)
				DEFINE_FUNCTION_IMPORT(cairo_xlib_surface_set_drawable)
				DEFINE_FUNCTION_IMPORT(cairo_xlib_surface_set_size)
				#endif
			} catch (...) { dlclose(_library); throw; }
		}
		CairoAPI::~CairoAPI(void) { dlclose(_library); }
		FontConfigAPI::FontConfigAPI(void)
		{
			_library = dlopen("libfontconfig.so", RTLD_NOW);
			if (!_library) throw NotImplementedException();
			try {
				DEFINE_FUNCTION_IMPORT(FcInit)
				DEFINE_FUNCTION_IMPORT(FcFini)
				DEFINE_FUNCTION_IMPORT(FcInitLoadConfigAndFonts)
				DEFINE_FUNCTION_IMPORT(FcConfigGetFonts)
				DEFINE_FUNCTION_IMPORT(FcPatternCreate)
				DEFINE_FUNCTION_IMPORT(FcPatternDestroy)
				DEFINE_FUNCTION_IMPORT(FcPatternAddInteger)
				DEFINE_FUNCTION_IMPORT(FcPatternAddDouble)
				DEFINE_FUNCTION_IMPORT(FcPatternAddString)
				DEFINE_FUNCTION_IMPORT(FcPatternAddCharSet)
				DEFINE_FUNCTION_IMPORT(FcPatternGetInteger)
				DEFINE_FUNCTION_IMPORT(FcPatternGetDouble)
				DEFINE_FUNCTION_IMPORT(FcPatternGetString)
				DEFINE_FUNCTION_IMPORT(FcPatternGetCharSet)
				DEFINE_FUNCTION_IMPORT(FcConfigSubstitute)
				DEFINE_FUNCTION_IMPORT(FcDefaultSubstitute)
				DEFINE_FUNCTION_IMPORT(FcFontSetMatch)
				DEFINE_FUNCTION_IMPORT(FcCharSetCreate)
				DEFINE_FUNCTION_IMPORT(FcCharSetDestroy)
				DEFINE_FUNCTION_IMPORT(FcCharSetAddChar)
				DEFINE_FUNCTION_IMPORT(FcObjectSetBuild)
				DEFINE_FUNCTION_IMPORT(FcObjectSetDestroy)
				DEFINE_FUNCTION_IMPORT(FcFontList)
				DEFINE_FUNCTION_IMPORT(FcFontSetDestroy)
			} catch (...) { dlclose(_library); throw; }
		}
		FontConfigAPI::~FontConfigAPI(void) { dlclose(_library); }
		FreeTypeAPI::FreeTypeAPI(void)
		{
			_library = dlopen("libfreetype.so", RTLD_NOW);
			if (!_library) throw NotImplementedException();
			try {
				DEFINE_FUNCTION_IMPORT(FT_Init_FreeType)
				DEFINE_FUNCTION_IMPORT(FT_Done_FreeType)
				DEFINE_FUNCTION_IMPORT(FT_New_Face)
				DEFINE_FUNCTION_IMPORT(FT_New_Memory_Face)
				DEFINE_FUNCTION_IMPORT(FT_Done_Face)
				DEFINE_FUNCTION_IMPORT(FT_Select_Charmap)
				DEFINE_FUNCTION_IMPORT(FT_Load_Glyph)
				DEFINE_FUNCTION_IMPORT(FT_Get_Char_Index)
				DEFINE_FUNCTION_IMPORT(FT_Select_Size)
			} catch (...) { dlclose(_library); throw; }
		}
		FreeTypeAPI::~FreeTypeAPI(void) { dlclose(_library); }
	}
}