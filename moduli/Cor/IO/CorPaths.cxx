#include "CorPaths.h"
#include "../Classes/CorArray.hxx"

namespace ESSE
{
	namespace IO
	{
		const handle InvalidHandle = handle(sintptr(-1));
		namespace Path
		{
			string Normalize(const string & path)
			{
				if (PathSeparator == U'\\') return path.Replace(U'/', U'\\');
				else if (PathSeparator == U'/') return path.Replace(U'\\', U'/');
				return string();
			}
			string GetExtension(const string & path)
			{
				intptr s = path.GetLength() - 1;
				while (s >= 0 && (path[s] == U'\\' || path[s] == U'/')) s--;
				for (intptr i = s; i >= 0; i--) {
					if (path[i] == U'.') {
						if (i != 0 && path[i - 1] != U'\\' && path[i - 1] != U'/') return path.Substring(i + 1, s - 1);
					} else if (path[i] == U'\\' || path[i] == U'/') return string();
				}
				return string();
			}
			string GetFileName(const string & path)
			{
				intptr s = path.GetLength() - 1;
				while (s >= 0 && (path[s] == U'\\' || path[s] == U'/')) s--;
				for (intptr i = s; i >= 0; i--) {
					if (path[i] == U'\\' || path[i] == U'/') return path.Substring(i + 1, s - i);
				}
				return path.Substring(0, s + 1);
			}
			string GetDirectory(const string & path)
			{
				intptr s = path.GetLength() - 1;
				while (s >= 0 && (path[s] == U'\\' || path[s] == U'/')) s--;
				for (intptr i = s; i >= 0; i--) {
					if (path[i] == U'\\' || path[i] == U'/') return path.Substring(0, i);
				}
				return string();
			}
			string GetPureFileName(const string & path)
			{
				auto name = GetFileName(path);
				if (!name.GetLength()) return string();
				for (uintptr i = name.GetLength() - 1; i > 0; i--) if (name[i] == L'.') return name.Substring(0, i);
				return name;
			}
			bool MatchFilterRange(const string & path, const string & filter, uintptr pfrom, uintptr ffrom) noexcept
			{
				if (ffrom >= filter.GetLength() && pfrom < path.GetLength()) return false;
				if (ffrom < filter.GetLength() && pfrom >= path.GetLength()) {
					for (uintptr k = ffrom; k < filter.GetLength(); k++) if (filter[k] != U'*') return false;
					if (ffrom == filter.GetLength() - 1) return true;
				}
				if (ffrom >= filter.GetLength()) return true;
				uintptr i = ffrom, j = pfrom;
				while (true) {
					uintptr ep = i;
					while (ep < filter.GetLength() && filter[ep] != U'*' && filter[ep] != U'?' && filter[ep] != U'\\' && filter[ep] != U'/') ep++;
					if (ep > i) {
						if (path.GetLength() - j < ep - i) return false;
						ErrorContext ectx; ErrorClear(ectx);
						if (Unicode::CaseInsensitiveCompare(path.GetData() + j, ep - i, filter.GetData() + i, ep - i, ectx)) return false;
						if (ErrorTest(ectx)) return false;
						j += ep - i;
						i = ep;
					}
					if (i >= filter.GetLength()) return j >= path.GetLength();
					if (j >= path.GetLength()) return i >= filter.GetLength() || MatchFilterRange(path, filter, j, i);
					if (filter[i] == U'*') {
						for (uintptr k = j; k <= path.GetLength(); k++) if (MatchFilterRange(path, filter, k, i + 1)) return true;
						return false;
					} else if (filter[i] == U'?') {
						i++; j++;
					} else if (filter[i] == U'\\') {
						if (path[i] != U'\\' && path[i] != U'/') return false;
						i++; j++;
					} else if (filter[i] == U'/') {
						if (path[i] != U'\\' && path[i] != U'/') return false;
						i++; j++;
					}
				}
			}
			bool MatchFilter(const string & path, const string & filter)
			{
				if (filter == U"*") return true;
				auto filters = SplitString(filter, U';');
				for (auto & f : filters) if (MatchFilterRange(path, f, 0, 0)) return true;
				return false;
			}
		}
	}
}