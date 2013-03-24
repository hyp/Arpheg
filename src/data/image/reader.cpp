#ifndef ARPHEG_RESOURCES_NO_FORMATTED

#include <string.h>
#include "../../core/platform.h"
#include "../../core/assert.h"
#include "../../core/io.h"
#include "../../services.h"
#include "../data.h"
#include "reader.h"

#ifdef ARPHEG_PLATFORM_MARMALADE
#include <IwImage.h> 
namespace data {
namespace image {
	void Reader::load(const char* name){
		CIwImage image;
		image.LoadFromFile(name);

		rendering::texture::Descriptor2D descriptor;
		descriptor.width  = image.GetWidth();
		descriptor.height = image.GetHeight();
		descriptor.format = image.HasAlpha()? rendering::texture::RGBA_8888 : rendering::texture::RGB_888;
		auto format = image.GetFormat();

		auto destFormat = image.HasAlpha()? CIwImage::ABGR_8888 : CIwImage::BGR_888;
		if(image.GetFormat() != destFormat){
			CIwImage dest;
			dest.SetFormat(destFormat);
			image.ConvertToImage(&dest);
			processData(descriptor,dest.GetTexels(),1);
		}
		else processData(descriptor,image.GetTexels(),1);
	}
} }
#else

#include "../../dependencies/stb/stb_image.h"
#include "../utils/path.h"

namespace data {
namespace image {
	void Reader::load(const char* name){
		//Get extension	
		auto ext = utils::path::extension(name);

		if(!strcmp(ext,"DDS") || !strcmp(ext,"dds")){
			loadDds(name);
		}
		else {
			int x,y,n;
			uint8* data = (uint8*)stbi_load(name, &x, &y, &n, 0);
			if(!data){
				services::logging()->resourceError("Unsupported image format",name);
				return;
			}
		
			rendering::texture::Descriptor2D descriptor;
			descriptor.width  = x;
			descriptor.height = y;
			static rendering::texture::Format fmt[4] = {
				rendering::texture::R_8,rendering::texture::RG_88,rendering::texture::RGB_888,rendering::texture::RGBA_8888
			};
			descriptor.format = fmt[n-1];
			processData(descriptor,data,x*y*n,1);
			stbi_image_free(data);
		}
	}

	void Reader::loadDds(const char* name){
		io::File file(name,io::File::Read);
	}
} }

#endif

#endif //ARPHEG_RESOURCES_NO_FORMATTED