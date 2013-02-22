#pragma once

namespace core {
namespace logging {

enum Level {
	Trace,
	Debug,
	Information,
	Warning,
	Error,
	Critical,
	Fatal
};

class Service {
public:
	Service();
	~Service();

	void fatalSolution(const char* text,const char* solution= "");
	inline void fatal(const char* text);
	inline void critical(const char* text);
	void resourceError(const char* text,const char* resource = "");
	inline void error(const char* text);
	inline void warning(const char* text);
	inline void information(const char* text);
	inline void debug(const char* text);
	inline void trace(const char* text);

	inline Level priority();
	void priority(Level level);
protected:
	Level level;
	void message(Level level,const char* text);
};

inline void Service::fatal(const char* text){ message(Fatal,text); }
inline void Service::critical(const char* text){ message(Critical,text); }
inline void Service::error(const char* text){ message(Error,text); }
inline void Service::warning(const char* text){ message(Warning,text); }
inline void Service::information(const char* text){ message(Information,text); }
inline void Service::debug(const char* text){ message(Debug,text); }
inline void Service::trace(const char* text){ message(Trace,text); }
inline Level Service::priority(){ return level; }

} }