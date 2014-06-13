#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "menu.h"

#include <SDL.h>
#include <SDL_image.h>
#include "uae.h"
#include "options.h"
#include "zfile.h"
#include "sound.h"
#include <png.h>
#include "fade.h"
#include "xwin.h"

#include <SDL_gfxPrimitives.h>

#include <SDL_gp2x.h>

extern int bReloadKickstart;
#ifdef USE_GUICHAN
extern int mainMenu_displayHires;
extern int visibleAreaWidth;
extern int mainMenu_displayedLines;
#endif

SDL_Surface *current_screenshot = NULL;

#define VIDEO_FLAGS_INIT SDL_SWSURFACE|SDL_FULLSCREEN
#ifdef ANDROIDSDL
#define VIDEO_FLAGS VIDEO_FLAGS_INIT
#else
#define VIDEO_FLAGS VIDEO_FLAGS_INIT | SDL_DOUBLEBUF
#endif
SDL_Surface *text_screen=NULL, *text_image, *text_background, *text_window_background, *window_screen;

static Uint32 menu_inv_color=0, menu_win0_color=0, menu_win1_color=0;
static Uint32 menu_barra0_color=0, menu_barra1_color=0;
static Uint32 menu_win0_color_base=0, menu_win1_color_base=0;

void write_text_pos(int x, int y, char * str);
void write_num(int x, int y, int v);
int menu_msg_pos=330;
int menu_moving=1;
Uint32 menu_msg_time=0x12345678;
int skipintro=1;
int kickstart_warning=0;

static void obten_colores(void)
{
	FILE *f=fopen(DATA_PREFIX "colors.txt", "rt");
	if (f)
	{
		Uint32 r,g,b;
		fscanf(f,"menu_inv_color=0x%X,0x%X,0x%X\n",&r,&g,&b);
		menu_inv_color=SDL_MapRGB(text_screen->format,r,g,b);
		fscanf(f,"menu_win0_color=0x%X,0x%X,0x%X\n",&r,&g,&b);
		menu_win0_color=SDL_MapRGB(text_screen->format,r,g,b);
		fscanf(f,"menu_win1_color=0x%X,0x%X,0x%X\n",&r,&g,&b);
		menu_win1_color=SDL_MapRGB(text_screen->format,r,g,b);
		fscanf(f,"menu_barra0_color=0x%X,0x%X,0x%X\n",&r,&g,&b);
		menu_barra0_color=SDL_MapRGB(text_screen->format,r,g,b);
		fscanf(f,"menu_barra1_color=0x%X,0x%X,0x%X\n",&r,&g,&b);
		menu_barra1_color=SDL_MapRGB(text_screen->format,r,g,b);
		fclose(f);
	}
	else
	{
		menu_inv_color=SDL_MapRGB(text_screen->format, 0x20, 0x20, 0x40);
		menu_win0_color=SDL_MapRGB(text_screen->format, 0x10, 0x08, 0x08);
		menu_win1_color=SDL_MapRGB(text_screen->format, 0x20, 0x10, 0x10);
		menu_barra0_color=SDL_MapRGB(text_screen->format, 0x30, 0x20, 0x20);
		menu_barra1_color=SDL_MapRGB(text_screen->format, 0x50, 0x40, 0x40);
	}
	menu_win0_color_base=menu_win0_color;
	menu_win1_color_base=menu_win1_color;
}

#define  systemRedShift      (prSDLScreen->format->Rshift)
#define  systemGreenShift    (prSDLScreen->format->Gshift)
#define  systemBlueShift     (prSDLScreen->format->Bshift)
#define  systemRedMask       (prSDLScreen->format->Rmask)
#define  systemGreenMask     (prSDLScreen->format->Gmask)
#define  systemBlueMask      (prSDLScreen->format->Bmask)

int save_png(SDL_Surface* surface,char *path)
{
  int w = surface->w;
  int h = surface->h;
  unsigned char * pix = (unsigned char *)surface->pixels;
  unsigned char writeBuffer[1024 * 3];
  FILE *f  = fopen(path,"wb");
  if(!f) return 0;
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                NULL,
                                                NULL,
                                                NULL);
  if(!png_ptr) {
    fclose(f);
    return 0;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);

  if(!info_ptr) {
    png_destroy_write_struct(&png_ptr,NULL);
    fclose(f);
    return 0;
  }

  png_init_io(png_ptr,f);

  png_set_IHDR(png_ptr,
               info_ptr,
               w,
               h,
               8,
               PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr,info_ptr);

  unsigned char *b = writeBuffer;

  int sizeX = w;
  int sizeY = h;
  int y;
  int x;

unsigned short *p = (unsigned short *)pix;
for(y = 0; y < sizeY; y++) 
{
   for(x = 0; x < sizeX; x++) 
   {
     unsigned short v = p[x];

     *b++ = ((v & systemRedMask  ) >> systemRedShift  ) << 3; // R
     *b++ = ((v & systemGreenMask) >> systemGreenShift) << 2; // G 
     *b++ = ((v & systemBlueMask ) >> systemBlueShift ) << 3; // B
   }
   p += surface->pitch / 2;
   png_write_row(png_ptr,writeBuffer);
   b = writeBuffer;
}

  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  fclose(f);
  return 1;
}


void CreateScreenshot(int code)
{
	int w, h;

  if(current_screenshot != NULL)
  {
    SDL_FreeSurface(current_screenshot);
    current_screenshot = NULL;
  }

  if (code == SCREENSHOT)
  {
		w=prSDLScreen->w;
		h=prSDLScreen->h;
	}
	else 
	{ 
		w=32;
		h=32;
	}
	current_screenshot = SDL_CreateRGBSurface(prSDLScreen->flags,w,h,prSDLScreen->format->BitsPerPixel,prSDLScreen->format->Rmask,prSDLScreen->format->Gmask,prSDLScreen->format->Bmask,prSDLScreen->format->Amask);
  SDL_BlitSurface(prSDLScreen, NULL, current_screenshot, NULL);
}


int save_thumb(int code,char *path)
{
//	CreateScreenshot(code);
	int ret = 0;
	if(current_screenshot != NULL)
	  ret = save_png(current_screenshot, path);
	return ret;
}


void menu_raise(void)
{
	int i;
	for(i=80;i>=0;i-=16)
	{
#if !defined(NO_SOUND) && defined(MENU_MUSIC)
		Mix_VolumeMusic(MUSIC_VOLUME-(i<<1));
#endif
		text_draw_background();
		fade16(text_screen,i);
		text_flip();
		SDL_Delay(10);
	}
}

void menu_unraise(void)
{
	int i;
	for(i=0;i<=80;i+=16)
	{
#if !defined(NO_SOUND) && defined(MENU_MUSIC)
		Mix_VolumeMusic(MUSIC_VOLUME-(i<<1));
#endif
		text_draw_background();
		fade16(text_screen,i);
		text_flip();
		SDL_Delay(10);
	}
}

static void update_window_color(void)
{
	static int cambio=0;
	static int spin=0;

	Uint8 r,g,b;
	int cambio2=cambio>>3;
	SDL_GetRGB(menu_win0_color_base,text_screen->format,&r,&g,&b);
	if (((int)r)-cambio2>0) r-=cambio2;
	else r=0;
	if (((int)g)-cambio2>0) g-=cambio2;
	else g=0;
	if (((int)b)-cambio2>0) b-=cambio2;
	else b=0;
	menu_win0_color=SDL_MapRGB(text_screen->format,r,g,b);
	SDL_GetRGB(menu_win1_color_base,text_screen->format,&r,&g,&b);
	if (((int)r)-cambio>0) r-=cambio;
	else r=0;
	if (((int)g)-cambio>0) g-=cambio;
	else g=0;
	if (((int)b)-cambio>0) b-=cambio;
	else b=0;
	menu_win1_color=SDL_MapRGB(text_screen->format,r,g,b);
	if (spin)
	{
		if (cambio<=0) spin=0;
		else cambio-=2;

	}
	else
	{
		if (cambio>=24) spin=1;
		else cambio+=2;
	}
}

void text_draw_background()
{
	static int pos_x=12345678;
	static int pos_y=12345678;
	SDL_Rect r;
	int i,j;
	int w=text_screen->w+text_background->w-1;
	int h=text_screen->h+text_background->h-1;

	if (menu_moving)
	{
		if (pos_x>=0) pos_x=-text_screen->w;
		else pos_x++;
		if (pos_y>=0) pos_y=-text_screen->h;
		else pos_y++;
	}

	for(i=pos_x;i<w;i+=text_background->w)
		for(j=pos_y;j<h;j+=text_background->h)
		{
			r.x=i;
			r.y=j;
			r.w=text_background->w;
			r.h=text_background->h;
			SDL_BlitSurface(text_background,NULL,text_screen,&r);
		}
	if (menu_moving)
		update_window_color();
}

void text_flip(void)
{
	SDL_Delay(10);
	SDL_BlitSurface(text_screen,NULL,prSDLScreen,NULL);	
	SDL_Flip(prSDLScreen);
}

void init_kickstart()
{
	if (uae4all_init_rom(romfile))
	{
#ifndef USE_GUICHAN
		SDL_Event ev;
		text_draw_background();
		text_draw_window(2, 6, 42, 12, "--- ERROR ---");
		write_text(6, 12, romfile);
		write_text(8, 14, "not found!");
		write_text(8, 16, "Please check settings");
		write_text(8, 18, "Press any button to continue");
		text_flip();
		SDL_Delay(333);
		while (SDL_PollEvent(&ev))
				SDL_Delay(10);
		while (!SDL_PollEvent(&ev))
			SDL_Delay(10);
		while (SDL_PollEvent(&ev))
			if (ev.type == SDL_QUIT)
				exit(1);
		text_draw_background();
		text_flip();
		SDL_Delay(333);
#endif
		kickstart_warning=1;
	}
	else
	{
		kickstart_warning=0;
		bReloadKickstart=1;
	}
}

void init_text(int splash)
{
	char fname[256];
	
	SDL_Surface *tmp;

	if (!text_screen)
	{
		text_screen=SDL_CreateRGBSurface(prSDLScreen->flags,prSDLScreen->w,prSDLScreen->h,prSDLScreen->format->BitsPerPixel,prSDLScreen->format->Rmask,prSDLScreen->format->Gmask,prSDLScreen->format->Bmask,prSDLScreen->format->Amask);
		window_screen=SDL_CreateRGBSurface(prSDLScreen->flags,prSDLScreen->w,prSDLScreen->h,prSDLScreen->format->BitsPerPixel,prSDLScreen->format->Rmask,prSDLScreen->format->Gmask,prSDLScreen->format->Bmask,prSDLScreen->format->Amask);
		tmp=SDL_LoadBMP(MENU_FILE_TEXT);
		if (text_screen==NULL || tmp==NULL)
			exit(-1);
		text_image=SDL_DisplayFormat(tmp);
		SDL_FreeSurface(tmp);
		if (text_image==NULL)
			exit(-2);
		SDL_SetColorKey(text_image,(SDL_SRCCOLORKEY | SDL_RLEACCEL),SDL_MapRGB(text_image -> format, 0, 0, 0));
		tmp=SDL_LoadBMP(MENU_FILE_BACKGROUND);
		if (tmp==NULL)
			exit(-3);
		text_background=SDL_DisplayFormat(tmp);
		SDL_FreeSurface(tmp);
		if (text_background==NULL)
			exit(-3);
		tmp=SDL_LoadBMP(MENU_FILE_WINDOW);
		if (tmp==NULL)
			exit(-4);
		SDL_Rect dest;
		dest.w=32;
		dest.h=24;
		for (int y=0;y<10;y++)
		{
			//text_window_background
			dest.y=24*y;
			for(int x=0;x<10;x++)
			{
				dest.x=32*x;
				SDL_BlitSurface(tmp,NULL,window_screen,&dest);
			}
		}
		SDL_FreeSurface(tmp);
	}
	if (splash)
	{
		SDL_Surface *sur;
		SDL_Rect r;
		int i,j;

		obten_colores();
		if (skipintro) 
			goto skipintro;
		tmp=SDL_LoadBMP(MENU_FILE_SPLASH);
		if (tmp==NULL)
			exit(-6);
		sur = SDL_DisplayFormat(tmp);
		SDL_FreeSurface(tmp);
		r.x=(text_screen->w - sur->w)/2;
		r.y=(text_screen->h - sur->h)/2;
		r.h=sur->w;
		r.w=sur->h;
		SDL_FillRect(text_screen,NULL,0xFFFFFFFF);
		for (i=128;i>-8;i-=8)
		{
			SDL_Delay(50);
			SDL_FillRect(text_screen,NULL,0xFFFFFFFF);
			SDL_BlitSurface(sur,NULL,text_screen,&r);
			fade16(text_screen,i);
			text_flip();
		}
		SDL_Delay(3000);
		for(i=0;i<128;i+=16)
		{
			SDL_Delay(50);
			SDL_FillRect(text_screen,NULL,0xFFFFFFFF);
			SDL_BlitSurface(sur,NULL,text_screen,&r);
			fade16(text_screen,i);
			text_flip();
		}
		for(i=128;i>-8;i-=8)
		{
			SDL_Delay(50);
			text_draw_background();
			fade16(text_screen,i);
			text_flip();
		}
		SDL_FreeSurface(sur);
	}
	else
	{
		SDL_FillRect(text_screen,NULL,0xFFFFFFFF);
		text_flip();
		uae4all_resume_music();
	}
skipintro:		
	menu_msg_time=SDL_GetTicks();
}

void quit_text(void)
{
	SDL_FreeSurface(text_image);
	text_image = NULL;
	SDL_FreeSurface(text_background);
	text_background = NULL;
	SDL_FreeSurface(text_window_background);
	text_window_background = NULL;
	SDL_FreeSurface(text_screen);
	text_screen = NULL;
}

void write_text_pos(int x, int y, char * str)
{
  int i, c;
  SDL_Rect src, dest;
  
  for (i = 0; i < strlen(str); i++)
    {
      c = -1;
      
      if (str[i] >= '0' && str[i] <= '9')
		c = str[i] - '0';
      else if (str[i] >= 'A' && str[i] <= 'Z')
		c = str[i] - 'A' + 10;
      else if (str[i] >= 'a' && str[i] <= 'z')
		c = str[i] - 'a' + 36;
      else if (str[i] == '#')
		c = 62;
      else if (str[i] == '=')
		c = 63;
      else if (str[i] == '.')
		c = 64;
      else if (str[i] == '_')
		c = -2;
      else if (str[i] == '-')
		c = -3;
      else if (str[i] == '(')
		c = 65;
      else if (str[i] == ')')
		c = 66;
      
		if (c >= 0)
		{
		  src.x = c * 7;
		  src.y = 0;
		  src.w = 7;
		  src.h = 8;
		  
		  dest.x = x + (i * 7);
		  dest.y = y;
		  dest.w = 7;
		  dest.h = 8;
		  
		  SDL_BlitSurface(text_image, &src,
				  text_screen, &dest);
		}
		else if (c == -2 || c == -3)
		{
		  dest.x = x + (i * 8);
		  
		  if (c == -2)
			dest.y = y  + 7;
		  else if (c == -3)
			dest.y = y  + 3;
		  
		  dest.w = 7;
		  dest.h = 1;
		  
		  SDL_FillRect(text_screen, &dest, menu_barra0_color);
		}
    }
}

void write_text(int x, int y, const char * str)
{
	int i, c;
	SDL_Rect src, dest;
  
	for (i = 0; i < strlen(str); i++)
    {
      c = -1;
	  
      
      if (str[i] >= '0' && str[i] <= '9')
		c = str[i] - '0';
      else if (str[i] >= 'A' && str[i] <= 'Z')
		c = str[i] - 'A' + 10;
      else if (str[i] >= 'a' && str[i] <= 'z')
		c = str[i] - 'a' + 36;
      else if (str[i] == '#')
		c = 62;
      else if (str[i] == '=')
		c = 63;
      else if (str[i] == '.')
		c = 64;
      else if (str[i] == '_')
		c = -2;
      else if (str[i] == '-')
		c = -3;
      else if (str[i] == '(')
		c = 65;
      else if (str[i] == ')')
		c = 66;
      
		if (c >= 0)
		{
		  src.x = c * 7;
		  src.y = 0;
		  src.w = 7;
		  src.h = 8;

		  dest.x = (x + i) * 7;
		  dest.y = y * 7; //10;
		  dest.w = 7;
		  dest.h = 8;
		  
		  SDL_BlitSurface(text_image, &src,
				  text_screen, &dest);
		}
		else if (c == -2 || c == -3)
		{
		  dest.x = (x + i) * 7;
		  
		  if (c == -2)
			dest.y = y * 7 /*10*/ + 7;
		  else if (c == -3)
			dest.y = y * 7 /*10*/ + 3;
		  dest.w = 7;
		  dest.h = 1;
		  
		  SDL_FillRect(text_screen, &dest, menu_barra0_color);
		}
		if (i>42)
			break;
    }
}

void write_text_inv(int x, int y, const char * str)
{
  SDL_Rect dest;
  dest.x = (x * 7) -2 ;
  dest.y = (y * 7) /*10*/ - 2;
  dest.w = (strlen(str) * 7) + 4;
  dest.h = 12;

  SDL_FillRect(text_screen, &dest, menu_inv_color);

  write_text(x, y, str);
}

void write_centered_text(int y, char * str)
{
  write_text(40 - (strlen(str) / 2), y/2, str);
}

void write_num(int x, int y, int v)
{
  char str[24];
  
  snprintf(str, 24, "%d", v);
  write_text(x, y, str);
}

void write_num_inv(int x, int y, int v)
{
  SDL_Rect dest;
  int i,l=1;

  for(i=10;i<1000000;i*=10)
	if (v/i)
		l++;
  	else
		break;
  	
  dest.x = (x * 7) -2 ;
  dest.y = (y * 8) /*10*/ - 2;
  dest.w = (l * 7) + 4;
  dest.h = 12;

  SDL_FillRect(text_screen, &dest, menu_inv_color);

  write_num(x, y, v);
}

void text_draw_window(int x, int y, int w, int h, const char *title)
{
	int i,j;
	int x_screen = x * 7;
	int y_screen = y * 8;

	int x_corn = x_screen-2;
	int y_corn = y_screen-12;
	int w_corn = (w*7)+4;
	int h_corn = (h*8)+4+12;

	int x_shadow = x_corn+w_corn;
	int y_shadow = y_corn+6;
	int w_shadow = 10;
	int h_shadow = h_corn+4;

	int x_shadow_d = x_corn+5;
	int y_shadow_d = y_corn+h_corn;
	int w_shadow_d = w_corn-5;
	int h_shadow_d = 10;

	SDL_Rect dest;

	dest.x = x_shadow;
	dest.y = y_shadow;
	dest.w = w_shadow;
	dest.h = h_shadow;

	SDL_FillRect(text_screen, &dest, menu_win0_color);

	dest.x = x_shadow_d;
	dest.y = y_shadow_d;
	dest.w = w_shadow_d;
	dest.h = h_shadow_d;

	SDL_FillRect(text_screen, &dest, menu_win0_color);

	dest.x = x_corn;
	dest.y = y_corn;
	dest.w = w_corn;

	dest.h = h_corn;
	SDL_FillRect(text_screen, &dest, menu_win1_color);

	dest.x=x_screen;
	dest.y=y_screen;
	dest.w=w*7;
	dest.h=h*8;
	SDL_BlitSurface(window_screen,&dest,text_screen,&dest);
	
	write_text(x + ((w-strlen(title)) / 2), y-1, title);
}

void _text_draw_window(SDL_Surface *sf, int x, int y, int w, int h, const char *title)
{
	SDL_Surface *back=text_screen;
	text_screen=sf;
	text_draw_window(x,y,w,h,title);
	text_screen=back;
}

void text_draw_barra(int x, int y, int w, int h, int per, int max)
{
	SDL_Rect dest;
	if (h>5)
		h-=4;
	dest.x=x-1;
	dest.y=y-1;
	dest.w=w+2;
	dest.h=h+2;
	SDL_FillRect(text_screen, &dest, menu_barra1_color);
	if (per>max)
		per=max;
	dest.x=x;
	dest.y=y;
	dest.h=h;
	dest.w=(w*per)/max;
	SDL_FillRect(text_screen, &dest, menu_barra0_color);
}

void text_draw_window_bar(int x, int y, int w, int h, int per, int max, const char *title)
{
	text_draw_window(x,y,w,h,title);
	text_draw_barra(x+4, y+28, w-24, 12, per, max);
	write_text((x/8)+4,(y/8)+1,"Please wait");
}

void _text_draw_window_bar(SDL_Surface *sf, int x, int y, int w, int h, int per, int max, const char *title)
{
	SDL_Surface *back=text_screen;
	text_screen=sf;
	text_draw_window_bar(x,y,w,h,per,max,title);
	text_screen=back;
}
