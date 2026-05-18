#include <Cor/CorSystemInformation.h>

#include <locale.h>
#include <time.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

namespace ESSE
{
	namespace System
	{
		intptr GetUserLocale(unichar8 * plocale, intptr buf_size) noexcept
		{
			if (buf_size > 3) buf_size = 3;
			auto data = setlocale(LC_ALL, 0); intptr i = 0;
			while (data[i] && i < buf_size) { plocale[i] = data[i]; i++; }
			if (i < buf_size) { plocale[i] = 0; return i + 1; } else { plocale[buf_size - 1] = 0; return buf_size; }
		}
		intptr GetUserLocale(unichar16 * plocale, intptr buf_size) noexcept
		{
			if (buf_size > 3) buf_size = 3;
			auto data = setlocale(LC_ALL, 0); intptr i = 0;
			while (data[i] && i < buf_size) { plocale[i] = data[i]; i++; }
			if (i < buf_size) { plocale[i] = 0; return i + 1; } else { plocale[buf_size - 1] = 0; return buf_size; }
		}
		intptr GetUserLocale(unichar32 * plocale, intptr buf_size) noexcept
		{
			if (buf_size > 3) buf_size = 3;
			auto data = setlocale(LC_ALL, 0); intptr i = 0;
			while (data[i] && i < buf_size) { plocale[i] = data[i]; i++; }
			if (i < buf_size) { plocale[i] = 0; return i + 1; } else { plocale[buf_size - 1] = 0; return buf_size; }
		}

		uint32 GetMonotonicTime(void) noexcept
		{
			timespec time;
			if (clock_gettime(CLOCK_MONOTONIC_RAW, &time) < 0) return 0;
			return time.tv_nsec / 1000000ULL + time.tv_sec * 1000ULL;
		}
		uint64 GetSystemTime(void) noexcept
		{
			struct timeval time;
			if (gettimeofday(&time, 0) < 0) return 0;
			return time.tv_sec * 1000ULL + time.tv_usec / 1000ULL;
		}
		uint64 TimeConvertToUniversal(uint64 time) noexcept
		{
			time_t sec = time_t(time / 1000ULL);
			tm time_data;
			gmtime_r(&sec, &time_data);
			return mktime(&time_data) * 1000ULL + time % 1000ULL;
		}
		uint64 TimeConvertToLocal(uint64 time) noexcept
		{
			time_t sec = time_t(time / 1000ULL);
			tm time_data;
			localtime_r(&sec, &time_data);
			return timegm(&time_data) * 1000ULL + time % 1000ULL;
		}

		Architecture GetSystemArchitecture(void) noexcept
		{
			struct utsname sn;
			if (uname(&sn) == 0) {
				if (Memory::StringCompare(sn.machine, "x86_64") == 0) return Architecture::X86_64;
				else if (Memory::StringCompare(sn.machine, "amd64") == 0) return Architecture::X86_64;
				else if (Memory::StringCompare(sn.machine, "i386") == 0) return Architecture::X86_32;
				else if (Memory::StringCompare(sn.machine, "i686") == 0) return Architecture::X86_32;
				else if (Memory::StringCompare(sn.machine, "i686-AT386") == 0) return Architecture::X86_32;
				else if (Memory::StringCompare(sn.machine, "aarch64") == 0) return Architecture::ARMv8_A64;
				else if (Memory::StringLength(sn.machine) >= 5 && Memory::MemoryCompare(sn.machine, "armv7", 5) == 0) return Architecture::ARMv7_T32;
				else return Architecture::Unknown;
			} else return Architecture::Unknown;
		}
		bool IsArchitectureEmulationEnabled(Architecture arch) noexcept
		{
			auto sysarch = GetSystemArchitecture();
			return (sysarch != Architecture::Unknown && sysarch == arch);
		}

		struct {
			bool initialized = false;
			unichar8 * name;
			uint32 physical_cores;
			uint64 frequency;
			ProcessorFeatureStatus feature_cpuid;
			ProcessorFeatureStatus feature_random;
			ProcessorFeatureStatus feature_aes;
			ProcessorFeatureStatus feature_sha256;
			ProcessorFeatureStatus feature_sha256_sw;
			ProcessorFeatureStatus feature_sha512;
			ProcessorFeatureStatus feature_sha512_sw;
		} Linux_ProcessorInformation;
		intptr Linux_FindSubstring(const unichar8 * in, intptr length, const unichar8 * subs) noexcept
		{
			auto subs_len = Memory::StringLength(subs);
			auto max_len = length + 1 - subs_len;
			for (intptr i = 0; i < max_len; i++) if (Memory::MemoryCompare(in + i, subs, subs_len * sizeof(unichar8)) == 0) return i;
			return -1;
		}
		bool Linux_CheckFeature(const unichar8 * in, intptr length, const unichar8 * name) noexcept { return Linux_FindSubstring(in, length, name) >= 0; }
		void Linux_ProcessFeatureSet(const unichar8 * in, intptr length) noexcept
		{
			#if defined(ESSE_MACHINA_X86_32) || defined(ESSE_MACHINA_X86_64)
			Linux_ProcessorInformation.feature_cpuid = ProcessorFeatureStatus::Present;
			Linux_ProcessorInformation.feature_random = Linux_CheckFeature(in, length, "rdrand") ? ProcessorFeatureStatus::Present : ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_aes = Linux_CheckFeature(in, length, "aes") ? ProcessorFeatureStatus::Present : ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha256 = Linux_CheckFeature(in, length, "sha256") ? ProcessorFeatureStatus::Present : ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha256_sw = Linux_CheckFeature(in, length, "pni") ? ProcessorFeatureStatus::Present : ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha512 = Linux_CheckFeature(in, length, "sha512") ? ProcessorFeatureStatus::Present : ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha512_sw = Linux_CheckFeature(in, length, "pni") ? ProcessorFeatureStatus::Present : ProcessorFeatureStatus::Unavailable;
			#elif defined(ESSE_MACHINA_ARM_V8)
			Linux_ProcessorInformation.feature_cpuid = ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_random = ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_aes = Linux_CheckFeature(in, length, "aes") ? ProcessorFeatureStatus::Present : ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha256 = Linux_CheckFeature(in, length, "sha2") ? ProcessorFeatureStatus::Present : ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha256_sw = ProcessorFeatureStatus::Present;
			Linux_ProcessorInformation.feature_sha512 = Linux_CheckFeature(in, length, "sha512") ? ProcessorFeatureStatus::Present : ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha512_sw = ProcessorFeatureStatus::Present;
			#else
			Linux_ProcessorInformation.feature_cpuid = ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_random = ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_aes = ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha256 = ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha256_sw = ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha512 = ProcessorFeatureStatus::Unavailable;
			Linux_ProcessorInformation.feature_sha512_sw = ProcessorFeatureStatus::Unavailable;
			#endif
		}
		void Linux_GetProcessorInformation(void) noexcept
		{
			Memory::AcquireRootLock();
			if (!Linux_ProcessorInformation.initialized) {
				Linux_ProcessorInformation.name = "";
				Linux_ProcessorInformation.physical_cores = 0;
				Linux_ProcessorInformation.frequency = 0;
				Linux_ProcessorInformation.feature_cpuid = ProcessorFeatureStatus::Unknown;
				Linux_ProcessorInformation.feature_random = ProcessorFeatureStatus::Unknown;
				Linux_ProcessorInformation.feature_aes = ProcessorFeatureStatus::Unknown;
				Linux_ProcessorInformation.feature_sha256 = ProcessorFeatureStatus::Unknown;
				Linux_ProcessorInformation.feature_sha256_sw = ProcessorFeatureStatus::Unknown;
				Linux_ProcessorInformation.feature_sha512 = ProcessorFeatureStatus::Unknown;
				Linux_ProcessorInformation.feature_sha512_sw = ProcessorFeatureStatus::Unknown;
				int cpuinfo_file;
				while (true) {
					cpuinfo_file = open("/proc/cpuinfo", O_RDONLY, 0666);
					if (cpuinfo_file >= 0 || errno != EINTR) break;
				}
				if (cpuinfo_file >= 0) {
					uintptr cpuinfo_size = 0;
					char * cpuinfo = 0;
					while (true) {
						char * new_cpuinfo = reinterpret_cast<char *>(realloc(cpuinfo, cpuinfo_size + 0x1000));
						if (!new_cpuinfo) { free(cpuinfo); cpuinfo = 0; break; }
						cpuinfo = new_cpuinfo;
						bool eof = false;
						while (true) {
							auto num = read(cpuinfo_file, cpuinfo + cpuinfo_size, 0x1000);
							if (num < 0 && errno != EINTR) { free(cpuinfo); cpuinfo = 0; eof = true; break; }
							else if (num >= 0) {
								cpuinfo_size += num;
								if (num == 0) eof = true;
								break;
							}
						}
						if (eof) break;
					}
					close(cpuinfo_file);
					uintptr coremap_size = 0x1000;
					uint * coremap = reinterpret_cast<uint *>(malloc(coremap_size * sizeof(uint)));
					if (cpuinfo && coremap) {
						for (intptr j = 0; j < coremap_size; j++) coremap[j] = 0xFFFFFFFF;
						intptr pos = 0;
						bool name_set = false, feat_set = false;
						while (pos < cpuinfo_size) {
							intptr next = Linux_FindSubstring(cpuinfo + pos + 1, cpuinfo_size - pos - 1, "processor");
							if (next < 0) next = cpuinfo_size; else next += pos + 1;
							if (!name_set) {
								auto name_offs = Linux_FindSubstring(cpuinfo + pos, cpuinfo_size - pos, "model name");
								if (name_offs >= 0) {
									auto col_offs = Linux_FindSubstring(cpuinfo + pos + name_offs, cpuinfo_size - pos - name_offs, ": ");
									auto lf_offs = Linux_FindSubstring(cpuinfo + pos + name_offs, cpuinfo_size - pos - name_offs, "\n");
									if (col_offs >= 0 && lf_offs >= 0) {
										name_set = true;
										col_offs += pos + name_offs + 2;
										lf_offs += pos + name_offs;
										auto len = lf_offs - col_offs;
										Linux_ProcessorInformation.name = reinterpret_cast<char *>(malloc(len + 1));
										if (Linux_ProcessorInformation.name) {
											Memory::MemoryCopy(Linux_ProcessorInformation.name, cpuinfo + col_offs, len);
											Linux_ProcessorInformation.name[len] = 0;
										} else Linux_ProcessorInformation.name = "";
									}
								}
							}
							uint core_id = 0, proc_id = 0;
							auto field_offs = Linux_FindSubstring(cpuinfo + pos, cpuinfo_size - pos, "physical id");
							if (field_offs >= 0) {
								auto col_offs = Linux_FindSubstring(cpuinfo + pos + field_offs, cpuinfo_size - pos - field_offs, ": ");
								auto lf_offs = Linux_FindSubstring(cpuinfo + pos + field_offs, cpuinfo_size - pos - field_offs, "\n");
								if (col_offs >= 0 && lf_offs >= 0) {
									name_set = true;
									col_offs += pos + field_offs + 2;
									lf_offs += pos + field_offs;
									cpuinfo[lf_offs] = 0;
									sscanf(cpuinfo + col_offs, "%i", &proc_id);
								}
							}
							field_offs = Linux_FindSubstring(cpuinfo + pos, cpuinfo_size - pos, "core id");
							if (field_offs >= 0) {
								auto col_offs = Linux_FindSubstring(cpuinfo + pos + field_offs, cpuinfo_size - pos - field_offs, ": ");
								auto lf_offs = Linux_FindSubstring(cpuinfo + pos + field_offs, cpuinfo_size - pos - field_offs, "\n");
								if (col_offs >= 0 && lf_offs >= 0) {
									name_set = true;
									col_offs += pos + field_offs + 2;
									lf_offs += pos + field_offs;
									cpuinfo[lf_offs] = 0;
									sscanf(cpuinfo + col_offs, "%i", &core_id);
								}
							}
							uint com_id = (proc_id << 16) | core_id;
							for (intptr i = 0; i < coremap_size; i++) {
								if (coremap[i] == com_id) break;
								if (coremap[i] == 0xFFFFFFFF) { coremap[i] = com_id; break; }
							}
							if (!feat_set) {
								auto feat_offs = Linux_FindSubstring(cpuinfo + pos, cpuinfo_size - pos, "flags");
								if (feat_offs < 0) feat_offs = Linux_FindSubstring(cpuinfo + pos, cpuinfo_size - pos, "Features");
								if (feat_offs >= 0) {
									auto col_offs = Linux_FindSubstring(cpuinfo + pos + feat_offs, cpuinfo_size - pos - feat_offs, ": ");
									auto lf_offs = Linux_FindSubstring(cpuinfo + pos + feat_offs, cpuinfo_size - pos - feat_offs, "\n");
									if (col_offs >= 0 && lf_offs >= 0) {
										feat_set = true;
										col_offs += pos + feat_offs + 2;
										lf_offs += pos + feat_offs;
										Linux_ProcessFeatureSet(cpuinfo + col_offs, lf_offs - col_offs);
									}
								}
							}
							pos = next;
						}
						for (intptr i = 0; i < coremap_size; i++) if (coremap[i] != 0xFFFFFFFF) Linux_ProcessorInformation.physical_cores++;
					}
					free(cpuinfo);
					free(coremap);
				}
				int cpufreq_file;
				while (true) {
					cpufreq_file = open("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", O_RDONLY, 0666);
					if (cpufreq_file >= 0 || errno != EINTR) break;
				}
				if (cpufreq_file >= 0) {
					char * data = reinterpret_cast<char *>(malloc(0x100));
					if (data) while (true) {
						auto size = read(cpufreq_file, data, 0xFF);
						if (size < 0 && errno != EINTR) { free(data); data = 0; break; }
						else if (size >= 0) { data[size] = 0; break; }
					}
					close(cpufreq_file);
					uint freq_khz = 0;
					sscanf(data, "%i", &freq_khz);
					free(data);
					Linux_ProcessorInformation.frequency = uint64(freq_khz) * 1000ULL;
				}
				Linux_ProcessorInformation.initialized = true;
			}
			Memory::ReleaseRootLock();
		}

		ProcessorFeatureStatus GetProcessorFeatureStatus(ProcessorFeature feat) noexcept
		{
			Linux_GetProcessorInformation();
			if (feat == ProcessorFeature::CPUID) return Linux_ProcessorInformation.feature_cpuid;
			else if (feat == ProcessorFeature::RNG) return Linux_ProcessorInformation.feature_random;
			else if (feat == ProcessorFeature::AES) return Linux_ProcessorInformation.feature_aes;
			else if (feat == ProcessorFeature::SHA256) return Linux_ProcessorInformation.feature_sha256;
			else if (feat == ProcessorFeature::SHA256SW) return Linux_ProcessorInformation.feature_sha256_sw;
			else if (feat == ProcessorFeature::SHA512) return Linux_ProcessorInformation.feature_sha512;
			else if (feat == ProcessorFeature::SHA512SW) return Linux_ProcessorInformation.feature_sha512_sw;
			else return ProcessorFeatureStatus::Unknown;
		}
		intptr GetProcessorName(unichar8 * pname, intptr buf_size) noexcept
		{
			Linux_GetProcessorInformation();
			intptr i = 0; while (Linux_ProcessorInformation.name[i] && i < buf_size) { pname[i] = Linux_ProcessorInformation.name[i]; i++; }
			if (i < buf_size) { pname[i] = 0; return i + 1; } else { pname[buf_size - 1] = 0; return buf_size; }
		}
		intptr GetProcessorName(unichar16 * pname, intptr buf_size) noexcept
		{
			Linux_GetProcessorInformation();
			intptr i = 0; while (Linux_ProcessorInformation.name[i] && i < buf_size) { pname[i] = Linux_ProcessorInformation.name[i]; i++; }
			if (i < buf_size) { pname[i] = 0; return i + 1; } else { pname[buf_size - 1] = 0; return buf_size; }
		}
		intptr GetProcessorName(unichar32 * pname, intptr buf_size) noexcept
		{
			Linux_GetProcessorInformation();
			intptr i = 0; while (Linux_ProcessorInformation.name[i] && i < buf_size) { pname[i] = Linux_ProcessorInformation.name[i]; i++; }
			if (i < buf_size) { pname[i] = 0; return i + 1; } else { pname[buf_size - 1] = 0; return buf_size; }
		}
		uint GetProcessorCores(bool physical) noexcept
		{
			if (physical) {
				Linux_GetProcessorInformation();
				return Linux_ProcessorInformation.physical_cores;
			} else return get_nprocs();
		}
		uint64 GetProcessorFrequency(void) noexcept
		{
			Linux_GetProcessorInformation();
			return Linux_ProcessorInformation.frequency;
		}
		uint64 GetPhysicalMemory(void) noexcept { struct sysinfo si; if (sysinfo(&si) < 0) return 0; return si.totalram; }
		uint64 GetVirtualMemoryPageSize(void) noexcept { return sysconf(_SC_PAGESIZE); }

		intptr GetSystemName(unichar8 * pname, intptr buf_size) noexcept
		{
			struct utsname sn;
			if (uname(&sn) >= 0) {
				intptr i = 0; while (sn.sysname[i] && i < buf_size) { pname[i] = sn.sysname[i]; i++; }
				if (i < buf_size) { pname[i] = 0; return i + 1; } else { pname[buf_size - 1] = 0; return buf_size; }
			} else { pname[0] = 0; return 1; }
		}
		intptr GetSystemName(unichar16 * pname, intptr buf_size) noexcept
		{
			struct utsname sn;
			if (uname(&sn) >= 0) {
				intptr i = 0; while (sn.sysname[i] && i < buf_size) { pname[i] = sn.sysname[i]; i++; }
				if (i < buf_size) { pname[i] = 0; return i + 1; } else { pname[buf_size - 1] = 0; return buf_size; }
			} else { pname[0] = 0; return 1; }
		}
		intptr GetSystemName(unichar32 * pname, intptr buf_size) noexcept
		{
			struct utsname sn;
			if (uname(&sn) >= 0) {
				intptr i = 0; while (sn.sysname[i] && i < buf_size) { pname[i] = sn.sysname[i]; i++; }
				if (i < buf_size) { pname[i] = 0; return i + 1; } else { pname[buf_size - 1] = 0; return buf_size; }
			} else { pname[0] = 0; return 1; }
		}
		void GetSystemVersion(uint * major, uint * minor) noexcept
		{
			uint v1 = 0, v2 = 0;
			struct utsname sn;
			if (uname(&sn) >= 0) {
				for (int i = 0; i < sizeof(sn.release); i++) if (sn.release[i] == '.') {
					for (int j = i + 1; j < sizeof(sn.release); j++) if (sn.release[j] == '.') { sn.release[j] = 0; break; }
					sn.release[i] = 0;
					sscanf(sn.release, "%i", &v1);
					sscanf(sn.release + i + 1, "%i", &v2);
					break;
				}
			}
			if (major) *major = v1;
			if (minor) *minor = v2;
		}
	}
}