#include<stdio.h>
#include<SDL.h>

extern SDL_Surface *prSDLScreen;

#define MENU_FILE_SPLASH DATA_PREFIX "gp2xsplash.bmp"
#define MENU_FILE_BACKGROUND DATA_PREFIX "background.bmp"
#define MENU_FILE_WINDOW DATA_PREFIX "window.bmp"
#define MENU_FILE_TEXT DATA_PREFIX "text.bmp"
#define MENU_DIR_DEFAULT "."

void init_kickstart();
void showWarning(const char *msg);

void text_draw_background();
void init_text(int splash);
void quit_text(void);
void write_text(int x, int y, const char * str);
void write_text_inv(int x, int y, const char * str);
void write_centered_text(int y, char * str);
void write_num(int x, int y, int v);
void write_num_inv(int x, int y, int v);
void text_draw_window(int x, int y, int w, int h, const char *title);
void text_draw_barra(int x, int y, int w, int h, int per, int max);
void text_draw_window_bar(int x, int y, int w, int h, int per, int max, const char *title);
void _text_draw_window(SDL_Surface *sf, int x, int y, int w, int h, const char *title);
void _text_draw_window_bar(SDL_Surface *sf, int x, int y, int w, int h, int per, int max, const char *title);
void CreateScreenshot(int code);
int save_thumb(int code,char *path);
int save_png(SDL_Surface* surface,char *path);

int createScript(int bIcon=0);

void text_flip(void);
void set_joyConf(void);
void loadconfig(int general=0);
int saveconfig(int general=0);
int create_configfilename(char *dest, char *basename, int fromDir);

void drawPleaseWait(void);
void menu_raise(void);
void menu_unraise(void);

int run_mainMenu();
int run_menuLoad(char *, int aLoadtype);
int run_menuGame();
int run_menuSavestates();
int run_menuMisc();
int run_menuControls();
int run_menuDisplay();
int run_menuMemDisk();
void update_display();
#ifdef USE_GUICHAN
int run_mainMenuGuichan();
#endif

int run_menuControl();
#ifdef USE_GUICHAN
extern    bool running;
#endif
enum { SCREENSHOT, ICON };

enum { MAIN_MENU_CASE_QUIT, MAIN_MENU_CASE_LOAD, MAIN_MENU_CASE_RUN, MAIN_MENU_CASE_RESET, MAIN_MENU_CASE_CANCEL, MAIN_MENU_CASE_SAVESTATES, MAIN_MENU_CASE_EJECT, MAIN_MENU_CASE_MISC, MAIN_MENU_CASE_SAVE, MAIN_MENU_CASE_CONTROLS, MAIN_MENU_CASE_DISPLAY, MAIN_MENU_CASE_MEMDISK};
enum { MEMDISK_MENU_CASE_MAIN, MEMDISK_MENU_CASE_MISC };

/* Just 0x0 and not 680x0, so that constants can fit in ARM instructions */
#define M68000 000
#define M68020 020

#define DEFAULT_STATUSLN 0
#define DEFAULT_MOUSEMULTIPLIER 2
#define DEFAULT_SOUND 1
#define DEFAULT_AUTOSAVE 1
#define DEFAULT_SYSTEMCLOCK 0
#define DEFAULT_SYNCTHRESHOLD 2
#define DEFAULT_SKIPINTRO 1
#define DEFAULT_CHIPSET_SELECT 0
#define DEFAULT_NTSC 0
#define DEFAULT_JOYCONF 2
#define DEFAULT_CHIPMEM_SELECT 1
#define DEFAULT_USE1MBCHIP 1
#define DEFAULT_ENABLE_HD 0
#define DEFAULT_AUTOFIRE 1
#define DEFAULT_DRIVES 4
#define DEFAULT_ENABLESCRIPTS 0
#define DEFAULT_ENABLESCREENSHOTS 0
#define DEFAULT_SCALING 0
#define DEFAULT_KICKSTART 1
#define DEFAULT_CPU_MODEL M68000

#define MENU_MEMDISK_WINDOW_WIDTH 40

/* Event intervals */
#define SELECTION_DRAWING_INTERVAL	250	/* [ms] inverted/normal text */
#define EVENT_POLLING_INTERVAL		50	/* [ms] SDL events polling */

/* What is being loaded */
#define MENU_LOAD_FLOPPY 1
#define MENU_LOAD_HD_DIR 2
#define MENU_LOAD_HDF 3
#define MENU_LOAD_CONFIG 4
