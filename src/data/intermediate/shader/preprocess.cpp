#include <string.h>
#include "../../../core/memory.h"
#include "../../../core/bufferStringStream.h"
#include "../../../core/text.h"
#include "preprocess.h"

namespace data {
namespace intermediate {
namespace shader {

Preprocessor::Preprocessor(core::Allocator* allocator) : dest(1024*8,allocator,core::BufferAllocator::GrowOnOverflow) {
}
core::Bytes Preprocessor::preprocess(application::logging::Service* logger,core::Bytes source){
	dest.reset();
	using namespace core::bufferStringStream;
	static const char* requireCommand = "require";
	Formatter idBuffer;

	for(core::text::Lines lines(source);!lines.empty();){
		auto line = lines.getLineTrimmedSkipEmpty();
		if(line.begin[0] == '#'){
			auto prevLine = line;
			line.begin++;
			auto cmd = core::text::parseAsciiUnderscore(line);
			if( cmd.length() >= 7 && memcmp(cmd.begin,requireCommand,7) == 0){
				core::text::skipSpaces(line);
				auto bundle = core::text::parseAsciiUnderscore(line);
				bool err = line.empty() || line.begin[0] != '.';
				if(!err){
					line.begin[0] = '\0';//Zero terminate bundle string.
					line.begin++;
					err = line.empty();
				}
				if(err){
					printf(idBuffer.allocator,"Error: expected an asset id after #require <bundle-name>.");
					logger->error(asCString(idBuffer.allocator));
					continue;
				}
				idBuffer.allocator.reset();
				idBuffer.allocator<<line;//Get the asset id
				dest<<require((const char*)bundle.begin,asCString(idBuffer))<<'\n';
				continue;
			}
			line = prevLine;
		}
		dest<<line<<'\n';
	}

	return core::Bytes(dest.bufferBase(),dest.size());
}

} } }