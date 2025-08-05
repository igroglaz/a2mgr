#include "Image.h"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <windows.h>
#include "zxmgr.h"
#include "lib\utils.hpp"
#include "File.h"
#include "a2mgr.h"

Image::Image(std::string filename)
{
	TryInitSDL();
	
	myPixels = NULL;
	myWidth = 0;
	myHeight = 0;

	File fil;
	if(!fil.Open(filename))
	{
		// produce system error
		MessageBoxA(zxmgr::GetHWND(), Format("FATAL ERROR: can't load %s", filename.c_str()).c_str(),
			"Allods2", MB_ICONWARNING | MB_OK);
		zxmgr::AfxAbort();
	}

	uint8_t* buffer = new uint8_t[fil.GetLength()];
	memset(buffer, 0, fil.GetLength());
	fil.Seek(0);
	uint32_t count_read = fil.Read(buffer, fil.GetLength());
	SDL_RWops* rw = SDL_RWFromMem(buffer, fil.GetLength());
	fil.Close();

	SDL_Surface* img = IMG_Load_RW(rw, false);
	if(!img || !img->w || !img->h)
	{
		if(img) SDL_FreeSurface(img);
		delete[] buffer;
		MessageBoxA(zxmgr::GetHWND(), Format("FATAL ERROR: can't load %s", filename.c_str()).c_str(),
			"Allods2", MB_ICONWARNING | MB_OK);
		zxmgr::AfxAbort();
	}

	SDL_PixelFormat pfd;
	pfd.palette = NULL;
	pfd.BitsPerPixel = 32;
	pfd.BytesPerPixel = 4;
	pfd.Rmask = 0xFF000000;
	pfd.Gmask = 0x00FF0000;
	pfd.Bmask = 0x0000FF00;
	pfd.Amask = 0x000000FF;
	pfd.Rshift = 24;
	pfd.Gshift = 16;
	pfd.Bshift = 8;
	pfd.Ashift = 0;
	pfd.Rloss = 0;
	pfd.Gloss = 0;
	pfd.Bloss = 0;
	pfd.Aloss = 0;
	pfd.alpha = 255;
	pfd.colorkey = 0;

	SDL_Surface* img_r = SDL_ConvertSurface(img, &pfd, 0);
	if(!img_r)
	{
		SDL_FreeSurface(img);
		delete[] buffer;
		MessageBoxA(zxmgr::GetHWND(), Format("FATAL ERROR: can't load %s", filename.c_str()).c_str(),
			"Allods2", MB_ICONWARNING | MB_OK);
		zxmgr::AfxAbort();
	}

	myWidth = img_r->w;
	myHeight = img_r->h;
	myPixels = new uint32_t[myWidth*myHeight];

	memcpy(myPixels, img_r->pixels, myWidth*myHeight*4);
	SDL_FreeSurface(img_r);
	delete[] buffer;
}

Image::~Image()
{
	if(myPixels) delete[] myPixels;
	myPixels = NULL;
	myWidth = 0;
	myHeight = 0;
}

void Image::Display(int16_t x, int16_t y)
{
	DisplayEx(x, y, 0, 0, myWidth, myHeight, false);
}

void Image::DisplayEx(int16_t x, int16_t y, int16_t inX, int16_t inY, int16_t w, int16_t h, bool no_alpha)
{
	if(!myPixels) return;
	if(*(uint32_t*)(0x00631770)) return;

	uint16_t* pPixels = *(uint16_t**)(0x0062571C);
	uint32_t pPitch = *(uint32_t*)(0x00625708);

	if(x < 0)
	{
		int16_t oldX = x;
		x = 0;
		inX = -oldX;
	}

	if(y < 0)
	{
		int16_t oldY = y;
		y = 0;
		inY = -oldY;
	}

	if(inX+w > (int16_t)myWidth)
		w = (myWidth-inX);
	if(inY+h > (int16_t)myHeight)
		h = (myHeight-inY);

	RECT pRect = *(RECT*)(0x00625768);
	if(x > pRect.right ||
		y > pRect.bottom) return;
	if(x+w < pRect.left ||
		y+h < pRect.top) return;

	int32_t pTotalWidth = pPitch / 2;
	int32_t inBorderLeft = x;
	int32_t inBorderRight = pTotalWidth-(w+inBorderLeft);

	if(inBorderLeft < 0 ||
		inBorderRight < 0) return;

	int32_t rBorderLeft = inX;
	int32_t rBorderRight = myWidth-(w+rBorderLeft);
	uint32_t* rPixels = myPixels;

	pPixels += y * pTotalWidth;
	rPixels += inY * myWidth;
	for(int y = 0; y < h; y++)
	{
		rPixels += rBorderLeft;
		pPixels += inBorderLeft;
		for(int x = 0; x < w; x++)
		{
			uint32_t src_color = *rPixels++;
			uint8_t src_r = (src_color & 0xFF000000) >> 27;
			uint8_t src_g = (src_color & 0x00FF0000) >> 18;
			uint8_t src_b = (src_color & 0x0000FF00) >> 11;
			uint8_t src_a = (src_color & 0x000000FF);
			if(no_alpha)
			{
				if(src_a > 127) src_a = 255;
				else src_a = 0;
			}

			if(!src_a) continue;

			uint16_t dst_color = *pPixels;
			uint8_t dst_r = (dst_color & 0xF800) >> 11;
			uint8_t dst_g = (dst_color & 0x07E0) >> 5;
			uint8_t dst_b = (dst_color & 0x001F);
			// max for blue&red = 31.0
			// max for green = 63.0
			
			uint8_t out_r;
			uint8_t out_g;
			uint8_t out_b;

			if(src_a != 255)
			{
				double alpha = (double)(src_a) / 255.0;
				double Fout_r = (src_r * alpha) + (dst_r * (1.0-alpha));
				double Fout_g = (src_g * alpha) + (dst_g * (1.0-alpha));
				double Fout_b = (src_b * alpha) + (dst_b * (1.0-alpha));

				out_r = min((uint8_t)Fout_r, 0x1F);
				out_g = min((uint8_t)Fout_g, 0x3F);
				out_b = min((uint8_t)Fout_b, 0x1F);
			}
			else
			{
				out_r = src_r;
				out_g = src_g;
				out_b = src_b;
			}

			dst_color = (out_r & 0x1F) << 11;
			dst_color |= (out_g & 0x3F) << 5;
			dst_color |= (out_b);
			*pPixels++ = dst_color;
		}
		rPixels += rBorderRight;
		pPixels += inBorderRight;
	}
}

uint32_t Image::GetWidth()
{
	return myWidth;
}

uint32_t Image::GetHeight()
{
	return myHeight;
}

uint32_t Image::GetPixelAt(int32_t x, int32_t y)
{
	if(!myPixels) return 0;
	if(x < 0 || x >= (int32_t)myWidth) return 0;
	if(y < 0 || y >= (int32_t)myHeight) return 0;
	return myPixels[y*myWidth+x];
}

uint32_t* Image::GetPixels()
{
	return myPixels;
}
