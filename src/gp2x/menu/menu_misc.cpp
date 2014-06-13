#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "menu.h"

#include <sys/stat.h>
#include <unistd.h>
#include<dirent.h>

#include "uae.h"
#include "options.h"
#include "sound.h"
#include <SDL.h>
#include "gp2x.h"
#include <SDL_ttf.h>
#include "custom.h"
#include "menu_config.h"

static const char *text_str_status_line="Status ln";

const char *text_str_misc_separator="----------------------------------";
static const char *text_str_misc_title=    "            Miscellanous         -";
static const char *text_str_stylus_offset="StylusOffset";
static const char *text_str_0px="0px";
static const char *text_str_1px="1px";
static const char *text_str_3px="3px";
static const char *text_str_5px="5px";
static const char *text_str_8px="8px";
static const char *text_str_tap_delay="Tap delay";
static const char *text_str_normal="normal";
static const char *text_str_short="short";
static const char *text_str_none="no";
static const char *text_str_mouse_multiplier="Mouse speed";
static const char *text_str_025x=".25";
static const char *text_str_05x=".5";
static const char *text_str_1x="1x";
static const char *text_str_2x="2x";
static const char *text_str_4x="4x";
int menuMisc = 0;

extern int kickstart;

enum { 
	MENUMISC_RETURNMAIN = 0,
	MENUMISC_CPU,
	MENUMISC_CHIPSET,
	MENUMISC_KICKSTART,
	MENUMISC_CPUSPEED,
#ifdef PANDORA
	MENUMISC_PANDORASPEED,
#endif
#ifdef ANDROIDSDL
	MENUMISC_ONSCREEN,
#endif
	MENUMISC_CONTROLCFG,
	MENUMISC_JOYSTICK,
	MENUMISC_AUTOFIRE,
	MENUMISC_STATUSLINE,
	MENUMISC_MOUSEMULTIPLIER,
	MENUMISC_STYLUSOFFSET,
	MENUMISC_TAPDELAY,
	MENUMISC_END
};

static void draw_miscMenu(int c)
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

	int menuLine = 2;
	static int b=0;
	int bb=(b%6)/3;
	SDL_Rect r;
	extern SDL_Surface *text_screen;
	char cpuSpeed[8];
	r.x=80-64; r.y=0; r.w=110+64+64; r.h=240;

	text_draw_background();
	text_draw_window(2,1,40,30,text_str_misc_title);

	// MENUMISC_RETURNMAIN
	if (menuMisc == MENUMISC_RETURNMAIN && bb)
		write_text_inv(3, menuLine, "Return to main menu");
	else
		write_text(3, menuLine, "Return to main menu");

	menuLine++;
	write_text(leftMargin,menuLine,text_str_misc_separator);
	menuLine++;

	// MENUMISC_CPU
	write_text(leftMargin,menuLine,"CPU");
	if ((mainMenu_CPU_model==0)&&((menuMisc!=MENUMISC_CPU)||(bb)))
	  write_text_inv(tabstop1,menuLine,"68000");
	else
	  write_text(tabstop1,menuLine,"68000");
	
	if ((mainMenu_CPU_model==1)&&((menuMisc!=MENUMISC_CPU)||(bb)))
	  write_text_inv(tabstop4,menuLine,"68020");
	else
	  write_text(tabstop4,menuLine,"68020");

	// MENUMISC_CHIPSET
	menuLine+=2;
	write_text(leftMargin,menuLine,"Chipset");
	
	if ((mainMenu_chipset==0)&&((menuMisc!=MENUMISC_CHIPSET)||(bb)))
		write_text_inv(tabstop1,menuLine,"OCS");
	else
		write_text(tabstop1,menuLine,"OCS");
	
	if ((mainMenu_chipset==1)&&((menuMisc!=MENUMISC_CHIPSET)||(bb)))
		write_text_inv(tabstop3,menuLine,"ECS");
	else
		write_text(tabstop3,menuLine,"ECS");
	
	if ((mainMenu_chipset==2)&&((menuMisc!=MENUMISC_CHIPSET)||(bb)))
		write_text_inv(tabstop5,menuLine,"AGA");
	else
		write_text(tabstop5,menuLine,"AGA");

	// MENUMISC_KICKSTART
	menuLine+=2;
	write_text(leftMargin,menuLine,"Kickstart");
	if ((kickstart==0)&&((menuMisc!=MENUMISC_KICKSTART)||(bb)))
		write_text_inv(tabstop1,menuLine,"1.2");
	else
		write_text(tabstop1,menuLine,"1.2");

	if ((kickstart==1)&&((menuMisc!=MENUMISC_KICKSTART)||(bb)))
		write_text_inv(tabstop3,menuLine,"1.3");
	else
		write_text(tabstop3,menuLine,"1.3");

	if ((kickstart==2)&&((menuMisc!=MENUMISC_KICKSTART)||(bb)))
		write_text_inv(tabstop5,menuLine,"2.0");
	else
		write_text(tabstop5,menuLine,"2.0");

	if ((kickstart==3)&&((menuMisc!=MENUMISC_KICKSTART)||(bb)))
		write_text_inv(tabstop7,menuLine,"3.1");
	else
		write_text(tabstop7,menuLine,"3.1");

	// MENUMISC_CPUSPEED
	menuLine+=2;
	write_text(leftMargin,menuLine,"CPU Speed");
	if ((mainMenu_CPU_speed==0)&&((menuMisc!=MENUMISC_CPUSPEED)||(bb)))
		write_text_inv(tabstop1,menuLine,"7MHz");
	else
		write_text(tabstop1,menuLine,"7MHz");

	if ((mainMenu_CPU_speed==1)&&((menuMisc!=MENUMISC_CPUSPEED)||(bb)))
		write_text_inv(tabstop4,menuLine,"14MHz");
	else
		write_text(tabstop4,menuLine,"14MHz");

	if ((mainMenu_CPU_speed==2)&&((menuMisc!=MENUMISC_CPUSPEED)||(bb)))
		write_text_inv(tabstop8,menuLine,"28MHz");
	else
		write_text(tabstop8,menuLine,"28MHz");

#ifdef PANDORA
  // MENUMISC_PANDORASPEED
	menuLine+=2;
	write_text(leftMargin,menuLine,"Pandora CPU-Speed");
	snprintf((char*)cpuSpeed, 8, "%d",mainMenu_cpuSpeed);
	if ((menuMisc!=MENUMISC_PANDORASPEED)||(bb))
		write_text_inv(tabstop4-1,menuLine,cpuSpeed);
	else
		write_text(tabstop4-1,menuLine,cpuSpeed);
	write_text(tabstop6-1,menuLine,"MHz");
#endif
#ifdef ANDROIDSDL
  // MENUMISC_ONSCREEN
	menuLine+=2;
	write_text(leftMargin,menuLine,"On-Screen Control");
	if ((mainMenu_onScreen==1)&&((menuMisc!=MENUMISC_ONSCREEN)||(bb)))
		write_text_inv(tabstop3,menuLine,"Show");
	else
		write_text(tabstop3,menuLine,"Show");
	if ((mainMenu_onScreen==0)&&((menuMisc!=MENUMISC_ONSCREEN)||(bb)))
		write_text_inv(tabstop6,menuLine,"Hide");
	else
		write_text(tabstop6,menuLine,"Hide");
#endif
  
	menuLine++;
	write_text(leftMargin,menuLine,text_str_misc_separator);
	menuLine++;

	// MENUMISC_CONTROLCFG
	write_text(leftMargin,menuLine,"Control config");

	if ((mainMenu_joyConf==0)&&((menuMisc!=MENUMISC_CONTROLCFG)||(bb)))
		write_text_inv(tabstop5,menuLine,"1");
	else
		write_text(tabstop5,menuLine,"1");

	if ((mainMenu_joyConf==1)&&((menuMisc!=MENUMISC_CONTROLCFG)||(bb)))
		write_text_inv(tabstop6,menuLine,"2");
	else
		write_text(tabstop6,menuLine,"2");

	if ((mainMenu_joyConf==2)&&((menuMisc!=MENUMISC_CONTROLCFG)||(bb)))
		write_text_inv(tabstop7,menuLine,"3");
	else
		write_text(tabstop7,menuLine,"3");

	if ((mainMenu_joyConf==3)&&((menuMisc!=MENUMISC_CONTROLCFG)||(bb)))
		write_text_inv(tabstop8,menuLine,"4");
	else
		write_text(tabstop8,menuLine,"4");

	menuLine+=2;
	if (mainMenu_joyConf==0) write_text(5,menuLine,"A=Autofire X=Fire Y=Space B=2nd");
	else if (mainMenu_joyConf==1) write_text(5,menuLine,"A=Fire X=Autofire Y=Space B=2nd");
	else if (mainMenu_joyConf==2) write_text(5,menuLine,"A=Autofire X=Jump Y=Fire B=2nd");
	else if (mainMenu_joyConf==3) write_text(5,menuLine,"A=Fire X=Jump Y=Autofire B=2nd");

  // MENUMISC_JOYSTICK
	menuLine+=2;
	write_text(leftMargin,menuLine,"Joystick");
  
	if ((mainMenu_joyPort==1)&&((menuMisc!=MENUMISC_JOYSTICK)||(bb)))
		write_text_inv(tabstop3-2,menuLine,"Port0");
	else
		write_text(tabstop3-2,menuLine,"Port0");

	if ((mainMenu_joyPort==2)&&((menuMisc!=MENUMISC_JOYSTICK)||(bb)))
		write_text_inv(tabstop6-2,menuLine,"Port1");
	else
		write_text(tabstop6-2,menuLine,"Port1");

	if ((mainMenu_joyPort==0)&&((menuMisc!=MENUMISC_JOYSTICK)||(bb)))
		write_text_inv(tabstop9-1,menuLine,"Both");
	else
		write_text(tabstop9-1,menuLine,"Both");
  
	// MENUMISC_AUTOFIRE
	menuLine+=2;
	write_text(leftMargin,menuLine,"Autofire Rate");

	if ((mainMenu_autofireRate==12)&&((menuMisc!=MENUMISC_AUTOFIRE)||(bb)))
		write_text_inv(tabstop3-2,menuLine,"Light");
	else
		write_text(tabstop3-2,menuLine,"Light");

	if ((mainMenu_autofireRate==8)&&((menuMisc!=MENUMISC_AUTOFIRE)||(bb)))
		write_text_inv(tabstop6-2,menuLine,"Medium");
	else
		write_text(tabstop6-2,menuLine,"Medium");

	if ((mainMenu_autofireRate==4)&&((menuMisc!=MENUMISC_AUTOFIRE)||(bb)))
		write_text_inv(tabstop9-1,menuLine,"Heavy");
	else
		write_text(tabstop9-1,menuLine,"Heavy");

	// MENUMISC_STATUSLINE
	menuLine+=2;
	write_text(leftMargin, menuLine,text_str_status_line);
	if ((!mainMenu_showStatus)&&((menuMisc!=MENUMISC_STATUSLINE)||(bb)))
		write_text_inv(tabstop1,menuLine, "Off");
	else
		write_text(tabstop1, menuLine, "Off");
	if ((mainMenu_showStatus)&&((menuMisc!=MENUMISC_STATUSLINE)||(bb)))
		write_text_inv(tabstop3, menuLine,"On");
	else
		write_text(tabstop3, menuLine,"On");

	menuLine++;
	write_text(leftMargin,menuLine,text_str_misc_separator);
	menuLine++;

	// MENUMISC_MOUSEMULTIPLIER
	write_text(leftMargin,menuLine,text_str_mouse_multiplier);

	if ((mainMenu_mouseMultiplier==25)&&((menuMisc!=MENUMISC_MOUSEMULTIPLIER)||(bb)))
		write_text_inv(tabstop1,menuLine,text_str_025x);
	else
		write_text(tabstop1,menuLine,text_str_025x);

	if ((mainMenu_mouseMultiplier==50)&&((menuMisc!=MENUMISC_MOUSEMULTIPLIER)||(bb)))
		write_text_inv(tabstop3,menuLine,text_str_05x);
	else
		write_text(tabstop3,menuLine,text_str_05x);

	if ((mainMenu_mouseMultiplier==1)&&((menuMisc!=MENUMISC_MOUSEMULTIPLIER)||(bb)))
		write_text_inv(tabstop5,menuLine,text_str_1x);
	else
		write_text(tabstop5,menuLine,text_str_1x);

	if ((mainMenu_mouseMultiplier==2)&&((menuMisc!=MENUMISC_MOUSEMULTIPLIER)||(bb)))
		write_text_inv(tabstop7,menuLine,text_str_2x);
	else
		write_text(tabstop7,menuLine,text_str_2x);

	if ((mainMenu_mouseMultiplier==4)&&((menuMisc!=MENUMISC_MOUSEMULTIPLIER)||(bb)))
		write_text_inv(tabstop9,menuLine,text_str_4x);
	else
		write_text(tabstop9,menuLine,text_str_4x);

	// MENUMISC_STYLUSOFFSET
	menuLine+=2;
	write_text(leftMargin,menuLine,text_str_stylus_offset);

	if ((mainMenu_stylusOffset==0)&&((menuMisc!=MENUMISC_STYLUSOFFSET)||(bb)))
		write_text_inv(tabstop1,menuLine,text_str_0px);
	else
		write_text(tabstop1,menuLine,text_str_0px);

	if ((mainMenu_stylusOffset==2)&&((menuMisc!=MENUMISC_STYLUSOFFSET)||(bb)))
		write_text_inv(tabstop3,menuLine,text_str_1px);
	else
		write_text(tabstop3,menuLine,text_str_1px);

	if ((mainMenu_stylusOffset==6)&&((menuMisc!=MENUMISC_STYLUSOFFSET)||(bb)))
		write_text_inv(tabstop5,menuLine,text_str_3px);
	else
		write_text(tabstop5,menuLine,text_str_3px);

	if ((mainMenu_stylusOffset==10)&&((menuMisc!=MENUMISC_STYLUSOFFSET)||(bb)))
		write_text_inv(tabstop7,menuLine,text_str_5px);
	else
		write_text(tabstop7,menuLine,text_str_5px);

	if ((mainMenu_stylusOffset==16)&&((menuMisc!=MENUMISC_STYLUSOFFSET)||(bb)))
		write_text_inv(tabstop9,menuLine,text_str_8px);
	else
		write_text(tabstop9,menuLine,text_str_8px);

	// MENUMISC_TAPDELAY
	menuLine+=2;
	write_text(leftMargin,menuLine,text_str_tap_delay);

	if ((mainMenu_tapDelay==10)&&((menuMisc!=MENUMISC_TAPDELAY)||(bb)))
		write_text_inv(tabstop1,menuLine,text_str_normal);
	else
		write_text(tabstop1,menuLine,text_str_normal);

	if ((mainMenu_tapDelay==5)&&((menuMisc!=MENUMISC_TAPDELAY)||(bb)))
		write_text_inv(tabstop5,menuLine,text_str_short);
	else
		write_text(tabstop5,menuLine,text_str_short);

	if ((mainMenu_tapDelay==2)&&((menuMisc!=MENUMISC_TAPDELAY)||(bb)))
		write_text_inv(tabstop9,menuLine,text_str_none);
	else
		write_text(tabstop9,menuLine,text_str_none);

	menuLine++;
	write_text(leftMargin,menuLine,text_str_misc_separator);
	menuLine++;

	text_flip();
	b++;
}

static int key_miscMenu(int *c)
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
			if (menuMisc==0) menuMisc=MENUMISC_END-1;
			else menuMisc--;
		}
		else if (down)
		{
			if (menuMisc==MENUMISC_END-1) menuMisc=0;
			else menuMisc++;
		}
		switch (menuMisc)
		{
			case MENUMISC_CPU:
				if (left)
				{
				    if (mainMenu_CPU_model > 0)
				       mainMenu_CPU_model--;
				    else
				       mainMenu_CPU_model=1;
				}
				else if (right)
				{
				    if (mainMenu_CPU_model < 1)
				       mainMenu_CPU_model++;
				    else
				       mainMenu_CPU_model=0;
				}
				UpdateCPUModelSettings();
				break;
			case MENUMISC_CHIPSET:
				if (left)
				{
					if (mainMenu_chipset > 0)
						mainMenu_chipset--;
					else
						mainMenu_chipset=2;
				}
				else if (right)
				{
					if (mainMenu_chipset < 2)
						mainMenu_chipset++;
					else
						mainMenu_chipset=0;
				}
				UpdateChipsetSettings();
				break;
			case MENUMISC_KICKSTART:
				if (left)
				{
					if (kickstart>0)
						kickstart--;
					else
						kickstart=3;
				}
				else if (right)
				{
					if (kickstart<3)
						kickstart++;
					else
						kickstart=0;
				}
				break;
			case MENUMISC_CPUSPEED:
				if (left)
				{
					if (mainMenu_CPU_speed>0)
						mainMenu_CPU_speed--;
					else
						mainMenu_CPU_speed=2;
				}
				else if (right)
				{
					if (mainMenu_CPU_speed<2)
						mainMenu_CPU_speed++;
					else
						mainMenu_CPU_speed=0;
				}
				break;
      
#ifdef PANDORA
      case MENUMISC_PANDORASPEED:
				if(left)
					mainMenu_cpuSpeed-=10;
				else if(right)
					mainMenu_cpuSpeed+=10;
        break;
#endif
#ifdef ANDROIDSDL
			case MENUMISC_ONSCREEN:
				if ((left)||(right))
						mainMenu_onScreen = !mainMenu_onScreen;
        break;
#endif
          
			case MENUMISC_CONTROLCFG:
				if (left)
				{
					if (mainMenu_joyConf>0)
						mainMenu_joyConf--;
					else
						mainMenu_joyConf=3;
				}
				else if (right)
				{
					if (mainMenu_joyConf<3)
						mainMenu_joyConf++;
					else
						mainMenu_joyConf=0;
				}
 				break;
			case MENUMISC_JOYSTICK:
				if (left)
				{
					if (mainMenu_joyPort>0)
						mainMenu_joyPort--;
					else
						mainMenu_joyPort=2;
				}
				else if (right)
				{
					if (mainMenu_joyPort<2)
						mainMenu_joyPort++;
					else
						mainMenu_joyPort=0;
				}
			  break;
			case MENUMISC_AUTOFIRE:
				if(left)
				{
					if(mainMenu_autofireRate==4)
						mainMenu_autofireRate=8;
					else if(mainMenu_autofireRate==8)
						mainMenu_autofireRate=12;
					else
						mainMenu_autofireRate=4;
				}
				else if (right)
				{
					if(mainMenu_autofireRate==4)
						mainMenu_autofireRate=12;
					else if(mainMenu_autofireRate==8)
						mainMenu_autofireRate=4;
					else
						mainMenu_autofireRate=8;
				}
 				break;
			case MENUMISC_STATUSLINE:
				if ((left)||(right))
					mainMenu_showStatus=!mainMenu_showStatus;
				break;
			case MENUMISC_MOUSEMULTIPLIER:
				if (left)
				{
					if (mainMenu_mouseMultiplier == 4)
						mainMenu_mouseMultiplier = 2;
					else if (mainMenu_mouseMultiplier == 2)
						mainMenu_mouseMultiplier = 1;
					else if (mainMenu_mouseMultiplier == 1)
						mainMenu_mouseMultiplier = 50;
					else if (mainMenu_mouseMultiplier == 50)
						mainMenu_mouseMultiplier = 25;
					else
						mainMenu_mouseMultiplier = 4;
				}
				else if (right)
				{
					if (mainMenu_mouseMultiplier == 4)
						mainMenu_mouseMultiplier = 25;
					else if (mainMenu_mouseMultiplier == 2)
						mainMenu_mouseMultiplier = 4;
					else if (mainMenu_mouseMultiplier == 1)
						mainMenu_mouseMultiplier = 2;
					else if (mainMenu_mouseMultiplier == 50)
						mainMenu_mouseMultiplier = 1;
					else
						mainMenu_mouseMultiplier = 50;
				}
				break;
			case MENUMISC_STYLUSOFFSET:
				if (left)
				{
					if (mainMenu_stylusOffset == 0)
						mainMenu_stylusOffset = 16;
					else if (mainMenu_stylusOffset == 2)
						mainMenu_stylusOffset = 0;
					else if (mainMenu_stylusOffset == 6)
						mainMenu_stylusOffset = 2;
					else if (mainMenu_stylusOffset == 10)
						mainMenu_stylusOffset = 6;
					else
						mainMenu_stylusOffset = 10;
				}
				else if (right)
				{
					if (mainMenu_stylusOffset == 0)
						mainMenu_stylusOffset = 2;
					else if (mainMenu_stylusOffset == 2)
						mainMenu_stylusOffset = 6;
					else if (mainMenu_stylusOffset == 6)
						mainMenu_stylusOffset = 10;
					else if (mainMenu_stylusOffset == 10)
						mainMenu_stylusOffset = 16;
					else
						mainMenu_stylusOffset = 0;
				}
				break;
			case MENUMISC_TAPDELAY:
				if (left)
				{
					if (mainMenu_tapDelay == 10)
						mainMenu_tapDelay = 2;
					else if (mainMenu_tapDelay == 5)
						mainMenu_tapDelay = 10;
					else
						mainMenu_tapDelay = 5;
				}
				else if (right)
				{
					if (mainMenu_tapDelay == 10)
						mainMenu_tapDelay = 5;
					else if (mainMenu_tapDelay == 5)
						mainMenu_tapDelay = 2;
					else
						mainMenu_tapDelay = 10;
				}
				break;
		}
	}

	return end;
}

static void raise_miscMenu()
{
	int i;

	text_draw_background();
	text_flip();
	for(i=0;i<10;i++)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_misc_title);
		text_flip();
	}
}

static void unraise_miscMenu()
{
	int i;

	for(i=9;i>=0;i--)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_misc_title);
		text_flip();
	}
	text_draw_background();
	text_flip();
}

int run_menuMisc()
{
	SDL_Event event;
	SDL_Delay(150);
	while(SDL_PollEvent(&event))
		SDL_Delay(10);
	int end=0, c=0;
	raise_miscMenu();
	while(!end)
	{
		draw_miscMenu(c);
		end=key_miscMenu(&c);
	}
	set_joyConf();
	unraise_miscMenu();
	return end;
}
