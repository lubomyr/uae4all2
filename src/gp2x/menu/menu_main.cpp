#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <SDL.h>

#if defined(ANDROID)
#include <SDL_screenkeyboard.h>
#endif

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "gp2xutil.h"
#include "menu.h"
#include "menu_config.h"
#include "autoconf.h"
#ifdef USE_UAE4ALL_VKBD
#include "vkbd.h"
#endif
#include "options.h"
#include "sound.h"
#include "zfile.h"
#include "gui.h"
#include "gp2x.h"
#include "disk.h"
#include "cpuspeed/cpuctrl.h"
#include "custom.h"

/* PocketUAE config file. Used for parsing PocketUAE-like options. */
#include "savestate.h"

extern int kickstart;
extern int oldkickstart;
extern int bReloadKickstart;
extern unsigned int sound_rate;
extern int skipintro;
extern int moveX;
extern int moveY;
extern int timeslice_mode;
extern int emulating;
extern int gp2xMouseEmuOn;
extern int gp2xButtonRemappingOn;

extern int init_sound(void);
extern void gp2x_stop_sound(void);
extern void leave_program(void);
extern void extractFileName(char * str,char *buffer);
extern void update_display(void);
extern int saveAdfDir(void);
extern void setCpuSpeed(void);

extern char launchDir[300];
extern char currentDir[300];

extern char filename0[256];
extern char filename1[256];
extern char filename2[256];
extern char filename3[256];
#ifdef PANDORA
static const char *text_str_title=    "----- UAE4All Pandora -----";
#else
static const char *text_str_title=    "----- UAE4All Android -----";
#endif
static const char *text_str_df0=		"DF0:";
static const char *text_str_df1=		"DF1:";
static const char *text_str_df2=		"DF2:";
static const char *text_str_df3=		"DF3:";
static const char* text_str_hdnmem="Harddisk and Memory Options (H)";
static const char *text_str_display="Display and Sound (L-trigger)";
static const char *text_str_savestates="Savestates (S)";
static const char *text_str_eject="Eject All Drives";
const char *text_str_separator="--------------------------------";
#ifdef PANDORA
static const char *text_str_reset="Reset (R-trigger)";
#else
static const char *text_str_reset="Reset (Start Emulator)";
#endif
static const char *text_str_exit= "Quit (Q)";

int mainMenu_case=-1;
int mainMenu_system=-1;

int nr_drives=DEFAULT_DRIVES;
int current_drive=0;

int lastCpuSpeed=600;
int ntsc=0;

extern SDL_Surface *prSDLScreen;

static void adjustToWindow(char *str, char* buffer)
{
	if (strlen(str)<33) return;
	char *p=str+strlen(str)-13;
	for (int i=0;i<15;i++) {buffer[i]=*str;str++;}
	char tt[]={'.','.','.','\0',};
	strcat(buffer,tt);
	strcat(buffer,p);
}

static void showInfo()
{
	text_draw_background();
	char buffer[128];
	char buffertext[128];

	text_draw_window(2,2,35,20,"Info");
	SDL_Rect r;
	r.x=80-64; r.y=0; r.w=35*7; r.h=140;
	extern SDL_Surface *text_screen;
	SDL_SetClipRect(text_screen,&r);
	write_text(4,2,"DF0");

	extractFileName(uae4all_image_file0,buffer);
	adjustToWindow(buffer,buffertext);
	write_text(10,2,buffertext);

	write_text(4,4,"DF1");
	if (!uae4all_image_file1[0]) write_text(10,4,"Empty");
	else 
	{
		extractFileName(uae4all_image_file1,buffer);
		adjustToWindow(buffer,buffertext);
		write_text(10,4,buffer);
	}

	write_text(4,6,"DF2");
	if (!uae4all_image_file2[0]) write_text(10,4,"Empty");
	else 
	{
		extractFileName(uae4all_image_file2,buffer);
		adjustToWindow(buffer,buffertext);
		write_text(10,6,buffer);
	}
	
	write_text(4,8,"DF3");
	if (!uae4all_image_file3[0]) write_text(10,4,"Empty");
	else 
	{
		extractFileName(uae4all_image_file3,buffer);
		adjustToWindow(buffer,buffertext);
		write_text(10,8,buffer);
	}

		text_flip();
		SDL_Event ev;
		SDL_Delay(333);
		while(SDL_PollEvent(&ev))
		SDL_Delay(10);
		while(!SDL_PollEvent(&ev))
				SDL_Delay(10);
		while(SDL_PollEvent(&ev))
				if (ev.type==SDL_QUIT)
					exit(1);
		SDL_Delay(200);
		SDL_SetClipRect(text_screen,NULL);
}

static void draw_mainMenu(int c)
{
	/* New Menu
	0 = DF0:
	1 = DF1:
	2 = DF2:
	3 = DF3:
	4 = eject all drives
	5 = number of drives
	6 = preset system setup
	7 = harddisk and memory options
	8 = display settings
	9 = savestates
	10 = custom controls
	11 = more options
	12 = reset
	13 = save config current game
	14 = save general config
	15 = quit
	*/
	static int b=0;
	int bb=(b%6)/3;
	int menuLine = 3;
	int leftMargin = 8;
	int tabstop1 = 17+4;
	int tabstop2 = 19+4;
	int tabstop3 = 21+4;
	int tabstop4 = 23+4;
	int tabstop5 = 25+4;
	int tabstop6 = 27+4;
	int tabstop7 = 29+4;
	int tabstop8 = 31+4;
	int tabstop9 = 33+4;

	text_draw_background();
	text_draw_window(leftMargin-1,menuLine-1,34,40,text_str_title);

	// 1
	if ((c==0)&&(bb))
		write_text_inv(leftMargin,menuLine,text_str_df0);
	else
		write_text(leftMargin,menuLine,text_str_df0);
	if(strcmp(uae4all_image_file0, "")==0)
		write_text_inv(13,menuLine,"insert disk image");
	else
		write_text_inv(13,menuLine,filename0);

	// 2
	menuLine+=2;
	if(nr_drives > 1)
	{
		if((c==1)&&(bb))
			write_text_inv(leftMargin,menuLine,text_str_df1);
		else
			write_text(leftMargin,menuLine,text_str_df1);
		if(strcmp(uae4all_image_file1, "")==0)
			write_text_inv(13,menuLine,"insert disk image");
		else
			write_text_inv(13,menuLine,filename1);
	}

	// 3
	menuLine+=2;
	if(nr_drives > 2)
	{
		if ((c==2)&&(bb))
			write_text_inv(leftMargin,menuLine,text_str_df2);
		else
			write_text(leftMargin,menuLine,text_str_df2);
		if(strcmp(uae4all_image_file2, "")==0)
			write_text_inv(13,menuLine,"insert disk image");
		else
			write_text_inv(13,menuLine,filename2);
	}

	// 4
	menuLine+=2;
	if(nr_drives > 3)
	{
		if ((c==3)&&(bb))
			write_text_inv(leftMargin,menuLine,text_str_df3);
		else
			write_text(leftMargin,menuLine,text_str_df3);
		if(strcmp(uae4all_image_file3, "")==0)
			write_text_inv(13,menuLine,"insert disk image");
		else
			write_text_inv(13,menuLine,filename3);
	}

	menuLine++;
	write_text(leftMargin,menuLine,text_str_separator);
	menuLine++;

	// 5
	if ((c==4)&&(bb))
		write_text_inv(leftMargin,menuLine,text_str_eject);
	else
		write_text(leftMargin, menuLine,text_str_eject);

	// 6
	menuLine+=2;
	write_text(leftMargin,menuLine,"Number of drives:");
	
	if ((nr_drives==1)&&((c!=5)||(bb)))
		write_text_inv(tabstop3,menuLine,"1");
	else
		write_text(tabstop3,menuLine,"1");

	if ((nr_drives==2)&&((c!=5)||(bb)))
		write_text_inv(tabstop4,menuLine,"2");
	else
		write_text(tabstop4,menuLine,"2");

	if ((nr_drives==3)&&((c!=5)||(bb)))
		write_text_inv(tabstop5,menuLine,"3");
	else
		write_text(tabstop5,menuLine,"3");

	if ((nr_drives==4)&&((c!=5)||(bb)))
		write_text_inv(tabstop6,menuLine,"4");
	else
		write_text(tabstop6,menuLine,"4");

	menuLine++;
	write_text(leftMargin,menuLine,text_str_separator);
	menuLine++;

	// 7
	write_text(leftMargin,menuLine,"Preset System Setup:");

	if ((mainMenu_system!=1)&&((c!=6)||(bb)))
		write_text_inv(tabstop5,menuLine,"A500");
	else
		write_text(tabstop5,menuLine,"A500");

	if ((mainMenu_system!=0)&&((c!=6)||(bb)))
		write_text_inv(tabstop8-1,menuLine,"A1200");
	else
		write_text(tabstop8-1,menuLine,"A1200");

	// 8
	menuLine+=2;
	if ((c==7)&&(bb))
		write_text_inv(leftMargin,menuLine,text_str_hdnmem);
	else
		write_text(leftMargin,menuLine,text_str_hdnmem);

	menuLine++;
	write_text(leftMargin,menuLine,text_str_separator);
	menuLine++;

	// 9
	if ((c==8)&&(bb))
		write_text_inv(leftMargin,menuLine,text_str_display);
	else
		write_text(leftMargin,menuLine,text_str_display);

	// 10
	menuLine+=2;
	if ((c==9)&&(bb))
		write_text_inv(leftMargin,menuLine,text_str_savestates);
	else
		write_text(leftMargin,menuLine,text_str_savestates);

	// 11
	menuLine+=2;
	if ((c==10)&&(bb))
		write_text_inv(leftMargin,menuLine,"Custom Control Config (Y)");
	else
		write_text(leftMargin,menuLine,"Custom Control Config (Y)");

	// 12
	menuLine+=2;
	if ((c==11)&&(bb))
		write_text_inv(leftMargin,menuLine,"More Options (B)");
	else
		write_text(leftMargin,menuLine,"More Options (B)");

	menuLine++;
	write_text(leftMargin,menuLine,text_str_separator);

	// 13
	menuLine++;
	if ((c==12)&&(bb))
		write_text_inv(leftMargin,menuLine,text_str_reset);
	else
		write_text(leftMargin,menuLine,text_str_reset);

	menuLine++;
	write_text(leftMargin,menuLine,text_str_separator);

	// 14
	menuLine++;
	if ((c==13)&&(bb))
		write_text_inv(leftMargin,menuLine,"Save Config for current game");
	else
		write_text(leftMargin,menuLine,"Save Config for current game");

	// 15
	menuLine+=2;
	if ((c==14)&&(bb))
		write_text_inv(leftMargin,menuLine,"Save General Config");
	else
		write_text(leftMargin,menuLine,"Save General Config");

	menuLine++;
	write_text(leftMargin,menuLine,text_str_separator);

	// 16
	menuLine++;
	if ((c==15)&&(bb))
		write_text_inv(leftMargin,menuLine,text_str_exit);
	else
		write_text(leftMargin,menuLine,text_str_exit);

	menuLine++;
	write_text(leftMargin,menuLine,text_str_separator);

	text_flip();
	
	b++;
}

void showWarning(const char *msg)
{
	text_draw_window(54/7,91/8,255/7,64/8,"--- Config ---");
	write_text(12,14,msg);
	write_text(11,16,"Press any button to continue");
	text_flip();
	SDL_Event ev;
	SDL_Delay(333);
	while(SDL_PollEvent(&ev))
	{
		if (ev.type==SDL_QUIT)
			exit(1);
		SDL_Delay(10);
	}
}

void setSystem()
{
	if(mainMenu_system > 0)
	{
		mainMenu_chipMemory=2;
		mainMenu_slowMemory=0;
		mainMenu_fastMemory=4;
		kickstart=3;
		mainMenu_CPU_model=1;
		mainMenu_chipset=2;
		mainMenu_CPU_speed=1;
	}
	else
	{
		mainMenu_chipMemory=1;
		mainMenu_slowMemory=0;
		mainMenu_fastMemory=0;
		kickstart=1;
		mainMenu_CPU_model=0;
		mainMenu_chipset=0;
		mainMenu_CPU_speed=0;
	}
	UpdateMemorySettings();
	UpdateCPUModelSettings();
	UpdateChipsetSettings();
}

static int key_mainMenu(int *cp)
{
#ifdef ANDROIDSDL
SDL_ANDROID_SetScreenKeyboardShown(1);
#endif
	int back_c=-1;
	int c=(*cp);
	int end=0;
	int left=0, right=0, up=0, down=0, hit0=0, hit1=0, hit2=0, hit3=0, hit4=0, hit5=0, hit6=0, hitH=0, hitS=0, hitQ=0, hitN1=0, hitN2=0, hitN3=0, hitN4=0;
	SDL_Event event;
	int info=0;

	while (SDL_PollEvent(&event) > 0)
	{
		if (event.type == SDL_QUIT)
		{
			mainMenu_case=MAIN_MENU_CASE_QUIT;
			end=-1;
		}
		else if (event.type == SDL_KEYDOWN)
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
				case SDLK_LCTRL: hit2=1; break;
				case SDLK_RSHIFT: hit3=1; break;
				case SDLK_RCTRL: hit4=1; break;
				case SDLK_END: hit5=1; break;
				case SDLK_PAGEUP: hit6=1; break;
				case SDLK_i: info=1; break;
				case SDLK_h: hitH=1; break;
				case SDLK_s: hitS=1; break;
				case SDLK_q: hitQ=1; break;
				case SDLK_1: hitN1=1; break;
				case SDLK_2: hitN2=1; break;
				case SDLK_3: hitN3=1; break;
				case SDLK_4: hitN4=1;
			}
		}

		if (info)
			showInfo();
		else if (hit1)
		{
			mainMenu_case=MAIN_MENU_CASE_RUN;
			end=1;
		}
		else if (hit2)
		{
			mainMenu_case=MAIN_MENU_CASE_CANCEL;
			end=1;
		}
		else if (hit3)
		{
			mainMenu_case=MAIN_MENU_CASE_DISPLAY;
			end=1;
		}
		else if (hit4)
		{
			// reset
			back_c = c;
			hit0 = 1;
			c = 12;
		}
		else if (hit5)
		{
			// more options
			back_c = c;
			hit0 = 1;
			c = 11;
		}
		else if (hit6)
		{
			// custom controls
			back_c = c;
			hit0 = 1;
			c = 10;
		}
		else if (hitH)
		{
			mainMenu_case=MAIN_MENU_CASE_MEMDISK;
			end=1;
		}
		else if (hitS)
		{
			mainMenu_case=MAIN_MENU_CASE_SAVESTATES;
			end=1;
		}
		else if (hitQ)
		{
			mainMenu_case=MAIN_MENU_CASE_QUIT;
			end=1;
		}
		else if(hitN1)
		{
			current_drive=0;
			mainMenu_case=MAIN_MENU_CASE_LOAD;
			end=1;
		}
		else if(hitN2)
		{
			current_drive=1;
			mainMenu_case=MAIN_MENU_CASE_LOAD;
			end=1;
		}
		else if(hitN3)
		{
			current_drive=2;
			mainMenu_case=MAIN_MENU_CASE_LOAD;
			end=1;
		}
		else if(hitN4)
		{
			current_drive=3;
			mainMenu_case=MAIN_MENU_CASE_LOAD;
			end=1;
		}
		else if (up)
		{
			if(nr_drives<2 && c==4)
				c=0;
			else if(nr_drives<3 && c==4)
				c=1;
			else if(nr_drives<4 && c==4)
				c=2;
			else
				c--;
			if (c < 0) c = 15;
		}
		else if (down)
		{
			if(nr_drives<4 && c==2)
				c=4;
			else if(nr_drives<3 && c==1)
				c=4;
			else if(nr_drives<2 && c==0)
				c=4;
			else
				c=(c+1)%16;
		}

	/* New Menu
	0 = DF0:
	1 = DF1:
	2 = DF2:
	3 = DF3:
	4 = eject all drives
	5 = number of drives
	6 = preset system setup
	7 = display settings
	8 = sound
	9 = savestates
	10 = custom controls
	11 = more options
	12 = reset
	13 = save config current game
	14 = save general config
	15 = exit
	*/
		switch(c)
		{
			case 0:
				if (hit0)
				{
					current_drive=0;
					mainMenu_case=MAIN_MENU_CASE_LOAD;
					end=1;
				}
				break;
			case 1:
				if (hit0)
				{
					current_drive=1;
					mainMenu_case=MAIN_MENU_CASE_LOAD;
					end=1;
				}
				break;
			case 2:
				if (hit0)
				{
					current_drive=2;
					mainMenu_case=MAIN_MENU_CASE_LOAD;
					end=1;
				}
				break;
			case 3:
				if (hit0)
				{
					current_drive=3;
					mainMenu_case=MAIN_MENU_CASE_LOAD;
					end=1;
				}
				break;
			case 4:
				if (hit0)
				{
					strcpy(uae4all_image_file0, "");
					strcpy(uae4all_image_file1, "");
					strcpy(uae4all_image_file2, "");
					strcpy(uae4all_image_file3, "");
				}
				break;
			case 5:
				if (left)
				{
					if (nr_drives>1)
						nr_drives--;
					else
						nr_drives=4;
				}
				else if (right)
				{
					if (nr_drives<4)
						nr_drives++;
					else
						nr_drives=1;
				}	
				break;
			case 6:
				if (left)
				{
					if (mainMenu_system==0)
						mainMenu_system=1;
					else
						mainMenu_system=0;
					setSystem();
				}
				else if (right)
				{
					if (mainMenu_system<1)
						mainMenu_system=1;
					else
						mainMenu_system=0;
					setSystem();
				}
				break;
			case 7:
				if (hit0)
				{
					mainMenu_case=MAIN_MENU_CASE_MEMDISK;
					end=1;
				}
				break;
			case 8:
				if (hit0)
				{
					mainMenu_case=MAIN_MENU_CASE_DISPLAY;
					end=1;
				}
				break;
			case 9:
				if (hit0)
				{
					mainMenu_case=MAIN_MENU_CASE_SAVESTATES;
					end=1;
				}
				break;
			case 10:
				if (hit0)
				{
					mainMenu_case=MAIN_MENU_CASE_CONTROLS;
					end=1;
				}
				break;
			case 11:
				if (hit0)
				{
					mainMenu_case=MAIN_MENU_CASE_MISC;
					printf("Launch main menu MISC\n");
					end=1;
				}
				break;
			case 12:
				if (hit0)
				{
					mainMenu_case=MAIN_MENU_CASE_RESET;
					end=1;
				}
				break;
			case 13:
				if (hit0)
				{
					mainMenu_case=MAIN_MENU_CASE_SAVE;
					if (saveconfig())
						showWarning("Config saved for this game");
				}
				break;
			case 14:
				if (hit0)
				{
					mainMenu_case=MAIN_MENU_CASE_SAVE;
					saveconfig(1);
					showWarning("General config file saved");
				}
				break;
			case 15:
				if (hit0)
				{
					mainMenu_case=MAIN_MENU_CASE_QUIT;
					end=1;
				}
				break;
		}
		if (back_c>=0)
		{
			c=back_c;
			back_c=-1;
		}			
	}

	(*cp)=c;
	return end;
}

static void raise_mainMenu()
{
	setenv("SDL_OMAP_LAYER_SIZE","640x480",1);
	setenv("SDL_OMAP_BORDER_CUT","0,0,0,30",1);
#ifdef PANDORA
	prSDLScreen = SDL_SetVideoMode(320, 270, 16, SDL_SWSURFACE|SDL_FULLSCREEN|SDL_DOUBLEBUF);
#else
	prSDLScreen = SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE|SDL_FULLSCREEN);
#endif

	int i;

	text_draw_background();
	text_flip();
	for(i=0;i<10;i++)
	{
		text_draw_background();
		text_draw_window(40,(10-i)*24,260,200,text_str_title);
		text_flip();
	}
}

static void unraise_mainMenu()
{
	int i;

	for(i=9;i>=0;i--)
	{
		text_draw_background();
		text_draw_window(40,(10-i)*24,260,200,text_str_title);
		text_flip();
	}
	text_draw_background();
	text_flip();
}

int run_mainMenu()
{
	static int c=0;
	int end;
	int old_sound_rate = sound_rate;
	int old_stereo = mainMenu_soundStereo;
	mainMenu_case=-1;
	init_text(0);

	while(mainMenu_case<0)
	{
		raise_mainMenu();
		end=0;
		draw_mainMenu(c);
		while(!end)
		{
			draw_mainMenu(c);
			end=key_mainMenu(&c);
		}
		unraise_mainMenu();
		switch(mainMenu_case)
		{
		case MAIN_MENU_CASE_LOAD:
			if(run_menuLoad(currentDir, MENU_LOAD_FLOPPY) && current_drive==0)
			{ 
				// Check for disk-specific config
				char path[300];
				create_configfilename(path, uae4all_image_file0, 0);
				FILE *f=fopen(path,"rt");
				if(f)
				{
					// config file exists -> load
					fclose(f);
					loadconfig();
				}
			}
			mainMenu_case=-1;
			break;
		case MAIN_MENU_CASE_MEMDISK:
			run_menuMemDisk();
			mainMenu_case=-1;
			break;
		case MAIN_MENU_CASE_SAVESTATES:
			run_menuSavestates();
			if(savestate_state == STATE_DORESTORE || savestate_state == STATE_DOSAVE)
			{
				setCpuSpeed();
				mainMenu_case=1;
			}
			else
				mainMenu_case=-1;
			break;
		case MAIN_MENU_CASE_EJECT:
			mainMenu_case=3;
			break;
		case MAIN_MENU_CASE_CANCEL:
			if (emulating)
			{
				setCpuSpeed();
				mainMenu_case=1;
			}
			else
				mainMenu_case=-1;
			break;
		case MAIN_MENU_CASE_RESET:
			setCpuSpeed();
			gp2xMouseEmuOn=0;
			gp2xButtonRemappingOn=0;
			mainMenu_drives=nr_drives;
			if (kickstart!=oldkickstart) 
			{
				oldkickstart=kickstart;
				snprintf(romfile, 256, "%s/kickstarts/%s",launchDir,kickstarts_rom_names[kickstart]);
				bReloadKickstart=1;
				uae4all_init_rom(romfile);
#ifdef ANDROIDSDL
				if (uae4all_init_rom(romfile)==-1)
				{
				    snprintf(romfile, 256, "%s/../../com.cloanto.amigaforever.essentials/files/rom/%s",launchDir,af_kickstarts_rom_names[kickstart]);			
				    uae4all_init_rom(romfile);
				} 				
#endif
			}
			reset_hdConf();
			if (emulating)
			{
				mainMenu_case=2;	
				break;
			}
		case MAIN_MENU_CASE_RUN:
			setCpuSpeed();
			mainMenu_case=1;
			break;
		case MAIN_MENU_CASE_CONTROLS:
			{
				run_menuControls();
				mainMenu_case=-1;
			}
			break;
		case MAIN_MENU_CASE_DISPLAY:
			{
				run_menuDisplay();
				mainMenu_case=-1;
			}
			break;
		case MAIN_MENU_CASE_MISC:
			{
				run_menuMisc();
				mainMenu_case=-1;
			}
			break;
		case MAIN_MENU_CASE_QUIT:
#ifndef USE_SDLSOUND
			gp2x_stop_sound();
#endif
			saveAdfDir();
      leave_program();
			sync();
			exit(0);
			break;
		default:
			mainMenu_case=-1;
		}
	}

	if (sound_rate != old_sound_rate || mainMenu_soundStereo != old_stereo)
		init_sound();

	return mainMenu_case;
}
