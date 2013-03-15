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

#include "../../dependencies/libpng/png.h"
#include "../utils/path.h"

namespace data {
namespace image {
	void Reader::load(const char* name){
		//Get extension	
		auto ext = utils::path::extension(name);
		if(!strcmp(ext,"TGA") || !strcmp(ext,"tga")){
			loadTga(name);
		}
		else if(!strcmp(ext,"DDS") || !strcmp(ext,"dds")){
			loadDds(name);
		}
		else if(!strcmp(ext,"PNG") || !strcmp(ext,"png")){
			loadPng(name);
		}
		else
			services::logging()->resourceError("Unsupported image format",name);
	}

	static void png_cexcept_error(png_structp png_ptr, png_const_charp msg){
		services::logging()->resourceError(msg,"PNG image");
	}

	void Reader::loadPng(const char* name){
		io::File file(name,io::File::Read);
		auto logging = services::logging();

		/* first check the eight byte PNG signature */

		png_byte signature[8];
		fread(signature, 1, 8, file);
		if (png_sig_cmp(signature, 0, 8)) {
			logging->resourceError("Invalid PNG header!",name);
			return;
		}

		/* create the two png(-info) structures */

		auto png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,png_cexcept_error,nullptr);
		auto info_ptr = png_create_info_struct(png_ptr);
		if(!png_ptr || !info_ptr){
			if(png_ptr) png_destroy_read_struct(&png_ptr, NULL, NULL);
			logging->resourceError("Failed PNG read",name);
			return;
		}

		/* initialize the png structure */

		png_init_io(png_ptr,file.fptr);
		png_set_sig_bytes(png_ptr, 8);
        png_read_info(png_ptr, info_ptr);

        /* get width, height, bit-depth and color-type */
		uint32 width,height;
		int bitDepth,colourType;
        png_get_IHDR(png_ptr, info_ptr,&width,&height, &bitDepth,&colourType, NULL, NULL, NULL);

		auto nuberOfPasses = png_set_interlace_handling(png_ptr);
		png_read_update_info(png_ptr,info_ptr);

		png_bytep* row_pointers = (png_bytep*) new png_bytep[height];
		auto rowBytes = png_get_rowbytes(png_ptr,info_ptr);
		for(uint32 y = 0;y<height;++y){
			row_pointers[y] = (png_byte*) core::memory::globalAllocator()->allocate(rowBytes);
		}
		png_read_image(png_ptr,row_pointers);

		rendering::texture::Descriptor2D descriptor;
		int bytespp;
		if(colourType == PNG_COLOR_TYPE_RGB){
			descriptor.format = rendering::texture::RGB_888;
			bytespp = 3;
		}
		else if(colourType == PNG_COLOR_TYPE_RGBA){
			descriptor.format = rendering::texture::RGBA_8888;
			bytespp = 4;
		}
		else if(colourType == PNG_COLOR_TYPE_GRAY){
			descriptor.format = rendering::texture::R_8;
			bytespp = 1;
		}
		else {
			logging->resourceError("Unsupported PNG colour format",name);
			return;
		}
		descriptor.width  = width;
		descriptor.height = height;
		size_t size = width*height*bytespp;

		uint8* texels = (uint8*)core::memory::globalAllocator()->allocate(size);

		// Copy data
		uint8* dest = texels;
		for(uint32 y = 0;y<height;++y){
			auto row = row_pointers[y];
			memcpy(dest,row,width*bytespp);
			dest+=width*bytespp;
		}

		// Process data
		processData(descriptor,texels,size,1);

		core::memory::globalAllocator()->deallocate(texels);

		png_destroy_info_struct(png_ptr,&info_ptr);
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		
	}
	void Reader::loadTga(const char* name){
		#pragma pack (push, 1)
		struct  TgaHeader {
			uint8    idLength;
			uint8    colormapType;
			uint8    imageType;
			uint16   colormapIndex;
			uint16   colormapLength;
			uint8    colormapEntrySize;
			uint16   xOrigin;
			uint16   yOrigin;
			uint16   width;
			uint16   height;
			uint8    pixelSize;
			uint8    imageDesc;
		};//6+6(2)=18 bytes
		#pragma pack (pop)
		//Loading TGA files
		assert(sizeof(TgaHeader) == 18);
		io::File file(name,io::File::Read);
		TgaHeader header;
		fread(&header,sizeof(header),1,file.fptr);
		//uncompressed non palleted image
		if ( header.imageType != 0 && header.imageType != 1 && header.imageType != 2 ){
			services::logging()->resourceError("Unsupported TGA image format",name);
			return;
		}

		int width = header.width, height = header.height, bytespp = header.pixelSize/8;
		auto size = width*height*bytespp;
		auto texels = new uint8[size];
		fread(texels,1,size,file.fptr);
		//swap blue and red channel
		if(bytespp>=2) {
			for(int i=0;i<size;i+=bytespp){
				auto temp=texels[i];	
				texels[i] = texels[i+2]; 
				texels[i+2] = temp;
			}
		}

		rendering::texture::Descriptor2D descriptor;
		descriptor.width  = width;
		descriptor.height = height;
		descriptor.format = bytespp == 4? rendering::texture::RGBA_8888 : bytespp == 1? rendering::texture::R_8 : rendering::texture::RGB_888;

		processData(descriptor,texels,size,1);

		delete[] texels;
	}
	void Reader::loadDds(const char* name){
		io::File file(name,io::File::Read);
	}
} }

#endif

#endif //ARPHEG_RESOURCES_NO_FORMATTED