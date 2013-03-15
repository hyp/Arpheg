#pragma once

#include "../../core/bytes.h"
#include "../../core/bufferStringStream.h"

namespace data {
namespace text {

	union Value;
	struct TaggedValue;

	//InPlace array
	struct TaggedArray {
		enum { kMaxElements = 32 };
		TaggedValue* begin;
		uint32 length;
	};
	struct String {
		uint8* begin,*end;
	};
	union Value {
		float number;
		String string;
		TaggedArray array;
	};
	struct TaggedValue {
		enum {
			Nothing,Number,String,Array
		};
		uint32 tag;
		Value value;
	};


	class Parser {
	public:
		enum SubDataHandling {
			SubDataNone = 0,
			SubDataObject,
			SubDataCustomBlock,
		};
		Parser();
		void parse(core::Bytes data);

		virtual void get(core::Bytes id);
		virtual void set(core::Bytes id);
		virtual SubDataHandling handleSubdata(core::Bytes id);
		virtual void endSubdata();
		virtual void subdata(core::Bytes subdata);
		virtual void error();
		
		//Value getters
		void string(const char* str);
		void number(float n);
		
		//Value setters
		core::Bytes string();
		uint32 strings(core::Bytes* dest,uint32 max);
		float number();
		bool boolean();
		vec2f vec2();
		vec3f vec3();
		vec4f vec4();

		void typecheck(uint32 type);
		void typecheck(uint32 type,uint32 count);
		void typecheck(uint32 types[],uint32 count);

		static core::Bytes identifier(core::Bytes& data);
		static bool compare(core::Bytes id,const char* str);
		static bool match(core::Bytes& id,const char* str);
	protected:
		TaggedValue value();
		TaggedValue values();
		void statement();
		
		bool setResult_;
		TaggedValue getResult_;
		
		core::Bytes data;
		TaggedValue* currentVal;
		uint32 arrayStorageLength;
		TaggedValue arrayStorage[TaggedArray::kMaxElements];
		core::bufferStringStream::Formatter formatter;
	};
} }