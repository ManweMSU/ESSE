#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <wait.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

const char * defines[] = {
	"ESSE_SYSTEMA_UNIX=1",
	"ESSE_SYSTEMA_LINUX=1",
	"ESSE_SUBSYSTEMA_CONSOLE=1",
	"ESSE_VERSIO_CORDIS_MAJOR=0",
	"ESSE_VERSIO_CORDIS_MINOR=0",
	"ESSE_VERSIO_CORDIS_MICRO=0"
};
const char * defines_x86[] = {
	"ESSE_MACHINA_X86_32=1"
};
const char * defines_x64[] = {
	"ESSE_MACHINA_64=1",
	"ESSE_MACHINA_X86_64=1"
};
const char * defines_arm[] = {
	"ESSE_MACHINA_ARM_V7=1"
};
const char * defines_arm64[] = {
	"ESSE_MACHINA_64=1",
	"ESSE_MACHINA_ARM_V8=1"
};

struct {
	int input_count;
	char ** input;
	int include_count;
	char ** include;
	int objects_count;
	char ** objects;
	char * object;
	char * destination;
} state;

void state_init(void)
{
	state.input_count = 0; state.input = 0;
	state.include_count = 0; state.include = 0;
	state.objects_count = 0; state.objects = 0;
	state.object = 0; state.destination = 0;
}
int string_set(char ** string, const char * text)
{
	char * copy = malloc(strlen(text) + 1);
	if (!copy) return 0;
	strcpy(copy, text);
	free(*string);
	*string = copy;
	return 1;
}
void string_array_destroy(int length, char ** volume) { for (int i = 0; i < length; i++) free(volume[i]); free(volume); }
int string_array_add(int * length, char *** volume, const char * text)
{
	char ** reallocation = realloc((*length) ? *volume : 0, sizeof(char *) * (*length + 1));
	if (!reallocation) return 0;
	*volume = reallocation;
	reallocation[*length] = 0;
	if (!string_set(reallocation + *length, text)) return 0;
	(*length)++;
	return 1;
}
int string_array_finish(int * length, char *** volume)
{
	char ** reallocation = realloc((*length) ? *volume : 0, sizeof(char *) * (*length + 1));
	if (!reallocation) return 0;
	*volume = reallocation;
	reallocation[*length] = 0;
	(*length)++;
	return 1;
}
int add_input(const char * path) { return string_array_add(&state.input_count, &state.input, path); }
int add_include(const char * path) { return string_array_add(&state.include_count, &state.include, path); }
int set_object(const char * path) { return string_set(&state.object, path); }
int set_output(const char * path) { return string_set(&state.destination, path); }
int index_directory(const char * path)
{
	DIR * dir = opendir(path);
	if (!dir) return 0;
	struct dirent * ent;
	int status = 1;
	while (ent = readdir(dir)) {
		if (ent->d_name[0] == '.') continue;
		char * common_path = malloc(strlen(path) + strlen(ent->d_name) + 2);
		if (!common_path) { status = 0; break; }
		strcpy(common_path, path);
		strcat(common_path, "/");
		strcat(common_path, ent->d_name);
		if (ent->d_type == 8) { // regular file
			int i = strlen(ent->d_name);
			while (i > 0 && ent->d_name[i] != '.') i--;
			if (strcasecmp(ent->d_name + i, ".cpp") == 0 || strcasecmp(ent->d_name + i, ".cxx") == 0 ||
				strcasecmp(ent->d_name + i, ".c++") == 0 || strcasecmp(ent->d_name + i, ".cc") == 0 ||
				strcasecmp(ent->d_name + i, ".c") == 0) {
				status = add_input(common_path);
			}
		} else if (ent->d_type == 4) { // directory
			status = index_directory(common_path);
		}
		free(common_path);
		if (!status) break;
	}
	closedir(dir);
	return status;
}

int main(int argc, char ** argv)
{
	state_init();
	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			if (argv[i][0] == 'I' && argv[i][1] == ':') {
				if (!add_include(argv[i] + 2)) return 1;
			} else if (argv[i][0] == 'J' && argv[i][1] == ':') {
				if (!set_object(argv[i] + 2)) return 1;
			} else if (argv[i][0] == 'O' && argv[i][1] == ':') {
				if (!set_output(argv[i] + 2)) return 1;
			} else {
				int length = strlen(argv[i]);
				if (length > 0 && argv[i][length - 1] == '/') {
					argv[i][length - 1] = 0;
					if (!index_directory(argv[i])) return 1;
				} else {
					if (!add_input(argv[i])) return 1;
				}
			}
		}
		if (!state.input_count || !state.object || !state.destination) return 3;
		int main_defines_length = sizeof(defines) / sizeof(*defines);
		const char ** main_defines = defines;
		int platform_defines_length = 0;
		const char ** platform_defines = 0;
		struct utsname sn;
		if (uname(&sn) == 0) {
			if (strcmp(sn.machine, "x86_64") == 0) { platform_defines_length = sizeof(defines_x64) / sizeof(*defines_x64); platform_defines = defines_x64; }
			else if (strcmp(sn.machine, "amd64") == 0) { platform_defines_length = sizeof(defines_x64) / sizeof(*defines_x64); platform_defines = defines_x64; }
			else if (strcmp(sn.machine, "i386") == 0) { platform_defines_length = sizeof(defines_x86) / sizeof(*defines_x86); platform_defines = defines_x86; }
			else if (strcmp(sn.machine, "i686") == 0) { platform_defines_length = sizeof(defines_x86) / sizeof(*defines_x86); platform_defines = defines_x86; }
			else if (strcmp(sn.machine, "i686-AT386") == 0) { platform_defines_length = sizeof(defines_x86) / sizeof(*defines_x86); platform_defines = defines_x86; }
			else if (strcmp(sn.machine, "aarch64") == 0) { platform_defines_length = sizeof(defines_arm64) / sizeof(*defines_arm64); platform_defines = defines_arm64; }
			else if (strlen(sn.machine) >= 5 && memcmp(sn.machine, "armv7", 5) == 0) { platform_defines_length = sizeof(defines_arm) / sizeof(*defines_arm); platform_defines = defines_arm; }
			else return 2;
		} else return 2;
		for (int i = 0; i < state.input_count; i++) {
			int e = -1;
			int s = strlen(state.input[i]);
			while (s > 0 && state.input[i][s] != '/') {
				if (e < 0 && state.input[i][s] == '.') e = s;
				s--;
			}
			if (e < 0) e = strlen(state.input[i]);
			if (state.input[i][s] == '/') s++;
			char * object_path = malloc(strlen(state.object) + e - s + 16);
			if (!object_path) return 1;
			strcpy(object_path, state.object);
			strcat(object_path, "/");
			sprintf(object_path + strlen(object_path), "s%i-", i);
			int j = strlen(object_path);
			for (; s < e; s++, j++) {
				object_path[j] = state.input[i][s];
				object_path[j + 1] = 0;
			}
			strcat(object_path, ".o");
			struct stat stat_c, stat_o;
			if (stat(state.input[i], &stat_c) >= 0 && stat(object_path, &stat_o) >= 0) {
				if (stat_c.st_mtime <= stat_o.st_mtime) {
					if (!string_array_add(&state.objects_count, &state.objects, object_path)) return 1;
					free(object_path);
					continue;
				}
			}
			int local_argc = 0;
			char ** local_argv = 0;
			if (!string_array_add(&local_argc, &local_argv, "g++")) return 1;
			if (!string_array_add(&local_argc, &local_argv, state.input[i])) return 1;
			if (!string_array_add(&local_argc, &local_argv, "-c")) return 1;
			if (!string_array_add(&local_argc, &local_argv, "-std=c++17")) return 1;
			if (!string_array_add(&local_argc, &local_argv, "-fpermissive")) return 1;
			if (!string_array_add(&local_argc, &local_argv, "-O0")) return 1;
			if (!string_array_add(&local_argc, &local_argv, "-ggdb")) return 1;
			for (j = 0; j < main_defines_length; j++) {
				if (!string_array_add(&local_argc, &local_argv, "-D")) return 1;
				if (!string_array_add(&local_argc, &local_argv, main_defines[j])) return 1;
			}
			for (j = 0; j < platform_defines_length; j++) {
				if (!string_array_add(&local_argc, &local_argv, "-D")) return 1;
				if (!string_array_add(&local_argc, &local_argv, platform_defines[j])) return 1;
			}
			for (j = 0; j < state.include_count; j++) {
				if (!string_array_add(&local_argc, &local_argv, "-I")) return 1;
				if (!string_array_add(&local_argc, &local_argv, state.include[j])) return 1;
			}
			if (!string_array_add(&local_argc, &local_argv, "-o")) return 1;
			if (!string_array_add(&local_argc, &local_argv, object_path)) return 1;
			if (!string_array_finish(&local_argc, &local_argv)) return 1;
			pid_t fork_state = fork();
			if (fork_state < 0) return 4;
			if (fork_state > 0) {
				int status;
				waitpid(fork_state, &status, 0);
				if (!WIFEXITED(status) || WEXITSTATUS(status)) return 5;
			} else {
				execvp(local_argv[0], local_argv);
				exit(5);
			}
			string_array_destroy(local_argc, local_argv);
			if (!string_array_add(&state.objects_count, &state.objects, object_path)) return 1;
			free(object_path);
		}
		int local_argc = 0;
		char ** local_argv = 0;
		if (!string_array_add(&local_argc, &local_argv, "g++")) return 1;
		for (int i = 0; i < state.objects_count; i++) {
			if (!string_array_add(&local_argc, &local_argv, state.objects[i])) return 1;
		}
		if (!string_array_add(&local_argc, &local_argv, "-O0")) return 1;
		if (!string_array_add(&local_argc, &local_argv, "-ggdb")) return 1;
		if (!string_array_add(&local_argc, &local_argv, "-pthread")) return 1;
		if (!string_array_add(&local_argc, &local_argv, "-lrt")) return 1;
		if (!string_array_add(&local_argc, &local_argv, "-lm")) return 1;
		if (!string_array_add(&local_argc, &local_argv, "-o")) return 1;
		if (!string_array_add(&local_argc, &local_argv, state.destination)) return 1;
		if (!string_array_finish(&local_argc, &local_argv)) return 1;
		pid_t fork_state = fork();
		if (fork_state < 0) return 4;
		if (fork_state > 0) {
			int status;
			waitpid(fork_state, &status, 0);
			if (!WIFEXITED(status) || WEXITSTATUS(status)) return 5;
		} else {
			execvp(local_argv[0], local_argv);
			exit(5);
		}
		string_array_destroy(local_argc, local_argv);
	} else {
		printf("ESSE RESURRECTOR\n");
		printf("  resurrector {path/to/file.c} {path/to/path/}\n");
		printf("  {I:include/path} [J:object/path] [O:output/path]\n");
	}
	return 0;
}