#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "menu.h"
#include<SDL.h>
#include <sys/stat.h>
#include <unistd.h>
#include<dirent.h>
#include "uae.h"
#include "options.h"
#include "sound.h"
#include "gp2x.h"

static const char *text_str_fileinfo_title=    "            File info              -";
char* fileInfo_fileName;

static void draw_fileinfoMenu(int c)
{
	int menuLine = 0;
	SDL_Rect r;
	extern SDL_Surface *text_screen;
	r.x=80-64; r.y=60; r.w=110+64+64; r.h=120;

	text_draw_background();
	text_draw_window(2,2,40,40,text_str_fileinfo_title);

	menuLine = 4;
	write_text(3, menuLine, "File info:");
	menuLine+=2;
	write_text(3, menuLine, "----------");
	menuLine+=4;

	// now wrap the filename if necessary (at 32)
	int i = 0;
	char line [40];
	const int LINELEN = 32;

	for (i = 0; i < strlen(fileInfo_fileName); i+=LINELEN)
	{
		strncpy(line, fileInfo_fileName + i, LINELEN);
		line[LINELEN] = '\0';
		write_text(3, menuLine, line);
		menuLine+=2;
	}
	
	text_flip();
}

static int key_fileinfoMenu(int *c)
{
	int end=0;
	int left=0, right=0, up=0, down=0, hit0=0, hit1=0;
	SDL_Event event;

	while (SDL_PollEvent(&event) > 0)
	{
		if (event.type == SDL_QUIT)
			end=-1;
		else if (event.type == SDL_KEYUP )
			end=-1;
		else if (event.type == SDL_JOYBUTTONUP )
			end=-1;
	}
	return end;
}

static void raise_fileinfoMenu()
{
	int i;

	text_draw_background();
	text_flip();
	for(i=0;i<10;i++)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_fileinfo_title);
		text_flip();
	}
}

static void unraise_fileinfoMenu()
{
	int i;

	for(i=9;i>=0;i--)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_fileinfo_title);
		text_flip();
	}
	text_draw_background();
	text_flip();
}

int run_menuFileinfo(char* fileName)
{
	int end=0,c=0;

	fileInfo_fileName = fileName;

	while(!end)
	{
		draw_fileinfoMenu(c);
		end=key_fileinfoMenu(&c);
	}

	return end;
}
