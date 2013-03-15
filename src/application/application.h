#pragma once

#include "../core/types.h"
#ifndef ARPHEG_PLATFORM_MOBILE
	#include "mainWindow.h"
#endif

namespace application {

class Service {
public:
	inline bool quitRequest() const;
	inline bool active() const;
	inline uint32 frameID() const ;
#ifndef ARPHEG_PLATFORM_MOBILE
	inline MainWindow* mainWindow();
#endif

	void quit();
	void activate(bool active);

	Service();
	~Service();
	void servicePreStep();
	void servicePostStep();
private:
	uint32 frameID_;
	bool quit_;
	bool active_;
#ifndef ARPHEG_PLATFORM_MOBILE
	MainWindow mainWindow_;
#endif
};

inline bool         Service::quitRequest() const { return quit_; }
#ifndef ARPHEG_PLATFORM_MOBILE
	inline MainWindow*  Service::mainWindow() { return &mainWindow_; }
#endif
inline uint32       Service::frameID()  const { return frameID_; }
inline bool			Service::active() const { return active_;    }

}