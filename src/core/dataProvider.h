#pragma once

#include "bytes.h"

namespace io {

	class DataProvider {
	public:
		virtual core::Bytes get() = 0;
	};
}