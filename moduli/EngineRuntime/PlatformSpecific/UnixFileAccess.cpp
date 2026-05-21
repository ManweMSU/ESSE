#include "UnixFileAccess.h"

namespace Engine
{
	namespace IO
	{
		namespace Unix
		{
			void SetFileAccessRights(handle file, uint user, uint group, uint other)
			{
				ESSE::IO::SetFilePermissions(file, user, group, other);
			}
			uint GetFileUserAccessRights(handle file)
			{
				uint perm;
				ESSE::IO::GetFilePermissions(file, &perm, 0, 0);
				return perm;
			}
			uint GetFileGroupAccessRights(handle file)
			{
				uint perm;
				ESSE::IO::GetFilePermissions(file, 0, &perm, 0);
				return perm;
			}
			uint GetFileOtherAccessRights(handle file)
			{
				uint perm;
				ESSE::IO::GetFilePermissions(file, 0, 0, &perm);
				return perm;
			}
		}
	}
}