#include "esse_reconf.h"

using namespace Engine;
using namespace Engine::IO;
using namespace Engine::IO::ConsoleControl;
using namespace Engine::Streaming;
using namespace Engine::Storage;

#ifdef ENGINE_X64
#ifdef ENGINE_ARM
#define THIS_MACHINE_ARCH L"ARM64"
#else
#define THIS_MACHINE_ARCH L"X64"
#endif
#else
#ifdef ENGINE_ARM
#define THIS_MACHINE_ARCH L"ARM"
#else
#define THIS_MACHINE_ARCH L"X86"
#endif
#endif

namespace esse {
	namespace constructor {
		RegistryNode * load_init_file(const string & name, io_context & io)
		{
			SafePointer<RegistryNode> result;
			for (auto & i : io.init_list) try { result = load_configuration(i + L"/" + name + L".ini"); break; } catch (...) {}
			if (!result) throw Exception();
			result->Retain();
			return result;
		}
		void reconfigure_linux_select_arch(RegistryNode * arch)
		{
			SafePointer<RegistryNode> dest = arch->OpenNode(L"Destinationes");
			if (!dest) throw InvalidStateException();
			Array<string> remove_list(0x10), defaultize_list(0x10);
			for (auto & name : dest->GetSubnodes()) {
				if (string::CompareIgnoreCase(name, THIS_MACHINE_ARCH) == 0) defaultize_list.Append(name);
				else remove_list.Append(name);
			}
			for (auto & name : remove_list) dest->RemoveNode(name);
			for (auto & name : defaultize_list) {
				SafePointer<RegistryNode> def = dest->OpenNode(name);
				if (!def) throw InvalidStateException();
				def->CreateValue(L"Defalta", RegistryValueType::Boolean);
				def->SetValue(L"Defalta", true);
			}
		}
		bool reconfigure_linux(const string & tsc, io_context & io)
		{
			try {
				SafePointer<RegistryNode> node_common = load_init_file(L"common", io);
				SafePointer<RegistryNode> node_common_arch = load_init_file(L"common-arch", io);
				SafePointer<RegistryNode> node_linux = load_init_file(L"linux", io);
				reconfigure_linux_select_arch(node_common_arch);
				ObjectArray<RegistryNode> nodes(0x10);
				nodes.Append(node_common); nodes.Append(node_common_arch); nodes.Append(node_linux);
				SafePointer<RegistryNode> merged = CreateMergedNode(nodes);
				SafePointer<Registry> reg = CreateRegistryFromNode(merged);
				SafePointer<FileStream> stream = new FileStream(tsc, AccessWrite, CreateAlways);
				try { Unix::SetFileAccessRights(stream->Handle(), Unix::AccessRightRegular, Unix::AccessRightRead, Unix::AccessRightRead); } catch (...) {}
				RegistryToText(reg, stream, Encoding::UTF8);
				return true;
			} catch (...) { return false; }
		}
		bool reconfigure(const string & tsc, io_context & io)
		{
			#ifdef ENGINE_LINUX
			return reconfigure_linux(tsc, io);
			#else
			// TODO: IMPLEMENT
			return false;
			#endif
		}
	}
}