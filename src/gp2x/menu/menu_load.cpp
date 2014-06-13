#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "menu.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "uae.h"
#include "options.h"
#include "sound.h"
#include "gui.h"
#include "gp2x.h"
#include "gp2xutil.h"
#include <limits.h>
#include <SDL.h>

#ifndef PATH_MAX
	#define PATH_MAX 256
#endif

char romFileName[PATH_MAX];

/* What is being loaded, floppy/hd dir/hdf */
int menu_load_type;

extern char filename0[256];
extern char filename1[256];
extern char filename2[256];
extern char filename3[256];
extern char currentDir[300];

const char *text_str_load_separator="----------------------------------------";
const char *text_str_load_dir="#DIR#";
static const char *text_str_load_title="            Filemanager            -";
int text_dir_num_files=0, text_dir_num_files_index=0;

#define SHOW_MAX_FILES 13

extern int run_menuFileinfo(char* fileName);
static int min_in_dir=0, max_in_dir=SHOW_MAX_FILES;

extern int current_drive;
#ifdef PANDORA
static int scandir_cmp(const void *p1, const void *p2)
{
	struct dirent **d1 = (struct dirent **)p1, **d2 = (struct dirent **)p2;
#else
static int scandir_cmp(const dirent **p1, const dirent **p2)
{
        const struct dirent **d1 = (const struct dirent **)p1, **d2 = (const struct dirent **)p2;
#endif
	if ((*d1)->d_type == (*d2)->d_type) return alphasort(d1, d2);
	if ((*d1)->d_type == DT_DIR) return -1;
	if ((*d2)->d_type == DT_DIR) return  1;
	return alphasort(d1, d2);
}

static const char *filter_exts[] = {
	".adf", ".gz",".rom",".adf.gz"
};

static int scandir_filter(const struct dirent *ent)
{
	const char *p;
	int i;
	return 1;

	if (ent == NULL || ent->d_name == NULL)
		return 0;
	if (strlen(ent->d_name) < 5)
		return 1;

	p = ent->d_name + strlen(ent->d_name) - 4;

	for (i = 0; i < sizeof(filter_exts)/sizeof(filter_exts[0]); i++)
	{
		if (strcmp(p, filter_exts[i]) == 0)
			return 0;
	}

	return 1;
}

static void extractFileName(char * str,char *buffer)
{
	char *p=str+strlen(str)-1;
	while(*p != '/') p--;
	p++;
	strcpy(buffer,p);
}

static void draw_dirlist(char *curdir, struct dirent **namelist, int n, int sel)
{
	int i,j;
	n--;
	static int b=0;
	int bb=(b%6)/3;
	SDL_Rect r;
	extern SDL_Surface *text_screen;
	r.x=80-64; r.y=0; r.w=150-24+64+64; r.h=240;
	text_draw_background();
	if (menu_load_type == MENU_LOAD_HD_DIR)
		text_draw_window(2,2,41,25,"  Press L-key to load HD-dir  ");
	else if (menu_load_type == MENU_LOAD_HDF)
		text_draw_window(2,2,41,25,"       Select .HDF-file       ");
	else if (current_drive==0)
		text_draw_window(2,2,41,25," Insert .ADF or .ADZ into DF0 ");
	else if (current_drive==1)
		text_draw_window(2,2,41,25," Insert .ADF or .ADZ into DF1 ");
	else if (current_drive==2)
		text_draw_window(2,2,41,25," Insert .ADF or .ADZ into DF2 ");
	else if (current_drive==3)
		text_draw_window(2,2,41,25," Insert .ADF or .ADZ into DF3 ");
	else
		text_draw_window(2,2,41,25,text_str_load_title);

	if (sel<min_in_dir)
	{
		min_in_dir=sel;
		max_in_dir=sel+SHOW_MAX_FILES;
	}
	else
		if (sel>=max_in_dir)
		{
			max_in_dir=sel+1;
			min_in_dir=max_in_dir-SHOW_MAX_FILES;
		}
	if (max_in_dir>n)
		max_in_dir=n-min_in_dir;

	for (i=min_in_dir,j=3;i<max_in_dir;i++,j+=2)
	{
		
		write_text(3,j,text_str_load_separator);
		SDL_SetClipRect(text_screen,&r);
		
	if ((sel+1==i+1)&&(bb))
			write_text_inv(4,j+1,namelist[i+1]->d_name);
		else
			write_text(4,j+1,namelist[i+1]->d_name);


		SDL_SetClipRect(text_screen,NULL);

		if (namelist[i+1]->d_type==DT_DIR)
			write_text(38,j+1,text_str_load_dir);
	}
	write_text(3,j,text_str_load_separator);

	text_flip();

	b++;
}

static int menuLoadLoop(char *curr_path)
{
	char *ret = NULL, *fname = NULL;
	struct dirent **namelist;
	DIR *dir;
	int n, sel = 0;
	int sel_last = 0;
	unsigned long inp = 0;

	min_in_dir=0;
	max_in_dir=SHOW_MAX_FILES;

	// is this a dir or a full path?
	if ((dir = opendir(curr_path)))
		closedir(dir);
	else 
	{
		char *p;
		for (p = curr_path + strlen(curr_path) - 1; p > curr_path && *p != '/'; p--);
		*p = 0;
		fname = p+1;
	}
	
	n = scandir(curr_path, &namelist, scandir_filter, scandir_cmp);

	if (n < 0) 
	{
		// try root
		n = scandir("/", &namelist, scandir_filter, scandir_cmp);
		if (n < 0) 
		{
			// oops, we failed
			printf("dir: "); printf(curr_path); printf("\n");
			perror("scandir");
			return 0;
		}
	}
	if (n<10) usleep(70*1024);
	else usleep(40*1024);
	// try to find sel
	if (fname != NULL) 
	{
		int i;
		for (i = 1; i < n; i++) 
		{
			if (strcmp(namelist[i]->d_name, fname) == 0) 
			{
				sel = i - 1;
				break;
			}
		}
	}

	int loaded=0;
	int delay=0;
	SDL_Event event;
	int left=0, right=0, up=0, down=0, hit0=0, hit1=0, hit2=0, hit3=0, hit4=0, hitL=0;
	while(hit0+hit1+hitL==0)
	{
		//unsigned long keys;
		draw_dirlist(curr_path, namelist, n, sel);
		delay ++;
		left=right=up=down=hit0=hit1=hit2=hit3=hit4=hitL=0;
		while (SDL_PollEvent(&event) > 0 && hit0+hit1+hitL==0)
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
					case SDLK_LCTRL: hit2=1; break;
					case SDLK_RSHIFT: hit3=1; break;
					case SDLK_RCTRL: hit4=1; break;
					case SDLK_END: hit0=1; break;
					case SDLK_PAGEUP: hit0=1; break;
					case SDLK_l: hitL=1;
				}
			}
			if(up)  { sel--;   if (sel < 0)   sel = n-2; /*usleep(10*1024);*/ }
			if(down)  { sel++;   if (sel > n-2) sel = 0;/*usleep(10*1024);*/}
			if(left)  { sel-=10; if (sel < 0)   sel = 0;/*usleep(10*1024);*/}
			if(hit3)     { sel-=24; if (sel < 0)   sel = 0;/*usleep(10*1024);*/}
			if(right) { sel+=10; if (sel > n-2) sel = n-2;/*usleep(10*1024);*/}
			if(hit4)     { sel+=24; if (sel > n-2) sel = n-2;/*usleep(10*1024);*/}
			if(hit2)     { run_menuFileinfo(namelist[sel+1]->d_name);}
			if(hit0 || hitL)
			{
				if (namelist[sel+1]->d_type == DT_REG) 
				{
					int df;
					int newlen = strlen(curr_path) + strlen(namelist[sel+1]->d_name) + 2;
					char *p; 
					char *filename;
					filename=(char*)malloc(newlen);
					strcpy(filename, curr_path);
					p = filename + strlen(filename) - 1;
					while (*p == '/' && p >= filename) *p-- = 0;
					strcat(filename, "/");
					strcat(filename, namelist[sel+1]->d_name);
					printf("Selecting file %s\n",filename);
					switch (menu_load_type)
					{
						case MENU_LOAD_FLOPPY:
							if (current_drive==0){strcpy(uae4all_image_file0,filename);extractFileName(uae4all_image_file0, filename0);df=0;}
							else if(current_drive==1) {strcpy(uae4all_image_file1,filename);extractFileName(uae4all_image_file1, filename1);df=1;}
							else if(current_drive==2) {strcpy(uae4all_image_file2,filename);extractFileName(uae4all_image_file2, filename2);df=2;}
							else if(current_drive==3) {strcpy(uae4all_image_file3,filename);extractFileName(uae4all_image_file3, filename3);df=3;}
							printf("DF0 %s\n",uae4all_image_file0);
							break;
						case MENU_LOAD_HDF:
							if (strstr(filename, ".hdf") == NULL)
								showWarning("HDF file must be selected");
							else
								strcpy(uae4all_hard_file, filename);
							break;
					}
					loaded=1;
					strcpy(currentDir,filename);
					free(filename);
					break;
				}
				else if (namelist[sel+1]->d_type == DT_DIR)
				{
					int newlen = strlen(curr_path) + strlen(namelist[sel+1]->d_name) + 2;
					char *p;
					char *newdir;
					/* Hard file dir is being selected ? (L-key is used to select dir) */
					if ((menu_load_type == MENU_LOAD_HD_DIR) && hitL)
					{
						strcpy(uae4all_hard_dir, curr_path);
						strcat(uae4all_hard_dir, "/");
						strcat(uae4all_hard_dir, namelist[sel+1]->d_name);
						loaded = 1;
						break;
					}
					else
					{
						newdir=(char*)malloc(newlen);
						if (strcmp(namelist[sel+1]->d_name, "..") == 0) 
						{
							char *start = curr_path;
							p = start + strlen(start) - 1;
							while (*p == '/' && p > start) p--;
							while (*p != '/' && p > start) p--;
							if (p <= start) strcpy(newdir, "/");
							else { strncpy(newdir, start, p-start); newdir[p-start] = 0; }
						} 
						else 
						{
							strcpy(newdir, curr_path);
							p = newdir + strlen(newdir) - 1;
							while (*p == '/' && p >= newdir) *p-- = 0;
							strcat(newdir, "/");
							strcat(newdir, namelist[sel+1]->d_name);
						}
						strcpy(currentDir,newdir);
						loaded = menuLoadLoop(newdir);
						free(newdir);
						break;
					}
				} 
			}
			if(hit1)
				break;
		}
	}
	if (n > 0) 
	{
		while(n--) free(namelist[n]);
		free(namelist);
	}

	return loaded;
}

static void raise_loadMenu()
{
	int i;

	text_draw_background();
	text_flip();
	for(i=0;i<10;i++)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_load_title);
		text_flip();
	}
}

static void unraise_loadMenu()
{
	int i;

	for(i=9;i>=0;i--)
	{
		text_draw_background();
		text_draw_window(80-64,(10-i)*24,160+64+64,220,text_str_load_title);
		text_flip();
	}
	text_draw_background();
	text_flip();
}

int run_menuLoad(char *curr_path, int aLoadType)
{
	menu_load_type = aLoadType;

	raise_loadMenu();
	int ret=menuLoadLoop(curr_path);
	unraise_loadMenu();
	return ret;
}
