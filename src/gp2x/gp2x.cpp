#include <sys/file.h>

#ifndef WIN32
#include <sys/ioctl.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <sys/mman.h>
#endif

#include <unistd.h>
#include <fcntl.h>

#if !( defined(WIN32) || defined(ANDROIDSDL) )
#include <sys/soundcard.h>
#endif

#include <sys/time.h>

#ifndef WIN32
#include <sys/resource.h>
#endif

#include <string.h>
#include <stdlib.h>

#if !( defined(WIN32) || defined(ANDROIDSDL) )
#include <linux/soundcard.h>
#endif

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "SDL.h"
#include "gp2x.h"
#include "gp2xutil.h"

#include "menu/menu.h"

extern int uae4all_keystate[256];
extern void record_key(int);

int vol = 100;
unsigned long memDev;

extern SDL_Joystick *uae4all_joy0;

static int mixerdev;
int soundVolume = 50;
int flashLED;

int gp2xMouseEmuOn=0;
int gp2xButtonRemappingOn=0;
#ifndef PANDORA
int hasGp2xButtonRemapping=1;
#endif
int GFXVIDINFO_HEIGHT=240;
char launchDir[300];
char currentDir[300];

extern int graphics_init (void);

#define GPIOHOUT 0x106E
#define GPIOHPINLVL 0x118E


unsigned long gp2x_joystick_read(int allow_usb_joy)
{
	unsigned long value = 0;
	char u,d,l,r,ul,ur,dl,dr;
	
	SDL_JoystickUpdate();
	r  = (SDL_JoystickGetAxis(uae4all_joy0, 0) > 0) ? 1 : 0;
	l  = (SDL_JoystickGetAxis(uae4all_joy0, 0) < 0) ? 1 : 0;
	u  = (SDL_JoystickGetAxis(uae4all_joy0, 1) < 0) ? 1 : 0;
	d  = (SDL_JoystickGetAxis(uae4all_joy0, 1) > 0) ? 1 : 0;
	ul = (u && l) ? 1 : 0; 
	ur = (u && r) ? 1 : 0;
	dl = (d && l) ? 1 : 0;
	dr = (d && r) ? 1 : 0;
	
	if (r)  value |= GP2X_RIGHT;
	if (l)  value |= GP2X_LEFT;
	if (u)  value |= GP2X_UP;
	if (d)  value |= GP2X_DOWN;
	if (ul) value |= GP2X_UP_LEFT;
	if (ur) value |= GP2X_UP_RIGHT;
	if (dl) value |= GP2X_DOWN_LEFT;
	if (dr) value |= GP2X_DOWN_RIGHT;
	
	if (SDL_JoystickGetButton(uae4all_joy0, GP2X_BUTTON_A))	     value |= GP2X_A;
	if (SDL_JoystickGetButton(uae4all_joy0, GP2X_BUTTON_X))	     value |= GP2X_X;
	if (SDL_JoystickGetButton(uae4all_joy0, GP2X_BUTTON_Y))	     value |= GP2X_Y;
	if (SDL_JoystickGetButton(uae4all_joy0, GP2X_BUTTON_B))	     value |= GP2X_B;
	if (SDL_JoystickGetButton(uae4all_joy0, GP2X_BUTTON_L))	     value |= GP2X_L;
	if (SDL_JoystickGetButton(uae4all_joy0, GP2X_BUTTON_R))	     value |= GP2X_R;
	if (SDL_JoystickGetButton(uae4all_joy0, GP2X_BUTTON_START))  value |= GP2X_START;
	if (SDL_JoystickGetButton(uae4all_joy0, GP2X_BUTTON_SELECT)) value |= GP2X_SELECT;

	return value;
}


void gp2x_init(int argc, char **argv)
{
	unsigned long memdev;

	mixerdev = open("/dev/mixer", O_RDWR);
	
	SDL_ShowCursor(SDL_DISABLE);
	getcwd(launchDir, 250);
	getcwd(currentDir, 250);
	strcat(currentDir,"/roms/");
}

void gp2x_close( void )
{
}

int is_overridden_button(int button)
{
	// TODO: load from file
	return button == GP2X_BUTTON_L || button == GP2X_BUTTON_R || 
		   button == GP2X_BUTTON_A || button == GP2X_BUTTON_B ||
		   button == GP2X_BUTTON_X || button == GP2X_BUTTON_Y;
}

int get_key_for_button(int button)
{
	return 0;
}

// apply the remapped button keystroke
void handle_remapped_button_down(int button)
{
	int key = get_key_for_button(button);
	if (!uae4all_keystate[key])
	{
		uae4all_keystate[key] = 1;
		record_key(key << 1);
	}
}

void handle_remapped_button_up(int button)
{
	int key = get_key_for_button(button);

	if (uae4all_keystate[key])
	{
		uae4all_keystate[key] = 0;
		record_key((key << 1) | 1);
	}
}

void switch_to_hw_sdl(int first_time)
{
	GFXVIDINFO_HEIGHT=240;
	SDL_ShowCursor(SDL_DISABLE);
	usleep(100*1000);

	// reinit video
	graphics_init();
}

void switch_to_sw_sdl(void)
{
}

