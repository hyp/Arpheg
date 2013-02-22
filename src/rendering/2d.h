/**
# rendering::Layer2D - a layer for 2D rendering.
Can draw sprites, images, shapes and text (TODO).
Supports unbatched and batched(TODO) sprite rendering.
*/
#pragma once

#include "rendering.h"
#include "geometryBatcher.h"
#include "../components/sprite.h"
#include "../components/transform2D.h"

namespace rendering {

	class Layer2D {
	public:
		struct Effect {
			rendering::Pipeline pipeline;
			rendering::Pipeline::Constant matrix;
			rendering::Pipeline::Constant texture0;
			Effect();
		};
	private:
		Service* renderer;
		mat44f projView;
		rendering::Shader   texturedPrimitiveShaders[2];
		
		
		mat44f projection_;
		mat44f view_;
	public:
		Effect* currentEffect;
		Effect texturedPrimitiveEffect;

		Layer2D();
		~Layer2D();

		void projection(const mat44f& matrix);
		void view      (const mat44f& matrix);
		inline mat44f& projectionView() { return projView; }

		void bind(rendering::Texture2D texture);
		void bind(Effect* effect);
		void drawQuad(uint32 bufferOffset,vec2f position,vec2f rotation,vec2f scale);

		//Sprite geometry managing.
		void allocateSpriteGeometry(resources::Sprite* sprite,vec2f* frameUVdata);

		//Call this before using this layer for this frame - performs setup for 2D rendering.
		void bind(Service* renderer);
		
		//Low level binding access
		//Rebuilt the ModelViewProjection matrix and bind it as a constant to the current pipeline.
		void bind(const components::Transform2D& transformation);

		//Direct unbatched rendering
		void draw(const components::Sprite& entity,const components::Transform2D& transformation);


	};

}