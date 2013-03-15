#pragma once

#include "../core/types.h"
#include "../core/memoryTypes.h"
#include "../rendering/types.h"
#include "../data/types.h"
#include "types.h"

namespace ui {

class Service {
public:
	enum {
		kTexturedColouredTrianglesBatch = 0,
		kColouredTrianglesBatch = 1,
	};

	void setFocus(Widget* widget);
	inline Widget* focused() const;
	inline Group* root() const;
	inline rendering::ui::Service* renderer() const;
	core::Allocator* componentAllocator() const;

	void loadData(data::ID bundle);
	
	//Tasks(Not parallel)
	void updateWidgets(); //NB: this invokes the connected callbacks.
	void drawWidgets();

	//Rendering dispatch (called from rendering thread)
	void render();

	void servicePreStep();
	void servicePostStep();
	Service(core::Allocator* allocator);
	~Service();

	void enterLayer(rendering::Texture2D t);
private:
	rendering::ui::Service* renderer_;
	Group* root_;
	Widget* focused_;
	data::Pipeline* textPipeline,*outlineTextPipeline;
	data::Pipeline* uiPipeline;
	rendering::Texture2D uiAtlas;
	
};
inline rendering::ui::Service* Service::renderer() const { return renderer_; }
inline Group*  Service::root()    const { return root_;    }
inline Widget* Service::focused() const { return focused_; }

}