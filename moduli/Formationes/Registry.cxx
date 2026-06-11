#include "Registry.h"
#include "Language.h"
#include <math.h>

namespace ESSE
{
	namespace Formationes
	{
		namespace Format
		{
			ESSE_PACKED_STRUCTURE(EngineRegistryHeader)
				char signature[8];		// "ecs.1.0"
				uint32 signature_ex;	// 0x80000004
				uint32 version;			// 0
				uint32 data_offset;
				uint32 data_size;
				uint32 root_offset;
			ESSE_END_PACKED_STRUCTURE
		}
		class RegistryValue
		{
			friend class RegularRegistryNode;
		private:
			union
			{
				int32 _value_int32;
				int64 _value_int64;
				float _value_float;
				double _value_double;
				struct { uint8 * _value_binary; uintptr _value_binary_size; };
				uintptr _value[2];
			};
			RegistryValueType _type;
		private:
			void _deallocate(void) noexcept { if (_type == RegistryValueType::String || _type == RegistryValueType::Binary) free(_value_binary); }
		public:
			RegistryValue(void) : RegistryValue(RegistryValueType::Unknown) {}
			RegistryValue(RegistryValueType type) : _type(type) { Memory::ZeroMemory(&_value, sizeof(_value)); }
			RegistryValue(const RegistryValue & src) : _type(src._type)
			{
				if (_type == RegistryValueType::String || _type == RegistryValueType::Binary) {
					_value_binary_size = src._value_binary_size;
					_value_binary = reinterpret_cast<uint8 *>(malloc(_value_binary_size));
					if (_value_binary_size && !_value_binary) throw OutOfMemoryException();
					if (_value_binary_size) Memory::MemoryCopy(_value_binary, src._value_binary, _value_binary_size);
				} else Memory::MemoryCopy(&_value, &src._value, sizeof(src._value));
			}
			RegistryValue(RegistryValue && src) noexcept : _type(src._type)
			{
				Memory::MemoryCopy(&_value, &src._value, sizeof(src._value));
				Memory::ZeroMemory(&src._value, sizeof(src._value));
			}
			~RegistryValue(void) { _deallocate(); }
			RegistryValue & operator = (const RegistryValue & src)
			{
				if (this == &src) return *this;
				if (src._type == RegistryValueType::String || src._type == RegistryValueType::Binary) {
					auto copy = reinterpret_cast<uint8 *>(malloc(src._value_binary_size));
					if (src._value_binary_size && !copy) throw OutOfMemoryException();
					_deallocate(); _type = src._type;
					_value_binary_size = src._value_binary_size;
					_value_binary = copy;
					if (_value_binary_size) Memory::MemoryCopy(_value_binary, src._value_binary, _value_binary_size);
				} else {
					_deallocate(); _type = src._type;
					Memory::MemoryCopy(&_value, &src._value, sizeof(src._value));
				}
				return *this;
			}
			RegistryValue & operator = (RegistryValue && src) noexcept
			{
				if (this == &src) return *this;
				_deallocate(); _type = src._type;
				Memory::MemoryCopy(&_value, &src._value, sizeof(src._value));
				Memory::ZeroMemory(&src._value, sizeof(src._value));
				return *this;
			}
			void SetBinarySize(uintptr size)
			{
				if (_type == RegistryValueType::String || _type == RegistryValueType::Binary) {
					auto resized = reinterpret_cast<uint8 *>(realloc(_value_binary, size));
					if (!resized && size) throw OutOfMemoryException();
					_value_binary = resized;
					_value_binary_size = size;
				}
			}
			void Set(int32 new_value)
			{
				if (_type == RegistryValueType::Integer) _value_int32 = new_value;
				else if (_type == RegistryValueType::Float) _value_float = float(new_value);
				else if (_type == RegistryValueType::Boolean) _value_int32 = new_value ? 1 : 0;
				else if (_type == RegistryValueType::String) Set(string(new_value));
				else if (_type == RegistryValueType::LongInteger) _value_int64 = new_value;
				else if (_type == RegistryValueType::LongFloat) _value_double = double(new_value);
				else if (_type == RegistryValueType::Color) _value_int32 = new_value;
				else if (_type == RegistryValueType::Time) _value_int64 = new_value;
				else if (_type == RegistryValueType::Binary) Set(&new_value, sizeof(new_value));
			}
			void Set(float new_value)
			{
				if (_type == RegistryValueType::Integer) _value_int32 = int32(new_value);
				else if (_type == RegistryValueType::Float) _value_float = new_value;
				else if (_type == RegistryValueType::Boolean) _value_int32 = new_value ? 1 : 0;
				else if (_type == RegistryValueType::String) Set(string(new_value));
				else if (_type == RegistryValueType::LongInteger) _value_int64 = int64(new_value);
				else if (_type == RegistryValueType::LongFloat) _value_double = double(new_value);
				else if (_type == RegistryValueType::Color) _value_int32 = int32(new_value);
				else if (_type == RegistryValueType::Time) _value_int64 = int64(new_value);
				else if (_type == RegistryValueType::Binary) Set(&new_value, sizeof(new_value));
			}
			void Set(bool new_value)
			{
				if (_type == RegistryValueType::Integer) _value_int32 = new_value ? 1 : 0;
				else if (_type == RegistryValueType::Float) _value_float = new_value ? 1.0f : 0.0f;
				else if (_type == RegistryValueType::Boolean) _value_int32 = new_value ? 1 : 0;
				else if (_type == RegistryValueType::String) Set(string(new_value));
				else if (_type == RegistryValueType::LongInteger) _value_int64 = new_value ? 1 : 0;
				else if (_type == RegistryValueType::LongFloat) _value_double = new_value ? 1.0 : 0.0;
				else if (_type == RegistryValueType::Color) _value_int32 = new_value ? 1 : 0;
				else if (_type == RegistryValueType::Time) _value_int64 = new_value ? 1 : 0;
				else if (_type == RegistryValueType::Binary) Set(&new_value, sizeof(new_value));
			}
			void Set(const string & new_value)
			{
				if (_type == RegistryValueType::Integer) _value_int32 = new_value.ToInt32();
				else if (_type == RegistryValueType::Float) _value_float = new_value.ToFloat();
				else if (_type == RegistryValueType::Boolean) _value_int32 = new_value.ToBoolean();
				else if (_type == RegistryValueType::String || _type == RegistryValueType::Binary) {
					if (new_value.GetLength()) {
						auto length = (new_value.GetLength() + 1) * sizeof(unichar32);
						SetBinarySize(length);
						Memory::MemoryCopy(_value_binary, new_value.GetData(), length);
					} else SetBinarySize(0);
				} else if (_type == RegistryValueType::LongInteger) _value_int64 = new_value.ToInt64();
				else if (_type == RegistryValueType::LongFloat) _value_double = new_value.ToDouble();
				else if (_type == RegistryValueType::Color) _value_int32 = new_value.ToUInt32();
				else if (_type == RegistryValueType::Time) _value_int64 = new_value.ToUInt64();
			}
			void Set(int64 new_value)
			{
				if (_type == RegistryValueType::Integer) _value_int32 = int32(new_value);
				else if (_type == RegistryValueType::Float) _value_float = float(new_value);
				else if (_type == RegistryValueType::Boolean) _value_int32 = new_value ? 1 : 0;
				else if (_type == RegistryValueType::String) Set(string(new_value));
				else if (_type == RegistryValueType::LongInteger) _value_int64 = new_value;
				else if (_type == RegistryValueType::LongFloat) _value_double = double(new_value);
				else if (_type == RegistryValueType::Color) _value_int32 = int32(new_value);
				else if (_type == RegistryValueType::Time) _value_int64 = new_value;
				else if (_type == RegistryValueType::Binary) Set(&new_value, sizeof(new_value));
			}
			void Set(double new_value)
			{
				if (_type == RegistryValueType::Integer) _value_int32 = int32(new_value);
				else if (_type == RegistryValueType::Float) _value_float = float(new_value);
				else if (_type == RegistryValueType::Boolean) _value_int32 = new_value ? 1 : 0;
				else if (_type == RegistryValueType::String) Set(string(new_value));
				else if (_type == RegistryValueType::LongInteger) _value_int64 = int64(new_value);
				else if (_type == RegistryValueType::LongFloat) _value_double = new_value;
				else if (_type == RegistryValueType::Color) _value_int32 = int32(new_value);
				else if (_type == RegistryValueType::Time) _value_int64 = int64(new_value);
				else if (_type == RegistryValueType::Binary) Set(&new_value, sizeof(new_value));
			}
			void Set(Color new_value) { Set(int32(new_value.value)); }
			void Set(Time new_value) { Set(int64(new_value.Ticks)); }
			void Set(const void * new_value, uintptr size)
			{
				if (_type == RegistryValueType::Integer) {
					if (size >= 4) _value_int32 = *reinterpret_cast<const int32 *>(new_value);
					else _value_int32 = 0;
				} else if (_type == RegistryValueType::Float) {
					if (size >= 4) _value_float = *reinterpret_cast<const float *>(new_value);
					else _value_float = 0.0;
				} else if (_type == RegistryValueType::Boolean) {
					if (size >= 1) _value_int32 = (*reinterpret_cast<const uint8 *>(new_value)) ? 1 : 0;
					else _value_int32 = 0;
				} else if (_type == RegistryValueType::String) {
					dynamic_string_ucs4 result;
					for (uintptr i = 0; i < size; i++) result += string(uint32(reinterpret_cast<const uint8 *>(new_value)[i]), HexadecimalBase, 2);
					Set(result.ToString());
				} else if (_type == RegistryValueType::LongInteger) {
					if (size >= 8) _value_int64 = *reinterpret_cast<const int64 *>(new_value);
					else _value_int64 = 0;
				} else if (_type == RegistryValueType::LongFloat) {
					if (size >= 8) _value_double = *reinterpret_cast<const double *>(new_value);
					else _value_double = 0.0;
				} else if (_type == RegistryValueType::Color) {
					if (size >= 4) _value_int32 = *reinterpret_cast<const int32 *>(new_value);
					else _value_int32 = 0;
				} else if (_type == RegistryValueType::Time) {
					if (size >= 8) _value_int64 = *reinterpret_cast<const int64 *>(new_value);
					else _value_int64 = 0;
				} else if (_type == RegistryValueType::Binary) {
					SetBinarySize(size);
					Memory::MemoryCopy(_value_binary, new_value, size);
				}
			}
			int32 GetInteger(void) const noexcept
			{
				try {
					if (_type == RegistryValueType::Integer || _type == RegistryValueType::Boolean || _type == RegistryValueType::Color) return _value_int32;
					else if (_type == RegistryValueType::Float) return int32(_value_float);
					else if (_type == RegistryValueType::String) return _value_binary_size ? string(reinterpret_cast<const unichar32 *>(_value_binary)).ToInt32() : 0;
					else if (_type == RegistryValueType::LongInteger || _type == RegistryValueType::Time) return int32(_value_int64);
					else if (_type == RegistryValueType::LongFloat) return int32(_value_double);
					else if (_type == RegistryValueType::Binary) {
						if (_value_binary_size >= 4) return *reinterpret_cast<const int32 *>(_value_binary);
						else return 0;
					} else return 0;
				} catch (...) { return 0; }
			}
			float GetFloat(void) const noexcept
			{
				try {
					if (_type == RegistryValueType::Integer || _type == RegistryValueType::Boolean || _type == RegistryValueType::Color) return float(_value_int32);
					else if (_type == RegistryValueType::Float) return _value_float;
					else if (_type == RegistryValueType::String) return _value_binary_size ? string(reinterpret_cast<const unichar32 *>(_value_binary)).ToFloat() : 0.0f;
					else if (_type == RegistryValueType::LongInteger || _type == RegistryValueType::Time) return float(_value_int64);
					else if (_type == RegistryValueType::LongFloat) return float(_value_double);
					else if (_type == RegistryValueType::Binary) {
						if (_value_binary_size >= 4) return *reinterpret_cast<const float *>(_value_binary);
						else return 0.0f;
					} else return 0.0f;
				} catch (...) { return 0.0f; }
			}
			bool GetBoolean(void) const noexcept
			{
				try {
					if (_type == RegistryValueType::Integer || _type == RegistryValueType::Boolean || _type == RegistryValueType::Color) return _value_int32 ? true : false;
					else if (_type == RegistryValueType::Float) return _value_float ? true : false;
					else if (_type == RegistryValueType::String) return _value_binary_size ? string(reinterpret_cast<const unichar32 *>(_value_binary)).ToBoolean() : false;
					else if (_type == RegistryValueType::LongInteger || _type == RegistryValueType::Time) return _value_int64 ? true : false;
					else if (_type == RegistryValueType::LongFloat) return _value_double ? true : false;
					else if (_type == RegistryValueType::Binary) {
						if (_value_binary_size >= 1) return *reinterpret_cast<const bool *>(_value_binary);
						else return false;
					} else return false;
				} catch (...) { return false; }
			}
			string GetString(void) const
			{
				if (_type == RegistryValueType::Integer) return string(_value_int32);
				else if (_type == RegistryValueType::Float) return string(_value_float);
				else if (_type == RegistryValueType::Boolean) return string(_value_int32 ? true : false);
				else if (_type == RegistryValueType::String) return _value_binary_size ? string(reinterpret_cast<const unichar32 *>(_value_binary)) : string();
				else if (_type == RegistryValueType::LongInteger) return string(_value_int64);
				else if (_type == RegistryValueType::LongFloat) return string(_value_double);
				else if (_type == RegistryValueType::Color) {
					Color val = Color(uint32(_value_int32));
					return U"(" + string(val.r) + U", " + string(val.g) + U", " + string(val.b) + U", " + string(val.a) + U")";
				} else if (_type == RegistryValueType::Time) return Time(uint64(_value_int64)).ToString();
				else if (_type == RegistryValueType::Binary) {
					dynamic_string_ucs4 result;
					for (uintptr i = 0; i < _value_binary_size; i++) result += string(uint32(_value_binary[i]), HexadecimalBase, 2);
					return result;
				} else return 0;
			}
			int64 GetLongInteger(void) const noexcept
			{
				try {
					if (_type == RegistryValueType::Integer || _type == RegistryValueType::Boolean || _type == RegistryValueType::Color) return int64(_value_int32);
					else if (_type == RegistryValueType::Float) return int64(_value_float);
					else if (_type == RegistryValueType::String) return _value_binary_size ? string(reinterpret_cast<const unichar32 *>(_value_binary)).ToInt64() : 0;
					else if (_type == RegistryValueType::LongInteger || _type == RegistryValueType::Time) return _value_int64;
					else if (_type == RegistryValueType::LongFloat) return int64(_value_double);
					else if (_type == RegistryValueType::Binary) {
						if (_value_binary_size >= 8) return *reinterpret_cast<const int64 *>(_value_binary);
						else return 0;
					} else return 0;
				} catch (...) { return 0; }
			}
			double GetLongFloat(void) const noexcept
			{
				try {
					if (_type == RegistryValueType::Integer || _type == RegistryValueType::Boolean || _type == RegistryValueType::Color) return double(_value_int32);
					else if (_type == RegistryValueType::Float) return double(_value_float);
					else if (_type == RegistryValueType::String) return _value_binary_size ? string(reinterpret_cast<const unichar32 *>(_value_binary)).ToDouble() : 0.0;
					else if (_type == RegistryValueType::LongInteger || _type == RegistryValueType::Time) return double(_value_int64);
					else if (_type == RegistryValueType::LongFloat) return _value_double;
					else if (_type == RegistryValueType::Binary) {
						if (_value_binary_size >= 8) return *reinterpret_cast<const double *>(_value_binary);
						else return 0.0;
					} else return 0.0;
				} catch (...) { return 0.0; }
			}
			Color GetColor(void) const noexcept { return Color(uint32(GetInteger())); }
			Time GetTime(void) const noexcept { return Time(uint64(GetLongInteger())); }
			uintptr GetBinarySize() const noexcept
			{
				if (_type == RegistryValueType::Integer) return 4;
				else if (_type == RegistryValueType::Float) return 4;
				else if (_type == RegistryValueType::Boolean) return 1;
				else if (_type == RegistryValueType::String) return _value_binary_size;
				else if (_type == RegistryValueType::LongInteger) return 8;
				else if (_type == RegistryValueType::LongFloat) return 8;
				else if (_type == RegistryValueType::Color) return 4;
				else if (_type == RegistryValueType::Time) return 8;
				else if (_type == RegistryValueType::Binary) return _value_binary_size;
				else return 0;
			}
			void GetBinary(void * buffer) const noexcept
			{
				if (_type == RegistryValueType::String || _type == RegistryValueType::Binary) Memory::MemoryCopy(buffer, _value_binary, _value_binary_size);
				else Memory::MemoryCopy(buffer, &_value, GetBinarySize());
			}
		};
		class RegularRegistryNode : public RegistryNode
		{
			friend class RegularRegistry;
			struct _string_encoding_record { uint32 base_offset; uint32 length; };
		private:
			array<string> _node_names, _value_names;
			object_array<RegularRegistryNode> _nodes;
			array<RegistryValue> _values;
		private:
			static intptr _binary_search(const unichar32 * name, uintptr name_length, const array<string> & in) noexcept
			{
				if (!in.GetLength()) return -1;
				ErrorContext ectx; ErrorClear(ectx);
				uintptr min = 0;
				uintptr max = in.GetLength() - 1;
				while (max > min) {
					uintptr med = (min + max) >> 1U;
					auto cmp = Unicode::CaseInsensitiveCompare(name, name_length, in[med], in[med].GetLength(), ectx);
					if (ErrorTest(ectx)) return -1;
					if (cmp > 0) { if (med + 1 > max) return -1; min = med + 1; }
					else if (cmp < 0) { if (med < min + 1) return -1; max = med - 1; }
					else return med;
				}
				auto cmp = Unicode::CaseInsensitiveCompare(name, name_length, in[max], in[max].GetLength(), ectx);
				if (ErrorTest(ectx) || cmp) return -1; else return max;
			}
			static intptr _binary_search_ip(const unichar32 * name, uintptr name_length, const array<string> & in) noexcept
			{
				if (!in.GetLength()) return 0;
				ErrorContext ectx; ErrorClear(ectx);
				uintptr min = 0;
				uintptr max = in.GetLength();
				while (max > min) {
					uintptr med = (min + max) >> 1U;
					auto cmp = Unicode::CaseInsensitiveCompare(name, name_length, in[med], in[med].GetLength(), ectx);
					if (ErrorTest(ectx)) return -1;
					if (cmp > 0) min = med + 1;
					else if (cmp < 0) max = med;
					else return -1;
				}
				return max;
			}
			static intptr _find_delimeter(const unichar32 * path, uintptr path_length) noexcept
			{
				intptr i = path_length - 1;
				while (i >= 0) { if (path[i] == U'/' || path[i] == U'\\') break; i--; }
				return i;
			}
		public:
			RegularRegistryNode(void) noexcept : _node_names(0x10), _value_names(0x10), _nodes(0x10), _values(0x10) {}
			virtual ~RegularRegistryNode(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Registry node"; ESSE_TRY_OUTRO(string()) }
			bool FindNode(const unichar32 * name, uintptr name_length, RegularRegistryNode ** pnode, uintptr * pindex) const noexcept
			{
				auto index = _binary_search(name, name_length, _node_names);
				if (index < 0) return false;
				if (pnode) *pnode = _nodes(index);
				if (pindex) *pindex = index;
				return true;
			}
			bool FindValue(const unichar32 * name, uintptr name_length, const RegistryValue ** pvalue, uintptr * pindex) const noexcept
			{
				auto index = _binary_search(name, name_length, _value_names);
				if (index < 0) return false;
				if (pvalue) *pvalue = &_values[index];
				if (pindex) *pindex = index;
				return true;
			}
			bool FindValue(const unichar32 * name, uintptr name_length, RegistryValue ** pvalue, uintptr * pindex) noexcept
			{
				auto index = _binary_search(name, name_length, _value_names);
				if (index < 0) return false;
				if (pvalue) *pvalue = &_values[index];
				if (pindex) *pindex = index;
				return true;
			}
			RegularRegistryNode * ResolveNode(const unichar32 * path, uintptr path_length) noexcept
			{
				auto result = this;
				uint cp = 0;
				while (cp < path_length) {
					uintptr sp = cp;
					while (cp < path_length && path[cp] != U'/' && path[cp] != U'\\') cp++;
					if (cp > sp) {
						RegularRegistryNode * subnode;
						if (!result->FindNode(path + sp, cp - sp, &subnode, 0)) return 0;
						result = subnode;
					}
					cp++;
				}
				return result;
			}
			const RegularRegistryNode * ResolveNode(const unichar32 * path, uintptr path_length) const noexcept
			{
				auto result = this;
				uint cp = 0;
				while (cp < path_length) {
					uintptr sp = cp;
					while (cp < path_length && path[cp] != U'/' && path[cp] != U'\\') cp++;
					if (cp > sp) {
						RegularRegistryNode * subnode;
						if (!result->FindNode(path + sp, cp - sp, &subnode, 0)) return 0;
						result = subnode;
					}
					cp++;
				}
				return result;
			}
			RegistryValue * ResolveValue(const string & path) noexcept
			{
				auto i = _find_delimeter(path.GetData(), path.GetLength());
				if (i >= 0) {
					RegistryValue * value;
					auto node = ResolveNode(path.GetData(), i);
					if (!node) return 0;
					if (!node->FindValue(path.GetData() + i, path.GetLength() - i, &value, 0)) return 0;
					return value;
				} else {
					RegistryValue * value;
					if (!FindValue(path.GetData(), path.GetLength(), &value, 0)) return 0;
					return value;
				}
			}
			const RegistryValue * ResolveValue(const string & path) const noexcept
			{
				auto i = _find_delimeter(path.GetData(), path.GetLength());
				if (i >= 0) {
					const RegistryValue * value;
					auto node = ResolveNode(path.GetData(), i);
					if (!node) return 0;
					if (!node->FindValue(path.GetData() + i, path.GetLength() - i, &value, 0)) return 0;
					return value;
				} else {
					const RegistryValue * value;
					if (!FindValue(path.GetData(), path.GetLength(), &value, 0)) return 0;
					return value;
				}
			}
			uintptr InternalCreateNode(const unichar32 * name, uintptr name_length, RegularRegistryNode * insert = 0)
			{
				if (!name_length) throw InputOutputException(Errores::SuberrorIO::BadPathName);
				auto position = _binary_search_ip(name, name_length, _node_names);
				if (position < 0) throw InputOutputException(Errores::SuberrorIO::FileExists);
				if (insert) _nodes.Insert(insert, position);
				else _nodes.Insert(owrap(new RegularRegistryNode), position);
				try { _node_names.Insert(string(name, name_length), position); } catch (...) { _nodes.Remove(position); throw; }
				return position;
			}
			uintptr InternalCreateValue(const unichar32 * name, uintptr name_length, RegistryValueType type)
			{
				if (!name_length) throw InputOutputException(Errores::SuberrorIO::BadPathName);
				if (uint(type) < 1 || uint(type) > 9) throw InvalidArgumentException();
				auto position = _binary_search_ip(name, name_length, _value_names);
				if (position < 0) throw InputOutputException(Errores::SuberrorIO::FileExists);
				_values.Insert(RegistryValue(type), position);
				try { _value_names.Insert(string(name, name_length), position); } catch (...) { _values.Remove(position); throw; }
				return position;
			}
			virtual const array<string> & GetSubnodes(void) const noexcept override { return _node_names; }
			virtual const array<string> & GetValues(void) const noexcept override { return _value_names; }
			virtual void CreateNode(const string & path) override
			{
				auto del = _find_delimeter(path.GetData(), path.GetLength());
				auto node = del >= 0 ? ResolveNode(path.GetData(), del) : this;
				if (!node) throw InputOutputException(Errores::SuberrorIO::PathNotFound);
				node->InternalCreateNode(path.GetData() + 1 + del, path.GetLength() - 1 - del);
			}
			virtual void RemoveNode(const string & path) override
			{
				auto del = _find_delimeter(path.GetData(), path.GetLength());
				auto node = del >= 0 ? ResolveNode(path.GetData(), del) : this;
				if (!node) throw InputOutputException(Errores::SuberrorIO::PathNotFound);
				uintptr index;
				if (!node->FindNode(path.GetData() + 1 + del, path.GetLength() - 1 - del, 0, &index)) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				node->_nodes.Remove(index);
				node->_node_names.Remove(index);
			}
			virtual void RenameNode(const string & node, const string & name) override
			{
				auto del_src = _find_delimeter(node.GetData(), node.GetLength());
				auto del_new = _find_delimeter(name.GetData(), name.GetLength());
				auto node_src = del_src >= 0 ? ResolveNode(node.GetData(), del_src) : this;
				auto node_dst = del_new >= 0 ? ResolveNode(name.GetData(), del_new) : this;
				if (!node_src || !node_dst) throw InputOutputException(Errores::SuberrorIO::PathNotFound);
				auto remove_at = _binary_search(node.GetData() + 1 + del_src, node.GetLength() - 1 - del_src, node_src->_node_names);
				if (remove_at < 0) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				if (node_src == node_dst) {
					auto new_name = name.Substring(del_new + 1, -1);
					auto insert_at = _binary_search_ip(name.GetData() + 1 + del_new, name.GetLength() - 1 - del_new, node_dst->_node_names);
					if (insert_at < 0) {
						insert_at = _binary_search(name.GetData() + 1 + del_new, name.GetLength() - 1 - del_new, node_dst->_node_names);
						if (insert_at != remove_at) throw InputOutputException(Errores::SuberrorIO::FileExists);
						insert_at++;
					}
					insert_at--;
					while (insert_at < remove_at) {
						node_src->_nodes.SwapAt(remove_at, remove_at - 1);
						node_src->_node_names.SwapAt(remove_at, remove_at - 1);
						remove_at--;
					}
					while (insert_at > remove_at) {
						node_src->_nodes.SwapAt(remove_at, remove_at + 1);
						node_src->_node_names.SwapAt(remove_at, remove_at + 1);
						remove_at++;
					}
					node_src->_node_names[remove_at] = static_cast<string &&>(new_name);
				} else {
					auto insert_at = _binary_search_ip(name.GetData() + 1 + del_new, name.GetLength() - 1 - del_new, node_dst->_node_names);
					if (insert_at < 0) throw InputOutputException(Errores::SuberrorIO::FileExists);
					oref<RegularRegistryNode> move = node_src->_nodes(remove_at);
					node_dst->_nodes.Insert(move, insert_at);
					try { node_dst->_node_names.Insert(name.Substring(del_new + 1, -1), insert_at); } catch (...) { node_dst->_nodes.Remove(insert_at); throw; }
					node_src->_nodes.Remove(remove_at);
					node_src->_node_names.Remove(remove_at);
				}
			}
			virtual oref<RegistryNode> OpenNode(const string & path) override { return oref<RegistryNode>(ResolveNode(path.GetData(), path.GetLength())); }
			virtual void CreateValue(const string & path, RegistryValueType type) override
			{
				auto del = _find_delimeter(path.GetData(), path.GetLength());
				auto node = del >= 0 ? ResolveNode(path.GetData(), del) : this;
				if (!node) throw InputOutputException(Errores::SuberrorIO::PathNotFound);
				node->InternalCreateValue(path.GetData() + 1 + del, path.GetLength() - 1 - del, type);
			}
			virtual void RemoveValue(const string & path) override
			{
				auto del = _find_delimeter(path.GetData(), path.GetLength());
				auto node = del >= 0 ? ResolveNode(path.GetData(), del) : this;
				if (!node) throw InputOutputException(Errores::SuberrorIO::PathNotFound);
				uintptr index;
				if (!node->FindValue(path.GetData() + 1 + del, path.GetLength() - 1 - del, 0, &index)) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				node->_values.Remove(index);
				node->_value_names.Remove(index);
			}
			virtual void RenameValue(const string & name_from, const string & name_to) override
			{
				auto del_src = _find_delimeter(name_from.GetData(), name_from.GetLength());
				auto del_new = _find_delimeter(name_to.GetData(), name_to.GetLength());
				auto node_src = del_src >= 0 ? ResolveNode(name_from.GetData(), del_src) : this;
				auto node_dst = del_new >= 0 ? ResolveNode(name_to.GetData(), del_new) : this;
				if (!node_src || !node_dst) throw InputOutputException(Errores::SuberrorIO::PathNotFound);
				auto remove_at = _binary_search(name_from.GetData() + 1 + del_src, name_from.GetLength() - 1 - del_src, node_src->_value_names);
				if (remove_at < 0) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				if (node_src == node_dst) {
					auto new_name = name_to.Substring(del_new + 1, -1);
					auto insert_at = _binary_search_ip(name_to.GetData() + 1 + del_new, name_to.GetLength() - 1 - del_new, node_dst->_value_names);
					if (insert_at < 0) {
						insert_at = _binary_search(name_to.GetData() + 1 + del_new, name_to.GetLength() - 1 - del_new, node_dst->_value_names);
						if (insert_at != remove_at) throw InputOutputException(Errores::SuberrorIO::FileExists);
						insert_at++;
					}
					insert_at--;
					while (insert_at < remove_at) {
						node_src->_values.SwapAt(remove_at, remove_at - 1);
						node_src->_value_names.SwapAt(remove_at, remove_at - 1);
						remove_at--;
					}
					while (insert_at > remove_at) {
						node_src->_values.SwapAt(remove_at, remove_at + 1);
						node_src->_value_names.SwapAt(remove_at, remove_at + 1);
						remove_at++;
					}
					node_src->_value_names[remove_at] = static_cast<string &&>(new_name);
				} else {
					auto insert_at = _binary_search_ip(name_to.GetData() + 1 + del_new, name_to.GetLength() - 1 - del_new, node_dst->_value_names);
					if (insert_at < 0) throw InputOutputException(Errores::SuberrorIO::FileExists);
					auto move = node_src->_values[remove_at];
					node_dst->_values.Insert(move, insert_at);
					try { node_dst->_value_names.Insert(name_to.Substring(del_new + 1, -1), insert_at); } catch (...) { node_dst->_values.Remove(insert_at); throw; }
					node_src->_values.Remove(remove_at);
					node_src->_value_names.Remove(remove_at);
				}
			}
			virtual RegistryValueType GetValueType(const string & path) const override { auto value = ResolveValue(path); return value ? value->_type : RegistryValueType::Unknown; }
			virtual int32 GetValueInteger(const string & path) const override
			{
				auto value = ResolveValue(path);
				return value ? value->GetInteger() : 0;
			}
			virtual float GetValueFloat(const string & path) const override
			{
				auto value = ResolveValue(path);
				return value ? value->GetFloat() : 0.0f;
			}
			virtual bool GetValueBoolean(const string & path) const override
			{
				auto value = ResolveValue(path);
				return value ? value->GetBoolean() : false;
			}
			virtual string GetValueString(const string & path) const override
			{
				auto value = ResolveValue(path);
				return value ? value->GetString() : string();
			}
			virtual int64 GetValueLongInteger(const string & path) const override
			{
				auto value = ResolveValue(path);
				return value ? value->GetLongInteger() : 0;
			}
			virtual double GetValueLongFloat(const string & path) const override
			{
				auto value = ResolveValue(path);
				return value ? value->GetLongFloat() : 0.0;
			}
			virtual Color GetValueColor(const string & path) const override
			{
				auto value = ResolveValue(path);
				return value ? value->GetColor() : Color(0);
			}
			virtual Time GetValueTime(const string & path) const override
			{
				auto value = ResolveValue(path);
				return value ? value->GetTime() : Time(0);
			}
			virtual void GetValueBinary(const string & path, void * buffer) const override
			{
				auto value = ResolveValue(path);
				if (value) value->GetBinary(buffer);
			}
			virtual uintptr GetValueBinarySize(const string & path) const override
			{
				auto value = ResolveValue(path);
				return value ? value->GetBinarySize() : 0;
			}
			virtual void SetValue(const string & path, int32 value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(value);
			}
			virtual void SetValue(const string & path, float value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(value);
			}
			virtual void SetValue(const string & path, bool value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(value);
			}
			virtual void SetValue(const string & path, const string & value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(value);
			}
			virtual void SetValue(const string & path, const unichar32 * value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(string(value));
			}
			virtual void SetValue(const string & path, const unichar16 * value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(string(value));
			}
			virtual void SetValue(const string & path, const unichar8 * value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(string(value));
			}
			virtual void SetValue(const string & path, int64 value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(value);
			}
			virtual void SetValue(const string & path, double value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(value);
			}
			virtual void SetValue(const string & path, Color value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(value);
			}
			virtual void SetValue(const string & path, Time value) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(value);
			}
			virtual void SetValue(const string & path, const void * value, uintptr size) override
			{
				auto store = ResolveValue(path);
				if (!store) throw InputOutputException(Errores::SuberrorIO::FileNotFound);
				store->Set(value, size);
			}
			static void ValidateMemoryRange(uintptr size, uintptr data_at, uintptr of_length) { if (data_at > size || size - data_at < of_length) throw InvalidFormatException(); }
			static void ValidateMemoryString(const uint8 * data, uintptr size, uintptr data_at)
			{
				if (data_at > size || size - data_at < 2) throw InvalidFormatException();
				auto pos = data_at;
				while (size - pos >= 2 && *reinterpret_cast<const uint16 *>(data + pos)) pos += 2;
				if (size - pos < 2) throw InvalidFormatException();
			}
			void InitializeFromMemoryData(const uint8 * data, uintptr size, uintptr data_at)
			{
				if (data_at > size || size - data_at < 8) throw InvalidFormatException();
				uint32 node_count, value_count;
				node_count = *reinterpret_cast<const uint32 *>(data + data_at);
				value_count = *reinterpret_cast<const uint32 *>(data + data_at + 4);
				if (size - data_at < 8 + 8 * node_count + 4 * value_count) throw InvalidFormatException();
				for (uint i = 0; i < node_count; i++) {
					uint32 node_offset = *reinterpret_cast<const uint32 *>(data + data_at + 8 + 4 * i);
					uint32 node_name_offset = *reinterpret_cast<const uint32 *>(data + data_at + 8 + 4 * node_count + 4 * i);
					ValidateMemoryString(data, size, node_name_offset);
					auto subnode = owrap(new RegularRegistryNode);
					subnode->InitializeFromMemoryData(data, size, node_offset);
					try {
						auto node_name = string(data + node_name_offset, -1, Unicode::Encoding::UTF16_LE);
						InternalCreateNode(node_name.GetData(), node_name.GetLength(), subnode);
					} catch (...) { throw InvalidFormatException(); }
				}
				for (uint i = 0; i < value_count; i++) {
					int32 value_offset = *reinterpret_cast<const int32 *>(data + data_at + 8 + 8 * node_count + 4 * i);
					ValidateMemoryRange(size, value_offset, 8);
					int32 value_name_offset = *reinterpret_cast<const int32 *>(data + value_offset);
					int32 value_type_code = *reinterpret_cast<const int32 *>(data + value_offset + 4);
					ValidateMemoryString(data, size, value_name_offset);
					uintptr index;
					try {
						auto value_name = string(data + value_name_offset, -1, Unicode::Encoding::UTF16_LE);
						index = InternalCreateValue(value_name.GetData(), value_name.GetLength(), static_cast<RegistryValueType>(value_type_code));
					} catch (...) { throw InvalidFormatException(); }
					auto & value = _values[index];
					if (value._type == RegistryValueType::Integer || value._type == RegistryValueType::Float || value._type == RegistryValueType::Boolean || value._type == RegistryValueType::Color) {
						ValidateMemoryRange(size, value_offset, 12);
						int32 short_value = *reinterpret_cast<const int32 *>(data + value_offset + 8);
						value._value_int32 = short_value;
					} else if (value._type == RegistryValueType::LongInteger || value._type == RegistryValueType::Time || value._type == RegistryValueType::LongFloat) {
						ValidateMemoryRange(size, value_offset, 16);
						int64 long_value = *reinterpret_cast<const int64 *>(data + value_offset + 8);
						value._value_int64 = long_value;
					} else if (value._type == RegistryValueType::String) {
						ValidateMemoryRange(size, value_offset, 12);
						uint32 offset = *reinterpret_cast<const uint32 *>(data + value_offset + 8);
						ValidateMemoryString(data, size, offset);
						value.Set(string(data + offset, -1, Unicode::Encoding::UTF16_LE));
					} else if (value._type == RegistryValueType::Binary) {
						ValidateMemoryRange(size, value_offset, 16);
						uint32 size_bin = *reinterpret_cast<const uint32 *>(data + value_offset + 8);
						uint32 offset = *reinterpret_cast<const uint32 *>(data + value_offset + 12);
						ValidateMemoryRange(size, offset, size_bin);
						value.Set(data + offset, size_bin);
					} else value._type = RegistryValueType::Unknown;
				}
			}
			void InitializeFromNode(const RegularRegistryNode * src)
			{
				for (uintptr i = 0; i < src->_node_names.GetLength(); i++) {
					_node_names << src->_node_names[i];
					auto node = owrap(new RegularRegistryNode);
					node->InitializeFromNode(src->_nodes(i));
					_nodes.Append(node);
				}
				for (uintptr i = 0; i < src->_value_names.GetLength(); i++) {
					_value_names << src->_value_names[i];
					_values << src->_values[i];
				}
			}
			static void WriteString(MemoryStream * stream, array<_string_encoding_record> & serec, const uint8 * string, uint32 length, uint32 & result_offset)
			{
				for (auto & o : serec) {
					if (o.length < length) continue;
					bool match = true;
					for (uint32 j = 0; j < length; j++) { if (string[j] != reinterpret_cast<uint8 *>(stream->GetData())[o.base_offset + o.length - length + j]) { match = false; break; } }
					if (match) { result_offset = o.base_offset + o.length - length; return; }
				}
				auto origin = stream->Seek(0, SeekOrigin::Current);
				if (origin & 0xFFFFFFFF00000000) throw OutOfMemoryException();
				result_offset = uint32(origin);
				stream->Write(string, length);
				serec << _string_encoding_record { .base_offset = result_offset, .length = length };
			}
			uint32 Serialize(MemoryStream * stream, array<_string_encoding_record> & serec) const
			{
				uint32 node_base = uint32(stream->Seek(0, SeekOrigin::Current));
				uint32 node_count = _node_names.GetLength();
				uint32 value_count = _value_names.GetLength();
				array<uint32> node_data_offset(1);
				array<uint32> node_name_offset(1);
				array<uint32> value_data_offset(1);
				node_data_offset.SetLength(node_count);
				node_name_offset.SetLength(node_count);
				value_data_offset.SetLength(value_count);
				Memory::ZeroMemory(node_data_offset.GetBuffer(), node_count * 4);
				Memory::ZeroMemory(node_name_offset.GetBuffer(), node_count * 4);
				Memory::ZeroMemory(value_data_offset.GetBuffer(), value_count * 4);
				stream->Write(&node_count, 4);
				stream->Write(&value_count, 4);
				stream->Write(node_data_offset.GetBuffer(), node_count * 4);
				stream->Write(node_name_offset.GetBuffer(), node_count * 4);
				stream->Write(value_data_offset.GetBuffer(), value_count * 4);
				for (uintptr i = 0; i < _nodes.GetLength(); i++) {
					node_data_offset[i] = _nodes[i].Serialize(stream, serec);
				}
				for (uintptr i = 0; i < _node_names.GetLength(); i++) {
					auto name = ucs2_string(_node_names[i]);
					WriteString(stream, serec, reinterpret_cast<const uint8 *>(name.GetData()), (name.GetLength() + 1) * 2, node_name_offset[i]);
				}
				for (uintptr i = 0; i < _values.GetLength(); i++) {
					auto name = ucs2_string(_value_names[i]);
					uint32 name_offset = 0;
					WriteString(stream, serec, reinterpret_cast<const uint8 *>(name.GetData()), (name.GetLength() + 1) * 2, name_offset);
					uint32 data_offset = 0;
					uint32 type = uint32(_values[i]._type);
					if (_values[i]._type == RegistryValueType::Binary) {
						data_offset = uint32(stream->Seek(0, SeekOrigin::Current));
						stream->Write(_values[i]._value_binary, uint32(_values[i]._value_binary_size));
					} else if (_values[i]._type == RegistryValueType::String) {
						if (_values[i]._value_binary_size == 0) {
							uint16 word = 0;
							WriteString(stream, serec, reinterpret_cast<uint8 *>(&word), 2, data_offset);
						} else {
							auto value_utf16 = ucs2_string(reinterpret_cast<const unichar32 *>(_values[i]._value_binary));
							WriteString(stream, serec, reinterpret_cast<const uint8 *>(value_utf16.GetData()), (value_utf16.GetLength() + 1) * 2, data_offset);
						}
					}
					value_data_offset[i] = uint32(stream->Seek(0, SeekOrigin::Current));
					stream->Write(&name_offset, 4);
					stream->Write(&type, 4);
					if (_values[i]._type == RegistryValueType::Integer || _values[i]._type == RegistryValueType::Float || _values[i]._type == RegistryValueType::Boolean || _values[i]._type == RegistryValueType::Color) {
						stream->Write(&_values[i]._value_int32, 4);
					} else if (_values[i]._type == RegistryValueType::LongInteger || _values[i]._type == RegistryValueType::Time || _values[i]._type == RegistryValueType::LongFloat) {
						stream->Write(&_values[i]._value_int64, 8);
					} else if (_values[i]._type == RegistryValueType::String) {
						stream->Write(&data_offset, 4);
					} else if (_values[i]._type == RegistryValueType::Binary) {
						stream->Write(&_values[i]._value_binary_size, 4);
						stream->Write(&data_offset, 4);
					}
				}
				auto rev = stream->Seek(0, SeekOrigin::Current);
				stream->Seek(node_base + 8, SeekOrigin::Begin);
				stream->Write(node_data_offset.GetBuffer(), node_count * 4);
				stream->Write(node_name_offset.GetBuffer(), node_count * 4);
				stream->Write(value_data_offset.GetBuffer(), value_count * 4);
				stream->Seek(rev, SeekOrigin::Begin);
				return node_base;
			}
		};
		class RegularRegistry : public Registry
		{
			oref<RegularRegistryNode> _root;
		public:
			RegularRegistry(void) { _root = owrap(new RegularRegistryNode); }
			RegularRegistry(RegularRegistryNode * root) noexcept : _root(root) {}
			virtual ~RegularRegistry(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Registry"; ESSE_TRY_OUTRO(string()) }
			virtual const array<string> & GetSubnodes(void) const noexcept override { return _root->GetSubnodes(); }
			virtual const array<string> & GetValues(void) const noexcept override { return _root->GetValues(); }
			virtual void CreateNode(const string & path) override { _root->CreateNode(path); }
			virtual void RemoveNode(const string & path) override { _root->RemoveNode(path); }
			virtual void RenameNode(const string & node, const string & name) override { _root->RenameNode(node, name); }
			virtual oref<RegistryNode> OpenNode(const string & path) override { return _root->OpenNode(path); }
			virtual void CreateValue(const string & path, RegistryValueType type) override { _root->CreateValue(path, type); }
			virtual void RemoveValue(const string & path) override { _root->RemoveValue(path); }
			virtual void RenameValue(const string & name_from, const string & name_to) override { _root->RenameValue(name_from, name_to); }
			virtual RegistryValueType GetValueType(const string & path) const override { return _root->GetValueType(path); }
			virtual int32 GetValueInteger(const string & path) const override { return _root->GetValueInteger(path); }
			virtual float GetValueFloat(const string & path) const override { return _root->GetValueFloat(path); }
			virtual bool GetValueBoolean(const string & path) const override { return _root->GetValueBoolean(path); }
			virtual string GetValueString(const string & path) const override { return _root->GetValueString(path); }
			virtual int64 GetValueLongInteger(const string & path) const override { return _root->GetValueLongInteger(path); }
			virtual double GetValueLongFloat(const string & path) const override { return _root->GetValueLongFloat(path); }
			virtual Color GetValueColor(const string & path) const override { return _root->GetValueColor(path); }
			virtual Time GetValueTime(const string & path) const override { return _root->GetValueTime(path); }
			virtual void GetValueBinary(const string & path, void * buffer) const override { _root->GetValueBinary(path, buffer); }
			virtual uintptr GetValueBinarySize(const string & path) const override { return _root->GetValueBinarySize(path); }
			virtual void SetValue(const string & path, int32 value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, float value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, bool value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, const string & value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, const unichar32 * value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, const unichar16 * value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, const unichar8 * value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, int64 value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, double value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, Color value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, Time value) override { _root->SetValue(path, value); }
			virtual void SetValue(const string & path, const void * value, uintptr size) override { _root->SetValue(path, value, size); }
			virtual oref<Registry> Clone(void) const override { return Registry::Create(_root); }
			virtual void Save(Stream * stream) const override
			{
				auto buffer = MemoryStream::Create(0x10000);
				array<RegularRegistryNode::_string_encoding_record> serec(0x100);
				auto root_offset = _root->Serialize(buffer, serec);
				if (buffer->GetLength() & 0xFFFFFFFF00000000) throw OutOfMemoryException();
				Format::EngineRegistryHeader hdr;
				Memory::MemoryCopy(hdr.signature, "ecs.1.0", 8);
				hdr.signature_ex = 0x80000004;
				hdr.version = 0;
				hdr.data_offset = sizeof(hdr);
				hdr.data_size = buffer->GetLength();
				hdr.root_offset = root_offset;
				buffer->Seek(0, SeekOrigin::Begin);
				stream->Write(&hdr, sizeof(hdr));
				buffer->CopyToUntilEof(stream);
			}
			template <class O> static void EncodeRegistryNodeToText(RegistryNode * node, O & output, string prefix, Unicode::Encoding mode)
			{
				auto & values = node->GetValues();
				for (uintptr i = 0; i < values.GetLength(); i++) {
					string name = values[i];
					bool illegal = false;
					if (name[0] >= U'0' && name[0] <= U'9') illegal = true;
					else for (uintptr i = 0; i < name.GetLength(); i++) { if ((name[i] < U'a' || name[i] > U'z') && (name[i] < U'A' || name[i] > U'Z') && (name[i] < U'0' || name[i] > U'9') && name[i] != U'_') { illegal = true; break; } }
					if (illegal) name = CreateToken(name, mode);
					auto type = node->GetValueType(values[i]);
					if (type == RegistryValueType::Integer) {
						auto value = node->GetValueInteger(values[i]);
						if (value >= 0) output << prefix + name + U" = " + CreateToken(uint64(value)) + IO::LineFeedSequence;
						else output << prefix + name + U" = -" + CreateToken(uint64(-value)) + IO::LineFeedSequence;
					} else if (type == RegistryValueType::Float) {
						auto value = node->GetValueFloat(values[i]);
						if (isnan(value)) output << prefix + name + U" = nan" + IO::LineFeedSequence;
						else if (!isfinite(value)) {
							if (value > 0.0) output << prefix + name + U" = +infinity" + IO::LineFeedSequence;
							else output << prefix + name + U" = -infinity" + IO::LineFeedSequence;
						} else output << prefix + name + U" = " + CreateToken(double(value)) + IO::LineFeedSequence;
					} else if (type == RegistryValueType::Boolean) {
						auto notation = node->GetValueBoolean(values[i]) ? U"true" : U"false";
						output << prefix + name + U" = " + notation + IO::LineFeedSequence;
					} else if (type == RegistryValueType::String) {
						auto notation = CreateToken(node->GetValueString(values[i]), mode);
						output << prefix + name + U" = " + notation + IO::LineFeedSequence;
					} else if (type == RegistryValueType::LongInteger) {
						auto value = node->GetValueLongInteger(values[i]);
						if (value >= 0) output << prefix + name + U" = long " + CreateToken(uint64(value)) + IO::LineFeedSequence;
						else output << prefix + name + U" = long -" + CreateToken(uint64(-value)) + IO::LineFeedSequence;
					} else if (type == RegistryValueType::LongFloat) {
						auto value = node->GetValueLongFloat(values[i]);
						if (isnan(value)) output << prefix + name + U" = long nan" + IO::LineFeedSequence;
						else if (!isfinite(value)) {
							if (value > 0.0) output << prefix + name + U" = long +infinity" + IO::LineFeedSequence;
							else output << prefix + name + U" = long -infinity" + IO::LineFeedSequence;
						} else output << prefix + name + U" = long " + CreateToken(double(value)) + IO::LineFeedSequence;
					} else if (type == RegistryValueType::Color) {
						output << prefix + name + U" = color 0x" + string(node->GetValueColor(values[i]), HexadecimalBase, 8) + IO::LineFeedSequence;
					} else if (type == RegistryValueType::Time) {
						auto value = node->GetValueTime(values[i]);
						uint32 y, m, d;
						value.GetDate(y, m, d);
						output << prefix + name + U" = time { " +
							string(y) + U" " + string(m, DecimalBase, 2) + U" " + string(d, DecimalBase, 2) + U" " +
							string(value.GetHour(), DecimalBase, 2) + U" " + string(value.GetMinute(), DecimalBase, 2) + U" " +
							string(value.GetSecond(), DecimalBase, 2) + U" " + string(value.GetMillisecond(), DecimalBase, 4) +
							U" }" + IO::LineFeedSequence;
					} else if (type == RegistryValueType::Binary) {
						output << prefix + name + U" = binary {";
						array<uint8> data;
						data.SetLength(node->GetValueBinarySize(values[i]));
						node->GetValueBinary(values[i], data.GetBuffer());
						for (uintptr i = 0; i < data.GetLength(); i++) {
							if (i % 16 == 0) output << IO::LineFeedSequence + prefix + U"\t"; else output << U" ";
							output << U"0x" + string(uint32(data[i]), HexadecimalBase, 2);
						}
						if (data.GetLength()) output << IO::LineFeedSequence;
						output << prefix + U"}" + IO::LineFeedSequence;
					}
				}
				auto & nodes = node->GetSubnodes();
				for (uintptr i = 0; i < nodes.GetLength(); i++) {
					string name = nodes[i];
					bool illegal = false;
					if (name[0] >= U'0' && name[0] <= U'9') illegal = true;
					else for (uintptr i = 0; i < name.GetLength(); i++) { if ((name[i] < U'a' || name[i] > U'z') && (name[i] < U'A' || name[i] > U'Z') && (name[i] < U'0' || name[i] > U'9') && name[i] != U'_') { illegal = true; break; } }
					if (illegal) name = CreateToken(name, mode);
					auto entry = node->OpenNode(nodes[i]);
					output << prefix + name + U" {" + IO::LineFeedSequence;
					EncodeRegistryNodeToText(entry, output, prefix + U"\t", mode);
					output << prefix + U"}" + IO::LineFeedSequence;
				}
			}
			virtual void SaveToText(Stream * stream) const override
			{
				if (!stream) throw InvalidArgumentException();
				auto encoder = owrap(new TextEncoder(stream, Unicode::Encoding::UTF16));
				encoder->WriteEncodingSignature();
				dynamic_string_ucs4 result;
				EncodeRegistryNodeToText(_root, result, U"", Unicode::Encoding::UTF16);
				encoder->Write(result);
			}
			virtual void SaveToText(Stream * stream, Unicode::Encoding enc) const override
			{
				if (!stream) throw InvalidArgumentException();
				auto encoder = owrap(new TextEncoder(stream, enc));
				encoder->WriteEncodingSignature();
				dynamic_string_ucs4 result;
				EncodeRegistryNodeToText(_root, result, U"", enc);
				encoder->Write(result);
			}
			virtual void SaveToText(Stream * stream, const Unicode::EncodingCodepage & cp) const override
			{
				if (!stream) throw InvalidArgumentException();
				auto encoder = owrap(new TextEncoder(stream, cp));
				encoder->WriteEncodingSignature();
				dynamic_string_ucs4 result;
				EncodeRegistryNodeToText(_root, result, U"", Unicode::Encoding::ASCII);
				encoder->Write(result);
			}
			virtual void SaveToText(ITextEncoder * enc, bool escape_unicode) const override
			{
				if (!enc) throw InvalidArgumentException();
				dynamic_string_ucs4 result;
				EncodeRegistryNodeToText(_root, result, U"", escape_unicode ? Unicode::Encoding::ASCII : Unicode::Encoding::UTF32);
				enc->Write(result);
			}
			virtual string SaveToText(void) const override
			{
				dynamic_string_ucs4 result;
				EncodeRegistryNodeToText(_root, result, U"", Unicode::Encoding::UTF32);
				return result;
			}
		};
		class MergedRegistryNode : public RegistryNode
		{
			object_array<RegistryNode> _nodes;
			array<string> _value_name_cache, _node_name_cache;
		private:
			void _build_cache(void)
			{
				for (auto & node : _nodes) {
					auto & vals = node.GetValues();
					auto & nodes = node.GetSubnodes();
					for (auto & e : vals) {
						bool present = false;
						for (auto & f : _value_name_cache) if (string::CompareCaseInsensitively(f, e) == 0) { present = true; break; }
						if (!present) _value_name_cache.Append(e);
					}
					for (auto & e : nodes) {
						bool present = false;
						for (auto & f : _node_name_cache) if (string::CompareCaseInsensitively(f, e) == 0) { present = true; break; }
						if (!present) _node_name_cache.Append(e);
					}
				}
			}
		public:
			MergedRegistryNode(RegistryNode ** nodes, uintptr count) : _nodes(1), _value_name_cache(0x20), _node_name_cache(0x20)
			{
				_nodes.SetLength(count);
				for (uintptr i = 0; i < count; i++) _nodes.SetElement(nodes[i], i);
				_build_cache();
			}
			virtual ~MergedRegistryNode(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Merged registry node"; ESSE_TRY_OUTRO(string()) }
			virtual const array<string> & GetSubnodes(void) const noexcept override { return _node_name_cache; }
			virtual const array<string> & GetValues(void) const noexcept override { return _value_name_cache; }
			virtual void CreateNode(const string & path) override { throw NotImplementedException(); }
			virtual void RemoveNode(const string & path) override { throw NotImplementedException(); }
			virtual void RenameNode(const string & node, const string & name) override { throw NotImplementedException(); }
			virtual oref<RegistryNode> OpenNode(const string & path) override
			{
				object_array<RegistryNode> subnodes(0x10);
				for (auto & node : _nodes) { auto local = node.OpenNode(path); if (local) subnodes.Append(local); }
				if (subnodes.GetLength()) return RegistryNode::Merge(subnodes.GetBuffer(), subnodes.GetLength());
				else return 0;
			}
			virtual void CreateValue(const string & path, RegistryValueType type) override { throw NotImplementedException(); }
			virtual void RemoveValue(const string & path) override { throw NotImplementedException(); }
			virtual void RenameValue(const string & name_from, const string & name_to) override { throw NotImplementedException(); }
			virtual RegistryValueType GetValueType(const string & path) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) return type;
				}
				return RegistryValueType::Unknown;
			}
			virtual int32 GetValueInteger(const string & path) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) return node.GetValueInteger(path);
				}
				return 0;
			}
			virtual float GetValueFloat(const string & path) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) return node.GetValueFloat(path);
				}
				return 0.0f;
			}
			virtual bool GetValueBoolean(const string & path) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) return node.GetValueBoolean(path);
				}
				return false;
			}
			virtual string GetValueString(const string & path) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) return node.GetValueString(path);
				}
				return string();
			}
			virtual int64 GetValueLongInteger(const string & path) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) return node.GetValueLongInteger(path);
				}
				return 0;
			}
			virtual double GetValueLongFloat(const string & path) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) return node.GetValueLongFloat(path);
				}
				return 0.0;
			}
			virtual Color GetValueColor(const string & path) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) return node.GetValueColor(path);
				}
				return 0;
			}
			virtual Time GetValueTime(const string & path) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) return node.GetValueTime(path);
				}
				return 0;
			}
			virtual void GetValueBinary(const string & path, void * buffer) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) { node.GetValueBinary(path, buffer); return; }
				}
			}
			virtual uintptr GetValueBinarySize(const string & path) const override
			{
				for (auto & node : _nodes) {
					auto type = node.GetValueType(path);
					if (type != RegistryValueType::Unknown) return node.GetValueBinarySize(path);
				}
				return 0;
			}
			virtual void SetValue(const string & path, int32 value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, float value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, bool value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, const string & value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, const unichar32 * value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, const unichar16 * value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, const unichar8 * value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, int64 value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, double value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, Color value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, Time value) override { throw NotImplementedException(); }
			virtual void SetValue(const string & path, const void * value, uintptr size) override { throw NotImplementedException(); }
		};

		void ReplicateRegistryNode(RegistryNode * from, RegistryNode * to)
		{
			for (uintptr i = 0; i < from->GetValues().GetLength(); i++) {
				auto & v = from->GetValues()[i];
				auto type = from->GetValueType(v);
				to->CreateValue(v, type);
				if (type == RegistryValueType::Integer) {
					to->SetValue(v, from->GetValueInteger(v));
				} else if (type == RegistryValueType::Float) {
					to->SetValue(v, from->GetValueFloat(v));
				} else if (type == RegistryValueType::Boolean) {
					to->SetValue(v, from->GetValueBoolean(v));
				} else if (type == RegistryValueType::String) {
					to->SetValue(v, from->GetValueString(v));
				} else if (type == RegistryValueType::LongInteger) {
					to->SetValue(v, from->GetValueLongInteger(v));
				} else if (type == RegistryValueType::LongFloat) {
					to->SetValue(v, from->GetValueLongFloat(v));
				} else if (type == RegistryValueType::Color) {
					to->SetValue(v, from->GetValueColor(v));
				} else if (type == RegistryValueType::Time) {
					to->SetValue(v, from->GetValueTime(v));
				} else if (type == RegistryValueType::Binary) {
					DataBlock block(1);
					block.SetLength(from->GetValueBinarySize(v));
					from->GetValueBinary(v, block.GetBuffer());
					to->SetValue(v, block.GetBuffer(), block.GetLength());
				}
			}
			for (int i = 0; i < from->GetSubnodes().GetLength(); i++) {
				auto & n = from->GetSubnodes()[i];
				auto node = from->OpenNode(n);
				if (node) {
					to->CreateNode(n);
					auto node_to = to->OpenNode(n);
					if (node_to) ReplicateRegistryNode(node, node_to);
				}
			}
		}
		void CompileRegistryNode(RegistryNode * node, TokenStream & stream, Token & current)
		{
			while (!TokenIsEOS(current) && !TokenIsPunctuation(current, U"}")) {
				if (!TokenIsWord(current) && !TokenIsStringLiteral(current)) throw InvalidFormatException();
				auto entity_name = GetTokenString(current);
				if (!stream.ReadToken(current)) throw InvalidFormatException();
				if (TokenIsPunctuation(current, U"=")) {
					if (!stream.ReadToken(current)) throw InvalidFormatException();
					bool negative = false;
					bool long_type = false;
					if (TokenIsWord(current, U"long")) {
						long_type = true;
						if (!stream.ReadToken(current)) throw InvalidFormatException();
					}
					if (TokenIsPunctuation(current, U"+")) {
						if (!stream.ReadToken(current)) throw InvalidFormatException();
					} else if (TokenIsPunctuation(current, U"-")) {
						negative = true;
						if (!stream.ReadToken(current)) throw InvalidFormatException();
					}
					if (TokenIsWord(current, U"true")) {
						if (negative || long_type) throw InvalidFormatException();
						node->CreateValue(entity_name, RegistryValueType::Boolean);
						node->SetValue(entity_name, true);
						if (!stream.ReadToken(current)) throw InvalidFormatException();
					} else if (TokenIsWord(current, U"false")) {
						if (negative || long_type) throw InvalidFormatException();
						node->CreateValue(entity_name, RegistryValueType::Boolean);
						node->SetValue(entity_name, false);
						if (!stream.ReadToken(current)) throw InvalidFormatException();
					} else if (TokenIsWord(current, U"infinity")) {
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						node->CreateValue(entity_name, long_type ? RegistryValueType::LongFloat : RegistryValueType::Float);
						node->SetValue(entity_name, negative ? -INFINITY : INFINITY);
					} else if (TokenIsWord(current, U"nan")) {
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						node->CreateValue(entity_name, long_type ? RegistryValueType::LongFloat : RegistryValueType::Float);
						node->SetValue(entity_name, nanf64(""));
					} else if (TokenIsWord(current, U"color")) {
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						if (negative || long_type) throw InvalidFormatException();
						if (!TokenIsIntegerLiteral(current)) throw InvalidFormatException();
						node->CreateValue(entity_name, RegistryValueType::Color);
						node->SetValue(entity_name, Color(GetTokenInteger(current)));
						if (!stream.ReadToken(current)) throw InvalidFormatException();
					} else if (TokenIsWord(current, U"time")) {
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						if (negative || long_type) throw InvalidFormatException();
						if (!TokenIsPunctuation(current, U"{")) throw InvalidFormatException();
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						array<uint> tcom(0x10);
						while (TokenIsIntegerLiteral(current)) {
							tcom << GetTokenInteger(current);
							if (!stream.ReadToken(current)) throw InvalidFormatException();
						}
						if (!TokenIsPunctuation(current, U"}")) throw InvalidFormatException();
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						if (tcom.GetLength() > 7 || !tcom.GetLength()) throw InvalidFormatException();
						while (tcom.GetLength() < 7) tcom.Insert(0, 0);
						node->CreateValue(entity_name, RegistryValueType::Time);
						node->SetValue(entity_name, Time(tcom[0], tcom[1], tcom[2], tcom[3], tcom[4], tcom[5], tcom[6]));
					} else if (TokenIsWord(current, U"binary")) {
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						if (negative || long_type) throw InvalidFormatException();
						if (!TokenIsPunctuation(current, U"{")) throw InvalidFormatException();
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						array<uint8> bcom(0x400);
						while (TokenIsIntegerLiteral(current)) {
							bcom << GetTokenInteger(current);
							if (!stream.ReadToken(current)) throw InvalidFormatException();
						}
						if (!TokenIsPunctuation(current, U"}")) throw InvalidFormatException();
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						node->CreateValue(entity_name, RegistryValueType::Binary);
						node->SetValue(entity_name, bcom.GetBuffer(), bcom.GetLength());
					} else if (TokenIsIntegerLiteral(current)) {
						int64 number = GetTokenInteger(current);
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						node->CreateValue(entity_name, long_type ? RegistryValueType::LongInteger : RegistryValueType::Integer);
						node->SetValue(entity_name, negative ? -number : number);
					} else if (TokenIsFloatLiteral(current)) {
						double number = GetTokenFloat(current);
						if (!stream.ReadToken(current)) throw InvalidFormatException();
						node->CreateValue(entity_name, long_type ? RegistryValueType::LongFloat : RegistryValueType::Float);
						node->SetValue(entity_name, negative ? -number : number);
					} else if (TokenIsStringLiteral(current)) {
						if (negative || long_type) throw InvalidFormatException();
						node->CreateValue(entity_name, RegistryValueType::String);
						node->SetValue(entity_name, GetTokenString(current));
						if (!stream.ReadToken(current)) throw InvalidFormatException();
					} else throw InvalidFormatException();
				} else if (TokenIsPunctuation(current, U"{")) {
					if (!stream.ReadToken(current)) throw InvalidFormatException();
					node->CreateNode(entity_name);
					auto subnode = node->OpenNode(entity_name);
					if (!subnode) throw InvalidFormatException();
					CompileRegistryNode(subnode, stream, current);
					if (!TokenIsPunctuation(current, U"}")) throw InvalidFormatException();
					if (!stream.ReadToken(current)) throw InvalidFormatException();
				} else throw InvalidFormatException();
			}
		}
		oref<RegistryNode> RegistryNode::Merge(RegistryNode ** nodes, uintptr count) { return oref<RegistryNode>::CreateOwned(new MergedRegistryNode(nodes, count)); }
		oref<Registry> Registry::Create(void) { return oref<Registry>::CreateOwned(new RegularRegistry); }
		oref<Registry> Registry::Create(RegistryNode * node)
		{
			auto result = Registry::Create();
			ReplicateRegistryNode(node, result);
			return result;
		}
		oref<Registry> Registry::Load(Stream * source)
		{
			Format::EngineRegistryHeader hdr;
			source->Seek(0, SeekOrigin::Begin);
			if (source->Read(&hdr, sizeof(hdr)) != sizeof(hdr)) throw InvalidFormatException();
			if (Memory::MemoryCompare(hdr.signature, "ecs.1.0", 8) != 0 || hdr.signature_ex != 0x80000004 || hdr.version != 0) throw InvalidFormatException();
			source->Seek(hdr.data_offset, SeekOrigin::Begin);
			auto data = source->ReadBlock(hdr.data_size);
			auto root = owrap(new RegularRegistryNode);
			auto result = oref<Registry>::CreateOwned(new RegularRegistry(root));
			root->InitializeFromMemoryData(data->GetBuffer(), data->GetLength(), hdr.root_offset);
			return result;
		}
		oref<Registry> Registry::LoadFromText(const string & data)
		{
			Token token;
			TokenStream stream(data.GetData(), data.GetLength());
			if (!stream.ReadToken(token)) throw InvalidFormatException();
			auto result = Registry::Create();
			CompileRegistryNode(result, stream, token);
			if (!TokenIsEOS(token)) throw InvalidFormatException();
			return result;
		}
		oref<Registry> Registry::LoadFromText(Stream * source)
		{
			if (!source) throw InvalidArgumentException();
			auto decoder = owrap(new TextDecoder(source, Unicode::Encoding::Unknown));
			return LoadFromText(decoder);
		}
		oref<Registry> Registry::LoadFromText(Stream * source, Unicode::Encoding enc)
		{
			if (!source) throw InvalidArgumentException();
			auto decoder = owrap(new TextDecoder(source, enc));
			return LoadFromText(decoder);
		}
		oref<Registry> Registry::LoadFromText(Stream * source, const Unicode::DecodingCodepage & cp)
		{
			if (!source) throw InvalidArgumentException();
			auto decoder = owrap(new TextDecoder(source, cp));
			return LoadFromText(decoder);
		}
		oref<Registry> Registry::LoadFromText(ITextDecoder * dec)
		{
			if (!dec) throw InvalidArgumentException();
			return LoadFromText(dec->ReadAll());
		}
		oref<Registry> Registry::LoadGeneric(Stream * source)
		{
			if (!source) throw InvalidArgumentException();
			try { return Load(source); }
			catch (Exception & e) { if (e.GetError().error_code != Errores::ErrorInvalidFormat) throw e; }
			source->Seek(0, SeekOrigin::Begin);
			auto buffer = MemoryStream::Create(0x10000);
			source->CopyToUntilEof(buffer);
			buffer->Seek(0, SeekOrigin::Begin);
			return LoadFromText(buffer);
		}
	}
}