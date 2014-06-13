#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"

#include "uae.h"
#include "options.h"
#include "menu.h"
#include "menu_config.h"
#include "sound.h"
#include "disk.h"
#include "memory-uae.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"

#if defined(ANDROID)
#include <SDL_screenkeyboard.h>
#include <android/log.h>
#endif

extern int screenWidth;
extern int mainMenu_case;

extern int lastCpuSpeed;
extern int ntsc;

extern char launchDir[300];
extern char currentDir[300];

int saveAdfDir()
{
	char path[300];
	snprintf(path, 300, "%s/conf/adfdir.conf", launchDir);
	FILE *f=fopen(path,"w");
	if (!f) return 0;
	char buffer[310];
	snprintf((char*)buffer, 310, "path=%s\n",currentDir);
	fputs(buffer,f);
	fclose(f);
	return 1;
}


void extractFileName(char * str,char *buffer)
{
	char *p=str+strlen(str)-1;
	while(*p != '/')
		p--;
	p++;
	strcpy(buffer,p);
}

#ifdef ANDROIDSDL
void update_onscreen()
{
	if (mainMenu_case != MAIN_MENU_CASE_DISPLAY && mainMenu_case != MAIN_MENU_CASE_MEMDISK && mainMenu_onScreen==0)
	{
	  SDL_ANDROID_SetScreenKeyboardShown(0);
	}
	else
	{
	  SDL_ANDROID_SetScreenKeyboardShown(1);
	    SDL_Rect pos_textinput, pos_dpad, pos_button1, pos_button2, pos_button3, pos_button4, pos_button5, pos_button6;
	    pos_textinput.x = mainMenu_pos_x_textinput*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
	    pos_textinput.y = mainMenu_pos_y_textinput*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
	    pos_textinput.h=SDL_ListModes(NULL, 0)[0]->h / (float)10;
	    pos_textinput.w=pos_textinput.h;
	    SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_TEXT, &pos_textinput);
	    pos_dpad.x = mainMenu_pos_x_dpad*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
	    pos_dpad.y = mainMenu_pos_y_dpad*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
	    pos_dpad.h=SDL_ListModes(NULL, 0)[0]->h / (float)2.5;
	    pos_dpad.w=pos_dpad.h;
	    SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD, &pos_dpad);
	    pos_button1.x = mainMenu_pos_x_button1*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
	    pos_button1.y = mainMenu_pos_y_button1*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
	    pos_button1.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
	    pos_button1.w=pos_button1.h;
	    SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, &pos_button1);
	    pos_button2.x = mainMenu_pos_x_button2*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
	    pos_button2.y = mainMenu_pos_y_button2*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
	    pos_button2.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
	    pos_button2.w=pos_button2.h;
	    SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, &pos_button2);
	    pos_button3.x = mainMenu_pos_x_button3*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
	    pos_button3.y = mainMenu_pos_y_button3*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
	    pos_button3.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
	    pos_button3.w=pos_button3.h;
	    SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, &pos_button3);
	    pos_button4.x = mainMenu_pos_x_button4*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
	    pos_button4.y = mainMenu_pos_y_button4*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
	    pos_button4.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
	    pos_button4.w=pos_button4.h;
	    SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, &pos_button4);
	    pos_button5.x = mainMenu_pos_x_button5*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
	    pos_button5.y = mainMenu_pos_y_button5*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
	    pos_button5.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
	    pos_button5.w=pos_button5.h;
	    SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_4, &pos_button5);
	    pos_button6.x = mainMenu_pos_x_button6*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
	    pos_button6.y = mainMenu_pos_y_button6*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
	    pos_button6.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
	    pos_button6.w=pos_button6.h;
	    SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_5, &pos_button6);

	    SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_TEXT, mainMenu_onScreen_textinput);
	    SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD, mainMenu_onScreen_dpad);
	    SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, mainMenu_onScreen_button1);
	    SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, mainMenu_onScreen_button2);
	    SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, mainMenu_onScreen_button3);
	    SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, mainMenu_onScreen_button4);
	    SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_4, mainMenu_onScreen_button5);
	    SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_5, mainMenu_onScreen_button6);
	}  
}
#endif

void update_display()
{
	char layersize[20];
	snprintf(layersize, 20, "%dx480", screenWidth);

#ifndef WIN32
	setenv("SDL_OMAP_LAYER_SIZE",layersize,1);
#endif

	char bordercut[20];
	snprintf(bordercut, 20, "%d,%d,0,0", mainMenu_cutLeft, mainMenu_cutRight);

#ifndef WIN32
	setenv("SDL_OMAP_BORDER_CUT",bordercut,1);
#endif

#ifdef ANDROIDSDL
	update_onscreen();
#endif

#if defined(PANDORA) && !defined(WIN32)
	prSDLScreen = SDL_SetVideoMode(visibleAreaWidth, mainMenu_displayedLines, 16, SDL_SWSURFACE|SDL_FULLSCREEN|SDL_DOUBLEBUF);
#elif defined(PANDORA) && defined(WIN32)
	prSDLScreen = SDL_SetVideoMode(visibleAreaWidth, mainMenu_displayedLines, 16, SDL_SWSURFACE|SDL_DOUBLEBUF);
#else
	prSDLScreen = SDL_SetVideoMode(visibleAreaWidth, mainMenu_displayedLines, 16, SDL_SWSURFACE|SDL_FULLSCREEN);
#endif
  SDL_ShowCursor(SDL_DISABLE);

	if(mainMenu_displayHires)
		InitDisplayArea(visibleAreaWidth >> 1);
	else
		InitDisplayArea(visibleAreaWidth);
}


static bool cpuSpeedChanged = false;

void setCpuSpeed()
{
	char speedCmd[128];

	if(mainMenu_cpuSpeed != lastCpuSpeed)
	{
		snprintf((char*)speedCmd, 128, "unset DISPLAY; echo y | sudo -n /usr/pandora/scripts/op_cpuspeed.sh %d", mainMenu_cpuSpeed);
		system(speedCmd);
		lastCpuSpeed = mainMenu_cpuSpeed;
		cpuSpeedChanged = true;
	}
	if(mainMenu_ntsc != ntsc)
	{
		ntsc = mainMenu_ntsc;
		if(ntsc)
			system("sudo /usr/pandora/scripts/op_lcdrate.sh 60");
		else
			system("sudo /usr/pandora/scripts/op_lcdrate.sh 50");
	}
	update_display();
}


#ifdef PANDORA

void resetCpuSpeed(void)
{
  if(cpuSpeedChanged)
  {
    FILE* f = fopen ("/etc/pandora/conf/cpu.conf", "rt");
    if(f)
    {
      char line[128];
      for(int i=0; i<6; ++i)
      {
        fscanf(f, "%s\n", &line);
        if(strncmp(line, "default:", 8) == 0)
        {
          int value = 0;
          sscanf(line, "default:%d", &value);
          if(value > 500 && value < 1200)
          {
            lastCpuSpeed = value - 10;
            mainMenu_cpuSpeed = value;
            setCpuSpeed();
            printf("CPU speed reset to %d\n", value);
          }
        }
      }
      fclose(f);
    }
  }
}

#endif
