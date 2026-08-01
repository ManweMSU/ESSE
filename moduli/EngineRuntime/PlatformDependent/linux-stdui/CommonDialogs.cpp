#include "CommonDialogs.h"
#include "CommonDialogsUI.h"
#include "../SystemGraphicsEx.h"
#include "../../Interfaces/Assembly.h"
#include "../../Interfaces/KeyCodes.h"
#include "../../Interfaces/Shell.h"
#include "../../UserInterface/StaticControls.h"
#include "../../UserInterface/ListControls.h"
#include "../../UserInterface/CombinedControls.h"
#include "../../UserInterface/EditControls.h"
#include "../../UserInterface/BinaryLoader.h"
#include <Graphica-Linux/DeviceCairo.h>

#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <dlfcn.h>
#include <string.h>

namespace Engine
{
	namespace Linux
	{
		ESSE::oref<ESSE::Dictionary<ESSE::string, ESSE::string>> _environment;
		SafePointer<UI::InterfaceTemplate> _common_ui;
		string GetEnvironmentVariable(const char * var)
		{
			if (_environment) {
				auto v = _environment->GetElementByKey(var);
				if (v) return v->GetData(); else return U"";
			} else return U"";
		}
		UI::InterfaceTemplate * GetCommonTemplate(void) noexcept { return _common_ui; }
		void SetCommonTemplate(UI::InterfaceTemplate * common) noexcept { _common_ui.SetRetain(common); }
		void LoadCommonTemplate(void) noexcept
		{
			try {
				if (!_environment) _environment = ESSE::GetEnvironment();
				const void * pdata; int length;
				GetCommonUIData(&pdata, &length);
				Streaming::MemoryStream stream(pdata, length);
				SafePointer<UI::InterfaceTemplate> ui = new UI::InterfaceTemplate;
				UI::Loader::LoadUserInterfaceFromBinary(*ui, &stream);
				SetCommonTemplate(ui);
			} catch (...) {}
		}

		class LibRsvg : public Object
		{
			handle _librsvg;
		public:
			typedef handle (* rsvg_handle_new_from_file_f) (const char * filename, handle * error);
			typedef void (* g_object_unref_f) (handle object);
			typedef int (* rsvg_handle_render_document_f) (handle svg, ESSE::Cairo::cairo_t cairo, const Math::Vector4 & rect, handle * error);
		public:
			LibRsvg(void)
			{
				_librsvg = dlopen("librsvg-2.so", RTLD_NOW | RTLD_LOCAL);
				if (_librsvg) {
					g_object_unref = reinterpret_cast<g_object_unref_f>(dlsym(_librsvg, "g_object_unref"));
					rsvg_handle_new_from_file = reinterpret_cast<rsvg_handle_new_from_file_f>(dlsym(_librsvg, "rsvg_handle_new_from_file"));
					rsvg_handle_render_document = reinterpret_cast<rsvg_handle_render_document_f>(dlsym(_librsvg, "rsvg_handle_render_document"));
				} else {
					g_object_unref = 0;
					rsvg_handle_new_from_file = 0;
					rsvg_handle_render_document = 0;
				}
			}
			virtual ~LibRsvg(void) override { if (_librsvg) dlclose(_librsvg); }
		public:
			rsvg_handle_new_from_file_f rsvg_handle_new_from_file;
			rsvg_handle_render_document_f rsvg_handle_render_document;
			g_object_unref_f g_object_unref;
		};
		struct MessageBoxCallback : public UI::IEventCallback
		{
			SafePointer<UI::InterfaceTemplate> ui;
			Windows::MessageBoxResult * result;
			Windows::MessageBoxButtonSet buttons;
			Windows::MessageBoxStyle style;
			string text, title;
			SafePointer<IDispatchTask> on_exit;
		private:
			void _exit_dialog(Windows::IWindow * window, Windows::MessageBoxResult r) noexcept
			{
				if (result) *result = r;
				auto task = on_exit;
				auto ws = Windows::GetWindowSystem();
				ws->ExitModalSession(window);
				if (task) task->DoTask(ws);
			}
		public:
			virtual void Created(Windows::IWindow * window) override
			{
				UI::GetRootControl(window)->AddDialogStandardAccelerators();
				window->SetText(title);
				UI::FindControl(window, 101)->SetText(U"\ea2" + text + U"\e");
				if (style == Windows::MessageBoxStyle::Information) {
					UI::FindControl(window, 102)->As<UI::Controls::Static>()->SetImage(ui->Texture[U"IconInformation"]);
				} else if (style == Windows::MessageBoxStyle::Warning) {
					UI::FindControl(window, 102)->As<UI::Controls::Static>()->SetImage(ui->Texture[U"IconWarning"]);
				} else if (style == Windows::MessageBoxStyle::Error) {
					UI::FindControl(window, 102)->As<UI::Controls::Static>()->SetImage(ui->Texture[U"IconError"]);
				}
				if (buttons == Windows::MessageBoxButtonSet::Ok) {
					UI::FindControl(window, 203)->Show(false);
					UI::FindControl(window, 202)->Show(false);
					UI::FindControl(window, 201)->SetText(Assembly::GetLocalizedCommonString(101, U"OK"));
					UI::FindControl(window, 201)->SetID(1);
				} else if (buttons == Windows::MessageBoxButtonSet::OkCancel) {
					UI::FindControl(window, 203)->Show(false);
					UI::FindControl(window, 202)->SetText(Assembly::GetLocalizedCommonString(101, U"OK"));
					UI::FindControl(window, 202)->SetID(1);
					UI::FindControl(window, 201)->SetText(Assembly::GetLocalizedCommonString(102, U"Cancel"));
					UI::FindControl(window, 201)->SetID(2);
				} else if (buttons == Windows::MessageBoxButtonSet::YesNo) {
					UI::FindControl(window, 203)->Show(false);
					UI::FindControl(window, 202)->SetText(Assembly::GetLocalizedCommonString(103, U"Yes"));
					UI::FindControl(window, 202)->SetID(3);
					UI::FindControl(window, 201)->SetText(Assembly::GetLocalizedCommonString(104, U"No"));
					UI::FindControl(window, 201)->SetID(4);
				} else if (buttons == Windows::MessageBoxButtonSet::YesNoCancel) {
					UI::FindControl(window, 203)->SetText(Assembly::GetLocalizedCommonString(103, U"Yes"));
					UI::FindControl(window, 203)->SetID(3);
					UI::FindControl(window, 202)->SetText(Assembly::GetLocalizedCommonString(104, U"No"));
					UI::FindControl(window, 202)->SetID(4);
					UI::FindControl(window, 201)->SetText(Assembly::GetLocalizedCommonString(102, U"Cancel"));
					UI::FindControl(window, 201)->SetID(2);
				}
			}
			virtual void Destroyed(Windows::IWindow * window) override { delete this; }
			virtual void WindowClose(Windows::IWindow * window) override { HandleControlEvent(window, 2, UI::ControlEvent::AcceleratorCommand, 0); }
			virtual void HandleControlEvent(Windows::IWindow * window, int ID, UI::ControlEvent event, UI::Control * sender) override
			{
				if (ID == 1 && (buttons == Windows::MessageBoxButtonSet::Ok || buttons == Windows::MessageBoxButtonSet::OkCancel)) {
					_exit_dialog(window, Windows::MessageBoxResult::Ok);
				} else if (ID == 2 && (buttons == Windows::MessageBoxButtonSet::OkCancel || buttons == Windows::MessageBoxButtonSet::YesNoCancel)) {
					_exit_dialog(window, Windows::MessageBoxResult::Cancel);
				} else if (ID == 2 && buttons == Windows::MessageBoxButtonSet::Ok) {
					_exit_dialog(window, Windows::MessageBoxResult::Ok);
				} else if (ID == 3 && (buttons == Windows::MessageBoxButtonSet::YesNo || buttons == Windows::MessageBoxButtonSet::YesNoCancel)) {
					_exit_dialog(window, Windows::MessageBoxResult::Yes);
				} else if (ID == 4 && (buttons == Windows::MessageBoxButtonSet::YesNo || buttons == Windows::MessageBoxButtonSet::YesNoCancel)) {
					_exit_dialog(window, Windows::MessageBoxResult::No);
				}
			}
		};
		struct FileDialogCallback : public UI::IEventCallback, public IFileDialog
		{
			ENGINE_REFLECTED_CLASS(file_entity, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, ImageNormal);
				ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, ImageOverlay);
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text);
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Type);
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Date);
				IO::FileType RefereType;
				string ReferePath;
			ENGINE_END_REFLECTED_CLASS
			struct mime_entry {
				string Description;
				SafePointer<Graphics::IBitmap> Icon;
			};
		public:
			static string home_directory;
			static SafePointer<LibRsvg> rsvg;
			Windows::IWindow * host;
			SafePointer<UI::InterfaceTemplate> ui;
			SafePointer<IFileDialogCallback> callback;
			Array<string> xdg_roots, icon_roots;
			Volumes::Dictionary<string, string> ext_mime;
			Volumes::Dictionary<string, mime_entry> mime_desc;
			SafeArray<file_entity> left_panel, main_panel;
			Array<string> history;
			int current;
			Volumes::Set<string> allowed_formats;
			SafePointer<Windows::IMenu> file_menu;
			UI::Control * name_editor;
			string name_editor_rename_from;
			bool allow_all, show_hidden;
		private:
			bool _xml_extract(const string & xml, const string & open, const string & close, string & out)
			{
				int f = xml.FindFirst(open);
				if (f >= 0) {
					auto subt = xml.Fragment(f + open.Length(), -1);
					f = subt.FindFirst(close);
					out = subt.Fragment(0, f).Replace(U"&lt;", U"<").Replace(U"&gt;", U">").Replace(U"&amp;", U"&");
					return true;
				} else return false;
			}
			void _index_icon_theme(Volumes::Dictionary<uint, string> & dest, const string & path, const string & usage, const string & type, uint size, uint scale)
			{
				int fxsize = size * scale;
				int refsize = 16 * UI::CurrentScaleFactor;
				uint diff = (type == U"Scalable") ? 0 : uint(abs(fxsize - refsize));
				if (usage == U"MimeTypes") dest.Append(diff, path);
			}
			void _index_icon_theme(const string & root)
			{
				try {
					SafePointer<Streaming::FileStream> stream = new Streaming::FileStream(root + U"/index.theme", Streaming::AccessRead, Streaming::OpenExisting);
					SafePointer<Streaming::TextReader> reader = new Streaming::TextReader(stream, Encoding::UTF8);
					Volumes::Dictionary<uint, string> scale_variants;
					string path, usage, type;
					uint pxsize = 0, scale = 1;
					while (!reader->EofReached()) {
						auto line = reader->ReadLine();
						if (line[0] == U'[') {
							if (line == U"[Icon Theme]") path = U""; else {
								if (path.Length()) _index_icon_theme(scale_variants, root + U"/" + path, usage, type, pxsize, scale);
								path = line.Fragment(1, line.Length() - 2);
								usage = U""; type = U""; pxsize = 0; scale = 1;
							}
						} else if (line.Length() && line[0] != U'#' && path.Length()) {
							if (line.Fragment(0, 5) == U"Size=") pxsize = line.Fragment(5, -1).ToUInt32();
							else if (line.Fragment(0, 6) == U"Scale=") scale = line.Fragment(6, -1).ToUInt32();
							else if (line.Fragment(0, 8) == U"Context=") usage = line.Fragment(8, -1);
							else if (line.Fragment(0, 5) == U"Type=") type = line.Fragment(5, -1);
						}
					}
					if (path.Length()) _index_icon_theme(scale_variants, root + U"/" + path, usage, type, pxsize, scale);
					if (!scale_variants.IsEmpty()) icon_roots.Append(scale_variants.GetFirst()->GetValue().value);
				} catch (...) {}
			}
			mime_entry * _load_mime_data(const string & mime)
			{
				auto cached = mime_desc[mime];
				if (cached) return cached;
				try {
					mime_entry ent;
					for (auto & r : xdg_roots) try {
						SafePointer<Streaming::FileStream> stream = new Streaming::FileStream(r + U"/mime/" + mime + U".xml", Streaming::AccessRead, Streaming::OpenExisting);
						SafePointer<Streaming::TextReader> reader = new Streaming::TextReader(stream, Encoding::UTF8);
						auto xml = reader->ReadAll();
						if (Assembly::CurrentLocale.Length()) {
							if (!_xml_extract(xml, FormatString(U"<comment xml:lang=\"%0\">", Assembly::CurrentLocale), U"</comment>", ent.Description)) _xml_extract(xml, U"<comment>", U"</comment>", ent.Description);
						} else _xml_extract(xml, U"<comment>", U"</comment>", ent.Description);
						break;
					} catch (...) {}
					if (!icon_roots.Length()) for (auto & r : xdg_roots) try {
						SafePointer< Array<string> > themes = IO::Search::GetDirectories(r + U"/icons/*");
						for (auto & t : themes->Elements()) _index_icon_theme(r + U"/icons/" + t);
					} catch (...) {}
					for (auto & i : icon_roots) try {
						auto file_base = i + U"/" + mime.Replace(U'/', U'-');
						if (IO::FileExists(file_base + U".png")) {
							SafePointer<Streaming::FileStream> stream = new Streaming::FileStream(file_base + U".png", Streaming::AccessRead, Streaming::OpenExisting);
							SafePointer<Codec::Frame> frame = Codec::DecodeFrame(stream);
							if (!frame) continue;
							auto loader = UI::GetControlSystem(host)->GetRenderingDevice()->GetParentFactory();
							ent.Icon = loader->LoadBitmap(frame);
						} else if (IO::FileExists(file_base + U".svg")) {
							if (!rsvg) rsvg = new LibRsvg;
							if (rsvg->rsvg_handle_new_from_file && rsvg->g_object_unref) {
								SafePointer<DataBlock> path_utf8 = (file_base + U".svg").EncodeSequence(Encoding::UTF8, true);
								auto svg = rsvg->rsvg_handle_new_from_file(reinterpret_cast<char *>(path_utf8->GetBuffer()), 0);
								if (svg) {
									auto loader = UI::GetControlSystem(host)->GetRenderingDevice()->GetParentFactory();
									uint size = 16 * UI::CurrentScaleFactor;
									ent.Icon = loader->CreateBitmap(size, size, 0);
									SafePointer<Graphics::IBitmapContext> context = loader->CreateBitmapContext();
									if (ent.Icon && context && rsvg->rsvg_handle_render_document) {
										if (context->BeginRendering(ent.Icon)) {
											auto cairo_context = static_cast<ESSE::Cairo::CairoDevice *>(ESSEIO::UnwrapContext(context));
											rsvg->rsvg_handle_render_document(svg, cairo_context->GetCairo(), Math::Vector4(0.0, 0.0, size, size), 0);
											context->EndRendering();
										}
									}
									rsvg->g_object_unref(svg);
								}
							}
						}
					} catch (...) {}
					mime_desc.Append(mime, ent);
				} catch (...) {}
				return mime_desc[mime];
			}
			void _init_left_panel(void)
			{
				auto panel = UI::FindControl(host, 201)->As<UI::Controls::TreeView>();
				auto user_home = GetEnvironmentVariable("HOME");
				auto icon_folder = ui->Texture[U"IconFileDirectory"];
				auto icon_volume = ui->Texture[U"IconFileVolume"];
				auto root = panel->GetRootItem();
				SafePointer< Array<string> > home_subdirs = IO::Search::GetDirectories(user_home + U"/*");
				SafePointer< Array<IO::Search::Volume> > volumes = IO::Search::GetVolumes();
				file_entity ent;
				ent.ImageNormal.SetRetain(icon_folder);
				ent.Text = *ui->Strings[U"ObjectNameHome"];
				ent.RefereType = IO::FileType::Directory;
				ent.ReferePath = user_home;
				left_panel.Append(ent);
				auto home_item = root->AddItem(left_panel.LastElement(), &left_panel.LastElement());
				home_item->Expand(true);
				for (auto & d : home_subdirs->Elements()) {
					if (d[0] == U'.') continue;
					ent.Text = d;
					ent.ReferePath = user_home + U"/" + d;
					left_panel.Append(ent);
					home_item->AddItem(left_panel.LastElement(), &left_panel.LastElement());
				}
				ent.ImageNormal.SetRetain(icon_volume);
				ent.Text = *ui->Strings[U"ObjectNameRoot"];
				ent.ReferePath = U"/";
				left_panel.Append(ent);
				auto root_item = root->AddItem(left_panel.LastElement(), &left_panel.LastElement());
				root_item->Expand(true);
				for (auto & v : volumes->Elements()) {
					ent.Text = v.Label;
					ent.ReferePath = v.Path;
					left_panel.Append(ent);
					root_item->AddItem(left_panel.LastElement(), &left_panel.LastElement());
				}
			}
			UI::Controls::TreeView::TreeViewItem * _tree_find_item(UI::Controls::TreeView::TreeViewItem * root, const string & path)
			{
				auto user = reinterpret_cast<file_entity *>(root->User);
				if (user && user->ReferePath == path) return root;
				auto count = root->GetChildrenCount();
				for (int i = 0; i < count; i++) {
					auto nested = root->GetChild(i);
					auto nested_result = _tree_find_item(nested, path);
					if (nested_result) return nested_result;
				}
				return 0;
			}
			UI::Controls::TreeView::TreeViewItem * _tree_find_item(UI::Controls::TreeView * view, const string & path) { return _tree_find_item(view->GetRootItem(), path); }
			void _set_history_index(int index)
			{
				_end_rename(false);
				current = min(max(index, 0), history.Length() - 1);
				UI::FindControl(host, 101)->Enable(current > 0);
				UI::FindControl(host, 102)->Enable(current < history.Length() - 1);
				UI::FindControl(host, 103)->Enable(history[current].Length() > 1);
				UI::FindControl(host, 104)->SetText(history[current]);
				auto tree = UI::FindControl(host, 201)->As<UI::Controls::TreeView>();
				auto item = _tree_find_item(tree, history[current]);
				tree->SetSelectedItem(item, true);
				callback->WorkingDirectoryWasChanged(*this);
				_refresh();
			}
			void _get_file_types(const string & path, IO::FileType & primary, IO::FileType & effective, string & date, bool & exec)
			{
				SafePointer<DataBlock> path_utf8 = path.EncodeSequence(Encoding::UTF8, true);
				struct stat sp, se;
				if (lstat(reinterpret_cast<char *>(path_utf8->GetBuffer()), &sp) >= 0) {
					if ((sp.st_mode & S_IFMT) == S_IFDIR) primary = IO::FileType::Directory;
					else if ((sp.st_mode & S_IFMT) == S_IFREG) primary = IO::FileType::Regular;
					else if ((sp.st_mode & S_IFMT) == S_IFLNK) primary = IO::FileType::SymbolicLink;
					else primary = IO::FileType::Unknown;
					date = Time::FromUnixTime(sp.st_mtim.tv_sec * 1000 + sp.st_mtim.tv_nsec / 1000000).ToLocal().ToString();
					exec = (sp.st_mode & 0111) != 0;
				} else {
					primary = IO::FileType::Unknown;
					date = U"-";
					exec = false;
				}
				if (stat(reinterpret_cast<char *>(path_utf8->GetBuffer()), &se) >= 0) {
					if ((se.st_mode & S_IFMT) == S_IFDIR) effective = IO::FileType::Directory;
					else if ((se.st_mode & S_IFMT) == S_IFREG) effective = IO::FileType::Regular;
					else if ((se.st_mode & S_IFMT) == S_IFLNK) effective = IO::FileType::SymbolicLink;
					else effective = IO::FileType::Unknown;
				} else effective = IO::FileType::Unknown;
			}
			bool _check_file_filter(const string & path)
			{
				if (allow_all) return true;
				return allowed_formats[IO::Path::GetExtension(path).LowerCase()];
			}
			void _refresh(void)
			{
				auto list = UI::FindControl(host, 301)->As<UI::Controls::ListView>();
				auto file = UI::FindControl(host, 302)->As<UI::Controls::TextComboBox>();
				list->ClearItems();
				file->ClearItems();
				main_panel.Clear();
				SafePointer< Array<uint8> > path_utf8 = history[current].EncodeSequence(Encoding::UTF8, true);
				struct dirent ** elements;
				int count = scandir(reinterpret_cast<char *>(path_utf8->GetBuffer()), &elements, 0, alphasort);
				if (count >= 0) {
					for (int i = 0; i < count; i++) {
						try {
							bool hidden = elements[i]->d_name[0] == U'.';
							if (show_hidden && hidden) hidden = strcmp(elements[i]->d_name, ".") == 0 || strcmp(elements[i]->d_name, "..") == 0;
							if (!hidden) {
								auto name = string(elements[i]->d_name, -1, Encoding::UTF8);
								file->AddItem(name);
								IO::FileType fxt;
								bool present = false, executable = false;
								file_entity ent;
								ent.Text = name;
								ent.ReferePath = history[current].Length() > 1 ? history[current] + U"/" + name : U"/" + name;
								_get_file_types(ent.ReferePath, ent.RefereType, fxt, ent.Date, executable);
								if (ent.RefereType == IO::FileType::SymbolicLink) {
									ent.RefereType = fxt;
									ent.ImageOverlay.SetRetain(ui->Texture[U"IconFileLink"]);
								}
								if (fxt == IO::FileType::Directory) {
									ent.Type = *ui->Strings[U"ObjectNameDirectory"];
									ent.ImageNormal.SetRetain(ui->Texture[U"IconFileDirectory"]);
									present = true;
								} else if (fxt == IO::FileType::Regular) {
									auto ext = IO::Path::GetExtension(name).LowerCase();
									if (ext.Length()) {
										auto mime = ext_mime[ext];
										mime_entry * mime_desc;
										if (mime && (mime_desc = _load_mime_data(*mime))) {
											if (mime_desc->Description.Length()) ent.Type = mime_desc->Description;
											else ent.Type = FormatString(*ui->Strings[U"ObjectNameFileTyped"], ext.UpperCase());
											if (mime_desc->Icon) ent.ImageNormal.SetRetain(mime_desc->Icon);
											else if (executable) ent.ImageNormal.SetRetain(ui->Texture[U"IconFileRegularExec"]);
											else ent.ImageNormal.SetRetain(ui->Texture[U"IconFileRegular"]);
										} else {
											ent.Type = FormatString(*ui->Strings[U"ObjectNameFileTyped"], ext.UpperCase());
											if (executable) ent.ImageNormal.SetRetain(ui->Texture[U"IconFileRegularExec"]);
											else ent.ImageNormal.SetRetain(ui->Texture[U"IconFileRegular"]);
										}
									} else {
										ent.Type = *ui->Strings[U"ObjectNameFile"];
										if (executable) ent.ImageNormal.SetRetain(ui->Texture[U"IconFileRegularExec"]);
										else ent.ImageNormal.SetRetain(ui->Texture[U"IconFileRegular"]);
									}
									present = _check_file_filter(ent.ReferePath);
								} else {
									ent.Type = *ui->Strings[U"ObjectNameFile"];
									ent.ImageNormal.SetRetain(ui->Texture[U"IconFileUnknown"]);
									present = _check_file_filter(ent.ReferePath);
								}
								if (present) main_panel.Append(ent);
							}
						} catch (...) {}
						free(elements[i]);
					}
					free(elements);
				}
				SortArray(main_panel, [](const file_entity & a, const file_entity & b) -> int {
					if (a.RefereType == IO::FileType::Directory && b.RefereType != IO::FileType::Directory) return -1;
					else if (a.RefereType != IO::FileType::Directory && b.RefereType == IO::FileType::Directory) return 1;
					else return string::CompareIgnoreCase(a.Text, b.Text);
				});
				for (auto & e : main_panel) list->AddItem(e, &e);
				callback->FileSelectionWasChanged(*this);
			}
			void _init_mime_mapping(void)
			{
				auto xdg_data_home = GetEnvironmentVariable("XDG_DATA_HOME");
				if (!xdg_data_home.Length()) {
					auto home = GetEnvironmentVariable("HOME");
					xdg_data_home = home + U"/.local/share";
				}
				auto xdg_data_dirs = GetEnvironmentVariable("XDG_DATA_DIRS");
				if (!xdg_data_dirs.Length()) xdg_data_dirs = U"/usr/local/share:/usr/share";
				xdg_roots = xdg_data_dirs.Split(U':');
				xdg_roots.Insert(xdg_data_home, 0);
				for (auto & r : xdg_roots) try {
					Streaming::FileStream stream(r + U"/mime/globs", Streaming::AccessRead, Streaming::OpenExisting);
					Streaming::TextReader reader(&stream, Encoding::UTF8);
					while (!reader.EofReached()) {
						auto line = reader.ReadLine();
						if (!line.Length() || line[0] == U'#') continue;
						auto del = line.FindFirst(U':');
						if (del < 0) continue;
						auto mime = line.Fragment(0, del);
						auto filter = line.Fragment(del + 1, -1);
						if (filter.Fragment(0, 2) != U"*.") continue;
						auto extension = filter.Fragment(2, -1);
						ext_mime.Append(extension.LowerCase(), mime);
					}
				} catch (...) {}
			}
			bool _end_rename(bool approve)
			{
				if (!name_editor) return false;
				if (approve) {
					auto new_name = name_editor->FindChild(4001)->GetText();
					auto view = UI::FindControl(host, 301)->As<UI::Controls::ListView>();
					view->CloseEmbeddedEditor();
					try { IO::MoveFile(history[current] + U"/" + name_editor_rename_from, history[current] + U"/" + new_name); } catch (...) {}
					name_editor = 0;
					name_editor_rename_from = U"";
					_refresh();
					auto count = view->ItemCount();
					for (int i = 0; i < count; i++) {
						auto user = reinterpret_cast<file_entity *>(view->GetItemUserData(i));
						if (user && user->Text == new_name) { view->SetSelectedIndex(i, true); break; }
					}
					HandleControlEvent(host, 301, UI::ControlEvent::ValueChange, view);
				} else {
					auto view = UI::FindControl(host, 301)->As<UI::Controls::ListView>();
					view->CloseEmbeddedEditor();
					name_editor = 0;
					name_editor_rename_from = U"";
				}
				return true;
			}
		public:
			FileDialogCallback(void) : xdg_roots(0x10), icon_roots(0x10), left_panel(0x100), main_panel(0x100), history(0x100), name_editor(0) {}
			virtual void Created(Windows::IWindow * window) override
			{
				host = window;
				current = -1;
				allow_all = show_hidden = false;
				UI::GetRootControl(window)->AddDialogStandardAccelerators();
				UI::GetRootControl(window)->GetAcceleratorTable() << UI::Accelerators::AcceleratorCommand(2001, KeyCodes::F5, false);
				UI::GetRootControl(window)->GetAcceleratorTable() << UI::Accelerators::AcceleratorCommand(2002, KeyCodes::F2, false);
				UI::GetRootControl(window)->GetAcceleratorTable() << UI::Accelerators::AcceleratorCommand(2003, KeyCodes::A);
				UI::GetRootControl(window)->GetAcceleratorTable() << UI::Accelerators::AcceleratorCommand(2004, KeyCodes::F5);
				_init_mime_mapping();
				_init_left_panel();
				callback->DialogWasCreated(*this);
				if (!home_directory.Length()) home_directory = GetEnvironmentVariable("HOME");
				SetWorkingDirectory(home_directory);
			}
			virtual void Destroyed(Windows::IWindow * window) override { delete this; }
			virtual void WindowClose(Windows::IWindow * window) override { HandleControlEvent(window, 2, UI::ControlEvent::AcceleratorCommand, 0); }
			virtual void HandleControlEvent(Windows::IWindow * window, int ID, UI::ControlEvent event, UI::Control * sender) override
			{
				if (ID == 1) {
					if (_end_rename(true)) return;
					callback->OpenButtonWasPressed(*this);
				} else if (ID == 2) {
					if (_end_rename(false)) return;
					callback->CancelButtonWasPressed(*this);
				} else if (ID == 101) {
					_end_rename(false);
					if (current > 0) {
						UI::FindControl(host, 302)->SetText(U"");
						_set_history_index(current - 1);
					}
				} else if (ID == 102) {
					_end_rename(false);
					if (current < history.Length() - 1) {
						UI::FindControl(host, 302)->SetText(U"");
						_set_history_index(current + 1);
					}
				} else if (ID == 103) {
					_end_rename(false);
					auto parent = IO::Path::GetDirectory(history[current]);
					if (!parent.Length()) parent = U"/";
					UI::FindControl(host, 302)->SetText(U"");
					SetWorkingDirectory(parent);
				} else if (ID == 105) {
					_end_rename(false);
					uint index = 0;
					string path;
					while (true) {
						auto name = index ? *ui->Strings[U"ObjectNameNewDirectory"] + U" \x2013 " + string(index) : *ui->Strings[U"ObjectNameNewDirectory"];
						path = IO::ExpandPath(history[current] + U"/" + name);
						try { IO::CreateDirectory(path); break; }
						catch (IO::DirectoryAlreadyExistsException &) { index++; continue; }
						catch (...) { return; }
					}
					_refresh();
					auto view = UI::FindControl(host, 301)->As<UI::Controls::ListView>();
					auto count = view->ItemCount();
					for (int i = 0; i < count; i++) {
						auto user = reinterpret_cast<file_entity *>(view->GetItemUserData(i));
						if (user && user->ReferePath == path) { view->SetSelectedIndex(i, true); break; }
					}
					HandleControlEvent(window, 301, UI::ControlEvent::ValueChange, view);
					HandleControlEvent(window, 2002, UI::ControlEvent::AcceleratorCommand, 0);
				} else if (ID == 201 && event == UI::ControlEvent::ValueChange) {
					_end_rename(false);
					auto item = UI::FindControl(host, 201)->As<UI::Controls::TreeView>()->GetSelectedItem();
					if (!item) return;
					auto ent = reinterpret_cast<file_entity *>(item->User);
					if (ent) {
						UI::FindControl(host, 302)->SetText(U"");
						SetWorkingDirectory(ent->ReferePath);
					}
				} else if (ID == 301) {
					if (event == UI::ControlEvent::ValueChange) {
						_end_rename(false);
						Array<string> names(0x40);
						auto view = UI::FindControl(host, 301)->As<UI::Controls::ListView>();
						auto count = view->ItemCount();
						for (int i = 0; i < count; i++) if (view->IsItemSelected(i)) {
							names << reinterpret_cast<file_entity *>(view->GetItemUserData(i))->Text;
						}
						SetSelectedFiles(names);
						callback->FileSelectionWasChanged(*this);
					} else if (event == UI::ControlEvent::DoubleClick) {
						_end_rename(false);
						callback->OpenButtonWasPressed(*this);
					} else if (event == UI::ControlEvent::ContextClick) {
						_end_rename(false);
						if (!file_menu) file_menu = UI::CreateMenu(ui->Dialog[U"FileMenu"]);
						if (file_menu) {
							file_menu->FindMenuItem(2004)->Check(show_hidden);
							auto pos = Windows::GetWindowSystem()->GetCursorPosition();
							pos = host->PointGlobalToClient(pos);
							UI::RunMenu(file_menu, UI::GetRootControl(host), pos);
						}
					}
				} else if (ID == 302 && event == UI::ControlEvent::ValueChange) {
					callback->FileSelectionWasChanged(*this);
				} else if (ID == 303 && event == UI::ControlEvent::ValueChange) {
					callback->FilterSelectionWasChanged(*this);
				} else if (ID == 2001) {
					_end_rename(false);
					_refresh();
				} else if (ID == 2002) {
					auto view = UI::FindControl(host, 301)->As<UI::Controls::ListView>();
					auto count = view->ItemCount();
					int index = -1;
					for (int i = 0; i < count; i++) if (view->IsItemSelected(i)) { index = i; break; }
					file_entity * ent = 0;
					if (index >= 0 && (ent = reinterpret_cast<file_entity *>(view->GetItemUserData(index)))) {
						name_editor_rename_from = ent->Text;
						name_editor = view->CreateEmbeddedEditor(ui->Dialog[U"NameEditor"], 3001);
						if (name_editor) {
							auto edit = name_editor->FindChild(4001)->As<UI::Controls::Edit>();
							edit->SetText(name_editor_rename_from);
							edit->SetFocus();
							edit->SetSelection(0, 0xFFFFFF);
						}
					}
				} else if (ID == 2003) {
					_end_rename(false);
					auto view = UI::FindControl(host, 301)->As<UI::Controls::ListView>();
					if (!view->MultiChoose) return;
					auto count = view->ItemCount();
					for (int i = 0; i < count; i++) view->SelectItem(i, true);
					HandleControlEvent(window, 301, UI::ControlEvent::ValueChange, view);
				} else if (ID == 2004) {
					_end_rename(false);
					show_hidden = !show_hidden;
					_refresh();
				} else if (ID == 2005) {
					auto view = UI::FindControl(host, 301)->As<UI::Controls::ListView>();
					auto count = view->ItemCount();
					for (int i = 0; i < count; i++) if (view->IsItemSelected(i)) {
						auto user = reinterpret_cast<file_entity *>(view->GetItemUserData(i));
						if (user) Shell::OpenFile(user->ReferePath);
					}
				} else if (ID == 2006) {
					Shell::ShowInBrowser(history[current], true);
				} else if (ID == 2007) {
					_end_rename(false);
					auto task = CreateStructuredTask<Windows::MessageBoxResult>([this](Windows::MessageBoxResult result) {
						if (result == Windows::MessageBoxResult::Yes) {
							auto view = UI::FindControl(host, 301)->As<UI::Controls::ListView>();
							auto count = view->ItemCount();
							for (int i = 0; i < count; i++) if (view->IsItemSelected(i)) try {
								auto path = reinterpret_cast<file_entity *>(view->GetItemUserData(i))->ReferePath;
								if (IO::GetFileType(path) == IO::FileType::Directory) IO::RemoveEntireDirectory(path);
								else IO::RemoveFile(path);
							} catch (...) {}
							_refresh();
						}
					});
					CommonMessageBox(&task->Value1, *ui->Strings[U"MessageRemovalConfirmation"], host->GetText(), host, Windows::MessageBoxButtonSet::YesNo, Windows::MessageBoxStyle::Warning, task);
				}
			}
			virtual void SetOpenButtonState(const string & text, bool enable) override { auto button = UI::FindControl(host, 1); button->SetText(text); button->Enable(enable); }
			virtual void SetDialogTitle(const string & text) override { host->SetText(text); }
			virtual void SetWorkingDirectory(const string & path) override
			{
				if (current >= 0 && history[current] == path) return;
				UI::GetControlSystem(host)->ReleaseCapture();
				history.SetLength(current + 1);
				history.Append(path);
				_set_history_index(current + 1);
			}
			virtual void AddFilter(const string & desc) override
			{
				auto view = UI::FindControl(host, 303);
				if (view) view->As<UI::Controls::ComboBox>()->AddItem(desc);
			}
			virtual void SetFilterIndex(int index) override
			{
				auto view = UI::FindControl(host, 303);
				if (view) view->As<UI::Controls::ComboBox>()->SetSelectedIndex(index);
			}
			virtual void SetMultipleChoices(bool set) override { UI::FindControl(host, 301)->As<UI::Controls::ListView>()->MultiChoose = set; }
			virtual void SetFileFilter(const Volumes::Set<string> & formats) override { allowed_formats = formats; allow_all = false; if (current >= 0) _refresh(); }
			virtual void SetFileFilter(bool allow_files) override { allowed_formats.Clear(); allow_all = allow_files; if (current >= 0) _refresh(); }
			virtual int GetFilterIndex(void) override
			{
				auto view = UI::FindControl(host, 303);
				if (view) return view->As<UI::Controls::ComboBox>()->GetSelectedIndex(); else return 0;
			}
			virtual Windows::IWindow * GetHostWindow(void) override { return host; }
			virtual UI::InterfaceTemplate & GetHostTemplate(void) override { return *ui; }
			virtual string GetWorkingDirectory(void) override { return history[current]; }
			virtual void GetSelectedFiles(Array<string> & names) override
			{
				auto text = UI::FindControl(host, 302)->GetText();
				if (text.FindFirst(U'\'') >= 0) {
					auto spl = text.Split(U'\'');
					for (int i = 0; i < spl.Length(); i++) if (i & 1) names << spl[i];
				} else if (text.Length()) names << text;
			}
			virtual void SetSelectedFiles(const Array<string> & names) override
			{
				if (names.Length() == 0) SetSelectedFiles();
				else if (names.Length() == 1) SetSelectedFiles(names[0]);
				else {
					DynamicString concat;
					for (int i = 0; i < names.Length(); i++) {
						if (i) concat << U' ';
						concat << U'\'' << names[i] << U'\'';
					}
					UI::FindControl(host, 302)->SetText(concat.ToString());
				}
			}
			virtual void SetSelectedFiles(const string & name) override { UI::FindControl(host, 302)->SetText(name); }
			virtual void SetSelectedFiles(void) override { UI::FindControl(host, 302)->SetText(U""); }
			virtual void EndDialog(void) override { _end_rename(false); home_directory = GetWorkingDirectory(); Windows::GetWindowSystem()->ExitModalSession(host); }
		};
		struct OpenFileCallback : public IFileDialogCallback
		{
			Windows::OpenFileInfo * info;
			SafePointer<IDispatchTask> task;
			int format_bias;
			virtual bool NeedsFullDialog(void) override { return info->Formats.Length() > 0; }
			virtual void DialogWasCreated(IFileDialog & dialog) override
			{
				if (info->Title.Length()) dialog.SetDialogTitle(info->Title);
				else dialog.SetDialogTitle(*dialog.GetHostTemplate().Strings[U"TitleOpenFile"]);
				dialog.SetMultipleChoices(info->MultiChoose);
				if (info->Formats.Length()) {
					if (info->Formats.Length() > 1) {
						format_bias = -1;
						DynamicString desc;
						desc << Assembly::GetLocalizedCommonString(201, U"All supported") << U" (";
						Volumes::Set<string> exts;
						for (auto & f : info->Formats) for (auto & e : f.Extensions) {
							if (exts[e.LowerCase()]) continue;
							if (!exts.IsEmpty()) desc << U";";
							exts.AddElement(e.LowerCase());
							desc << U"*." << e;
						}
						desc << U")";
						dialog.AddFilter(desc.ToString());
					} else format_bias = 0;
					for (auto & f : info->Formats) {
						DynamicString desc;
						desc << f.Description << U" (";
						for (int i = 0; i < f.Extensions.Length(); i++) {
							if (i) desc << U";";
							desc << U"*." << f.Extensions[i];
						}
						desc << U")";
						dialog.AddFilter(desc.ToString());
					}
					dialog.AddFilter(string(Assembly::GetLocalizedCommonString(202, U"All files")) + U" (*.*)");
					int index = min(max(info->DefaultFormat, format_bias), info->Formats.Length()) - format_bias;
					dialog.SetFilterIndex(index);
					FilterSelectionWasChanged(dialog);
				} else {
					format_bias = 0;
					dialog.SetFileFilter(true);
				}
			}
			virtual void WorkingDirectoryWasChanged(IFileDialog & dialog) override {}
			virtual void FilterSelectionWasChanged(IFileDialog & dialog) override
			{
				int index = dialog.GetFilterIndex() + format_bias;
				if (index < 0) {
					Volumes::Set<string> extensions;
					for (auto & f : info->Formats) for (auto & e : f.Extensions) extensions.AddElement(e.LowerCase());
					dialog.SetFileFilter(extensions);
				} else if (index < info->Formats.Length()) {
					Volumes::Set<string> extensions;
					for (auto & e : info->Formats[index].Extensions) extensions.AddElement(e.LowerCase());
					dialog.SetFileFilter(extensions);
				} else dialog.SetFileFilter(true);
			}
			virtual void FileSelectionWasChanged(IFileDialog & dialog) override
			{
				Array<string> files(0x40);
				dialog.GetSelectedFiles(files);
				if (files.Length()) dialog.SetOpenButtonState(*dialog.GetHostTemplate().Strings[U"ButtonOpen"], true);
				else dialog.SetOpenButtonState(*dialog.GetHostTemplate().Strings[U"ButtonOpen"], false);
			}
			virtual void OpenButtonWasPressed(IFileDialog & dialog) override
			{
				Array<string> files(0x40), directories(0x40), regulars(0x40), inexistant(0x40);
				dialog.GetSelectedFiles(files);
				auto w = dialog.GetWorkingDirectory();
				for (auto & f : files) {
					auto fn = files[0][0] == U'/' ? IO::ExpandPath(files[0]) : IO::ExpandPath(w + U"/" + files[0]);
					SafePointer<DataBlock> fn_utf8 = fn.EncodeSequence(Encoding::UTF8, true);
					struct stat s;
					if (stat(reinterpret_cast<char *>(fn_utf8->GetBuffer()), &s) >= 0) {
						if ((s.st_mode & S_IFMT) == S_IFDIR) directories << fn;
						else regulars << fn;
					} else inexistant << fn;
				}
				if (inexistant.Length()) {
					DynamicString message;
					string * base = dialog.GetHostTemplate().Strings[U"MessageInexistantFile"];
					for (auto & fn : inexistant) {
						if (message.Length()) {
							message << U'\n';
							base = dialog.GetHostTemplate().Strings[U"MessageInexistantFiles"];
						}
						message << fn;
					}
					CommonMessageBox(0, FormatString(*base, message.ToString()), dialog.GetHostWindow()->GetText(), dialog.GetHostWindow(),
						Windows::MessageBoxButtonSet::Ok, Windows::MessageBoxStyle::Warning, 0);
					return;
				}
				if (regulars.Length()) {
					info->Files.Clear();
					info->Files.Append(regulars);
					auto exec = task;
					dialog.EndDialog();
					if (exec) exec->DoTask(Windows::GetWindowSystem());
					return;
				}
				if (directories.Length()) {
					dialog.SetSelectedFiles();
					dialog.SetWorkingDirectory(directories[0]);
				}
			}
			virtual void CancelButtonWasPressed(IFileDialog & dialog) override { auto exec = task; info->Files.Clear(); dialog.EndDialog(); if (exec) exec->DoTask(Windows::GetWindowSystem()); }
		};
		struct SaveFileCallback : public IFileDialogCallback
		{
			Windows::SaveFileInfo * info;
			SafePointer<IDispatchTask> task;
			virtual bool NeedsFullDialog(void) override { return info->Formats.Length() > 0; }
			virtual void DialogWasCreated(IFileDialog & dialog) override
			{
				if (info->Title.Length()) dialog.SetDialogTitle(info->Title);
				else dialog.SetDialogTitle(*dialog.GetHostTemplate().Strings[U"TitleSaveFile"]);
				if (info->Formats.Length()) {
					for (auto & f : info->Formats) {
						DynamicString desc;
						desc << f.Description << U" (";
						for (int i = 0; i < f.Extensions.Length(); i++) {
							if (i) desc << U";";
							desc << U"*." << f.Extensions[i];
						}
						desc << U")";
						dialog.AddFilter(desc.ToString());
					}
					int index = min(max(info->Format, 0), info->Formats.Length() - 1);
					dialog.SetFilterIndex(index);
					FilterSelectionWasChanged(dialog);
				} else dialog.SetFileFilter(true);
			}
			virtual void WorkingDirectoryWasChanged(IFileDialog & dialog) override {}
			virtual void FilterSelectionWasChanged(IFileDialog & dialog) override
			{
				int index = dialog.GetFilterIndex();
				Volumes::Set<string> extensions;
				for (auto & e : info->Formats[index].Extensions) extensions.AddElement(e.LowerCase());
				dialog.SetFileFilter(extensions);
			}
			virtual void FileSelectionWasChanged(IFileDialog & dialog) override
			{
				Array<string> files(0x40);
				dialog.GetSelectedFiles(files);
				if (files.Length()) dialog.SetOpenButtonState(*dialog.GetHostTemplate().Strings[U"ButtonSave"], true);
				else dialog.SetOpenButtonState(*dialog.GetHostTemplate().Strings[U"ButtonSave"], false);
			}
			virtual void OpenButtonWasPressed(IFileDialog & dialog) override
			{
				Array<string> files(0x40);
				dialog.GetSelectedFiles(files);
				if (!files.Length()) return;
				auto w = dialog.GetWorkingDirectory();
				auto fn = files[0][0] == U'/' ? IO::ExpandPath(files[0]) : IO::ExpandPath(w + U"/" + files[0]);
				SafePointer<DataBlock> fn_utf8 = fn.EncodeSequence(Encoding::UTF8, true);
				struct stat s;
				if (stat(reinterpret_cast<char *>(fn_utf8->GetBuffer()), &s) >= 0 && (s.st_mode & S_IFMT) == S_IFDIR) {
					dialog.SetSelectedFiles();
					dialog.SetWorkingDirectory(fn);
					return;
				}
				if (info->AppendExtension && info->Formats.Length()) {
					auto fxext = IO::Path::GetExtension(fn);
					auto index = dialog.GetFilterIndex();
					bool present = false;
					for (auto & ext : info->Formats[index].Extensions) if (string::CompareIgnoreCase(fxext, ext) == 0) { present = true; break; }
					if (!present) fn += U"." + info->Formats[index].Extensions[0];
				}
				fn_utf8 = fn.EncodeSequence(Encoding::UTF8, true);
				if (stat(reinterpret_cast<char *>(fn_utf8->GetBuffer()), &s) >= 0) {
					if ((s.st_mode & S_IFMT) == S_IFDIR) {
						dialog.SetSelectedFiles();
						dialog.SetWorkingDirectory(fn);
					} else {
						auto message = FormatString(*dialog.GetHostTemplate().Strings[U"MessagePreexistantFile"], fn);
						auto message_task = CreateStructuredTask<Windows::MessageBoxResult>([this, d = &dialog, fn, i = dialog.GetFilterIndex()](Windows::MessageBoxResult result) {
							if (result != Windows::MessageBoxResult::Yes) return;
							info->File = fn;
							info->Format = i;
							auto exec = task;
							d->EndDialog();
							exec->DoTask(Windows::GetWindowSystem());
						});
						CommonMessageBox(&message_task->Value1, message, dialog.GetHostWindow()->GetText(), dialog.GetHostWindow(),
							Windows::MessageBoxButtonSet::YesNo, Windows::MessageBoxStyle::Warning, message_task);
					}
				} else {
					info->File = fn;
					info->Format = dialog.GetFilterIndex();
					auto exec = task;
					dialog.EndDialog();
					if (exec) exec->DoTask(Windows::GetWindowSystem());
				}
			}
			virtual void CancelButtonWasPressed(IFileDialog & dialog) override { auto exec = task; info->File = U""; dialog.EndDialog(); if (exec) exec->DoTask(Windows::GetWindowSystem()); }
		};
		struct ChooseDirectoryCallback : public IFileDialogCallback
		{
			Windows::ChooseDirectoryInfo * info;
			SafePointer<IDispatchTask> task;
			virtual bool NeedsFullDialog(void) override { return false; }
			virtual void DialogWasCreated(IFileDialog & dialog) override
			{
				if (info->Title.Length()) dialog.SetDialogTitle(info->Title);
				else dialog.SetDialogTitle(*dialog.GetHostTemplate().Strings[U"TitleChooseDirectory"]);
				dialog.SetFileFilter(false);
			}
			virtual void WorkingDirectoryWasChanged(IFileDialog & dialog) override {}
			virtual void FilterSelectionWasChanged(IFileDialog & dialog) override {}
			virtual void FileSelectionWasChanged(IFileDialog & dialog) override
			{
				Array<string> files(0x40);
				dialog.GetSelectedFiles(files);
				if (files.Length()) dialog.SetOpenButtonState(*dialog.GetHostTemplate().Strings[U"ButtonOpen"], true);
				else dialog.SetOpenButtonState(*dialog.GetHostTemplate().Strings[U"ButtonSelect"], true);
			}
			virtual void OpenButtonWasPressed(IFileDialog & dialog) override
			{
				Array<string> files(0x40);
				dialog.GetSelectedFiles(files);
				if (files.Length()) {
					auto w = dialog.GetWorkingDirectory();
					auto fn = files[0][0] == U'/' ? IO::ExpandPath(files[0]) : IO::ExpandPath(w + U"/" + files[0]);
					SafePointer<DataBlock> fn_utf8 = fn.EncodeSequence(Encoding::UTF8, true);
					struct stat s;
					if (stat(reinterpret_cast<char *>(fn_utf8->GetBuffer()), &s) < 0 || (s.st_mode & S_IFMT) != S_IFDIR) {
						auto message = FormatString(*dialog.GetHostTemplate().Strings[U"MessageInexistantDirectory"], fn);
						CommonMessageBox(0, message, dialog.GetHostWindow()->GetText(), dialog.GetHostWindow(),
							Windows::MessageBoxButtonSet::Ok, Windows::MessageBoxStyle::Warning, 0);
						return;
					}
					dialog.SetSelectedFiles();
					dialog.SetWorkingDirectory(fn);
				} else {
					info->Directory = dialog.GetWorkingDirectory();
					auto exec = task;
					dialog.EndDialog();
					if (exec) exec->DoTask(Windows::GetWindowSystem());
				}
			}
			virtual void CancelButtonWasPressed(IFileDialog & dialog) override { auto exec = task; info->Directory = U""; dialog.EndDialog(); if (exec) exec->DoTask(Windows::GetWindowSystem()); }
		};
		string FileDialogCallback::home_directory;
		SafePointer<LibRsvg> FileDialogCallback::rsvg;

		bool CommonFileDialog(IFileDialogCallback * callback, Windows::IWindow * parent) noexcept
		{
			try {
				auto templates = GetCommonTemplate();
				if (!templates) {
					LoadCommonTemplate();
					templates = GetCommonTemplate();
					if (!templates) return false;
				}
				auto file_callback = new FileDialogCallback;
				file_callback->ui.SetRetain(templates);
				file_callback->callback.SetRetain(callback);
				const widechar * dialog_name = U"";
				if (callback->NeedsFullDialog()) dialog_name = U"FileExplorerFull"; else dialog_name = U"FileExplorer";
				UI::CreateModalWindow(templates->Dialog[dialog_name], file_callback, UI::Rectangle::Entire(), parent);
				return true;
			} catch (...) { return false; }
		}
		bool CommonOpenFileDialog(Windows::OpenFileInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept
		{
			auto callback = new (std::nothrow) OpenFileCallback;
			if (!callback) return false;
			callback->info = info;
			callback->task.SetRetain(on_exit);
			auto status = CommonFileDialog(callback, parent);
			callback->Release();
			return status;
		}
		bool CommonSaveFileDialog(Windows::SaveFileInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept
		{
			auto callback = new (std::nothrow) SaveFileCallback;
			if (!callback) return false;
			callback->info = info;
			callback->task.SetRetain(on_exit);
			auto status = CommonFileDialog(callback, parent);
			callback->Release();
			return status;
		}
		bool CommonDirectoryDialog(Windows::ChooseDirectoryInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept
		{
			auto callback = new (std::nothrow) ChooseDirectoryCallback;
			if (!callback) return false;
			callback->info = info;
			callback->task.SetRetain(on_exit);
			auto status = CommonFileDialog(callback, parent);
			callback->Release();
			return status;
		}
		bool CommonMessageBox(Windows::MessageBoxResult * result, const string & text, const string & title, Windows::IWindow * parent, Windows::MessageBoxButtonSet buttons, Windows::MessageBoxStyle style, IDispatchTask * on_exit) noexcept
		{
			try {
				auto templates = GetCommonTemplate();
				if (!templates) {
					LoadCommonTemplate();
					templates = GetCommonTemplate();
					if (!templates) return false;
				}
				auto callback = new MessageBoxCallback;
				callback->ui.SetRetain(templates);
				callback->result = result;
				callback->buttons = buttons;
				callback->style = style;
				callback->text = text;
				callback->title = title;
				callback->on_exit.SetRetain(on_exit);
				UI::CreateModalWindow(templates->Dialog[U"MessageBox"], callback, UI::Rectangle::Entire(), parent);
				return true;
			} catch (...) { return false; }
		}
	}
}