#include <stdio.h>

#include "types.h"
#include "logging.h"

namespace core {
namespace logging {

void Service::priority(Level level){
	this->level = level;
}
void Service::fatalSolution(const char* text,const char* solution) {
#ifdef ARPHEG_PLATFORM_MARMALADE
	s3eDebugErrorShow(S3E_MESSAGE_CONTINUE,text);//s3eDebugErrorPrintf("%s: %s",title,error);
#else
	printf("[Fatal Error]: %s.\n  %s.\n",text,solution);
#endif
}
void Service::message(Level level,const char* text) {
	if(this->level > level) return;
#ifdef ARPHEG_PLATFORM_MARMALADE
	s3eDebugErrorShow(S3E_MESSAGE_CONTINUE,text);//s3eDebugErrorPrintf("%s: %s",title,error);
#else
	printf("[Error]: %s\n",text);
#endif
}
void Service::resourceError(const char* text,const char* resource) {
	if(level > Error) return;
#ifdef ARPHEG_PLATFORM_MARMALADE
	s3eDebugErrorShow(S3E_MESSAGE_CONTINUE,text);
#else
	printf("[Resource Error]: \"%s\" %s\n",resource,text);
#endif
}
Service::Service(){
#if defined(DEBUG) || defined (_DEBUG) 
	level = Trace;
#else
	level = Information;
#endif
}
Service::~Service(){}

} }