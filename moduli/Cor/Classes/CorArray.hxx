#pragma once

#include "CorObject.h"

#include <new>
#include <type_traits>

#define ESSE_MAKE_LINEAR_ITERATORS(array_type, element_type, length) \
	LinearArrayEnumerator< array_type<element_type>, element_type > Elements(void) noexcept { return LinearArrayEnumerator< array_type<element_type>, element_type >(*this, 0, length, 1); } \
	LinearArrayEnumerator< const array_type<element_type>, const element_type > Elements(void) const noexcept { return LinearArrayEnumerator< const array_type<element_type>, const element_type >(*this, 0, length, 1); } \
	LinearArrayEnumerator< array_type<element_type>, element_type > ReversedElements(void) noexcept { return LinearArrayEnumerator< array_type<element_type>, element_type >(*this, length - 1, -1, -1); } \
	LinearArrayEnumerator< const array_type<element_type>, const element_type > ReversedElements(void) const noexcept { return LinearArrayEnumerator< const array_type<element_type>, const element_type >(*this, length - 1, -1, -1); } \
	LinearArrayIterator< array_type<element_type>, element_type > begin(void) noexcept { return LinearArrayIterator< array_type<element_type>, element_type >(*this, 0, 1); } \
	LinearArrayIterator< const array_type<element_type>, const element_type > begin(void) const noexcept { return LinearArrayIterator< const array_type<element_type>, const element_type >(*this, 0, 1); } \
	LinearArrayIterator< array_type<element_type>, element_type > end(void) noexcept { return LinearArrayIterator< array_type<element_type>, element_type >(*this, length, 1); } \
	LinearArrayIterator< const array_type<element_type>, const element_type > end(void) const noexcept { return LinearArrayIterator< const array_type<element_type>, const element_type >(*this, length, 1); }

namespace ESSE
{
	template<class T, class = void> struct HasToStringTraits { static constexpr const bool value = false; };
	template<class T> struct HasToStringTraits<T, std::void_t<decltype(&T::ToString)> > { static constexpr const bool value = true; };
	
	template<class T> std::enable_if_t<std::is_convertible<T, string>::value, string> ConvertToString(const T & t) { return t; }
	template<class T> std::enable_if_t<!std::is_convertible<T, string>::value && HasToStringTraits<T>::value, string> ConvertToString(const T & t) { return t.ToString(); }
	template<class T> std::enable_if_t<!std::is_convertible<T, string>::value && !HasToStringTraits<T>::value, string> ConvertToString(const T & t) { return U"?"; }

	template<class A, class V> class LinearArrayIterator
	{
		A & _array;
		intptr _current, _delta;
	public:
		LinearArrayIterator(A & volume, intptr current, intptr delta) noexcept : _array(volume), _current(current), _delta(delta) {}
		V & operator * (void) const noexcept { return _array[_current]; }
		V & operator * (void) noexcept { return _array[_current]; }
		V & operator -> (void) const noexcept { return _array[_current]; }
		V & operator -> (void) noexcept { return _array[_current]; }

		bool operator == (const LinearArrayIterator & b) const noexcept { return _current == b._current; }
		bool operator != (const LinearArrayIterator & b) const noexcept { return _current != b._current; }
		bool operator == (int b) const noexcept { return _current == b; }
		bool operator != (int b) const noexcept { return _current != b; }

		LinearArrayIterator & operator ++ (void) noexcept { _current += _delta; return *this; }
		LinearArrayIterator operator ++ (int) noexcept { LinearArrayIterator result(_array, _current, _delta); _current += _delta; return result; }
	};
	template<class A, class V> class LinearArrayEnumerator
	{
		A & _array;
		intptr _begin, _end, _delta;
	public:
		LinearArrayEnumerator(A & volume, intptr begin, intptr end, intptr delta) noexcept : _array(volume), _begin(begin), _end(end), _delta(delta) {}
		LinearArrayIterator<A, V> begin(void) noexcept { return LinearArrayIterator<A, V>(_array, _begin, _delta); }
		LinearArrayIterator<A, V> end(void) noexcept { return LinearArrayIterator<A, V>(_array, _end, _delta); }
	};

	template <class V> class array : public Object
	{
		V * _data;
		uintptr _count, _allocated, _block;
	private:
		uintptr _block_align(uintptr elements) noexcept { return ((elements + _block - 1) / _block) * _block; }
		void _expand(uintptr elements)
		{
			auto new_size = _block_align(elements);
			if (new_size != _allocated) {
				if (new_size > _allocated) {
					V * new_data = reinterpret_cast<V *>(realloc(_data, sizeof(V) * new_size));
					if (new_data || new_size == 0) {
						_data = new_data;
						_allocated = new_size;
					} else throw OutOfMemoryException();
				}
			}
		}
		void _collapse(uintptr elements) noexcept
		{
			auto new_size = _block_align(elements);
			if (new_size != _allocated) {
				if (new_size < _allocated) {
					V * new_data = reinterpret_cast<V *>(realloc(_data, sizeof(V) * new_size));
					if (new_data || new_size == 0) {
						_data = new_data;
						_allocated = new_size;
					}
				}
			}
		}
		void _append(const V & v) { new (_data + _count) V(v); _count++; }
		void _append(V && v) { new (_data + _count) V(static_cast<V &&>(v)); _count++; }
	public:
		array(void) noexcept : _count(0), _allocated(0), _data(0), _block(0x100) {}
		explicit array(uintptr block) noexcept : _count(0), _allocated(0), _data(0), _block(max(block, uintptr(1))) {}
		array(const array & src) : _count(src._count), _allocated(0), _data(0), _block(src._block)
		{
			_expand(_count); uintptr i;
			try { for (i = 0; i < _count; i++) new (_data + i) V(src._data[i]); } catch (...) {
				for (uintptr j = i - 1; j < _count; j--) _data[j].V::~V();
				free(_data); throw;
			}
		}
		array(array && src) noexcept : _count(src._count), _allocated(src._allocated), _data(src._data), _block(src._block) { src._data = 0; src._allocated = 0; src._count = 0; }
		~array(void) override { for (uintptr i = 0; i < _count; i++) _data[i].V::~V(); free(_data); }
		array & operator = (const array & src)
		{
			if (this == &src) return *this;
			array copy(src);
			for (uintptr i = 0; i < _count; i++) _data[i].V::~V();
			free(_data);
			_data = copy._data; _count = copy._count; _allocated = copy._allocated; _block = copy._block;
			copy._data = 0; copy._count = 0; copy._allocated = 0;
			return *this;
		}
		array & operator = (array && src) noexcept
		{
			for (uintptr i = 0; i < _count; i++) _data[i].V::~V();
			free(_data);
			_data = src._data; _count = src._count; _allocated = src._allocated; _block = src._block;
			src._data = 0; src._count = 0; src._allocated = 0;
			return *this;
		}

		void Append(const V & v) { _expand(_count + 1); _append(v); }
		void Append(V && v) { _expand(_count + 1); _append(static_cast<V &&>(v)); }
		void Append(const array & src) { if (&src == this) throw InvalidArgumentException(); _expand(_count + src._count); for (uintptr i = 0; i < src._count; i++) _append(src._data[i]); }
		void Append(const V * v, uintptr count) { if (!count) return; if (_data == v) throw InvalidArgumentException(); _expand(_count + count); for (uintptr i = 0; i < count; i++) _append(v[i]); }
		void SwapAt(uintptr i, uintptr j) { swap(_data[i], _data[j]); }
		void Insert(const V & v, uintptr at)
		{
			_expand(_count + 1);
			for (uintptr i = _count; i > at; i--) swap(_data[i], _data[i - 1]);
			try { new (_data + at) V(v); _count++; }
			catch (...) { for (uintptr i = at; i < _count; i++) swap(_data[i], _data[i + 1]); throw; }
		}
		void Insert(V && v, uintptr at)
		{
			_expand(_count + 1);
			for (uintptr i = _count; i > at; i--) swap(_data[i], _data[i - 1]);
			try { new (_data + at) V(static_cast<V &&>(v)); _count++; }
			catch (...) { for (uintptr i = at; i < _count; i++) swap(_data[i], _data[i + 1]); throw; }
		}
		void Insert(const array & v, uintptr at)
		{
			if (v._count) {
				_expand(_count + v._count);
				for (uintptr i = _count + v._count - 1; i >= at + v._count; i--) swap(_data[i], _data[i - v._count]);
				uintptr j;
				try { for (j = 0; j < v._count; j++) new (_data + at + j) V(v._data[j]); } catch (...) {
					for (uintptr i = j - 1; i < v._count; i--) _data[at + i].V::~V();
					for (uintptr i = at; i < _count; i++) swap(_data[i], _data[i + v._count]);
					throw;
				}
				_count += v._count;
			}
		}
		void Insert(const V * v, uintptr count, uintptr at)
		{
			if (count) {
				_expand(_count + count);
				for (uintptr i = _count + count - 1; i >= at + count; i--) swap(_data[i], _data[i - count]);
				uintptr j;
				try { for (j = 0; j < count; j++) new (_data + at + j) V(v[j]); } catch (...) {
					for (uintptr i = j - 1; i < count; i--) _data[at + i].V::~V();
					for (uintptr i = at; i < _count; i++) swap(_data[i], _data[i + count]);
					throw;
				}
				_count += count;
			}
		}
		V & ElementAt(uintptr index) noexcept { return _data[index]; }
		const V & ElementAt(uintptr index) const noexcept { return _data[index]; }
		V & FirstElement(void) noexcept { return _data[0]; }
		const V & FirstElement(void) const noexcept { return _data[0]; }
		V & LastElement(void) noexcept { return _data[_count - 1]; }
		const V & LastElement(void) const noexcept { return _data[_count - 1]; }
		void Remove(uintptr index) noexcept { _data[index].V::~V(); for (uintptr i = index; i < _count - 1; i++) swap(_data[i], _data[i + 1]); _count--; _collapse(_count); }
		void RemoveRange(uintptr at, uintptr length) noexcept
		{
			for (uintptr i = 0; i < length; i++) _data[at + i].V::~V();
			for (uintptr i = at; i < _count - length; i++) swap(_data[i], _data[i + length]);
			_count -= length; _collapse(_count);
		}
		void RemoveFirst(void) noexcept { Remove(0); }
		void RemoveLast(void) noexcept { Remove(_count - 1); }
		void Clear(void) noexcept { for (uintptr i = 0; i < _count; i++) _data[i].V::~V(); free(_data); _data = 0; _count = 0; _allocated = 0; }
		void SetLength(uintptr length)
		{
			if (length > _count) {
				_expand(length); uintptr i;
				try { for (i = _count; i < length; i++) new (_data + i) V(); }
				catch (...) { for (uintptr j = i; j > _count; j--) _data[j - 1].V::~V(); throw; }
				_count = length;
			} else if (length < _count) {
				for (uintptr i = length; i < _count; i++) _data[i].V::~V(); _count = length;
				_collapse(_count);
			}
		}
		uintptr GetLength(void) const noexcept { return _count; }
		V * GetBuffer(void) { return _data; }
		const V * GetBuffer(void) const { return _data; }
		virtual string ToStringE(ErrorContext & ectx) const noexcept override
		{
			ESSE_TRY_INTRO
			string result = U"ordo : [";
			for (uintptr i = 0; i < _count; i++) {
				if (i) result += U", ";
				result += ConvertToString(_data[i]);
			}
			return result + U"]";
			ESSE_TRY_OUTRO(string())
		}

		operator V * (void) noexcept { return _data; }
		operator const V * (void) const noexcept { return _data; }
		array & operator << (const V & v) { Append(v); return *this; }
		array & operator << (V && v) { Append(static_cast<V &&>(v)); return *this; }
		array & operator << (const array & src) { Append(src); return *this; }
		V & operator [] (uintptr index) noexcept { return _data[index]; }
		const V & operator [] (uintptr index) const noexcept{ return _data[index]; }
		bool friend operator == (const array & a, const array & b)
		{
			if (a._count != b._count) return false;
			for (uintptr i = 0; i < a._count; i++) if (a[i] != b[i]) return false;
			return true;
		}
		bool friend operator != (const array & a, const array & b)
		{
			if (a._count != b._count) return true;
			for (uintptr i = 0; i < a._count; i++) if (a[i] != b[i]) return true;
			return false;
		}

		ESSE_MAKE_LINEAR_ITERATORS(array, V, _count)
	};
	template <class V> class stable_array : public Object
	{
		V ** _data;
		uintptr _count, _allocated, _block;
	private:
		uintptr _block_align(uintptr elements) noexcept { return ((elements + _block - 1) / _block) * _block; }
		void _expand(uintptr elements)
		{
			auto new_size = _block_align(elements);
			if (new_size != _allocated) {
				if (new_size > _allocated) {
					V ** new_data = reinterpret_cast<V **>(realloc(_data, sizeof(V*) * new_size));
					if (new_data || new_size == 0) {
						_data = new_data;
						_allocated = new_size;
					} else throw OutOfMemoryException();
				}
			}
		}
		void _collapse(uintptr elements) noexcept
		{
			auto new_size = _block_align(elements);
			if (new_size != _allocated) {
				if (new_size < _allocated) {
					V ** new_data = reinterpret_cast<V **>(realloc(_data, sizeof(V*) * new_size));
					if (new_data || new_size == 0) {
						_data = new_data;
						_allocated = new_size;
					}
				}
			}
		}
		void _append(const V & v) { _data[_count] = new V(v); _count++; }
		void _append(V && v) { _data[_count] = new V(static_cast<V &&>(v)); _count++; }
	public:
		stable_array(void) noexcept : _count(0), _allocated(0), _data(0), _block(0x100) {}
		explicit stable_array(uintptr block) noexcept : _count(0), _allocated(0), _data(0), _block(max(block, uintptr(1))) {}
		stable_array(const stable_array & src) : _count(src._count), _allocated(0), _data(0), _block(src._block)
		{
			_expand(_count); uintptr i;
			try { for (i = 0; i < _count; i++) _data[i] = new V(*src._data[i]); } catch (...) {
				for (uintptr j = i - 1; j < _count; j--) delete _data[j];
				free(_data); throw;
			}
		}
		stable_array(stable_array && src) noexcept : _count(src._count), _allocated(src._allocated), _data(src._data), _block(src._block) { src._data = 0; src._allocated = 0; src._count = 0; }
		~stable_array(void) override { for (uintptr i = 0; i < _count; i++) delete _data[i]; free(_data); }
		stable_array & operator = (const stable_array & src)
		{
			if (this == &src) return *this;
			stable_array copy(src);
			for (uintptr i = 0; i < _count; i++) delete _data[i];
			free(_data);
			_data = copy._data; _count = copy._count; _allocated = copy._allocated; _block = copy._block;
			copy._data = 0; copy._count = 0; copy._allocated = 0;
			return *this;
		}
		stable_array & operator = (stable_array && src) noexcept
		{
			for (uintptr i = 0; i < _count; i++) delete _data[i];
			free(_data);
			_data = src._data; _count = src._count; _allocated = src._allocated; _block = src._block;
			src._data = 0; src._count = 0; src._allocated = 0;
			return *this;
		}

		void Append(const V & v) { _expand(_count + 1); _append(v); }
		void Append(V && v) { _expand(_count + 1); _append(static_cast<V &&>(v)); }
		void Append(const stable_array & src) { if (&src == this) throw InvalidArgumentException(); _expand(_count + src._count); for (uintptr i = 0; i < src._count; i++) _append(*src._data[i]); }
		void Append(const V * v, uintptr count) { if (!count) return; if (_data == v) throw InvalidArgumentException(); _expand(_count + count); for (uintptr i = 0; i < count; i++) _append(v[i]); }
		void SwapAt(uintptr i, uintptr j) { safe_swap(_data[i], _data[j]); }
		void Insert(const V & v, uintptr at)
		{
			_expand(_count + 1);
			for (uintptr i = _count; i > at; i--) safe_swap(_data[i], _data[i - 1]);
			try { _data[at] = new V(v); _count++; }
			catch (...) { for (uintptr i = at; i < _count; i++) safe_swap(_data[i], _data[i + 1]); throw; }
		}
		void Insert(V && v, uintptr at)
		{
			_expand(_count + 1);
			for (uintptr i = _count; i > at; i--) safe_swap(_data[i], _data[i - 1]);
			try { _data[at] = new V(static_cast<V &&>(v)); _count++; }
			catch (...) { for (uintptr i = at; i < _count; i++) safe_swap(_data[i], _data[i + 1]); throw; }
		}
		void Insert(const stable_array & v, uintptr at)
		{
			if (v._count) {
				_expand(_count + v._count);
				for (uintptr i = _count + v._count - 1; i >= at + v._count; i--) safe_swap(_data[i], _data[i - v._count]);
				uintptr j;
				try { for (j = 0; j < v._count; j++) _data[at + j] = new V(*v._data[j]); } catch (...) {
					for (uintptr i = j - 1; i < v._count; i--) delete _data[at + i];
					for (uintptr i = at; i < _count; i++) safe_swap(_data[i], _data[i + v._count]);
					throw;
				}
				_count += v._count;
			}
		}
		void Insert(const V * v, uintptr count, uintptr at)
		{
			if (count) {
				_expand(_count + count);
				for (uintptr i = _count + count - 1; i >= at + count; i--) safe_swap(_data[i], _data[i - count]);
				uintptr j;
				try { for (j = 0; j < count; j++) _data[at + j] = new V(v[j]); } catch (...) {
					for (uintptr i = j - 1; i < count; i--) delete _data[at + i];
					for (uintptr i = at; i < _count; i++) safe_swap(_data[i], _data[i + count]);
					throw;
				}
				_count += count;
			}
		}
		V & ElementAt(uintptr index) noexcept { return *_data[index]; }
		const V & ElementAt(uintptr index) const noexcept { return *_data[index]; }
		V & FirstElement(void) noexcept { return *_data[0]; }
		const V & FirstElement(void) const noexcept { return *_data[0]; }
		V & LastElement(void) noexcept { return *_data[_count - 1]; }
		const V & LastElement(void) const noexcept { return *_data[_count - 1]; }
		void Remove(uintptr index) noexcept { delete _data[index]; for (uintptr i = index; i < _count - 1; i++) safe_swap(_data[i], _data[i + 1]); _count--; _collapse(_count); }
		void RemoveRange(uintptr at, uintptr length) noexcept
		{
			for (uintptr i = 0; i < length; i++) delete _data[at + i];
			for (uintptr i = at; i < _count - length; i++) safe_swap(_data[i], _data[i + length]);
			_count -= length; _collapse(_count);
		}
		void RemoveFirst(void) noexcept { Remove(0); }
		void RemoveLast(void) noexcept { Remove(_count - 1); }
		void Clear(void) noexcept { for (uintptr i = 0; i < _count; i++) delete _data[i]; free(_data); _data = 0; _count = 0; _allocated = 0; }
		void SetLength(uintptr length)
		{
			if (length > _count) {
				_expand(length); uintptr i;
				try { for (i = _count; i < length; i++) _data[i] = new V(); }
				catch (...) { for (uintptr j = i; j > _count; j--) delete _data[j - 1]; throw; }
				_count = length;
			} else if (length < _count) {
				for (uintptr i = length; i < _count; i++) delete _data[i]; _count = length;
				_collapse(_count);
			}
		}
		uintptr GetLength(void) const noexcept { return _count; }
		virtual string ToStringE(ErrorContext & ectx) const noexcept override
		{
			ESSE_TRY_INTRO
			string result = U"ordo : [";
			for (uintptr i = 0; i < _count; i++) {
				if (i) result += U", ";
				result += ConvertToString(*_data[i]);
			}
			return result + U"]";
			ESSE_TRY_OUTRO(string())
		}

		stable_array & operator << (const V & v) { Append(v); return *this; }
		stable_array & operator << (V && v) { Append(static_cast<V &&>(v)); return *this; }
		stable_array & operator << (const stable_array & src) { Append(src); return *this; }
		V & operator [] (uintptr index) noexcept { return *_data[index]; }
		const V & operator [] (uintptr index) const noexcept{ return *_data[index]; }
		bool friend operator == (const stable_array & a, const stable_array & b)
		{
			if (a._count != b._count) return false;
			for (uintptr i = 0; i < a._count; i++) if (a[i] != b[i]) return false;
			return true;
		}
		bool friend operator != (const stable_array & a, const stable_array & b)
		{
			if (a._count != b._count) return true;
			for (uintptr i = 0; i < a._count; i++) if (a[i] != b[i]) return true;
			return false;
		}

		ESSE_MAKE_LINEAR_ITERATORS(stable_array, V, _count)
	};
	template <class O> class object_array : public Object
	{
		O ** _data;
		uintptr _count, _allocated, _block;
	private:
		uintptr _block_align(uintptr elements) noexcept { return ((elements + _block - 1) / _block) * _block; }
		void _expand(uintptr elements)
		{
			auto new_size = _block_align(elements);
			if (new_size != _allocated) {
				if (new_size > _allocated) {
					O ** new_data = reinterpret_cast<O **>(realloc(_data, sizeof(O*) * new_size));
					if (new_data || new_size == 0) {
						_data = new_data;
						_allocated = new_size;
					} else throw OutOfMemoryException();
				}
			}
		}
		void _collapse(uintptr elements) noexcept
		{
			auto new_size = _block_align(elements);
			if (new_size != _allocated) {
				if (new_size < _allocated) {
					O ** new_data = reinterpret_cast<O **>(realloc(_data, sizeof(O*) * new_size));
					if (new_data || new_size == 0) {
						_data = new_data;
						_allocated = new_size;
					}
				}
			}
		}
		void _append(O * o) { _data[_count] = o; if (o) o->Retain(); _count++; }
	public:
		object_array(void) noexcept : _count(0), _allocated(0), _data(0), _block(0x100) {}
		explicit object_array(uintptr block) noexcept : _count(0), _allocated(0), _data(0), _block(max(block, uintptr(1))) {}
		object_array(const object_array & src) : _count(src._count), _allocated(0), _data(0), _block(src._block)
		{
			_expand(_count); uintptr i;
			for (i = 0; i < _count; i++) { _data[i] = src._data[i]; if (_data[i]) _data[i]->Retain(); }
		}
		object_array(object_array && src) noexcept : _count(src._count), _allocated(src._allocated), _data(src._data), _block(src._block) { src._data = 0; src._allocated = 0; src._count = 0; }
		~object_array(void) override { for (uintptr i = 0; i < _count; i++) if (_data[i]) _data[i]->Release(); free(_data); }
		object_array & operator = (const object_array & src)
		{
			if (this == &src) return *this;
			object_array copy(src);
			for (uintptr i = 0; i < _count; i++) if (_data[i]) _data[i]->Release();
			free(_data);
			_data = copy._data; _count = copy._count; _allocated = copy._allocated; _block = copy._block;
			copy._data = 0; copy._count = 0; copy._allocated = 0;
			return *this;
		}
		object_array & operator = (object_array && src) noexcept
		{
			for (uintptr i = 0; i < _count; i++) if (_data[i]) _data[i]->Release();
			free(_data);
			_data = src._data; _count = src._count; _allocated = src._allocated; _block = src._block;
			src._data = 0; src._count = 0; src._allocated = 0;
			return *this;
		}

		void Append(O * o) { _expand(_count + 1); _append(o); }
		void Append(const object_array & src) { if (&src == this) throw InvalidArgumentException(); _expand(_count + src._count); for (uintptr i = 0; i < src._count; i++) _append(src._data[i]); }
		void Append(O ** o, uintptr count) { if (!count) return; if (_data == o) throw InvalidArgumentException(); _expand(_count + count); for (uintptr i = 0; i < count; i++) _append(o[i]); }
		void SwapAt(uintptr i, uintptr j) { safe_swap(_data[i], _data[j]); }
		void Insert(O * o, uintptr at)
		{
			_expand(_count + 1);
			for (uintptr i = _count; i > at; i--) safe_swap(_data[i], _data[i - 1]);
			_data[at] = o; if (o) o->Retain(); _count++;
		}
		void Insert(const object_array & v, uintptr at)
		{
			if (v._count) {
				_expand(_count + v._count);
				for (uintptr i = _count + v._count - 1; i >= at + v._count; i--) safe_swap(_data[i], _data[i - v._count]);
				uintptr j;
				for (j = 0; j < v._count; j++) { _data[at + j] = v._data[j]; if (_data[at + j]) _data[at + j]->Retain(); }
				_count += v._count;
			}
		}
		void Insert(O ** o, uintptr count, uintptr at)
		{
			if (count) {
				_expand(_count + count);
				for (uintptr i = _count + count - 1; i >= at + count; i--) safe_swap(_data[i], _data[i - count]);
				uintptr j;
				for (j = 0; j < count; j++) { _data[at + j] = o[j]; if (_data[at + j]) _data[at + j]->Retain(); }
				_count += count;
			}
		}
		O * ReferenceAt(uintptr index) const noexcept { return _data[index]; }
		O * FirstReference(void) const noexcept { return _data[0]; }
		O * LastReference(void) const noexcept { return _data[_count - 1]; }
		O & ElementAt(uintptr index) const noexcept { return *_data[index]; }
		O & FirstElement(void) const noexcept { return *_data[0]; }
		O & LastElement(void) const noexcept { return *_data[_count - 1]; }
		void SetElement(O * o, uintptr at) noexcept { if (_data[at]) _data[at]->Release(); _data[at] = o; if (_data[at]) _data[at]->Retain(); }
		void Remove(uintptr index) noexcept { if (_data[index]) _data[index]->Release(); for (uintptr i = index; i < _count - 1; i++) swap(_data[i], _data[i + 1]); _count--; _collapse(_count); }
		void RemoveRange(uintptr at, uintptr length) noexcept
		{
			for (uintptr i = 0; i < length; i++) { if (_data[at + i]) _data[at + i]->Release(); }
			for (uintptr i = at; i < _count - length; i++) safe_swap(_data[i], _data[i + length]);
			_count -= length; _collapse(_count);
		}
		void RemoveFirst(void) noexcept { Remove(0); }
		void RemoveLast(void) noexcept { Remove(_count - 1); }
		void Clear(void) noexcept { for (uintptr i = 0; i < _count; i++) if (_data[i]) _data[i]->Release(); free(_data); _data = 0; _count = 0; _allocated = 0; }
		void SetLength(uintptr length)
		{
			if (length > _count) {
				_expand(length); uintptr i;
				for (i = _count; i < length; i++) _data[i] = 0;
				_count = length;
			} else if (length < _count) {
				for (uintptr i = length; i < _count; i++) if (_data[i]) _data[i]->Release();
				_count = length;
				_collapse(_count);
			}
		}
		uintptr GetLength(void) const noexcept { return _count; }
		O ** GetBuffer(void) const { return _data; }
		virtual string ToStringE(ErrorContext & ectx) const noexcept override
		{
			ESSE_TRY_INTRO
			string result = U"ordo : [";
			for (uintptr i = 0; i < _count; i++) {
				if (i) result += U", ";
				result += _data[i] ? _data[i]->ToString() : U"NULL";
			}
			return result + U"]";
			ESSE_TRY_OUTRO(string())
		}

		object_array & operator << (O * o) { Append(o); return *this; }
		object_array & operator << (const object_array & src) { Append(src); return *this; }
		O & operator [] (uintptr index) const noexcept { return *_data[index]; }
		O * operator () (uintptr index) const noexcept { return _data[index]; }
		bool friend operator == (const object_array & a, const object_array & b)
		{
			if (a._count != b._count) return false;
			for (uintptr i = 0; i < a._count; i++) if (a(i) != b(i)) return false;
			return true;
		}
		bool friend operator != (const object_array & a, const object_array & b)
		{
			if (a._count != b._count) return true;
			for (uintptr i = 0; i < a._count; i++) if (a(i) != b(i)) return true;
			return false;
		}

		ESSE_MAKE_LINEAR_ITERATORS(object_array, O, _count)
	};

	template <class A, class F> void SortArrayRange(A & volume, F comparator, int from, int count)
	{
		if (count < 2) return;
		int ref = from + count / 2;
		int l = from - 1;
		int r = from + count;
		while (true) {
			do l++; while (comparator(volume[l], volume[ref]) < 0);
			do r--; while (comparator(volume[r], volume[ref]) > 0);
			if (l >= r) {
				if (l > r) ref = r;
				else if (r == from + count - 1) ref = r - 1;
				else ref = r;
				break;
			}
			volume.SwapAt(l, r);
			if (ref == l) ref = r;
			else if (ref == r) ref = l;
		}
		SortArrayRange(volume, comparator, from, ref - from + 1);
		SortArrayRange(volume, comparator, ref + 1, count - ref + from - 1);
	}
	template <class A> void SortArrayRange(A & volume, bool ascending, int from, int count)
	{
		if (count < 2) return;
		int ref = from + count / 2;
		int l = from - 1;
		int r = from + count;
		while (true) {
			if (ascending) {
				do l++; while (volume[l] < volume[ref]);
				do r--; while (volume[r] > volume[ref]);
			} else {
				do l++; while (volume[l] > volume[ref]);
				do r--; while (volume[r] < volume[ref]);
			}
			if (l >= r) {
				if (l > r) ref = r;
				else if (r == from + count - 1) ref = r - 1;
				else ref = r;
				break;
			}
			volume.SwapAt(l, r);
			if (ref == l) ref = r;
			else if (ref == r) ref = l;
		}
		SortArrayRange(volume, ascending, from, ref - from + 1);
		SortArrayRange(volume, ascending, ref + 1, count - ref + from - 1);
	}
	template <class A, class F> void SortArray(A & volume, F comparator) { SortArrayRange(volume, comparator, 0, volume.GetLength()); }
	template <class A> void SortArray(A & volume, bool ascending = true) { SortArrayRange(volume, ascending, 0, volume.GetLength()); }

	typedef array<uint8> DataBlock;

	oref<DataBlock> EncodeString(const string & str, Unicode::Encoding enc, bool include_terminator);
	oref<DataBlock> EncodeString(const string & str, const Unicode::EncodingCodepage & cp, bool include_terminator);
	array<string> SplitString(const string & str, unichar32 div);
	string GatherString(const array<string> & str, unichar32 div);

	string HexStringFromData(const void * data, uintptr length, uintptr max_length, bool byte_spaces);
	string HexStringFromData(const DataBlock * data, uintptr max_length, bool byte_spaces);
	oref<DataBlock> DataFromHexString(const string & str);
	string Base64StringFromData(const void * data, uintptr length);
	string Base64StringFromData(const DataBlock * data);
	oref<DataBlock> DataFromBase64String(const string & str);

	class dynamic_string_ucs1
	{
		uintptr _length;
		array<unichar8> _data;
	public:
		dynamic_string_ucs1(void);
		dynamic_string_ucs1(uintptr block);
		dynamic_string_ucs1(const ucs1_string & src);
		dynamic_string_ucs1(const ucs1_string & src, uintptr block);

		operator ucs1_string (void) const;
		operator unichar8 * (void) noexcept;
		operator const unichar8 * (void) const noexcept;
		unichar8 & operator [] (uintptr index) noexcept;
		unichar8 operator [] (uintptr index) const noexcept;

		string ToString(void) const;
		uintptr GetLength(void) const noexcept;
		unichar8 * GetData(void) noexcept;
		const unichar8 * GetData(void) const noexcept;
		unichar8 & GetCharacterAt(uintptr index) noexcept;
		unichar8 GetCharacterAt(uintptr index) const noexcept;

		void Append(const ucs1_string & str);
		void Append(unichar8 chr);
		dynamic_string_ucs1 & operator += (const ucs1_string & str);
		dynamic_string_ucs1 & operator += (unichar8 chr);
		dynamic_string_ucs1 & operator << (const ucs1_string & str);
		dynamic_string_ucs1 & operator << (unichar8 chr);

		void Insert(const ucs1_string & str, uintptr at);
		void RemoveRange(uintptr from, uintptr length);
		void Clear(void);
		void Reserve(uintptr units);
		void ReevaluateLength(void) noexcept;
		uintptr GetReservedLength(void) const noexcept;
	};
	class dynamic_string_ucs2
	{
		uintptr _length;
		array<unichar16> _data;
	public:
		dynamic_string_ucs2(void);
		dynamic_string_ucs2(uintptr block);
		dynamic_string_ucs2(const ucs2_string & src);
		dynamic_string_ucs2(const ucs2_string & src, uintptr block);

		operator ucs2_string (void) const;
		operator unichar16 * (void) noexcept;
		operator const unichar16 * (void) const noexcept;
		unichar16 & operator [] (uintptr index) noexcept;
		unichar16 operator [] (uintptr index) const noexcept;

		string ToString(void) const;
		uintptr GetLength(void) const noexcept;
		unichar16 * GetData(void) noexcept;
		const unichar16 * GetData(void) const noexcept;
		unichar16 & GetCharacterAt(uintptr index) noexcept;
		unichar16 GetCharacterAt(uintptr index) const noexcept;

		void Append(const ucs2_string & str);
		void Append(unichar16 chr);
		dynamic_string_ucs2 & operator += (const ucs2_string & str);
		dynamic_string_ucs2 & operator += (unichar16 chr);
		dynamic_string_ucs2 & operator << (const ucs2_string & str);
		dynamic_string_ucs2 & operator << (unichar16 chr);

		void Insert(const ucs2_string & str, uintptr at);
		void RemoveRange(uintptr from, uintptr length);
		void Clear(void);
		void Reserve(uintptr units);
		void ReevaluateLength(void) noexcept;
		uintptr GetReservedLength(void) const noexcept;
	};
	class dynamic_string_ucs4
	{
		uintptr _length;
		array<unichar32> _data;
	public:
		dynamic_string_ucs4(void);
		dynamic_string_ucs4(uintptr block);
		dynamic_string_ucs4(const ucs4_string & src);
		dynamic_string_ucs4(const ucs4_string & src, uintptr block);

		operator ucs4_string (void) const;
		operator unichar32 * (void) noexcept;
		operator const unichar32 * (void) const noexcept;
		unichar32 & operator [] (uintptr index) noexcept;
		unichar32 operator [] (uintptr index) const noexcept;

		string ToString(void) const;
		uintptr GetLength(void) const noexcept;
		unichar32 * GetData(void) noexcept;
		const unichar32 * GetData(void) const noexcept;
		unichar32 & GetCharacterAt(uintptr index) noexcept;
		unichar32 GetCharacterAt(uintptr index) const noexcept;

		void Append(const ucs4_string & str);
		void Append(unichar32 chr);
		dynamic_string_ucs4 & operator += (const ucs4_string & str);
		dynamic_string_ucs4 & operator += (unichar32 chr);
		dynamic_string_ucs4 & operator << (const ucs4_string & str);
		dynamic_string_ucs4 & operator << (unichar32 chr);

		void Insert(const ucs4_string & str, uintptr at);
		void RemoveRange(uintptr from, uintptr length);
		void Clear(void);
		void Reserve(uintptr units);
		void ReevaluateLength(void) noexcept;
		uintptr GetReservedLength(void) const noexcept;
	};
}