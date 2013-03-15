#pragma once

#include "../types.h"

namespace data {
namespace image {
	class Reader {
	public:
		void load(const char* name);
	protected:
		virtual void processData(const rendering::texture::Descriptor2D& format,const void* data,size_t dataSize,uint32 stride) = 0;
	private:
		void loadTga(const char* name);
		void loadDds(const char* name);
		void loadPng(const char* name);
	};
} }