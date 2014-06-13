#ifndef GP2X_MAME_H
#define GP2X_MAME_H

#define DEFAULT_SAMPLE_RATE 22050

#include "stdarg.h"
#include "string.h"
#include "minimal.h"

#if defined(__cplusplus) && !defined(USE_CPLUS)
extern "C" {
#endif

typedef void (*update_display_func)(void);

void gp2x_video_init(void);
void gp2x_gamelist_text_out(int x, int y, char *texto);
void gp2x_gamelist_text_out_fmt(int x, int y, char* fmt, ...);
void gp2x_text_out(int x, int y, char *texto);
void gp2x_text_out_int(int x, int y, int entero);
void gp2x_text_log(char *texto);
void gp2x_text_log_int(int entero);
void gp2x_text_log_fmt(char* fmt, ...);
void gp2x_clear_screen(void);
void gp2x_mame_palette(void);
void gp2x_adjust_display(void);
void gp2x_gamelist_zero(void);
void gp2x_text_pause(void);

extern int gp2x_rotate;		/* Screen Rotation */

struct KeySettings {
	int JOY_FIRE1;
	int JOY_FIRE2;
	int JOY_FIRE3;
	int JOY_FIRE4;
	int JOY_FIRE5;
	int JOY_FIRE6;
	int JOY_FIRE7;
	int JOY_FIRE8;
	int JOY_FIRE9;
	int JOY_FIRE10;
	int JOY_FIRE1_AUTO;
	int JOY_FIRE2_AUTO;
	int JOY_FIRE3_AUTO;
};

#if defined(__cplusplus) && !defined(USE_CPLUS)
}
#endif
#endif	/* defined GP2X_MAME_H */
