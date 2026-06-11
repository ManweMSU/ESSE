#pragma once

#include <Cor/Classes/CorArray.hxx>
#include <Cor/IO/CorStreams.h>
#include <Cor/Images/CorImages.h>

namespace ESSE
{
	namespace Formationes
	{
		enum class RegistryValueType : uint {
			Unknown = 0, Integer = 1, Float = 2, Boolean = 3, String = 4,
			LongInteger = 5, LongFloat = 6, Color = 7, Time = 8, Binary = 9
		};
		class RegistryNode : public Object
		{
		public:
			virtual const array<string> & GetSubnodes(void) const noexcept = 0;
			virtual const array<string> & GetValues(void) const noexcept = 0;

			virtual void CreateNode(const string & path) = 0;
			virtual void RemoveNode(const string & path) = 0;
			virtual void RenameNode(const string & node, const string & name) = 0;
			virtual oref<RegistryNode> OpenNode(const string & path) = 0;

			virtual void CreateValue(const string & path, RegistryValueType type) = 0;
			virtual void RemoveValue(const string & path) = 0;
			virtual void RenameValue(const string & name_from, const string & name_to) = 0;
			virtual RegistryValueType GetValueType(const string & path) const = 0;

			virtual int32 GetValueInteger(const string & path) const = 0;
			virtual float GetValueFloat(const string & path) const = 0;
			virtual bool GetValueBoolean(const string & path) const = 0;
			virtual string GetValueString(const string & path) const = 0;
			virtual int64 GetValueLongInteger(const string & path) const = 0;
			virtual double GetValueLongFloat(const string & path) const = 0;
			virtual Color GetValueColor(const string & path) const = 0;
			virtual Time GetValueTime(const string & path) const = 0;
			virtual void GetValueBinary(const string & path, void * buffer) const = 0;
			virtual uintptr GetValueBinarySize(const string & path) const = 0;

			virtual void SetValue(const string & path, int32 value) = 0;
			virtual void SetValue(const string & path, float value) = 0;
			virtual void SetValue(const string & path, bool value) = 0;
			virtual void SetValue(const string & path, const string & value) = 0;
			virtual void SetValue(const string & path, const unichar32 * value) = 0;
			virtual void SetValue(const string & path, const unichar16 * value) = 0;
			virtual void SetValue(const string & path, const unichar8 * value) = 0;
			virtual void SetValue(const string & path, int64 value) = 0;
			virtual void SetValue(const string & path, double value) = 0;
			virtual void SetValue(const string & path, Color value) = 0;
			virtual void SetValue(const string & path, Time value) = 0;
			virtual void SetValue(const string & path, const void * value, uintptr size) = 0;

			static oref<RegistryNode> Merge(RegistryNode ** nodes, uintptr count);
		};
		class Registry : public RegistryNode
		{
		public:
			virtual oref<Registry> Clone(void) const = 0;
			virtual void Save(Stream * stream) const = 0;
			virtual void SaveToText(Stream * stream) const = 0;
			virtual void SaveToText(Stream * stream, Unicode::Encoding enc) const = 0;
			virtual void SaveToText(Stream * stream, const Unicode::EncodingCodepage & cp) const = 0;
			virtual void SaveToText(ITextEncoder * enc, bool escape_unicode) const = 0;
			virtual string SaveToText(void) const = 0;

			static oref<Registry> Create(void);
			static oref<Registry> Create(RegistryNode * node);
			static oref<Registry> Load(Stream * source);
			static oref<Registry> LoadFromText(const string & data);
			static oref<Registry> LoadFromText(Stream * source);
			static oref<Registry> LoadFromText(Stream * source, Unicode::Encoding enc);
			static oref<Registry> LoadFromText(Stream * source, const Unicode::DecodingCodepage & cp);
			static oref<Registry> LoadFromText(ITextDecoder * dec);
			static oref<Registry> LoadGeneric(Stream * source);
		};
	}
}