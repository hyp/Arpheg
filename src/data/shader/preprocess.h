#pragma once

#include "../../core/memoryTypes.h"
#include "../../core/bytes.h"

//A shader preprocessor appends things like GL version for glsl shaders.

namespace data {
namespace shader {

struct Preprocessor {
	Preprocessor(core::Allocator* allocator);
	~Preprocessor();
	core::Bytes preprocess(core::Bytes source);

private:
	uint8* src;
	core::Allocator* allocator;
};

} }