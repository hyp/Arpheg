#include "../core/assert.h"
#include "../services.h"
#include "ui.h"
#include "widget.h"
#include "components.h"

int main() {
	services::init();
	services::application()->mainWindow()->create("UI unittest");

	ui::Widget testWidget;
	class Test {
	public:
		bool clicked;
		bool clicked2;
		void stuff(){
			clicked= true;
		}
		void stuff2(){
			clicked2 = true;
		}
		Test(ui::Widget& widget){
			clicked = false;
			clicked2 = false;
			ARPHEG_UI_CONNECT(widget,clicked(),this,stuff());
		}
	};
	testWidget.onClick();
	Test test(testWidget);
	assert(test.clicked == false);
	testWidget.onClick();
	assert(test.clicked == true);
	ARPHEG_UI_CONNECT(testWidget,clicked(),test,stuff2());
	assert(test.clicked2 == false);
	test.clicked = false;
	testWidget.onClick();
	assert(test.clicked == true);
	assert(test.clicked2 == true);

	services::shutdown();
	return 0;
}