#include "../core/branchHint.h"
#include "../services.h"
#include "2d.h"

namespace rendering {

	Layer2D::Effect::Effect() : matrix("matrix"), texture0("texture0") {
	}
	Layer2D::Layer2D() {
			auto renderer = services::rendering();
			const char* vertex = "attribute vec2 position; attribute vec2 texcoord; uniform highp mat4 matrix; varying mediump vec2 uv; void main() { gl_Position = matrix * vec4(position.xy,0,1); uv = texcoord; }";
			const char* pixel =  "varying mediump vec2 uv; uniform sampler2D texture0; void main() { gl_FragColor = texture2D(texture0,uv); }";
			texturedPrimitiveShaders[0] = renderer->create(Shader::Vertex,vertex);
			texturedPrimitiveShaders[1] = renderer->create(Shader::Pixel,pixel);
			texturedPrimitiveEffect.pipeline = renderer->create(VertexDescriptor::positionAs2Float_texcoordAs2Float(),texturedPrimitiveShaders[0],texturedPrimitiveShaders[1]);

			projection_ = mat44f::identity();
			view(mat44f::identity());
	}
	Layer2D::~Layer2D() {
		services::rendering()->release(texturedPrimitiveEffect.pipeline);
		services::rendering()->release(texturedPrimitiveShaders[0]);
		services::rendering()->release(texturedPrimitiveShaders[1]);
	}
	void Layer2D::projection(const mat44f& matrix) {
		projection_ = matrix;
		projView = projection_ * view_;
	}
	void Layer2D::view(const mat44f& matrix) {
		view_ = matrix;
		projView = projection_ * view_;
	}
	void Layer2D::bind(Service* renderer){
		this->renderer = renderer;
		//Default projection matrix
		auto size = renderer->context()->frameBufferSize();
		projection(mat44f::ortho(vec2f(0.0,0.0),vec2f(float(size.x),float(size.y))));
		//Enable blending.
		auto blendState = rendering::blending::State(rendering::blending::SrcAlpha,rendering::blending::InvertedSrcAlpha);
		renderer->bind(blendState);	
		currentEffect = &texturedPrimitiveEffect;
		renderer->bind(currentEffect->pipeline);
	}
	void Layer2D::bind(rendering::Texture2D texture) {
		int32 slot = 0;
		renderer->bind(texture,slot);
		renderer->bind(currentEffect->texture0,&slot);
	}
	void Layer2D::bind(Effect* effect) {
		currentEffect = effect;
		renderer->bind(currentEffect->pipeline);
	}
	void Layer2D::drawQuad(uint32 bufferOffset,vec2f position,vec2f rotation,vec2f scale) {
		mat44f pvm = mat44f::translateRotateScale2D(projView,position,rotation,scale);
		renderer->bind(currentEffect->matrix,pvm);
		renderer->bind(rendering::topology::TriangleStrip);
		renderer->draw(bufferOffset,4);
	}

	void Layer2D::allocateSpriteGeometry(resources::Sprite* sprite,vec2f* frameUVdata) {
		auto frameCount = sprite->frameCount();
		uint32 startingOffset = 0;
		auto renderer = services::rendering();
		for(uint32 i = 0;i < frameCount;++i){
			vec2f* coords = frameUVdata + i*2;
			vec2f data[] = { vec2f(-1,-1),coords[0] , vec2f(1,-1),vec2f(coords[1].x,coords[0].y)  , vec2f(-1,1),vec2f(coords[0].x,coords[1].y) ,  vec2f(1,1),coords[1] };
			renderer->update(Buffer::Vertex,sprite->buffer,startingOffset,data,sizeof(data));
		}
	}


	void Layer2D::bind(const components::Transform2D& transformation) {
		mat44f pvm = mat44f::translateRotateScale2D(projView,transformation.translation,transformation.rotation,transformation.scale);
		renderer->bind(currentEffect->matrix,pvm);
	}
	void Layer2D::draw(const components::Sprite& entity,const components::Transform2D& transformation) {
		renderer->bindVertices(entity.sprite->buffer);
		bind(transformation);
		int32 slot = 0;
		renderer->bind(entity.sprite->texture,slot);
		renderer->bind(currentEffect->texture0,&slot);
		renderer->bind(rendering::topology::TriangleStrip);
		renderer->draw(entity.sprite->frame(entity.currentFrame)->bufferOffset,4);
	}
}
