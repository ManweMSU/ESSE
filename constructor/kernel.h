#pragma once

#include <EngineRuntime.h>

#include "ioss.h"

typedef Engine::ImmutableString string;
typedef Engine::Object object;
typedef Engine::Volumes::Dictionary<string, string> localized_string;

namespace esse {
	namespace constructor {
		enum class module_class { common = 0, abstraction_layer = 1, abstraction_layer_implementation = 2, codec = 3, resource_engine = 4, common_root = 0x100 };
		struct compile_task
		{
			string from_module;
			string source_path;
			string option;
		};
		struct resource_task
		{
			string source_path;
			string resource_name;
			string resource_locale;
		};
		struct attach_task
		{
			string source_path;
			string destination_path;
		};
		struct module : public object
		{
			// core metadata
			module_class type;
			string name, author, identifier, version;
			Engine::Volumes::Set<string> systems, subsystems;
			Engine::Time manifest_alternation_date;
			// dependencies
			Engine::Volumes::Set<string> needs_modules;
			// abstractions and implementations
			string needs_abstraction_layer_implementation, needs_codecs;
			string implements_abstraction_layer;
			bool is_default_abstraction_layer_implementation;
			Engine::Volumes::Dictionary<string, string> implements_codecs; // codec domain - codec function name
			// build rules
			Engine::Volumes::List<compile_task> compile_list;
			Engine::Volumes::List<resource_task> resource_list;
			Engine::Volumes::List<attach_task> attach_list;
			Engine::Volumes::List<string> include_list;
			Engine::Volumes::Dictionary<string, string> defines_list;
		};

		enum class file_format_role { viewer = 0, editor = 1 };
		struct file_format
		{
			localized_string file_format_description;
			string file_format_icon, file_format_extension;
			file_format_role role;
		};
		struct uri_scheme
		{
			localized_string uri_scheme_description;
			string uri_prefix;
		};
		struct store_extension
		{
			localized_string name;
			string target_product_identifier;
			string extension_file;
		};
		struct application : public object
		{
			// root metadata
			string name, build_path, subsystem;
			string application_identifier, author_identifier;
			bool no_high_dpi_scale, no_dock_icon, make_version_defines;
			// application metadata
			string application_icon, application_internal_name, application_version;
			uint application_version_major, application_version_minor, application_version_micro, application_build_number;
			Engine::Volumes::List<string> localizations;
			localized_string application_name, application_author, application_copyright, application_description;
			Engine::Volumes::List<file_format> file_formats;
			Engine::Volumes::List<uri_scheme> uri_schemes;
			Engine::Volumes::List<string> command_line_tools;
			// permissions
			bool needs_elevation;
			localized_string needs_camera_reason, needs_microphone_reson;
			// store integration
			string store_database_integration_file;
			string store_persistent_files;
			Engine::Volumes::List<store_extension> store_extensions;
		};
		
		struct build_state;
		enum class build_target_class { architecture, system, configuration, subsystem };
		enum class build_tool_status { built_new, built_renew, skipped, failed };
		struct process_context
		{
			virtual void build_status_notify(const string & input, build_tool_status status, Engine::Time t1, Engine::Time t2, const string & output) = 0;
			virtual void enter_state_critical_section(void) noexcept = 0;
			virtual void leave_state_critical_section(void) noexcept = 0;
		};
		struct build_tool : public object
		{
			Engine::Volumes::List<string> extra_command_line;
			virtual void process_file(const string & input, const string & mdl, const string & option, build_state * state, process_context & context) = 0;
			virtual void enumerate_extensions(Engine::Array<string> & list) = 0;
		};
		struct build_target
		{
			build_target_class type;
			localized_string name;
			string identifier;
			bool is_default;
		};
		struct build_state
		{
			// common paths
			io_context * io;
			Engine::Volumes::List<string> module_search_list, include_list, data_files_list;
			Engine::Volumes::Dictionary<string, build_target> installed_targets;
			Engine::SafePointer<Engine::Storage::Registry> build_set_common_configuration;
			// toolset state
			Engine::Volumes::ObjectDictionary<string, build_tool> compilers; // extension - handler
			Engine::SafePointer<build_tool> cc_tool, resource_tool, link_tool;
			// application build set
			Engine::SafePointer<Engine::Storage::Registry> project_common_configuration;
			application app;
			module * root_module;
			Engine::Volumes::ObjectDictionary<string, module> modules;
			Engine::Volumes::ObjectDictionary<string, module> modules_to_build;
			Engine::Volumes::Set<string> abstraction_layers;
			Engine::Volumes::ObjectDictionary< string, Engine::Volumes::Set<string> > codec_domains;
			build_target * project_system, * project_subsystem, * project_architecture, * project_configuration;
			// current build state
			bool idle_mode, clean_mode;
			Engine::Time project_alternation_date;
			string project_file_path, project_root_path;
			string project_build_path, project_object_path, project_build_name;
			string output_exec_path, output_bundle_path;
			Engine::Volumes::List<compile_task> compile_list;
			Engine::Volumes::List<resource_task> resource_list;
			Engine::Volumes::List<attach_task> attach_list;
			Engine::Volumes::Dictionary<string, string> defines_list;
			Engine::Volumes::List<string> link_list;
		};

		string read_localized_string(const localized_string & str);
		string read_localized_string(const localized_string & str, const string & locale);
		localized_string load_localized_string(Engine::Storage::RegistryNode * node, const string & path);
		string escape_string_c(const string & input);
		void command_line_append(Engine::Array<string> & cmd, const string & pattern, const string & value);
		bool build_state_initialize(build_state & state, const string & tsc, io_context & io);
		bool load_esse_project(build_state & state, const string & prc, const string & sys, const string & arch, const string & mode);
		bool load_engine_runtime_project(build_state & state, const string & prc, const string & sys, const string & arch, const string & mode);
		bool load_adhoc_project(build_state & state, const string & prc, const string & sys, const string & arch, const string & mode);
		bool make_project_dependencies(build_state & state);
		bool construct(build_state & state);
	}
}
