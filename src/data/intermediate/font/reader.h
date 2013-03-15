#pragma once

#include "../../font/font.h"

namespace data {
namespace intermediate {
namespace font {
	class Reader {
	public:
		Font result;

		void load(core::Bytes data);
		virtual void processTextureRequest(core::Bytes path) = 0;
	};
} } }