#pragma once

#include "../Classes/CorObject.h"
#include "../Classes/CorArray.hxx"
#include "../Images/CorGraphics.h"
#include "../Tasks/CorTasks.h"

namespace ESSE
{
	namespace Windows
	{
		class IScreen;
		class IWindowCallback;
		class IWindow;
		class IStatusBarIcon;
		class IMenuItem;
		class IMenu;

		enum class ThemeColor : uint {
			Accent = 0,
			WindowBackgroup = 1, WindowText = 2, SelectedBackground = 3, SelectedText = 4,
			MenuBackground = 5, MenuText = 6, MenuHotBackground = 7, MenuHotText = 8,
			GrayedText = 9, Hyperlink = 10
		};
		enum class ThemeColorScheme : uint { Light = 0, Dark = 1 };
		enum class SystemCursorClass : uint { Null = 0, Arrow = 1, Beam = 2, Link = 3, SizeLeftRight = 4, SizeUpDown = 5, SizeLeftUpRightDown = 6, SizeLeftDownRightUp = 7, SizeAll = 8 };
		enum class CreateWindowDescType : uintptr { CreateWindowDesc = 0, CreateWindowsWindowDesc = 1, CreateCocoaWindowDesc = 2 };
		enum class ProgressDisplayMode : uint { Hide = 0, Normal = 1, Paused = 2, Error = 3, Indeterminated = 4 };
		enum class CocoaEffectMaterial : uint { Titlebar = 0, Selection = 1, Menu = 2, Popover = 3, Sidebar = 4, HeaderView = 5, Sheet = 6, WindowBackground = 7, HUD = 8, FullScreenUI = 9, ToolTip = 10 };
		enum class CloseButtonState : uint { Disabled = 0, Enabled = 1, Alert = 2 };
		enum class AlertDialogStyle : uint { Information = 0, Warning = 1, Error = 2 };
		enum class StatusBarIconColorUsage : uint { Regular = 0, Monochromic = 1 };
		enum class ApplicationCommand : uint { Terminate = 0, CreateFile = 1, OpenSomeFile = 2, OpenSpecificFile = 3, ShowHelp = 4, ShowAbout = 5, ShowProperties = 6 };
		enum class WindowCommand : uint { Save = 0, SaveAs = 1, Export = 2, Print = 3, Undo = 4, Redo = 5, Cut = 6, Copy = 7, Paste = 8, Duplicate = 9, Delete = 10, Find = 11, Replace = 12, SelectAll = 13 };
		enum class IPCStatus : uint { Unknown = 0, Accepted = 1, Discarded = 2, ServerClosed = 3, InternalError = 4 };
		
		enum ClipboardDataFormats : uint {
			ClipboardDataFormatText			= 0x01,
			ClipboardDataFormatImage		= 0x02,
			ClipboardDataFormatFiles		= 0x04,
			ClipboardDataFormatDataFormat	= 0x08,
			ClipboardDataFormatData			= 0x18,
			ClipboardDataFormatDataTypeless	= 0x10,
		};
		enum WindowStyles : uint {
			WindowStyleHasTitle			= 0x00000001,
			WindowStyleResizeble		= 0x00000002,
			WindowStyleCloseButton		= 0x00000004,
			WindowStyleMinimizeButton	= 0x00000008,
			WindowStyleMaximizeButton	= 0x00000010,
			WindowStyleHelpButton		= 0x00000020,
			WindowStyleToolWindow		= 0x00000040,
			WindowStyleModal			= 0x00000080,
			WindowStylePopup			= 0x00000100,
			WindowStyleTopmost			= 0x00000200,
			WindowStyleBottommost		= 0x00000400,
			WindowStyleSetOpacity		= 0x00000800,
			WindowStyleSetColorScheme	= 0x00001000,
			WindowStyleTransparent		= 0x00002000,
			WindowStyleSetBlurBehind	= 0x00004000,
			WindowStyleSetBlurFactor	= 0x00008000,
		};
		enum WindowWindowsStyles : uint {
			WindowWindowsStyleExtendedFrame		= 0x0001,
			WindowWindowsStyleNormalTitle		= 0x0000,
			WindowWindowsStyleTabbedTitle		= 0x0010,
			WindowWindowsStyleTransientTitle	= 0x0020,
			WindowWindowsStyleColoredTitle		= 0x0030,
			WindowWindowsStyleTitleMask			= 0x0030,
		};
		enum WindowCocoaStyles : uint {
			WindowCocoaStyleTransparentTitle	= 0x0001,
			WindowCocoaStyleEffectBackground	= 0x0002,
			WindowCocoaStyleGlassBackground		= 0x0004,
			WindowCocoaStyleShadowless			= 0x0008,
			WindowCocoaStyleContentUnderTitle	= 0x0010,
			WindowCocoaStyleCustomBackground	= 0x0020,
		};
		enum MouseButtonStates : uint {
			MouseLeftButtonIsDown	= 1,
			MouseRightButtonIsDown	= 2,
		};
		
		struct ClipboardDataDesc
		{
			uintptr format_mask;
			string text, data_format;
			oref<array<string>> files;
			oref<Picturae::Picture> image;
			oref<DataBlock> data;
		};
		struct CreateWindowDescBase
		{
			CreateWindowDescType desc_type;
			const void * next_desc;
		};
		struct CreateWindowDesc
		{
			CreateWindowDescType desc_type = CreateWindowDescType::CreateWindowDesc;
			const void * next_desc = 0;
			uint style;
			ThemeColorScheme color_scheme;
			string title;
			IWindowCallback * callback;
			Rectangle position;
			Index2 minimal_constraints, maximal_constraints;
			double opacity, blur_behind_factor;
			oref<IWindow> parent_window;
			oref<IScreen> screen;
		};
		struct CreateWindowsWindowDesc
		{
			CreateWindowDescType desc_type = CreateWindowDescType::CreateWindowsWindowDesc;
			const void * next_desc = 0;
			uint extended_style;
			Color title_color;
			Rectangle margins;
		};
		struct CreateCocoaWindowDesc
		{
			CreateWindowDescType desc_type = CreateWindowDescType::CreateCocoaWindowDesc;
			const void * next_desc = 0;
			uint extended_style;
			Color background_color;
		};
		struct FileFormatDesc
		{
			string description;
			array<string> extensions = array<string>(0x10);
		};
		struct OpenFileDialogDesc
		{
			array<FileFormatDesc> formats = array<FileFormatDesc>(0x10);
			array<string> files = array<string>(0x10);
			string title;
			int default_format_index = -1;
			bool allow_multiple_choices = false;
		};
		struct SaveFileDialogDesc
		{
			array<FileFormatDesc> formats = array<FileFormatDesc>(0x10);
			string title, file;
			int format_index = -1;
			bool append_extension = true;
		};
		struct ChooseDirectoryDialogDesc
		{
			string title, directory;
		};
		struct AlertDialogDesc
		{
			string title, text;
			string buttons[4];
			uint button_index = 0;
			AlertDialogStyle style = AlertDialogStyle::Information;
		};

		class IWindowExtensionClass : public Object
		{
		public:
			virtual bool ExtensionAttached(IWindow * window, Object * extension) noexcept = 0;
			virtual void ExtensionDetached(IWindow * window, Object * extension) noexcept = 0;
		};
		class IMenuItemCallback
		{
		public:
			virtual Index2 MeasureMenuItem(IMenuItem * item, Graphica::IDeviceContext2D * device) noexcept = 0;
			virtual void RenderMenuItem(IMenuItem * item, Graphica::IDeviceContext2D * device, const Rectangle & at, bool hot_state) noexcept = 0;
			virtual void MenuClosed(IMenuItem * item) noexcept = 0;
			virtual void MenuItemDisposed(IMenuItem * item) noexcept;
		};
		class IStatusCallback
		{
		public:
			virtual void HandleStatusIconCommand(IStatusBarIcon * icon, int id) noexcept;
		};
		class IWindowCallback
		{
		public:
			// Fundamental callback
			virtual void Created(IWindow * window) noexcept;
			virtual void Destroyed(IWindow * window) noexcept;
			virtual void Shown(IWindow * window, bool show) noexcept;
			// Presentation callback
			virtual void RenderWindow(IWindow * window) noexcept;
			// Frame event callback
			virtual void WindowClosed(IWindow * window) noexcept;
			virtual void WindowMaximized(IWindow * window) noexcept;
			virtual void WindowMinimized(IWindow * window) noexcept;
			virtual void WindowRestored(IWindow * window) noexcept;
			virtual void WindowHelpRequired(IWindow * window) noexcept;
			virtual void WindowActivated(IWindow * window) noexcept;
			virtual void WindowDeactivated(IWindow * window) noexcept;
			// Position callback
			virtual void WindowMoved(IWindow * window) noexcept;
			virtual void WindowResized(IWindow * window) noexcept;
			// Keyboard events
			virtual void FocusChanged(IWindow * window, bool got) noexcept;
			virtual bool KeyIsDown(IWindow * window, uint vkc, uint vkm) noexcept;
			virtual void KeyIsUp(IWindow * window, uint vkc, uint vkm) noexcept;
			virtual void CharacterIsDown(IWindow * window, unichar32 ucs) noexcept;
			// Mouse events
			virtual void MouseEntered(IWindow * window, uint button_state) noexcept;
			virtual void MouseLeft(IWindow * window, uint button_state) noexcept;
			virtual void MouseMoved(IWindow * window, const Index2 & at, uint button_state) noexcept;
			virtual void LeftButtonIsDown(IWindow * window, const Index2 & at, bool double_click) noexcept;
			virtual void LeftButtonIsUp(IWindow * window, const Index2 & at) noexcept;
			virtual void RightButtonIsDown(IWindow * window, const Index2 & at, bool double_click) noexcept;
			virtual void RightButtonIsUp(IWindow * window, const Index2 & at) noexcept;
			// Scroll events
			virtual void ScrollVertically(IWindow * window, const Index2 & at, double delta) noexcept;
			virtual void ScrollHorizontally(IWindow * window, const Index2 & at, double delta) noexcept;
			// Miscellaneous event
			virtual void Timer(IWindow * window, int timer_id) noexcept;
			virtual void ThemeChanged(IWindow * window) noexcept;
			virtual bool IsWindowCommandEnabled(IWindow * window, WindowCommand command) noexcept;
			virtual void HandleWindowCommand(IWindow * window, WindowCommand command) noexcept;
		};
		class IApplicationCallback
		{
		public:
			virtual bool AcceptsApplicationCommand(ApplicationCommand command) noexcept;
			virtual bool AcceptsWindowCommand(WindowCommand command) noexcept;
			virtual bool HandleApplicationCommand(ApplicationCommand command, const string & argument) noexcept;
			virtual void HandleHotKeyEvent(uint event_id) noexcept;
			virtual bool IPCReceiveData(handle client, const string & verb, const void * data, uintptr length) noexcept;
			virtual oref<DataBlock> IPCSendData(handle client, const string & verb) noexcept;
			virtual void IPCClientDisconnect(handle client) noexcept;
		};

		class IKeyboardManager
		{
		public:
			virtual bool IsKeyPressed(uint vkc) noexcept = 0;
			virtual bool IsKeyToggled(uint vkc) noexcept = 0;
			virtual bool RegisterHotKey(int event_id, uint vkc, uint vkm) noexcept = 0;
			virtual void UnregisterHotKey(int event_id) noexcept = 0;
			virtual uint GetKeyboardDelay(void) noexcept = 0;
			virtual uint GetKeyboardSpeed(void) noexcept = 0;
		};
		class IClipboardManager
		{
		public:
			virtual uint ProbeClipboardFormats(uint format_mask) noexcept = 0;
			virtual bool ReadClipboard(uint format_mask, ClipboardDataDesc & dest) noexcept = 0;
			virtual bool WriteClipboard(const ClipboardDataDesc & dest) noexcept = 0;
		};
		class ITheme : public Object
		{
		public:
			virtual ThemeColorScheme GetColorScheme(void) noexcept = 0;
			virtual Color GetColor(ThemeColor color) noexcept = 0;
		};
		class ICursor : public Object
		{
		public:
			virtual handle GetOSHandle(void) noexcept = 0;
		};
		class IScreen : public DynamicObject
		{
		public:
			virtual string GetName(void) noexcept = 0;
			virtual Rectangle GetScreenRectangle(void) noexcept = 0;
			virtual Rectangle GetUserRectangle(void) noexcept = 0;
			virtual Index2 GetResolution(void) noexcept = 0;
			virtual double GetScaleFactor(void) noexcept = 0;
			virtual oref<Picturae::Picture> Capture(void) noexcept = 0;
		};
		class IWindow : public DynamicObject
		{
		public:
			// Hierarchy control
			virtual IWindow * GetParentWindow(void) noexcept = 0;
			virtual IWindow * GetChildWindow(uintptr index) noexcept = 0;
			virtual uintptr GetChildrenCount(void) noexcept = 0;
			virtual void Destroy(void) noexcept = 0;
			// Frame properties control
			virtual uint GetEffectiveStyle(CreateWindowDescType domain) noexcept = 0;
			virtual bool GetVisibility(void) noexcept = 0;
			virtual void SetVisibility(const bool & show) noexcept = 0;
			virtual string GetTitle(void) noexcept = 0;
			virtual void SetTitle(const string & text) noexcept = 0;
			virtual Rectangle GetPosition(void) noexcept = 0;
			virtual void SetPosition(const Rectangle & rect) noexcept = 0;
			virtual Index2 GetClientSize(void) noexcept = 0;
			virtual Index2 GetMinimalConstraints(void) noexcept = 0;
			virtual void SetMinimalConstraints(const Index2 & size) noexcept = 0;
			virtual Index2 GetMaximalConstraints(void) noexcept = 0;
			virtual void SetMaximalConstraints(const Index2 & size) noexcept = 0;
			virtual double GetOpacity(void) noexcept = 0;
			virtual void SetOpacity(const double & opacity) noexcept = 0;
			virtual CloseButtonState GetCloseButtonState(void) noexcept = 0;
			virtual void SetCloseButtonState(const CloseButtonState & state) noexcept = 0;
			virtual bool IsActive(void) noexcept = 0;
			virtual bool IsMaximized(void) noexcept = 0;
			virtual bool IsMinimized(void) noexcept = 0;
			virtual void Activate(void) noexcept = 0;
			virtual void Maximize(void) noexcept = 0;
			virtual void Minimize(void) noexcept = 0;
			virtual void Restore(void) noexcept = 0;
			virtual void RequireAttention(void) noexcept = 0;
			// Frame system-dependent properties control
			virtual void SetProgressMode(const ProgressDisplayMode & mode) noexcept = 0;
			virtual void SetProgressValue(const double & value) noexcept = 0;
			virtual void SetCocoaEffectMaterial(const CocoaEffectMaterial & material) noexcept = 0;
			// Event handling
			virtual IWindowCallback * GetCallback(void) noexcept = 0;
			virtual void SetCallback(IWindowCallback * const & callback) noexcept = 0;
			virtual void Invalidate(void) noexcept = 0;
			virtual bool PerformHitTest(const Index2 & at) noexcept = 0;
			virtual Index2 ConvertClientToGlobal(const Index2 & at) noexcept = 0;
			virtual Index2 ConvertGlobalToClient(const Index2 & at) noexcept = 0;
			virtual ICursor * GetCursor(void) noexcept = 0;
			virtual void SetCursor(ICursor * const & cursor) noexcept = 0;
			virtual bool IsFocused(void) noexcept = 0;
			virtual void SetFocus(void) noexcept = 0;
			virtual void SetTimer(uint32 id, uint32 period) noexcept = 0;
			virtual bool AddExtension(Object * ext, IWindowExtensionClass * extcls) noexcept = 0;
			virtual bool RemoveExtension(IWindowExtensionClass * extcls) noexcept = 0;
			virtual Object * GetExtension(IWindowExtensionClass * extcls) noexcept = 0;
			// Getting visual appearance
			virtual double GetScaleFactor(void) noexcept = 0;
			virtual oref<IScreen> GetScreen(void) noexcept = 0;
			virtual oref<ITheme> GetTheme(void) noexcept = 0;
		};
		class IMenuItem : public Object
		{
		public:
			virtual IMenuItemCallback * GetCallback(void) noexcept = 0;
			virtual void SetCallback(IMenuItemCallback * const & callback) noexcept = 0;
			virtual void * GetUserData(void) noexcept = 0;
			virtual void SetUserData(void * const & data) noexcept = 0;
			virtual IMenu * GetSubmenu(void) noexcept = 0;
			virtual void SetSubmenu(IMenu * const & menu) noexcept = 0;
			virtual int GetID(void) noexcept = 0;
			virtual void SetID(const int & id) noexcept = 0;
			virtual string GetText(void) noexcept = 0;
			virtual void SetText(const string & text) noexcept = 0;
			virtual string GetSideText(void) noexcept = 0;
			virtual void SetSideText(const string & text) noexcept = 0;
			virtual bool IsSeparator(void) noexcept = 0;
			virtual void SetIsSeparator(const bool & separator) noexcept = 0;
			virtual bool IsEnabled(void) noexcept = 0;
			virtual void Enable(const bool & enable) noexcept = 0;
			virtual bool IsChecked(void) noexcept = 0;
			virtual void Check(const bool & check) noexcept = 0;
		};
		class IMenu : public Object
		{
		public:
			virtual void AppendMenuItem(IMenuItem * item) noexcept = 0;
			virtual void InsertMenuItem(IMenuItem * item, int at) noexcept = 0;
			virtual void RemoveMenuItem(int at) noexcept = 0;
			virtual IMenuItem * ElementAt(int at) noexcept = 0;
			virtual int Length(void) noexcept = 0;
			virtual IMenuItem * FindMenuItem(int id) noexcept = 0;
			virtual int Perform(IWindow * owner, const Index2 & at) noexcept = 0;
			virtual handle GetOSHandle(void) noexcept = 0;
		};
		class IStatusBarIcon : public Object
		{
		public:
			virtual IStatusCallback * GetCallback(void) noexcept = 0;
			virtual void SetCallback(IStatusCallback * const & callback) noexcept = 0;
			virtual Index2 GetIconSize(void) noexcept = 0;
			virtual Picturae::Image * GetIcon(void) noexcept = 0;
			virtual void SetIcon(Picturae::Image * const & image) noexcept = 0;
			virtual StatusBarIconColorUsage GetIconColorUsage(void) noexcept = 0;
			virtual void SetIconColorUsage(const StatusBarIconColorUsage & color_usage) noexcept = 0;
			virtual string GetTooltip(void) noexcept = 0;
			virtual void SetTooltip(const string & text) noexcept = 0;
			virtual int GetEventID(void) noexcept = 0;
			virtual void SetEventID(const int & id) noexcept = 0;
			virtual IMenu * GetMenu(void) noexcept = 0;
			virtual void SetMenu(IMenu * const & menu) noexcept = 0;
			virtual bool PresentIcon(bool present) noexcept = 0;
			virtual bool IsVisible(void) noexcept = 0;
		};
		class IIPCClient : public Object
		{
		public:
			virtual bool SendData(const string & verb, const void * data, uintptr length, IDispatchTask * on_responce, IPCStatus * result) noexcept = 0;
			virtual bool RequireData(const string & verb, IDispatchTask * on_responce, IPCStatus * result, oref<DataBlock> * data) noexcept = 0;
			virtual IPCStatus GetStatus(void) noexcept = 0;
		};
		class IWindowSystem : public IDispatchQueue
		{
		public:
			// Dynamic cast extension
			virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept = 0;
			// Screen and theme enumeration
			virtual oref<object_array<IScreen>> EnumerateScreens(void) noexcept = 0;
			virtual oref<IScreen> GetDefaultScreen(void) noexcept = 0;
			virtual oref<ITheme> GetSystemTheme(void) noexcept = 0;
			// Getting the managers
			virtual IKeyboardManager * GetKeyboardManager(void) noexcept = 0;
			virtual IClipboardManager * GetClipboardManager(void) noexcept = 0;
			// Cursor manipulation
			virtual Index2 GetCursorPosition(void) noexcept = 0;
			virtual void SetCursorPosition(const Index2 & position) noexcept = 0;
			virtual oref<ICursor> LoadCursor(Picturae::Picture * source) noexcept = 0;
			virtual oref<ICursor> GetSystemCursor(SystemCursorClass cursor) noexcept = 0;
			// Miscellaneous
			virtual oref<array<Index2>> GetApplicationIconSizes(void) noexcept = 0;
			virtual void SetApplicationIcon(Picturae::Image * icon) noexcept = 0;
			virtual void SetApplicationBadge(const string & text) noexcept = 0;
			virtual void SetApplicationIconVisibility(bool visible) noexcept = 0;
			virtual void Beep(void) noexcept = 0;
			// Creating windows
			virtual oref<array<IWindow *>> EnumerateTopLevelWindows(void) noexcept = 0;
			virtual IWindow * CreateWindow(const void * desc) noexcept = 0;
			virtual Rectangle ConvertClientToWindow(const Rectangle & rect, uint style, uint wstyle = 0, uint cstyle = 0) noexcept = 0;
			virtual Index2 ConvertClientToWindow(const Index2 & size, uint style, uint wstyle = 0, uint cstyle = 0) noexcept = 0;
			// Event handling
			virtual void ScheduleFilesToBeOpened(const string * files, int num_files) noexcept = 0;
			virtual IApplicationCallback * GetCallback(void) noexcept = 0;
			virtual void SetCallback(IApplicationCallback * callback) noexcept = 0;
			// Application loop control
			virtual void RunMainLoop(bool while_there_are_windows = false) noexcept = 0;
			virtual void ExitMainLoop(void) noexcept = 0;
			// Standard system dialogs
			virtual bool OpenFileDialog(OpenFileDialogDesc * desc, IWindow * parent, IDispatchTask * on_responce) noexcept = 0;
			virtual bool SaveFileDialog(SaveFileDialogDesc * desc, IWindow * parent, IDispatchTask * on_responce) noexcept = 0;
			virtual bool ChooseDirectoryDialog(ChooseDirectoryDialogDesc * desc, IWindow * parent, IDispatchTask * on_responce) noexcept = 0;
			virtual bool AlertDialog(AlertDialogDesc * desc, IWindow * parent, IDispatchTask * on_responce) noexcept = 0;
			// Menu control
			virtual oref<IMenu> CreateMenu(void) noexcept = 0;
			virtual oref<IMenuItem> CreateMenuItem(void) noexcept = 0;
			// Notification control
			virtual Index2 GetUserNotificationIconSize(void) noexcept = 0;
			virtual void PushUserNotification(const string & title, const string & text, Picturae::Image * icon = 0) noexcept = 0;
			// Status icon control
			virtual oref<IStatusBarIcon> CreateStatusBarIcon(void) noexcept = 0;
			// Interprocess communication / Dynamic data exchange
			virtual bool LaunchIPCServer(const string & app_id, const string & auth_id) noexcept = 0;
			virtual oref<IIPCClient> CreateIPCClient(const string & server_app_id, const string & server_auth_id) noexcept = 0;
		};
	}
}