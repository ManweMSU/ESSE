#include "esse_rsrc.h"

using namespace Engine;
using namespace Engine::IO;
using namespace Engine::Streaming;

namespace esse {
	namespace constructor {
		struct resource_record
		{
			string name, locale, id;
			SafePointer<DataBlock> data;
		};
		struct dynamic_string_writer
		{
			Array<char> buffer;
			dynamic_string_writer(void) : buffer(0x100000) {}
			DataBlock * finalize(void)
			{
				SafePointer<DataBlock> result = new DataBlock(1);
				result->SetLength(buffer.Length());
				MemoryCopy(result->GetBuffer(), buffer.GetBuffer(), buffer.Length());
				result->Retain();
				return result;
			}
			friend dynamic_string_writer & operator << (dynamic_string_writer & wri, const wchar_t * input) { uint i = 0; while (input[i]) wri << input[i++]; return wri; }
			friend dynamic_string_writer & operator << (dynamic_string_writer & wri, wchar_t input) { wri.buffer.Append(char(input)); return wri; }
			friend dynamic_string_writer & operator << (dynamic_string_writer & wri, const char32_t * input) { uint i = 0; while (input[i]) wri << input[i++]; return wri; }
			friend dynamic_string_writer & operator << (dynamic_string_writer & wri, char32_t input) { wri.buffer.Append(char(input)); return wri; }
		};
		
		void write_hexadecimal(dynamic_string_writer & dest, const DataBlock & data) { for (auto & b : data) dest << L"0x" << HexadecimalBase[b >> 4] << HexadecimalBase[b & 15] << L","; }
		void write_localized_string(Storage::RegistryNode * node, const string & name, const localized_string & str)
		{
			node->CreateNode(name);
			for (auto & v : str) {
				auto subname = name + L"/" + (v.key.Length() ? v.key : L"_");
				node->CreateValue(subname, Storage::RegistryValueType::String);
				node->SetValue(subname, v.value);
			}
		}

		resource_tool::resource_tool(Engine::Storage::RegistryNode * node)
		{
			auto mode_string = node->GetValueString(L"Modus");
			if (string::CompareIgnoreCase(mode_string, L"incorporaliter") == 0) mode = resource_tool_mode::embed;
			else if (string::CompareIgnoreCase(mode_string, L"native") == 0) mode = resource_tool_mode::native;
			else if (string::CompareIgnoreCase(mode_string, L"sarcina") == 0) mode = resource_tool_mode::bundle;
			else mode = resource_tool_mode::none;
			create_installation_metadata = node->GetValueBoolean(L"CumMetadata");
			resource_driver_module = node->GetValueString(L"Modulus").LowerCase();
			resource_tool_command = node->GetValueString(L"Imperatum");
			output_argument = node->GetValueString(L"ArgumentumDestinationis");
			resource_object_extension = node->GetValueString(L"ExtensioDestinationis");
			SafePointer<Storage::RegistryNode> extra = node->OpenNode(L"ArgumentaExtra");
			if (extra) for (auto & e : extra->GetValues()) extra_command_line.InsertLast(extra->GetValueString(e));
			SafePointer<Storage::RegistryNode> icon = node->OpenNode(L"Icones");
			if (icon) {
				icon_codec = icon->GetValueString(L"Codificator");
				icon_extension = icon->GetValueString(L"Extensio");
				SafePointer<Storage::RegistryNode> sizes = icon->OpenNode(L"Magnitudines");
				if (sizes) for (auto & s : sizes->GetValues()) icon_sizes.InsertLast(sizes->GetValueInteger(s));
			}
		}
		bool resource_tool::prepare_icon(const string & input_path, Engine::Volumes::Dictionary<string, icon_record> & map, uint & counter, build_state * state) noexcept
		{
			try {
				if (map[input_path]) return true;
				auto index = counter++;
				auto dest_name = ExpandPath(state->project_object_path + L"/" + Path::GetFileNameWithoutExtension(state->project_file_path) + L"-icon-" + string(index) + L"." + icon_extension);
				SafePointer<FileStream> source_stream = new FileStream(input_path, AccessRead, OpenExisting);
				SafePointer<FileStream> dest_stream = new FileStream(dest_name, AccessReadWrite, OpenAlways);
				if (!dest_stream->Length() || DateTime::GetFileAlterTime(source_stream->Handle()) > DateTime::GetFileAlterTime(dest_stream->Handle())) {
					SafePointer<Codec::Image> source = Codec::DecodeImage(source_stream);
					SafePointer<Codec::Image> dest = new Codec::Image;
					for (auto & s : icon_sizes) {
						auto f = source->GetFramePreciseSize(s, s);
						if (f) {
							f->Usage = Codec::FrameUsage::ColorMap;
							f->DpiUsage = 1.0;
							f->HotPointX = f->HotPointY = 0;
							f->Duration = 0;
							dest->Frames.Append(f);
						}
					}
					if (!dest->Frames.Length()) throw InvalidFormatException();
					dest_stream->SetLength(0);
					dest_stream->Seek(0, Begin);
					Codec::EncodeImage(dest_stream, dest, icon_codec);
				}
				map.Append(input_path, icon_record { .internal_name = index, .intermediate_path = dest_name });
				return true;
			} catch (...) { return false; }
		}
		void resource_tool::process_file(const string & input, const string & mdl, const string & option, build_state * state, process_context & context)
		{
			Volumes::Dictionary<string, icon_record> icon_map;
			if (!state->idle_mode) {
				if (state->app.application_icon.Length()) {
					uint icon_counter = 1;
					if (!prepare_icon(state->app.application_icon, icon_map, icon_counter, state)) {
						context.build_status_notify(input, build_tool_status::failed, 0, 0, FormatString(state->io->localized(233), state->app.application_icon));
						return;
					}
				}
				uint icon_counter = 2;
				for (auto & ff : state->app.file_formats) if (ff.file_format_icon.Length()) {
					if (!prepare_icon(ff.file_format_icon, icon_map, icon_counter, state)) {
						context.build_status_notify(input, build_tool_status::failed, 0, 0, FormatString(state->io->localized(233), ff.file_format_icon));
						return;
					}
				}
			}
			if (mode == resource_tool_mode::embed || mode == resource_tool_mode::bundle) {
				build_tool_status exit_status = build_tool_status::built_new;
				Time time_from = 0, time_to = 0;
				if (mode == resource_tool_mode::embed) for (auto & i : icon_map) if (i.value.internal_name == 1) {
					context.enter_state_critical_section();
					try {
						state->resource_list.InsertLast(resource_task {
							.source_path = i.value.intermediate_path,
							.resource_name = L"1",
							.resource_locale = L"ICON"
						});
					} catch (...) {}
					context.leave_state_critical_section();
				}
				if (!state->resource_list.IsEmpty() || state->modules_to_build[resource_driver_module]) {
					if (!state->idle_mode) {
						Array<resource_record> resource_table(0x20);
						uint counter = 0;
						for (auto & r : state->resource_list) {
							try {
								FileStream stream(r.source_path, AccessRead, OpenExisting);
								resource_record rr;
								rr.data = stream.ReadAll();
								rr.name = r.resource_name;
								rr.locale = r.resource_locale;
								rr.id = L"__resdata_" + string(counter++, HexadecimalBase, 2);
								resource_table.Append(rr);
							} catch (...) {
								context.build_status_notify(input, build_tool_status::failed, 0, 0, FormatString(state->io->localized(226), r.source_path));
								return;
							}
						}
						dynamic_string_writer resfile;
						string resfile_path = ExpandPath(state->project_object_path + L"/auxilia-projecti.cxx");
						resfile << L"namespace ESSE {";
						for (auto & rr : resource_table) {
							resfile << L"const unsigned char " << rr.id << L"[] = {";
							write_hexadecimal(resfile, *rr.data);
							resfile << L"};";
						}
						resfile << L"unsigned int __rescnt = " << string(resource_table.Length()) << L";";
						resfile << L"const char * __resnamesA[] = {";
						for (auto & rr : resource_table) resfile << L"\"" << escape_string_c(rr.name) << L"\",";
						resfile << L"};";
						resfile << L"const char ** __resnames = __resnamesA;";
						resfile << L"const char * __reslocalesA[] = {";
						for (auto & rr : resource_table) resfile << L"\"" << escape_string_c(rr.locale) << L"\",";
						resfile << L"};";
						resfile << L"const char ** __reslocales = __reslocalesA;";
						resfile << L"const void * __resdataA[] = {";
						for (auto & rr : resource_table) resfile << rr.id << L",";
						resfile << L"};";
						resfile << L"const void ** __resdata = __resdataA;";
						resfile << L"unsigned int __reslengthsA[] = {";
						for (auto & rr : resource_table) resfile << string(rr.data->Length()) << L",";
						resfile << L"};";
						resfile << L"unsigned int * __reslengths = __reslengthsA;";
						resfile << L"}";
						SafePointer<DataBlock> resfile_data_new = resfile.finalize();
						auto resfile_data_update = true;
						if (FileExists(resfile_path)) try {
							FileStream stream(resfile_path, AccessRead, OpenExisting);
							SafePointer<DataBlock> resfile_data_old = stream.ReadAll();
							if (*resfile_data_new == *resfile_data_old) resfile_data_update = false;
						} catch (...) {}
						if (resfile_data_update) {
							try {
								FileStream stream(resfile_path, AccessWrite, CreateAlways);
								stream.WriteArray(resfile_data_new);
							} catch (...) {
								context.build_status_notify(input, build_tool_status::failed, 0, 0, FormatString(state->io->localized(227), resfile_path));
								return;
							}
						}
						auto skip_resource_tool = false;
						auto output = ExpandPath(state->project_object_path + L"/auxilia-projecti." + resource_object_extension);
						try {
							FileStream src(resfile_path, AccessRead, OpenExisting);
							FileStream out(output, AccessRead, OpenExisting);
							time_to = DateTime::GetFileAlterTime(src.Handle());
							time_from = DateTime::GetFileAlterTime(out.Handle());
							if (time_from > time_to) {
								skip_resource_tool = true;
								context.enter_state_critical_section();
								state->link_list.InsertLast(output);
								context.leave_state_critical_section();
							} else exit_status = build_tool_status::built_renew;
						} catch (...) {}
						if (!skip_resource_tool) {
							Array<string> res_args(0x80);
							res_args << resfile_path;
							for (auto & x : extra_command_line) res_args.Append(x);
							command_line_append(res_args, output_argument, output);
							SafePointer<Process> resource_process;
							SafePointer<Stream> pipe_read;
							context.enter_state_critical_section();
							try {
								SafePointer<Stream> pipe_write;
								handle log_read, log_write;
								IO::CreatePipe(&log_write, &log_read);
								try { pipe_read = new FileStream(log_read, true); } catch (...) { CloseHandle(log_read); CloseHandle(log_write); throw; }
								try { pipe_write = new FileStream(log_write, true); } catch (...) { CloseHandle(log_write); throw; }
								IO::SetStandardOutput(log_write);
								IO::SetStandardError(log_write);
								resource_process = CreateCommandProcess(resource_tool_command, &res_args);
								if (!resource_process) throw Exception();
								IO::SetStandardOutput(InvalidHandle);
								IO::SetStandardError(InvalidHandle);
							} catch (...) {
								IO::SetStandardOutput(InvalidHandle);
								IO::SetStandardError(InvalidHandle);
								context.leave_state_critical_section();
								context.build_status_notify(input, build_tool_status::failed, 0, 0, state->io->localized(228));
								return;
							}
							context.leave_state_critical_section();
							SafePointer<TextReader> reader = new TextReader(pipe_read, Encoding::UTF8);
							auto log = reader->ReadAll();
							resource_process->Wait();
							if (resource_process->GetExitCode()) {
								context.build_status_notify(input, build_tool_status::failed, 0, 0, log);
								return;
							}
							context.enter_state_critical_section();
							state->link_list.InsertLast(output);
							context.leave_state_critical_section();
						}
					}
				}
				if (mode == resource_tool_mode::embed) {
					if (create_installation_metadata && !state->idle_mode) try {
						SafePointer<Storage::Registry> meta = Storage::CreateRegistry();
						if (!meta) throw OutOfMemoryException();
						meta->CreateNode(L"Icones");
						meta->CreateNode(L"Applicatio");
						meta->CreateNode(L"FormatiLimarum");
						meta->CreateNode(L"Protocolla");
						meta->CreateNode(L"Instrumenta");
						meta->CreateNode(L"Entheca");
						for (auto i : icon_map) {
							SafePointer<Stream> source = new FileStream(i.value.intermediate_path, AccessRead, OpenExisting);
							SafePointer<DataBlock> id = source->ReadAll();
							auto in = L"Icones/" + string(i.value.internal_name);
							meta->CreateValue(in, Storage::RegistryValueType::Binary);
							meta->SetValue(in, id->GetBuffer(), id->Length());
						}
						auto app_icon_index = icon_map[state->app.application_icon];
						DynamicString locales;
						for (auto & l : state->app.localizations) { if (locales.Length()) locales += L","; locales += l; }
						meta->CreateValue(L"Applicatio/IndentitasApplicationis", Storage::RegistryValueType::String);
						meta->SetValue(L"Applicatio/IndentitasApplicationis", state->app.application_identifier);
						meta->CreateValue(L"Applicatio/IndentitasAuthoris", Storage::RegistryValueType::String);
						meta->SetValue(L"Applicatio/IndentitasAuthoris", state->app.author_identifier);
						meta->CreateValue(L"Applicatio/NomenInternum", Storage::RegistryValueType::String);
						meta->SetValue(L"Applicatio/NomenInternum", state->app.application_internal_name);
						meta->CreateValue(L"Applicatio/NomenExecutabilis", Storage::RegistryValueType::String);
						meta->SetValue(L"Applicatio/NomenExecutabilis", state->app.name);
						meta->CreateValue(L"Applicatio/Linguae", Storage::RegistryValueType::String);
						meta->SetValue(L"Applicatio/Linguae", locales.ToString());
						meta->CreateValue(L"Applicatio/Versio", Storage::RegistryValueType::String);
						meta->SetValue(L"Applicatio/Versio", state->app.application_version);
						meta->CreateValue(L"Applicatio/IndexIconis", Storage::RegistryValueType::Integer);
						if (app_icon_index) meta->SetValue(L"Applicatio/IndexIconis", int(app_icon_index->internal_name));
						meta->CreateValue(L"Applicatio/VersioMajor", Storage::RegistryValueType::Integer);
						meta->SetValue(L"Applicatio/VersioMajor", int(state->app.application_version_major));
						meta->CreateValue(L"Applicatio/VersioMinor", Storage::RegistryValueType::Integer);
						meta->SetValue(L"Applicatio/VersioMinor", int(state->app.application_version_minor));
						meta->CreateValue(L"Applicatio/VersioMicro", Storage::RegistryValueType::Integer);
						meta->SetValue(L"Applicatio/VersioMicro", int(state->app.application_version_micro));
						meta->CreateValue(L"Applicatio/NumerusStruendi", Storage::RegistryValueType::Integer);
						meta->SetValue(L"Applicatio/NumerusStruendi", int(state->app.application_build_number));
						write_localized_string(meta, L"Applicatio/Nomen", state->app.application_name);
						write_localized_string(meta, L"Applicatio/Author", state->app.application_author);
						write_localized_string(meta, L"Applicatio/JuraExempli", state->app.application_copyright);
						write_localized_string(meta, L"Applicatio/Descriptio", state->app.application_description);
						uint counter = 0;
						for (auto & ff : state->app.file_formats) {
							auto prefix = L"FormatiLimarum/" + string(++counter, DecimalBase, 4);
							auto file_format_icon_index = icon_map[ff.file_format_icon];
							meta->CreateNode(prefix);
							meta->CreateValue(prefix + L"/Extensio", Storage::RegistryValueType::String);
							meta->SetValue(prefix + L"/Extensio", ff.file_format_extension);
							meta->CreateValue(prefix + L"/IndexIconis", Storage::RegistryValueType::Integer);
							if (file_format_icon_index) meta->SetValue(prefix + L"/IndexIconis", int(file_format_icon_index->internal_name));
							meta->CreateValue(prefix + L"/Creator", Storage::RegistryValueType::Boolean);
							meta->SetValue(prefix + L"/Creator", ff.role == file_format_role::editor);
							write_localized_string(meta, prefix + L"/Descriptio", ff.file_format_description);
						}
						counter = 0;
						for (auto & us : state->app.uri_schemes) {
							auto prefix = L"Protocolla/" + string(++counter, DecimalBase, 4);
							meta->CreateNode(prefix);
							meta->CreateValue(prefix + L"/Schema", Storage::RegistryValueType::String);
							meta->SetValue(prefix + L"/Schema", us.uri_prefix);
							write_localized_string(meta, prefix + L"/Descriptio", us.uri_scheme_description);
						}
						counter = 0;
						for (auto & cli : state->app.command_line_tools) {
							auto prefix = L"Instrumenta/" + string(++counter, DecimalBase, 4);
							meta->CreateValue(prefix, Storage::RegistryValueType::String);
							meta->SetValue(prefix, cli);
						}
						meta->CreateValue(L"Entheca/LimaCommunicationis", Storage::RegistryValueType::String);
						meta->SetValue(L"Entheca/LimaCommunicationis", state->app.store_database_integration_file);
						meta->CreateValue(L"Entheca/LimaePersistae", Storage::RegistryValueType::String);
						meta->SetValue(L"Entheca/LimaePersistae", state->app.store_persistent_files);
						meta->CreateNode(L"Entheca/Extensiones");
						counter = 0;
						for (auto & sx : state->app.store_extensions) {
							auto prefix = L"Entheca/Extensiones/" + string(++counter, DecimalBase, 4);
							meta->CreateNode(prefix);
							meta->CreateValue(prefix + L"/Indentitas", Storage::RegistryValueType::String);
							meta->SetValue(prefix + L"/Indentitas", sx.target_product_identifier);
							meta->CreateValue(prefix + L"/Lima", Storage::RegistryValueType::String);
							meta->SetValue(prefix + L"/Lima", sx.extension_file);
							write_localized_string(meta, prefix + L"/Nomen", sx.name);
						}
						SafePointer<FileStream> stream = new FileStream(state->project_build_path + L"/_esse_imdt.ecsr", AccessReadWrite, CreateAlways);
						meta->Save(stream);
						try { Unix::SetFileAccessRights(stream->Handle(), Unix::AccessRightRegular, Unix::AccessRightRead, Unix::AccessRightRead); } catch (...) {}
					} catch (...) { context.build_status_notify(input, build_tool_status::failed, 0, 0, state->io->localized(234)); return; }
					context.enter_state_critical_section();
					state->output_exec_path = state->app.name;
					context.leave_state_critical_section();
					context.build_status_notify(input, exit_status, time_from, time_to, L"");
				} else if (mode == resource_tool_mode::bundle) {

					// TODO: IMPLEMENT METADATA BUILD
					
				}
			} else if (mode == resource_tool_mode::native) {
				
				// TODO: IMPLEMENT ON WINDOWS
				// Engine::Volumes::List<string> extra_command_line;
				// resource_tool_mode mode;
				// string resource_tool_command;
				// string output_argument;
				// string resource_object_extension;
				// string icon_codec, icon_extension;
				// Engine::Volumes::List<int> icon_sizes;

				// TODO: CORRECT: raw-exec-name -- bundle folder (if bundled) -- new-exec-root
				// string output_exec_path, output_bundle_path, project_build_path;

			} else {
				context.enter_state_critical_section();
				state->output_exec_path = state->app.name;
				context.leave_state_critical_section();
				context.build_status_notify(input, build_tool_status::skipped, 0, 0, L"");
			}
		}
		void resource_tool::enumerate_extensions(Engine::Array<string> & list) {}
		string resource_tool::ToString(void) const
		{
			Engine::DynamicString result;
			if (mode == resource_tool_mode::embed) result << L"Adhaesor: incorporaliter";
			else if (mode == resource_tool_mode::native) result << L"Adhaesor: native";
			else if (mode == resource_tool_mode::bundle) result << L"Adhaesor: in sarcinam";
			else result << L"Adhaesor: error";
			if (create_installation_metadata) result << L" cum metadata";
			if (resource_driver_module.Length()) result << L", pons: " << resource_driver_module;
			result << L", coprocessor: " << resource_tool_command << L" " << output_argument;
			for (auto & e : extra_command_line) result << L" " << e;
			result << L" --> ." << resource_object_extension;
			result << L", icones: ." << icon_extension << L"@" << icon_codec;
			for (auto & s : icon_sizes) result << L", x" << string(s);
			return result.ToString();
		}
	}
}