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

#define MAX_CUSTOM_ID 96
#define MIN_CUSTOM_ID -8

const char *text_str_controls_separator="----------------------------------";
const char *text_str_controls_title=    "         Custom Controls         -";
char mapping[32]="";
int menuControls = 0;

enum { 
	MENUCONTROLS_RETURNMAIN = 0,
#ifdef ANDROIDSDL
  MENUCONTROLS_ONSCREEN,
#endif
  MENUCONTROLS_CUSTOM_ON_OFF,
  MENUCONTROLS_DPAD,
  MENUCONTROLS_UP,
  MENUCONTROLS_DOWN,
  MENUCONTROLS_LEFT,
  MENUCONTROLS_RIGHT,
  MENUCONTROLS_A,
  MENUCONTROLS_B,
  MENUCONTROLS_X,
  MENUCONTROLS_Y,
  MENUCONTROLS_L,
  MENUCONTROLS_R,
	MENUCONTROLS_END
};

static void getMapping(int customId)
{
	switch(customId)
	{
		case -8: strcpy(mapping, "Joystick RIGHT"); break;
		case -7: strcpy(mapping, "Joystick LEFT"); break;
		case -6: strcpy(mapping, "Joystick DOWN"); break;
		case -5: strcpy(mapping, "Jump (Joystick UP)"); break;
		case -4: strcpy(mapping, "Joystick fire button 2"); break;
		case -3: strcpy(mapping, "Joystick fire button 1"); break;
		case -2: strcpy(mapping, "Mouse right button"); break;
		case -1: strcpy(mapping, "Mouse left button"); break;
		case 0: strcpy(mapping, "---"); break;
		case 1: strcpy(mapping, "arrow UP"); break;
		case 2: strcpy(mapping, "arrow DOWN"); break;
		case 3: strcpy(mapping, "arrow LEFT"); break;
		case 4: strcpy(mapping, "arrow RIGHT"); break;
		case 5: strcpy(mapping, "numpad 0"); break;
		case 6: strcpy(mapping, "numpad 1"); break;
		case 7: strcpy(mapping, "numpad 2"); break;
		case 8: strcpy(mapping, "numpad 3"); break;
		case 9: strcpy(mapping, "numpad 4"); break;
		case 10: strcpy(mapping, "numpad 5"); break;
		case 11: strcpy(mapping, "numpad 6"); break;
		case 12: strcpy(mapping, "numpad 7"); break;
		case 13: strcpy(mapping, "numpad 8"); break;
		case 14: strcpy(mapping, "numpad 9"); break;
		case 15: strcpy(mapping, "numpad ENTER"); break;
		case 16: strcpy(mapping, "numpad DIVIDE"); break;
		case 17: strcpy(mapping, "numpad MULTIPLY"); break;
		case 18: strcpy(mapping, "numpad MINUS"); break;
		case 19: strcpy(mapping, "numpad PLUS"); break;
		case 20: strcpy(mapping, "numpad DELETE"); break;
		case 21: strcpy(mapping, "numpad LEFT PARENTHESIS"); break;
		case 22: strcpy(mapping, "numpad RIGHT PARENTHESIS"); break;
		case 23: strcpy(mapping, "SPACE"); break;
		case 24: strcpy(mapping, "BACKSPACE"); break;
		case 25: strcpy(mapping, "TAB"); break;
		case 26: strcpy(mapping, "RETURN"); break;
		case 27: strcpy(mapping, "ESCAPE"); break;
		case 28: strcpy(mapping, "DELETE"); break;
		case 29: strcpy(mapping, "left SHIFT"); break;
		case 30: strcpy(mapping, "right SHIFT"); break;
		case 31: strcpy(mapping, "CAPS LOCK"); break;
		case 32: strcpy(mapping, "CTRL"); break;
		case 33: strcpy(mapping, "left ALT"); break;
		case 34: strcpy(mapping, "right ALT"); break;
		case 35: strcpy(mapping, "left AMIGA key"); break;
		case 36: strcpy(mapping, "right AMIGA key"); break;
		case 37: strcpy(mapping, "HELP"); break;
		case 38: strcpy(mapping, "left bracket"); break;
		case 39: strcpy(mapping, "right bracket"); break;
		case 40: strcpy(mapping, "semicolon"); break;
		case 41: strcpy(mapping, "comma"); break;
		case 42: strcpy(mapping, "period"); break;
		case 43: strcpy(mapping, "slash"); break;
		case 44: strcpy(mapping, "backslash"); break;
		case 45: strcpy(mapping, "quote"); break;
		case 46: strcpy(mapping, "numbersign"); break;
		case 47: strcpy(mapping, "less than - greater than"); break;
		case 48: strcpy(mapping, "backquote"); break;
		case 49: strcpy(mapping, "minus"); break;
		case 50: strcpy(mapping, "equal"); break;
		case 51: strcpy(mapping, "A"); break;
		case 52: strcpy(mapping, "B"); break;
		case 53: strcpy(mapping, "C"); break;
		case 54: strcpy(mapping, "D"); break;
		case 55: strcpy(mapping, "E"); break;
		case 56: strcpy(mapping, "F"); break;
		case 57: strcpy(mapping, "G"); break;
		case 58: strcpy(mapping, "H"); break;
		case 59: strcpy(mapping, "I"); break;
		case 60: strcpy(mapping, "J"); break;
		case 61: strcpy(mapping, "K"); break;
		case 62: strcpy(mapping, "L"); break;
		case 63: strcpy(mapping, "M"); break;
		case 64: strcpy(mapping, "N"); break;
		case 65: strcpy(mapping, "O"); break;
		case 66: strcpy(mapping, "P"); break;
		case 67: strcpy(mapping, "Q"); break;
		case 68: strcpy(mapping, "R"); break;
		case 69: strcpy(mapping, "S"); break;
		case 70: strcpy(mapping, "T"); break;
		case 71: strcpy(mapping, "U"); break;
		case 72: strcpy(mapping, "V"); break;
		case 73: strcpy(mapping, "W"); break;
		case 74: strcpy(mapping, "X"); break;
		case 75: strcpy(mapping, "Y"); break;
		case 76: strcpy(mapping, "Z"); break;
		case 77: strcpy(mapping, "1"); break;
		case 78: strcpy(mapping, "2"); break;
		case 79: strcpy(mapping, "3"); break;
		case 80: strcpy(mapping, "4"); break;
		case 81: strcpy(mapping, "5"); break;
		case 82: strcpy(mapping, "6"); break;
		case 83: strcpy(mapping, "7"); break;
		case 84: strcpy(mapping, "8"); break;
		case 85: strcpy(mapping, "9"); break;
		case 86: strcpy(mapping, "0"); break;
		case 87: strcpy(mapping, "F1"); break;
		case 88: strcpy(mapping, "F2"); break;
		case 89: strcpy(mapping, "F3"); break;
		case 90: strcpy(mapping, "F4"); break;
		case 91: strcpy(mapping, "F5"); break;
		case 92: strcpy(mapping, "F6"); break;
		case 93: strcpy(mapping, "F7"); break;
		case 94: strcpy(mapping, "F8"); break;
		case 95: strcpy(mapping, "F9"); break;
		case 96: strcpy(mapping, "F10");
	}
	  /*
	  -8 joy right
	  -7 joy left
	  -6 joy down
    -5 joy up
		-4 joy fire button 2
		-3 joy fire button 1
		-2 mouse right button
		-1 mouse left button
		 0 ---
		 1 AK_UP 0x4C
		 2 AK_DN 0x4D
		 3 AK_LF 0x4F
		 4 AK_RT 0x4E
		 5 AK_NP0 0x0F
		 6 AK_NP1 0x1D
		 7 AK_NP2 0x1E
		 8 AK_NP3 0x1F
		 9 AK_NP4 0x2D
		10 AK_NP5 0x2E
		11 AK_NP6 0x2F
		12 AK_NP7 0x3D
		13 AK_NP8 0x3E
		14 AK_NP9 0x3F
		15 AK_ENT 0x43
		16 AK_NPDIV 0x5C
		17 AK_NPMUL 0x5D
		18 AK_NPSUB 0x4A
		19 AK_NPADD 0x5E
		20 AK_NPDEL 0x3C
		21 AK_NPLPAREN 0x5A
		22 AK_NPRPAREN 0x5B
		23 AK_SPC 0x40
		24 AK_BS 0x41
		25 AK_TAB 0x42
		26 AK_RET 0x44
		27 AK_ESC 0x45
		28 AK_DEL 0x46
		29 AK_LSH 0x60
		30 AK_RSH 0x61
		31 AK_CAPSLOCK 0x62
		32 AK_CTRL 0x63
		33 AK_LALT 0x64
		34 AK_RALT 0x65
		35 AK_LAMI 0x66
		36 AK_RAMI 0x67
		37 AK_HELP 0x5F
		38 AK_LBRACKET 0x1A
		39 AK_RBRACKET 0x1B
		40 AK_SEMICOLON 0x29
		41 AK_COMMA 0x38
		42 AK_PERIOD 0x39
		43 AK_SLASH 0x3A
		44 AK_BACKSLASH 0x0D
		45 AK_QUOTE 0x2A
		46 AK_NUMBERSIGN 0x2B
		47 AK_LTGT 0x30
		48 AK_BACKQUOTE 0x00
		49 AK_MINUS 0x0B
		50 AK_EQUAL 0x0C
		51 AK_A 0x20
		52 AK_B 0x35
		53 AK_C 0x33
		54 AK_D 0x22
		55 AK_E 0x12
		56 AK_F 0x23
		57 AK_G 0x24
		58 AK_H 0x25
		59 AK_I 0x17
		60 AK_J 0x26
		61 AK_K 0x27
		62 AK_L 0x28
		63 AK_M 0x37
		64 AK_N 0x36
		65 AK_O 0x18
		66 AK_P 0x19
		67 AK_Q 0x10
		68 AK_R 0x13
		69 AK_S 0x21
		70 AK_T 0x14
		71 AK_U 0x16
		72 AK_V 0x34
		73 AK_W 0x11
		74 AK_X 0x32
		75 AK_Y 0x15
		76 AK_Z 0x31
		77 AK_0 0x0A
		78 AK_1 0x01
		79 AK_2 0x02
		80 AK_3 0x03
		81 AK_4 0x04
		82 AK_5 0x05
		83 AK_6 0x06
		84 AK_7 0x07
		85 AK_8 0x08
		86 AK_9 0x09
		87 AK_F1 0x50
		88 AK_F2 0x51
		89 AK_F3 0x52
		90 AK_F4 0x53
		91 AK_F5 0x54
		92 AK_F6 0x55
		93 AK_F7 0x56
		94 AK_F8 0x57
		95 AK_F9 0x58
		96 AK_F10 0x59*/
}

static void draw_controlsMenu(int c)
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
	r.x=80-64; r.y=0; r.w=110+64+64; r.h=240;

	text_draw_background();
	text_draw_window(2,2,40,30,text_str_controls_title);

	// MENUCONTROLS_RETURNMAIN
	if (menuControls == MENUCONTROLS_RETURNMAIN && bb)
		write_text_inv(3, menuLine, "Return to main menu");
	else
		write_text(3, menuLine, "Return to main menu");

	menuLine++;
	write_text(leftMargin,menuLine,text_str_controls_separator);
	menuLine++;

#ifdef ANDROIDSDL
	// MENUCONTROLS_ONSCREEN
	write_text(leftMargin,menuLine,"On-Screen Control");
	if ((mainMenu_onScreen==1)&&((menuControls!=MENUCONTROLS_ONSCREEN)||(bb)))
		write_text_inv(tabstop3,menuLine,"Show");
	else
		write_text(tabstop3,menuLine,"Show");
	if ((mainMenu_onScreen==0)&&((menuControls!=MENUCONTROLS_ONSCREEN)||(bb)))
		write_text_inv(tabstop6,menuLine,"Hide");
	else
		write_text(tabstop6,menuLine,"Hide");

	menuLine++;
	write_text(leftMargin,menuLine,text_str_controls_separator);
	menuLine++;	
#endif

	// MENUCONTROLS_CUSTOM_ON_OFF
	write_text(leftMargin,menuLine,"Custom Controls");
	if ((mainMenu_customControls==1)&&((menuControls!=MENUCONTROLS_CUSTOM_ON_OFF)||(bb)))
		write_text_inv(tabstop3,menuLine,"On");
	else
		write_text(tabstop3,menuLine,"On");
	if ((mainMenu_customControls==0)&&((menuControls!=MENUCONTROLS_CUSTOM_ON_OFF)||(bb)))
		write_text_inv(tabstop6,menuLine,"Off");
	else
		write_text(tabstop6,menuLine,"Off");

	menuLine++;
	write_text(leftMargin,menuLine,text_str_controls_separator);
	menuLine++;

	// MENUCONTROLS_DPAD
	write_text(leftMargin,menuLine," DPAD");
	if ((mainMenu_custom_dpad==0)&&((menuControls!=MENUCONTROLS_DPAD)||(bb)))
		write_text_inv(tabstop1-6,menuLine,"Custom");
	else
		write_text(tabstop1-6,menuLine,"Custom");
	if ((mainMenu_custom_dpad==1)&&((menuControls!=MENUCONTROLS_DPAD)||(bb)))
		write_text_inv(tabstop2,menuLine,"Joystick");
	else
		write_text(tabstop2,menuLine,"Joystick");
	if ((mainMenu_custom_dpad==2)&&((menuControls!=MENUCONTROLS_DPAD)||(bb)))
		write_text_inv(tabstop6+2,menuLine,"Mouse");
	else
		write_text(tabstop6+2,menuLine,"Mouse");

	if (mainMenu_custom_dpad==0)
	{
		// MENUCONTROLS_UP
		menuLine+=2;
		write_text(leftMargin,menuLine,"    up");
		getMapping(mainMenu_custom_up);
		if ((menuControls!=MENUCONTROLS_UP)||(bb))
			write_text_inv(tabstop1-4,menuLine,mapping);
		else
			write_text(tabstop1-4,menuLine,mapping);
		// MENUCONTROLS_DOWN
		menuLine+=2;
		write_text(leftMargin,menuLine,"  down");
		getMapping(mainMenu_custom_down);
		if ((menuControls!=MENUCONTROLS_DOWN)||(bb))
			write_text_inv(tabstop1-4,menuLine,mapping);
		else
			write_text(tabstop1-4,menuLine,mapping);
		// MENUCONTROLS_LEFT
		menuLine+=2;
		write_text(leftMargin,menuLine,"  left");
		getMapping(mainMenu_custom_left);
		if ((menuControls!=MENUCONTROLS_LEFT)||(bb))
			write_text_inv(tabstop1-4,menuLine,mapping);
		else
			write_text(tabstop1-4,menuLine,mapping);
		// MENUCONTROLS_RIGHT
		menuLine+=2;
		write_text(leftMargin,menuLine," right");
		getMapping(mainMenu_custom_right);
		if ((menuControls!=MENUCONTROLS_RIGHT)||(bb))
			write_text_inv(tabstop1-4,menuLine,mapping);
		else
			write_text(tabstop1-4,menuLine,mapping);
	}

	// MENUCONTROLS_A
	menuLine+=3;
	write_text(leftMargin,menuLine,"   (A)");
	getMapping(mainMenu_custom_A);
	if ((menuControls!=MENUCONTROLS_A)||(bb))
		write_text_inv(tabstop1-4,menuLine,mapping);
	else
		write_text(tabstop1-4,menuLine,mapping);
	// MENUCONTROLS_B
	menuLine+=2;
	write_text(leftMargin,menuLine,"   (B)");
	getMapping(mainMenu_custom_B);
	if ((menuControls!=MENUCONTROLS_B)||(bb))
		write_text_inv(tabstop1-4,menuLine,mapping);
	else
		write_text(tabstop1-4,menuLine,mapping);
	// MENUCONTROLS_X
	menuLine+=2;
	write_text(leftMargin,menuLine,"   (X)");
	getMapping(mainMenu_custom_X);
	if ((menuControls!=MENUCONTROLS_X)||(bb))
		write_text_inv(tabstop1-4,menuLine,mapping);
	else
		write_text(tabstop1-4,menuLine,mapping);
	// MENUCONTROLS_Y
	menuLine+=2;
	write_text(leftMargin,menuLine,"   (Y)");
	getMapping(mainMenu_custom_Y);
	if ((menuControls!=MENUCONTROLS_Y)||(bb))
		write_text_inv(tabstop1-4,menuLine,mapping);
	else
		write_text(tabstop1-4,menuLine,mapping);

	// MENUCONTROLS_L
	menuLine+=3;
	write_text(leftMargin,menuLine,"   (L)");
	getMapping(mainMenu_custom_L);
	if ((menuControls!=MENUCONTROLS_L)||(bb))
		write_text_inv(tabstop1-4,menuLine,mapping);
	else
		write_text(tabstop1-4,menuLine,mapping);
	// MENUCONTROLS_R
	menuLine+=2;
	write_text(leftMargin,menuLine,"   (R)");
	getMapping(mainMenu_custom_R);
	if ((menuControls!=MENUCONTROLS_R)||(bb))
		write_text_inv(tabstop1-4,menuLine,mapping);
	else
		write_text(tabstop1-4,menuLine,mapping);

	text_flip();
	b++;
}

static int key_controlsMenu(int *c)
{
	int end=0;
	static int delay=0;
	int left=0, right=0, up=0, down=0, hit0=0, hit1=0;
	SDL_Event event;
	delay ++;
	if (delay<5) return end;
	delay=0;

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
			if (menuControls==MENUCONTROLS_A && mainMenu_custom_dpad>0) menuControls=MENUCONTROLS_DPAD;
			else if (menuControls==MENUCONTROLS_RETURNMAIN) menuControls=MENUCONTROLS_R;
			else menuControls--;
		}
		else if (down)
		{
			if (menuControls==MENUCONTROLS_DPAD && mainMenu_custom_dpad>0) menuControls=MENUCONTROLS_A;
			else if (menuControls==MENUCONTROLS_R) menuControls=MENUCONTROLS_RETURNMAIN;
			else menuControls++;
		}
		switch (menuControls)
		{
			
#ifdef ANDROIDSDL
			case MENUCONTROLS_ONSCREEN:
				if ((left)||(right))
						mainMenu_onScreen = !mainMenu_onScreen;
				break;
#endif

			case MENUCONTROLS_CUSTOM_ON_OFF:
				if ((left)||(right))
						mainMenu_customControls = !mainMenu_customControls;
				break;
			case MENUCONTROLS_DPAD:
				if (left)
				{
					if (mainMenu_custom_dpad>0)
						mainMenu_custom_dpad--;
					else
						mainMenu_custom_dpad=2;
				}
				else if (right)
				{
					if (mainMenu_custom_dpad<2)
						mainMenu_custom_dpad++;
					else
						mainMenu_custom_dpad=0;
				}
				break;
			case MENUCONTROLS_UP:
				if (left)
				{
					if (mainMenu_custom_up>MIN_CUSTOM_ID)
						mainMenu_custom_up--;
					else
						mainMenu_custom_up=MAX_CUSTOM_ID;
				}
				else if (right)
				{
					if (mainMenu_custom_up<MAX_CUSTOM_ID)
						mainMenu_custom_up++;
					else
						mainMenu_custom_up=MIN_CUSTOM_ID;
				}
				break;
			case MENUCONTROLS_DOWN:
				if (left)
				{
					if (mainMenu_custom_down>MIN_CUSTOM_ID)
						mainMenu_custom_down--;
					else
						mainMenu_custom_down=MAX_CUSTOM_ID;
				}
				else if (right)
				{
					if (mainMenu_custom_down<MAX_CUSTOM_ID)
						mainMenu_custom_down++;
					else
						mainMenu_custom_down=MIN_CUSTOM_ID;
				}
				break;
			case MENUCONTROLS_LEFT:
				if (left)
				{
					if (mainMenu_custom_left>MIN_CUSTOM_ID)
						mainMenu_custom_left--;
					else
						mainMenu_custom_left=MAX_CUSTOM_ID;
				}
				else if (right)
				{
					if (mainMenu_custom_left<MAX_CUSTOM_ID)
						mainMenu_custom_left++;
					else
						mainMenu_custom_left=MIN_CUSTOM_ID;
				}
				break;
			case MENUCONTROLS_RIGHT:
				if (left)
				{
					if (mainMenu_custom_right>MIN_CUSTOM_ID)
						mainMenu_custom_right--;
					else
						mainMenu_custom_right=MAX_CUSTOM_ID;
				}
				else if (right)
				{
					if (mainMenu_custom_right<MAX_CUSTOM_ID)
						mainMenu_custom_right++;
					else
						mainMenu_custom_right=MIN_CUSTOM_ID;
				}
				break;
			case MENUCONTROLS_A:
				if (left)
				{
					if (mainMenu_custom_A>MIN_CUSTOM_ID)
						mainMenu_custom_A--;
					else
						mainMenu_custom_A=MAX_CUSTOM_ID;
				}
				else if (right)
				{
					if (mainMenu_custom_A<MAX_CUSTOM_ID)
						mainMenu_custom_A++;
					else
						mainMenu_custom_A=MIN_CUSTOM_ID;
				}
				break;
			case MENUCONTROLS_B:
				if (left)
				{
					if (mainMenu_custom_B>MIN_CUSTOM_ID)
						mainMenu_custom_B--;
					else
						mainMenu_custom_B=MAX_CUSTOM_ID;
				}
				else if (right)
				{
					if (mainMenu_custom_B<MAX_CUSTOM_ID)
						mainMenu_custom_B++;
					else
						mainMenu_custom_B=MIN_CUSTOM_ID;
				}
				break;
			case MENUCONTROLS_X:
				if (left)
				{
					if (mainMenu_custom_X>MIN_CUSTOM_ID)
						mainMenu_custom_X--;
					else
						mainMenu_custom_X=MAX_CUSTOM_ID;
				}
				else if (right)
				{
					if (mainMenu_custom_X<MAX_CUSTOM_ID)
						mainMenu_custom_X++;
					else
						mainMenu_custom_X=MIN_CUSTOM_ID;
				}
				break;
			case MENUCONTROLS_Y:
				if (left)
				{
					if (mainMenu_custom_Y>MIN_CUSTOM_ID)
						mainMenu_custom_Y--;
					else
						mainMenu_custom_Y=MAX_CUSTOM_ID;
				}
				else if (right)
				{
					if (mainMenu_custom_Y<MAX_CUSTOM_ID)
						mainMenu_custom_Y++;
					else
						mainMenu_custom_Y=MIN_CUSTOM_ID;
				}
				break;
			case MENUCONTROLS_L:
				if (left)
				{
					if (mainMenu_custom_L>MIN_CUSTOM_ID)
						mainMenu_custom_L--;
					else
						mainMenu_custom_L=MAX_CUSTOM_ID;
				}
				else if (right)
				{
					if (mainMenu_custom_L<MAX_CUSTOM_ID)
						mainMenu_custom_L++;
					else
						mainMenu_custom_L=MIN_CUSTOM_ID;
				}
				break;
			case MENUCONTROLS_R:
				if (left)
				{
					if (mainMenu_custom_R>MIN_CUSTOM_ID)
						mainMenu_custom_R--;
					else
						mainMenu_custom_R=MAX_CUSTOM_ID;
				}
				else if (right)
				{
					if (mainMenu_custom_R<MAX_CUSTOM_ID)
						mainMenu_custom_R++;
					else
						mainMenu_custom_R=MIN_CUSTOM_ID;
				}
				break;
		}
	}

	return end;
}

static void raise_controlsMenu()
{
	int i;

	text_draw_background();
	text_flip();
	for(i=0;i<10;i++)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_controls_title);
		text_flip();
	}
}

static void unraise_controlsMenu()
{
	int i;

	for(i=9;i>=0;i--)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_controls_title);
		text_flip();
	}
	text_draw_background();
	text_flip();
}

int run_menuControls()
{
	SDL_Event event;
	SDL_Delay(150);
	while(SDL_PollEvent(&event))
		SDL_Delay(10);
	int end=0, c=0;
	raise_controlsMenu();
	while(!end)
	{
		draw_controlsMenu(c);
		end=key_controlsMenu(&c);
	}
	set_joyConf();
	unraise_controlsMenu();
	return end;
}
