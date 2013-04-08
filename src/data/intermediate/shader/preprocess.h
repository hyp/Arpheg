#pragma once

#include "../../../core/memory.h"
#include "../../../core/bytes.h"
#include "../../../application/logging.h"

namespace data {
namespace intermediate {
namespace shader {

class Preprocessor {
public:
	explicit Preprocessor(core::Allocator* allocator = nullptr);
	core::Bytes preprocess(application::logging::Service* logger,core::Bytes source);

	virtual core::Bytes require(const char* bundle,const char* id) = 0;
private:
	core::BufferAllocator dest;
};

} } }