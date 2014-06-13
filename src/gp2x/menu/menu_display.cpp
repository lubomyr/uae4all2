#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "menu.h"
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "uae.h"
#include "options.h"
#include "sound.h"
#include <SDL.h>
#include "gp2x.h"
#include <SDL_ttf.h>
#include "menu_config.h"

const char *text_str_display_separator="----------------------------------";
const char *text_str_display_title=    "   Display and Sound Settings    ";
static const char *text_str_sound="Sound";
static const char *text_str_fast="fast";
static const char *text_str_accurate="accurate";
static const char *text_str_off="off";
static const char *text_str_sndrate="Sound rate";
static const char *text_str_44k="44k";
static const char *text_str_32k="32k";
static const char *text_str_22k="22k";
static const char *text_str_11k="11k";
static const char *text_str_8k="8k";

#define MAX_CUSTOM_ID 96
#define MIN_CUSTOM_ID -5

int menuDisplay = 0;

extern int moveY;
extern int screenWidth;
extern int sound_rate;

enum { 
	MENUDISPLAY_RETURNMAIN = 0,
	MENUDISPLAY_PRESETWIDTH,
	MENUDISPLAY_PRESETHEIGHT,
	MENUDISPLAY_DISPLINES,
	MENUDISPLAY_SCREENWIDTH,
	MENUDISPLAY_VERTPOS,
	MENUDISPLAY_CUTLEFT,
	MENUDISPLAY_CUTRIGHT,
	MENUDISPLAY_FRAMESKIP,
	MENUDISPLAY_REFRESHRATE,
	MENUDISPLAY_SOUND,
	MENUDISPLAY_SNDRATE,
	MENUDISPLAY_STEREO,
	MENUDISPLAY_END
};



static void draw_displayMenu(int c)
{
	int leftMargin=3;
	int tabstop1 = 17;
	int tabstop2 = 19;
	int tabstop3 = 21;
	int tabstop4 = 23;
	int tabstop5 = 25;
	int tabstop6 = 27;
	int tabstop7 = 29;
	int tabstop8 = 31;
	int tabstop9 = 33;

	int menuLine = 3;
	static int b=0;
	int bb=(b%6)/3;
	SDL_Rect r;
	extern SDL_Surface *text_screen;
	char value[20]="";
	r.x=80-64; r.y=0; r.w=110+64+64; r.h=240;

	text_draw_background();
	text_draw_window(2,2,40,30,text_str_display_title);

	// MENUDISPLAY_RETURNMAIN
	if (menuDisplay == MENUDISPLAY_RETURNMAIN && bb)
		write_text_inv(3, menuLine, "Return to main menu");
	else
		write_text(3, menuLine, "Return to main menu");

	menuLine++;
	write_text(leftMargin,menuLine,text_str_display_separator);
	menuLine++;

	// MENUDISPLAY_PRESETWIDTH
	write_text(leftMargin,menuLine,"Preset Width");
	snprintf(value, 20, "%d", visibleAreaWidth);
	if ((menuDisplay!=MENUDISPLAY_PRESETWIDTH)||(bb))
		write_text(tabstop3,menuLine,value);
	else
		write_text_inv(tabstop3,menuLine,value);

	// MENUDISPLAY_PRESETHEIGHT
	menuLine+=2;
	write_text(leftMargin,menuLine,"Preset Height");
	if ((menuDisplay!=MENUDISPLAY_PRESETHEIGHT)||(bb))
		write_text(tabstop3,menuLine,presetMode);
	else
		write_text_inv(tabstop3,menuLine,presetMode);

	menuLine++;
	write_text(leftMargin,menuLine,text_str_display_separator);
	menuLine+=2;
	write_text(leftMargin,menuLine,"Custom Settings");
	menuLine++;
	write_text(leftMargin,menuLine,"---------------");
	menuLine++;

	// MENUDISPLAY_DISPLINES
	write_text(leftMargin,menuLine,"Displayed Lines");
	sprintf(value, "%d", mainMenu_displayedLines);
	if ((menuDisplay!=MENUDISPLAY_DISPLINES)||(bb))
		write_text(tabstop3,menuLine,value);
	else
		write_text_inv(tabstop3,menuLine,value);

	// MENUDISPLAY_SCREENWIDTH
	menuLine+=2;
	write_text(leftMargin,menuLine,"Screen Width");
	sprintf(value, "%d", screenWidth);
	if ((menuDisplay!=MENUDISPLAY_SCREENWIDTH)||(bb))
		write_text(tabstop3,menuLine,value);
	else
		write_text_inv(tabstop3,menuLine,value);

	// MENUDISPLAY_VERTPOS
	menuLine+=2;
	write_text(leftMargin,menuLine,"Vertical Position");
	sprintf(value, "%d", moveY);
	if ((menuDisplay!=MENUDISPLAY_VERTPOS)||(bb))
		write_text(tabstop3,menuLine,value);
	else
		write_text_inv(tabstop3,menuLine,value);

	// MENUDISPLAY_CUTLEFT
	menuLine+=2;
	write_text(leftMargin,menuLine,"Cut Left");
	sprintf(value, "%d", mainMenu_cutLeft);
	if ((menuDisplay!=MENUDISPLAY_CUTLEFT)||(bb))
		write_text(tabstop3,menuLine,value);
	else
		write_text_inv(tabstop3,menuLine,value);

	// MENUDISPLAY_CUTRIGHT
	menuLine+=2;
	write_text(leftMargin,menuLine,"Cut Right");
	sprintf(value, "%d", mainMenu_cutRight);
	if ((menuDisplay!=MENUDISPLAY_CUTRIGHT)||(bb))
		write_text(tabstop3,menuLine,value);
	else
		write_text_inv(tabstop3,menuLine,value);

	// MENUDISPLAY_FRAMESKIP
	menuLine++;
	write_text(leftMargin,menuLine,text_str_display_separator);
	menuLine++;
	write_text(leftMargin,menuLine,"Frameskip");
	if ((mainMenu_frameskip==0)&&((menuDisplay!=MENUDISPLAY_FRAMESKIP)||(bb)))
		write_text_inv(tabstop1,menuLine,"0");
	else
		write_text(tabstop1,menuLine,"0");
#ifdef PANDORA
	if ((mainMenu_frameskip==1)&&((menuDisplay!=MENUDISPLAY_FRAMESKIP)||(bb)))
		write_text_inv(tabstop3,menuLine,"1");
	else
		write_text(tabstop3,menuLine,"1");
#else
	if ((mainMenu_frameskip==1)&&((menuDisplay!=MENUDISPLAY_FRAMESKIP)||(bb)))
		write_text_inv(tabstop2,menuLine,"1");
	else
		write_text(tabstop2,menuLine,"1");
	if ((mainMenu_frameskip==2)&&((menuDisplay!=MENUDISPLAY_FRAMESKIP)||(bb)))
		write_text_inv(tabstop3,menuLine,"2");
	else
		write_text(tabstop3,menuLine,"2");
	if ((mainMenu_frameskip==3)&&((menuDisplay!=MENUDISPLAY_FRAMESKIP)||(bb)))
		write_text_inv(tabstop4,menuLine,"3");
	else
		write_text(tabstop4,menuLine,"3");
	if ((mainMenu_frameskip==4)&&((menuDisplay!=MENUDISPLAY_FRAMESKIP)||(bb)))
		write_text_inv(tabstop5,menuLine,"4");
	else
		write_text(tabstop5,menuLine,"4");
	if ((mainMenu_frameskip==5)&&((menuDisplay!=MENUDISPLAY_FRAMESKIP)||(bb)))
		write_text_inv(tabstop6,menuLine,"5");
	else
		write_text(tabstop6,menuLine,"5");
	if ((mainMenu_frameskip==6)&&((menuDisplay!=MENUDISPLAY_FRAMESKIP)||(bb)))
		write_text_inv(tabstop7,menuLine,"6");
	else
		write_text(tabstop7,menuLine,"6");
	if ((mainMenu_frameskip==7)&&((menuDisplay!=MENUDISPLAY_FRAMESKIP)||(bb)))
		write_text_inv(tabstop8,menuLine,"7");
	else
		write_text(tabstop8,menuLine,"7");
	if ((mainMenu_frameskip==8)&&((menuDisplay!=MENUDISPLAY_FRAMESKIP)||(bb)))
		write_text_inv(tabstop9,menuLine,"8");
	else
		write_text(tabstop9,menuLine,"8");
#endif

	// MENUDISPLAY_REFRESHRATE
	menuLine+=2;
	write_text(leftMargin,menuLine,"Refresh Rate");
	if ((!mainMenu_ntsc)&&((menuDisplay!=MENUDISPLAY_REFRESHRATE)||(bb)))
		write_text_inv(tabstop1,menuLine,"50Hz");
	else
		write_text(tabstop1,menuLine,"50Hz");

	if ((mainMenu_ntsc)&&((menuDisplay!=MENUDISPLAY_REFRESHRATE)||(bb)))
		write_text_inv(tabstop3+1,menuLine,"60Hz");
	else
		write_text(tabstop3+1,menuLine,"60Hz");

	menuLine++;
	write_text(leftMargin,menuLine,text_str_display_separator);
	menuLine++;

	// MENUDISPLAY_SOUND
	write_text(leftMargin,menuLine,text_str_sound);
	if ((mainMenu_sound==0)&&((menuDisplay!=MENUDISPLAY_SOUND)||(bb)))
		write_text_inv(tabstop1,menuLine,text_str_off);
	else
		write_text(tabstop1,menuLine,text_str_off);

	if ((mainMenu_sound==1)&&((menuDisplay!=MENUDISPLAY_SOUND)||(bb)))
		write_text_inv(tabstop3,menuLine,text_str_fast);
	else
		write_text(tabstop3,menuLine,text_str_fast);

	if ((mainMenu_sound==2)&&((menuDisplay!=MENUDISPLAY_SOUND)||(bb)))
		write_text_inv(tabstop5+1,menuLine,text_str_accurate);
	else
		write_text(tabstop5+1,menuLine,text_str_accurate);

	// MENUDISPLAY_SNDRATE
	menuLine+=2;
	write_text(leftMargin,menuLine,text_str_sndrate);

	if ((sound_rate==8000)&&((menuDisplay!=MENUDISPLAY_SNDRATE)||(bb)))
		write_text_inv(tabstop1,menuLine,text_str_8k);
	else
		write_text(tabstop1,menuLine,text_str_8k);

	if ((sound_rate==11025)&&((menuDisplay!=MENUDISPLAY_SNDRATE)||(bb)))
		write_text_inv(tabstop3,menuLine,text_str_11k);
	else
		write_text(tabstop3,menuLine,text_str_11k);

	if ((sound_rate==22050)&&((menuDisplay!=MENUDISPLAY_SNDRATE)||(bb)))
		write_text_inv(tabstop5,menuLine,text_str_22k);
	else
		write_text(tabstop5,menuLine,text_str_22k);

	if ((sound_rate==32000)&&((menuDisplay!=MENUDISPLAY_SNDRATE)||(bb)))
		write_text_inv(tabstop7,menuLine,text_str_32k);
	else
		write_text(tabstop7,menuLine,text_str_32k);

	if ((sound_rate==44100)&&((menuDisplay!=MENUDISPLAY_SNDRATE)||(bb)))
		write_text_inv(tabstop9,menuLine,text_str_44k);
	else
		write_text(tabstop9,menuLine,text_str_44k);

	// MENUDISPLAY_STEREO
	menuLine+=2;
	write_text(leftMargin,menuLine,"Sound mode");
	if ((mainMenu_soundStereo==0)&&((menuDisplay!=MENUDISPLAY_STEREO)||(bb)))
		write_text_inv(tabstop1,menuLine,"mono");
	else
		write_text(tabstop1,menuLine,"mono");

	if ((mainMenu_soundStereo==1)&&((menuDisplay!=MENUDISPLAY_STEREO)||(bb)))
		write_text_inv(tabstop5,menuLine,"stereo");
	else
		write_text(tabstop5,menuLine,"stereo");
	
	text_flip();
	b++;
}

static int key_displayMenu(int *c)
{
	int end=0;
	int left=0, right=0, up=0, down=0, hit0=0, hit1=0;
	SDL_Event event;

	while (SDL_PollEvent(&event) > 0)
	{
		if (event.type == SDL_KEYDOWN)
		{
			uae4all_play_click();
			switch(event.key.keysym.sym)
			{
			case SDLK_RIGHT: right=1; break;
			case SDLK_LEFT: left=1; break;
			case SDLK_UP: up=1; break;
			case SDLK_DOWN: down=1; break;
			case SDLK_PAGEDOWN: hit0=1; break;
			case SDLK_HOME: hit0=1; break;
			case SDLK_LALT: hit1=1; break;
			case SDLK_END: hit0=1; break;
			case SDLK_PAGEUP: hit0=1;
			}
		}
	}

	if (hit0)
	{
		end = -1;
	}
	else if (hit1)
	{
		end = -1;
	}
	else if (up)
	{
		if (menuDisplay==0) menuDisplay = MENUDISPLAY_END - 1;
		else menuDisplay--;
	}
	else if (down)
	{
		if (menuDisplay == MENUDISPLAY_END - 1) menuDisplay=0;
		else menuDisplay++;
	}
	switch (menuDisplay)
	{
		case MENUDISPLAY_PRESETWIDTH:
			if (left)
			{
				if(presetModeId < 10)
					SetPresetMode(presetModeId + 50);
				else
					SetPresetMode(presetModeId - 10);
			}
			if(right)
			{
				if(presetModeId > 50)
					SetPresetMode(presetModeId - 50);
				else
					SetPresetMode(presetModeId + 10);
			}
			break;
		case MENUDISPLAY_PRESETHEIGHT:
			if (left)
			{
				switch(presetModeId)
				{
					case 0:
					case 10:
					case 20:
					case 30:
					case 40:
					case 50:
						SetPresetMode(presetModeId + 7);
						break;
					default:
						SetPresetMode(presetModeId - 1);
				}
			}
			else if (right)
			{
				switch(presetModeId)
				{
					case 7:
					case 17:
					case 27:
					case 37:
					case 47:
					case 57:
						SetPresetMode(presetModeId - 7);
						break;
					default:
						SetPresetMode(presetModeId + 1);
				}
			}
			break;
		case MENUDISPLAY_DISPLINES:
			if (left)
			{
				if (mainMenu_displayedLines>100)
					mainMenu_displayedLines--;
			}
			else if (right)
			{
				if (mainMenu_displayedLines<270)
					mainMenu_displayedLines++;
			}
			break;
		case MENUDISPLAY_SCREENWIDTH:
			if (left)
			{
				screenWidth-=10;
				if (screenWidth<200)
					screenWidth=200;
			}
			else if (right)
			{
				screenWidth+=10;
				if (screenWidth>800)
					screenWidth=800;
			}
			break;
		case MENUDISPLAY_VERTPOS:
			if (left)
			{
				if (moveY>-42)
					moveY--;
			}
			else if (right)
			{
				if (moveY<50)
					moveY++;
			}
			break;
		case MENUDISPLAY_CUTLEFT:
			if (left)
			{
				if (mainMenu_cutLeft>0)
					mainMenu_cutLeft--;
			}
			else if (right)
			{
				if (mainMenu_cutLeft<100)
					mainMenu_cutLeft++;
			}
			break;
		case MENUDISPLAY_CUTRIGHT:
			if (left)
			{
				if (mainMenu_cutRight>0)
					mainMenu_cutRight--;
			}
			else if (right)
			{
				if (mainMenu_cutRight<100)
					mainMenu_cutRight++;
			}
			break;
		case MENUDISPLAY_FRAMESKIP:
#ifdef PANDORA
			if ((left)||(right))
					mainMenu_frameskip = !mainMenu_frameskip;
#else
			if (left)
			{
				if (mainMenu_frameskip>0)
					mainMenu_frameskip--;
				else
					mainMenu_frameskip=8;
			}
			else if (right)
			{
				if (mainMenu_frameskip<8)
					mainMenu_frameskip++;
				else
					mainMenu_frameskip=0;
			}
#endif
			break;
		case MENUDISPLAY_REFRESHRATE:
			if ((left)||(right))
					mainMenu_ntsc = !mainMenu_ntsc;
			break;
			
		case MENUDISPLAY_SOUND:
				if (left)
				{
					if (mainMenu_sound == 1)
						mainMenu_sound = 0;
					else if (mainMenu_sound == 2)
						mainMenu_sound = 1;
					else if (mainMenu_sound == 0)
						mainMenu_sound = 2;
				}
				else if (right)
				{
					if (mainMenu_sound == 2)
						mainMenu_sound = 0;
					else if (mainMenu_sound == 0)
						mainMenu_sound = 1;
					else if (mainMenu_sound == 1)
						mainMenu_sound = 2;
				}
				break;
			case MENUDISPLAY_SNDRATE:
				if ((left)||(right))
				{
					static int rates[] = { 8000, 11025, 22050, 32000, 44100 };
					int sel;
					for (sel = 0; sel < sizeof(rates) / sizeof(rates[0]); sel++)
						if (rates[sel] == sound_rate) break;
					sel += left ? -1 : 1;
					if (sel < 0) sel = 4;
					if (sel > 4) sel = 0;
					sound_rate = rates[sel];
				}
				break;
      
      case MENUDISPLAY_STEREO:
				if (left || right)
				{
				  if(mainMenu_soundStereo == 0)
				    mainMenu_soundStereo = 1;
				  else
				    mainMenu_soundStereo = 0;
        }
        break;
	}

	return end;
}

static void raise_displayMenu()
{
	int i;

	text_draw_background();
	text_flip();
	for(i=0;i<10;i++)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_display_title);
		text_flip();
	}
}

static void unraise_displayMenu()
{
	int i;

	for(i=9;i>=0;i--)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_display_title);
		text_flip();
	}
	text_draw_background();
	text_flip();
}

int run_menuDisplay()
{
	SDL_Event event;
	SDL_Delay(150);
	while(SDL_PollEvent(&event))
		SDL_Delay(10);
	int end=0, c=0;
	raise_displayMenu();
	while(!end)
	{
		draw_displayMenu(c);
		end=key_displayMenu(&c);
	}
	set_joyConf();
	unraise_displayMenu();
	return end;
}
