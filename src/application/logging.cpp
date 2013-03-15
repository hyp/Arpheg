#include <stdio.h>

#include "../core/platform.h"
#include "logging.h"

namespace application {
namespace logging {

void Service::priority(Level level){
	this->level = level;
}

static const char* headers[] = {
	"Trace",
	"Debug",
	"Information",
	"Warning",
	"Error",
	"Critical Error",
	"Fatal Error"
};

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
	printf("[%s]: %s\n",headers[level],text);
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