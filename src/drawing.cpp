 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Screen drawing functions
  *
  * Copyright 1995-2000 Bernd Schmidt
  * Copyright 1995 Alessandro Bissacco
  * Copyright 2000,2001 Toni Wilen
  */

#define UNROLL_PFIELD
#define UNROLL_DRAW_SPRITES

/* There are a couple of concepts of "coordinates" in this file.
   - DIW coordinates
   - DDF coordinates (essentially cycles, resolution lower than lores by a factor of 2)
   - Pixel coordinates
     * in the Amiga's resolution as determined by BPLCON0 ("Amiga coordinates")
     * in the window resolution as determined by the preferences ("window coordinates").
     * in the window resolution, and with the origin being the topmost left corner of
       the window ("native coordinates")
   One note about window coordinates.  The visible area depends on the width of the
   window, and the centering code.  The first visible horizontal window coordinate is
   often _not_ 0, but the value of VISIBLE_LEFT_BORDER instead.

   One important thing to remember: DIW coordinates are in the lowest possible
   resolution.

   To prevent extremely bad things (think pixels cut in half by window borders) from
   happening, all ports should restrict window widths to be multiples of 16 pixels.  */

#include "sysconfig.h"
#include "sysdeps.h"
#include <ctype.h>
#include <assert.h>
#include "config.h"
#include "uae.h"
#include "options.h"
#include "thread.h"
#include "memory-uae.h"
#include "custom.h"
#include "m68k/m68k_intrf.h"
#include "xwin.h"
#include "autoconf.h"
#include "gui.h"
#include "drawing.h"
#include "savestate.h"
#include "sound.h"
#include "debug_uae4all.h"
#include <sys/time.h>
#include <time.h>
#include "menu_config.h"
#include "menu.h"

static int fps_counter = 0, fps_counter_changed = 0;

#ifdef USE_DRAWING_EXTRA_INLINE
#define _INLINE_ __inline__
#else
#define _INLINE_ 
#endif

#define GFXVIDINFO_PIXBYTES 2
//#define GFXVIDINFO_WIDTH 320
#define GFXVIDINFO_HEIGHT 270
#define MAXBLOCKLINES 270
//#define VISIBLE_LEFT_BORDER 72
//#define VISIBLE_RIGHT_BORDER 392
//#define LINETOSCR_X_ADJUST_BYTES 144

static int var_GFXVIDINFO_WIDTH = 320;
static int var_VISIBLE_LEFT_BORDER = 73;
static int var_VISIBLE_RIGHT_BORDER = 393;
static int var_LINETOSCR_X_ADJUST_BYTES = 144;

#ifdef USE_GUICHAN
static char screenshot_filename_default[255]={
	'/', 't', 'm', 'p', '/', 'n', 'u', 'l', 'l', '.', 'p', 'n', 'g', '\0'
};
char *screenshot_filename=(char *)&screenshot_filename_default[0];
FILE *screenshot_file=NULL;
#endif

// newWidth is always in LORES
void InitDisplayArea(int newWidth)
{
	int deltaToBorder = newWidth - 320;
	
	var_GFXVIDINFO_WIDTH = newWidth;
	var_VISIBLE_LEFT_BORDER = 73 - (deltaToBorder >> 1);
	var_VISIBLE_RIGHT_BORDER = 393 + (deltaToBorder >> 1);
	var_LINETOSCR_X_ADJUST_BYTES = GFXVIDINFO_PIXBYTES * var_VISIBLE_LEFT_BORDER;
}


/* The shift factor to apply when converting between Amiga coordinates and window
   coordinates.  Zero if the resolution is the same, positive if window coordinates
   have a higher resolution (i.e. we're stretching the image), negative if window
   coordinates have a lower resolution (i.e. we're shrinking the image).  */
static int res_shift;

static int interlace_seen = 0;
extern int drawfinished;

extern SDL_Surface *prSDLScreen;

/* Lookup tables for dual playfields.  The dblpf_*1 versions are for the case
   that playfield 1 has the priority, dbplpf_*2 are used if playfield 2 has
   priority.  If we need an array for non-dual playfield mode, it has no number.  */
/* The dbplpf_ms? arrays contain a shift value.  plf_spritemask is initialized
   to contain two 16 bit words, with the appropriate mask if pf1 is in the
   foreground being at bit offset 0, the one used if pf2 is in front being at
   offset 16.  */
static int dblpf_ms1[256], dblpf_ms2[256], dblpf_ms[256];
static int dblpf_ind1[256], dblpf_ind2[256];
static int dblpf_2nd1[256], dblpf_2nd2[256];
static int dblpf_ind1_aga[256], dblpf_ind2_aga[256];
static int dblpfofs[] = { 0, 2, 4, 8, 16, 32, 64, 128 };
static int sprite_offs[256];
static uae_u32 clxtab[256];

/* Video buffer description structure. Filled in by the graphics system
 * dependent code. */
/* OCS/ECS color lookup table. */
xcolnr xcolors[4096];
/* AGA mode color lookup tables */
unsigned int xredcolors[256], xgreencolors[256], xbluecolors[256];

struct color_entry colors_for_drawing;

/* The size of these arrays is pretty arbitrary; it was chosen to be "more
   than enough".  The coordinates used for indexing into these arrays are
   almost, but not quite, Amiga coordinates (there's a constant offset).  */
union {
    /* Let's try to align this thing. */
    double uupzuq;
    long int cruxmedo;
    uae_u8 apixels[MAX_PIXELS_PER_LINE * 2];
    uae_u16 apixels_w[MAX_PIXELS_PER_LINE * 2 / 2];
    uae_u32 apixels_l[MAX_PIXELS_PER_LINE * 2 / 4];
} pixdata UAE4ALL_ALIGN;

uae_u16 spixels[MAX_SPR_PIXELS];
/* Eight bits for every pixel.  */
union sps_union spixstate;

static uae_u32 ham_linebuf[MAX_PIXELS_PER_LINE * 2];
static uae_u8 spriteagadpfpixels[MAX_PIXELS_PER_LINE * 2]; /* AGA dualplayfield sprite */

char *xlinebuffer;

static int *amiga2aspect_line_map, *native2amiga_line_map;
static char *row_map[gfxHeight + 1] UAE4ALL_ALIGN;
static int max_drawn_amiga_line;

/* line_draw_funcs: pfield_do_linetoscr, pfield_do_fill_line */
typedef void (*line_draw_func)(int, int);

#define LINE_UNDECIDED 1
#define LINE_DECIDED 2
#define LINE_DONE 7

static int linestate_first_undecided = 0;

uae_u8 line_data[(MAXVPOS + 1) * 2][MAX_PLANES * MAX_WORDS_PER_LINE * 2] UAE4ALL_ALIGN;

/* Centering variables.  */
static int min_diwstart, max_diwstop;
static int thisframe_y_adjust;
static int thisframe_y_adjust_real, max_ypos_thisframe, min_ypos_for_screen;
static int extra_y_adjust;
int moveX = 0, moveY = 0;

/* A frame counter that forces a redraw after at least one skipped frame in
   interlace mode.  */
static int last_redraw_point;

/* These are generated by the drawing code from the line_decisions array for
   each line that needs to be drawn.  These are basically extracted out of
   bit fields in the hardware registers.  */
static int bplehb, bplham, bpldualpf, bpldualpfpri, bpldualpf2of, bplplanecnt, bplres;
static int plf1pri, plf2pri, bplxor;
static uae_u32 plf_sprite_mask;
static int sbasecol[2];

int framecnt = 0, fs_framecnt = 0;

extern Uint32 uae4all_numframes;

uint64_t ticksPerFrame = 0;
static uint64_t lastTick = -1;
static int skipped_frames = 0;


void reset_frameskip(void)
{
	// will reset ticks automatically
	ticksPerFrame = 0;
	skipped_frames = 0;
}



static void fps_counter_upd(void)
{
	struct timeval tv;
	static int thissec, fcount;

	gettimeofday(&tv, 0);
	if (tv.tv_sec != thissec)
	{
		thissec = tv.tv_sec;
		fps_counter = fcount;
		fcount = 0;
		fps_counter_changed = 1;
	}
	else
	{
		fps_counter_changed = 0;
	}
	fcount++;
}


#define getTime10MicroSec() (uint64_t( tv.tv_sec ) * 100000 + tv.tv_usec / 10)
static __inline__ void count_frame (void)
{
  uint64_t tick;
	struct timeval tv;

	gettimeofday(&tv, 0);
	if(ticksPerFrame == 0)
	{
		ticksPerFrame = 100000 / ((beamcon0 & 0x20) ? 50 : 60);
		lastTick = getTime10MicroSec();
		skipped_frames = 0;
	}

  uae4all_numframes++;

	switch(prefs_gfx_framerate)
	{
		case 0: // draw every frame (Limiting is done by waiting for vsync...)
#if defined(ANDROIDSDL) || defined(AROS)
			if (!mainMenu_vsync)
#endif
			  fs_framecnt = 0;
#if defined(ANDROIDSDL) || defined(AROS)
			else
			{
			  // Limiter
			  while(getTime10MicroSec() < lastTick + ticksPerFrame)
			  {
				  usleep(100);
				  gettimeofday(&tv, 0);
			  }
			  lastTick += ticksPerFrame;
			}
#endif
			break;
			
		case 1: // draw every second frame
			fs_framecnt++;
			if (fs_framecnt > 1)
				fs_framecnt = 0;
			// Limiter
			while(getTime10MicroSec() < lastTick + ticksPerFrame)
			{
				usleep(100);
				gettimeofday(&tv, 0);
			}
			lastTick += ticksPerFrame;
			break;
#ifdef ANDROIDSDL
		case 2: // draw every third frame
			fs_framecnt++;
			if (fs_framecnt > 2)
				fs_framecnt = 0;
			// Limiter
			while(getTime10MicroSec() < lastTick + ticksPerFrame - 500)
			{
				usleep(100);
				gettimeofday(&tv, 0);
			}
			lastTick += ticksPerFrame;
			break;
		case 3: // draw every fourth frame
			fs_framecnt++;
			if (fs_framecnt > 3)
				fs_framecnt = 0;
			// Limiter
			while(getTime10MicroSec() < lastTick + ticksPerFrame - 500)
			{
				usleep(100);
				gettimeofday(&tv, 0);
			}
			lastTick += ticksPerFrame;
			break;
		case 4: // draw every fifth frame
			fs_framecnt++;
			if (fs_framecnt > 4)
				fs_framecnt = 0;
			// Limiter
			while(getTime10MicroSec() < lastTick + ticksPerFrame - 500)
			{
				usleep(100);
				gettimeofday(&tv, 0);
			}
			lastTick += ticksPerFrame;
			break;
		case 5: // draw every sixth frame
			fs_framecnt++;
			if (fs_framecnt > 5)
				fs_framecnt = 0;
			// Limiter
			while(getTime10MicroSec() < lastTick + ticksPerFrame - 500)
			{
				usleep(100);
				gettimeofday(&tv, 0);
			}
			lastTick += ticksPerFrame;
			break;
		case 6: // draw every seventh frame
			fs_framecnt++;
			if (fs_framecnt > 6)
				fs_framecnt = 0;
			// Limiter
			while(getTime10MicroSec() < lastTick + ticksPerFrame - 500)
			{
				usleep(100);
				gettimeofday(&tv, 0);
			}
			lastTick += ticksPerFrame;
			break;
		case 7: // draw every eighth frame
			fs_framecnt++;
			if (fs_framecnt > 7)
				fs_framecnt = 0;
			// Limiter
			while(getTime10MicroSec() < lastTick + ticksPerFrame - 500)
			{
				usleep(100);
				gettimeofday(&tv, 0);
			}
			lastTick += ticksPerFrame;
			break;
		case 8: // draw every ninth frame
			fs_framecnt++;
			if (fs_framecnt > 8)
				fs_framecnt = 0;
			// Limiter
			while(getTime10MicroSec() < lastTick + ticksPerFrame - 500)
			{
				usleep(100);
				gettimeofday(&tv, 0);
			}
			lastTick += ticksPerFrame;
			break;
#endif
	}
	
}

int coord_native_to_amiga_x (int x)
{
  if(mainMenu_displayHires)
  {
    x += (var_VISIBLE_LEFT_BORDER << 1);
    return x + 2*DISPLAY_LEFT_SHIFT - 2*DIW_DDF_OFFSET;
  }
  else
  {
    x += var_VISIBLE_LEFT_BORDER;
    x <<= 1;
    return x + 2*DISPLAY_LEFT_SHIFT - 2*DIW_DDF_OFFSET;
  }
}

int coord_native_to_amiga_y (int y)
{
    return native2amiga_line_map[y] + thisframe_y_adjust - minfirstline;
}

static __inline__ int res_shift_from_window (int x)
{
    if (res_shift >= 0)
		return x >> res_shift;
    return x << -res_shift;
}

static __inline__ int res_shift_from_amiga (int x)
{
    if (res_shift >= 0)
		return x << res_shift;
    return x >> -res_shift;
}

void notice_screen_contents_lost (void)
{
}

static struct decision *dp_for_drawing;
static struct draw_info *dip_for_drawing;

/* Record DIW of the current line for use by centering code.  */
void record_diw_line (int first, int last)
{
    if (last > max_diwstop)
		max_diwstop = last;
    if (first < min_diwstart)
		min_diwstart = first;
}

/*
 * Screen update macros/functions
 */

/* The important positions in the line: where do we start drawing the left border,
   where do we start drawing the playfield, where do we start drawing the right border.
   All of these are forced into the visible window (VISIBLE_LEFT_BORDER .. VISIBLE_RIGHT_BORDER).
   PLAYFIELD_START and PLAYFIELD_END are in window coordinates.  */
static int playfield_start, playfield_end;
static int real_playfield_start, real_playfield_end;
static int pixels_offset;
static int src_pixel, ham_src_pixel;
/* How many pixels in window coordinates which are to the left of the left border.  */
static int unpainted;
static int seen_sprites;

#define TYPE uae_u16

#define LNAME linetoscr_16
#define SRC_INC 1
#define HDOUBLE 0
#define AGA 0
#include "linetoscr.h"

#define LNAME linetoscr_16_stretch1
#define SRC_INC 1
#define HDOUBLE 1
#define AGA 0
#include "linetoscr.h"

#define LNAME linetoscr_16_shrink1
#define SRC_INC 2
#define HDOUBLE 0
#define AGA 0
#include "linetoscr.h"

#define LNAME linetoscr_16_aga
#define SRC_INC 1
#define HDOUBLE 0
#define AGA 1
#include "linetoscr.h"

#define LNAME linetoscr_16_stretch1_aga
#define SRC_INC 1
#define HDOUBLE 1
#define AGA 1
#include "linetoscr.h"

#define LNAME linetoscr_16_shrink1_aga
#define SRC_INC 2
#define HDOUBLE 0
#define AGA 1
#include "linetoscr.h"

static void pfield_do_linetoscr_0_640 (int start, int stop)
{
	int local_res_shift = 1 - bplres; // stretch LORES, nothing for HIRES, shrink for SUPERHIRES
	start = start << 1;
	stop = stop << 1;

	if (currprefs.chipset_mask & CSMASK_AGA)
	{
		if (local_res_shift == 0)
			src_pixel = linetoscr_16_aga (src_pixel, start, stop);
		else if (local_res_shift == 1)
			src_pixel = linetoscr_16_stretch1_aga (src_pixel, start, stop);
		else //if (local_res_shift == -1)
			src_pixel = linetoscr_16_shrink1_aga (src_pixel, start, stop);
	}
	else
	{
		if (local_res_shift == 0)
			src_pixel = linetoscr_16 (src_pixel, start, stop);
		else if (local_res_shift == 1)
			src_pixel = linetoscr_16_stretch1 (src_pixel, start, stop);
		else //if (local_res_shift == -1)
			src_pixel = linetoscr_16_shrink1 (src_pixel, start, stop);
	}
}

static void pfield_do_linetoscr_0 (int start, int stop)
{
	if (currprefs.chipset_mask & CSMASK_AGA)
	{
		if (res_shift == 0)
			src_pixel = linetoscr_16_aga (src_pixel, start, stop);
		else if (res_shift == 1)
			src_pixel = linetoscr_16_stretch1_aga (src_pixel, start, stop);
		else //if (res_shift == -1)
			src_pixel = linetoscr_16_shrink1_aga (src_pixel, start, stop);
	}
	else
	{
		if (res_shift == 0)
			src_pixel = linetoscr_16 (src_pixel, start, stop);
		else if (res_shift == 1)
			src_pixel = linetoscr_16_stretch1 (src_pixel, start, stop);
		else //if (res_shift == -1)
			src_pixel = linetoscr_16_shrink1 (src_pixel, start, stop);
	}
}


static void pfield_do_fill_line_0_640(int start, int stop)
{
	register uae_u16 *b = &(((uae_u16 *)xlinebuffer)[start << 1]);
	register xcolnr col = colors_for_drawing.acolors[0];
	register int i;
	register int max=(stop-start) << 1;
	for (i = 0; i < max; i++,b++)
		*b = col;
}

static void pfield_do_fill_line_0(int start, int stop)
{
	register uae_u16 *b = &(((uae_u16 *)xlinebuffer)[start]);
	register xcolnr col = colors_for_drawing.acolors[0];
	register int i;
	register int max=(stop-start);
	for (i = 0; i < max; i++,b++)
		*b = col;
}

static line_draw_func *pfield_do_linetoscr=(line_draw_func *)pfield_do_linetoscr_0;
static line_draw_func *pfield_do_fill_line=(line_draw_func *)pfield_do_fill_line_0;

/* Initialize the variables necessary for drawing a line.
 * This involves setting up start/stop positions and display window
 * borders.  */
static _INLINE_ void pfield_init_linetoscr (void)
{
	/* First, get data fetch start/stop in DIW coordinates.  */
	int ddf_left = (dp_for_drawing->plfleft << 1) + DIW_DDF_OFFSET;
	int ddf_right = (dp_for_drawing->plfright << 1) + DIW_DDF_OFFSET;

	/* Compute datafetch start/stop in pixels; native display coordinates.  */
	int native_ddf_left = coord_hw_to_window_x (ddf_left);
	int native_ddf_right = coord_hw_to_window_x (ddf_right);

	int linetoscr_diw_start = dp_for_drawing->diwfirstword;
	int linetoscr_diw_end = dp_for_drawing->diwlastword;

	if (dip_for_drawing->nr_sprites == 0) {
		if (linetoscr_diw_start < native_ddf_left)
			linetoscr_diw_start = native_ddf_left;
		if (linetoscr_diw_end > native_ddf_right)
			linetoscr_diw_end = native_ddf_right;
	}

	/* Perverse cases happen. */
	if (linetoscr_diw_end < linetoscr_diw_start)
		linetoscr_diw_end = linetoscr_diw_start;

	playfield_start = linetoscr_diw_start;
	playfield_end = linetoscr_diw_end;

	unpainted = var_VISIBLE_LEFT_BORDER < playfield_start ? 0 : var_VISIBLE_LEFT_BORDER - playfield_start;
	ham_src_pixel = MAX_PIXELS_PER_LINE + res_shift_from_window (playfield_start - native_ddf_left);
	unpainted = res_shift_from_window (unpainted);

	if (playfield_start < var_VISIBLE_LEFT_BORDER)
		playfield_start = var_VISIBLE_LEFT_BORDER;
	if (playfield_start > var_VISIBLE_RIGHT_BORDER)
		playfield_start = var_VISIBLE_RIGHT_BORDER;
	if (playfield_end < var_VISIBLE_LEFT_BORDER)
		playfield_end = var_VISIBLE_LEFT_BORDER;
	if (playfield_end > var_VISIBLE_RIGHT_BORDER)
		playfield_end = var_VISIBLE_RIGHT_BORDER;

	real_playfield_end = playfield_end;
	real_playfield_start = playfield_start;

	res_shift = -bplres;

	/* Now, compute some offsets.  */
	ddf_left -= DISPLAY_LEFT_SHIFT;
	ddf_left <<= bplres;
	pixels_offset = MAX_PIXELS_PER_LINE - ddf_left;

	src_pixel = MAX_PIXELS_PER_LINE + res_shift_from_window (playfield_start - native_ddf_left);
   
	if (dip_for_drawing->nr_sprites == 0)
		return;
	/* Must clear parts of apixels.  */
	if (linetoscr_diw_start < native_ddf_left) {
		int size = res_shift_from_window (native_ddf_left - linetoscr_diw_start);
		linetoscr_diw_start = native_ddf_left;
		uae4all_memclr (pixdata.apixels + MAX_PIXELS_PER_LINE - size, size);
	}
	if (linetoscr_diw_end > native_ddf_right) {
		int pos = res_shift_from_window (native_ddf_right - native_ddf_left);
		int size = res_shift_from_window (linetoscr_diw_end - native_ddf_right);
		linetoscr_diw_start = native_ddf_left;
		uae4all_memclr (pixdata.apixels + MAX_PIXELS_PER_LINE + pos, size);
	}
}

static __inline__ void fill_line (void)
{
    //int nints, nrem;
	unsigned int nints;
    int *start;
    xcolnr val;

	if(mainMenu_displayHires)
	{
		nints = var_GFXVIDINFO_WIDTH;
		start = (int *)(((char *)xlinebuffer) + ((var_VISIBLE_LEFT_BORDER << 1) * 2));
	}
	else
	{
		nints = var_GFXVIDINFO_WIDTH /2;
		start = (int *)(((char *)xlinebuffer) + (var_VISIBLE_LEFT_BORDER << 1));
	}

	val = colors_for_drawing.acolors[0];
	val |= val << 16;
	for (; nints > 0; nints -= 8, start += 8) {
		*start = val;
		*(start+1) = val;
		*(start+2) = val;
		*(start+3) = val;
		*(start+4) = val;
		*(start+5) = val;
		*(start+6) = val;
		*(start+7) = val;
	}
}

static void dummy_worker (int start, int stop)
{
}

static int ham_decode_pixel;
static unsigned int ham_lastcolor;
static unsigned int ham_lastcolor_pix0;
static int ham_firstpixel_in_line;

/* Decode HAM in the invisible portion of the display (left of VISIBLE_LEFT_BORDER),
   but don't draw anything in.  This is done to prepare HAM_LASTCOLOR for later,
   when decode_ham runs.  */
static void init_ham_decoding (void)
{
	int unpainted_amiga = res_shift_from_window (unpainted);
	ham_decode_pixel = src_pixel;
	ham_lastcolor = color_reg_get (&colors_for_drawing, 0);
	ham_firstpixel_in_line = 1;
	
	if (! bplham || (bplplanecnt != 6 && ((currprefs.chipset_mask & CSMASK_AGA) == 0 || bplplanecnt != 8))) {
		if (unpainted_amiga > 0) {
			int pv = pixdata.apixels[ham_decode_pixel + unpainted_amiga - 1];
			if (currprefs.chipset_mask & CSMASK_AGA)
				ham_lastcolor = colors_for_drawing.color_regs_aga[pv];
			else
				ham_lastcolor = colors_for_drawing.color_uae_regs_ecs[pv];
		}
	} else if (currprefs.chipset_mask & CSMASK_AGA) {
		if (bplplanecnt == 8) { /* AGA mode HAM8 */
			while (unpainted_amiga-- > 0) {
				if(ham_firstpixel_in_line)
					ham_lastcolor = ham_lastcolor_pix0;
				int pv = pixdata.apixels[ham_decode_pixel++];
				switch (pv & 0x3) {
					case 0x0: ham_lastcolor = colors_for_drawing.color_regs_aga[pv >> 2]; break;
					case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pv & 0xFC); break;
					case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pv & 0xFC) << 16; break;
					case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pv & 0xFC) << 8; break;
				}
				if(ham_firstpixel_in_line)
				{
					ham_lastcolor_pix0 = ham_lastcolor;
					ham_firstpixel_in_line = 0;
				}
			}
		} else if (bplplanecnt == 6) { /* AGA mode HAM6 */
			while (unpainted_amiga-- > 0) {
				if(ham_firstpixel_in_line)
					ham_lastcolor = ham_lastcolor_pix0;
				int pv = pixdata.apixels[ham_decode_pixel++];
				switch (pv & 0x30) {
					case 0x00: ham_lastcolor = colors_for_drawing.color_regs_aga[pv]; break;
					case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= (pv & 0xF) << 4; break;
					case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= (pv & 0xF) << 20; break;
					case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= (pv & 0xF) << 12; break;
				}
				if(ham_firstpixel_in_line)
				{
					ham_lastcolor_pix0 = ham_lastcolor;
					ham_firstpixel_in_line = 0;
				}
			}
		}
	} else {
		if (bplplanecnt == 6) { /* OCS/ECS mode HAM6 */
			while (unpainted_amiga-- > 0) {
				if(ham_firstpixel_in_line)
					ham_lastcolor = ham_lastcolor_pix0;
				int pv = pixdata.apixels[ham_decode_pixel++];
				switch (pv & 0x30) {
					case 0x00: ham_lastcolor = colors_for_drawing.color_uae_regs_ecs[pv]; break;
					case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
					case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
					case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
				}
				if(ham_firstpixel_in_line)
				{
					ham_lastcolor_pix0 = ham_lastcolor;
					ham_firstpixel_in_line = 0;
				}
			}
		}
	}
}

static void decode_ham (int pix, int stoppos)
{
	int todraw_amiga = res_shift_from_window (stoppos - pix);
	
	if (! bplham || (bplplanecnt != 6 && ((currprefs.chipset_mask & CSMASK_AGA) == 0 || bplplanecnt != 8))) {
		while (todraw_amiga-- > 0) {
			int pv = pixdata.apixels[ham_decode_pixel];
			if (currprefs.chipset_mask & CSMASK_AGA)
				ham_lastcolor = colors_for_drawing.color_regs_aga[pv];
			else
				ham_lastcolor = colors_for_drawing.color_uae_regs_ecs[pv];
			
			ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
		}
	} else if (currprefs.chipset_mask & CSMASK_AGA) {
		if (bplplanecnt == 8) { /* AGA mode HAM8 */
			while (todraw_amiga-- > 0) {
				if(ham_firstpixel_in_line)
					ham_lastcolor = ham_lastcolor_pix0;
				int pv = pixdata.apixels[ham_decode_pixel];
				switch (pv & 0x3) {
					case 0x0: ham_lastcolor = colors_for_drawing.color_regs_aga[pv >> 2]; break;
					case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pv & 0xFC); break;
					case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pv & 0xFC) << 16; break;
					case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pv & 0xFC) << 8; break;
				}
				if(ham_firstpixel_in_line)
				{
					ham_lastcolor_pix0 = ham_lastcolor;
					ham_firstpixel_in_line = 0;
				}
				ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
			}
		} else if (bplplanecnt == 6) { /* AGA mode HAM6 */
			while (todraw_amiga-- > 0) {
				if(ham_firstpixel_in_line)
					ham_lastcolor = ham_lastcolor_pix0;
				int pv = pixdata.apixels[ham_decode_pixel];
				switch (pv & 0x30) {
					case 0x00: ham_lastcolor = colors_for_drawing.color_regs_aga[pv]; break;
					case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= (pv & 0xF) << 4; break;
					case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= (pv & 0xF) << 20; break;
					case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= (pv & 0xF) << 12; break;
				}
				if(ham_firstpixel_in_line)
				{
					ham_lastcolor_pix0 = ham_lastcolor;
					ham_firstpixel_in_line = 0;
				}
				ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
			}
		}
	} else {
		if (bplplanecnt == 6) { /* OCS/ECS mode HAM6 */
			while (todraw_amiga-- > 0) {
				if(ham_firstpixel_in_line)
					ham_lastcolor = ham_lastcolor_pix0;
				int pv = pixdata.apixels[ham_decode_pixel];
				switch (pv & 0x30) {
					case 0x00: ham_lastcolor = colors_for_drawing.color_uae_regs_ecs[pv]; break;
					case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
					case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
					case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
				}
				if(ham_firstpixel_in_line)
				{
					ham_lastcolor_pix0 = ham_lastcolor;
					ham_firstpixel_in_line = 0;
				}
				ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
			}
		}
	}
}

static __inline__ void gen_pfield_tables (void)
{
	int i;

    /* For now, the AGA stuff is broken in the dual playfield case. We encode
     * sprites in dpf mode by ORing the pixel value with 0x80. To make dual
     * playfield rendering easy, the lookup tables contain are made linear for
     * values >= 128. That only works for OCS/ECS, though. */

	for (i = 0; i < 256; i++) {
		int plane1 = (i & 1) | ((i >> 1) & 2) | ((i >> 2) & 4) | ((i >> 3) & 8);
		int plane2 = ((i >> 1) & 1) | ((i >> 2) & 2) | ((i >> 3) & 4) | ((i >> 4) & 8);

		dblpf_2nd1[i] = plane1 == 0 ? (plane2 == 0 ? 0 : 2) : 1;
		dblpf_2nd2[i] = plane2 == 0 ? (plane1 == 0 ? 0 : 1) : 2;
		
		dblpf_ind1_aga[i] = plane1 == 0 ? plane2 : plane1;
		dblpf_ind2_aga[i] = plane2 == 0 ? plane1 : plane2;
		
		dblpf_ms1[i] = plane1 == 0 ? (plane2 == 0 ? 16 : 8) : 0;
		dblpf_ms2[i] = plane2 == 0 ? (plane1 == 0 ? 16 : 0) : 8;
		dblpf_ms[i] = i == 0 ? 16 : 8;
		if (plane2 > 0)
			plane2 += 8;
		dblpf_ind1[i] = i >= 128 ? i & 0x7F : (plane1 == 0 ? plane2 : plane1);
		dblpf_ind2[i] = i >= 128 ? i & 0x7F : (plane2 == 0 ? plane1 : plane2);

		sprite_offs[i] = (i & 15) ? 0 : 2;

		clxtab[i] = ((((i & 3) && (i & 12)) << 9)
				| (((i & 3) && (i & 48)) << 10)
				| (((i & 3) && (i & 192)) << 11)
				| (((i & 12) && (i & 48)) << 12)
				| (((i & 12) && (i & 192)) << 13)
				| (((i & 48) && (i & 192)) << 14));
	}
}

#ifndef UNROLL_DRAW_SPRITES

static __inline__ void draw_sprites_1 (struct sprite_entry *_GCCRES_ e, int dualpf,
				   int doubling, int has_attach)
{
    int *shift_lookup = dualpf ? (bpldualpfpri ? dblpf_ms2 : dblpf_ms1) : dblpf_ms;
    uae_u16 *buf = spixels + e->first_pixel;
    uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
    int pos, window_pos;
    register unsigned char change = line_changes[active_line];
    uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

    buf -= e->pos;
    stbuf -= e->pos;

    window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
    if (doubling)
	window_pos <<= 1;
    window_pos += pixels_offset;
    for (pos = e->pos; pos < e->max; pos += 1) {
	int maskshift, plfmask;
	unsigned int v = buf[pos];

	maskshift = shift_lookup[pixdata.apixels[window_pos]];
	plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
	v &= ~plfmask;
	if (v != 0) {
	    unsigned int vlo, vhi, col;
	    unsigned int v1 = v & 255;
	    int offs;

	    /* If non-transparent pixel is drawn in previously unchanged line,
        * that line has to be redrawn as well.
        */
       if (!change) {
          change = 1;
          pfield_draw_line_1 ();
       }

	    if (v1 == 0)
		offs = 4 + sprite_offs[v >> 8];
	    else
		offs = sprite_offs[v1];

	    v >>= offs << 1;
	    v &= 15;
 
	    if (has_attach && (stbuf[pos] & (1 << offs))) {
		col = v;
		    col += 16;
	    } else {
		vlo = v & 3;
		vhi = (v & (vlo - 1)) >> 2;
		col = (vlo | vhi);
		    col += 16;
		col += (offs << 1);
	    }
	    if (dualpf) {
		    col += 128;
		    if (doubling)
			pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
		    else
			pixdata.apixels[window_pos] = col;
	    } else {
		if (doubling)
		    pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
		else
		    pixdata.apixels[window_pos] = col;
	    }
	}
	window_pos += 1 << doubling;
    }
    line_changes_sprites = change;
}

#define draw_sprites_normal_sp_lo_nat(ENTRY) draw_sprites_1	(ENTRY, 0, 0, 0)
#define draw_sprites_normal_dp_lo_nat(ENTRY) draw_sprites_1	(ENTRY, 1, 0, 0)
#define draw_sprites_normal_sp_lo_at(ENTRY)  draw_sprites_1	(ENTRY, 0, 0, 1)
#define draw_sprites_normal_dp_lo_at(ENTRY)  draw_sprites_1	(ENTRY, 1, 0, 1)
#define draw_sprites_normal_sp_hi_nat(ENTRY) draw_sprites_1	(ENTRY, 0, 1, 0)
#define draw_sprites_normal_dp_hi_nat(ENTRY) draw_sprites_1	(ENTRY, 1, 1, 0)
#define draw_sprites_normal_sp_hi_at(ENTRY)  draw_sprites_1	(ENTRY, 0, 1, 1)
#define draw_sprites_normal_dp_hi_at(ENTRY)  draw_sprites_1	(ENTRY, 1, 1, 1)

#define decide_draw_sprites()

static __inline__ void draw_sprites_ecs (struct sprite_entry *_GCCRES_ e)
{
    uae4all_prof_start(12);
    if (e->has_attached)
	if (bplres == 1)
		if (bpldualpf)
		    draw_sprites_normal_dp_hi_at (e);
		else
		    draw_sprites_normal_sp_hi_at (e);
	else
		if (bpldualpf)
		    draw_sprites_normal_dp_lo_at (e);
		else
		    draw_sprites_normal_sp_lo_at (e);
    else
	if (bplres == 1)
		if (bpldualpf)
		    draw_sprites_normal_dp_hi_nat (e);
		else
		    draw_sprites_normal_sp_hi_nat (e);
	else
		if (bpldualpf)
		    draw_sprites_normal_dp_lo_nat (e);
		else
		    draw_sprites_normal_sp_lo_nat (e);
    uae4all_prof_end(12);
}

#else

static void draw_sprites_normal_sp_lo_nat(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;
   uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
         unsigned int vlo, vhi, col;
         unsigned int v1 = v & 255;
         int offs;
         if (v1 == 0)
            offs = 4 + sprite_offs[v >> 8];
         else
            offs = sprite_offs[v1];
         v >>= offs << 1;
         v &= 15;
         vlo = v & 3;
         vhi = (v & (vlo - 1)) >> 2;
         col = (vlo | vhi);
         col += 16;
         col += (offs << 1);
         pixdata.apixels[window_pos] = col;
      }
      window_pos ++;
   }
}

static void draw_sprites_normal_dp_lo_nat(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = (bpldualpfpri ? dblpf_ms2 : dblpf_ms1);
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;
   uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
         unsigned int vlo, vhi, col;
         unsigned int v1 = v & 255;
         int offs;
         if (v1 == 0)
            offs = 4 + sprite_offs[v >> 8];
         else
            offs = sprite_offs[v1];

         v >>= offs << 1;
         v &= 15;

         vlo = v & 3;
         vhi = (v & (vlo - 1)) >> 2;
         col = (vlo | vhi);
         col += 16;
         col += (offs << 1);
         col += 128;
         pixdata.apixels[window_pos] = col;
      }
      window_pos++;
   }
}

static void draw_sprites_normal_sp_lo_at(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;
   uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
         unsigned int vlo, vhi, col;
         unsigned int v1 = v & 255;
         int offs;
         if (v1 == 0)
            offs = 4 + sprite_offs[v >> 8];
         else
            offs = sprite_offs[v1];

         v >>= offs << 1;
         v &= 15;

         if ((stbuf[pos] & (1 << offs))) {
            col = v;
            col += 16;
         } else {
            vlo = v & 3;
            vhi = (v & (vlo - 1)) >> 2;
            col = (vlo | vhi);
            col += 16;
            col += (offs << 1);
         }
         pixdata.apixels[window_pos] = col;
      }
      window_pos++;
   }
}

static void draw_sprites_normal_dp_lo_at(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = (bpldualpfpri ? dblpf_ms2 : dblpf_ms1);
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;
   uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
         unsigned int vlo, vhi, col;
         unsigned int v1 = v & 255;
         int offs;
         if (v1 == 0)
            offs = 4 + sprite_offs[v >> 8];
         else
            offs = sprite_offs[v1];

         v >>= offs << 1;
         v &= 15;

         if ((stbuf[pos] & (1 << offs))) {
            col = v;
            col += 16;
         } else {
            vlo = v & 3;
            vhi = (v & (vlo - 1)) >> 2;
            col = (vlo | vhi);
            col += 16;
            col += (offs << 1);
         }
         col += 128;
         pixdata.apixels[window_pos] = col;
      }
      window_pos++;
   }
}

static void draw_sprites_normal_sp_hi_nat(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;
   uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos <<= 1;
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos ++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
         unsigned int vlo, vhi, col;
         unsigned int v1 = v & 255;
         int offs;
         if (v1 == 0)
            offs = 4 + sprite_offs[v >> 8];
         else
            offs = sprite_offs[v1];

         v >>= offs << 1;
         v &= 15;

         vlo = v & 3;
         vhi = (v & (vlo - 1)) >> 2;
         col = (vlo | vhi);
         col += 16;
         col += (offs << 1);
         pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
      }
      window_pos += 2;
   }
}


static void draw_sprites_normal_dp_hi_nat(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = (bpldualpfpri ? dblpf_ms2 : dblpf_ms1);
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;
   uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos <<= 1;
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos ++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
         unsigned int vlo, vhi, col;
         unsigned int v1 = v & 255;
         int offs;
         if (v1 == 0)
            offs = 4 + sprite_offs[v >> 8];
         else
            offs = sprite_offs[v1];

         v >>= offs << 1;
         v &= 15;

         vlo = v & 3;
         vhi = (v & (vlo - 1)) >> 2;
         col = (vlo | vhi);
         col += 16;
         col += (offs << 1);
         col += 128;
         pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
      }
      window_pos += 2;
   }
}

static void draw_sprites_normal_sp_hi_at(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;
   uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos <<= 1;
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
         unsigned int vlo, vhi, col;
         unsigned int v1 = v & 255;
         int offs;
         if (v1 == 0)
            offs = 4 + sprite_offs[v >> 8];
         else
            offs = sprite_offs[v1];

         v >>= offs << 1;
         v &= 15;

         if ((stbuf[pos] & (1 << offs))) {
            col = v;
            col += 16;
         } else {
            vlo = v & 3;
            vhi = (v & (vlo - 1)) >> 2;
            col = (vlo | vhi);
            col += 16;
            col += (offs << 1);
         }
         pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
      }
      window_pos += 2;
   }
}

static void draw_sprites_normal_dp_hi_at(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = (bpldualpfpri ? dblpf_ms2 : dblpf_ms1);
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;
   uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos <<= 1;
   window_pos += pixels_offset;

   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
         unsigned int vlo, vhi, col;
         unsigned int v1 = v & 255;
         int offs;
         if (v1 == 0)
            offs = 4 + sprite_offs[v >> 8];
         else
            offs = sprite_offs[v1];

         v >>= offs << 1;
         v &= 15;

         if ((stbuf[pos] & (1 << offs))) {
            col = v;
            col += 16;
         } else {
            vlo = v & 3;
            vhi = (v & (vlo - 1)) >> 2;
            col = (vlo | vhi);
            col += 16;
            col += (offs << 1);
         }
         col += 128;
         pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
      }
      window_pos += 2;
   }
}

typedef void (*draw_sprites_func)(struct sprite_entry *_GCCRES_ e);
static draw_sprites_func draw_sprites_dp_hi[2]={
	draw_sprites_normal_dp_hi_nat, draw_sprites_normal_dp_hi_at };
static draw_sprites_func draw_sprites_sp_hi[2]={
	draw_sprites_normal_sp_hi_nat, draw_sprites_normal_sp_hi_at };
static draw_sprites_func draw_sprites_dp_lo[2]={
	draw_sprites_normal_dp_lo_nat, draw_sprites_normal_dp_lo_at };
static draw_sprites_func draw_sprites_sp_lo[2]={
	draw_sprites_normal_sp_lo_nat, draw_sprites_normal_sp_lo_at };

static draw_sprites_func *draw_sprites_punt=draw_sprites_sp_lo;

static __inline__ void decide_draw_sprites(void) {
	if (bplres == 1)
		if (bpldualpf)
			draw_sprites_punt=draw_sprites_dp_hi;
		else
			draw_sprites_punt=draw_sprites_sp_hi;
	else
		if (bpldualpf)
			draw_sprites_punt=draw_sprites_dp_lo;
		else
			draw_sprites_punt=draw_sprites_sp_lo;
}

/* When looking at this function and the ones that inline it, bear in mind
   what an optimizing compiler will do with this code.  All callers of this
   function only pass in constant arguments (except for E).  This means
   that many of the if statements will go away completely after inlining.  */

/* NOTE: This function is called for AGA modes *only* */
STATIC_INLINE void draw_sprites_1 (struct sprite_entry *e, int ham, int dualpf,
				   int doubling, int skip, int has_attach, int aga)
{
   int *shift_lookup = dualpf ? (bpldualpfpri ? dblpf_ms2 : dblpf_ms1) : dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;
   uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) << 1);
   if (skip)
      window_pos >>= 1;
   else if (doubling)
      window_pos <<= 1;
   window_pos += pixels_offset;
   for (pos = e->pos; pos < e->max; pos += 1 << skip) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      /* The value in the shift lookup table is _half_ the shift count we
	   need.  This is because we can't shift 32 bits at once (undefined
	   behaviour in C).  */
      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
         unsigned int vlo, vhi, col;
         unsigned int v1 = v & 255;
		 
         /* OFFS determines the sprite pair with the highest priority that has
	       any bits set.  E.g. if we have 0xFF00 in the buffer, we have sprite
	       pairs 01 and 23 cleared, and pairs 45 and 67 set, so OFFS will
	       have a value of 4.
	       2 * OFFS is the bit number in V of the sprite pair, and it also
	       happens to be the color offset for that pair.  */
         int offs;
         if (v1 == 0)
            offs = 4 + sprite_offs[v >> 8];
         else
            offs = sprite_offs[v1];

         /* Shift highest priority sprite pair down to bit zero.  */
         v >>= offs * 2;
         v &= 15;

         if (has_attach && (stbuf[pos] & (1 << offs))) {
            col = v;
					col += sbasecol[1];
			} else {
				/* This sequence computes the correct color value.  We have to select
		   either the lower-numbered or the higher-numbered sprite in the pair.
		   We have to select the high one if the low one has all bits zero.
		   If the lower-numbered sprite has any bits nonzero, (VLO - 1) is in
		   the range of 0..2, and with the mask and shift, VHI will be zero.
		   If the lower-numbered sprite is zero, (VLO - 1) is a mask of
		   0xFFFFFFFF, and we select the bits of the higher numbered sprite
		   in VHI.
		   This is _probably_ more efficient than doing it with branches.  */
				vlo = v & 3;
				vhi = (v & (vlo - 1)) >> 2;
				col = (vlo | vhi);
					if (vhi > 0)
						col += sbasecol[1];
					else
						col += sbasecol[0];
				col += (offs * 2);
			}

         if (dualpf) {
					spriteagadpfpixels[window_pos] = col;
					if (doubling)
						spriteagadpfpixels[window_pos + 1] = col;
			} else if (ham) {
				col = color_reg_get (&colors_for_drawing, col);
					col ^= xor_val;
				ham_linebuf[window_pos] = col;
				if (doubling)
					ham_linebuf[window_pos + 1] = col;
			} else {
				col ^= xor_val;
				if (doubling)
					pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
				else
					pixdata.apixels[window_pos] = col;
			}
		}
		
		window_pos += 1 << doubling;
	}
}

/* not very optimized */
STATIC_INLINE void draw_sprites_aga (struct sprite_entry *e)
{
   int diff = RES_HIRES - bplres;
   if (diff > 0)
      draw_sprites_1 (e, dp_for_drawing->ham_seen, bpldualpf, 0, diff, e->has_attached, 1);
   else
      draw_sprites_1 (e, dp_for_drawing->ham_seen, bpldualpf, -diff, 0, e->has_attached, 1);
}

static __inline__ void draw_sprites_ecs (struct sprite_entry *_GCCRES_ e)
{
	uae4all_prof_start(12);
	draw_sprites_punt[e->has_attached](e);
	uae4all_prof_end(12);
}

#endif

#define MERGE(a,b,mask,shift) {\
    register uae_u32 tmp = mask & (a ^ (b >> shift)); \
    a ^= tmp; \
    b ^= (tmp << shift); \
}

#define MERGE_0(a,b,mask,shift) {\
   register uae_u32 tmp = mask & (b>>shift); \
   a = tmp; \
   b ^= (tmp << shift); \
}

#define GETLONG(P) (*(uae_u32 *)P)
#define DATA_POINTER(n) (line_data[lineno] + (n)*MAX_WORDS_PER_LINE*2)

#define DO_SWLONG(A,V) {\
	register uae_u8 *b = (uae_u8 *)(A); \
	register uae_u32 v = (V); \
	*b++ = v >> 24; \
	*b++ = v >> 16; \
	*b++ = v >> 8; \
	*b = v; \
}

#ifndef UNROLL_PFIELD

static __inline__ void pfield_doline_1 (uae_u32 *_GCCRES_ pixels, int wordcount, int planes)
{
    while (wordcount-- > 0) {
	uae_u32 b0, b1, b2, b3, b4, b5, b6, b7;

	b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0;
	switch (planes) {
	case 8: b0 = GETLONG ((uae_u32 *)real_bplpt[7]); real_bplpt[7] += 4;
	case 7: b1 = GETLONG ((uae_u32 *)real_bplpt[6]); real_bplpt[6] += 4;
	case 6: b2 = GETLONG ((uae_u32 *)real_bplpt[5]); real_bplpt[5] += 4;
	case 5: b3 = GETLONG ((uae_u32 *)real_bplpt[4]); real_bplpt[4] += 4;
	case 4: b4 = GETLONG ((uae_u32 *)real_bplpt[3]); real_bplpt[3] += 4;
	case 3: b5 = GETLONG ((uae_u32 *)real_bplpt[2]); real_bplpt[2] += 4;
	case 2: b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
	case 1: b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;
	}

	MERGE (b0, b1, 0x55555555, 1);
	MERGE (b2, b3, 0x55555555, 1);
	MERGE (b4, b5, 0x55555555, 1);
	MERGE (b6, b7, 0x55555555, 1);

	MERGE (b0, b2, 0x33333333, 2);
	MERGE (b1, b3, 0x33333333, 2);
	MERGE (b4, b6, 0x33333333, 2);
	MERGE (b5, b7, 0x33333333, 2);

	MERGE (b0, b4, 0x0f0f0f0f, 4);
	MERGE (b1, b5, 0x0f0f0f0f, 4);
	MERGE (b2, b6, 0x0f0f0f0f, 4);
	MERGE (b3, b7, 0x0f0f0f0f, 4);

	MERGE (b0, b1, 0x00ff00ff, 8);
	MERGE (b2, b3, 0x00ff00ff, 8);
	MERGE (b4, b5, 0x00ff00ff, 8);
	MERGE (b6, b7, 0x00ff00ff, 8);

	MERGE (b0, b2, 0x0000ffff, 16);
	DO_SWLONG(pixels, b0);
	DO_SWLONG(pixels + 4, b2);
	MERGE (b1, b3, 0x0000ffff, 16);
	DO_SWLONG(pixels + 2, b1);
	DO_SWLONG(pixels + 6, b3);
	MERGE (b4, b6, 0x0000ffff, 16);
	DO_SWLONG(pixels + 1, b4);
	DO_SWLONG(pixels + 5, b6);
	MERGE (b5, b7, 0x0000ffff, 16);
	DO_SWLONG(pixels + 3, b5);
	DO_SWLONG(pixels + 7, b7);
	pixels += 8;
    }
}

#define pfield_doline_n1(DTA,CNT) pfield_doline_1 (DTA, CNT, 1)
#define pfield_doline_n2(DTA,CNT) pfield_doline_1 (DTA, CNT, 2)
#define pfield_doline_n3(DTA,CNT) pfield_doline_1 (DTA, CNT, 3)
#define pfield_doline_n4(DTA,CNT) pfield_doline_1 (DTA, CNT, 4)
#define pfield_doline_n5(DTA,CNT) pfield_doline_1 (DTA, CNT, 5)
#define pfield_doline_n6(DTA,CNT) pfield_doline_1 (DTA, CNT, 6)
#define pfield_doline_n7(DTA,CNT) pfield_doline_1 (DTA, CNT, 7)
#define pfield_doline_n8(DTA,CNT) pfield_doline_1 (DTA, CNT, 8)

static _INLINE_ void pfield_doline (int lineno)
{
	return;
    uae4all_prof_start(11);
    int wordcount = dp_for_drawing->plflinelen;
    uae_u32 *data = pixdata.apixels_l + MAX_PIXELS_PER_LINE/4;

    real_bplpt[0] = DATA_POINTER (0);
    real_bplpt[1] = DATA_POINTER (1);
    real_bplpt[2] = DATA_POINTER (2);
    real_bplpt[3] = DATA_POINTER (3);
    real_bplpt[4] = DATA_POINTER (4);
    real_bplpt[5] = DATA_POINTER (5);
    real_bplpt[6] = DATA_POINTER (6);
    real_bplpt[7] = DATA_POINTER (7);

    switch (bplplanecnt) {
    default: break;
    case 0: uae4all_memclr(data, wordcount << 5); break;
    case 1: pfield_doline_n1 (data, wordcount); break;
    case 2: pfield_doline_n2 (data, wordcount); break;
    case 3: pfield_doline_n3 (data, wordcount); break;
    case 4: pfield_doline_n4 (data, wordcount); break;
    case 5: pfield_doline_n5 (data, wordcount); break;
    case 6: pfield_doline_n6 (data, wordcount); break;
    case 7: pfield_doline_n7 (data, wordcount); break;
    case 8: pfield_doline_n8 (data, wordcount); break;
    }
    uae4all_prof_end(11);
}

#else

#if defined(__SYMBIAN32__) && !defined(__WINS__) && defined(USE_ASSEMBLER_CODE)
   /* Assembler functions (arm_support.s) */
   #ifdef __cplusplus
      extern "C" {
   #endif
         void PFIELD_DOLINE_N4 (uae_u32 *_GCCRES_ pixels, int wordcount, uae_u8 *line_data, uae_u32 max_words);
         void PFIELD_DOLINE_N5 (uae_u32 *_GCCRES_ pixels, int wordcount, uae_u8 *line_data, uae_u32 max_words);
         void PFIELD_DOLINE_N6 (uae_u32 *_GCCRES_ pixels, int wordcount, uae_u8 *line_data, uae_u32 max_words);
         void PFIELD_DOLINE_N7 (uae_u32 *_GCCRES_ pixels, int wordcount, uae_u8 *line_data, uae_u32 max_words);
         void PFIELD_DOLINE_N8 (uae_u32 *_GCCRES_ pixels, int wordcount, uae_u8 *line_data, uae_u32 max_words);
   #ifdef __cplusplus
      }
   #endif
#endif

#ifdef USE_ARMNEON

#ifdef __cplusplus
  extern "C" {
#endif
    void ARM_doline_n1(uae_u32 *pixels, int wordcount, int lineno);
    void NEON_doline_n2(uae_u32 *pixels, int wordcount, int lineno);
    void NEON_doline_n3(uae_u32 *pixels, int wordcount, int lineno);
    void NEON_doline_n4(uae_u32 *pixels, int wordcount, int lineno);
    void NEON_doline_n6(uae_u32 *pixels, int wordcount, int lineno);
    void NEON_doline_n8(uae_u32 *pixels, int wordcount, int lineno);
#ifdef __cplusplus
  }
#endif

#endif

static void pfield_doline_n1 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
  uae_u8 *real_bplpt0;

   real_bplpt0 = DATA_POINTER (0);

   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;
      b7 = GETLONG ((uae_u32 *)real_bplpt0); real_bplpt0 += 4;

      MERGE_0(b6, b7, 0x55555555, 1);

      MERGE_0(b4, b6, 0x33333333, 2);
      MERGE_0(b5, b7, 0x33333333, 2);

      MERGE_0(b0, b4, 0x0f0f0f0f, 4);
      MERGE_0(b1, b5, 0x0f0f0f0f, 4);
      MERGE_0(b2, b6, 0x0f0f0f0f, 4);
      MERGE_0(b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      DO_SWLONG(pixels, b0);
      DO_SWLONG(pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      DO_SWLONG(pixels + 2, b1);
      DO_SWLONG(pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      DO_SWLONG(pixels + 1, b4);
      DO_SWLONG(pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      DO_SWLONG(pixels + 3, b5);
      DO_SWLONG(pixels + 7, b7);
      pixels += 8;
   }
}

static void pfield_doline_n2 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
  uae_u8 *real_bplpt[2];

   real_bplpt[0] = DATA_POINTER (0);
   real_bplpt[1] = DATA_POINTER (1);

   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;
      b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
      b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;

      MERGE (b6, b7, 0x55555555, 1);

      MERGE_0(b4, b6, 0x33333333, 2);
      MERGE_0(b5, b7, 0x33333333, 2);

      MERGE_0(b0, b4, 0x0f0f0f0f, 4);
      MERGE_0(b1, b5, 0x0f0f0f0f, 4);
      MERGE_0(b2, b6, 0x0f0f0f0f, 4);
      MERGE_0(b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      DO_SWLONG(pixels, b0);
      DO_SWLONG(pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      DO_SWLONG(pixels + 2, b1);
      DO_SWLONG(pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      DO_SWLONG(pixels + 1, b4);
      DO_SWLONG(pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      DO_SWLONG(pixels + 3, b5);
      DO_SWLONG(pixels + 7, b7);
      pixels += 8;
   }
}

static void pfield_doline_n3 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
  uae_u8 *real_bplpt[3];

   real_bplpt[0] = DATA_POINTER (0);
   real_bplpt[1] = DATA_POINTER (1);
   real_bplpt[2] = DATA_POINTER (2);
   
   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;
      b5 = GETLONG ((uae_u32 *)real_bplpt[2]); real_bplpt[2] += 4;
      b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
      b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;

      MERGE_0(b4, b5, 0x55555555, 1);
      MERGE (b6, b7, 0x55555555, 1);

      MERGE (b4, b6, 0x33333333, 2);
      MERGE (b5, b7, 0x33333333, 2);

      MERGE_0(b0, b4, 0x0f0f0f0f, 4);
      MERGE_0(b1, b5, 0x0f0f0f0f, 4);
      MERGE_0(b2, b6, 0x0f0f0f0f, 4);
      MERGE_0(b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      DO_SWLONG(pixels, b0);
      DO_SWLONG(pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      DO_SWLONG(pixels + 2, b1);
      DO_SWLONG(pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      DO_SWLONG(pixels + 1, b4);
      DO_SWLONG(pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      DO_SWLONG(pixels + 3, b5);
      DO_SWLONG(pixels + 7, b7);
      pixels += 8;
   }
}

static void pfield_doline_n4 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
#if defined(__SYMBIAN32__) && !defined(__WINS__) && defined(USE_ASSEMBLER_CODE)
   PFIELD_DOLINE_N4 (pixels, wordcount, (uae_u8 *)&line_data[lineno], MAX_WORDS_PER_LINE * 2);
#else

  uae_u8 *real_bplpt[4];

  real_bplpt[0] = DATA_POINTER (0);
  real_bplpt[1] = DATA_POINTER (1);
  real_bplpt[2] = DATA_POINTER (2);
  real_bplpt[3] = DATA_POINTER (3);

   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;
      b4 = GETLONG ((uae_u32 *)real_bplpt[3]); real_bplpt[3] += 4;
      b5 = GETLONG ((uae_u32 *)real_bplpt[2]); real_bplpt[2] += 4;
      b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
      b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;

      MERGE (b4, b5, 0x55555555, 1);
      MERGE (b6, b7, 0x55555555, 1);

      MERGE (b4, b6, 0x33333333, 2);
      MERGE (b5, b7, 0x33333333, 2);

      MERGE_0(b0, b4, 0x0f0f0f0f, 4);
      MERGE_0(b1, b5, 0x0f0f0f0f, 4);
      MERGE_0(b2, b6, 0x0f0f0f0f, 4);
      MERGE_0(b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      DO_SWLONG(pixels, b0);
      DO_SWLONG(pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      DO_SWLONG(pixels + 2, b1);
      DO_SWLONG(pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      DO_SWLONG(pixels + 1, b4);
      DO_SWLONG(pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      DO_SWLONG(pixels + 3, b5);
      DO_SWLONG(pixels + 7, b7);
      pixels += 8;
   }
#endif
}

static void pfield_doline_n5 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
#if defined(__SYMBIAN32__) && !defined(__WINS__) && defined(USE_ASSEMBLER_CODE)
   PFIELD_DOLINE_N5 (pixels, wordcount, (uae_u8 *)&line_data[lineno], MAX_WORDS_PER_LINE * 2);
#else
  uae_u8 *real_bplpt[5];

   real_bplpt[0] = DATA_POINTER (0);
   real_bplpt[1] = DATA_POINTER (1);
   real_bplpt[2] = DATA_POINTER (2);
   real_bplpt[3] = DATA_POINTER (3);
   real_bplpt[4] = DATA_POINTER (4);

   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;
      b3 = GETLONG ((uae_u32 *)real_bplpt[4]); real_bplpt[4] += 4;
      b4 = GETLONG ((uae_u32 *)real_bplpt[3]); real_bplpt[3] += 4;
      b5 = GETLONG ((uae_u32 *)real_bplpt[2]); real_bplpt[2] += 4;
      b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
      b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;

      MERGE_0(b2, b3, 0x55555555, 1);
      MERGE (b4, b5, 0x55555555, 1);
      MERGE (b6, b7, 0x55555555, 1);

      MERGE_0(b0, b2, 0x33333333, 2);
      MERGE_0(b1, b3, 0x33333333, 2);
      MERGE (b4, b6, 0x33333333, 2);
      MERGE (b5, b7, 0x33333333, 2);

      MERGE (b0, b4, 0x0f0f0f0f, 4);
      MERGE (b1, b5, 0x0f0f0f0f, 4);
      MERGE (b2, b6, 0x0f0f0f0f, 4);
      MERGE (b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      DO_SWLONG(pixels, b0);
      DO_SWLONG(pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      DO_SWLONG(pixels + 2, b1);
      DO_SWLONG(pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      DO_SWLONG(pixels + 1, b4);
      DO_SWLONG(pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      DO_SWLONG(pixels + 3, b5);
      DO_SWLONG(pixels + 7, b7);
      pixels += 8;
   }
#endif
}

static void pfield_doline_n6 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
#if defined(__SYMBIAN32__) && !defined(__WINS__) && defined(USE_ASSEMBLER_CODE)
   PFIELD_DOLINE_N6 (pixels, wordcount, (uae_u8 *)&line_data[lineno], MAX_WORDS_PER_LINE * 2);
#else
  uae_u8 *real_bplpt[6];

   real_bplpt[0] = DATA_POINTER (0);
   real_bplpt[1] = DATA_POINTER (1);
   real_bplpt[2] = DATA_POINTER (2);
   real_bplpt[3] = DATA_POINTER (3);
   real_bplpt[4] = DATA_POINTER (4);
   real_bplpt[5] = DATA_POINTER (5);
   
   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;
      b2 = GETLONG ((uae_u32 *)real_bplpt[5]); real_bplpt[5] += 4;
      b3 = GETLONG ((uae_u32 *)real_bplpt[4]); real_bplpt[4] += 4;
      b4 = GETLONG ((uae_u32 *)real_bplpt[3]); real_bplpt[3] += 4;
      b5 = GETLONG ((uae_u32 *)real_bplpt[2]); real_bplpt[2] += 4;
      b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
      b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;

      MERGE (b2, b3, 0x55555555, 1);
      MERGE (b4, b5, 0x55555555, 1);
      MERGE (b6, b7, 0x55555555, 1);

      MERGE_0(b0, b2, 0x33333333, 2);
      MERGE_0(b1, b3, 0x33333333, 2);
      MERGE (b4, b6, 0x33333333, 2);
      MERGE (b5, b7, 0x33333333, 2);

      MERGE (b0, b4, 0x0f0f0f0f, 4);
      MERGE (b1, b5, 0x0f0f0f0f, 4);
      MERGE (b2, b6, 0x0f0f0f0f, 4);
      MERGE (b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      DO_SWLONG(pixels, b0);
      DO_SWLONG(pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      DO_SWLONG(pixels + 2, b1);
      DO_SWLONG(pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      DO_SWLONG(pixels + 1, b4);
      DO_SWLONG(pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      DO_SWLONG(pixels + 3, b5);
      DO_SWLONG(pixels + 7, b7);
      pixels += 8;
   }
#endif
}

static void pfield_doline_n7 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
#if defined(__SYMBIAN32__) && !defined(__WINS__) && defined(USE_ASSEMBLER_CODE)
   PFIELD_DOLINE_N7 (pixels, wordcount, (uae_u8 *)&line_data[lineno], MAX_WORDS_PER_LINE * 2);
#else
  uae_u8 *real_bplpt[7];
   real_bplpt[0] = DATA_POINTER (0);
   real_bplpt[1] = DATA_POINTER (1);
   real_bplpt[2] = DATA_POINTER (2);
   real_bplpt[3] = DATA_POINTER (3);
   real_bplpt[4] = DATA_POINTER (4);
   real_bplpt[5] = DATA_POINTER (5);
   real_bplpt[6] = DATA_POINTER (6);

   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;
      b1 = GETLONG ((uae_u32 *)real_bplpt[6]); real_bplpt[6] += 4;
      b2 = GETLONG ((uae_u32 *)real_bplpt[5]); real_bplpt[5] += 4;
      b3 = GETLONG ((uae_u32 *)real_bplpt[4]); real_bplpt[4] += 4;
      b4 = GETLONG ((uae_u32 *)real_bplpt[3]); real_bplpt[3] += 4;
      b5 = GETLONG ((uae_u32 *)real_bplpt[2]); real_bplpt[2] += 4;
      b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
      b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;

      MERGE_0(b0, b1, 0x55555555, 1);
      MERGE (b2, b3, 0x55555555, 1);
      MERGE (b4, b5, 0x55555555, 1);
      MERGE (b6, b7, 0x55555555, 1);

      MERGE (b0, b2, 0x33333333, 2);
      MERGE (b1, b3, 0x33333333, 2);
      MERGE (b4, b6, 0x33333333, 2);
      MERGE (b5, b7, 0x33333333, 2);

      MERGE (b0, b4, 0x0f0f0f0f, 4);
      MERGE (b1, b5, 0x0f0f0f0f, 4);
      MERGE (b2, b6, 0x0f0f0f0f, 4);
      MERGE (b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      DO_SWLONG(pixels, b0);
      DO_SWLONG(pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      DO_SWLONG(pixels + 2, b1);
      DO_SWLONG(pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      DO_SWLONG(pixels + 1, b4);
      DO_SWLONG(pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      DO_SWLONG(pixels + 3, b5);
      DO_SWLONG(pixels + 7, b7);
      pixels += 8;
   }
#endif
}

static void pfield_doline_n8 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
#if defined(__SYMBIAN32__) && !defined(__WINS__) && defined(USE_ASSEMBLER_CODE)
   PFIELD_DOLINE_N8 (pixels, wordcount, (uae_u8 *)&line_data[lineno], MAX_WORDS_PER_LINE * 2);
#else
  uae_u8 *real_bplpt[8];

  real_bplpt[0] = DATA_POINTER (0);
  real_bplpt[1] = DATA_POINTER (1);
  real_bplpt[2] = DATA_POINTER (2);
  real_bplpt[3] = DATA_POINTER (3);
  real_bplpt[4] = DATA_POINTER (4);
  real_bplpt[5] = DATA_POINTER (5);
  real_bplpt[6] = DATA_POINTER (6);
  real_bplpt[7] = DATA_POINTER (7);

   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;
      b0 = GETLONG ((uae_u32 *)real_bplpt[7]); real_bplpt[7] += 4;
      b1 = GETLONG ((uae_u32 *)real_bplpt[6]); real_bplpt[6] += 4;
      b2 = GETLONG ((uae_u32 *)real_bplpt[5]); real_bplpt[5] += 4;
      b3 = GETLONG ((uae_u32 *)real_bplpt[4]); real_bplpt[4] += 4;
      b4 = GETLONG ((uae_u32 *)real_bplpt[3]); real_bplpt[3] += 4;
      b5 = GETLONG ((uae_u32 *)real_bplpt[2]); real_bplpt[2] += 4;
      b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
      b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;

      MERGE (b0, b1, 0x55555555, 1);
      MERGE (b2, b3, 0x55555555, 1);
      MERGE (b4, b5, 0x55555555, 1);
      MERGE (b6, b7, 0x55555555, 1);

      MERGE (b0, b2, 0x33333333, 2);
      MERGE (b1, b3, 0x33333333, 2);
      MERGE (b4, b6, 0x33333333, 2);
      MERGE (b5, b7, 0x33333333, 2);

      MERGE (b0, b4, 0x0f0f0f0f, 4);
      MERGE (b1, b5, 0x0f0f0f0f, 4);
      MERGE (b2, b6, 0x0f0f0f0f, 4);
      MERGE (b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      DO_SWLONG(pixels, b0);
      DO_SWLONG(pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      DO_SWLONG(pixels + 2, b1);
      DO_SWLONG(pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      DO_SWLONG(pixels + 1, b4);
      DO_SWLONG(pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      DO_SWLONG(pixels + 3, b5);
      DO_SWLONG(pixels + 7, b7);
      pixels += 8;
   }
#endif
}

static void pfield_doline_n0 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
	uae4all_memclr(pixels, wordcount << 5);
}

static void pfield_doline_dummy (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
}

typedef void (*pfield_doline_func)(uae_u32 *_GCCRES_, int, int);
#ifdef USE_ARMNEON

static pfield_doline_func pfield_doline_n[16]={
	pfield_doline_n0, ARM_doline_n1, NEON_doline_n2, NEON_doline_n3,
	NEON_doline_n4, pfield_doline_n5, NEON_doline_n6, pfield_doline_n7,
	NEON_doline_n8, pfield_doline_dummy,pfield_doline_dummy,pfield_doline_dummy,
	pfield_doline_dummy,pfield_doline_dummy,pfield_doline_dummy,pfield_doline_dummy
};

#else

static pfield_doline_func pfield_doline_n[16]={
	pfield_doline_n0, pfield_doline_n1,pfield_doline_n2,pfield_doline_n3,
	pfield_doline_n4, pfield_doline_n5,pfield_doline_n6,pfield_doline_n7,
	pfield_doline_n8, pfield_doline_dummy,pfield_doline_dummy,pfield_doline_dummy,
	pfield_doline_dummy,pfield_doline_dummy,pfield_doline_dummy,pfield_doline_dummy
};
#endif

static __inline__ void pfield_doline (int lineno)
{
  pfield_doline_n[bplplanecnt](pixdata.apixels_l + MAX_PIXELS_PER_LINE/4,dp_for_drawing->plflinelen,lineno);
}

#endif

void init_row_map (void)
{
  int i;

	gfx_mem = (char *)prSDLScreen->pixels;
	gfx_rowbytes = prSDLScreen->pitch;
  for (i = 0; i < gfxHeight + 1; i++)
		row_map[i] = gfx_mem + gfx_rowbytes * i;
}

static _INLINE_ void init_aspect_maps (void)
{
    int i, maxl;

    if (native2amiga_line_map)
	free (native2amiga_line_map);
    if (amiga2aspect_line_map)
	free (amiga2aspect_line_map);

    /* At least for this array the +1 is necessary. */
    amiga2aspect_line_map = (int *)xmalloc (sizeof (int) * (MAXVPOS + 1)*2 + 1);
    native2amiga_line_map = (int *)xmalloc (sizeof (int) * GFXVIDINFO_HEIGHT);

    maxl = (MAXVPOS + 1);
    min_ypos_for_screen = minfirstline;
    max_drawn_amiga_line = -1;
    for (i = 0; i < maxl; i++) {
	int v = (int) (i - min_ypos_for_screen);
	if (v >= GFXVIDINFO_HEIGHT && max_drawn_amiga_line == -1)
	    max_drawn_amiga_line = i - min_ypos_for_screen;
	if (i < min_ypos_for_screen || v >= GFXVIDINFO_HEIGHT)
	    v = -1;
	amiga2aspect_line_map[i] = v;
    }

    for (i = GFXVIDINFO_HEIGHT; i--;)
	native2amiga_line_map[i] = -1;

    for (i = maxl-1; i >= min_ypos_for_screen; i--) {
	register int j;
	if (amiga2aspect_line_map[i] == -1)
	    continue;
	for (j = amiga2aspect_line_map[i]; j < GFXVIDINFO_HEIGHT && native2amiga_line_map[j] == -1; j++)
	    native2amiga_line_map[j] = i;
    }
}

/*
 * One drawing frame has been finished. Tell the graphics code about it.
 */
static __inline__ void do_flush_screen ()
{
	flush_block ();
  unlockscr ();
}

static int drawing_color_matches;
static enum { color_match_acolors, color_match_full } color_match_type;

/* Set up colors_for_drawing to the state at the beginning of the currently drawn
   line.  Try to avoid copying color tables around whenever possible.  */
static __inline__ void adjust_drawing_colors (int ctable, int need_full)
{
    if (drawing_color_matches != ctable)
    {
		if (need_full)
		{
			color_reg_cpy (&colors_for_drawing, curr_color_tables + ctable);
			color_match_type = color_match_full;
		}
		else
		{
			uae4all_memcpy (colors_for_drawing.acolors, curr_color_tables[ctable].acolors,
			sizeof colors_for_drawing.acolors);
			color_match_type = color_match_acolors;
		}
		drawing_color_matches = ctable;
    }
    else if (need_full && color_match_type != color_match_full)
    {
		color_reg_cpy (&colors_for_drawing, &curr_color_tables[ctable]);
		color_match_type = color_match_full;
    }
}

static _INLINE_ void do_color_changes (line_draw_func worker_border, line_draw_func worker_pfield)
{
    int i;
    int lastpos = var_VISIBLE_LEFT_BORDER;

    for (i = dip_for_drawing->first_color_change; i <= dip_for_drawing->last_color_change; i++)
    {
		int regno = curr_color_changes[i].regno;
		unsigned int value = curr_color_changes[i].value;
		int nextpos, nextpos_in_range;
		if (i == dip_for_drawing->last_color_change)
			nextpos = var_VISIBLE_RIGHT_BORDER;
		else
			nextpos = coord_hw_to_window_x (curr_color_changes[i].linepos << 1);

		nextpos_in_range = nextpos;
		if (nextpos > var_VISIBLE_RIGHT_BORDER)
			nextpos_in_range = var_VISIBLE_RIGHT_BORDER;

		if (nextpos_in_range > lastpos)
		{
			if (lastpos < playfield_start)
			{
			int t = nextpos_in_range <= playfield_start ? nextpos_in_range : playfield_start;
			(*worker_border) (lastpos, t);
			lastpos = t;
			}
		}
		if (nextpos_in_range > lastpos)
		{
			if (lastpos >= playfield_start && lastpos < playfield_end)
			{
			int t = nextpos_in_range <= playfield_end ? nextpos_in_range : playfield_end;
			(*worker_pfield) (lastpos, t);
			lastpos = t;
			}
		}
		if (nextpos_in_range > lastpos)
		{
			if (lastpos >= playfield_end)
			(*worker_border) (lastpos, nextpos_in_range);
			lastpos = nextpos_in_range;
		}
		if (i != dip_for_drawing->last_color_change)
		{
			if (regno != -1)
			{
			color_reg_set (&colors_for_drawing, regno, value);
			colors_for_drawing.acolors[regno] = getxcolor (value);
			}
		}
		if (lastpos >= var_VISIBLE_RIGHT_BORDER)
			break;
    }
}

/* We only save hardware registers during the hardware frame. Now, when
 * drawing the frame, we expand the data into a slightly more useful
 * form. */
static __inline__ void pfield_expand_dp_bplcon (void)
{
    int brdblank_2;
    static int b2;

    bplres = dp_for_drawing->bplres;
    bplplanecnt = dp_for_drawing->nr_planes;
    bplham = dp_for_drawing->ham_seen;

    if (currprefs.chipset_mask & CSMASK_AGA) {
    	/* The KILLEHB bit exists in ECS, but is apparently meant for Genlock
    	 * stuff, and it's set by some demos (e.g. Andromeda Seven Seas) */
    	bplehb = ((dp_for_drawing->bplcon0 & 0xFCC0) == 0x6000 && !(dp_for_drawing->bplcon2 & 0x200));
        bpldualpf2of = (dp_for_drawing->bplcon3 >> 10) & 7;
        sbasecol[0] = ((dp_for_drawing->bplcon4 >> 4) & 15) << 4;
        sbasecol[1] = ((dp_for_drawing->bplcon4 >> 0) & 15) << 4;
    } else {
    	bplehb = (dp_for_drawing->bplcon0 & 0xFC00) == 0x6000;
    }
    plf1pri = dp_for_drawing->bplcon2 & 7;
    plf2pri = (dp_for_drawing->bplcon2 >> 3) & 7;
    plf_sprite_mask = 0xFFFF0000 << (4 * plf2pri);
    plf_sprite_mask |= (0x0000FFFF << (4 * plf1pri)) & 0xFFFF;
    bpldualpf = (dp_for_drawing->bplcon0 & 0x400) == 0x400;
    bpldualpfpri = (dp_for_drawing->bplcon2 & 0x40) == 0x40;
}

static __inline__ void pfield_draw_line_1 (int lineno) {
   pfield_doline (lineno);

   adjust_drawing_colors (dp_for_drawing->ctable, dp_for_drawing->ham_seen || bplehb);
   
   /* The problem is that we must call decode_ham() BEFORE we do the
    * sprites. */
   if (dp_for_drawing->ham_seen)
   {
      init_ham_decoding ();
      if (dip_for_drawing->nr_color_changes == 0)
      {
         /* The easy case: need to do HAM decoding only once for the
          * full line. */
         decode_ham (var_VISIBLE_LEFT_BORDER, var_VISIBLE_RIGHT_BORDER);
      }
      else
      {
         do_color_changes (dummy_worker, decode_ham);
         adjust_drawing_colors (dp_for_drawing->ctable, dp_for_drawing->ham_seen || bplehb);
      }
      bplham = dp_for_drawing->ham_at_start;
   }
}

static __inline__ void pfield_draw_line (int lineno, int gfx_ypos)
{
   xlinebuffer = row_map[gfx_ypos];
	 if(mainMenu_displayHires)
	 		xlinebuffer -= (var_LINETOSCR_X_ADJUST_BYTES + moveX) << 1;
	 else
   		xlinebuffer -= var_LINETOSCR_X_ADJUST_BYTES + moveX;

   dp_for_drawing = line_decisions + lineno;
   dip_for_drawing = curr_drawinfo + lineno;
   
	if (dp_for_drawing->plfleft != -1)
	{
		pfield_expand_dp_bplcon ();
		pfield_init_linetoscr ();
		pfield_draw_line_1 (lineno);
      
		/* Lines in which sprites are drawn (transparent color is ignored),
		* have to be redrawn as well.
		*/
		if (dip_for_drawing->nr_sprites)
		{
			int i;
			decide_draw_sprites();
			for (i = 0; i < dip_for_drawing->nr_sprites; i++) {
				if (currprefs.chipset_mask & CSMASK_AGA)
					draw_sprites_aga (curr_sprite_entries + dip_for_drawing->first_sprite_entry + i);
				else
					draw_sprites_ecs (curr_sprite_entries + dip_for_drawing->first_sprite_entry + i);
			}
		}
   
		/* Line was changed or sprites have been drawn in it? */
		do_color_changes ((void (*)(int, int))pfield_do_fill_line, (void (*)(int, int))pfield_do_linetoscr);
	}
	else
	{
		adjust_drawing_colors (dp_for_drawing->ctable, 0);
		if (dip_for_drawing->nr_color_changes == 0)
		{
			fill_line ();
		}
		else
		{
			playfield_start = var_VISIBLE_RIGHT_BORDER;
			playfield_end = var_VISIBLE_RIGHT_BORDER;
			do_color_changes((void (*)(int, int))pfield_do_fill_line, (void (*)(int, int))pfield_do_fill_line);
		}
	}
}

static _INLINE_ void init_drawing_frame (void)
{
    init_hardware_for_drawing_frame ();

    linestate_first_undecided = 0;

    thisframe_y_adjust = minfirstline;

    thisframe_y_adjust_real = thisframe_y_adjust + moveY;
    max_ypos_thisframe = (maxvpos - thisframe_y_adjust);

    max_diwstop = 0;
    min_diwstart = 10000;

    drawing_color_matches = -1;
    
    if (currprefs.chipset_mask & CSMASK_AGA)
    	ham_lastcolor_pix0 = colors_for_drawing.color_regs_aga[0];
    else
    	ham_lastcolor_pix0 = colors_for_drawing.color_uae_regs_ecs[0];
}

/*
 * Some code to put status information on the screen.
 */
#define TD_PADX 10
#define TD_PADY 2
#define TD_WIDTH 32
#define TD_LED_WIDTH 24
#define TD_LED_HEIGHT 4

#define TD_RIGHT 1
#define TD_BOTTOM 2

static int td_pos = (TD_RIGHT|TD_BOTTOM);

#define TD_NUM_WIDTH 7
#define TD_NUM_HEIGHT 7

#define TD_TOTAL_HEIGHT (TD_PADY * 2 + TD_NUM_HEIGHT)

static const char *numbers = { /* ugly */
"------ ------ ------ ------ ------ ------ ------ ------ ------ ------ "
"-xxxxx ---xx- -xxxxx -xxxxx -x---x -xxxxx -xxxxx -xxxxx -xxxxx -xxxxx "
"-x---x ----x- -----x -----x -x---x -x---- -x---- -----x -x---x -x---x "
"-x---x ----x- -xxxxx -xxxxx -xxxxx -xxxxx -xxxxx ----x- -xxxxx -xxxxx "
"-x---x ----x- -x---- -----x -----x -----x -x---x ---x-- -x---x -----x "
"-xxxxx ----x- -xxxxx -xxxxx -----x -xxxxx -xxxxx ---x-- -xxxxx -xxxxx "
"------ ------ ------ ------ ------ ------ ------ ------ ------ ------ "
};

static const char *letters = { /* ugly */
"------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ "
"-xxxxx -xxxxx -xxxxx -xxxx- -xxxxx -xxxxx -xxxxx -x---x --xx-- -----x -x--x- -x---- -x---x -x---x --xxx- -xxxx- -xxxx- -xxxx- -xxxxx -xxxxx -x---x -x---x -x---x -x---x -x---x -xxxxx "
"-x---x -x---x -x---- -x---x -x---- -x---- -x---- -x---x --xx-- -----x -x-x-- -x---- -xxxxx -xx--x -x---x -x---x -x---- -x---x -x---- ---x-- -x---x -x---x --x-x- --x-x- -x---x ----x- "
"-xxxxx -xxxxx -x---- -x---x -xxxxx -xxxx- -xxxxx -xxxxx --xx-- -----x -xx--- -x---- -x---x -x-x-x -x---x -xxxx- -x---- -xxxx- -xxxxx ---x-- -x---x -x---x ---x-- ---x-- -x-x-x ---x-- "
"-x---x -x---x -x---- -x---x -x---- -x---- -x---x -x---x --xx-- -x---x -x-x-- -x---- -x---x -x--xx -x---x -x---- -x---- -x-x-- -----x ---x-- -x---x -xx-xx --x-x- ---x-- -xx-xx --x--- "
"-x---x -xxxxx -xxxxx -xxxx- -xxxxx -x---- -xxxxx -x---x --xx-- -xxxxx -x--x- -xxxx- -x---x -x---x --xxx- -x---- -xxxx- -x--xx -xxxxx ---x-- --xxx- ---x-- -x---x ---x-- -x---x -xxxxx "
"------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ "
};


static __inline__ void putpixel (int x, xcolnr c8)
{
	register uae_u16 *p = (uae_u16 *)xlinebuffer + x;
	*p = c8;
}

static _INLINE_ void write_tdnumber (int x, int y, int num)
{
    int j;
    uae_u8 *numptr;
    numptr = (uae_u8 *)(numbers + num * TD_NUM_WIDTH + 10 * TD_NUM_WIDTH * y);
    for (j = 0; j < TD_NUM_WIDTH; j++) {
		putpixel (x + j, *numptr == 'x' ? xcolors[0xfff] : xcolors[0x000]);
		numptr++;
    }
}

static _INLINE_ void write_tdletter (int x, int y, char ch)
{
    int j;
    uae_u8 *numptr;
	
    numptr = (uae_u8 *)(letters + (ch-65) * TD_NUM_WIDTH + 26 * TD_NUM_WIDTH * y);
  
	for (j = 0; j < TD_NUM_WIDTH; j++) {
	putpixel (x + j, *numptr == 'x' ? xcolors[0xfff] : xcolors[0x000]);
	numptr++;
    }
}

static _INLINE_ void draw_status_line (int line)
{
    int x, y, i, j, led, on;
    int on_rgb, off_rgb, c;
    uae_u8 *buf;
    int gfxvid_width;
    
    if(mainMenu_displayHires)
    	gfxvid_width = var_GFXVIDINFO_WIDTH * 2;
    else
    	gfxvid_width = var_GFXVIDINFO_WIDTH;
    	
    if (td_pos & TD_RIGHT)
        x = gfxvid_width - TD_PADX - 5*TD_WIDTH;
    else
        x = TD_PADX;

    y = line - (mainMenu_displayedLines - TD_TOTAL_HEIGHT);
    xlinebuffer = row_map[line];

	x+=100 - (TD_WIDTH*(mainMenu_drives-1)) - TD_WIDTH;

  uae4all_memclr (xlinebuffer + (x - 4) * GFXVIDINFO_PIXBYTES, (gfxvid_width - x + 4) * GFXVIDINFO_PIXBYTES);

	for (led = -1; led < (mainMenu_drives+1); led++) {
		int track;
		if (led > 0) {
			/* Floppy */
			track = gui_data.drive_track[led-1];
			on = gui_data.drive_motor[led-1];
			on_rgb = 0x0f0;
			off_rgb = 0x040;
		} else if (led < 0) {
			/* Power */
			track = -1;
			on = gui_data.powerled;
			on_rgb = 0xf00;
			off_rgb = 0x400;
		} else {
			/* Hard disk */
			track = -2;
			
			switch (gui_data.hdled) {
				case HDLED_OFF:
					on = 0;
					off_rgb = 0x004;
					break;
				case HDLED_READ:
					on = 1;
					on_rgb = 0x00f;
					off_rgb = 0x004;
					break;
				case HDLED_WRITE:
					on = 1;
					on_rgb = 0xf00;
					off_rgb = 0x400;
					break;
			}
		}
	c = xcolors[on ? on_rgb : off_rgb];

	for (j = 0; j < TD_LED_WIDTH; j++) 
	    putpixel (x + j, c);

	if (y >= TD_PADY && y - TD_PADY < TD_NUM_HEIGHT) {
	    if (track >= 0) {
		int offs = (TD_WIDTH - 2 * TD_NUM_WIDTH) / 2;
		write_tdnumber (x + offs, y - TD_PADY, track / 10);
		write_tdnumber (x + offs + TD_NUM_WIDTH, y - TD_PADY, track % 10);
	    }
	}
	x += TD_WIDTH;
    }

        x = gfxvid_width - TD_PADX - 5*TD_WIDTH;
	x+=100 - (TD_WIDTH*(mainMenu_drives-1)) - TD_WIDTH;
	if (y >= TD_PADY && y - TD_PADY < TD_NUM_HEIGHT) {
	    int offs = (TD_WIDTH - 2 * TD_NUM_WIDTH) / 2;
	    if(fps_counter >= 100)
	    	write_tdnumber (x + offs - TD_NUM_WIDTH, y - TD_PADY, fps_counter / 100);
	    write_tdnumber (x + offs, y - TD_PADY, (fps_counter / 10) % 10);
	    write_tdnumber (x + offs + TD_NUM_WIDTH, y - TD_PADY, fps_counter % 10);
		
		if (mainMenu_filesysUnits > 0) {
			write_tdletter(x + offs + TD_WIDTH, y - TD_PADY, 'H');
			write_tdletter(x + offs + TD_WIDTH + TD_NUM_WIDTH, y - TD_PADY, 'D');
		}
	}
}

void check_all_prefs(void)
{

	check_prefs_changed_audio ();
	check_prefs_changed_custom ();
	init_row_map ();
	init_aspect_maps ();
	notice_screen_contents_lost ();
	notice_new_xcolors ();
}


static _INLINE_ void finish_drawing_frame (void)
{
	int i;

	if (mainMenu_showStatus)
		fps_counter_upd();

	lockscr();

	if(mainMenu_displayHires)
	{
		pfield_do_linetoscr=(line_draw_func *)pfield_do_linetoscr_0_640;
		pfield_do_fill_line=(line_draw_func *)pfield_do_fill_line_0_640;
	}
	else
	{
		pfield_do_linetoscr=(line_draw_func *)pfield_do_linetoscr_0;
		pfield_do_fill_line=(line_draw_func *)pfield_do_fill_line_0;
	}

	for (i = 0; i < max_ypos_thisframe; i++) {
		int where,i1;
		int active_line = i + thisframe_y_adjust_real;

    if(active_line >= linestate_first_undecided)
			break;

		i1 = i + min_ypos_for_screen;
		where = amiga2aspect_line_map[i1];
		if (where >= mainMenu_displayedLines)
			break;
		if (where == -1)
			continue;

		pfield_draw_line (active_line, where);
	}

	if (mainMenu_showStatus)
	{
		/* HD LED off delay */
		static int countdown = HDLED_TIMEOUT;
		if (gui_data.hdled != HDLED_OFF) 
		{
			if (countdown-- <= 0) {
				gui_data.hdled = HDLED_OFF;
				countdown = HDLED_TIMEOUT;
			}
		} else {
			countdown = HDLED_TIMEOUT;
		}

		for (i = 0; i < TD_TOTAL_HEIGHT; i++) {
			int line = mainMenu_displayedLines - TD_TOTAL_HEIGHT + i;
			draw_status_line (line);
		}
	}
	drawfinished=1;
	do_flush_screen ();
}

void vsync_handle_redraw (int long_frame, int lof_changed)
{
	if(gfx_mem != (char *)prSDLScreen->pixels)
	{
		// These may have changed in vsync_handler
    init_row_map();
	}

	last_redraw_point++;
	count_frame ();
	if (lof_changed || ! interlace_seen || last_redraw_point >= 2 || long_frame) {
		last_redraw_point = 0;
		interlace_seen = 0;
      
		if (framecnt == 0)
			finish_drawing_frame ();
		
		/* At this point, we have finished both the hardware and the
		 * drawing frame. Essentially, we are outside of all loops and
		 * can do some things which would cause confusion if they were
		 * done at other times.
		 */
		if (savestate_state == STATE_DOSAVE)
		{
			custom_prepare_savestate ();
			savestate_state = STATE_SAVE;
			pause_sound();
			save_thumb(SCREENSHOT, screenshot_filename);
			save_state (savestate_filename, "Description!");
			resume_sound();
			gui_set_message("Saved", 50);
			savestate_state = 0;
		}
		else if (savestate_state == STATE_DORESTORE)
		{
			pause_sound();
			savestate_state = STATE_RESTORE;
			uae_reset ();
		}

		if (quit_program < 0) {
			quit_program = -quit_program;
			set_special (SPCFLAG_BRK);
#ifdef USE_FAME_CORE
			m68k_release_timeslice();
#endif
			filesys_prepare_reset (); 
			return;
		}

		framecnt = fs_framecnt;

		if (framecnt == 0)
			init_drawing_frame ();
	}
}

void hsync_record_line_state (int lineno)
{
  if (framecnt != 0)
  	return;

  linestate_first_undecided = lineno + 1;
}

void reset_drawing (void)
{
    max_diwstop = 0;

    linestate_first_undecided = 0;
    
    xlinebuffer = gfx_mem;

    init_aspect_maps ();

    init_row_map();

    last_redraw_point = 0;

    uae4all_memclr(spixels, sizeof spixels);
    uae4all_memclr(&spixstate, sizeof spixstate);

    init_drawing_frame ();
}

void drawing_init ()
{
    native2amiga_line_map = 0;
    amiga2aspect_line_map = 0;
    gen_pfield_tables();
}


void moveVertical(int value)
{
	moveY += value;
	if(moveY<-42)
		moveY=-42;
	else if(moveY>50)
		moveY=50;
}
