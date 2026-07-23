#pragma once

#include "../../UserInterface/ControlBase.h"

namespace Engine
{
	namespace Linux
	{
		class IFileDialog
		{
		public:
			virtual void SetOpenButtonState(const string & text, bool enable) = 0;
			virtual void SetDialogTitle(const string & text) = 0;
			virtual void SetWorkingDirectory(const string & path) = 0;
			virtual void AddFilter(const string & desc) = 0;
			virtual void SetFilterIndex(int index) = 0;
			virtual void SetMultipleChoices(bool set) = 0;
			virtual void SetFileFilter(const Volumes::Set<string> & formats) = 0;
			virtual void SetFileFilter(bool allow_files) = 0;
			virtual int GetFilterIndex(void) = 0;
			virtual Windows::IWindow * GetHostWindow(void) = 0;
			virtual UI::InterfaceTemplate & GetHostTemplate(void) = 0;
			virtual string GetWorkingDirectory(void) = 0;
			virtual void GetSelectedFiles(Array<string> & names) = 0;
			virtual void SetSelectedFiles(const Array<string> & names) = 0;
			virtual void SetSelectedFiles(const string & name) = 0;
			virtual void SetSelectedFiles(void) = 0;
			virtual void EndDialog(void) = 0;
		};
		class IFileDialogCallback : public Object
		{
		public:
			virtual bool NeedsFullDialog(void) = 0;
			virtual void DialogWasCreated(IFileDialog & dialog) = 0;
			virtual void WorkingDirectoryWasChanged(IFileDialog & dialog) = 0;
			virtual void FilterSelectionWasChanged(IFileDialog & dialog) = 0;
			virtual void FileSelectionWasChanged(IFileDialog & dialog) = 0;
			virtual void OpenButtonWasPressed(IFileDialog & dialog) = 0;
			virtual void CancelButtonWasPressed(IFileDialog & dialog) = 0;
		};

		string GetEnvironmentVariable(const char * var);
		UI::InterfaceTemplate * GetCommonTemplate(void) noexcept;
		void SetCommonTemplate(UI::InterfaceTemplate * common) noexcept;
		void LoadCommonTemplate(void) noexcept;

		bool CommonFileDialog(IFileDialogCallback * callback, Windows::IWindow * parent) noexcept;
		bool CommonOpenFileDialog(Windows::OpenFileInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept;
		bool CommonSaveFileDialog(Windows::SaveFileInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept;
		bool CommonDirectoryDialog(Windows::ChooseDirectoryInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept;
		bool CommonMessageBox(Windows::MessageBoxResult * result, const string & text, const string & title, Windows::IWindow * parent, Windows::MessageBoxButtonSet buttons, Windows::MessageBoxStyle style, IDispatchTask * on_exit) noexcept;
	}
}