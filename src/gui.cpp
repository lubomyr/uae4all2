 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Interface to the Tcl/Tk GUI
  *
  * Copyright 1996 Bernd Schmidt
  */

#define _GUI_CPP

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "gui.h"
#include "menu.h"
#include "menu_config.h"
#ifdef USE_UAE4ALL_VKBD
#include "vkbd.h"
#endif
#include "debug_uae4all.h"
#include "custom.h"
#include "memory-uae.h"
#include "xwin.h"
#include "drawing.h"
#include "sound.h"
#include "audio.h"
#include "keybuf.h"
#include "keyboard.h"
#include "disk.h"
#include "savestate.h"
#include <SDL.h>

#define VIDEO_FLAGS_INIT SDL_SWSURFACE|SDL_FULLSCREEN
#ifdef ANDROIDSDL
#define VIDEO_FLAGS VIDEO_FLAGS_INIT
#else
#define VIDEO_FLAGS VIDEO_FLAGS_INIT | SDL_DOUBLEBUF
#endif

#include "gp2x.h"
#include "gp2xutil.h"

#ifdef ANDROIDSDL
#include <android/log.h>
#endif

extern int gp2xMouseEmuOn, gp2xButtonRemappingOn;
extern bool switch_autofire;
int justMovedUp=0, justMovedDown=0, justMovedLeft=0, justMovedRight=0;
int justLComma=0, justLPeriod=0;
#ifdef USE_UAE4ALL_VKBD
int justLK=0;
#endif
int justPressedA=0, justPressedB=0, justPressedX=0, justPressedY=0;
int justPressedL=0, justPressedR=0, justPressedQ=0;
int stylusClickOverride=0;
int stylusAdjustX=0, stylusAdjustY=0;
int screenWidth = 640;


static char _show_message_str[40]={
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

int show_message=0;
char *show_message_str=(char *)&_show_message_str[0];

extern SDL_Surface *prSDLScreen;

extern SDL_Joystick *uae4all_joy0, *uae4all_joy1;

#ifdef USE_UAE4ALL_VKBD
extern int keycode2amiga(SDL_keysym *prKeySym);
#endif
extern int uae4all_keystate[];

int emulating=0;
char uae4all_image_file0[256]  = { 0, };
char uae4all_image_file1[256] = { 0, };
char uae4all_image_file2[256]  = { 0, };
char uae4all_image_file3[256] = { 0, };

char uae4all_hard_dir[256] = { 0, };
char uae4all_hard_file[256] = { 0, };

int drawfinished=0;

int moved_x = 0;
int moved_y = 0;

int dpadUp=0;
int dpadDown=0;
int dpadLeft=0;
int dpadRight=0;
int buttonA=0;
int buttonB=0;
int buttonX=0;
int buttonY=0;
int triggerL=0;
int triggerR=0;
int buttonSelect=0;
int buttonStart=0;

extern int mainMenu_case;
#ifdef WITH_TESTMODE
int no_limiter = 0;
#endif

static void getChanges(void)
{
    if (mainMenu_sound)
    {
		if (mainMenu_sound == 1)
			changed_produce_sound=2;
		else
			changed_produce_sound=3;
	    sound_default_evtime();
    }
    else
	    changed_produce_sound=0;
    changed_gfx_framerate=mainMenu_frameskip;
}

int gui_init (void)
{
    SDL_ShowCursor(SDL_DISABLE);
#if !(defined(ANDROIDSDL) || defined(AROS))
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_JoystickOpen(0);
#endif
    if (prSDLScreen!=NULL)
    {
		emulating=0;
		uae4all_init_sound();
		init_kickstart();
#ifdef USE_GUICHAN
		if (!uae4all_image_file0[0])
			run_mainMenuGuichan();
#else
		init_text(1);
		if (!uae4all_image_file0[0])
			run_mainMenu();
		quit_text();
#endif
#ifdef USE_UAE4ALL_VKBD
		vkbd_init();
#endif
		inputmode_init();
		uae4all_pause_music();
		emulating=1;
		getChanges();
		check_all_prefs();
		return 0;
    }
    return -1;
}

int gui_update (void)
{
    extern char *savestate_filename;
#ifdef USE_GUICHAN
    extern char *screenshot_filename;
#endif
    strcpy(changed_df[0],uae4all_image_file0);
    strcpy(changed_df[1],uae4all_image_file1);
    strcpy(changed_df[2],uae4all_image_file2);
    strcpy(changed_df[3],uae4all_image_file3);
    strcpy(savestate_filename,uae4all_image_file0);
#ifdef USE_GUICHAN
    strcpy(screenshot_filename,uae4all_image_file0);
#endif
    switch(saveMenu_n_savestate)
    {
	    case 1:
    		strcat(savestate_filename,"-1.asf");
#ifdef USE_GUICHAN
		strcat(screenshot_filename,"-1.png");
#endif
		break;
	    case 2:
    		strcat(savestate_filename,"-2.asf");
#ifdef USE_GUICHAN
		strcat(screenshot_filename,"-2.png");
#endif
		break;
	    case 3:
    		strcat(savestate_filename,"-3.asf");
#ifdef USE_GUICHAN
		strcat(screenshot_filename,"-3.png");
#endif
		break;
	    default: 
    	   	strcat(savestate_filename,".asf");
#ifdef USE_GUICHAN
		strcat(screenshot_filename,".png");
#endif
    }
    real_changed_df[0]=1;
    real_changed_df[1]=1;
    real_changed_df[2]=1;
    real_changed_df[3]=1;
    return 0;
}

static void goMenu(void)
{
	int exitmode=0;
	int autosave=mainMenu_autosave;
	if (quit_program != 0)
		return;
	emulating=1;
#ifdef USE_UAE4ALL_VKBD
	vkbd_quit();
#endif	
	pause_sound();
#ifdef USE_GUICHAN
	running=true;
	exitmode=run_mainMenuGuichan();
#else
	init_text(0);
	menu_raise();
	exitmode=run_mainMenu();
#endif

	/* Clear menu garbage at the bottom of the screen */
	black_screen_now();
	notice_screen_contents_lost();
	resume_sound();
#ifndef USE_GUICHAN
	if ((!(strcmp(prefs_df[0],uae4all_image_file0))) || ((!(strcmp(prefs_df[1],uae4all_image_file1)))))
		menu_unraise();
	quit_text();
#endif
#ifdef USE_UAE4ALL_VKBD
	vkbd_init();
#endif
    getChanges();
#ifdef USE_UAE4ALL_VKBD
	vkbd_init_button2();
#endif
    if (exitmode==1 || exitmode==2)
    {
		extern char *savestate_filename;
#ifdef USE_GUICHAN
		extern char *screenshot_filename;
#endif
		extern int saveMenu_n_savestate;
		for(int i=0;i<mainMenu_drives;i++)
	    {
			if (i==0 && strcmp(changed_df[0],uae4all_image_file0)) {
				strcpy(changed_df[0],uae4all_image_file0);
				real_changed_df[0]=1;
			}
			else if (i==1 && strcmp(changed_df[1],uae4all_image_file1)) {
				strcpy(changed_df[1],uae4all_image_file1);
				real_changed_df[1]=1;
			}
			else if (i==2 && strcmp(changed_df[2],uae4all_image_file2)) {
				strcpy(changed_df[2],uae4all_image_file2);
				real_changed_df[2]=1;
			}
			else if (i==3 && strcmp(changed_df[3],uae4all_image_file3)) {
				strcpy(changed_df[3],uae4all_image_file3);
				real_changed_df[3]=1;
			}
	    }
		strcpy(savestate_filename,uae4all_image_file0);
#ifdef USE_GUICHAN
		strcpy(screenshot_filename,uae4all_image_file0);
#endif
	    switch(saveMenu_n_savestate)
		{
			case 1:
				strcat(savestate_filename,"-1.asf");
#ifdef USE_GUICHAN
				strcat(screenshot_filename,"-1.png");
#endif
				break;
			case 2:
				strcat(savestate_filename,"-2.asf");
#ifdef USE_GUICHAN
				strcat(screenshot_filename,"-2.png");
#endif
				break;
			case 3:
				strcat(savestate_filename,"-3.asf");
#ifdef USE_GUICHAN
				strcat(screenshot_filename,"-3.png");
#endif
				break;
			default: 
				strcat(savestate_filename,".asf");
#ifdef USE_GUICHAN
				strcat(screenshot_filename,".png");
#endif
		}
    }
    if (exitmode==3)
    {
		extern char *savestate_filename;
#ifdef USE_GUICHAN
		extern char *screenshot_filename;
#endif
		extern int saveMenu_n_savestate;
		for(int i=0;i<mainMenu_drives;i++)
		{
			changed_df[i][0]=0;
			if (i==0) {
				uae4all_image_file0[0]=0;
				if (strcmp(changed_df[0],uae4all_image_file0))
				{ 
				strcpy(changed_df[0],uae4all_image_file0);
				real_changed_df[0]=1;
				}
			}
			else if (i==1) {
				uae4all_image_file1[0]=0;
				if (strcmp(changed_df[1],uae4all_image_file1))
				{ 
				strcpy(changed_df[1],uae4all_image_file1);
				real_changed_df[1]=1;
				}
			}
			else if (i==2) {
				uae4all_image_file2[0]=0;
				if (strcmp(changed_df[2],uae4all_image_file2))
				{ 
				strcpy(changed_df[2],uae4all_image_file2);
				real_changed_df[2]=1;
				}
			}
			else if (i==3) {
				uae4all_image_file3[0]=0;
				if (strcmp(changed_df[3],uae4all_image_file3))
				{ 
				strcpy(changed_df[3],uae4all_image_file3);
				real_changed_df[3]=1;
				}
			}
			disk_eject(i);
		}
		strcpy(savestate_filename,uae4all_image_file0);
#ifdef USE_GUICHAN
		strcpy(screenshot_filename,uae4all_image_file0);
#endif
		switch(saveMenu_n_savestate)
		{
		case 1:
    			strcat(savestate_filename,"-1.asf");
#ifdef USE_GUICHAN
			strcat(screenshot_filename,"-1.png");
#endif
			break;
	    	case 2:
    			strcat(savestate_filename,"-2.asf");
#ifdef USE_GUICHAN
			strcat(screenshot_filename,"-2.png");
#endif
			break;
	    	case 3:
    			strcat(savestate_filename,"-3.asf");
#ifdef USE_GUICHAN
			strcat(screenshot_filename,"-3.png");
#endif
			break;
	    	default: 
    	  	 	strcat(savestate_filename,".asf");
#ifdef USE_GUICHAN
			strcat(screenshot_filename,".png");
#endif
		}
    }
    if (exitmode==2)
    {
	    if (autosave!=mainMenu_autosave)
	    {
	    	prefs_df[0][0]=0;
	   		prefs_df[1][0]=0;
			prefs_df[2][0]=0;
	   		prefs_df[3][0]=0;
	    }
		if(gp2xButtonRemappingOn)
			togglemouse();
	    uae_reset ();
    }
    check_all_prefs();
    gui_purge_events();
    fpscounter_reset();
    notice_screen_contents_lost();
}

int customKey;
void getMapping(int customId)
{
	switch(customId)
	{
		case 1: customKey=AK_UP; break;
		case 2: customKey=AK_DN; break;
		case 3: customKey=AK_LF; break;
		case 4: customKey=AK_RT; break;
		case 5: customKey=AK_NP0; break;
		case 6: customKey=AK_NP1; break;
		case 7: customKey=AK_NP2; break;
		case 8: customKey=AK_NP3; break;
		case 9: customKey=AK_NP4; break;
		case 10: customKey=AK_NP5; break;
		case 11: customKey=AK_NP6; break;
		case 12: customKey=AK_NP7; break;
		case 13: customKey=AK_NP8; break;
		case 14: customKey=AK_NP9; break;
		case 15: customKey=AK_ENT; break;
		case 16: customKey=AK_NPDIV; break;
		case 17: customKey=AK_NPMUL; break;
		case 18: customKey=AK_NPSUB; break;
		case 19: customKey=AK_NPADD; break;
		case 20: customKey=AK_NPDEL; break;
		case 21: customKey=AK_NPLPAREN; break;
		case 22: customKey=AK_NPRPAREN; break;
		case 23: customKey=AK_SPC; break;
		case 24: customKey=AK_BS; break;
		case 25: customKey=AK_TAB; break;
		case 26: customKey=AK_RET; break;
		case 27: customKey=AK_ESC; break;
		case 28: customKey=AK_DEL; break;
		case 29: customKey=AK_LSH; break;
		case 30: customKey=AK_RSH; break;
		case 31: customKey=AK_CAPSLOCK; break;
		case 32: customKey=AK_CTRL; break;
		case 33: customKey=AK_LALT; break;
		case 34: customKey=AK_RALT; break;
		case 35: customKey=AK_LAMI; break;
		case 36: customKey=AK_RAMI; break;
		case 37: customKey=AK_HELP; break;
		case 38: customKey=AK_LBRACKET; break;
		case 39: customKey=AK_RBRACKET; break;
		case 40: customKey=AK_SEMICOLON; break;
		case 41: customKey=AK_COMMA; break;
		case 42: customKey=AK_PERIOD; break;
		case 43: customKey=AK_SLASH; break;
		case 44: customKey=AK_BACKSLASH; break;
		case 45: customKey=AK_QUOTE; break;
		case 46: customKey=AK_NUMBERSIGN; break;
		case 47: customKey=AK_LTGT; break;
		case 48: customKey=AK_BACKQUOTE; break;
		case 49: customKey=AK_MINUS; break;
		case 50: customKey=AK_EQUAL; break;
		case 51: customKey=AK_A; break;
		case 52: customKey=AK_B; break;
		case 53: customKey=AK_C; break;
		case 54: customKey=AK_D; break;
		case 55: customKey=AK_E; break;
		case 56: customKey=AK_F; break;
		case 57: customKey=AK_G; break;
		case 58: customKey=AK_H; break;
		case 59: customKey=AK_I; break;
		case 60: customKey=AK_J; break;
		case 61: customKey=AK_K; break;
		case 62: customKey=AK_L; break;
		case 63: customKey=AK_M; break;
		case 64: customKey=AK_N; break;
		case 65: customKey=AK_O; break;
		case 66: customKey=AK_P; break;
		case 67: customKey=AK_Q; break;
		case 68: customKey=AK_R; break;
		case 69: customKey=AK_S; break;
		case 70: customKey=AK_T; break;
		case 71: customKey=AK_U; break;
		case 72: customKey=AK_V; break;
		case 73: customKey=AK_W; break;
		case 74: customKey=AK_X; break;
		case 75: customKey=AK_Y; break;
		case 76: customKey=AK_Z; break;
		case 77: customKey=AK_1; break;
		case 78: customKey=AK_2; break;
		case 79: customKey=AK_3; break;
		case 80: customKey=AK_4; break;
		case 81: customKey=AK_5; break;
		case 82: customKey=AK_6; break;
		case 83: customKey=AK_7; break;
		case 84: customKey=AK_8; break;
		case 85: customKey=AK_9; break;
		case 86: customKey=AK_0; break;
		case 87: customKey=AK_F1; break;
		case 88: customKey=AK_F2; break;
		case 89: customKey=AK_F3; break;
		case 90: customKey=AK_F4; break;
		case 91: customKey=AK_F5; break;
		case 92: customKey=AK_F6; break;
		case 93: customKey=AK_F7; break;
		case 94: customKey=AK_F8; break;
		case 95: customKey=AK_F9; break;
		case 96: customKey=AK_F10; break;
		default: customKey=0;
	}
}

void gui_handle_events (void)
{
	Uint8 *keystate = SDL_GetKeyState(NULL);
	dpadUp = keystate[SDLK_UP];
	dpadDown = keystate[SDLK_DOWN];
	dpadLeft = keystate[SDLK_LEFT];
	dpadRight = keystate[SDLK_RIGHT];
	buttonA = keystate[SDLK_HOME];
	buttonB = keystate[SDLK_END];
	buttonX = keystate[SDLK_PAGEDOWN];
	buttonY = keystate[SDLK_PAGEUP];
#ifndef ANDROIDSDL
	triggerL = keystate[SDLK_RSHIFT];
	triggerR = keystate[SDLK_RCTRL];
	buttonSelect = keystate[SDLK_LCTRL];
	buttonStart = keystate[SDLK_LALT];

	if(keystate[SDLK_LCTRL])
		goMenu();

	if(keystate[SDLK_F12])
		SDL_WM_ToggleFullScreen(prSDLScreen);
#else
	triggerL = keystate[SDLK_F13];
	triggerR = keystate[SDLK_RCTRL];
	buttonSelect = keystate[SDLK_F12];
	buttonStart = keystate[SDLK_F11];

	if(keystate[SDLK_F12])
		goMenu();
#endif

#ifdef ANDROIDSDL

#ifdef USE_UAE4ALL_VKBD
	//textUI virtual keyboard via F15 
	if(keystate[SDLK_F15])
	{
		if(!justLK)
		{
			sleep(1);
			vkbd_mode = !vkbd_mode;
			justLK=1;
		}
	}
	else if(justLK)
		justLK=0;
	//Quick Switch - textUI virtual keyboard via buttonB+buttonY
	if((mainMenu_quickSwitch!=0) && buttonB && buttonY)
	{
		if(!justLK)
		{
			sleep(1);
			vkbd_mode = !vkbd_mode;
			justLK=1;
		}
	}
	else if(justLK)
		justLK=0;
#endif
	// Quick Switch - screen lowres/hires
	if (((mainMenu_quickSwitch==1) && buttonB && dpadUp) || ((mainMenu_quickSwitch==2) && buttonY && dpadUp))
	{
	  if (visibleAreaWidth==320) {
	    visibleAreaWidth=640;
	    mainMenu_displayHires=1;
	  }
	  else if (visibleAreaWidth==640) {
	    visibleAreaWidth=320;
	    mainMenu_displayHires=0;
	    }
		getChanges();
		check_all_prefs();
		update_display();
	}
	// Quick Switch - screen height
	if (((mainMenu_quickSwitch==1) && buttonB && dpadDown) || ((mainMenu_quickSwitch==2) && buttonY && dpadDown))
	{
	  if (mainMenu_displayedLines==200)
	    mainMenu_displayedLines=216;
	  else if (mainMenu_displayedLines==216)
	    mainMenu_displayedLines=240;
	  else if (mainMenu_displayedLines==240)
	    mainMenu_displayedLines=256;
	  else if (mainMenu_displayedLines==256)
	    mainMenu_displayedLines=262;
	  else if (mainMenu_displayedLines==262)
	    mainMenu_displayedLines=270;
	  else if (mainMenu_displayedLines==270)
	    mainMenu_displayedLines=200;
		getChanges();
		check_all_prefs();
		update_display();
	}
	// Quick Switch - save state
	if (((mainMenu_quickSwitch==1) && buttonB && dpadLeft) || ((mainMenu_quickSwitch==2) && buttonY && dpadLeft))
	{	
	  keystate[SDLK_s]=0;
	  savestate_state = STATE_DOSAVE;
	}
	// Quick Switch - restore state
	if (((mainMenu_quickSwitch==1) && buttonB && dpadRight) || ((mainMenu_quickSwitch==2) && buttonY && dpadRight))
	{
		extern char *savestate_filename;
		FILE *f=fopen(savestate_filename, "rb");
		keystate[SDLK_l]=0;
		if(f)
		{
			fclose(f);
			savestate_state = STATE_DORESTORE;
		}
		else
			gui_set_message("Failed: Savestate not found", 100);
	}
#endif 		
	
#ifdef USE_UAE4ALL_VKBD
if(!vkbd_mode)
#endif
{
	//L + R
	if(triggerL && triggerR)
	{
		//up
		if(dpadUp)
		{
			moveVertical(1);
			moved_y += 2;
		}
		//down
		else if(dpadDown)
		{
			moveVertical(-1);
			moved_y -= 2;
		}
		//left
		else if(dpadLeft)
		{
			screenWidth -=10;
			if(screenWidth<200)
				screenWidth = 200;
			update_display();
		}
		//right
		else if(dpadRight)
		{
			screenWidth +=10;
			if(screenWidth>800)
				screenWidth = 800;
			update_display();
		}
		//1
		else if(keystate[SDLK_1])
		{
			SetPresetMode((presetModeId / 10) * 10 + 0);
			update_display();
		}
		//2
		else if(keystate[SDLK_2])
		{
			SetPresetMode((presetModeId / 10) * 10 + 1);
			update_display();
		}
		//3
		else if(keystate[SDLK_3])
		{
			SetPresetMode((presetModeId / 10) * 10 + 2);
			update_display();
		}
		//4
		else if(keystate[SDLK_4])
		{
			SetPresetMode((presetModeId / 10) * 10 + 3);
			update_display();
		}
		//5
		else if(keystate[SDLK_5])
		{
			SetPresetMode((presetModeId / 10) * 10 + 4);
			update_display();
		}
		//6
		else if(keystate[SDLK_6])
		{
			SetPresetMode((presetModeId / 10) * 10 + 5);
			update_display();
		}
		//7
		else if(keystate[SDLK_7])
		{
			SetPresetMode((presetModeId / 10) * 10 + 6);
			update_display();
		}
		//8
		else if(keystate[SDLK_8])
		{
			SetPresetMode((presetModeId / 10) * 10 + 7);
			update_display();
		}
		//9
		else if(keystate[SDLK_9])
		{
			if(mainMenu_displayedLines > 100)
				mainMenu_displayedLines--;
			update_display();
		}
		//0
		else if(keystate[SDLK_0])
		{
			if(mainMenu_displayedLines < 270)
				mainMenu_displayedLines++;
			update_display();
		}
		else if(keystate[SDLK_w])
		{
			// Change width
			if(presetModeId < 50)
				SetPresetMode(presetModeId + 10);
			else
				SetPresetMode(presetModeId - 50);
			update_display();
		}
#ifdef WITH_TESTMODE
		else if(keystate[SDLK_t])
		{
			if(no_limiter)
				no_limiter = 0;
			else
				no_limiter = 1;
			char value[8];
			snprintf(value, 8, "%d", no_limiter ? 0 : 1);
#ifndef WIN32
			setenv("SDL_OMAP_VSYNC",value,1);
#endif
			update_display();
		}		
#endif
	}

	else if(triggerL)
	{
		//cutRight
		if(keystate[SDLK_COMMA] && mainMenu_cutLeft > 0)
		{
			mainMenu_cutLeft--;
			update_display();
		}
		else if(keystate[SDLK_PERIOD] && mainMenu_cutLeft < 100)
		{
			mainMenu_cutLeft++;
			update_display();
		}
		else if(keystate[SDLK_a])
		{
			keystate[SDLK_a]=0;
			mainMenu_CPU_speed == 2 ? mainMenu_CPU_speed = 0 : mainMenu_CPU_speed++;
		}
		else if(keystate[SDLK_c])
		{
			keystate[SDLK_c]=0;
			mainMenu_customControls = !mainMenu_customControls;
		}
		else if(keystate[SDLK_d])
		{
			keystate[SDLK_d]=0;
			mainMenu_showStatus = !mainMenu_showStatus;
		}
		else if(keystate[SDLK_f])
		{
			keystate[SDLK_f]=0;
			mainMenu_frameskip ? mainMenu_frameskip = 0 : mainMenu_frameskip = 1;
			getChanges();
			check_all_prefs();
		}

		//Q key
		if(keystate[SDLK_q])
		{
			if(!justPressedQ)
			{
				uae4all_keystate[AK_NPMUL] = 1;
				record_key(AK_NPMUL << 1);
				uae4all_keystate[AK_F10] = 1;
				record_key(AK_F10 << 1);				
				justPressedQ=1;
			}
		}
		else if(justPressedQ)
		{
			uae4all_keystate[AK_NPMUL] = 0;
			record_key((AK_NPMUL << 1) | 1);
			uae4all_keystate[AK_F10] = 0;
			record_key((AK_F10 << 1) | 1);
			justPressedQ=0;
		}
	}

	//autofire on/off
	else if(triggerR)
	{
		//(Y) button
		if(buttonY)
		{
			if(!justPressedY)
			{
				//autofire on/off
				switch_autofire = !switch_autofire;
				justPressedY=1;
			}
		}
		else if(justPressedY)
			justPressedY=0;

		//Q key
		if(keystate[SDLK_q])
		{
			if(!justPressedQ)
			{
				uae4all_keystate[AK_NPMUL] = 1;
				record_key(AK_NPMUL << 1);
				uae4all_keystate[AK_F10] = 1;
				record_key(AK_F10 << 1);				
				justPressedQ=1;
			}
		}
		else if(justPressedQ)
		{
			uae4all_keystate[AK_NPMUL] = 0;
			record_key((AK_NPMUL << 1) | 1);
			uae4all_keystate[AK_F10] = 0;
			record_key((AK_F10 << 1) | 1);
			justPressedQ=0;
		}

		//cutRight
		if(keystate[SDLK_COMMA] && mainMenu_cutRight > 0)
		{
			mainMenu_cutRight--;
			update_display();
		}
		else if(keystate[SDLK_PERIOD] && mainMenu_cutRight < 100)
		{
			mainMenu_cutRight++;
			update_display();
		}
	}

	if (mainMenu_customControls && !gp2xMouseEmuOn && !gp2xButtonRemappingOn)
	{
		if(mainMenu_custom_dpad == 0)
		{
			//UP
			if(dpadUp)
			{
				if(!justMovedUp)
				{
					if(mainMenu_custom_up == -1) buttonstate[0]=1;
					else if(mainMenu_custom_up == -2) buttonstate[2]=1;
					else if(mainMenu_custom_up > 0)
					{
						getMapping(mainMenu_custom_up);
						uae4all_keystate[customKey] = 1;
						record_key(customKey << 1);
					}
					justMovedUp=1;
				}
			}
			else if(justMovedUp)
			{
				if(mainMenu_custom_up == -1) buttonstate[0]=0;
				else if(mainMenu_custom_up == -2) buttonstate[2]=0;
				else if(mainMenu_custom_up > 0)
				{		
					getMapping(mainMenu_custom_up);
					uae4all_keystate[customKey] = 0;
					record_key((customKey << 1) | 1);
				}
				justMovedUp=0;
			}

			//DOWN
			if(dpadDown)
			{
				if(!justMovedDown)
				{
					if(mainMenu_custom_down == -1) buttonstate[0]=1;
					else if(mainMenu_custom_down == -2) buttonstate[2]=1;
					else if(mainMenu_custom_down > 0)
					{
						getMapping(mainMenu_custom_down);
						uae4all_keystate[customKey] = 1;
						record_key(customKey << 1);
					}
					justMovedDown=1;
				}
			}
			else if(justMovedDown)
			{
				if(mainMenu_custom_down == -1) buttonstate[0]=0;
				else if(mainMenu_custom_down == -2) buttonstate[2]=0;
				else if(mainMenu_custom_down > 0)
				{		
					getMapping(mainMenu_custom_down);
					uae4all_keystate[customKey] = 0;
					record_key((customKey << 1) | 1);
				}
				justMovedDown=0;
			}

			//LEFT
			if(dpadLeft)
			{
				if(!justMovedLeft)
				{
					if(mainMenu_custom_left == -1) buttonstate[0]=1;
					else if(mainMenu_custom_left == -2) buttonstate[2]=1;
					else if(mainMenu_custom_left > 0)
					{
						getMapping(mainMenu_custom_left);
						uae4all_keystate[customKey] = 1;
						record_key(customKey << 1);
					}
					justMovedLeft=1;
				}
			}
			else if(justMovedLeft)
			{
				if(mainMenu_custom_left == -1) buttonstate[0]=0;
				else if(mainMenu_custom_left == -2) buttonstate[2]=0;
				else if(mainMenu_custom_left > 0)
				{		
					getMapping(mainMenu_custom_left);
					uae4all_keystate[customKey] = 0;
					record_key((customKey << 1) | 1);
				}
				justMovedLeft=0;
			}

 			//RIGHT
			if(dpadRight)
			{
				if(!justMovedRight)
				{
					if(mainMenu_custom_right == -1) buttonstate[0]=1;
					else if(mainMenu_custom_right == -2) buttonstate[2]=1;
					else if(mainMenu_custom_right > 0)
					{
						getMapping(mainMenu_custom_right);
						uae4all_keystate[customKey] = 1;
						record_key(customKey << 1);
					}
					justMovedRight=1;
				}
			}
			else if(justMovedRight)
			{
				if(mainMenu_custom_right == -1) buttonstate[0]=0;
				else if(mainMenu_custom_right == -2) buttonstate[2]=0;
				else if(mainMenu_custom_right > 0)
				{		
					getMapping(mainMenu_custom_right);
					uae4all_keystate[customKey] = 0;
					record_key((customKey << 1) | 1);
				}
				justMovedRight=0;
			}
		}

		//(A)
		if(buttonA)
		{
			if(!justPressedA)
			{
				if(mainMenu_custom_A == -1) buttonstate[0]=1;
				else if(mainMenu_custom_A == -2) buttonstate[2]=1;
				else if(mainMenu_custom_A > 0)
				{
					getMapping(mainMenu_custom_A);
					uae4all_keystate[customKey] = 1;
					record_key(customKey << 1);
				}
				justPressedA=1;
			}
		}
		else if(justPressedA)
		{
			if(mainMenu_custom_A == -1) buttonstate[0]=0;
			else if(mainMenu_custom_A == -2) buttonstate[2]=0;
			else if(mainMenu_custom_A > 0)
			{
				getMapping(mainMenu_custom_A);
				uae4all_keystate[customKey] = 0;
				record_key((customKey << 1) | 1);
			}
			justPressedA=0;
		}

		//(B)
		if(buttonB)
		{
			if(!justPressedB)
			{
				if(mainMenu_custom_B == -1) buttonstate[0]=1;
				else if(mainMenu_custom_B == -2) buttonstate[2]=1;
				else if(mainMenu_custom_B > 0)
				{
					getMapping(mainMenu_custom_B);
					uae4all_keystate[customKey] = 1;
					record_key(customKey << 1);
				}
				justPressedB=1;
			}
		}
		else if(justPressedB)
		{
			if(mainMenu_custom_B == -1) buttonstate[0]=0;
			else if(mainMenu_custom_B == -2) buttonstate[2]=0;
			else if(mainMenu_custom_B > 0)
			{
				getMapping(mainMenu_custom_B);
				uae4all_keystate[customKey] = 0;
				record_key((customKey << 1) | 1);
			}
			justPressedB=0;
		}

		//(X)
		if(buttonX)
		{
			if(!justPressedX)
			{
				if(mainMenu_custom_X == -1) buttonstate[0]=1;
				else if(mainMenu_custom_X == -2) buttonstate[2]=1;
				else if(mainMenu_custom_X > 0)
				{
					getMapping(mainMenu_custom_X);
					uae4all_keystate[customKey] = 1;
					record_key(customKey << 1);
				}
				justPressedX=1;
			}
		}
		else if(justPressedX)
		{
			if(mainMenu_custom_X == -1) buttonstate[0]=0;
			else if(mainMenu_custom_X == -2) buttonstate[2]=0;
			else if(mainMenu_custom_X > 0)
			{		
				getMapping(mainMenu_custom_X);
				uae4all_keystate[customKey] = 0;
				record_key((customKey << 1) | 1);
			}
			justPressedX=0;
		}

		//(Y)
		if(buttonY)
		{
			if(!justPressedY)
			{
				if(mainMenu_custom_Y == -1) buttonstate[0]=1;
				else if(mainMenu_custom_Y == -2) buttonstate[2]=1;
				else if(mainMenu_custom_Y > 0)
				{
					getMapping(mainMenu_custom_Y);
					uae4all_keystate[customKey] = 1;
					record_key(customKey << 1);
				}
				justPressedY=1;
			}
		}
		else if(justPressedY)
		{
			if(mainMenu_custom_Y == -1) buttonstate[0]=0;
			else if(mainMenu_custom_Y == -2) buttonstate[2]=0;
			else if(mainMenu_custom_Y > 0)
			{		
				getMapping(mainMenu_custom_Y);
				uae4all_keystate[customKey] = 0;
				record_key((customKey << 1) | 1);
			}
			justPressedY=0;
		}

		//(L)
		if(triggerL)
		{
			if(!justPressedL)
			{
				if(mainMenu_custom_L == -1) buttonstate[0]=1;
				else if(mainMenu_custom_L == -2) buttonstate[2]=1;
				else if(mainMenu_custom_L > 0)
				{
					getMapping(mainMenu_custom_L);
					uae4all_keystate[customKey] = 1;
					record_key(customKey << 1);
				}
				justPressedL=1;
			}
		}
		else if(justPressedL)
		{
			if(mainMenu_custom_L == -1) buttonstate[0]=0;
			else if(mainMenu_custom_L == -2) buttonstate[2]=0;
			else if(mainMenu_custom_L > 0)
			{		
				getMapping(mainMenu_custom_L);
				uae4all_keystate[customKey] = 0;
				record_key((customKey << 1) | 1);
			}
			justPressedL=0;
		}

		//(R)
		if(triggerR)
		{
			if(!justPressedR)
			{
				if(mainMenu_custom_R == -1) buttonstate[0]=1;
				else if(mainMenu_custom_R == -2) buttonstate[2]=1;
				else if(mainMenu_custom_R > 0)
				{
					getMapping(mainMenu_custom_R);
					uae4all_keystate[customKey] = 1;
					record_key(customKey << 1);
				}
				justPressedR=1;
			}
		}
		else if(justPressedR)
		{
			if(mainMenu_custom_R == -1) buttonstate[0]=0;
			else if(mainMenu_custom_R == -2) buttonstate[2]=0;
			else if(mainMenu_custom_R > 0)
			{		
				getMapping(mainMenu_custom_R);
				uae4all_keystate[customKey] = 0;
				record_key((customKey << 1) | 1);
			}
			justPressedR=0;
		}
	}
	else if(!gp2xMouseEmuOn)
	{
		//DPad = arrow keys in stylus-mode
		if(gp2xButtonRemappingOn)
		{
			//dpad up
			if (dpadUp)
			{
				if(!justMovedUp)
				{
					//left and right mouse-buttons down
					buttonstate[0] = 1;
					buttonstate[2] = 1;
					stylusClickOverride = 1;
					justMovedUp=1;
				}
			}
			else if(justMovedUp)
			{
				//left and right mouse-buttons up
				buttonstate[0] = 0;
				buttonstate[2] = 0;
				stylusClickOverride = 0;
				justMovedUp=0;
			}
			//dpad down
			if (dpadDown)
			{
				if(!justMovedDown)
				{
					//no clicks with stylus now
					stylusClickOverride=1;
					justMovedDown=1;
				}
			}
			else if(justMovedDown)
			{
				//clicks active again
				stylusClickOverride=0;
				justMovedDown=0;
			}
			//dpad left
			if (dpadLeft)
			{
				if(!justMovedLeft)
				{
					//left mouse-button down
					buttonstate[0] = 1;
					stylusClickOverride = 1;
					justMovedLeft=1;
				}
			}
			else if(justMovedLeft)
			{
				//left mouse-button up
				buttonstate[0] = 0;
				stylusClickOverride = 0;
				justMovedLeft=0;
			}
			//dpad right
			if (dpadRight)
			{
				if(!justMovedRight)
				{
					//right mouse-button down
					buttonstate[2] = 1;
					stylusClickOverride = 1;
					justMovedRight=1;
				}
			}
			else if(justMovedRight)
			{
				//right mouse-button up
				buttonstate[2] = 0;
				stylusClickOverride = 0;
				justMovedRight=0;
			}
			//L + up
			if(triggerL && dpadUp)
				stylusAdjustY-=2;
			//L + down
			if(triggerL && dpadDown)
				stylusAdjustY+=2;
			//L + left
			if(triggerL && dpadLeft)
				stylusAdjustX-=2;
			//L + right
			if(triggerL && dpadRight)
				stylusAdjustX+=2;
		}
		//R-trigger in joystick mode
		else if(triggerR)
		{
			//(A) button
			if(buttonA)
			{
				if(!justPressedA)
				{
					//CTRL
					uae4all_keystate[AK_CTRL] = 1;
					record_key(AK_CTRL << 1);
					justPressedA=1;
				}
			}
			else if(justPressedA)
			{
				uae4all_keystate[AK_CTRL] = 0;
				record_key((AK_CTRL << 1) | 1);
				justPressedA=0;
			}
			//(B) button
			if(buttonB)
			{
				if(!justPressedB)
				{
					//left ALT
					uae4all_keystate[AK_LALT] = 1;
					record_key(AK_LALT << 1);
					justPressedB=1;
				}
			}
			else if(justPressedB)
			{
				uae4all_keystate[AK_LALT] = 0;
				record_key((AK_LALT << 1) | 1);
				justPressedB=0;
			}
			//(X) button
			if(buttonX)
			{
				if(!justPressedX)
				{
					//HELP
					uae4all_keystate[AK_HELP] = 1;
					record_key(AK_HELP << 1);
					justPressedX=1;
				}
			}
			else if(justPressedX)
			{
				//HELP
				uae4all_keystate[AK_HELP] = 0;
				record_key((AK_HELP << 1) | 1);
				justPressedX=0;
			}
		}
		else if(triggerL)
		{
			//(A) button
			if(buttonA)
			{
				if(!justPressedA)
				{
					//left mouse-button down
					buttonstate[0] = 1;
					justPressedA=1;
				}
			}
			else if(justPressedA)
			{
				//left mouse-button up
				buttonstate[0] = 0;
				justPressedA=0;
			}
			//(B) button
			if(buttonB)
			{
				if(!justPressedB)
				{
					//right mouse-button down
					buttonstate[2] = 1;
					justPressedB=1;
				}
			}
			else if(justPressedB)
			{
				//right mouse-button up
				buttonstate[2] = 0;
				justPressedB=0;
			}
		}
		else if(mainMenu_joyConf<2)
		{
			if(buttonY)
			{
				if(!justPressedY)
				{
					//SPACE
					uae4all_keystate[AK_SPC] = 1;
					record_key(AK_SPC << 1);
					justPressedY=1;
				}
			}
			else if(justPressedY)
			{
				//SPACE
				uae4all_keystate[AK_SPC] = 0;
				record_key((AK_SPC << 1) | 1);
				justPressedY=0;
			}
		}
	}
	else
	{
		if(buttonA)
		{
			if(!justPressedA)
			{
				//left mouse-button down
				buttonstate[0] = 1;
				justPressedA=1;
			}
		}
		else if(justPressedA)
		{
			//left mouse-button up
			buttonstate[0] = 0;
			justPressedA=0;
		}
		//(B) button
		if(buttonB)
		{
			if(!justPressedB)
			{
				//left mouse-button down
				buttonstate[2] = 1;
				justPressedB=1;
			}
		}
		else if(justPressedB)
		{
			//left mouse-button up
			buttonstate[2] = 0;
			justPressedB=0;
		}
		if(buttonY)
		{
			if(!justPressedY)
			{
				//SPACE
				uae4all_keystate[AK_SPC] = 1;
				record_key(AK_SPC << 1);
				justPressedY=1;
			}
		}
		else if(justPressedY)
		{
			//SPACE
			uae4all_keystate[AK_SPC] = 0;
			record_key((AK_SPC << 1) | 1);
			justPressedY=0;
		}
		if(dpadLeft)
		{
			if(!justMovedLeft)
			{
				//left ALT
				uae4all_keystate[0x64] = 1;
				record_key(0x64 << 1);
				justMovedLeft=1;
			}
		}
		else if(justMovedLeft)
		{
			//left ALT
			uae4all_keystate[0x64] = 0;
			record_key((0x64 << 1) | 1);
			justMovedLeft=0;
		}
		if(dpadRight)
		{
			if(!justMovedRight)
			{
				//left ALT
				uae4all_keystate[0x64] = 1;
				record_key(0x64 << 1);
				justMovedRight=1;
			}
		}
		else if(justMovedRight)
		{
			//left ALT
			uae4all_keystate[0x64] = 0;
			record_key((0x64 << 1) | 1);
			justMovedRight=0;
		}
	}

	if(!mainMenu_customControls && triggerR)
	{
		//R+dpad = arrow keys in joystick mode
		//dpad up
		if(dpadUp)
		{
			if(!justMovedUp)
			{
				//arrow up
				uae4all_keystate[0x4C] = 1;
				record_key(0x4C << 1);
				justMovedUp=1;
			}
		}
		else if(justMovedUp)
		{
			//arrow up
			uae4all_keystate[0x4C] = 0;
			record_key((0x4C << 1) | 1);
			justMovedUp=0;
		}
		//dpad down
		if(dpadDown)
		{
			if(!justMovedDown)
			{
				//arrow down
				uae4all_keystate[0x4D] = 1;
				record_key(0x4D << 1);
				justMovedDown=1;
			}
		}
		else if(justMovedDown)
		{
			//arrow down
			uae4all_keystate[0x4D] = 0;
			record_key((0x4D << 1) | 1);
			justMovedDown=0;
		}
		//dpad left
		if(dpadLeft)
		{
			if(!justMovedLeft)
			{
				//arrow left
				uae4all_keystate[0x4F] = 1;
				record_key(0x4F << 1);
				justMovedLeft=1;
			}
		}
		else if(justMovedLeft)
		{
			//arrow left
			uae4all_keystate[0x4F] = 0;
			record_key((0x4F << 1) | 1);
			justMovedLeft=0;
		}
		//dpad right
		if (dpadRight)
		{
			if(!justMovedRight)
			{
				//arrow right
				uae4all_keystate[0x4E] = 1;
				record_key(0x4E << 1);
				justMovedRight=1;
			}
		}
		else if(justMovedRight)
		{
			//arrow right
			uae4all_keystate[0x4E] = 0;
			record_key((0x4E << 1) | 1);
			justMovedRight=0;
		}
	}
} // if(!vkbd_mode)

#ifdef USE_UAE4ALL_VKBD
	//L+K: virtual keyboard
	if(triggerL && keystate[SDLK_k])
	{
		if(!justLK)
		{
			vkbd_mode = !vkbd_mode;
			justLK=1;
		}
	}
	else if(justLK)
		justLK=0;
#endif

	if(triggerL && keystate[SDLK_s])
	{
		keystate[SDLK_s]=0;
		savestate_state = STATE_DOSAVE;
	}
	if(triggerL && keystate[SDLK_l])
	{
		extern char *savestate_filename;
		FILE *f=fopen(savestate_filename, "rb");
		keystate[SDLK_l]=0;
		if(f)
		{
			fclose(f);
			savestate_state = STATE_DORESTORE;
		}
		else
			gui_set_message("Failed: Savestate not found", 100);
	}

#ifdef USE_UAE4ALL_VKBD
	if (vkbd_key)
	{
		if (vkbd_keysave==-1234567)
		{
			SDL_keysym ks;
			ks.sym=vkbd_key;
			vkbd_keysave=keycode2amiga(&ks);
			if (vkbd_keysave >= 0)
			{
				if (!uae4all_keystate[vkbd_keysave])
				{
					uae4all_keystate[vkbd_keysave]=1;
					record_key(vkbd_keysave<<1);
				}
			}
		}
	}
	else if (vkbd_keysave!=-1234567)
	{
		if (vkbd_keysave >= 0)
		{
			uae4all_keystate[vkbd_keysave]=0;
			record_key((vkbd_keysave << 1) | 1);
		}
		vkbd_keysave=-1234567;
	}
#endif
}

void gui_set_message(const char *msg, int t)
{
	return;

	show_message=t;
	strncpy(show_message_str, msg, 36);
}

void gui_show_window_bar(int per, int max, int case_title)
{
	return;

	char *title;
	if (case_title)
		title=(char*)"  Restore State";
	else
		title=(char*)"  Save State";
	_text_draw_window_bar(prSDLScreen,80,64,172,48,per,max,title);
	SDL_Flip(prSDLScreen);
}
