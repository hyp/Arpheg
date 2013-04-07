#include "../core/assert.h"
#include "../services.h"
#include "rendering.h"
#include "dynamicBuffer.h"

namespace rendering {

DynamicConstantBuffer::DynamicConstantBuffer(){
	buffer = Buffer::nullBuffer();
}
DynamicConstantBuffer::~DynamicConstantBuffer(){
	assert(services::tasking()->isRenderingThread());

	if(!buffer.isNull())
		services::rendering()->release(buffer);
}
void DynamicConstantBuffer::update(core::Bytes data){
	auto renderer = services::rendering();
	if(buffer.isNull()){
		buffer = renderer->create(Buffer::Constant,true,data.length(),data.begin);
	} else 
		renderer->recreate(Buffer::Constant,buffer,true,data.length(),data.begin);
}

DynamicBufferTexture::DynamicBufferTexture(){
	buffer = Buffer::nullBuffer();
}
DynamicBufferTexture::~DynamicBufferTexture(){
	assert(services::tasking()->isRenderingThread());

	if(!buffer.isNull()){
		auto renderer = services::rendering();
		renderer->release(buffer);
		renderer->release(textureView);
	}
}
void DynamicBufferTexture::update(texture::Format format, core::Bytes data){
	auto renderer = services::rendering();
	if(buffer.isNull()){
		buffer = renderer->create(Buffer::Vertex,true,data.length(),data.begin);
		textureView = renderer->create(uint32(format),buffer);
	} else 
		renderer->recreate(Buffer::Vertex,buffer,true,data.length(),data.begin);
}

}

