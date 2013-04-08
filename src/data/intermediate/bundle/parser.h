#pragma once

#include "../../../core/memory.h"
#include "../../internal/types.h"
#include "../../text/parser.h"

namespace data {
namespace intermediate {
namespace bundle {

	class Parser;

	//SubParser is used to parse different types of data from a single bundle file
	class SubParser {
	public:
		inline static bool equals(core::Bytes id,const char* str) { return text::Parser::compare(id,str); }

		virtual void begin();
		virtual void set(core::Bytes id);
		virtual text::Parser::SubDataHandling handleSubdata(core::Bytes id);
		virtual void subdata(core::Bytes data);
		virtual void end();

		Parser* parser;
	};

	class Parser : public text::Parser {
	public:

		Parser();
		void parse(core::Bytes bundlePath,core::Bytes bytes,data::internal_::Service* service,data::internal_::ResourceBundle* bundle);
	
		void set(core::Bytes id);
		SubDataHandling handleSubdata(core::Bytes id);
		void subdata(core::Bytes data);
		void endSubdata();

		void registerSubdata(const char* id,size_t size,SubParser* (* factory)(void* ));
		template<typename T>
		void registerSubdata(const char* id);
		SubParser* identifySubdata(core::Bytes id);
		
		void pushPath(core::Bytes subpath);
		void popPath();
		const char* makePath(core::Bytes bytes);

		data::internal_::Service* service;
		data::internal_::ResourceBundle* bundle;
		core::Bytes currentPath_, bundlePath_;

		//Subparser and storage
		SubParser* subparser;
		core::BufferAllocator subparserBuffer_;
		uint64 subparserBufferStorage_[256/8];
		//Storage for the data object
		void* data;
		uint64 dataBuffer[1024/8];
		
		//Path management
		//Storage for path formatting
		core::BufferAllocator pathBuffer;
		char pathBufferStorage[2048];	
		//Storage for path stack
		core::BufferAllocator pathBufferStack;
		char pathBufferStackStorage[2048];
	};


	template<typename T>
	void Parser::registerSubdata(const char* id){
		struct Factory {
			static SubParser* make(void* p){
				return new(core::memory::align_forward(p,alignof(T))) T; //NB: T is now guaranteed to be a descendand of SubParser
			}
		};
		registerSubdata(id,sizeof(T)+alignof(T),&Factory::make);
	}

} } }