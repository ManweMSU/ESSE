#pragma once

#include "CorString.h"

namespace ESSE
{
	class Object
	{
		uintptr _reference_count;
	public:
		Object(void) noexcept;
		Object(const Object & src) = delete;
		Object & operator = (const Object & src) = delete;
		virtual uintptr Retain(void) noexcept;
		virtual uintptr Release(void) noexcept;
		virtual ~Object(void);
		#ifdef ESSE_SYSTEMA_WINDOWS
		private: virtual void _delete(void) noexcept; public:
		#endif
		virtual string ToStringE(ErrorContext & ectx) const noexcept;
		string ToString(void) const;
		uintptr GetReferenceCount(void) const noexcept;
	};
	class WaitableObject : public Object
	{
	public:
		virtual void Wait(void) noexcept = 0;
		virtual bool WaitFor(uint32 ms) noexcept = 0;
	};
	class DynamicObject : public Object
	{
	public:
		virtual void * DynamicCast(const void * cls, ErrorContext & ectx) noexcept = 0;
		virtual const void * GetType(void) noexcept = 0;
	};

	struct DynamicCastStandardClasses
	{
		const void * Object;
		const void * DynamicObject;
		const void * Console;
		const void * ITextEncoder;
		const void * ITextDecoder;
		const void * IDevice;
		const void * IDeviceContext;
		const void * IDeviceContext2D;
		const void * IDeviceContextFactory2D;
	};
	extern DynamicCastStandardClasses Classes;

	template <class V> void swap(V & a, V & b) noexcept { if (&a == &b) return; uint8 buffer[sizeof(V)]; Memory::MemoryCopy(buffer, &a, sizeof(V)); Memory::MemoryCopy(&a, &b, sizeof(V)); Memory::MemoryCopy(&b, buffer, sizeof(V)); }
	template <class V> void safe_swap(V & a, V & b) { V e = a; a = b; b = e; }
	template <class V> V min(V a, V b) { return (a < b) ? a : b; }
	template <class V> V max(V a, V b) { return (a < b) ? b : a; }
	template <class V> V sgn(V x) noexcept { return (x > V(0)) ? V(1) : ((x < V(0)) ? V(-1) : V(0)); }

	template <class O> class oref final
	{
		O * _reference = 0;
	public:
		oref(void) noexcept : _reference(0) {}
		oref(O * ref) noexcept : _reference(ref) { if (_reference) _reference->Retain(); }
		oref(const oref & src) noexcept : _reference(src._reference) { if (_reference) _reference->Retain(); }
		oref(oref && src) noexcept : _reference(src._reference) { src._reference = 0; }
		~oref(void) { if (_reference) _reference->Release(); _reference = 0; }
		oref & operator = (const oref & src) noexcept
		{
			if (this == &src) return *this;
			if (_reference) _reference->Release();
			_reference = src._reference;
			if (_reference) _reference->Retain();
			return *this;
		}
		oref & operator = (oref && src) noexcept
		{
			if (_reference) _reference->Release();
			_reference = src._reference;
			src._reference = 0;
			return *this;
		}

		O & operator * (void) const noexcept { return *_reference; }
		O * operator -> (void) const noexcept { return _reference; }
		operator O * (void) const noexcept { return _reference; }
		operator bool (void) const noexcept { return _reference != 0; }
		O * Inner(void) const noexcept { return _reference; }
		O ** InnerRef(void) noexcept { return &_reference; }

		void SetOwned(O * ref) noexcept { if (_reference) _reference->Release(); _reference = ref; }
		void SetRetain(O * ref) noexcept { if (_reference) _reference->Release(); _reference = ref; if (_reference) _reference->Retain(); }
		void Clear(void) noexcept { if (_reference) _reference->Release(); _reference = 0; }
		static oref CreateOwned(O * ref) noexcept { oref r; r.SetOwned(ref); return r; }

		string ToString(void) const { return _reference ? _reference->ToString() : U"NULL"; }

		bool friend operator == (const oref & a, const oref & b) noexcept { return a._reference == b._reference; }
		bool friend operator != (const oref & a, const oref & b) noexcept { return a._reference != b._reference; }
		bool friend operator == (const oref & a, O * b) noexcept { return a._reference == b; }
		bool friend operator != (const oref & a, O * b) noexcept { return a._reference != b; }
		bool friend operator == (O * a, const oref & b) noexcept { return a == b._reference; }
		bool friend operator != (O * a, const oref & b) noexcept { return a != b._reference; }
	};
	template <class O> oref<O> owrap(O * o) noexcept { return oref<O>::CreateOwned(o); }
}