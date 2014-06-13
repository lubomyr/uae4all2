 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Custom chip emulation
  *
  * Copyright 1995-2002 Bernd Schmidt
  * Copyright 1995 Alessandro Bissacco
  * Copyright 2000-2002 Toni Wilen
  */

#define USE_IMASK_TABLE
#define STOP_WHEN_NASTY
#define SPR0_HPOS 0x15

#include "sysconfig.h"
#include "sysdeps.h"
#include <ctype.h>
#include <assert.h>
#include "config.h"
#include "uae.h"
#include "options.h"
#include "thread.h"
#include "gensound.h"
#include "sound.h"
#include "debug_uae4all.h"
#include "events.h"
#include "memory-uae.h"
#include "custom.h"
#include "m68k/m68k_intrf.h"
#include "cia.h"
#include "disk.h"
#include "blitter.h"
#include "xwin.h"
#include "joystick.h"
#include "audio.h"
#include "keybuf.h"
#include "serial.h"
#include "osemu.h"
#include "autoconf.h"
#include "gui.h"
#include "drawing.h"
#include "savestate.h"
#include "menu_config.h"

#ifdef STOP_WHEN_COPPER
static __inline__ void setcopper(void)
{
#ifdef USE_FAME_CORE
	m68k_release_timeslice();
#endif
	set_special (SPCFLAG_COPPER);
}
#else
#define setcopper() set_special(SPCFLAG_COPPER)
#endif

#ifdef STOP_WHEN_NASTY
static __inline__ void setnasty(void)
{
#ifdef USE_FAME_CORE
	m68k_release_timeslice();
#endif
	set_special (SPCFLAG_BLTNASTY);
}
#else
#define setnasty() set_special (SPCFLAG_BLTNASTY)
#endif

#if !defined(DEBUG_INTERRUPTS) && defined(USE_FAME_CORE) && defined(USE_IMASK_TABLE)
#include "imask_tab.h"
#endif

#ifdef USE_CUSTOM_EXTRA_INLINE
#define _INLINE_ __inline__
#else
#define _INLINE_ 
#endif

static uae_u16 last_custom_value;

/* Line buffers */
static uae_u32 todisplay[MAX_PLANES][8] UAE4ALL_ALIGN;
static uae_u32 outword[MAX_PLANES];
static uae_u32 fetched[MAX_PLANES];
static uae_u32 fetched_aga0[MAX_PLANES];
static uae_u32 fetched_aga1[MAX_PLANES];

/* Mouse and joystick emulation */
int buttonstate[3];
int mouse_x, mouse_y;
int joy0button, joy1button;
unsigned int joy0dir, joy1dir;
extern int mouseMoving;
extern int fcounter;


/* Events */
unsigned long int currcycle, nextevent;
struct ev eventtab[ev_max];

static int vpos;
static uae_u16 lof;
static int next_lineno;
static int lof_changed = 0;

static uae_u32 sprtaba[256],sprtabb[256];
static uae_u32 sprite_ab_merge[256];

/* Tables for collision detection.  */
static uae_u32 sprclx[16], clxmask[16];

/*
 * Hardware registers of all sorts.
 */
static void custom_wput_1 (int, uaecptr, uae_u32) REGPARAM;

uae_u16 intena,intreq;
uae_u16 dmacon;
uae_u16 adkcon; /* used by audio code */

static uae_u32 cop1lc,cop2lc,copcon;
 
int maxhpos = MAXHPOS_PAL;
int maxvpos = MAXVPOS_PAL;
int minfirstline = MINFIRSTLINE_PAL;
static int fmode;
unsigned int beamcon0 = -1, new_beamcon0;
#ifdef USE_UAE4ALL_VKBD
extern int vkbd_mode;
#endif

/* This is but an educated guess. It seems to be correct, but this stuff
 * isn't documented well. */
enum sprstate { SPR_restart, SPR_waiting_start, SPR_waiting_stop };

struct sprite {
    uaecptr pt;
    int xpos;
    int vstart;
    int vstop;
    int armed;
    enum sprstate state;
} UAE4ALL_ALIGN;

static struct sprite spr[MAX_SPRITES];

static int sprite_vblank_endline = 25;

static unsigned int sprpos[MAX_SPRITES] UAE4ALL_ALIGN;
static unsigned int sprctl[MAX_SPRITES] UAE4ALL_ALIGN;
static uae_u16 sprdata[MAX_SPRITES][4] UAE4ALL_ALIGN;
static uae_u16 sprdatb[MAX_SPRITES][4] UAE4ALL_ALIGN;
static int sprite_last_drawn_at[MAX_SPRITES] UAE4ALL_ALIGN;

static int last_sprite_point, nr_armed;
static int sprite_width, sprres, sprite_buffer_res;

static uae_u32 bpl1dat, bpl2dat, bpl3dat, bpl4dat, bpl5dat, bpl6dat, bpl7dat, bpl8dat;
static uae_s16 bpl1mod, bpl2mod;

typedef struct {
	uaecptr pt;
	int off;
} bpl_t;

static bpl_t bpl[8] UAE4ALL_ALIGN;

/* Used as a debugging aid, to offset any bitplane temporarily.  */
static struct color_entry current_colors;
static unsigned int bplcon0, bplcon1, bplcon2, bplcon3, bplcon4;
static unsigned int planes_bplcon0, res_bplcon0;
static unsigned int diwstrt, diwstop, diwhigh;
static int diwhigh_written;
static int ddfstrt, ddfstop;
static unsigned int ddfstrt_old_hpos, ddfstrt_old_vpos;
static int ddf_change;

/* The display and data fetch windows */
enum diw_states
{
    DIW_waiting_start, DIW_waiting_stop
};

static int plffirstline, plflastline;
static int plfstrt, plfstop;
static int last_diw_pix_hpos, last_decide_line_hpos;
static int last_fetch_hpos, last_sprite_hpos;
int diwfirstword, diwlastword;
static enum diw_states diwstate, hdiwstate, ddfstate;

/* Sprite collisions */
static unsigned int clxdat, clxcon, clxcon2, clxcon_bpl_enable, clxcon_bpl_match;
static int clx_sprmask;

enum copper_states {
    COP_stop,
    COP_read1_in2,
    COP_read1_wr_in4,
    COP_read1_wr_in2,
    COP_read1,
    COP_read2_wr_in2,
    COP_read2,
    COP_bltwait,
    COP_wait_in4,
    COP_wait_in2,
    COP_skip_in4,
    COP_skip_in2,
    COP_wait1,
    COP_wait
};

struct copper {
    /* The current instruction words.  */
    unsigned int i1, i2;
    unsigned int saved_i1, saved_i2;
    enum copper_states state;
    /* Instruction pointer.  */
    uaecptr ip, saved_ip;
    int hpos, vpos;
    unsigned int ignore_next;
    int vcmp, hcmp;

    /* When we schedule a copper event, knowing a few things about the future
       of the copper list can reduce the number of sync_with_cpu calls
       dramatically.  */
    unsigned int first_sync;
    unsigned int regtypes_modified;
} UAE4ALL_ALIGN;

#define REGTYPE_NONE 0
#define REGTYPE_COLOR 1
#define REGTYPE_SPRITE 2
#define REGTYPE_PLANE 4
#define REGTYPE_BLITTER 8
#define REGTYPE_JOYPORT 16
#define REGTYPE_DISK 32
#define REGTYPE_POS 64
#define REGTYPE_AUDIO 128

#define REGTYPE_ALL 255
/* Always set in regtypes_modified, to enable a forced update when things like
   DMACON, BPLCON0, COPJMPx get written.  */
#define REGTYPE_FORCE 256


static unsigned int regtypes[512];

static struct copper cop_state;
static int copper_enabled_thisline;


/* Recording of custom chip register changes.  */
struct sprite_entry *curr_sprite_entries;
struct color_change *curr_color_changes;
#define MAX_SPRITE_ENTRY (1024*6)
#define MAX_COLOR_CHANGE (1024*10)

struct decision line_decisions[2 * (MAXVPOS + 1) + 1];
struct draw_info curr_drawinfo[2 * (MAXVPOS + 1) + 1];
struct color_entry curr_color_tables[(MAXVPOS + 1) * 2];

static int next_sprite_entry = 0;
static int next_sprite_forced = 1;

static int next_color_change;
static int next_color_entry, remembered_color_entry;

static struct decision thisline_decision;
static int passed_plfstop, fetch_cycle, fetch_modulo_cycle;

enum fetchstate {
    fetch_not_started,
    fetch_started,
    fetch_was_plane0
} fetch_state;

/*
 * helper functions
 */

uae_u32 get_copper_address (int copno)
{
    switch (copno) {
    case 1: return cop1lc;
    case 2: return cop2lc;
    default: return 0;
    }
}

void check_prefs_changed_custom (void)
{
    prefs_gfx_framerate = changed_gfx_framerate;
}

static __inline__ void setclr (uae_u16 *_GCCRES_ p, uae_u16 val)
{
    if (val & 0x8000)
	*p |= val & 0x7FFF;
    else
	*p &= ~val;
}

#define current_hpos() ((get_cycles () - eventtab[ev_hsync].oldcycles) / CYCLE_UNIT)

static __inline__ uae_u8 *_GCCRES_ pfield_xlateptr (uaecptr plpt, int bytecount)
{
	if (!chipmem_bank.check (plpt, bytecount)) {
		static int count = 0;
		if (!count)
		{
			count++;
			write_log ("Warning: Bad playfield pointer\n");
		}
		return NULL;
	}
	return chipmem_bank.xlateaddr (plpt);
}

static __inline__ void docols (struct color_entry *_GCCRES_ colentry)
{
	int i;
	
	if (currprefs.chipset_mask & CSMASK_AGA) {
		for (i = 0; i < 256; i++) {
			int v = color_reg_get (colentry, i);
			if ((v < 0) || (v > 16777215))
				continue;
			colentry->acolors[i] = CONVERT_RGB (v);
		}
	} else {
		for (i = 0; i < 32; i++) {
			int v = color_reg_get (colentry, i);
			if ((v < 0) || (v > 4095))
				continue;
			colentry->acolors[i] = xcolors[v];
		}
	}
}

void notice_new_xcolors (void)
{
	int i;
	
	docols(&current_colors);
	for (i = 0; i < (MAXVPOS + 1)*2; i++) {
		docols(curr_color_tables + i);
	}
}

static _INLINE_ void do_sprites (int currhp);

static _INLINE_ void remember_ctable (void)
{
	if (remembered_color_entry == -1) {
		/* The colors changed since we last recorded a color map. Record a
		 * new one. */
		color_reg_cpy (curr_color_tables + next_color_entry, &current_colors);
		remembered_color_entry = next_color_entry++;
	}
	thisline_decision.ctable = remembered_color_entry;
}

/* Called to determine the state of the horizontal display window state
 * machine at the current position. It might have changed since we last
 * checked.  */
static _INLINE_ void decide_diw (int hpos)
{
	int pix_hpos = coord_diw_to_window_x (hpos << 1);
	if ((hdiwstate == DIW_waiting_start) && (pix_hpos >= diwfirstword) && (last_diw_pix_hpos < diwfirstword))
	{
		if(thisline_decision.diwfirstword == -1)
		{
			thisline_decision.diwfirstword = diwfirstword < 0 ? 0 : diwfirstword;
			thisline_decision.diwlastword = -1;
		}
		hdiwstate = DIW_waiting_stop;
	}
	if ((hdiwstate == DIW_waiting_stop)	&& (pix_hpos >= diwlastword) && (last_diw_pix_hpos < diwlastword))
	{
		if(thisline_decision.diwlastword == -1)
			thisline_decision.diwlastword = diwlastword < 0 ? 0 : diwlastword;
		hdiwstate = DIW_waiting_start;
	}
	last_diw_pix_hpos = pix_hpos;
}

static int fetchmode;
static int maxplanes_ocs[]={ 6,4,0,0 };
static int maxplanes_ecs[]={ 6,4,2,0 };
static int maxplanes_aga[]={ 8,4,2,0, 8,8,4,0, 8,8,8,0 };


/* The HRM says 0xD8, but that can't work... */
#define HARD_DDF_STOP (0xD4)
#define HARD_DDF_START 0x18

static __inline__ void add_modulos()
{
  if (dmaen (DMA_BITPLANE))
  {
    register int m1, m2;
   
    if (fmode & 0x4000) {
      if (((diwstrt >> 8) ^ vpos) & 1)
        m1 = m2 = bpl2mod;
      else
        m1 = m2 = bpl1mod;
    } else {
      m1 = bpl1mod;
      m2 = bpl2mod;
    }
   
    switch (planes_bplcon0) {
      case 8: bpl[7].pt += m2;
      case 7: bpl[6].pt += m1;
      case 6: bpl[5].pt += m2;
      case 5: bpl[4].pt += m1;
      case 4: bpl[3].pt += m2;
      case 3: bpl[2].pt += m1;
      case 2: bpl[1].pt += m2;
      case 1: bpl[0].pt += m1;
    }
  }
}

static __inline__ void finish_playfield_line()
{
/*    add_modulos();*/

    /* These are for comparison. */
    thisline_decision.bplcon0 = bplcon0;
    thisline_decision.bplcon2 = bplcon2;
    thisline_decision.bplcon3 = bplcon3;
    thisline_decision.bplcon4 = bplcon4;
}

/* The fetch unit mainly controls ddf stop.  It's the number of cycles that
   are contained in an indivisible block during which ddf is active.  E.g.
   if DDF starts at 0x30, and fetchunit is 8, then possible DDF stops are
   0x30 + n * 8.  */
static int fetchunit, fetchunit_mask;
/* The delay before fetching the same bitplane again.  Can be larger than
   the number of bitplanes; in that case there are additional empty cycles
   with no data fetch (this happens for high fetchmodes and low
   resolutions).  */
static int fetchstart, fetchstart_mask;
/* fm_maxplane holds the maximum number of planes possible with the current
   fetch mode.  This selects the cycle diagram:
   8 planes: 73516240
   4 planes: 3120
   2 planes: 10.  */
static int fm_maxplane;

/* The corresponding values, by fetchmode and display resolution.  */
static const int fetchunits[] = { 8,8,8,0, 16,8,8,0, 32,16,8,0 };
static const int fetchstarts[] = { 3,2,1,0, 4,3,2,0, 5,4,3,0 };
static const int fm_maxplanes[] = { 3,2,1,0, 3,3,2,0, 3,3,3,0 };

static int cycle_diagram_table[3][3][9][32];
static int *curr_diagram;
static int cycle_sequences[3 * 8] = { 2,1,2,1,2,1,2,1, 4,2,3,1,4,2,3,1, 8,4,6,2,7,3,5,1 };

static _INLINE_ void create_cycle_diagram_table(void)
{
   int fm, res, cycle, planes, v;
   int fetch_start, max_planes;
   int *cycle_sequence;
   
   for (fm = 0; fm <= 2; fm++) {
      for (res = 0; res <= 2; res++) {
         max_planes = fm_maxplanes[fm * 4 + res];
         fetch_start = 1 << fetchstarts[fm * 4 + res];
         cycle_sequence = &cycle_sequences[(max_planes - 1) * 8];
         max_planes = 1 << max_planes;
         for (planes = 0; planes <= 8; planes++) {
            for (cycle = 0; cycle < 32; cycle++)
               cycle_diagram_table[fm][res][planes][cycle] = -1;
            if (planes <= max_planes) {
               for (cycle = 0; cycle < fetch_start; cycle++) {
                  if ((cycle < max_planes) && (planes >= cycle_sequence[cycle & 7])) {
                     v = cycle_sequence[cycle & 7];
                  } else {
                     v = 0;
                  }
                  cycle_diagram_table[fm][res][planes][cycle] = v;
               }
            }
         }
      }
   }
}


/* Used by the copper.  */
static int estimated_last_fetch_cycle;
static int cycle_diagram_shift;

static _INLINE_ void estimate_last_fetch_cycle (int hpos)
{
	if (! passed_plfstop) {
		int stop = (plfstop < hpos) || (plfstop > HARD_DDF_STOP) ? HARD_DDF_STOP : plfstop;
		/* We know that fetching is up-to-date up until hpos, so we can use fetch_cycle.  */
		int fetch_cycle_at_stop = fetch_cycle + (stop - hpos);
		int starting_last_block_at = (fetch_cycle_at_stop + fetchunit - 1) & ~fetchunit_mask;
		
		estimated_last_fetch_cycle = hpos + (starting_last_block_at - fetch_cycle) + fetchunit;
	} else {
		int starting_last_block_at = (fetch_cycle + fetchunit - 1) & ~fetchunit_mask;
		if (passed_plfstop == 2)
			starting_last_block_at -= fetchunit;
		
		estimated_last_fetch_cycle = hpos + (starting_last_block_at - fetch_cycle) + fetchunit;
	}
}

static int out_nbits, out_offs;

/* Expansions from bplcon0/bplcon1.  */
static int toscr_res;
static int toscr_res2;
static int toscr_delay[2], toscr_nr_planes;
/* WinUAE code */
static int toscr_delay1x, toscr_delay2x;
/***************/

/* The number of bits left from the last fetched words.  
   This is an optimization - conceptually, we have to make sure the result is
   the same as if toscr is called in each clock cycle.  However, to speed this
   up, we accumulate display data; this variable keeps track of how much. 
   Thus, once we do call toscr_nbits (which happens at least every 16 bits),
   we can do more work at once.  */
static int toscr_nbits;

static int delayoffset;

/* WinUAE+UAE4ALL code */
#define compute_delay_offset(HPOS) \
{ \
    delayoffset = (16 << fetchmode) - (((plfstrt - HARD_DDF_START) & fetchstart_mask) << 1); \
}

static _INLINE_ void expand_fmodes (void)
{
//    int res = res_bplcon0;
    int fm_index = fetchmode * 4 + res_bplcon0;
    fetchunit = fetchunits[fm_index];
    fetchunit_mask = fetchunit - 1;
    int fetchstart_shift = fetchstarts[fm_index];
    fetchstart = 1 << fetchstart_shift;
    fetchstart_mask = fetchstart - 1;
    int fm_maxplane_shift = fm_maxplanes[fm_index];
    fm_maxplane = 1 << fm_maxplane_shift;
    fetch_modulo_cycle = fetchunit - fetchstart;
}

/* Expand bplcon0/bplcon1 into the toscr_xxx variables.  */
static __inline__ void compute_toscr_delay_1 ()
{
    int delay1 = (bplcon1 & 0x0f) | ((bplcon1 & 0x0c00) >> 6);
    int delay2 = ((bplcon1 >> 4) & 0x0f) | (((bplcon1 >> 4) & 0x0c00) >> 6);
    int shdelay1 = (bplcon1 >> 12) & 3;
    int shdelay2 = (bplcon1 >> 8) & 3;
    int delaymask;
    int fetchwidth = 16 << fetchmode;
    
    delay1 += delayoffset;
    delay2 += delayoffset;
    delaymask = (fetchwidth - 1) >> toscr_res;
    toscr_delay1x = (delay1 & delaymask) << toscr_res;
    toscr_delay1x |= shdelay1 >> (2 - toscr_res);
    toscr_delay2x = (delay2 & delaymask) << toscr_res;
    toscr_delay2x |= shdelay2 >> (2 - toscr_res);
}

STATIC_INLINE void compute_toscr_delay ()
{
   toscr_res = res_bplcon0;
   toscr_nr_planes = planes_bplcon0;
   toscr_res2 = 2 << toscr_res;
   compute_toscr_delay_1 ();
}

#define maybe_first_bpl1dat(HPOS) \
{ \
    if (thisline_decision.plfleft == -1) { \
    	thisline_decision.plfleft = HPOS; \
    	compute_delay_offset (HPOS); \
    	compute_toscr_delay_1 (); \
    } \
}

STATIC_INLINE void update_toscr_planes (void)
{
  if (toscr_nr_planes > thisline_decision.nr_planes)
    thisline_decision.nr_planes = toscr_nr_planes;
}

STATIC_INLINE void toscr_3_ecs (int nbits)
{
	register int delay1 = toscr_delay[0];
	register int delay2 = toscr_delay[1];
	register int i;
	register uae_u32 mask = 0xFFFF >> (16 - nbits);
	
	for (i = 0; i < toscr_nr_planes; i += 2) {
		outword[i] <<= nbits;
		outword[i] |= (todisplay[i][4] >> (16 - nbits + delay1)) & mask;
		todisplay[i][4] <<= nbits;
	}
	for (i = 1; i < toscr_nr_planes; i += 2) {
		outword[i] <<= nbits;
		outword[i] |= (todisplay[i][4] >> (16 - nbits + delay2)) & mask;
		todisplay[i][4] <<= nbits;
	}
}


static __inline__ void shift32plus (uae_u32 *p, int n)
{
	uae_u32 t = p[1];
	t <<= n;
	t |= p[0] >> (32 - n);
	p[1] = t;
}

static __inline__ void aga_shift (uae_u32 *p, int n, int fm)
{
	if (fm == 2) {
		shift32plus (p + 2, n);
		shift32plus (p + 1, n);
	}
	shift32plus (p + 0, n);
	p[0] <<= n;
}


STATIC_INLINE void toscr_3_aga (int nbits, int fm)
{
	int delay1 = toscr_delay[0];
	int delay2 = toscr_delay[1];
	int i;
	uae_u32 mask = 0xFFFF >> (16 - nbits);

	{
		int offs = (16 << fm) - nbits + delay1;
		int off1 = offs >> 5;
		if (off1 == 3)
			off1 = 2;
		offs -= off1 * 32;
		for (i = 0; i < toscr_nr_planes; i += 2) {
			uae_u32 t0 = todisplay[i][off1 + 4];
			uae_u32 t1 = todisplay[i][off1 + 5];
			uae_u64 t = (((uae_u64)t1) << 32) | t0;
			outword[i] <<= nbits;
			outword[i] |= (t >> offs) & mask;
			aga_shift (todisplay[i] + 4, nbits, fm);
		}
	}
	{
		int offs = (16 << fm) - nbits + delay2;
		int off1 = offs >> 5;
		if (off1 == 3)
			off1 = 2;
		offs -= off1 * 32;
		for (i = 1; i < toscr_nr_planes; i += 2) {
			uae_u32 t0 = todisplay[i][off1 + 4];
			uae_u32 t1 = todisplay[i][off1 + 5];
			uae_u64 t = (((uae_u64)t1) << 32) | t0;
			outword[i] <<= nbits;
			outword[i] |= (t >> offs) & mask;
			aga_shift (todisplay[i] + 4, nbits, fm);
		}
	}
}

STATIC_INLINE void toscr_1 (int nbits, int fm)
{
   if (fm == 0)
      toscr_3_ecs (nbits);
   else
      toscr_3_aga (nbits, fm);

	out_nbits += nbits;
   if (out_nbits == 32) {
      int i;
      register uae_u32 *dataptr32_start = (uae_u32 *)(line_data[next_lineno]);
      register uae_u32 *dataptr32 = dataptr32_start + out_offs;
      
      /* Don't use toscr_nr_planes here; if the plane count drops during the
       * line we still want the data to be correct for the full number of planes
       * over the full width of the line.
       */
      for (i = 0; i < thisline_decision.nr_planes; i++) {
         *dataptr32 = outword[i];
         dataptr32 += MAX_WORDS_PER_LINE >> 1;
      }
      out_offs++;
      out_nbits = 0;
   }
}

static __inline__ void toscr_0 (int nbits, int fm)
{
	int t;

	if (nbits > 16) {
		toscr_0 (16, fm);
		nbits -= 16;
	}

	t = 32 - out_nbits;
	if (t < nbits) {
		toscr_1 (t, fm);
		nbits -= t;
	}
	toscr_1 (nbits, fm);
}

#define flush_display(FM) \
{ \
   if ((toscr_nbits > 0) && (thisline_decision.plfleft != -1)) \
      toscr_0 (toscr_nbits, FM); \
   toscr_nbits = 0; \
} 

/* Called when all planes have been fetched, i.e. when a new block
   of data is available to be displayed.  The data in fetched[] is
   moved into todisplay[].  */
STATIC_INLINE void beginning_of_plane_block (int pos, int fm)
{
   flush_display (fm);

   if (fm == 0) {
      todisplay[0][4] |= fetched[0];
      todisplay[1][4] |= fetched[1];
      todisplay[2][4] |= fetched[2];
      todisplay[3][4] |= fetched[3];
      todisplay[4][4] |= fetched[4];
      todisplay[5][4] |= fetched[5];
      todisplay[6][4] |= fetched[6];
      todisplay[7][4] |= fetched[7];
   }
   else {
         if (fm == 2) {
            todisplay[0][5] = fetched_aga1[0];
            todisplay[1][5] = fetched_aga1[1];
            todisplay[2][5] = fetched_aga1[2];
            todisplay[3][5] = fetched_aga1[3];
            todisplay[4][5] = fetched_aga1[4];
            todisplay[5][5] = fetched_aga1[5];
            todisplay[6][5] = fetched_aga1[6];
            todisplay[7][5] = fetched_aga1[7];
         }
         todisplay[0][4] = fetched_aga0[0];
         todisplay[1][4] = fetched_aga0[1];
         todisplay[2][4] = fetched_aga0[2];
         todisplay[3][4] = fetched_aga0[3];
         todisplay[4][4] = fetched_aga0[4];
         todisplay[5][4] = fetched_aga0[5];
         todisplay[6][4] = fetched_aga0[6];
         todisplay[7][4] = fetched_aga0[7];
	}

   maybe_first_bpl1dat (pos);
   toscr_delay[0] = toscr_delay1x;
   toscr_delay[1] = toscr_delay2x;
}


#define long_fetch_ecs_init(PLANE, NWORDS, DMA) \
    uae_u16 *real_pt = (uae_u16 *)pfield_xlateptr (bpl[PLANE].pt + bpl[PLANE].off, NWORDS << 1); \
    int delay = toscr_delay[(PLANE & 1)]; \
    int tmp_nbits = out_nbits; \
    uae_u32 shiftbuffer = todisplay[PLANE][4]; \
    uae_u32 outval = outword[PLANE]; \
    uae_u32 fetchval = fetched[PLANE]; \
    uae_u32 *dataptr_start = (uae_u32 *)(line_data[next_lineno] + ((PLANE<<1)*MAX_WORDS_PER_LINE)); \
    register uae_u32 *dataptr = dataptr_start + out_offs; \
    if (DMA) \
		bpl[PLANE].pt += NWORDS << 1; \
    if (real_pt == 0) \
	    return; \
    while (NWORDS > 0) { \
		int bits_left = 32 - tmp_nbits; \
		uae_u32 t; \
		shiftbuffer |= fetchval; \
		t = (shiftbuffer >> delay) & 0xFFFF;

#define long_fetch_ecs_weird() \
		if (bits_left < 16) { \
			outval <<= bits_left; \
			outval |= t >> (16 - bits_left); \
      *dataptr++ = outval; \
      outval = t; \
			tmp_nbits = 16 - bits_left; \
			shiftbuffer <<= 16; \
		} else

#define long_fetch_ecs_end(PLANE,NWORDS, DMA) \
		{ \
			outval = (outval << 16) | t; \
			shiftbuffer <<= 16; \
			tmp_nbits += 16; \
			if (tmp_nbits == 32) { \
        *dataptr++ = outval; \
				tmp_nbits = 0; \
			} \
		} \
		NWORDS--; \
		if (DMA) { \
			fetchval = do_get_mem_word (real_pt); \
			real_pt++; \
		} \
	} \
    fetched[PLANE] = fetchval; \
    todisplay[PLANE][4] = shiftbuffer; \
    outword[PLANE] = outval;

static __inline__ void long_fetch_ecs_0(int plane, int nwords, int dma)
{
	long_fetch_ecs_init(plane, nwords, dma)
	long_fetch_ecs_end(plane, nwords, dma)
}

static __inline__ void long_fetch_ecs_1(int plane, int nwords, int dma)
{
	long_fetch_ecs_init(plane, nwords, dma)
	long_fetch_ecs_weird()
	long_fetch_ecs_end(plane, nwords, dma)
}


STATIC_INLINE void long_fetch_aga (int plane, int nwords, int weird_number_of_bits, int fm, int dma)
{
	uae_u32 *real_pt = (uae_u32 *)pfield_xlateptr (bpl[plane].pt + bpl[plane].off, nwords * 2);
	int tmp_nbits = out_nbits;
	uae_u32 outval = outword[plane];
	uae_u32 fetchval0 = fetched_aga0[plane];
	uae_u32 fetchval1 = fetched_aga1[plane];
   uae_u32 *dataptr_start = (uae_u32 *)(line_data[next_lineno] + (plane<<1)*MAX_WORDS_PER_LINE);
   uae_u32 *dataptr = dataptr_start + out_offs;

  int offs = (16 << fm) - 16 + toscr_delay[plane & 1];
	int off1 = offs >> 5;
	if (off1 == 3)
		off1 = 2;
	offs -= off1 << 5;

	if (dma)
		bpl[plane].pt += nwords << 1;

	if (real_pt == 0)
		/* @@@ Don't do this, fall back on chipmem_wget instead.  */
		return;

  // Instead of shifting a 64 bit value more than 16 bits, we
  // move the pointer for x bytes and shift a 32 bit value less
  // than 16 bits. See (1)
  int buffer_add = (offs >> 4);
  offs &= 15;

	while (nwords > 0) {
		int i;
  	uae_u32 *shiftbuffer = todisplay[plane] + 4;

		shiftbuffer[0] = fetchval0;
		if (fm == 2)
			shiftbuffer[1] = fetchval1;

    // (1)
    if(buffer_add)
      shiftbuffer = (uae_u32 *)((uae_u16 *)shiftbuffer + buffer_add);      
      
		for (i = 0; i < (1 << fm); i++) {
			int bits_left = 32 - tmp_nbits;

			uae_u32 t0 = shiftbuffer[off1];
			t0 = (t0 >> offs) & 0xFFFF;
      
			if (weird_number_of_bits && (bits_left < 16)) {
				outval <<= bits_left;
				outval |= t0 >> (16 - bits_left);
        *dataptr++ = outval;
				outval = t0;
				tmp_nbits = 16 - bits_left;
				// Instead of shifting 128 bit of data for 16 bit,
				// we move the pointer two bytes. See also (2) and (3)
				//aga_shift (shiftbuffer, 16, fm);
				shiftbuffer = (uae_u32 *)((uae_u16 *)shiftbuffer - 1);
			} else {
				outval = (outval << 16) | t0;
				// (2)
				//aga_shift (shiftbuffer, 16, fm);
				shiftbuffer = (uae_u32 *)((uae_u16 *)shiftbuffer - 1);
				tmp_nbits += 16;
				if (tmp_nbits == 32) {
               *dataptr++ = outval;
               tmp_nbits = 0;
				}
			}
		}

    // (3)
    // We have to move the data, but now, we can simply
    // copy long values and have to do it only once.
    if(fm == 2)
    {
      todisplay[plane][7] = todisplay[plane][5];
      todisplay[plane][6] = todisplay[plane][4];
    }
    else
    {
      todisplay[plane][5] = todisplay[plane][4];
    }
    
		nwords -= 1 << fm;

		if (dma) {
#ifndef USE_ARMNEON
			if (fm == 1)
				fetchval0 = do_get_mem_long (real_pt);
			else {
				fetchval1 = do_get_mem_long (real_pt);
				fetchval0 = do_get_mem_long (real_pt + 1);
			}
#else
			if (fm == 1)
			  __asm__ __volatile__ (
			    "ldr     %[val0], [%[pt]]   \n\t"
			    "ror     %[val0], %[val0], #16   \n\t"
			    : [val0] "=r" (fetchval0) : [pt] "r" (real_pt) );
			else {
			  __asm__ __volatile__ (
			    "ldr     %[val1], [%[pt]]   \n\t"
			    "ror     %[val1], %[val1], #16   \n\t"
			    "ldr     %[val0], [%[pt], #4]   \n\t"
			    "ror     %[val0], %[val0], #16   \n\t"
			    : [val0] "=r" (fetchval0), [val1] "=r" (fetchval1) : [pt] "r" (real_pt) );
			}
#endif
			real_pt += fm;
		}
	}
	fetched_aga0[plane] = fetchval0;
	fetched_aga1[plane] = fetchval1;
	outword[plane] = outval;
}

#define long_fetch_aga_1_0(hpos, nwords, dma) { long_fetch_aga (hpos, nwords,  0, 1, dma); }
#define long_fetch_aga_1_1(hpos, nwords, dma) { long_fetch_aga (hpos, nwords,  1, 1, dma); }
#define long_fetch_aga_2_0(hpos, nwords, dma) { long_fetch_aga (hpos, nwords,  0, 2, dma); }
#define long_fetch_aga_2_1(hpos, nwords, dma) { long_fetch_aga (hpos, nwords,  1, 2, dma); }

STATIC_INLINE void do_long_fetch (int nwords, int dma, int fm)
{
	int i;

	flush_display (fm);
	switch (fm) {
		case 0:
			if (out_nbits & 15) {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_ecs_1 (i, nwords, dma);
			} else {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_ecs_0 (i, nwords, dma);
			}
			break;
		case 1:
			if (out_nbits & 15) {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_aga_1_1 (i, nwords, dma);
			} else {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_aga_1_0 (i, nwords, dma);
			}
			break;
		case 2:
			if (out_nbits & 15) {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_aga_2_1 (i, nwords, dma);
			} else {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_aga_2_0 (i, nwords, dma);
			}
			break;
	}

	out_nbits += nwords * 16;
	out_offs += out_nbits >> 5;
	out_nbits &= 31;

	if (dma && (toscr_nr_planes > 0))
		fetch_state = fetch_was_plane0;
}


STATIC_INLINE int flush_plane_data (int fm)
{
	int i = 0;
	int fetchwidth = 16 << fm;

	if (out_nbits <= 16) {
		i += 16;
		toscr_1 (16, fm);
	}
	if (out_nbits != 0) {
		i += 32 - out_nbits;
		toscr_1 (32 - out_nbits, fm);
	}
	i += 32;
	toscr_1 (16, fm);
	toscr_1 (16, fm);
	return i >> (1 + toscr_res);
}

/* make sure fetch that goes beyond maxhpos is finished */
STATIC_INLINE void finish_final_fetch (int i, int fm)
{
   if ((thisline_decision.plfleft == -1) || 
            (passed_plfstop == 3))
      return;

   passed_plfstop = 3;

      ddfstate = DIW_waiting_start;
      i += flush_plane_data (fm);
      thisline_decision.plfright = i;
      thisline_decision.plflinelen = out_offs;
      finish_playfield_line ();
}

static void fetch(int nr, int FM)
{
   if (nr < toscr_nr_planes)
   {
      register uaecptr p = (bpl[nr].pt + bpl[nr].off)/*&0x000FFFFF*/;
      switch (FM) {
         case 0:
            fetched[nr] = CHIPMEM_WGET_CUSTOM (p);
            bpl[nr].pt += 2;
            break;
         case 1:
            fetched_aga0[nr] = CHIPMEM_LGET_CUSTOM (p);
            bpl[nr].pt += 4;
            break;
         case 2:
            fetched_aga1[nr] = CHIPMEM_LGET_CUSTOM (p);
            fetched_aga0[nr] = CHIPMEM_LGET_CUSTOM (p + 4);
            bpl[nr].pt += 8;
            break;
      }
      if (passed_plfstop == 2 && fetch_cycle >= (fetch_cycle & ~fetchunit_mask) + fetch_modulo_cycle) {
         int mod;
         if (fmode & 0x4000) {
            if (((diwstrt >> 8) ^ vpos) & 1)
               mod = bpl2mod;
            else
               mod = bpl1mod;
         } else if (nr & 1)
            mod = bpl2mod;
         else
            mod = bpl1mod;
         bpl[nr].pt += mod;
      }
      if (!nr)
         fetch_state = fetch_was_plane0;
   }
}

static __inline__ int ONE_FETCH(int POS, int DDFSTOP, int DMA, int FM)
{
	if (! passed_plfstop && (POS == DDFSTOP))
		passed_plfstop = 1;
	if (!(fetch_cycle & fetchunit_mask)) {
		if (passed_plfstop == 2) {
			finish_final_fetch (POS, FM);
			return 1;
		}
		if (passed_plfstop)
			passed_plfstop++;
	}
	if (DMA) {
	/* fetchstart_mask can be larger than fm_maxplane if FMODE > 0.  This means
	   that the remaining cycles are idle; we'll fall through the whole switch
	   without doing anything.  */
		int cycle_start = fetch_cycle & fetchstart_mask;
		switch (fm_maxplane) {
			case 8:
				switch (cycle_start) {
					case 0: fetch (7, FM); break;
					case 1: fetch (3, FM); break;
					case 2: fetch (5, FM); break;
					case 3: fetch (1, FM); break;
					case 4: fetch (6, FM); break;
					case 5: fetch (2, FM); break;
					case 6: fetch (4, FM); break;
					case 7: fetch (0, FM); break;
				}
				break;
			case 4:
				switch (cycle_start) {
					case 0: fetch (3, FM); break;
					case 1: fetch (1, FM); break;
					case 2: fetch (2, FM); break;
					case 3: fetch (0, FM); break;
				}
				break;
			case 2:
				switch (cycle_start) {
					case 0: fetch (1, FM); break;
					case 1: fetch (0, FM); break;
				}
				break;
		}
	}

	fetch_cycle++;
	toscr_nbits += toscr_res2;
	if (toscr_nbits == 16)
		flush_display (FM);
	
	return 0;
}

static void update_fetch (int until, int fm)
{
	int pos;
	int dma = dmaen (DMA_BITPLANE);
	int ddfstop_to_test;

	if ((framecnt != 0) || (passed_plfstop == 3))
		return;

    /* We need an explicit test against HARD_DDF_STOP here to guard against
       programs that move the DDFSTOP before our current position before we
       reach it.  */
	ddfstop_to_test = HARD_DDF_STOP;
	if ((ddfstop >= last_fetch_hpos) && (plfstop < HARD_DDF_STOP))
		ddfstop_to_test = plfstop;

	compute_toscr_delay();
	update_toscr_planes();

	pos = last_fetch_hpos;
	cycle_diagram_shift = (last_fetch_hpos - fetch_cycle) & fetchstart_mask;

	/* First, a loop that prepares us for the speedup code.  We want to enter
       the SPEEDUP case with fetch_state == fetch_was_plane0, and then unroll
       whole blocks, so that we end on the same fetch_state again.  */
	for (; ; pos++) {
		if (pos == until) {
			if ((until >= maxhpos) && (passed_plfstop == 2)) {
				finish_final_fetch (pos, fm);
				return;
			}
			flush_display (fm);
			return;
		}

		if (fetch_state == fetch_was_plane0)
			break;

		fetch_state = fetch_started;
		if (ONE_FETCH(pos, ddfstop_to_test, dma, fm))
		   return;
	}

	if (! passed_plfstop && (ddf_change != vpos) && (ddf_change + 1 != vpos)
			&& dma
			&& ((fetch_cycle & fetchstart_mask) == (fm_maxplane & fetchstart_mask))
			&& ((toscr_delay[0] == toscr_delay1x) && (toscr_delay[1] == toscr_delay2x))
			&& (toscr_nr_planes == thisline_decision.nr_planes))
	{
		int offs = (pos - fetch_cycle) & fetchunit_mask;
		int ddf2 = ((ddfstop_to_test - offs + fetchunit - 1) & ~fetchunit_mask) + offs;
		int ddf3 = ddf2 + fetchunit;
		int stop = until < ddf2 ? until : until < ddf3 ? ddf2 : ddf3;
		int count;

		count = stop - pos;

		if (count >= fetchstart) {
			count &= ~fetchstart_mask;

			if (thisline_decision.plfleft == -1) {
				compute_delay_offset (pos);
				compute_toscr_delay_1 ();
			}
			do_long_fetch (count >> (3 - toscr_res), dma, fm);

	    /* This must come _after_ do_long_fetch so as not to confuse flush_display
	       into thinking the first fetch has produced any output worth emitting to
	       the screen.  But the calculation of delay_offset must happen _before_.  */
			maybe_first_bpl1dat (pos);

			if ((pos <= ddfstop_to_test) && (pos + count > ddfstop_to_test))
				passed_plfstop = 1;
			if ((pos <= ddfstop_to_test) && (pos + count > ddf2))
				passed_plfstop = 2;
			if ((pos <= ddf2) && (pos + count >= ddf2 + fm_maxplane))
			   add_modulos ();
			pos += count;
			fetch_cycle += count;
		}
	}

	for (; pos < until; pos++) {
		if (fetch_state == fetch_was_plane0)
			beginning_of_plane_block (pos, fm);
		fetch_state = fetch_started;
		if (ONE_FETCH(pos, ddfstop_to_test, dma, fm))
		   return;
	}

	if ((until >= maxhpos) && (passed_plfstop == 2)) {
		finish_final_fetch (pos, fm);
		return;
	}
	flush_display (fm);
}

static __inline__ void decide_fetch (int hpos)
{
	if ((fetch_state != fetch_not_started) && (hpos > last_fetch_hpos))
	{
      update_fetch (hpos, fetchmode);
	}
	last_fetch_hpos = hpos;
}

static __inline__ void start_bpl_dma (int hstart)
{
   fetch_state = fetch_started;
   fetch_cycle = 0;
   last_fetch_hpos = hstart;
   out_nbits = 0;
   out_offs = 0;
   toscr_nbits = 0;
   thisline_decision.bplres = res_bplcon0;

   ddfstate = DIW_waiting_stop;
   compute_toscr_delay ();

   /* If someone already wrote BPL1DAT, clear the area between that point and
       the real fetch start.  */
   if (framecnt == 0) {
      if (thisline_decision.plfleft != -1) {
         out_nbits = (plfstrt - thisline_decision.plfleft) << (1 + toscr_res);
         out_offs = out_nbits >> 5;
         out_nbits &= 31;
      }
      update_toscr_planes ();
   }
}

/* this may turn on datafetch if program turns dma on during the ddf */
static __inline__ void maybe_start_bpl_dma (int hpos)
{
   /* OCS: BPL DMA never restarts if DMA is turned on during DDF
    * ECS/AGA: BPL DMA restarts but only if DMA was turned off
       outside of DDF or during current line, otherwise display
       processing jumps immediately to "DDFSTOP passed"-condition */
   if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
      return;
   if (fetch_state != fetch_not_started)
      return;
   if (diwstate != DIW_waiting_stop)
      return;
   if (hpos <= plfstrt)
      return;
   if (hpos > plfstop - fetchunit)
      return;
   if (ddfstate != DIW_waiting_start)
      passed_plfstop = 1;
   start_bpl_dma (hpos);
}

/* This function is responsible for turning on datafetch if necessary.  */
STATIC_INLINE void decide_line (int hpos)
{
	/* First, take care of the vertical DIW.  Surprisingly enough, this seems to be
	   correct here - putting this into decide_diw() results in garbage.  */
	if (vpos == plffirstline) {
		diwstate = DIW_waiting_stop;
		ddf_change = vpos;
	}
	if (vpos == plflastline) {
		diwstate = DIW_waiting_start;
		ddf_change = vpos;
	}
   
   if (hpos <= last_decide_line_hpos)
      return;
   if (fetch_state != fetch_not_started)
      return;

   /* Test if we passed the start of the DDF window.  */
    if (last_decide_line_hpos < plfstrt && hpos >= plfstrt) {
    	/* If DMA isn't on by the time we reach plfstrt, then there's no
	       bitplane DMA at all for the whole line.  */
       if (dmaen (DMA_BITPLANE) && diwstate == DIW_waiting_stop) {
         /* hack warning.. Writing to DDFSTRT when DMA should start must be ignored
          * (correct fix would be emulate this delay for every custom register, but why bother..) */
         if ((hpos - 2 != ddfstrt_old_hpos) || (ddfstrt_old_vpos != vpos))
         {
            start_bpl_dma (plfstrt);
            estimate_last_fetch_cycle (plfstrt);
            last_decide_line_hpos = hpos;
            do_sprites (plfstrt);
            return;
         }
      }
   }

   if (last_decide_line_hpos < SPR0_HPOS + 4 * MAX_SPRITES)
      do_sprites (hpos);
   
   last_decide_line_hpos = hpos;
}

/* Called when a color is about to be changed (write to a color register),
 * but the new color has not been entered into the table yet. */
static _INLINE_ void record_color_change (int hpos, int regno, unsigned long value)
{
	/* Early positions don't appear on-screen. */
	if ((framecnt != 0) || (vpos < minfirstline) || (hpos < HARD_DDF_START))
		return;
	
	decide_diw (hpos);
	decide_line (hpos);
	
	if (thisline_decision.ctable == -1)
		remember_ctable ();
	
	curr_color_changes[next_color_change].linepos = hpos;
	curr_color_changes[next_color_change].regno = regno;
	curr_color_changes[next_color_change++].value = value;
}

static _INLINE_ void record_register_change (int hpos, int regno, unsigned long value)
{
    if (regno == 0x100) {
       if (value & 0x800)
           thisline_decision.ham_seen = 1;
       if ((hpos < HARD_DDF_START) || (hpos < plfstrt + 0x20)) {
           thisline_decision.bplcon0 = value;
           thisline_decision.bplres = GET_RES (value);
       }
    }
    record_color_change (hpos, regno + 0x1000, value);
}

typedef int sprbuf_res_t, cclockres_t, hwres_t, bplres_t;

static _INLINE_ void expand_sprres (void)
{
	switch ((bplcon3 >> 6) & 3) {
		case 0: /* ECS defaults (LORES,HIRES=140ns,SHRES=70ns) */
			sprres = RES_LORES;
			break;
		case 1:
			sprres = RES_LORES;
			break;
		case 2:
			sprres = RES_HIRES;
			break;
		case 3:
			sprres = RES_SUPERHIRES;
			break;
	}
}

#define DO_PLAYFIELD_COLLISIONS \
{ \
   if (!clxcon_bpl_enable) \
      clxdat |= 1; \
   else \
      if (!(clxdat & 1)) \
         do_playfield_collisions (); \
}

#define DO_SPRITE_COLLISIONS \
{ \
   if (!clxcon_bpl_enable) \
      clxdat |= 0x1FE; \
   else \
      do_sprite_collisions (); \
}

/* handle very rarely needed playfield collision (CLXDAT bit 0) */
static void do_playfield_collisions (void)
{
   int bplres = res_bplcon0;
   hwres_t ddf_left = thisline_decision.plfleft * 2 << bplres;
   hwres_t hw_diwlast = coord_window_to_diw_x (thisline_decision.diwlastword);
   hwres_t hw_diwfirst = coord_window_to_diw_x (thisline_decision.diwfirstword);
   int i, collided, minpos, maxpos;
   int planes = (currprefs.chipset_mask & CSMASK_AGA) ? 8 : 6;

   collided = 0;
   minpos = thisline_decision.plfleft * 2;
   if (minpos < hw_diwfirst)
      minpos = hw_diwfirst;
   maxpos = thisline_decision.plfright * 2;
   if (maxpos > hw_diwlast)
      maxpos = hw_diwlast;
   for (i = minpos; i < maxpos && !collided; i+= 32) {
      int offs = ((i << bplres) - ddf_left) >> 3;
      int j;
      uae_u32 total = 0xffffffff;
      for (j = 0; j < planes; j++) {
         int ena = (clxcon_bpl_enable >> j) & 1;
         int match = (clxcon_bpl_match >> j) & 1;
         uae_u32 t = 0xffffffff;
         if (ena) {
            if (j < thisline_decision.nr_planes) {
               t = *(uae_u32 *)(line_data[next_lineno] + offs + 2 * j * MAX_WORDS_PER_LINE);
               t ^= (match & 1) - 1;
            } else {
               t = (match & 1) - 1;
            }
         }
         total &= t;
      }
      if (total) {
         collided = 1;
      }
   }
   if (collided)
      clxdat |= 1;
}

/* Sprite-to-sprite collisions are taken care of in record_sprite.  This one does
   playfield/sprite collisions. */
static void do_sprite_collisions (void)
{
   int nr_sprites = curr_drawinfo[next_lineno].nr_sprites;
   int first = curr_drawinfo[next_lineno].first_sprite_entry;
   int i;
   unsigned int collision_mask = clxmask[clxcon >> 12];
   int bplres = res_bplcon0;
   hwres_t ddf_left = thisline_decision.plfleft * 2 << bplres;
   hwres_t hw_diwlast = coord_window_to_diw_x (thisline_decision.diwlastword);
   hwres_t hw_diwfirst = coord_window_to_diw_x (thisline_decision.diwfirstword);

   for (i = 0; i < nr_sprites; i++) {
      struct sprite_entry *e = curr_sprite_entries + first + i;
      sprbuf_res_t j;
      sprbuf_res_t minpos = e->pos;
      sprbuf_res_t maxpos = e->max;
      hwres_t minp1 = minpos >> sprite_buffer_res;
      hwres_t maxp1 = maxpos >> sprite_buffer_res;

      if (maxp1 > hw_diwlast)
         maxpos = hw_diwlast << sprite_buffer_res;
      if (maxp1 > thisline_decision.plfright * 2)
         maxpos = thisline_decision.plfright * 2 << sprite_buffer_res;
      if (minp1 < hw_diwfirst)
         minpos = hw_diwfirst << sprite_buffer_res;
      if (minp1 < thisline_decision.plfleft * 2)
         minpos = thisline_decision.plfleft * 2 << sprite_buffer_res;

      for (j = minpos; j < maxpos; j++) {
         int sprpix = spixels[e->first_pixel + j - e->pos] & collision_mask;
         int k, offs, match = 1;

         if (sprpix == 0)
            continue;

         offs = ((j << bplres) >> sprite_buffer_res) - ddf_left;
         sprpix = sprite_ab_merge[sprpix & 255] | (sprite_ab_merge[sprpix >> 8] << 2);
         sprpix <<= 1;

         /* Loop over number of playfields.  */
         for (k = 1; k >= 0; k--) {
            int l;
            int planes = (currprefs.chipset_mask & CSMASK_AGA) ? 8 : 6;

            if (bplcon0 & 0x400)
               match = 1;
            for (l = k; match && l < planes; l += 2) {
               int t = 0;
               if (l < thisline_decision.nr_planes) {
                  uae_u32 *ldata = (uae_u32 *)(line_data[next_lineno] + 2 * l * MAX_WORDS_PER_LINE);
                  uae_u32 word = ldata[offs >> 5];
                  t = (word >> (31 - (offs & 31))) & 1;
               }
               if (clxcon_bpl_enable & (1 << l)) {
                  if (t != ((clxcon_bpl_match >> l) & 1))
                     match = 0;
               }
            }
            if (match) {
               clxdat |= sprpix << (k * 4);
            }
         }
      }
   }
}

static __inline__ void record_sprite_1 (uae_u16 *_GCCRES_ buf, uae_u32 datab, int num, int dbl,
				    unsigned int mask, int do_collisions, uae_u32 collision_mask)
{
	int j = 0;
	while (datab) {
		unsigned int tmp = *buf;
		unsigned int col = (datab & 3) << (2 * num);
		tmp |= col;
		if ((j & mask) == 0)
			*buf++ = tmp;
		if (dbl)
			*buf++ = tmp;
		j++;
		datab >>= 2;
		if (do_collisions) {
			tmp &= collision_mask;
			if (tmp) {
				unsigned int shrunk_tmp = sprite_ab_merge[tmp & 255] | (sprite_ab_merge[tmp >> 8] << 2);
				clxdat |= sprclx[shrunk_tmp];
			}
		}
	}
}

/* DATAB contains the sprite data; 16 pixels in two-bit packets.  Bits 0/1
   determine the color of the leftmost pixel, bits 2/3 the color of the next
   etc.
   This function assumes that for all sprites in a given line, SPRXP either
   stays equal or increases between successive calls.

   The data is recorded either in lores pixels (if ECS), or in hires pixels
   (if AGA).  No support for SHRES sprites.  */

static _INLINE_ void record_sprite (int line, int num, int sprxp, uae_u16 *_GCCRES_ data, uae_u16 *_GCCRES_ datb, unsigned int ctl)
{
	struct sprite_entry *e = curr_sprite_entries + next_sprite_entry;
	int i;
	int word_offs;
	uae_u16 *buf;
	uae_u32 collision_mask;
	int width = sprite_width;
	int dbl = 0, half = 0;
	unsigned int mask = 0;
	
	if (currprefs.chipset_mask & CSMASK_AGA) {
		width = (width << 1) >> sprres;
		dbl = sprite_buffer_res - sprres;
		if(dbl < 0)
	  {
	    half = -dbl;
	    dbl = 0;
	  }
		mask = sprres == RES_SUPERHIRES ? 1 : 0;
	}
	
	/* Try to coalesce entries if they aren't too far apart.  */
	if (! next_sprite_forced && (e[-1].max + 16 >= sprxp)) {
		e--;
	} else {
		next_sprite_entry++;
		e->pos = sprxp;
		e->has_attached = 0;
	}
	
	if (sprxp < e->pos)
		return;
	
	e->max = sprxp + width;
	e[1].first_pixel = e->first_pixel + ((e->max - e->pos + 3) & ~3);
	next_sprite_forced = 0;
	
	collision_mask = clxmask[clxcon >> 12];
	word_offs = e->first_pixel + sprxp - e->pos;
	
	for (i = 0; i < sprite_width; i += 16) {
		unsigned int da = *data;
		unsigned int db = *datb;
		uae_u32 datab = ((sprtaba[da & 0xFF] << 16) | sprtaba[da >> 8]
		                 | (sprtabb[db & 0xFF] << 16) | sprtabb[db >> 8]);
		
		buf = spixels + word_offs + ((i << dbl) >> half);
		record_sprite_1 (buf, datab, num, dbl, mask, 0, collision_mask);
		data++;
		datb++;
	}
	
	/* We have 8 bits per pixel in spixstate, two for every sprite pair.  The
       low order bit records whether the attach bit was set for this pair.  */
	
	if (ctl & (num << 7) & 0x80) {
		uae_u32 state = 0x01010101 << (num - 1);
		uae_u32 *stbuf = spixstate.words + (word_offs >> 2);
		uae_u8 *stb1 = spixstate.bytes + word_offs;	
		for (i = 0; i < width; i += 8) {
			stb1[0] |= state;
			stb1[1] |= state;
			stb1[2] |= state;
			stb1[3] |= state;
			stb1[4] |= state;
			stb1[5] |= state;
			stb1[6] |= state;
			stb1[7] |= state;
			stb1 += 8;
		}
		e->has_attached = 1;
	}
}

static _INLINE_ void decide_sprites (int hpos)
{
	int nrs[MAX_SPRITES], posns[MAX_SPRITES];
	int count, i;
	int point = hpos << 1;
	int width = sprite_width;
	int window_width = width >> sprres;
	
	if ((framecnt != 0) || (hpos < 0x14) || (nr_armed == 0) || (point == last_sprite_point))
		return;
	
	decide_diw (hpos);
	decide_line (hpos);
	
	count = 0;
	for (i = 0; i < MAX_SPRITES; i++) {
		int sprxp = spr[i].xpos;
		int hw_xp = (sprxp >> sprite_buffer_res);
		int window_xp = coord_hw_to_window_x (hw_xp) + (DIW_DDF_OFFSET);
		int j, bestp;
		
		if (! spr[i].armed || (sprxp < 0) || (hw_xp <= last_sprite_point) || (hw_xp > point))
			continue;
		if (((thisline_decision.diwfirstword >= 0) && (window_xp + window_width < thisline_decision.diwfirstword))
				|| ((thisline_decision.diwlastword >= 0) && (window_xp > thisline_decision.diwlastword)))
			continue;
		
		/* Sort the sprites in order of ascending X position before recording them.  */
		for (bestp = 0; bestp < count; bestp++) {
			if (posns[bestp] > sprxp)
				break;
			if ((posns[bestp] == sprxp) && (nrs[bestp] < i))
				break;
		}
		for (j = count; j > bestp; j--) {
			posns[j] = posns[j-1];
			nrs[j] = nrs[j-1];
		}
		posns[j] = sprxp;
		nrs[j] = i;
		count++;
	}
	for (i = 0; i < count; i++) {
		int nr = nrs[i];    
		record_sprite (next_lineno, nr, spr[nr].xpos, sprdata[nr], sprdatb[nr], sprctl[nr]);
	}
	last_sprite_point = point;
}

/* End of a horizontal scan line. Finish off all decisions that were not
 * made yet. */
static __inline__ void finish_decisions (void)
{
	struct draw_info *dip;
	
	if (framecnt != 0)
		return;
	
	int hpos = current_hpos ();

	decide_diw (hpos);
	decide_line (hpos);
	decide_fetch (hpos);
	
	if ((thisline_decision.plfleft != -1) && (thisline_decision.plflinelen == -1)) {
		if (fetch_state != fetch_not_started)
			return;
		thisline_decision.plfright = thisline_decision.plfleft;
		thisline_decision.plflinelen = 0;
		thisline_decision.bplres = RES_LORES;
	}
	
	/* Large DIWSTOP values can cause the stop position never to be
	 * reached, so the state machine always stays in the same state and
	 * there's a more-or-less full-screen DIW. */
	if ((hdiwstate == DIW_waiting_stop) || (thisline_decision.diwlastword > max_diwlastword))
		thisline_decision.diwlastword = max_diwlastword;
	
	if (thisline_decision.plfleft != -1) {
		record_diw_line (thisline_decision.diwfirstword, thisline_decision.diwlastword);
		decide_sprites (hpos);
	}
	
	dip = curr_drawinfo + next_lineno;

	dip->last_sprite_entry = next_sprite_entry;
	dip->last_color_change = next_color_change;
	
	if (thisline_decision.ctable == -1)
		remember_ctable();
	
	dip->nr_color_changes = next_color_change - dip->first_color_change;
	dip->nr_sprites = next_sprite_entry - dip->first_sprite_entry;
	
	line_decisions[next_lineno] = thisline_decision;
}

/* Set the state of all decisions to "undecided" for a new scanline. */
static __inline__ void reset_decisions (void)
{
   if (framecnt != 0)
      return;
   
   thisline_decision.nr_planes = 0;
   
   thisline_decision.plfleft = -1;
   thisline_decision.plflinelen = -1;
   thisline_decision.ham_seen = !! (bplcon0 & 0x800);
   thisline_decision.ham_at_start = !! (bplcon0 & 0x800);
   
   /* decided_res shouldn't be touched before it's initialized by decide_line(). */
   thisline_decision.diwfirstword = -1;
   thisline_decision.diwlastword = -2;
   if (hdiwstate == DIW_waiting_stop) {
      thisline_decision.diwfirstword = 0;
   }
   thisline_decision.ctable = -1;
   
   curr_drawinfo[next_lineno].first_color_change = next_color_change;
   curr_drawinfo[next_lineno].first_sprite_entry = next_sprite_entry;
   next_sprite_forced = 1;
   
   /* memset(sprite_last_drawn_at, 0, sizeof sprite_last_drawn_at); */
   last_sprite_point = 0;
   fetch_state = fetch_not_started;
   passed_plfstop = 0;
   
   {
      register int i;
      register int c = 0;
      register unsigned *ptr0=(uae_u32 *)todisplay;  /* clear only todisplay[i][0] if OCS/ECS */
      register unsigned *ptr1=fetched;
      register unsigned *ptr2=outword;
      if (currprefs.chipset_mask & CSMASK_AGA) 
      {
        register unsigned *ptr3=fetched_aga0;
        register unsigned *ptr4=fetched_aga1;
        for(i=0;i<MAX_PLANES;i++)
        {
            ptr0 += 4;
            *ptr0++ = c;
            *ptr0++ = c;
            *ptr0++ = c;
            *ptr0++ = c;
            *ptr3++ = c;
            *ptr4++ = c;
            *ptr1++ = c;
            *ptr2++ = c;
        }
      } 
      else 
      {
        for(i=0;i<MAX_PLANES;i++)
        {
            ptr0 += 4;
            *ptr0 = c;
            ptr0 += 4;
            *ptr1++ = c;
            *ptr2++ = c;
        }
      }
   } 
   
   last_decide_line_hpos = -1;
   last_diw_pix_hpos = -1;
   last_sprite_hpos = -1;
   last_fetch_hpos = -1;
}

/* set PAL or NTSC timing variables */
static _INLINE_ void init_hz (void)
{
	int isntsc;
	
	beamcon0 = new_beamcon0;
	
	isntsc = beamcon0 & 0x20 ? 0 : 1;
	if (!isntsc) {
		maxvpos = MAXVPOS_PAL;
		maxhpos = MAXHPOS_PAL;
		minfirstline = MINFIRSTLINE_PAL;
	} else {
		maxvpos = MAXVPOS_NTSC;
		maxhpos = MAXHPOS_NTSC;
		minfirstline = MINFIRSTLINE_NTSC;
	}
}

static _INLINE_ void calcdiw (void)
{
	int hstrt = diwstrt & 0xFF;
	int hstop = diwstop & 0xFF;
	int vstrt = diwstrt >> 8;
	int vstop = diwstop >> 8;
	
	if (diwhigh_written) {
		hstrt |= ((diwhigh >> 5) & 1) << 8;
		hstop |= ((diwhigh >> 13) & 1) << 8;
		vstrt |= (diwhigh & 7) << 8;
		vstop |= ((diwhigh >> 8) & 7) << 8;
	} else {
		hstop += 0x100;
		if ((vstop & 0x80) == 0)
			vstop |= 0x100;
	}
	
	diwfirstword = coord_diw_to_window_x (hstrt);
	diwlastword = coord_diw_to_window_x (hstop);
	/* WinUAE code */
   if (diwfirstword >= diwlastword) {
      diwfirstword = 0;
      diwlastword = max_diwlastword;
   }
   /***************/
	if (diwfirstword < 0)
		diwfirstword = 0;
	
	plffirstline = vstrt;
	plflastline = vstop;
	
	plfstrt = ddfstrt;
	plfstop = ddfstop;
   /* WinUAE code */
   if (ddfstop > maxhpos)
       plfstrt = 0;
   /***************/
   if (plfstrt < HARD_DDF_START)
      plfstrt = HARD_DDF_START;
}

/* Mousehack stuff */

#define defstepx (1<<16)
#define defstepy (1<<16)

static const int docal = 60, xcaloff = 40, ycaloff = 20;
static const int calweight = 3;
static int lastsampledmx, lastsampledmy;
static int lastspr0x,lastspr0y,spr0pos,spr0ctl;
static int mstepx,mstepy;
static int sprvbfl;

int lastmx, lastmy;
int newmousecounters;
int ievent_alive = 0;

static enum { unknown_mouse, normal_mouse, dont_care_mouse, follow_mouse } mousestate;

static _INLINE_ void mousehack_setdontcare (void)
{
    if (mousestate == dont_care_mouse)
	return;

    write_log ("Don't care mouse mode set\n");
    mousestate = dont_care_mouse;
    lastspr0x = lastmx; lastspr0y = lastmy;
    mstepx = defstepx; mstepy = defstepy;
}

static _INLINE_ void mousehack_setfollow (void)
{
    if (mousestate == follow_mouse)
	return;

    write_log ("Follow sprite mode set\n");
    mousestate = follow_mouse;
    sprvbfl = 0;
    spr0ctl = spr0pos = 0;
}

static _INLINE_ uae_u32 mousehack_helper (void)
{
    int mousexpos, mouseypos;

    {
	/* @@@ This isn't completely right, it doesn't deal with virtual
	   screen sizes larger than physical very well.  */
	if (lastmy >= PREFS_GFX_HEIGHT)
	    lastmy = PREFS_GFX_HEIGHT - 1;
	if (lastmy < 0)
	    lastmy = 0;
	if (lastmx < 0)
	    lastmx = 0;
	if (lastmx >= PREFS_GFX_WIDTH)
	    lastmx = PREFS_GFX_WIDTH - 1;
	mouseypos = coord_native_to_amiga_y (lastmy) << 1;
	mousexpos = coord_native_to_amiga_x (lastmx);
    }

    switch (_68k_dreg (0)) {
    case 0:
	return ievent_alive ? -1 : needmousehack ();
    case 1:
	ievent_alive = 10;
	return mousexpos;
    case 2:
	return mouseypos;
    }
    return 0;
}

void togglemouse (void)
{
    switch (mousestate) {
     case dont_care_mouse: mousehack_setfollow (); break;
     case follow_mouse: mousehack_setdontcare (); break;
     default: break; /* Nnnnnghh! */
    }
}

static __inline__ int adjust (int val)
{
  if (val > 127)
	  return 127;
  else 
    if (val < -127)
	    return -127;
  return val;
}

static _INLINE_ void do_mouse_hack (void)
{
  int spr0x = ((spr0pos & 0xff) << 2) | ((spr0ctl & 1) << 1);
  int spr0y = ((spr0pos >> 8) | ((spr0ctl & 4) << 6)) << 1;
  int diffx, diffy;

  if (ievent_alive > 0) 
  {
	  mouse_x = mouse_y = 0;
	  return;
  }
  switch (mousestate) 
  {
    case normal_mouse:
	    diffx = lastmx - lastsampledmx;
	    diffy = lastmy - lastsampledmy;
	    if (!newmousecounters) 
	    {
	      if (diffx > 127) 
	        diffx = 127;
	      if (diffx < -127) 
	        diffx = -127;
	      mouse_x += diffx;
	      if (diffy > 127) 
	        diffy = 127;
	      if (diffy < -127) 
	        diffy = -127;
	      mouse_y += diffy;
	    }
	    lastsampledmx += diffx; lastsampledmy += diffy;
	    break;

    case dont_care_mouse:
	    diffx = adjust (((lastmx - lastspr0x) * mstepx) >> 16);
	    diffy = adjust (((lastmy - lastspr0y) * mstepy) >> 16);
	    lastspr0x = lastmx; 
	    lastspr0y = lastmy;
	    mouse_x += diffx;
	    mouse_y += diffy;
	    break;

    case follow_mouse:
      //------------------------------------------
      // New stylus<->follow mouse mode
      //------------------------------------------
	    if (sprvbfl && (sprvbfl-- > 1)) 
      {
        int stylusxpos, stylusypos;          
	      stylusxpos = coord_native_to_amiga_x (lastmx);
	      stylusypos = coord_native_to_amiga_y (lastmy) << 1;
	      if(stylusxpos != spr0x || stylusypos != spr0y)
        {
          diffx = (stylusxpos - spr0x);
          diffy = (stylusypos - spr0y);
          if(diffx > 10 || diffx < -10)
            diffx = diffx * 50;
          else
            diffx = diffx * 100;
          if(diffy > 10 || diffy < -10)
            diffy = diffy * 50;
          else
            diffy = diffy * 100;
          mouse_x += adjust(diffx / 100);
          mouse_y += adjust(diffy / 100);
          mouseMoving = 1;
          fcounter = 0;
        }
      }          
      break;
	
    default:
	    return;
  }
}

static _INLINE_ void mousehack_handle (unsigned int ctl, unsigned int pos)
{
    if (!sprvbfl && (((pos & 0xff) << 2) > 2 * DISPLAY_LEFT_SHIFT)) {
	spr0ctl = ctl;
	spr0pos = pos;
	sprvbfl = 2;
    }
}

static uae_u32 timehack_helper (void)
{
    return 2;
}

 /*
  * register functions
  */
static __inline__ uae_u16 DENISEID (void)
{
	if (currprefs.chipset_mask & CSMASK_AGA)
		return 0xF8;
	if (currprefs.chipset_mask & CSMASK_ECS_DENISE)
		return 0xFC;
	return 0xFFFF;
}
static __inline__ uae_u16 DMACONR (void)
{
    return (dmacon | (bltstate==BLT_done ? 0 : 0x4000) | (blt_info.blitzero ? 0x2000 : 0));
}
static __inline__ uae_u16 INTENAR (void)
{
    return intena;
}
uae_u16 INTREQR (void)
{
    return intreq;
}
static __inline__ uae_u16 ADKCONR (void)
{
    return adkcon;
}

/* WinUAE code */
STATIC_INLINE uae_u16 VPOSR (void)
{
    unsigned int csbit = 0;
    int vp = (vpos >> 8) & 7;

	csbit |= (currprefs.chipset_mask & CSMASK_AGA) ? 0x2300 : 0;
	csbit |= (currprefs.chipset_mask & CSMASK_ECS_AGNUS) ? 0x2000 : 0;
	if ((prefs_chipmem_size > 1024 * 1024) && (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	    csbit |= 0x2100;
	if (mainMenu_ntsc)
	    csbit |= 0x1000;

    if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
    	vp &= 1;
    vp = vp | lof | csbit;
    return vp;
}
/***************/

static _INLINE_ void VPOSW (uae_u16 v)
{
	if (lof != (v & 0x8000))
		lof_changed = 1;
	lof = v & 0x8000;
	/*
	 * This register is much more fun on a real Amiga. You can program
	 * refresh rates with it ;) But I won't emulate this...
	 */
}

#define VHPOSR() ((vpos << 8) | current_hpos ())
#define COP1LCH(V) cop1lc = (cop1lc & 0xffff) | ((uae_u32)V << 16)
#define COP1LCL(V) cop1lc = (cop1lc & ~0xffff) | (V & 0xfffe)
#define COP2LCH(V) cop2lc = (cop2lc & 0xffff) | ((uae_u32)V << 16)
#define COP2LCL(V) cop2lc = (cop2lc & ~0xffff) | (V & 0xfffe)

static _INLINE_ void start_copper (void)
{
	int was_active = eventtab[ev_copper].active;
	eventtab[ev_copper].active = 0;
	if (was_active)
		events_schedule ();
	
	cop_state.ignore_next = 0;
	cop_state.state = COP_read1;
	cop_state.vpos = vpos;
	cop_state.hpos = current_hpos () & ~1;
	
	if (dmaen (DMA_COPPER)) {
		copper_enabled_thisline = 1;
		setcopper();
	}
}

static _INLINE_ void COPJMP1 (uae_u16 a)
{
    cop_state.ip = cop1lc;
    start_copper ();
}

static _INLINE_ void COPJMP2 (uae_u16 a)
{
    cop_state.ip = cop2lc;
    start_copper ();
}

static __inline__ void COPCON (uae_u16 a)
{
    copcon = a;
}

static _INLINE_ void DMACON (int hpos, uae_u16 v)
{
	int i;
	
	uae_u16 oldcon = dmacon;
	
	decide_line (hpos);
	decide_fetch (hpos);
	
	setclr (&dmacon, v);
	dmacon &= 0x1FFF;
	
	/* FIXME? Maybe we need to think a bit more about the master DMA enable
	 * bit in these cases. */
	if ((dmacon & DMA_COPPER) != (oldcon & DMA_COPPER)) {
		eventtab[ev_copper].active = 0;
	}
	if ((dmacon & DMA_COPPER) > (oldcon & DMA_COPPER)) {
		cop_state.ip = cop1lc;
		cop_state.ignore_next = 0;
		cop_state.state = COP_read1;
		cop_state.vpos = vpos;
		cop_state.hpos = hpos & ~1;
		copper_enabled_thisline = 1;
		setcopper();
	}
	if (! (dmacon & DMA_COPPER)) {
		copper_enabled_thisline = 0;
		unset_special (SPCFLAG_COPPER);
		cop_state.state = COP_stop;
	}
	
	if (((dmacon & DMA_BLITPRI) > (oldcon & DMA_BLITPRI)) && (bltstate != BLT_done)) {
		static int count = 0;
		if (!count) {
			count = 1;
			write_log ("warning: program is doing blitpri hacks.\n");
		}
		setnasty();
	}
	if ((dmacon & (DMA_BLITPRI | DMA_BLITTER | DMA_MASTER)) != (DMA_BLITPRI | DMA_BLITTER | DMA_MASTER))
		unset_special (SPCFLAG_BLTNASTY);
	
	if (produce_sound) {
		update_audio ();
		check_dma_audio();
		schedule_audio ();
	}
	if ((dmacon ^ oldcon) & (DMA_MASTER | DMA_BITPLANE)) {
	   ddf_change = vpos;
      if (dmaen (DMA_BITPLANE))
         maybe_start_bpl_dma (hpos);
	}
	events_schedule();
}


static _INLINE_ void SET_INTERRUPT(void)
{
	int new_irqs = 0;

	if (intena & 0x4000)
	{
		int imask = intreq & intena;
		if (imask & 0x0007) { new_irqs |= 1 << 1; }
		if (imask & 0x0008) { new_irqs |= 1 << 2; }
		if (imask & 0x0070) { new_irqs |= 1 << 3; }
		if (imask & 0x0780) { new_irqs |= 1 << 4; }
		if (imask & 0x1800) { new_irqs |= 1 << 5; }
		if (imask & 0x2000) { new_irqs |= 1 << 6; }
	}

	if (new_irqs == M68KCONTEXT.interrupts[0]); // nothing changed
	else
	{
		M68KCONTEXT.interrupts[0] = new_irqs;
		m68k_irq_update();
	}
}


static __inline__ void INTENA (uae_u16 v)
{
    setclr (&intena,v);
    /* There's stupid code out there that does
	[some INTREQ bits at level 3 are set]
	clear all INTREQ bits
	Enable one INTREQ level 3 bit
	Set level 3 handler

	If we set SPCFLAG_INT for the clear, then by the time the enable happens,
	we'll have SPCFLAG_DOINT set, and the interrupt happens immediately, but
	it needs to happen one insn later, when the new L3 handler has been
	installed.  */
    if (v & 0x8000)
	SET_INTERRUPT();

}

#define INTREQ_COMMON(VCLR) \
	setclr(&intreq,VCLR); \
	SET_INTERRUPT(); \

void INTREQ_0 (uae_u16 v)
{
    INTREQ_COMMON(v);
}

void INTREQ (uae_u16 v)
{
    INTREQ_COMMON(v);
    rethink_cias ();
}

static _INLINE_ void ADKCON (uae_u16 v)
{
   if (produce_sound)
      update_audio ();

    setclr (&adkcon,v);
    update_adkmasks ();
}

static _INLINE_ void BEAMCON0 (uae_u16 v)
{
    if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
	if (!(currprefs.chipset_mask & CSMASK_ECS_DENISE))
	    v &= 0x20;
	    new_beamcon0 = v;
    }
}

static __inline__ int is_bitplane_dma (int hpos)
{
   if (fetch_state == fetch_not_started || hpos < thisline_decision.plfleft)
      return 0;
   if ((passed_plfstop == 3 && hpos >= thisline_decision.plfright)
            || hpos >= estimated_last_fetch_cycle)
      return 0;
   return curr_diagram[(hpos - cycle_diagram_shift) & fetchstart_mask];
}

static _INLINE_ void BPLPTH (int hpos, uae_u16 v, int num)
{
    decide_line (hpos);
    decide_fetch (hpos);
    bpl[num].pt = (bpl[num].pt & 0xffff) | ((uae_u32)v << 16);
}
static void BPLPTL (int hpos, uae_u16 v, int num)
{
   int delta = 0;
   decide_line (hpos);
   decide_fetch (hpos);
   /* fix for "bitplane dma fetch at the same time while updating BPLxPTL" */
   /* fixes "3v Demo" by Cave and "New Year Demo" by Phoenix */
   if (is_bitplane_dma(hpos - 1) == num + 1 && num > 0)
      delta = 2 << fetchmode;
   bpl[num].pt = (bpl[num].pt & ~0xffff) | ((v + delta) & 0xfffe);
}

static _INLINE_ int isehb (uae_u16 bplcon0, uae_u16 bplcon2)
{
	if (currprefs.chipset_mask & CSMASK_AGA)
		return ((bplcon0 & 0x7010) == 0x6000);
    return (((bplcon0 & 0xFC00) == 0x6000) || ((bplcon0 & 0xFC00) == 0x7000));
}

static _INLINE_ void BPLCON0 (int hpos, uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_DENISE))
		v &= ~0x00F1;
	else if (! (currprefs.chipset_mask & CSMASK_AGA))
		v &= ~0x00B1;
 	
	if (bplcon0 == v)
		return;
	
   ddf_change = vpos;
	decide_line (hpos);
	decide_fetch (hpos);
	
	/* HAM change?  */
	if ((bplcon0 ^ v) & 0x800) {
	   record_color_change (hpos, -1, !! (v & 0x800));
	}
	
	bplcon0 = v;
	planes_bplcon0=GET_PLANES(v);
	res_bplcon0=GET_RES(v);
	curr_diagram = cycle_diagram_table[fetchmode][res_bplcon0][planes_bplcon0];

    expand_fmodes ();
}

static __inline__ void BPLCON1 (int hpos, uae_u16 v)
{
	if (bplcon1 == v)
		return;
   ddf_change = vpos;
	decide_line (hpos);
	decide_fetch (hpos);
	bplcon1 = v;
}

static __inline__ void BPLCON2 (int hpos, uae_u16 v)
{
	if (bplcon2 == v)
		return;
	decide_line (hpos);
	bplcon2 = v;
	record_register_change (hpos, 0x104, v);
}

STATIC_INLINE void BPLCON3 (int hpos, uae_u16 v)
{
    if (! (currprefs.chipset_mask & CSMASK_AGA))
	return;
	if (bplcon3 == v)
		return;
	decide_line (hpos);
	decide_sprites (hpos);
	bplcon3 = v;
	expand_sprres ();
}

STATIC_INLINE void BPLCON4 (int hpos, uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_AGA))
		return;
	if (bplcon4 == v)
		return;
	decide_line (hpos);
	bplcon4 = v;
}

static _INLINE_ void BPL1MOD (int hpos, uae_u16 v)
{
   v &= ~1;
   if ((uae_s16)bpl1mod == (uae_s16)v)
      return;
   decide_line (hpos);
   decide_fetch (hpos);
   bpl1mod = v;
}

static _INLINE_ void BPL2MOD (int hpos, uae_u16 v)
{
   v &= ~1;
   if ((uae_s16)bpl2mod == (uae_s16)v)
      return;
   decide_line (hpos);
   decide_fetch (hpos);
   bpl2mod = v;
}

static __inline__ void BPL1DAT (int hpos, uae_u16 v)
{
    decide_line (hpos);
    bpl1dat = v;

    maybe_first_bpl1dat (hpos);
}

/* We could do as well without those... */
#define BPL2DAT(V) bpl2dat = V
#define BPL3DAT(V) bpl3dat = V
#define BPL4DAT(V) bpl4dat = V
#define BPL5DAT(V) bpl5dat = V
#define BPL6DAT(V) bpl6dat = V
#define BPL7DAT(V) bpl7dat = V
#define BPL8DAT(V) bpl8dat = V

static _INLINE_ void DIWSTRT (int hpos, uae_u16 v)
{
	if ((diwstrt == v) && ! diwhigh_written)
		return;
	decide_line (hpos);
	diwhigh_written = 0;
	diwstrt = v;
	calcdiw ();
}

static _INLINE_ void DIWSTOP (int hpos, uae_u16 v)
{
	if ((diwstop == v) && ! diwhigh_written)
		return;
	decide_line (hpos);
	diwhigh_written = 0;
	diwstop = v;
	calcdiw ();
}

static _INLINE_ void DIWHIGH (int hpos, uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_DENISE))
		return;
	if (diwhigh_written && (diwhigh == v))
		return;
	decide_line (hpos);
	diwhigh_written = 1;
	diwhigh = v;
	calcdiw ();
}

static _INLINE_ void DDFSTRT (int hpos, uae_u16 v)
{
	v &= 0xFC;
	if (ddfstrt == v)
		return;
   ddf_change = vpos;
	decide_line (hpos);
   ddfstrt_old_hpos = hpos;
   ddfstrt_old_vpos = vpos;
	ddfstrt = v;
	calcdiw ();
	if ((ddfstop > 0xD4) && ((ddfstrt & 4) == 4)) {
		static int last_warned;
		last_warned = (last_warned + 1) & 4095;
		if (last_warned == 0)
			write_log ("WARNING! Very strange DDF values.\n");
	}
}

static _INLINE_ void DDFSTOP (int hpos, uae_u16 v)
{
	/* ??? "Virtual Meltdown" sets this to 0xD2 and expects it to behave
       differently from 0xD0.  RSI Megademo sets it to 0xd1 and expects it
       to behave like 0xd0.  Some people also write the high 8 bits and
       expect them to be ignored.  So mask it with 0xFE.  */
	v &= 0xFE;
	if (ddfstop == v)
		return;
	decide_line (hpos);
	decide_fetch (hpos);
	ddfstop = v;
	calcdiw ();
	if (fetch_state != fetch_not_started)
		estimate_last_fetch_cycle (hpos);
	if ((ddfstop > 0xD4) && ((ddfstrt & 4) == 4)) {
		static int last_warned;
		last_warned = (last_warned + 1) & 4095;
		if (last_warned == 0)
			write_log ("WARNING! Very strange DDF values.\n");
		write_log ("WARNING! Very strange DDF values.\n");
	}
}

static _INLINE_ void FMODE (uae_u16 v)
{
    if (!(currprefs.chipset_mask & CSMASK_AGA))
    	v = 0;
    
    ddf_change = vpos;
    fmode = v;
    sprite_width = GET_SPRITEWIDTH (fmode);
    switch (fmode & 3) {
    	case 0:
    		fetchmode = 0;
    		break;
    	case 1:
    	case 2:
    		fetchmode = 1;
    		break;
    	case 3:
    		fetchmode = 2;
    		break;
    }
    curr_diagram = cycle_diagram_table[fetchmode][GET_RES (v)][planes_bplcon0];
    expand_fmodes ();
}

static _INLINE_ void BLTADAT (uae_u16 v)
{
    maybe_blit (0);

    blt_info.bltadat = v;
}

/*
 * "Loading data shifts it immediately" says the HRM. Well, that may
 * be true for BLTBDAT, but not for BLTADAT - it appears the A data must be
 * loaded for every word so that AFWM and ALWM can be applied.
 */
static _INLINE_ void BLTBDAT (uae_u16 v)
{
    maybe_blit (0);

    if (bltcon1 & 2)
	blt_info.bltbhold = v << (bltcon1 >> 12);
    else
	blt_info.bltbhold = v >> (bltcon1 >> 12);
    blt_info.bltbdat = v;
}

#define BLTCDAT(V) maybe_blit (0); blt_info.bltcdat = V
#define BLTAMOD(V) maybe_blit (1); blt_info.bltamod = (uae_s16)(V & 0xFFFE)
#define BLTBMOD(V) maybe_blit (1); blt_info.bltbmod = (uae_s16)(V & 0xFFFE)
#define BLTCMOD(V) maybe_blit (1); blt_info.bltcmod = (uae_s16)(V & 0xFFFE)
#define BLTDMOD(V) maybe_blit (1); blt_info.bltdmod = (uae_s16)(V & 0xFFFE)
#define BLTCON0(V) maybe_blit (0); bltcon0 = V; blinea_shift = V >> 12

/* The next category is "Most useless hardware register".
 * And the winner is... */
static _INLINE_ void BLTCON0L (uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		return;
	maybe_blit (0); bltcon0 = (bltcon0 & 0xFF00) | (v & 0xFF);
}

#define BLTCON1(V) maybe_blit (0); bltcon1 = V
#define BLTAFWM(V) maybe_blit (0); blt_info.bltafwm = V
#define BLTALWM(V) maybe_blit (0); blt_info.bltalwm = V
#define BLTAPTH(V) maybe_blit (0); bltapt = (bltapt & 0xffff) | ((uae_u32)V << 16)
#define BLTAPTL(V) maybe_blit (0); bltapt = (bltapt & ~0xffff) | (V & 0xFFFE)
#define BLTBPTH(V) maybe_blit (0); bltbpt = (bltbpt & 0xffff) | ((uae_u32)V << 16)
#define BLTBPTL(V) maybe_blit (0); bltbpt = (bltbpt & ~0xffff) | (V & 0xFFFE)
#define BLTCPTH(V) maybe_blit (0); bltcpt = (bltcpt & 0xffff) | ((uae_u32)V << 16)
#define BLTCPTL(V) maybe_blit (0); bltcpt = (bltcpt & ~0xffff) | (V & 0xFFFE)
#define BLTDPTH(V) maybe_blit (0); bltdpt = (bltdpt & 0xffff) | ((uae_u32)V << 16)
#define BLTDPTL(V) maybe_blit (0); bltdpt = (bltdpt & ~0xffff) | (V & 0xFFFE)

static _INLINE_ void BLTSIZE (uae_u16 v)
{
    maybe_blit (0);

    blt_info.vblitsize = v >> 6;
    blt_info.hblitsize = v & 0x3F;
    if (!blt_info.vblitsize) blt_info.vblitsize = 1024;
    if (!blt_info.hblitsize) blt_info.hblitsize = 64;

    bltstate = BLT_init;
    do_blitter ();
}

static _INLINE_ void BLTSIZV (uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		return;
	maybe_blit (0);
	oldvblts = v & 0x7FFF;
}

static _INLINE_ void BLTSIZH (uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		return;
	maybe_blit (0);
	blt_info.hblitsize = v & 0x7FF;
	blt_info.vblitsize = oldvblts;
	if (!blt_info.vblitsize) blt_info.vblitsize = 32768;
	if (!blt_info.hblitsize) blt_info.hblitsize = 0x800;
	bltstate = BLT_init;
	do_blitter ();
}

static __inline__ void SPRxCTL_1 (uae_u16 v, int num, int hpos)
{
    int sprxp;
    struct sprite *s = &spr[num];
    sprctl[num] = v;
    nr_armed -= s->armed;
    s->armed = 0;
    sprxp = ((sprpos[num] & 0xFF) << 1) + (v & 1);

    /* Quite a bit salad in this register... */
    if (currprefs.chipset_mask & CSMASK_AGA) {
    	/* We ignore the SHRES 35ns increment for now; SHRES support doesn't
		 * work anyway, so we may as well restrict AGA sprites to a 70ns
		 * resolution. */
    	sprxp <<= 1;
    	sprxp |= (v >> 4) & 1;
    }
    s->xpos = sprxp;
    s->vstart = (sprpos[num] >> 8) | ((sprctl[num] << 6) & 0x100);
    s->vstop = (sprctl[num] >> 8) | ((sprctl[num] << 7) & 0x100);
    if (vpos == s->vstart)
	s->state = SPR_waiting_stop;
#ifdef SPRITE_DEBUG
    write_log ("%d:%d:SPR%dCTL V=%04.4X STATE=%d ARMED=%d\n", vpos, hpos, num, v, s->state, s->armed);
#endif
}
static __inline__ void SPRxPOS_1 (uae_u16 v, int num, int hpos)
{
    int sprxp;
    struct sprite *s = &spr[num];
    sprpos[num] = v;
    sprxp = ((v & 0xFF) << 1) + (sprctl[num] & 1);

    if (currprefs.chipset_mask & CSMASK_AGA) {
    	sprxp <<= 1;
    	sprxp |= (sprctl[num] >> 4) & 1;
    }
    s->xpos = sprxp;
    s->vstart = (sprpos[num] >> 8) | ((sprctl[num] << 6) & 0x100);
#ifdef SPRITE_DEBUG
    write_log ("%d:%d:SPR%dPOS %04.4X STATE=%d ARMED=%d\n", vpos, hpos, num, v, s->state, s->armed);
#endif
}
static __inline__ void SPRxDATA_1 (uae_u16 v, int num)
{
    sprdata[num][0] = v;
    nr_armed += 1 - spr[num].armed;
    spr[num].armed = 1;
}

#define SPRxDATB_1(V,NUM) sprdatb[NUM][0] = V
#define SPRxDATA(HPOS,V,NUM) decide_sprites (HPOS); SPRxDATA_1 (V, NUM)
#define SPRxDATB(HPOS,V,NUM) decide_sprites (HPOS); SPRxDATB_1 (V, NUM)
#define SPRxCTL(HPOS,V,NUM) decide_sprites (HPOS); SPRxCTL_1 (V, NUM, HPOS)
#define SPRxPOS(HPOS,V,NUM) decide_sprites (HPOS); SPRxPOS_1 (V, NUM, HPOS)

static _INLINE_ void SPRxPTH (int hpos, uae_u16 v, int num)
{
    decide_sprites (hpos);
    spr[num].pt &= 0xffff;
    spr[num].pt |= (uae_u32)v << 16;
#ifdef SPRITE_DEBUG
    write_log ("%d:%d:SPR%dPTH %08.8X\n", vpos, hpos, num, spr[num].pt);
#endif
}
static void SPRxPTL (int hpos, uae_u16 v, int num)
{
    decide_sprites (hpos);
    spr[num].pt &= ~0xffff;
    spr[num].pt |= v;
#ifdef SPRITE_DEBUG
    write_log ("%d:%d:SPR%dPTL %08.8X\n", vpos, hpos, num, spr[num].pt);
#endif
}

static _INLINE_ void CLXCON (uae_u16 v)
{
    clxcon = v;
    clxcon_bpl_enable = (v >> 6) & 63;
    clxcon_bpl_match = v & 63;
    clx_sprmask = ((((v >> 15) & 1) << 7) | (((v >> 14) & 1) << 5) | (((v >> 13) & 1) << 3) | (((v >> 12) & 1) << 1) | 0x55);
}

static _INLINE_ void CLXCON2 (uae_u16 v)
{
	if (!(currprefs.chipset_mask & CSMASK_AGA))
		return;
	clxcon2 = v;
	clxcon_bpl_enable |= v & (0x40|0x80);
	clxcon_bpl_match |= (v & (0x01|0x02)) << 6;
}

static _INLINE_ uae_u16 CLXDAT (void)
{
    uae_u16 v = clxdat;
    clxdat = 0;
    return v;
}

static _INLINE_ uae_u16 COLOR_READ (int num)
{
	int cr, cg, cb, colreg;
	uae_u16 cval;
	
	if (!(currprefs.chipset_mask & CSMASK_AGA) || !(bplcon2 & 0x0100))
		return 0xffff;
	
	colreg = ((bplcon3 >> 13) & 7) * 32 + num;
	cr = current_colors.color_regs_aga[colreg] >> 16;
	cg = (current_colors.color_regs_aga[colreg] >> 8) & 0xFF;
	cb = current_colors.color_regs_aga[colreg] & 0xFF;
	if (bplcon3 & 0x200)
		cval = ((cr & 15) << 8) | ((cg & 15) << 4) | ((cb & 15) << 0);
	else
		cval = ((cr >> 4) << 8) | ((cg >> 4) << 4) | ((cb >> 4) << 0);
	return cval;
}

static _INLINE_ void COLOR_WRITE (int hpos, uae_u16 v, int num)
{
	v &= 0xFFF;
	if (currprefs.chipset_mask & CSMASK_AGA) {
		int r,g,b;
		int cr,cg,cb;
		int colreg;
		uae_u32 cval;
		
		/* writing is disabled when RDRAM=1 */
		if (bplcon2 & 0x0100)
			return;
		
		colreg = ((bplcon3 >> 13) & 7) * 32 + num;
		r = (v & 0xF00) >> 8;
		g = (v & 0xF0) >> 4;
		b = (v & 0xF) >> 0;
		cr = current_colors.color_regs_aga[colreg] >> 16;
		cg = (current_colors.color_regs_aga[colreg] >> 8) & 0xFF;
		cb = current_colors.color_regs_aga[colreg] & 0xFF;
		
		if (bplcon3 & 0x200) {
			cr &= 0xF0; cr |= r;
			cg &= 0xF0; cg |= g;
			cb &= 0xF0; cb |= b;
		} else {
			cr = r + (r << 4);
			cg = g + (g << 4);
			cb = b + (b << 4);
		}
		cval = (cr << 16) | (cg << 8) | cb;
		if (cval == current_colors.color_regs_aga[colreg])
			return;
		
		/* Call this with the old table still intact. */
		record_color_change (hpos, colreg, cval);
		remembered_color_entry = -1;
		current_colors.color_regs_aga[colreg] = cval;
		current_colors.acolors[colreg] = CONVERT_RGB (cval);
	} else {
		if (current_colors.color_uae_regs_ecs[num] == v)
			return;
		/* Call this with the old table still intact. */
		record_color_change (hpos, num, v);
		remembered_color_entry = -1;
		current_colors.color_uae_regs_ecs[num] = v;
		current_colors.acolors[num] = xcolors[v];
	}
}

static uae_u16 potgo_value;

#define POTGO(V) potgo_value = V

static _INLINE_ uae_u16 POTGOR (void)
{
    uae_u16 v = (potgo_value | (potgo_value >> 1)) & 0x5500;

    v |= (~potgo_value & 0xAA00) >> 1;

    if (buttonstate[2] || (joy0button & 2))
    	v &= 0xFBFF;

    if (buttonstate[1] || (joy0button & 4))
	v &= 0xFEFF;

    if (joy1button & 2)
	    v &= 0xbfff;

    if (joy1button & 4)
	    v &= 0xefff;

    return v;
}

static _INLINE_ uae_u16 POT0DAT (void)
{
	static uae_u16 cnt = 0;
	
	if (buttonstate[2])
		cnt = ((cnt + 1) & 0xFF) | (cnt & 0xFF00);
	
	if (buttonstate[1])
		cnt += 0x100;
	
	return cnt;
}

static _INLINE_ uae_u16 JOY0DAT (void)
{
    do_mouse_hack ();
    return ((uae_u8)mouse_x) + ((uae_u16)mouse_y << 8) + joy0dir;
}

static _INLINE_ uae_u16 JOY1DAT (void)
{
    return joy1dir;
}

static _INLINE_ void JOYTEST (uae_u16 v)
{
    mouse_x = v & 0xFC;
    mouse_y = (v >> 8) & 0xFC;
}

/* The copper code.  The biggest nightmare in the whole emulator.

   Alright.  The current theory:
   1. Copper moves happen 2 cycles after state READ2 is reached.
      It can't happen immediately when we reach READ2, because the
      data needs time to get back from the bus.  An additional 2
      cycles are needed for non-Agnus registers, to take into account
      the delay for moving data from chip to chip.
   2. As stated in the HRM, a WAIT really does need an extra cycle
      to wake up.  This is implemented by _not_ falling through from
      a successful wait to READ1, but by starting the next cycle.
      (Note: the extra cycle for the WAIT apparently really needs a
      free cycle; i.e. contention with the bitplane fetch can slow
      it down).
   3. Apparently, to compensate for the extra wake up cycle, a WAIT
      will use the _incremented_ horizontal position, so the WAIT
      cycle normally finishes two clocks earlier than the position
      it was waiting for.  The extra cycle then takes us to the
      position that was waited for.
      If the earlier cycle is busy with a bitplane, things change a bit.
      E.g., waiting for position 0x50 in a 6 plane display: In cycle
      0x4e, we fetch BPL5, so the wait wakes up in 0x50, the extra cycle
      takes us to 0x54 (since 0x52 is busy), then we have READ1/READ2,
      and the next register write is at 0x5c.
   4. The last cycle in a line is not usable for the copper.
   5. A 4 cycle delay also applies to the WAIT instruction.  This means
      that the second of two back-to-back WAITs (or a WAIT whose
      condition is immediately true) takes 8 cycles.
   6. This also applies to a SKIP instruction.  The copper does not
      fetch the next instruction while waiting for the second word of
      a WAIT or a SKIP to arrive.
   7. A SKIP also seems to need an unexplained additional two cycles
      after its second word arrives; this is _not_ a memory cycle (I
      think, the documentation is pretty clear on this).
   8. Two additional cycles are inserted when writing to COPJMP1/2.  */

/* Determine which cycles are available for the copper in a display
 * with a agiven number of planes.  */

static __inline__ int copper_cant_read (int hpos)
{
   if (hpos + 1 >= maxhpos)
      return 1;
   return is_bitplane_dma (hpos);
}

static __inline__ int dangerous_reg (int reg)
{
	/* Safe:
	 * Bitplane pointers, control registers, modulos and data.
	 * Sprite pointers, control registers, and data.
	 * Color registers.  */
	if ((reg >= 0xE0) && (reg < 0x1C0))
		return 0;
	return 1;
}

/* The future, Conan?
   We try to look ahead in the copper list to avoid doing continuous calls
   to updat_copper (which is what happens when SPCFLAG_COPPER is set).  If
   we find that the same effect can be achieved by setting a delayed event
   and then doing multiple copper insns in one batch, we can get a massive
   speedup.

   We don't try to be precise here.  All copper reads take exactly 2 cycles,
   the effect of bitplane contention is ignored.  Trying to get it exactly
   right would be much more complex and as such carry a huge risk of getting
   it subtly wrong; and it would also be more expensive - we want this code
   to be fast.  */

static _INLINE_ void predict_copper (void)
{
	uaecptr ip = cop_state.ip;
	unsigned int c_hpos = cop_state.hpos;
	enum copper_states state = cop_state.state;
	unsigned int w1, w2, cycle_count;
	
	switch (state) {
		case COP_read1_wr_in2:
		case COP_read2_wr_in2:
		case COP_read1_wr_in4:
			if (dangerous_reg (cop_state.saved_i1))
				return;
			state = state == COP_read2_wr_in2 ? COP_read2 : COP_read1;
			break;
			
		case COP_read1_in2:
			c_hpos += 2;
			state = COP_read1;
			break;
			
		case COP_stop:
		case COP_bltwait:
		case COP_wait1:
		case COP_skip_in4:
		case COP_skip_in2:
			return;
			
		case COP_wait_in4:
			c_hpos += 2;
			/* fallthrough */
		case COP_wait_in2:
			c_hpos += 2;
			/* fallthrough */
		case COP_wait:
			state = COP_wait;
			break;
			
		default:
			break;
	}
	/* Only needed for COP_wait, but let's shut up the compiler.  */
	w1 = cop_state.saved_i1;
	w2 = cop_state.saved_i2;
	cop_state.first_sync = c_hpos;
	cop_state.regtypes_modified = REGTYPE_FORCE;
	
	/* Get this case out of the way, so that the loop below only has to deal
       with read1 and wait.  */
	if (state == COP_read2) {
		w1 = cop_state.i1;
		if (w1 & 1) {
			w2 = CHIPMEM_WGET_CUSTOM (ip/*&0x000FFFFF*/);
			if (w2 & 1)
				goto done;
			state = COP_wait;
			c_hpos += 4;
		} else if (dangerous_reg (w1)) {
			c_hpos += 4;
			goto done;
		} else {
			cop_state.regtypes_modified |= regtypes[w1 & 0x1FE];
			state = COP_read1;
			c_hpos += 2;
		}
		ip += 2;	
	}
	
	while (c_hpos + 1 < maxhpos) {
		if (state == COP_read1) {
			w1 = CHIPMEM_WGET_CUSTOM (ip/*&0x000FFFFF*/);
			if (w1 & 1) {
				w2 = CHIPMEM_WGET_CUSTOM ((ip + 2)/*&0x000FFFFF*/);
				if (w2 & 1)
					break;
				state = COP_wait;
				c_hpos += 6;
			} else if (dangerous_reg (w1)) {
				c_hpos += 6;
				goto done;
			} else {
				cop_state.regtypes_modified |= regtypes[w1 & 0x1FE];
				c_hpos += 4;
			}
			ip += 4;
		} else if (state == COP_wait) {
			if ((w2 & 0xFE) != 0xFE)
				break;
			else {
				unsigned int vcmp = (w1 & (w2 | 0x8000)) >> 8;
				unsigned int hcmp = (w1 & 0xFE);
				
				unsigned int vp = vpos & (((w2 >> 8) & 0x7F) | 0x80);
				if (vp < vcmp) {
					/* Whee.  We can wait until the end of the line!  */
					c_hpos = maxhpos;
				} else if ((vp > vcmp) || (hcmp <= c_hpos)) {
					state = COP_read1;
					/* minimum wakeup time */
					c_hpos += 2;
				} else {
					state = COP_read1;
					c_hpos = hcmp;
				}
				/* If this is the current instruction, remember that we don't
		   need to sync CPU and copper anytime soon.  */
				if (cop_state.ip == ip) {
					cop_state.first_sync = c_hpos;
				}
			}
		} else
			return;
	}
	
	done:
	cycle_count = c_hpos - cop_state.hpos;
	if (cycle_count >= 8) {
		unset_special (SPCFLAG_COPPER);
		eventtab[ev_copper].active = 1;
		eventtab[ev_copper].oldcycles = get_cycles ();
		eventtab[ev_copper].evtime = get_cycles () + cycle_count * CYCLE_UNIT;
		events_schedule ();
	}
}

static _INLINE_ void perform_copper_write (int old_hpos)
{
	int vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);
	unsigned int address = cop_state.saved_i1 & 0x1FE;
	
	if (address < (copcon & 2 ? ((currprefs.chipset_mask & CSMASK_AGA) ? 0 : 0x40u) : 0x80u)) {
		cop_state.state = COP_stop;	
		copper_enabled_thisline = 0;
		unset_special (SPCFLAG_COPPER);
		return;
	}
	
	if (address == 0x88) {
		cop_state.ip = cop1lc;
		cop_state.state = COP_read1_in2;
	} else if (address == 0x8A) {
		cop_state.ip = cop2lc;
		cop_state.state = COP_read1_in2;
	} else
	{
		custom_wput_1 (old_hpos, address, cop_state.saved_i2);
	}
}

static int isagnus[]= {
    1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
    1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* BPLxPT */
    0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* SPRxPT */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* colors */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static _INLINE_ void update_copper (int until_hpos)
{
	int vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);    
	int c_hpos = cop_state.hpos;
	
	if (eventtab[ev_copper].active)
		return;
	
	if ((cop_state.state == COP_wait) && (vp < cop_state.vcmp))
		return;
	
	until_hpos &= ~1;
	
	if (until_hpos > (maxhpos & ~1))
		until_hpos = maxhpos & ~1;
	
	until_hpos += 2;
	for (;;) {
		int old_hpos = c_hpos;
		int hp;
		
		if (c_hpos >= until_hpos)
			break;
		
		/* So we know about the fetch state.  */
		decide_line (c_hpos);
		
		switch (cop_state.state) {
			case COP_read1_in2:
				cop_state.state = COP_read1;
				break;
			case COP_read1_wr_in2:
				cop_state.state = COP_read1;
				perform_copper_write (old_hpos);
				/* That could have turned off the copper.  */
				if (! copper_enabled_thisline)
					goto out;
				
				break;
			case COP_read1_wr_in4:
				cop_state.state = COP_read1_wr_in2;
				break;
			case COP_read2_wr_in2:
				cop_state.state = COP_read2;
				perform_copper_write (old_hpos);
				/* That could have turned off the copper.  */
				if (! copper_enabled_thisline)
					goto out;
				
				break;
			case COP_wait_in2:
				cop_state.state = COP_wait1;
				break;
			case COP_wait_in4:
				cop_state.state = COP_wait_in2;
				break;
			case COP_skip_in2:
			{
				static int skipped_before;
				unsigned int vcmp, hcmp, vp1, hp1;
				cop_state.state = COP_read1_in2;
				
				vcmp = (cop_state.saved_i1 & (cop_state.saved_i2 | 0x8000)) >> 8;
				hcmp = (cop_state.saved_i1 & cop_state.saved_i2 & 0xFE);
				
				if (! skipped_before) {
					skipped_before = 1;
					write_log ("Program uses Copper SKIP instruction.\n");
				}
				
				vp1 = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);
				hp1 = old_hpos & (cop_state.saved_i2 & 0xFE);
				
				if (((vp1 > vcmp) || ((vp1 == vcmp) && (hp1 >= hcmp)))
						&& (((cop_state.saved_i2 & 0x8000) != 0) || ! (DMACONR() & 0x4000)))
					cop_state.ignore_next = 1;
				break;
			}
			case COP_skip_in4:
				cop_state.state = COP_skip_in2;
				break;
			default:
				break;
		}
		
		c_hpos += 2;
		if (copper_cant_read (old_hpos))
			continue;
		
		switch (cop_state.state) {
			case COP_read1_wr_in4:
				return;
				
			case COP_read1_wr_in2:
			case COP_read1:
				cop_state.i1 = CHIPMEM_WGET_CUSTOM (cop_state.ip/*&0x000FFFFF*/);
				cop_state.ip += 2;
				cop_state.state = cop_state.state == COP_read1 ? COP_read2 : COP_read2_wr_in2;
				break;
				
			case COP_read2_wr_in2:
				return;
				
			case COP_read2:
				cop_state.i2 = CHIPMEM_WGET_CUSTOM (cop_state.ip/*&0x000FFFFF*/);
				cop_state.ip += 2;
				if (cop_state.ignore_next) {
					cop_state.ignore_next = 0;
					cop_state.state = COP_read1;
					break;
				}
				
				cop_state.saved_i1 = cop_state.i1;
				cop_state.saved_i2 = cop_state.i2;
				cop_state.saved_ip = cop_state.ip;
				
				if (cop_state.i1 & 1) {
					if (cop_state.i2 & 1)
						cop_state.state = COP_skip_in4;
					else
						cop_state.state = COP_wait_in4;
				} else {
					unsigned int reg = cop_state.i1 & 0x1FE;
					cop_state.state = isagnus[reg >> 1] ? COP_read1_wr_in2 : COP_read1_wr_in4;
				}
				break;
				
			case COP_wait1:
				/* There's a nasty case here.  As stated in the "Theory" comment above, we
			   test against the incremented copper position.  I believe this means that
			   we have to increment the _vertical_ position at the last cycle in the line,
			   and set the horizontal position to 0.
			   Normally, this isn't going to make a difference, since we consider these
			   last cycles unavailable for the copper, so waking up in the last cycle has
			   the same effect as waking up at the start of the line.  However, there is
			   one possible problem:  If we're at 0xFFE0, any wait for an earlier position
			   must _not_ complete (since, in effect, the current position will be back
			   at 0/0).  This can be seen in the Superfrog copper list.
			   Things get monstrously complicated if we try to handle this "properly" by
			   incrementing vpos and setting c_hpos to 0.  Especially the various speedup
			   hacks really assume that vpos remains constant during one line.  Hence,
			   this hack: defer the entire decision until the next line if necessary.  */
				if (c_hpos >= (maxhpos & ~1))
					break;
				cop_state.state = COP_wait;
				
				cop_state.vcmp = (cop_state.saved_i1 & (cop_state.saved_i2 | 0x8000)) >> 8;
				cop_state.hcmp = (cop_state.saved_i1 & cop_state.saved_i2 & 0xFE);
				
				vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);
				
				if ((cop_state.saved_i1 == 0xFFFF) && (cop_state.saved_i2 == 0xFFFE)) {
					cop_state.state = COP_stop;
					copper_enabled_thisline = 0;
					unset_special (SPCFLAG_COPPER);
					goto out;
				}
				if (vp < cop_state.vcmp) {
					copper_enabled_thisline = 0;
					unset_special (SPCFLAG_COPPER);
					goto out;
				}
				
				/* fall through */
				do_wait:
			case COP_wait:
				if (vp < cop_state.vcmp)
					return;
				
				hp = c_hpos & (cop_state.saved_i2 & 0xFE);
				if ((vp == cop_state.vcmp) && (hp < cop_state.hcmp)) {
					/* Position not reached yet.  */
					if ((cop_state.saved_i2 & 0xFE) == 0xFE) {
						int wait_finish = cop_state.hcmp - 2;
						/* This will leave c_hpos untouched if it's equal to wait_finish.  */
						if (wait_finish < c_hpos)
							return;
						else if (wait_finish <= until_hpos) {
							c_hpos = wait_finish;
						} else
							c_hpos = until_hpos;
					}	      
					break;
				}
				
				/* Now we know that the comparisons were successful.  We might still
			   have to wait for the blitter though.  */
				if (((cop_state.saved_i2 & 0x8000) == 0) && (DMACONR() & 0x4000)) {
					/* We need to wait for the blitter.  */
					cop_state.state = COP_bltwait;
					copper_enabled_thisline = 0;
					unset_special (SPCFLAG_COPPER);
					goto out;
				}
				
				cop_state.state = COP_read1;
				break;
				
			default:
				break;
		}
	}
	
	out:
	cop_state.hpos = c_hpos;
	
    /* The test against maxhpos also prevents us from calling predict_copper
       when we are being called from hsync_handler, which would not only be
       stupid, but actively harmful.  */
	if ((_68k_spcflags & SPCFLAG_COPPER) && (c_hpos + 8 < maxhpos))
		predict_copper ();
}

static _INLINE_ void compute_spcflag_copper (void)
{
	copper_enabled_thisline = 0;
	unset_special (SPCFLAG_COPPER);
	if (! dmaen (DMA_COPPER) || (cop_state.state == COP_stop) || (cop_state.state == COP_bltwait))
		return;
	
	if (cop_state.state == COP_wait) {
		int vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);
		
		if (vp < cop_state.vcmp)
			return;
	}
	copper_enabled_thisline = 1;
	
	predict_copper ();
	
	if (! eventtab[ev_copper].active)
		setcopper();
}

static void copper_handler (void)
{
    /* This will take effect immediately, within the same cycle.  */
	setcopper();
	
	if (! copper_enabled_thisline)
		return;
	
	eventtab[ev_copper].active = 0;
}

void blitter_done_notify (void)
{
	if (cop_state.state != COP_bltwait)
		return;
	
	cop_state.hpos = current_hpos () & ~1;
	cop_state.vpos = vpos;
	cop_state.state = COP_wait;
	compute_spcflag_copper ();
}

void do_copper (void)
{
    int hpos = current_hpos ();
    update_copper (hpos);
}

/* ADDR is the address that is going to be read/written; this access is
   the reason why we want to update the copper.  This function is also
   used from hsync_handler to finish up the line; for this case, we check
   hpos against maxhpos.  */
static __inline__ void sync_copper_with_cpu (int hpos, int do_schedule, unsigned int addr)
{
	/* Need to let the copper advance to the current position.  */
	if (eventtab[ev_copper].active) {
		if (hpos != maxhpos) {
			/* There might be reasons why we don't actually need to bother
	       updating the copper.  */
			if (hpos < cop_state.first_sync)
				return;
			
			if ((cop_state.regtypes_modified & regtypes[addr & 0x1FE]) == 0)
				return;
		}
		
		eventtab[ev_copper].active = 0;
		if (do_schedule)
			events_schedule ();
		setcopper();
	}
	if (copper_enabled_thisline)
		update_copper (hpos);
}

STATIC_INLINE uae_u16 sprite_fetch (struct sprite *s, int dma)
{
    if (dma)
	  last_custom_value = CHIPMEM_WGET_CUSTOM (s->pt);
    s->pt += 2;
    return last_custom_value;
}

static __inline__ void do_sprites_1 (int num, int cycle, int hpos)
{
	struct sprite *s = &spr[num];
    int dma;
	
	if (!cycle) {
		if (vpos == s->vstart)
			s->state = SPR_waiting_stop;
		if (vpos == s->vstop)
			s->state = SPR_restart;
	}
	if (!dmaen (DMA_SPRITE))
		return;
    dma = hpos < ddfstrt || diwstate != DIW_waiting_stop || !dmaen (DMA_BITPLANE);
	if ((s->state == SPR_restart) || (vpos == sprite_vblank_endline)) {
	    uae_u16 data = sprite_fetch (s, dma);
	    s->pt += (sprite_width >> 3) - 2;
		if (cycle == 0) {
			SPRxPOS_1 (dma ? data : sprpos[num], num, hpos);
		} else {
			s->state = SPR_waiting_start;
			SPRxCTL_1 (dma ? data : sprctl[num], num, hpos);
		}
	} else if (s->state == SPR_waiting_stop) {
	/* Hack for X mouse auto-calibration */
		if ((num == 0) && (cycle == 0))
			mousehack_handle (sprctl[0], sprpos[0]);
		
	    uae_u16 data = sprite_fetch (s, dma);
		if (!cycle)
			SPRxDATA_1 (dma ? data : sprdata[num][0], num);
		else
			SPRxDATB_1 (dma ? data : sprdatb[num][0], num);
		switch (sprite_width)
		{
			case 64:
			{
		uae_u32 data32 = sprite_fetch (s, dma);
		uae_u32 data641 = sprite_fetch (s, dma);
		uae_u32 data642 = sprite_fetch (s, dma);
		if (dma) {
				if (cycle == 0) {
					sprdata[num][3] = data642;
					sprdata[num][2] = data641;
					sprdata[num][1] = data32;
				} else {
					sprdatb[num][3] = data642;
					sprdatb[num][2] = data641;
					sprdatb[num][1] = data32;
				}
		}
	    }
			break;
			case 32:
			{	
		uae_u32 data32 = sprite_fetch (s, dma);
		if (dma) {
				if (cycle == 0)
					sprdata[num][1] = data32;
				else
					sprdatb[num][1] = data32;
		}
			}
			break;
		}
	}
}

static _INLINE_ void do_sprites (int hpos)
{
    int maxspr, minspr;
    int i;

    /* I don't know whether this is right. Some programs write the sprite pointers
     * directly at the start of the copper list. With the test against currvp, the
     * first two words of data are read on the second line in the frame. The problem
     * occurs when the program jumps to another copperlist a few lines further down
     * which _also_ writes the sprite pointer registers. This means that a) writing
     * to the sprite pointers sets the state to SPR_restart; or b) that sprite DMA
     * is disabled until the end of the vertical blanking interval. The HRM
     * isn't clear - it says that the vertical sprite position can be set to any
     * value, but this wouldn't be the first mistake... */
    /* Update: I modified one of the programs to write the sprite pointers the
     * second time only _after_ the VBlank interval, and it showed the same behaviour
     * as it did unmodified under UAE with the above check. This indicates that the
     * solution below is correct. */
    /* Another update: seems like we have to use the NTSC value here (see Sanity Turmoil
     * demo).  */
    /* Maximum for Sanity Turmoil: 27.
       Minimum for Sanity Arte: 22.  */
    if (vpos < sprite_vblank_endline)
    	return;
    
    maxspr = hpos;
    minspr = last_sprite_hpos;
    
    if ((minspr >= SPR0_HPOS + MAX_SPRITES * 4) || (maxspr < SPR0_HPOS))
    	return;
    
    if (maxspr > SPR0_HPOS + MAX_SPRITES * 4)
    	maxspr = SPR0_HPOS + MAX_SPRITES * 4;
    if (minspr < SPR0_HPOS)
    	minspr = SPR0_HPOS;
    if (!(minspr&1))
    	minspr++;
    
    for (i = minspr; i < maxspr; i+=2)
    	do_sprites_1 ((i - SPR0_HPOS) / 4, ((i - SPR0_HPOS) & 3)>>1, i);
    last_sprite_hpos = hpos;
}

static _INLINE_ void init_sprites (void)
{
	int i;
	
	for (i = 0; i < MAX_SPRITES; i++)
		spr[i].state = SPR_restart;
	uae4all_memclr (sprpos, sizeof sprpos);
	uae4all_memclr (sprctl, sizeof sprctl);
}

static _INLINE_ void init_hardware_frame (void)
{
    next_lineno = 0;
    diwstate = DIW_waiting_start;
    hdiwstate = DIW_waiting_start;
    ddfstate = DIW_waiting_start;
}

void init_hardware_for_drawing_frame (void)
{
	/* Avoid this code in the first frame after a customreset.  */
	if (next_sprite_entry > 0) {
		int npixels = curr_sprite_entries[next_sprite_entry].first_pixel;
		uae4all_memclr(spixels, npixels * sizeof *spixels);
		uae4all_memclr(spixstate.bytes, npixels * sizeof *spixstate.bytes);
	}
	
	next_color_change = 0;
	next_sprite_entry = 0;
	next_color_entry = 0;
	remembered_color_entry = -1;
	
	curr_sprite_entries[0].first_pixel = 0;
	next_sprite_forced = 1;
}

static int vsync_handler_cnt_disk_change=0;

static void vsync_handler (void)
{
	int i;
	for (i = 0; i < MAX_SPRITES; i++)
		spr[i].state = SPR_waiting_start;
	
	{
		static int back_joy0button=0;
		handle_events ();
		getjoystate (0, &joy1dir, &joy1button);
		getjoystate (1, &joy0dir, &joy0button);
		if (joy0button!=back_joy0button)
	  {
			back_joy0button= joy0button;
			buttonstate[0]= joy0button & 0x01;
		}
	}
	
	INTREQ (0x8020);
	
	if (bplcon0 & 4)
		lof ^= 0x8000;
	
	vsync_handle_redraw (lof, lof_changed);

	if (quit_program > 0)
		return;
	
	if (vsync_handler_cnt_disk_change == 0) {
		/* resolution_check_change (); */
		DISK_check_change ();
		vsync_handler_cnt_disk_change = 5; //20;
	}
	vsync_handler_cnt_disk_change--;
	
	/* For now, let's only allow this to change at vsync time.  It gets too
	 * hairy otherwise.  */
	if (beamcon0 != new_beamcon0)
		init_hz ();
	
	lof_changed = 0;
	
	cop_state.ip = cop1lc;
	cop_state.state = COP_read1;
	cop_state.vpos = 0;
	cop_state.hpos = 0;
	cop_state.ignore_next = 0;
	
	init_hardware_frame ();
	
	if (ievent_alive > 0)
		ievent_alive--;
	CIA_vsync_handler ();
}

static void hsync_handler (void)
{
   /* Using 0x8A makes sure that we don't accidentally trip over the
       modified_regtypes check.  */
   sync_copper_with_cpu (maxhpos, 0, 0x8A);
   
   finish_decisions ();
   if (thisline_decision.plfleft != -1) {
      DO_SPRITE_COLLISIONS
      DO_PLAYFIELD_COLLISIONS
   }
   
   hsync_record_line_state (next_lineno);
   
   eventtab[ev_hsync].evtime += get_cycles () - eventtab[ev_hsync].oldcycles;
   eventtab[ev_hsync].oldcycles = get_cycles ();
   CIA_hsync_handler ();
   
   if (produce_sound) {
      update_audio();
      fetch_audio();
   }
   
   /* In theory only an equality test is needed here - but if a program
       goes haywire with the VPOSW register, it can cause us to miss this,
       with vpos going into the thousands (and all the nasty consequences
       this has).  */
   
   vpos++;
   if (vpos  >= (maxvpos + (lof != 0)))
   {
      vpos=0;
      vsync_handler ();
   }
   
   DISK_update ();
   
   if (framecnt == 0) {
      int lineno = vpos;
      next_lineno = lineno;
      reset_decisions ();
   }
#ifdef USE_AUTOCONFIG
   if (uae_int_requested) {
      set_uae_int_flag ();
      INTREQ (0xA000);
   }
#endif
   /* See if there's a chance of a copper wait ending this line.  */
   cop_state.hpos = 0;
   compute_spcflag_copper ();
}

static _INLINE_ void init_regtypes (void)
{
    int i;
    for (i = 0; i < 512; i += 2) {
	regtypes[i] = REGTYPE_ALL;
	if (((i >= 0x20) && (i < 0x28)) || (i == 0x08) || (i == 0x7E))
	    regtypes[i] = REGTYPE_DISK;
	else if ((i >= 0x68) && (i < 0x70))
	    regtypes[i] = REGTYPE_NONE;
	else if ((i >= 0x40) && (i < 0x78))
	    regtypes[i] = REGTYPE_BLITTER;
	else if ((i >= 0xA0) && (i < 0xE0) && ((i & 0xF) < 0xE))
	    regtypes[i] = REGTYPE_AUDIO;
	else if ((i >= 0xA0) && (i < 0xE0))
	    regtypes[i] = REGTYPE_NONE;
	else if ((i >= 0xE0) && (i < 0x100))
	    regtypes[i] = REGTYPE_PLANE;
	else if ((i >= 0x120) && (i < 0x180))
	    regtypes[i] = REGTYPE_SPRITE;
	else if ((i >= 0x180) && (i < 0x1C0))
	    regtypes[i] = REGTYPE_COLOR;
	else switch (i) {
		case 0x02:
			/* DMACONR - setting this to REGTYPE_BLITTER will cause it to
	         * conflict with DMACON (since that is REGTYPE_ALL), and the
	         * blitter registers (for the BBUSY bit), but nothing else,
	         * which is (I think) what we want.  */
			regtypes[i] = REGTYPE_BLITTER;
			break;
		case 0x04: case 0x06: case 0x2A: case 0x2C:
			regtypes[i] = REGTYPE_POS;
			break;
		case 0x0A: case 0x0C:
		case 0x12: case 0x14: case 0x16:
		case 0x36:
			regtypes[i] = REGTYPE_JOYPORT;
			break;
		case 0x104:
		case 0x102:
			regtypes[i] = REGTYPE_PLANE;
			break;
		case 0x88: case 0x8A:
		case 0x8E: case 0x90: case 0x92: case 0x94:
		case 0x96:
		case 0x100:
			regtypes[i] |= REGTYPE_FORCE;
			break;
	}
    }
}

void init_eventtab (void)
{
	int i;
	
	currcycle = 0;
	for (i = 0; i < ev_max; i++) {
		eventtab[i].active = 0;
		eventtab[i].oldcycles = 0;
	}
	
	eventtab[ev_cia].handler = CIA_handler;
	eventtab[ev_hsync].handler = hsync_handler;
	eventtab[ev_hsync].evtime = maxhpos * CYCLE_UNIT + get_cycles ();
	eventtab[ev_hsync].active = 1;
	
	eventtab[ev_copper].handler = copper_handler;
	eventtab[ev_copper].active = 0;
	eventtab[ev_blitter].handler = blitter_handler;
	eventtab[ev_blitter].active = 0;
	eventtab[ev_disk].handler = DISK_handler;
	eventtab[ev_disk].active = 0;
	eventtab[ev_audio].handler = audio_evhandler;
	eventtab[ev_audio].active = 0;
	events_schedule ();
}

void customreset (void)
{
	int i;
	int zero = 0;

	if (!savestate_state)
	{
		currprefs.chipset_mask = changed_prefs.chipset_mask; 
		if ((currprefs.chipset_mask & CSMASK_AGA) == 0) {
			for (i = 0; i < 32; i++) {
				current_colors.color_uae_regs_ecs[i] = 0;
				current_colors.acolors[i] = xcolors[0];
			}
		} else {
			for (i = 0; i < 256; i++) {
				current_colors.color_regs_aga[i] = 0;
				current_colors.acolors[i] = CONVERT_RGB (zero);
			}
		}
		
		clx_sprmask = 0xFF;
		clxdat = 0;
		
		/* Clear the armed flags of all sprites.  */
		memset (spr, 0, sizeof spr);
		nr_armed = 0;
		
		dmacon = intena = 0;
		
		copcon = 0;
		DSKLEN (0, 0);
		
		bplcon0 = 0;
		planes_bplcon0=0;
		res_bplcon0=0;
		bplcon4 = 0x11; /* Get AGA chipset into ECS compatibility mode */
		bplcon3 = 0xC00;
		
		FMODE (0);
		CLXCON (0);
		lof = 0;
	}
		
	vsync_handler_cnt_disk_change = 0;
	
#ifdef USE_AUTOCONFIG
	expamem_reset ();
#endif
	
	DISK_reset ();
	CIA_reset ();
	if (! savestate_state)
		unset_special (~(SPCFLAG_BRK));
	
	vpos = 0;
	
	if (needmousehack ()) {
		mousehack_setdontcare();
	} else {
		mousestate = normal_mouse;
	}
	ievent_alive = 0;
	
	curr_sprite_entries[0].first_pixel = 0;
	curr_sprite_entries[1].first_pixel = 0;
	next_sprite_entry = 0;
	memset (spixels, 0, sizeof spixels);
	memset (&spixstate, 0, sizeof spixstate);
	
	bltstate = BLT_done;
	cop_state.state = COP_stop;
	diwstate = DIW_waiting_start;
	hdiwstate = DIW_waiting_start;
	currcycle = 0;
	
	new_beamcon0 = mainMenu_ntsc ? 0 : 0x20;
	init_hz ();
	
	audio_reset ();
	
	init_sprites ();
	
	init_hardware_frame ();
	reset_drawing ();
	
	reset_decisions ();
	
	init_regtypes ();	
	sprite_buffer_res = currprefs.chipset_mask & CSMASK_AGA ? RES_HIRES : RES_LORES;
	if (savestate_state == STATE_RESTORE)
	{
		uae_u16 v;
		uae_u32 vv;
		
		update_adkmasks ();
		INTENA (0);
		INTREQ (0);
		
		COPJMP1 (0);
		if (diwhigh)
			diwhigh_written = 1;
		v = bplcon0;
		BPLCON0 (0, 0);
		BPLCON0 (0, v);
		FMODE (fmode);
		
		if (!(currprefs.chipset_mask & CSMASK_AGA)) {
			for(i = 0 ; i < 32 ; i++)  {
				vv = current_colors.color_uae_regs_ecs[i];
				current_colors.color_uae_regs_ecs[i] = (unsigned short)-1;
				record_color_change (0, i, vv);
				remembered_color_entry = -1;
				current_colors.color_uae_regs_ecs[i] = vv;
				current_colors.acolors[i] = xcolors[vv];
			}
		} else {
			for(i = 0 ; i < 256 ; i++)  {
				vv = current_colors.color_regs_aga[i];
				current_colors.color_regs_aga[i] = -1;
				record_color_change (0, i, vv);
				remembered_color_entry = -1;
				current_colors.color_regs_aga[i] = vv;
				current_colors.acolors[i] = CONVERT_RGB(vv);
			}
		}
		CLXCON (clxcon);
		CLXCON2 (clxcon2);
		calcdiw ();
		write_log ("State restored\n");
		dumpcustom ();
		for (i = 0; i < 8; i++)
			nr_armed += spr[i].armed != 0;
	}
	expand_sprres ();
}

void dumpcustom (void)
{
    write_log ("DMACON: %x INTENA: %x INTREQ: %x VPOS: %x HPOS: %x\n", DMACONR(),
	       (unsigned int)intena, (unsigned int)intreq, (unsigned int)vpos, (unsigned int)current_hpos());
    write_log ("COP1LC: %08lx, COP2LC: %08lx\n", (unsigned long)cop1lc, (unsigned long)cop2lc);
    write_log ("DIWSTRT: %04x DIWSTOP: %04x DDFSTRT: %04x DDFSTOP: %04x\n",
	       (unsigned int)diwstrt, (unsigned int)diwstop, (unsigned int)ddfstrt, (unsigned int)ddfstop);
}


static _INLINE_ void gen_custom_tables (void)
{
	int i;
	for (i = 0; i < 256; i++) {
		sprtaba[i] = ((((i >> 7) & 1) << 0)
				| (((i >> 6) & 1) << 2)
				| (((i >> 5) & 1) << 4)
				| (((i >> 4) & 1) << 6)
				| (((i >> 3) & 1) << 8)
				| (((i >> 2) & 1) << 10)
				| (((i >> 1) & 1) << 12)
				| (((i >> 0) & 1) << 14));
		sprtabb[i] = sprtaba[i] << 1;
		sprite_ab_merge[i] = (((i & 15) ? 1 : 0)
				| ((i & 240) ? 2 : 0));
	}
	for (i = 0; i < 16; i++) {
		clxmask[i] = (((i & 1) ? 0xF : 0x3)
				| ((i & 2) ? 0xF0 : 0x30)
				| ((i & 4) ? 0xF00 : 0x300)
				| ((i & 8) ? 0xF000 : 0x3000));
		sprclx[i] = (((i & 0x3) == 0x3 ? 1 : 0)
				| ((i & 0x5) == 0x5 ? 2 : 0)
				| ((i & 0x9) == 0x9 ? 4 : 0)
				| ((i & 0x6) == 0x6 ? 8 : 0)
				| ((i & 0xA) == 0xA ? 16 : 0)
				| ((i & 0xC) == 0xC ? 32 : 0)) << 9;
	}
}

void custom_init (void)
{
    uaecptr pos;

    curr_sprite_entries = (struct sprite_entry *)xmalloc (MAX_SPRITE_ENTRY * sizeof (struct sprite_entry));
    curr_color_changes = (struct color_change *)xmalloc (MAX_COLOR_CHANGE * sizeof (struct color_change));

#ifdef USE_AUTOCONFIG
    pos = here ();

    org (RTAREA_BASE+0xFF70);
    calltrap (deftrap (mousehack_helper));
    dw (RTS);

    org (RTAREA_BASE+0xFFA0);
    calltrap (deftrap (timehack_helper));
    dw (RTS);

    org (pos);
#endif

    gen_custom_tables ();
    build_blitfilltable ();

    drawing_init ();

    mousestate = unknown_mouse;

    if (needmousehack ())
	mousehack_setfollow ();

    create_cycle_diagram_table ();
}

/* Custom chip memory bank */

static uae_u32 custom_lget (uaecptr) REGPARAM;
static uae_u32 custom_wget (uaecptr) REGPARAM;
static uae_u32 custom_bget (uaecptr) REGPARAM;
static void custom_lput (uaecptr, uae_u32) REGPARAM;
static void custom_wput (uaecptr, uae_u32) REGPARAM;
static void custom_bput (uaecptr, uae_u32) REGPARAM;

addrbank custom_bank = {
    custom_lget, custom_wget, custom_bget,
    custom_lput, custom_wput, custom_bput,
    default_xlate, default_check, NULL
};

static __inline__ uae_u32 REGPARAM2 custom_wget_1 (uaecptr addr)
{
    uae_u16 v;
    /* Kickstart 3.1 and AGA games occasionally get stuck while
     * waiting for specific VPOS.
     * *Dirty* hack to unstuck them.
     */
    static uae_u16 v_last_006;
    extern int kickstart;
    
    switch (addr & 0x1FE) {
     case 0x002: v = DMACONR (); break;
     case 0x004: v = VPOSR (); break;
     case 0x006:
       v = VHPOSR ();
       if (kickstart == 3) {
          if (v_last_006 == v)
             currcycle += 1;
          else
             v_last_006 = v;
       }
       break;

     case 0x008: v = DSKDATR (current_hpos ()); break;

     case 0x00A: v = JOY0DAT (); break;
     case 0x00C: v =  JOY1DAT (); break;
     case 0x00E: v =  CLXDAT (); break;
     case 0x010: v = ADKCONR (); break;

     case 0x012: v = POT0DAT (); break;
     case 0x016: v = POTGOR (); break;
     case 0x018: v = 0x3000 /* no data */; break;
     case 0x01A: v = DSKBYTR (current_hpos ()); break;
     case 0x01C: v = INTENAR (); break;
     case 0x01E: v = INTREQR (); break;
     case 0x07C: v = DENISEID (); break;

     case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
     case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
     case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
     case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
     case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
     case 0x1BC: case 0x1BE:
		v = COLOR_READ ((addr & 0x3E) / 2);
		break;

     default:
		/* reading write-only register causes write with last value in bus */
		v = last_custom_value;
		custom_wput (addr, v);
		last_custom_value = 0xffff;
		return v;
    }
    last_custom_value = v;
    return v;
}

uae_u32 REGPARAM2 custom_wget (uaecptr addr)
{
    sync_copper_with_cpu (current_hpos (), 1, addr);
    /* WinUAE code */
    if (addr & 1) {
       uae_u32 v;
       
       /* think about move.w $dff005,d0.. (68020+ only) */
       addr &= ~1;
       v = custom_wget_1 (addr) << 8;
       v |= custom_wget_1 (addr + 2) >> 8;
       return v;
    }
    /***************/
    return custom_wget_1 (addr);
}

uae_u32 REGPARAM2 custom_bget (uaecptr addr)
{
    return custom_wget (addr & 0xfffe) >> (addr & 1 ? 0 : 8);
}

uae_u32 REGPARAM2 custom_lget (uaecptr addr)
{
    return ((uae_u32)custom_wget (addr & 0xfffe) << 16) | custom_wget ((addr + 2) & 0xfffe);
}

void REGPARAM2 custom_wput_1 (int hpos, uaecptr addr, uae_u32 value)
{
    addr &= 0x1FE;
    last_custom_value = value;
    switch (addr) {
     case 0x020: DSKPTH (value); break;
     case 0x022: DSKPTL (value); break;
     case 0x024: DSKLEN (value, hpos); break;
     case 0x026: DSKDAT (value); break;

     case 0x02A: VPOSW (value); break;
     case 0x02E: COPCON (value); break;
     case 0x030: break;
     case 0x032: break;
     case 0x034: POTGO (value); break;
     case 0x040: BLTCON0 (value); break;
     case 0x042: BLTCON1 (value); break;

     case 0x044: BLTAFWM (value); break;
     case 0x046: BLTALWM (value); break;

     case 0x050: BLTAPTH (value); break;
     case 0x052: BLTAPTL (value); break;
     case 0x04C: BLTBPTH (value); break;
     case 0x04E: BLTBPTL (value); break;
     case 0x048: BLTCPTH (value); break;
     case 0x04A: BLTCPTL (value); break;
     case 0x054: BLTDPTH (value); break;
     case 0x056: BLTDPTL (value); break;

     case 0x058: BLTSIZE (value); break;

     case 0x064: BLTAMOD (value); break;
     case 0x062: BLTBMOD (value); break;
     case 0x060: BLTCMOD (value); break;
     case 0x066: BLTDMOD (value); break;

     case 0x070: BLTCDAT (value); break;
     case 0x072: BLTBDAT (value); break;
     case 0x074: BLTADAT (value); break;

     case 0x07E: DSKSYNC (value); break;

     case 0x080: COP1LCH (value); break;
     case 0x082: COP1LCL (value); break;
     case 0x084: COP2LCH (value); break;
     case 0x086: COP2LCL (value); break;

     case 0x088: COPJMP1 (value); break;
     case 0x08A: COPJMP2 (value); break;

     case 0x08E: DIWSTRT (hpos, value); break;
     case 0x090: DIWSTOP (hpos, value); break;
     case 0x092: DDFSTRT (hpos, value); break;
     case 0x094: DDFSTOP (hpos, value); break;

     case 0x096: DMACON (hpos, value); break;
     case 0x098: CLXCON (value); break;
     case 0x09A: INTENA (value); break;
     case 0x09C: INTREQ (value); break;
     case 0x09E: ADKCON (value); break;

     case 0x0A0: AUDxLCH (0, value); break;
     case 0x0A2: AUDxLCL (0, value); break;
     case 0x0A4: AUDxLEN (0, value); break;
     case 0x0A6: AUDxPER (0, value); break;
     case 0x0A8: AUDxVOL (0, value); break;
     case 0x0AA: AUDxDAT (0, value); break;

     case 0x0B0: AUDxLCH (1, value); break;
     case 0x0B2: AUDxLCL (1, value); break;
     case 0x0B4: AUDxLEN (1, value); break;
     case 0x0B6: AUDxPER (1, value); break;
     case 0x0B8: AUDxVOL (1, value); break;
     case 0x0BA: AUDxDAT (1, value); break;

     case 0x0C0: AUDxLCH (2, value); break;
     case 0x0C2: AUDxLCL (2, value); break;
     case 0x0C4: AUDxLEN (2, value); break;
     case 0x0C6: AUDxPER (2, value); break;
     case 0x0C8: AUDxVOL (2, value); break;
     case 0x0CA: AUDxDAT (2, value); break;

     case 0x0D0: AUDxLCH (3, value); break;
     case 0x0D2: AUDxLCL (3, value); break;
     case 0x0D4: AUDxLEN (3, value); break;
     case 0x0D6: AUDxPER (3, value); break;
     case 0x0D8: AUDxVOL (3, value); break;
     case 0x0DA: AUDxDAT (3, value); break;

     case 0x0E0: BPLPTH (hpos, value, 0); break;
     case 0x0E2: BPLPTL (hpos, value, 0); break;
     case 0x0E4: BPLPTH (hpos, value, 1); break;
     case 0x0E6: BPLPTL (hpos, value, 1); break;
     case 0x0E8: BPLPTH (hpos, value, 2); break;
     case 0x0EA: BPLPTL (hpos, value, 2); break;
     case 0x0EC: BPLPTH (hpos, value, 3); break;
     case 0x0EE: BPLPTL (hpos, value, 3); break;
     case 0x0F0: BPLPTH (hpos, value, 4); break;
     case 0x0F2: BPLPTL (hpos, value, 4); break;
     case 0x0F4: BPLPTH (hpos, value, 5); break;
     case 0x0F6: BPLPTL (hpos, value, 5); break;
     case 0x0F8: BPLPTH (hpos, value, 6); break;
     case 0x0FA: BPLPTL (hpos, value, 6); break;
     case 0x0FC: BPLPTH (hpos, value, 7); break;
     case 0x0FE: BPLPTL (hpos, value, 7); break;

     case 0x100: BPLCON0 (hpos, value); break;
     case 0x102: BPLCON1 (hpos, value); break;
     case 0x104: BPLCON2 (hpos, value); break;
     case 0x106: BPLCON3 (hpos, value); break;

     case 0x108: BPL1MOD (hpos, value); break;
     case 0x10A: BPL2MOD (hpos, value); break;
     case 0x10E: CLXCON2 (value); break;

     case 0x110: BPL1DAT (hpos, value); break;
     case 0x112: BPL2DAT (value); break;
     case 0x114: BPL3DAT (value); break;
     case 0x116: BPL4DAT (value); break;
     case 0x118: BPL5DAT (value); break;
     case 0x11A: BPL6DAT (value); break;
     case 0x11C: BPL7DAT (value); break;
     case 0x11E: BPL8DAT (value); break;

     case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
     case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
     case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
     case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
     case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
     case 0x1BC: case 0x1BE:
    	 COLOR_WRITE (hpos, value & 0xFFF, (addr & 0x3E) / 2);
    	 break;
     case 0x120: case 0x124: case 0x128: case 0x12C:
     case 0x130: case 0x134: case 0x138: case 0x13C:
    	 SPRxPTH (hpos, value, (addr - 0x120) / 4);
    	 break;
     case 0x122: case 0x126: case 0x12A: case 0x12E:
     case 0x132: case 0x136: case 0x13A: case 0x13E:
    	 SPRxPTL (hpos, value, (addr - 0x122) / 4);
    	 break;
     case 0x140: case 0x148: case 0x150: case 0x158:
     case 0x160: case 0x168: case 0x170: case 0x178:
    	 SPRxPOS (hpos, value, (addr - 0x140) / 8);
    	 break;
     case 0x142: case 0x14A: case 0x152: case 0x15A:
     case 0x162: case 0x16A: case 0x172: case 0x17A:
    	 SPRxCTL (hpos, value, (addr - 0x142) / 8);
    	 break;
     case 0x144: case 0x14C: case 0x154: case 0x15C:
     case 0x164: case 0x16C: case 0x174: case 0x17C:
    	 SPRxDATA (hpos, value, (addr - 0x144) / 8);
    	 break;
     case 0x146: case 0x14E: case 0x156: case 0x15E:
     case 0x166: case 0x16E: case 0x176: case 0x17E:
    	 SPRxDATB (hpos, value, (addr - 0x146) / 8);
    	 break;

     case 0x36: JOYTEST (value); break;
     case 0x5A: BLTCON0L (value); break;
     case 0x5C: BLTSIZV (value); break;
     case 0x5E: BLTSIZH (value); break;
     case 0x1E4: DIWHIGH (hpos, value); break;
     case 0x10C: BPLCON4 (hpos, value); break;
     case 0x1FC: FMODE (value); break;
    }
}

void REGPARAM2 custom_wput (uaecptr addr, uae_u32 value)
{
    int hpos = current_hpos ();

    sync_copper_with_cpu (hpos, 1, addr);
    custom_wput_1 (hpos, addr, value);
}

void REGPARAM2 custom_bput (uaecptr addr, uae_u32 value)
{
    static int warned = 0;
    /* Is this correct now? (There are people who bput things to the upper byte of AUDxVOL). */
    uae_u16 rval = (value << 8) | (value & 0xFF);
    custom_wput (addr, rval);
    if (!warned)
    {
    	write_log ("Byte put to custom register.\n");
    	warned++;
    }
}

void REGPARAM2 custom_lput(uaecptr addr, uae_u32 value)
{
    custom_wput (addr & 0xfffe, value >> 16);
    custom_wput ((addr + 2) & 0xfffe, (uae_u16)value);
}

void custom_prepare_savestate (void)
{
	/* force blitter to finish, no support for saving full blitter state yet */
	if (eventtab[ev_blitter].active) {
		unsigned int olddmacon = dmacon;
		dmacon |= DMA_BLITTER; /* ugh.. */
		blitter_handler ();
		dmacon = olddmacon;
	}
}

#define RB restore_u8 ()
#define RW restore_u16 ()
#define RL restore_u32 ()

uae_u8 *restore_custom (uae_u8 *src)
{
    uae_u16 dsklen, dskbytr, dskdatr, ru16;
    int dskpt;
    int i;

    audio_reset ();

    RL;				/* 000 chipset_mask */
    RW;				/* 000 ? */
    RW;				/* 002 DMACONR */
    RW;				/* 004 VPOSR */
    RW;				/* 006 VHPOSR */
    dskdatr = RW;		/* 008 DSKDATR */
    RW;				/* 00A JOY0DAT */
    RW;				/* 00C JOY1DAT */
    clxdat = RW;		/* 00E CLXDAT */
    RW;				/* 010 ADKCONR */
    RW;				/* 012 POT0DAT* */
    RW;				/* 014 POT1DAT* */
    RW;				/* 016 POTINP* */
    RW;				/* 018 SERDATR* */
    dskbytr = RW;		/* 01A DSKBYTR */
    RW;				/* 01C INTENAR */
    RW;				/* 01E INTREQR */
    dskpt =  RL;			/* 020-022 DSKPT */
    dsklen = RW;		/* 024 DSKLEN */
    RW;				/* 026 DSKDAT */
    RW;				/* 028 REFPTR */
    lof = RW;			/* 02A VPOSW */
    RW;				/* 02C VHPOSW */
    ru16=RW; COPCON(ru16);	/* 02E COPCON */
    RW;				/* 030 SERDAT* */
    RW;				/* 032 SERPER* */
    ru16=RW; POTGO(ru16);		/* 034 POTGO */
    RW;				/* 036 JOYTEST* */
    RW;				/* 038 STREQU */
    RW;				/* 03A STRVHBL */
    RW;				/* 03C STRHOR */
    RW;				/* 03E STRLONG */
    ru16=RW; BLTCON0(ru16);	/* 040 BLTCON0 */
    ru16=RW; BLTCON1(ru16);	/* 042 BLTCON1 */
    ru16=RW; BLTAFWM(ru16);	/* 044 BLTAFWM */
    ru16=RW; BLTALWM(ru16);	/* 046 BLTALWM */
    bltcpt=RL; // u32=RL; BLTCPTH(u32);	/* 048-04B BLTCPT */
    bltbpt=RL; // u32=RL; BLTBPTH(u32);	/* 04C-04F BLTBPT */
    bltapt=RL; // u32=RL; BLTAPTH(u32);	/* 050-053 BLTAPT */
    bltdpt=RL; // u32=RL; BLTDPTH(u32);	/* 054-057 BLTDPT */
    RW;				/* 058 BLTSIZE */
    RW;				/* 05A BLTCON0L */
    oldvblts = RW;		/* 05C BLTSIZV */
    RW;				/* 05E BLTSIZH */
    ru16=RW; BLTCMOD(ru16);	/* 060 BLTCMOD */
    ru16=RW; BLTBMOD(ru16);	/* 062 BLTBMOD */
    ru16=RW; BLTAMOD(ru16);	/* 064 BLTAMOD */
    ru16=RW; BLTDMOD(ru16);	/* 066 BLTDMOD */
    RW;				/* 068 ? */
    RW;				/* 06A ? */
    RW;				/* 06C ? */
    RW;				/* 06E ? */
    ru16=RW; BLTCDAT(ru16);	/* 070 BLTCDAT */
    ru16=RW; BLTBDAT(ru16);	/* 072 BLTBDAT */
    ru16=RW; BLTADAT(ru16);	/* 074 BLTADAT */
    RW;				/* 076 ? */
    RW;				/* 078 ? */
    RW;				/* 07A ? */
    RW;				/* 07C LISAID */
    ru16=RW; DSKSYNC(ru16);	/* 07E DSKSYNC */
    cop1lc =  RL;		/* 080/082 COP1LC */
    cop2lc =  RL;		/* 084/086 COP2LC */
    RW;				/* 088 ? */
    RW;				/* 08A ? */
    RW;				/* 08C ? */
    diwstrt = RW;		/* 08E DIWSTRT */
    diwstop = RW;		/* 090 DIWSTOP */
    ddfstrt = RW;		/* 092 DDFSTRT */
    ddfstop = RW;		/* 094 DDFSTOP */
    dmacon = RW & ~(0x2000|0x4000); /* 096 DMACON */
    ru16=RW; CLXCON(ru16);	/* 098 CLXCON */
    intena = RW;		/* 09A INTENA */
    intreq = RW;		/* 09C INTREQ */
    adkcon = RW;		/* 09E ADKCON */
    SET_INTERRUPT();
    for (i = 0; i < 8; i++)
	bpl[i].pt = RL;
    bplcon0 = RW;		/* 100 BPLCON0 */
    bplcon1 = RW;		/* 102 BPLCON1 */
    bplcon2 = RW;		/* 104 BPLCON2 */
    bplcon3 = RW;		/* 106 BPLCON3 */
    bpl1mod = RW;		/* 108 BPL1MOD */
    bpl2mod = RW;		/* 10A BPL2MOD */
    bplcon4 = RW;		/* 10C BPLCON4 */
    clxcon2 = RW;		/* 10E CLXCON2* */
    for(i = 0; i < 8; i++)
	RW;			/*     BPLXDAT */
    for(i = 0; i < 32; i++)
	current_colors.color_uae_regs_ecs[i] = RW; /* 180 COLORxx */
    RW;				/* 1C0 ? */
    RW;				/* 1C2 ? */
    RW;				/* 1C4 ? */
    RW;				/* 1C6 ? */
    RW;				/* 1C8 ? */
    RW;				/* 1CA ? */
    RW;				/* 1CC ? */
    RW;				/* 1CE ? */
    RW;				/* 1D0 ? */
    RW;				/* 1D2 ? */
    RW;				/* 1D4 ? */
    RW;				/* 1D6 ? */
    RW;				/* 1D8 ? */
    RW;				/* 1DA ? */
    new_beamcon0 = RW;		/* 1DC BEAMCON0 */
    RW;				/* 1DE ? */

    RW;				/* 1E0 ? */
    RW;				/* 1E2 ? */
    RW;				/* 1E4 ? */

    RW;				/* 1E6 ? */
    RW;				/* 1E8 ? */
    RW;				/* 1EA ? */
    RW;				/* 1EC ? */
    RW;				/* 1EE ? */
    RW;				/* 1F0 ? */
    RW;				/* 1F2 ? */
    RW;				/* 1F4 ? */
    RW;				/* 1F6 ? */
    RW;				/* 1F8 ? */
    RW;				/* 1FA ? */
    fmode = RW;			/* 1FC FMODE */
    RW;				/* 1FE ? */

    DISK_restore_custom (dskpt, dsklen, dskdatr, dskbytr);

    return src;
}

#define SB save_u8
#define SW save_u16
#define SL save_u32

uae_u8 *save_custom (int *len)
{
    uae_u8 *dstbak, *dst;
    int i;
    uae_u32 dskpt;
    uae_u16 dsklen, dsksync, dskdatr, dskbytr;

    DISK_save_custom (&dskpt, &dsklen, &dsksync, &dskdatr, &dskbytr);
    dstbak = dst = (uae_u8 *)malloc (8+256*2);
    SL (0);			/* 000 chipset_mask */
    SW (0);			/* 000 ? */
    SW (dmacon);		/* 002 DMACONR */
    SW (VPOSR());		/* 004 VPOSR */
    SW (VHPOSR());		/* 006 VHPOSR */
    SW (dskdatr);		/* 008 DSKDATR */
    SW (JOY0DAT());		/* 00A JOY0DAT */
    SW (JOY1DAT());		/* 00C JOY1DAT */
    SW (clxdat);		/* 00E CLXDAT */
    SW (ADKCONR());		/* 010 ADKCONR */
    SW (POT0DAT());		/* 012 POT0DAT */
    SW (POT0DAT());		/* 014 POT1DAT */
    SW (0)	;		/* 016 POTINP * */
    SW (0);			/* 018 SERDATR * */
    SW (dskbytr);		/* 01A DSKBYTR */
    SW (INTENAR());		/* 01C INTENAR */
    SW (INTREQR());		/* 01E INTREQR */
    SL (dskpt);			/* 020-023 DSKPT */
    SW (dsklen);		/* 024 DSKLEN */
    SW (0);			/* 026 DSKDAT */
    SW (0);			/* 028 REFPTR */
    SW (lof);			/* 02A VPOSW */
    SW (0);			/* 02C VHPOSW */
    SW (copcon);		/* 02E COPCON */
    SW (0); //serper);		/* 030 SERDAT * */
    SW (0); //serdat);		/* 032 SERPER * */
    SW (potgo_value);		/* 034 POTGO */
    SW (0);			/* 036 JOYTEST * */
    SW (0);			/* 038 STREQU */
    SW (0);			/* 03A STRVBL */
    SW (0);			/* 03C STRHOR */
    SW (0);			/* 03E STRLONG */
    SW (bltcon0);		/* 040 BLTCON0 */
    SW (bltcon1);		/* 042 BLTCON1 */
    SW (blt_info.bltafwm);	/* 044 BLTAFWM */
    SW (blt_info.bltalwm);	/* 046 BLTALWM */
    SL (bltcpt);		/* 048-04B BLTCPT */
    SL (bltbpt);		/* 04C-04F BLTCPT */
    SL (bltapt);		/* 050-043 BLTCPT */
    SL (bltdpt);		/* 054-057 BLTCPT */
    SW (0);			/* 058 BLTSIZE */
    SW (0);			/* 05A BLTCON0L (use BLTCON0 instead) */
    SW (oldvblts);		/* 05C BLTSIZV */
    SW (blt_info.hblitsize);	/* 05E BLTSIZH */
    SW (blt_info.bltcmod);	/* 060 BLTCMOD */
    SW (blt_info.bltbmod);	/* 062 BLTBMOD */
    SW (blt_info.bltamod);	/* 064 BLTAMOD */
    SW (blt_info.bltdmod);	/* 066 BLTDMOD */
    SW (0);			/* 068 ? */
    SW (0);			/* 06A ? */
    SW (0);			/* 06C ? */
    SW (0);			/* 06E ? */
    SW (blt_info.bltcdat);	/* 070 BLTCDAT */
    SW (blt_info.bltbdat);	/* 072 BLTBDAT */
    SW (blt_info.bltadat);	/* 074 BLTADAT */
    SW (0);			/* 076 ? */
    SW (0);			/* 078 ? */
    SW (0);			/* 07A ? */
    SW (DENISEID());		/* 07C DENISEID/LISAID */
    SW (dsksync);		/* 07E DSKSYNC */
    SL (cop1lc);		/* 080-083 COP1LC */
    SL (cop2lc);		/* 084-087 COP2LC */
    SW (0);			/* 088 ? */
    SW (0);			/* 08A ? */
    SW (0);			/* 08C ? */
    SW (diwstrt);		/* 08E DIWSTRT */
    SW (diwstop);		/* 090 DIWSTOP */
    SW (ddfstrt);		/* 092 DDFSTRT */
    SW (ddfstop);		/* 094 DDFSTOP */
    SW (dmacon);		/* 096 DMACON */
    SW (clxcon);		/* 098 CLXCON */
    SW (intena);		/* 09A INTENA */
    SW (intreq);		/* 09C INTREQ */
    SW (adkcon);		/* 09E ADKCON */
    for (i = 0; i < 8; i++)
	SL (bpl[i].pt);		/* 0E0-0FE BPLxPT */
    SW (bplcon0);		/* 100 BPLCON0 */
    SW (bplcon1);		/* 102 BPLCON1 */
    SW (bplcon2);		/* 104 BPLCON2 */
    SW (bplcon3);		/* 106 BPLCON3 */
    SW (bpl1mod);		/* 108 BPL1MOD */
    SW (bpl2mod);		/* 10A BPL2MOD */
    SW (bplcon4);		/* 10C BPLCON4 */
    SW (clxcon2);		/* 10E CLXCON2 */
    for (i = 0;i < 8; i++)
	SW (0);			/* 110 BPLxDAT */
    for ( i = 0; i < 32; i++)
	SW (current_colors.color_uae_regs_ecs[i]); /* 180-1BE COLORxx */
    SW (0);			/* 1C0 */
    SW (0);			/* 1C2 */
    SW (0);			/* 1C4 */
    SW (0);			/* 1C6 */
    SW (0);			/* 1C8 */
    SW (0);			/* 1CA */
    SW (0);			/* 1CC */
    SW (0);			/* 1CE */
    SW (0);			/* 1D0 */
    SW (0);			/* 1D2 */
    SW (0);			/* 1D4 */
    SW (0);			/* 1D6 */
    SW (0);			/* 1D8 */
    SW (0);			/* 1DA */
    SW (beamcon0);		/* 1DC BEAMCON0 */
    SW (0);			/* 1DE */

    SW (0);			/* 1E0 */
    SW (0);			/* 1E2 */
    SW (0);			/* 1E4 */

    SW (0);			/* 1E6 */
    SW (0);			/* 1E8 */
    SW (0);			/* 1EA */
    SW (0);			/* 1EC */
    SW (0);			/* 1EE */
    SW (0);			/* 1F0 */
    SW (0);			/* 1F2 */
    SW (0);			/* 1F4 */
    SW (0);			/* 1F6 */
    SW (0);			/* 1F8 */
    SW (0);			/* 1FA */
    SW (fmode);			/* 1FC FMODE */
    SW (0xffff);		/* 1FE */

    *len = dst - dstbak;
    return dstbak;
}

uae_u8 *restore_custom_agacolors (uae_u8 *src)
{
    int i;

    for (i = 0; i < 256; i++)
	current_colors.color_regs_aga[i] = RL;
    return src;
}

uae_u8 *save_custom_agacolors (int *len)
{
    uae_u8 *dstbak, *dst;
    int i;

    dstbak = dst = (uae_u8 *)malloc (256*4);
    for (i = 0; i < 256; i++)
	SL (current_colors.color_regs_aga[i]);
    *len = dst - dstbak;
    return dstbak;
}

uae_u8 *restore_custom_sprite (uae_u8 *src, int num)
{
    spr[num].pt = RL;		/* 120-13E SPRxPT */
    sprpos[num] = RW;		/* 1x0 SPRxPOS */
    sprctl[num] = RW;		/* 1x2 SPRxPOS */
    sprdata[num][0] = RW;	/* 1x4 SPRxDATA */
    sprdatb[num][0] = RW;	/* 1x6 SPRxDATB */
    sprdata[num][1] = RW;
    sprdatb[num][1] = RW;
    sprdata[num][2] = RW;
    sprdatb[num][2] = RW;
    sprdata[num][3] = RW;
    sprdatb[num][3] = RW;
    spr[num].armed = RB;
    return src;
}

uae_u8 *save_custom_sprite(int *len, int num)
{
    uae_u8 *dstbak, *dst;

    dstbak = dst = (uae_u8 *)malloc (25);
    SL (spr[num].pt);		/* 120-13E SPRxPT */
    SW (sprpos[num]);		/* 1x0 SPRxPOS */
    SW (sprctl[num]);		/* 1x2 SPRxPOS */
    SW (sprdata[num][0]);	/* 1x4 SPRxDATA */
    SW (sprdatb[num][0]);	/* 1x6 SPRxDATB */
    SW (sprdata[num][1]);
    SW (sprdatb[num][1]);
    SW (sprdata[num][2]);
    SW (sprdatb[num][2]);
    SW (sprdata[num][3]);
    SW (sprdatb[num][3]);
    SB (spr[num].armed ? 1 : 0);
    *len = dst - dstbak;
    return dstbak;
}
