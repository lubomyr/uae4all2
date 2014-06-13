 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Joystick emulation for Linux and BSD. They share too much code to
  * split this file.
  * 
  * Copyright 1997 Bernd Schmidt
  * Copyright 1998 Krister Walfridsson
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "uae.h"
#include "options.h"
#include "memory-uae.h"
#include "custom.h"
#include "joystick.h"
#include "SDL.h"
#include "menu.h"
#include "menu_config.h"

#ifdef USE_UAE4ALL_VKBD
#include "vkbd.h"
#endif

#ifdef GP2X
#include "gp2x.h"
#include "xwin.h"
extern int gp2xMouseEmuOn;
extern int gp2xButtonRemappingOn;
#if defined(PANDORA) || defined (ANDROIDSDL)
extern int dpadUp;
extern int dpadDown;
extern int dpadLeft;
extern int dpadRight;
extern int buttonA;
extern int buttonB;
extern int buttonX;
extern int buttonY;
extern int triggerL;
extern int triggerR;
extern int buttonSelect;
extern int buttonStart;
#endif

extern char launchDir[300];
bool switch_autofire=false;
int delay=0;
#endif


int nr_joysticks;

SDL_Joystick *uae4all_joy0, *uae4all_joy1;
extern SDL_Surface *prSDLScreen;

void read_joystick(int nr, unsigned int *dir, int *button)
{
#ifndef MAX_AUTOEVENTS
    int x_axis, y_axis;
    int left = 0, right = 0, top = 0, bot = 0, upRight=0, downRight=0, upLeft=0, downLeft=0, x=0, y=0, a=0, b=0;
    int len, i, num;
    SDL_Joystick *joy = nr == 0 ? uae4all_joy0 : uae4all_joy1;

    *dir = 0;
    *button = 0;

    nr = (~nr)&0x1;

    SDL_JoystickUpdate ();

	int mouseScale = mainMenu_mouseMultiplier * 4;
	if (mouseScale > 99)
		mouseScale /= 100;

#ifdef USE_UAE4ALL_VKBD
	if (!vkbd_mode && ((mainMenu_customControls && mainMenu_custom_dpad==2) || gp2xMouseEmuOn || (triggerL && !triggerR && !gp2xButtonRemappingOn)))
#else
	if (((mainMenu_customControls && mainMenu_custom_dpad==2) || gp2xMouseEmuOn || (triggerL && !triggerR && !gp2xButtonRemappingOn)))
#endif
	{
		if (buttonY)
			mouseScale = mainMenu_mouseMultiplier;
#if defined(PANDORA) || defined(ANDROIDSDL)
		if (dpadLeft)
#else
		if (SDL_JoystickGetButton(joy, GP2X_BUTTON_LEFT))
#endif
		{
			lastmx -= mouseScale;
			newmousecounters=1;
		}
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
		if (SDL_JoystickGetButton(joy, GP2X_BUTTON_UPLEFT))
		{
			lastmx -= mouseScale;
			lastmy -= mouseScale;
			newmousecounters=1;
		}
		if (SDL_JoystickGetButton(joy, GP2X_BUTTON_DOWNLEFT))
		{
			lastmx -= mouseScale;
			lastmy += mouseScale;
			newmousecounters=1;
		}
#endif
		if (dpadRight)
		{
			lastmx += mouseScale;
			newmousecounters=1;
		}
		if (dpadUp)
		{    
			lastmy -= mouseScale;
			newmousecounters=1;
		}
		if (dpadDown)
		{
			lastmy += mouseScale;
			newmousecounters=1;
		}
	}
	else if (!triggerR /*R+dpad = arrow keys*/ && !(mainMenu_customControls && !mainMenu_custom_dpad))
	{
		if (dpadRight || SDL_JoystickGetAxis(joy, 0) > 0) right=1;
		if (dpadLeft || SDL_JoystickGetAxis(joy, 0) < 0) left=1;
		if (dpadUp || SDL_JoystickGetAxis(joy, 1) < 0) top=1;
		if (dpadDown || SDL_JoystickGetAxis(joy, 1) > 0) bot=1;
		if (mainMenu_joyConf)
		{
#ifdef USE_UAE4ALL_VKBD
			if (((buttonX && mainMenu_jump > -1) ) && !vkbd_mode)
#else
			if (((buttonX && mainMenu_jump > -1) || SDL_JoystickGetButton(joy, mainMenu_jump)))
#endif
				top = 1;
		}
	}

#ifdef USE_UAE4ALL_VKBD
	if(mainMenu_customControls && !vkbd_mode)
#else
	if(mainMenu_customControls)
#endif
	{
		if((mainMenu_custom_A==-5 && buttonA) || (mainMenu_custom_B==-5 && buttonB) || (mainMenu_custom_X==-5 && buttonX) || (mainMenu_custom_Y==-5 && buttonY) || (mainMenu_custom_L==-5 && triggerL) || (mainMenu_custom_R==-5 && triggerR))
			top = 1;
		else if(mainMenu_custom_dpad == 0)
		{
			if((mainMenu_custom_up==-5 && dpadUp) || (mainMenu_custom_down==-5 && dpadDown) || (mainMenu_custom_left==-5 && dpadLeft) || (mainMenu_custom_right==-5 && dpadRight))
				top = 1;
		}

		if((mainMenu_custom_A==-6 && buttonA) || (mainMenu_custom_B==-6 && buttonB) || (mainMenu_custom_X==-6 && buttonX) || (mainMenu_custom_Y==-6 && buttonY) || (mainMenu_custom_L==-6 && triggerL) || (mainMenu_custom_R==-6 && triggerR))
			bot = 1;
		else if(mainMenu_custom_dpad == 0)
		{
			if((mainMenu_custom_up==-6 && dpadUp) || (mainMenu_custom_down==-6 && dpadDown) || (mainMenu_custom_left==-6 && dpadLeft) || (mainMenu_custom_right==-6 && dpadRight))
				bot = 1;
		}

		if((mainMenu_custom_A==-7 && buttonA) || (mainMenu_custom_B==-7 && buttonB) || (mainMenu_custom_X==-7 && buttonX) || (mainMenu_custom_Y==-7 && buttonY) || (mainMenu_custom_L==-7 && triggerL) || (mainMenu_custom_R==-7 && triggerR))
			left = 1;
		else if(mainMenu_custom_dpad == 0)
		{
			if((mainMenu_custom_up==-7 && dpadUp) || (mainMenu_custom_down==-7 && dpadDown) || (mainMenu_custom_left==-7 && dpadLeft) || (mainMenu_custom_right==-7 && dpadRight))
				left = 1;
		}

		if((mainMenu_custom_A==-8 && buttonA) || (mainMenu_custom_B==-8 && buttonB) || (mainMenu_custom_X==-8 && buttonX) || (mainMenu_custom_Y==-8 && buttonY) || (mainMenu_custom_L==-8 && triggerL) || (mainMenu_custom_R==-8 && triggerR))
			right = 1;
		else if(mainMenu_custom_dpad == 0)
		{
			if((mainMenu_custom_up==-8 && dpadUp) || (mainMenu_custom_down==-8 && dpadDown) || (mainMenu_custom_left==-8 && dpadLeft) || (mainMenu_custom_right==-8 && dpadRight))
				right = 1;
		}
	}

  if(!gp2xMouseEmuOn && !gp2xButtonRemappingOn)
  {
  	if(!mainMenu_customControls && ((mainMenu_autofire & switch_autofire & delay>mainMenu_autofireRate) || (((mainMenu_autofireButton1==GP2X_BUTTON_B && buttonA) || (mainMenu_autofireButton1==GP2X_BUTTON_X && buttonX) || (mainMenu_autofireButton1==GP2X_BUTTON_Y && buttonY)) & delay>mainMenu_autofireRate)))
  	{
  		if(!buttonB)
  			*button=1;
  		delay=0;
  		*button |= (buttonB & 1) << 1;
  	}
#ifdef USE_UAE4ALL_VKBD
  	else if(mainMenu_customControls && !vkbd_mode)
#else
  	else if(mainMenu_customControls)
#endif
  	{
  		if((mainMenu_custom_A==-3 && buttonA) || (mainMenu_custom_B==-3 && buttonB) || (mainMenu_custom_X==-3 && buttonX) || (mainMenu_custom_Y==-3 && buttonY) || (mainMenu_custom_L==-3 && triggerL) || (mainMenu_custom_R==-3 && triggerR))
  			*button = 1;
  		else if(mainMenu_custom_dpad == 0)
  		{
  			if((mainMenu_custom_up==-3 && dpadUp) || (mainMenu_custom_down==-3 && dpadDown) || (mainMenu_custom_left==-3 && dpadLeft) || (mainMenu_custom_right==-3 && dpadRight))
  				*button = 1;
  		}
  
  		if((mainMenu_custom_A==-4 && buttonA) || (mainMenu_custom_B==-4 && buttonB) || (mainMenu_custom_X==-4 && buttonX) || (mainMenu_custom_Y==-4 && buttonY) || (mainMenu_custom_L==-4 && triggerL) || (mainMenu_custom_R==-4 && triggerR))
  			*button |= 1 << 1;
  		else if(mainMenu_custom_dpad == 0)
  		{
  			if((mainMenu_custom_up==-4 && dpadUp) || (mainMenu_custom_down==-4 && dpadDown) || (mainMenu_custom_left==-4 && dpadLeft) || (mainMenu_custom_right==-4 && dpadRight))
  				*button |= 1 << 1;
  		}
  		delay++;
  	}
  	else
  	{
#ifndef ANDROIDSDL
   		*button = ((mainMenu_button1==GP2X_BUTTON_B && buttonA) || (mainMenu_button1==GP2X_BUTTON_X && buttonX) || (mainMenu_button1==GP2X_BUTTON_Y && buttonY) || SDL_JoystickGetButton(joy, mainMenu_button1)) & 1;
#else
  		*button = ((mainMenu_button1==GP2X_BUTTON_B && buttonA) || (mainMenu_button1==GP2X_BUTTON_X && buttonX) || (mainMenu_button1==GP2X_BUTTON_Y && buttonY)) & 1;
#endif
  		delay++;
#ifdef PANDORA
  		*button |= ((buttonB || SDL_JoystickGetButton(joy, mainMenu_button2)) & 1) << 1;
#else
  		*button |= ((buttonB) & 1) << 1;
#endif
  	}
  }

#ifdef USE_UAE4ALL_VKBD
	if (vkbd_mode && nr)
	{
		// move around the virtual keyboard instead
		if (left)
			vkbd_move |= VKBD_LEFT;
		else
		{
			vkbd_move &= ~VKBD_LEFT;
			if (right)
				vkbd_move |= VKBD_RIGHT;
			else
				vkbd_move &= ~VKBD_RIGHT;
		}
		if (top)
			vkbd_move |= VKBD_UP;
		else
		{
			vkbd_move &= ~VKBD_UP;
			if (bot)
				vkbd_move |= VKBD_DOWN;
			else
				vkbd_move &= ~VKBD_DOWN;
		}
		if (*button)
		{
			vkbd_move=VKBD_BUTTON;
			*button=0;
		}
		// TODO: add vkbd_button2 mapped to button2
	}
	else
#endif
	{
	// normal joystick movement
  if (left) top = !top;
	if (right) bot = !bot;
	*dir = bot | (right << 1) | (top << 8) | (left << 9);
    }

  if(mainMenu_joyPort != 0)
  {
    // Only one joystick active    
    if((nr == 0 && mainMenu_joyPort == 2) || (nr == 1 && mainMenu_joyPort == 1))
    {
      *dir = 0;
      *button = 0;
    }
  }
#endif
}

void init_joystick(void)
{
    int i;
    nr_joysticks = SDL_NumJoysticks ();
    if (nr_joysticks > 0)
	uae4all_joy0 = SDL_JoystickOpen (0);
    if (nr_joysticks > 1)
	uae4all_joy1 = SDL_JoystickOpen (1);
    else
	uae4all_joy1 = NULL;
}

void close_joystick(void)
{
    if (nr_joysticks > 0)
	SDL_JoystickClose (uae4all_joy0);
    if (nr_joysticks > 1)
	SDL_JoystickClose (uae4all_joy1);
}
