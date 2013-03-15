// A parser for the application configuration file
#pragma once

#include "../text/parser.h"

namespace data {
namespace config {

	class Parser : public text::Parser {
	public:
		typedef void (*SetFunc)(Parser* parser,core::Bytes id);
		typedef void (*EndFunc)(Parser* parser);

		Parser();
		
		void get(core::Bytes id);
		void set(core::Bytes id);
	};
	
	void loadFromTextFile(const char* filename);

} }