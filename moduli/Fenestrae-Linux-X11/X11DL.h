#pragma once

#include <Cor/IO/CorDL.h>
#include <Cor/Classes/CorObject.h>

namespace ESSE
{
	namespace X11
	{
		#define DEFINE_HANDLE_TYPE(NAME) typedef struct __internal_##NAME * NAME;
		#define DEFINE_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
		#define DEFINE_FUNCTION_IMPORT(NAME) NAME = reinterpret_cast<func_##NAME>(ESSE::IO::GetLibraryRoutine(_library, #NAME)); if (!NAME) throw NotImplementedException();

		constexpr int True = 1;
		constexpr int False = 0;
		constexpr int Success = 0;
		constexpr int BadAlloc = 11;
		constexpr int IsUnmapped = 0;
		constexpr int XkbNumVirtualMods = 16;
		constexpr int XkbNumIndicators = 32;
		constexpr int XkbNumKbdGroups = 4;
		constexpr int XkbKeyNameLength = 4;
		constexpr int XkbMajorVersion = 1;
		constexpr int XkbMinorVersion = 0;
		constexpr int XkbAllComponentsMask = 0x7f;
		constexpr int XkbUseCoreKbd = 0x0100;

		constexpr int ForgetGravity = 0;
		constexpr int NorthWestGravity = 1;
		constexpr int NorthGravity = 2;
		constexpr int NorthEastGravity = 3;
		constexpr int WestGravity = 4;
		constexpr int CenterGravity = 5;
		constexpr int EastGravity = 6;
		constexpr int SouthWestGravity = 7;
		constexpr int SouthGravity = 8;
		constexpr int SouthEastGravity = 9;
		constexpr int StaticGravity = 10;

		constexpr int NoEventMask = 0L;
		constexpr int KeyPressMask = (1L<<0);
		constexpr int KeyReleaseMask = (1L<<1);
		constexpr int ButtonPressMask = (1L<<2);
		constexpr int ButtonReleaseMask = (1L<<3);
		constexpr int EnterWindowMask = (1L<<4);
		constexpr int LeaveWindowMask = (1L<<5);
		constexpr int PointerMotionMask = (1L<<6);
		constexpr int PointerMotionHintMask = (1L<<7);
		constexpr int Button1MotionMask = (1L<<8);
		constexpr int Button2MotionMask = (1L<<9);
		constexpr int Button3MotionMask = (1L<<10);
		constexpr int Button4MotionMask = (1L<<11);
		constexpr int Button5MotionMask = (1L<<12);
		constexpr int ButtonMotionMask = (1L<<13);
		constexpr int KeymapStateMask = (1L<<14);
		constexpr int ExposureMask = (1L<<15);
		constexpr int VisibilityChangeMask = (1L<<16);
		constexpr int StructureNotifyMask = (1L<<17);
		constexpr int ResizeRedirectMask = (1L<<18);
		constexpr int SubstructureNotifyMask = (1L<<19);
		constexpr int SubstructureRedirectMask = (1L<<20);
		constexpr int FocusChangeMask = (1L<<21);
		constexpr int PropertyChangeMask = (1L<<22);
		constexpr int ColormapChangeMask = (1L<<23);
		constexpr int OwnerGrabButtonMask = (1L<<24);

		constexpr int KeyPress = 2;
		constexpr int KeyRelease = 3;
		constexpr int ButtonPress = 4;
		constexpr int ButtonRelease = 5;
		constexpr int MotionNotify = 6;
		constexpr int EnterNotify = 7;
		constexpr int LeaveNotify = 8;
		constexpr int FocusIn = 9;
		constexpr int FocusOut = 10;
		constexpr int KeymapNotify = 11;
		constexpr int Expose = 12;
		constexpr int GraphicsExpose = 13;
		constexpr int NoExpose = 14;
		constexpr int VisibilityNotify = 15;
		constexpr int CreateNotify = 16;
		constexpr int DestroyNotify = 17;
		constexpr int UnmapNotify = 18;
		constexpr int MapNotify = 19;
		constexpr int MapRequest = 20;
		constexpr int ReparentNotify = 21;
		constexpr int ConfigureNotify = 22;
		constexpr int ConfigureRequest = 23;
		constexpr int GravityNotify = 24;
		constexpr int ResizeRequest = 25;
		constexpr int CirculateNotify = 26;
		constexpr int CirculateRequest = 27;
		constexpr int PropertyNotify = 28;
		constexpr int SelectionClear = 29;
		constexpr int SelectionRequest = 30;
		constexpr int SelectionNotify = 31;
		constexpr int ColormapNotify = 32;
		constexpr int ClientMessage = 33;
		constexpr int MappingNotify = 34;
		constexpr int GenericEvent = 35;

		constexpr int TrueColor = 4;
		constexpr int AllocNone = 0;
		constexpr int XYBitmap = 0;
		constexpr int XYPixmap = 1;
		constexpr int ZPixmap = 2;
		constexpr int LSBFirst = 0;
		constexpr int MSBFirst = 1;
		constexpr int AnyPropertyType = 0L;
		constexpr int Button1 = 1;
		constexpr int Button2 = 2;
		constexpr int Button3 = 3;
		constexpr int Button4 = 4;
		constexpr int Button5 = 5;
		constexpr int ShiftMask = (1<<0);
		constexpr int LockMask = (1<<1);
		constexpr int ControlMask = (1<<2);
		constexpr int Button1Mask = (1<<8);
		constexpr int Button2Mask = (1<<9);
		constexpr int Button3Mask = (1<<10);
		constexpr int CopyFromParent = 0L;

		constexpr long InputOutput = 1;
		constexpr long InputOnly = 2;
		constexpr long CWBackPixmap = (1L<<0);
		constexpr long CWBackPixel = (1L<<1);
		constexpr long CWBorderPixmap = (1L<<2);
		constexpr long CWBorderPixel = (1L<<3);
		constexpr long CWBitGravity = (1L<<4);
		constexpr long CWWinGravity = (1L<<5);
		constexpr long CWBackingStore = (1L<<6);
		constexpr long CWBackingPlanes = (1L<<7);
		constexpr long CWBackingPixel = (1L<<8);
		constexpr long CWOverrideRedirect = (1L<<9);
		constexpr long CWSaveUnder = (1L<<10);
		constexpr long CWEventMask = (1L<<11);
		constexpr long CWDontPropagate = (1L<<12);
		constexpr long CWColormap = (1L<<13);
		constexpr long CWCursor = (1L<<14);

		constexpr int PropModeReplace = 0;
		constexpr int PropModePrepend = 1;
		constexpr int PropModeAppend = 2;
		constexpr int PropertyNewValue = 0;
		constexpr int PropertyDelete = 1;
		constexpr int GrabModeSync = 0;
		constexpr int GrabModeAsync = 1;
		constexpr int XBufferOverflow = -1;
		constexpr int XLookupNone = 1;
		constexpr int XLookupChars = 2;
		constexpr int XLookupKeySym = 3;
		constexpr int XLookupBoth = 4;

		constexpr long CurrentTime = 0L;
		constexpr long XIMPreeditNothing = 0x0008L;
		constexpr long XIMStatusNothing = 0x0400L;

		constexpr const char * XNClientWindow = "clientWindow";
		constexpr const char * XNInputStyle = "inputStyle";

		DEFINE_HANDLE_TYPE(GC)
		DEFINE_HANDLE_TYPE(XIC)
		DEFINE_HANDLE_TYPE(XIM)

		typedef int Bool;
		typedef int Status;
		typedef char * XPointer;
		typedef unsigned long XID;
		typedef unsigned long Mask;
		typedef unsigned long Atom;
		typedef unsigned long VisualID;
		typedef unsigned long Time;
		typedef XID Window;
		typedef XID Drawable;
		typedef XID Font;
		typedef XID Pixmap;
		typedef XID Cursor;
		typedef XID Colormap;
		typedef XID GContext;
		typedef XID KeySym;
		typedef XID Picture;
		typedef XID PictFormat;
		typedef XID RROutput;
		typedef unsigned char KeyCode;
		struct _XDisplay;
		typedef struct _XExtData {
			int number;
			struct _XExtData * next;
			int (* free_private) (struct _XExtData * extension);
			XPointer private_data;
		} XExtData;
		typedef struct {
			XExtData * ext_data;
			int depth;
			int bits_per_pixel;
			int scanline_pad;
		} ScreenFormat;
		typedef struct {
			XExtData * ext_data;
			VisualID visualid;
			int c_class;
			unsigned long red_mask, green_mask, blue_mask;
			int bits_per_rgb;
			int map_entries;
		} Visual;
		typedef struct {
			Visual * visual;
			VisualID visualid;
			int screen;
			int depth;
			int c_class;
			unsigned long red_mask;
			unsigned long green_mask;
			unsigned long blue_mask;
			int colormap_size;
			int bits_per_rgb;
		} XVisualInfo;
		typedef struct {
			int depth;
			int nvisuals;
			Visual * visuals;
		} Depth;
		typedef struct {
			XExtData * ext_data;
			struct _XDisplay * display;
			Window root;
			int width, height;
			int mwidth, mheight;
			int ndepths;
			Depth * depths;
			int root_depth;
			Visual * root_visual;
			GC default_gc;
			Colormap cmap;
			unsigned long white_pixel;
			unsigned long black_pixel;
			int max_maps, min_maps;
			int backing_store;
			Bool save_unders;
			long root_input_mask;
		} Screen;
		typedef struct _XDisplay {
			XExtData * ext_data;
			void * private1;
			int fd;
			int private2;
			int proto_major_version;
			int proto_minor_version;
			char * vendor;
			XID private3;
			XID private4;
			XID private5;
			int private6;
			XID (* resource_alloc) (struct _XDisplay *);
			int byte_order;
			int bitmap_unit;
			int bitmap_pad;
			int bitmap_bit_order;
			int nformats;
			ScreenFormat * pixmap_format;
			int private8;
			int release;
			void * private9, * private10;
			int qlen;
			unsigned long last_request_read;
			unsigned long request;
			XPointer private11;
			XPointer private12;
			XPointer private13;
			XPointer private14;
			unsigned max_request_size;
			void * db;
			int (* private15) (struct _XDisplay *);
			char * display_name;
			int default_screen;
			int nscreens;
			Screen * screens;
			unsigned long motion_buffer;
			unsigned long private16;
			int min_keycode;
			int max_keycode;
			XPointer private17;
			XPointer private18;
			int private19;
			char * xdefaults;
		} Display;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			Window root;
			Window subwindow;
			Time time;
			int x, y;
			int x_root, y_root;
			unsigned int state;
			unsigned int keycode;
			Bool same_screen;
		} XKeyEvent;
		typedef XKeyEvent XKeyPressedEvent;
		typedef XKeyEvent XKeyReleasedEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			Window root;
			Window subwindow;
			Time time;
			int x, y;
			int x_root, y_root;
			unsigned int state;
			unsigned int button;
			Bool same_screen;
		} XButtonEvent;
		typedef XButtonEvent XButtonPressedEvent;
		typedef XButtonEvent XButtonReleasedEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			Window root;
			Window subwindow;
			Time time;
			int x, y;
			int x_root, y_root;
			unsigned int state;
			char is_hint;
			Bool same_screen;
		} XMotionEvent;
		typedef XMotionEvent XPointerMovedEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			Window root;
			Window subwindow;
			Time time;
			int x, y;
			int x_root, y_root;
			int mode;
			int detail;
			Bool same_screen;
			Bool focus;
			unsigned int state;
		} XCrossingEvent;
		typedef XCrossingEvent XEnterWindowEvent;
		typedef XCrossingEvent XLeaveWindowEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			int mode;
			int detail;
		} XFocusChangeEvent;
		typedef XFocusChangeEvent XFocusInEvent;
		typedef XFocusChangeEvent XFocusOutEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			char key_vector[32];
		} XKeymapEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			int x, y;
			int width, height;
			int count;
		} XExposeEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Drawable drawable;
			int x, y;
			int width, height;
			int count;
			int major_code;
			int minor_code;
		} XGraphicsExposeEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Drawable drawable;
			int major_code;
			int minor_code;
		} XNoExposeEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			int state;
		} XVisibilityEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window parent;
			Window window;
			int x, y;
			int width, height;
			int border_width;
			Bool override_redirect;
		} XCreateWindowEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window event;
			Window window;
		} XDestroyWindowEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window event;
			Window window;
			Bool from_configure;
		} XUnmapEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window event;
			Window window;
			Bool override_redirect;
		} XMapEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window parent;
			Window window;
		} XMapRequestEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window event;
			Window window;
			Window parent;
			int x, y;
			Bool override_redirect;
		} XReparentEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window event;
			Window window;
			int x, y;
			int width, height;
			int border_width;
			Window above;
			Bool override_redirect;
		} XConfigureEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window event;
			Window window;
			int x, y;
		} XGravityEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			int width, height;
		} XResizeRequestEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window parent;
			Window window;
			int x, y;
			int width, height;
			int border_width;
			Window above;
			int detail;
			unsigned long value_mask;
		} XConfigureRequestEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window event;
			Window window;
			int place;
		} XCirculateEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window parent;
			Window window;
			int place;
		} XCirculateRequestEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			Atom atom;
			Time time;
			int state;
		} XPropertyEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			Atom selection;
			Time time;
		} XSelectionClearEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window owner;
			Window requestor;
			Atom selection;
			Atom target;
			Atom property;
			Time time;
		} XSelectionRequestEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window requestor;
			Atom selection;
			Atom target;
			Atom property;
			Time time;
		} XSelectionEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			Colormap colormap;
			Bool c_new;
			int state;
		} XColormapEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			Atom message_type;
			int format;
			union { char b[20]; short s[10]; long l[5]; } data;
		} XClientMessageEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
			int request;
			int first_keycode;
			int count;
		} XMappingEvent;
		typedef struct {
			int type;
			Display * display;
			XID resourceid;
			unsigned long serial;
			unsigned char error_code;
			unsigned char request_code;
			unsigned char minor_code;
		} XErrorEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			Window window;
		} XAnyEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			int extension;
			int evtype;
		} XGenericEvent;
		typedef struct {
			int type;
			unsigned long serial;
			Bool send_event;
			Display * display;
			int extension;
			int evtype;
			unsigned int cookie;
			void * data;
		} XGenericEventCookie;
		typedef union _XEvent {
			int type;
			XAnyEvent xany;
			XKeyEvent xkey;
			XButtonEvent xbutton;
			XMotionEvent xmotion;
			XCrossingEvent xcrossing;
			XFocusChangeEvent xfocus;
			XExposeEvent xexpose;
			XGraphicsExposeEvent xgraphicsexpose;
			XNoExposeEvent xnoexpose;
			XVisibilityEvent xvisibility;
			XCreateWindowEvent xcreatewindow;
			XDestroyWindowEvent xdestroywindow;
			XUnmapEvent xunmap;
			XMapEvent xmap;
			XMapRequestEvent xmaprequest;
			XReparentEvent xreparent;
			XConfigureEvent xconfigure;
			XGravityEvent xgravity;
			XResizeRequestEvent xresizerequest;
			XConfigureRequestEvent xconfigurerequest;
			XCirculateEvent xcirculate;
			XCirculateRequestEvent xcirculaterequest;
			XPropertyEvent xproperty;
			XSelectionClearEvent xselectionclear;
			XSelectionRequestEvent xselectionrequest;
			XSelectionEvent xselection;
			XColormapEvent xcolormap;
			XClientMessageEvent xclient;
			XMappingEvent xmapping;
			XErrorEvent xerror;
			XKeymapEvent xkeymap;
			XGenericEvent xgeneric;
			XGenericEventCookie xcookie;
			long pad[24];
		} XEvent;
		typedef struct _XImage {
			int width, height;
			int xoffset;
			int format;
			char *data;
			int byte_order;
			int bitmap_unit;
			int bitmap_bit_order;
			int bitmap_pad;
			int depth;
			int bytes_per_line;
			int bits_per_pixel;
			unsigned long red_mask;
			unsigned long green_mask;
			unsigned long blue_mask;
			XPointer obdata;
			struct funcs {
				struct _XImage * (* create_image) (struct _XDisplay *, Visual *, unsigned int, int, int, char *, unsigned int, unsigned int, int, int);
				int (* destroy_image) (struct _XImage *);
				unsigned long (* get_pixel) (struct _XImage *, int, int);
				int (* put_pixel) (struct _XImage *, int, int, unsigned long);
				struct _XImage * (* sub_image) (struct _XImage *, int, int, unsigned int, unsigned int);
				int (* add_pixel) (struct _XImage *, long);
			} f;
		} XImage;
		typedef struct {
			Pixmap background_pixmap;
			unsigned long background_pixel;
			Pixmap border_pixmap;
			unsigned long border_pixel;
			int bit_gravity;
			int win_gravity;
			int backing_store;
			unsigned long backing_planes;
			unsigned long backing_pixel;
			Bool save_under;
			long event_mask;
			long do_not_propagate_mask;
			Bool override_redirect;
			Colormap colormap;
			Cursor cursor;
		} XSetWindowAttributes;
		typedef struct {
			int x, y;
			int width, height;
			int border_width;
			int depth;
			Visual * visual;
			Window root;
			int c_class;
			int bit_gravity;
			int win_gravity;
			int backing_store;
			unsigned long backing_planes;
			unsigned long backing_pixel;
			Bool save_under;
			Colormap colormap;
			Bool map_installed;
			int map_state;
			long all_event_masks;
			long your_event_mask;
			long do_not_propagate_mask;
			Bool override_redirect;
			Screen * screen;
		} XWindowAttributes;
		typedef struct {
			long flags;
			int x, y;
			int width, height;
			int min_width, min_height;
			int max_width, max_height;
			int width_inc, height_inc;
			struct {
				int x;
				int y;
			} min_aspect, max_aspect;
			int base_width, base_height;
			int win_gravity;
		} XSizeHints;
		typedef struct {
			long flags;
			Bool input;
			int initial_state;
			Pixmap icon_pixmap;
			Window icon_window;
			int icon_x, icon_y;
			Pixmap icon_mask;
			XID window_group;
		} XWMHints;
		typedef struct {
			const char * res_name;
			const char * res_class;
		} XClassHint;
		typedef struct _XkbStateRec {
			unsigned char group;
			unsigned char locked_group;
			unsigned short base_group;
			unsigned short latched_group;
			unsigned char mods;
			unsigned char base_mods;
			unsigned char latched_mods;
			unsigned char locked_mods;
			unsigned char compat_state;
			unsigned char grab_mods;
			unsigned char compat_grab_mods;
			unsigned char lookup_mods;
			unsigned char compat_lookup_mods;
			unsigned short ptr_buttons;
		} XkbStateRec, * XkbStatePtr;
		typedef	struct _XkbKeyNameRec {
			char name[XkbKeyNameLength];
		} XkbKeyNameRec, * XkbKeyNamePtr;
		typedef struct _XkbKeyAliasRec {
			char real[XkbKeyNameLength];
			char alias[XkbKeyNameLength];
		} XkbKeyAliasRec, * XkbKeyAliasPtr;
		typedef struct _XkbServerMapRec {
			unsigned short num_acts;
			unsigned short size_acts;
			void * acts;
			void * behaviors;
			unsigned short * key_acts;
			unsigned char * c_explicit;
			unsigned char vmods[XkbNumVirtualMods];
			unsigned short * vmodmap;
		} XkbServerMapRec, * XkbServerMapPtr;
		typedef	struct _XkbSymMapRec {
			unsigned char kt_index[XkbNumKbdGroups];
			unsigned char group_info;
			unsigned char width;
			unsigned short offset;
		} XkbSymMapRec, * XkbSymMapPtr;
		typedef struct _XkbClientMapRec {
			unsigned char size_types;
			unsigned char num_types;
			void * types;
			unsigned short size_syms;
			unsigned short num_syms;
			KeySym * syms;
			XkbSymMapPtr key_sym_map;
			unsigned char * modmap;
		} XkbClientMapRec, * XkbClientMapPtr;
		typedef struct _XkbNamesRec {
			Atom keycodes;
			Atom geometry;
			Atom symbols;
			Atom types;
			Atom compat;
			Atom vmods[XkbNumVirtualMods];
			Atom indicators[XkbNumIndicators];
			Atom groups[XkbNumKbdGroups];
			XkbKeyNamePtr keys;
			XkbKeyAliasPtr key_aliases;
			Atom * radio_groups;
			Atom phys_symbols;
			unsigned char num_keys;
			unsigned char num_key_aliases;
			unsigned short num_rg;
		} XkbNamesRec, * XkbNamesPtr;
		typedef	struct _XkbDesc {
			Display * dpy;
			unsigned short flags;
			unsigned short device_spec;
			KeyCode min_key_code;
			KeyCode max_key_code;
			void * ctrls;
			XkbServerMapPtr server;
			XkbClientMapPtr map;
			void * indicators;
			XkbNamesPtr names;
			void * compat;
			void * geom;
		} XkbDescRec, *XkbDescPtr;
		typedef struct _XRRMonitorInfo {
			Atom name;
			Bool primary;
			Bool automatic;
			int noutput;
			int x;
			int y;
			int width;
			int height;
			int mwidth;
			int mheight;
			RROutput * outputs;
		} XRRMonitorInfo;
		typedef struct {
			short red;
			short redMask;
			short green;
			short greenMask;
			short blue;
			short blueMask;
			short alpha;
			short alphaMask;
		} XRenderDirectFormat;
		typedef struct {
			PictFormat id;
			int type;
			int depth;
			XRenderDirectFormat direct;
			Colormap colormap;
		} XRenderPictFormat;
		typedef struct _XRenderPictureAttributes {
			int repeat;
			Picture alpha_map;
			int alpha_x_origin;
			int alpha_y_origin;
			int clip_x_origin;
			int clip_y_origin;
			Pixmap clip_mask;
			Bool graphics_exposures;
			int subwindow_mode;
			int poly_edge;
			int poly_mode;
			Atom dither;
			Bool component_alpha;
		} XRenderPictureAttributes;
		typedef int (* XErrorHandler) (Display *, XErrorEvent *);

		constexpr int VisualIDMask = 0x1;
		constexpr int InputHint = 1L << 0;
		constexpr int PictStandardARGB32 = 0;

		constexpr int XC_arrow = 2;
		constexpr int XC_xterm = 152;
		constexpr int XC_hand1 = 58;
		constexpr int XC_sb_h_double_arrow = 108;
		constexpr int XC_sb_v_double_arrow = 116;
		constexpr int XC_fleur = 52;

		constexpr long PPosition = 1L << 2;
		constexpr long PSize = 1L << 3;
		constexpr long PMinSize = 1L << 4;
		constexpr long PMaxSize = 1L << 5;
		constexpr long PResizeInc = 1L << 6;
		constexpr long PAspect = 1L << 7;
		constexpr long PBaseSize = 1L << 8;
		constexpr long PWinGravity = 1L << 9;

		class XLibAPI : public Object
		{
			handle _library;
		public:
			DEFINE_FUNCTION_POINTER(XOpenDisplay, Display *, (const char *))
			DEFINE_FUNCTION_POINTER(XCloseDisplay, int, (Display *))
			DEFINE_FUNCTION_POINTER(XConnectionNumber, int, (Display *))
			DEFINE_FUNCTION_POINTER(XPending, int, (Display *))
			DEFINE_FUNCTION_POINTER(XNextEvent, int, (Display *, XEvent *))
			DEFINE_FUNCTION_POINTER(XSendEvent, Status, (Display *, Window, Bool, long, XEvent *))
			DEFINE_FUNCTION_POINTER(XRefreshKeyboardMapping, int, (XMappingEvent *))
			DEFINE_FUNCTION_POINTER(XFlush, int, (Display *))
			DEFINE_FUNCTION_POINTER(XCheckIfEvent, Bool, (Display *, XEvent *, Bool (*) (Display *, XEvent *, XPointer), XPointer))
			DEFINE_FUNCTION_POINTER(XIfEvent, int, (Display *, XEvent *, Bool (*) (Display *, XEvent *, XPointer), XPointer))
			DEFINE_FUNCTION_POINTER(XDefaultScreen, int, (Display *))
			DEFINE_FUNCTION_POINTER(XRootWindow, Window, (Display *, int))
			DEFINE_FUNCTION_POINTER(XGetImage, XImage *, (Display *, Drawable, int, int, unsigned int, unsigned int, unsigned long, int))
			DEFINE_FUNCTION_POINTER(XDefaultScreenOfDisplay, Screen *, (Display *))
			DEFINE_FUNCTION_POINTER(XWidthOfScreen, int, (Screen *))
			DEFINE_FUNCTION_POINTER(XHeightOfScreen, int, (Screen *))
			DEFINE_FUNCTION_POINTER(XGetAtomName, char *, (Display *, Atom))
			DEFINE_FUNCTION_POINTER(XFree, int, (void *))
			DEFINE_FUNCTION_POINTER(XResourceManagerString, char *, (Display *))
			DEFINE_FUNCTION_POINTER(XSetErrorHandler, XErrorHandler, (XErrorHandler))
			DEFINE_FUNCTION_POINTER(XGetWindowProperty, int, (Display *, Window, Atom, long, long, Bool, Atom, Atom *, int *, unsigned long *, unsigned long *, void **))
			DEFINE_FUNCTION_POINTER(XInternAtom, Atom, (Display *, const char *, Bool))
			DEFINE_FUNCTION_POINTER(XCreateWindow, Window, (Display *, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual *, unsigned int, XSetWindowAttributes *))
			DEFINE_FUNCTION_POINTER(XDestroyWindow, int, (Display *, Window))
			DEFINE_FUNCTION_POINTER(XChangeProperty, int, (Display *, Window, Atom, Atom, int, int, const void *, int))
			DEFINE_FUNCTION_POINTER(XSync, int, (Display *, Bool))
			DEFINE_FUNCTION_POINTER(XChangeWindowAttributes, int, (Display *, Window, unsigned long, XSetWindowAttributes *))
			DEFINE_FUNCTION_POINTER(XQueryKeymap, int, (Display *, char[32]))
			DEFINE_FUNCTION_POINTER(XGrabKey, int, (Display *, int, unsigned int, Window, Bool, int, int))
			DEFINE_FUNCTION_POINTER(XUngrabKey, int, (Display *, int, unsigned int, Window))
			DEFINE_FUNCTION_POINTER(XGrabPointer, int, (Display *, Window, Bool, unsigned int, int, int, Window, Cursor, Time))
			DEFINE_FUNCTION_POINTER(XUngrabPointer, int, (Display *, Time))
			DEFINE_FUNCTION_POINTER(XGetSelectionOwner, Window, (Display *, Atom))
			DEFINE_FUNCTION_POINTER(XSetSelectionOwner, int, (Display *, Atom, Window, Time))
			DEFINE_FUNCTION_POINTER(XConvertSelection, int, (Display *, Atom, Atom, Atom, Window, Time))
			DEFINE_FUNCTION_POINTER(XCreatePixmap, Pixmap, (Display *, Drawable, unsigned int, unsigned int, unsigned int))
			DEFINE_FUNCTION_POINTER(XFreePixmap, int, (Display *, Pixmap))
			DEFINE_FUNCTION_POINTER(XCreateGC, GC, (Display *, Drawable, unsigned long, void *))
			DEFINE_FUNCTION_POINTER(XFreeGC, int, (Display *, GC))
			DEFINE_FUNCTION_POINTER(XFlushGC, void, (Display *, GC))
			DEFINE_FUNCTION_POINTER(XSetForeground, int, (Display *, GC, unsigned long))
			DEFINE_FUNCTION_POINTER(XFillRectangle, int, (Display *, Drawable, GC, int, int, unsigned int, unsigned int))
			DEFINE_FUNCTION_POINTER(XInitImage, Status, (XImage *))
			DEFINE_FUNCTION_POINTER(XPutImage, int, (Display *, Drawable, GC, XImage *, int, int, int, int, unsigned int, unsigned int))
			DEFINE_FUNCTION_POINTER(XCreateFontCursor, Cursor, (Display *, unsigned int))
			DEFINE_FUNCTION_POINTER(XQueryBestCursor, Status, (Display *, Drawable, unsigned int, unsigned int, unsigned int *, unsigned int *))
			DEFINE_FUNCTION_POINTER(XFreeCursor, int, (Display *, Cursor))
			DEFINE_FUNCTION_POINTER(XQueryPointer, Bool, (Display *, Window, Window *, Window *, int *, int *, int *, int *, unsigned int *))
			DEFINE_FUNCTION_POINTER(XWarpPointer, int, (Display *, Window, Window, int, int, unsigned int, unsigned int, int, int))
			DEFINE_FUNCTION_POINTER(XMapRaised, int, (Display *, Window))
			DEFINE_FUNCTION_POINTER(XUnmapWindow, int, (Display *, Window))
			DEFINE_FUNCTION_POINTER(XWithdrawWindow, Status, (Display *, Window, int))
			DEFINE_FUNCTION_POINTER(XSetSizeHints, int, (Display *, Window, XSizeHints *, Atom))
			DEFINE_FUNCTION_POINTER(XResizeWindow, int, (Display *, Window, unsigned int, unsigned int))
			DEFINE_FUNCTION_POINTER(XMoveWindow, int, (Display *, Window, int, int))
			DEFINE_FUNCTION_POINTER(XCreateIC, XIC, (XIM, ...))
			DEFINE_FUNCTION_POINTER(XDestroyIC, void, (XIC))
			DEFINE_FUNCTION_POINTER(XSetICFocus, void, (XIC))
			DEFINE_FUNCTION_POINTER(XUnsetICFocus, void, (XIC))
			DEFINE_FUNCTION_POINTER(XwcLookupString, int, (XIC, XKeyPressedEvent *, unichar32 *, int, KeySym *, Status *))
			DEFINE_FUNCTION_POINTER(XDefineCursor, int, (Display *, Window, Cursor))
			DEFINE_FUNCTION_POINTER(XMatchVisualInfo, Status, (Display *, int, int, int, XVisualInfo *))
			DEFINE_FUNCTION_POINTER(XCreateColormap, Colormap, (Display *, Window, Visual *, int))
			DEFINE_FUNCTION_POINTER(XFreeColormap, int, (Display *, Colormap))
			DEFINE_FUNCTION_POINTER(XSetWMHints, int, (Display *, Window, XWMHints *))
			DEFINE_FUNCTION_POINTER(XOpenIM, XIM, (Display *, void *, char *, char *))
			DEFINE_FUNCTION_POINTER(XCloseIM, Status, (XIM))
			DEFINE_FUNCTION_POINTER(XMapWindow, int, (Display *, Window))
			DEFINE_FUNCTION_POINTER(XIconifyWindow, Status, (Display *, Window, int))
			DEFINE_FUNCTION_POINTER(XDefaultRootWindow, Window, (Display *))
			DEFINE_FUNCTION_POINTER(XGetInputFocus, int, (Display *, Window *, int *))
			DEFINE_FUNCTION_POINTER(XSetInputFocus, int, (Display *, Window, int, Time))
			DEFINE_FUNCTION_POINTER(XTranslateCoordinates, Bool, (Display *, Window, Window, int, int, int *, int *, Window *))
			DEFINE_FUNCTION_POINTER(XCopyArea, int, (Display *, Drawable, Drawable, GC, int, int, unsigned int, unsigned int, int, int))
			DEFINE_FUNCTION_POINTER(XGetWindowAttributes, Status, (Display *, Window, XWindowAttributes *))
			DEFINE_FUNCTION_POINTER(XDefaultVisual, Visual *, (Display *, int))
			DEFINE_FUNCTION_POINTER(XQueryTree, Status, (Display *, Window, Window *, Window *, Window **, unsigned int *))
			DEFINE_FUNCTION_POINTER(XGetVisualInfo, XVisualInfo *, (Display *, long, XVisualInfo *, int *))
			DEFINE_FUNCTION_POINTER(XSelectInput, int, (Display *, Window, long))
			DEFINE_FUNCTION_POINTER(XSetClassHint, int, (Display *, Window, XClassHint *))
			DEFINE_FUNCTION_POINTER(XkbLibraryVersion, Bool, (int *, int *))
			DEFINE_FUNCTION_POINTER(XkbQueryExtension, Bool, (Display *, int *, int *, int *, int *, int *))
			DEFINE_FUNCTION_POINTER(XkbGetMap, XkbDescPtr, (Display *, unsigned int, unsigned int))
			DEFINE_FUNCTION_POINTER(XkbFreeKeyboard, void, (XkbDescPtr, unsigned int, Bool))
			DEFINE_FUNCTION_POINTER(XkbGetState, Status, (Display *, unsigned int, XkbStatePtr))
			DEFINE_FUNCTION_POINTER(XkbGetAutoRepeatRate, Bool, (Display *, unsigned int, unsigned int *, unsigned int *))
			DEFINE_FUNCTION_POINTER(XkbBell, Bool, (Display *, Window, int, Atom))
		public:
			XLibAPI(void);
			virtual ~XLibAPI(void) override;
		};
		class XRANDRAPI : public Object
		{
			handle _library;
		public:
			DEFINE_FUNCTION_POINTER(XRRQueryExtension, Bool, (Display *, int *, int *))
			DEFINE_FUNCTION_POINTER(XRRQueryVersion, Status, (Display *, int *, int *))
			DEFINE_FUNCTION_POINTER(XRRGetMonitors, XRRMonitorInfo *, (Display *, Window, Bool, int *))
			DEFINE_FUNCTION_POINTER(XRRFreeMonitors, void, (XRRMonitorInfo *))
		public:
			XRANDRAPI(void);
			virtual ~XRANDRAPI(void) override;
		};
		class XRenderAPI : public Object
		{
			handle _library;
		public:
			DEFINE_FUNCTION_POINTER(XRenderQueryExtension, Bool, (Display *, int *, int *))
			DEFINE_FUNCTION_POINTER(XRenderFindStandardFormat, XRenderPictFormat *, (Display *, int))
			DEFINE_FUNCTION_POINTER(XRenderCreatePicture, Picture, (Display *, Drawable, const XRenderPictFormat *, unsigned long, const XRenderPictureAttributes *))
			DEFINE_FUNCTION_POINTER(XRenderFreePicture, void, (Display *, Picture))
			DEFINE_FUNCTION_POINTER(XRenderCreateCursor, Cursor, (Display *, Picture, unsigned int, unsigned int))
			DEFINE_FUNCTION_POINTER(XRenderFindVisualFormat, XRenderPictFormat *, (Display *, const Visual *))
		public:
			XRenderAPI(void);
			virtual ~XRenderAPI(void) override;
		};
		class XCursorAPI : public Object
		{
			handle _library;
		public:
			DEFINE_FUNCTION_POINTER(XcursorLibraryLoadCursor, Cursor, (Display *, const char *))
		public:
			XCursorAPI(void);
			virtual ~XCursorAPI(void) override;
		};

		extern int LastXError;
	}
}