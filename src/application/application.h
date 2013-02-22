#pragma once

#include "../core/types.h"
#include "mainWindow.h"

namespace application {

class Service {
public:
	inline bool quitRequest() const;
	inline bool active() const;
	inline uint32 frameID() const ;
	inline MainWindow* mainWindow();

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
	MainWindow mainWindow_;
};

inline bool         Service::quitRequest() const { return quit_; }
inline MainWindow*  Service::mainWindow() { return &mainWindow_; }
inline uint32       Service::frameID()  const { return frameID_; }
inline bool			Service::active() const { return active_;    }

}