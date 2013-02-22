#pragma once

#include "../core/math.h"

namespace application {

class MainWindow {
public:

	void create(const char* title = "",vec2i size = vec2i(0,0));

	inline bool isCreated() const;
	inline vec2i size()   const;
	inline void* handle() const;

	void resize(vec2i size);

	MainWindow();
	~MainWindow();
private:
	bool  createdFirst_,created_;
	vec2i size_;
	void* handle_;
};

inline bool  MainWindow::isCreated() const { return created_; }
inline void* MainWindow::handle() const { return handle_; }
inline vec2i MainWindow::size()   const { return size_;   }

}