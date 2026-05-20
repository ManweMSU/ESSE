#pragma once

#include "../Classes/CorString.h"

namespace ESSE
{
	enum class ShellExecuteMode { OpenFile = 0, ShowFile = 1, ShowDirectory = 2, OpenConsole = 3 };
	bool ShellExecute(const string & path, ShellExecuteMode mode) noexcept;
}