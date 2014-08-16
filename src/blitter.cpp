 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Custom chip emulation
  *
  * (c) 1995 Bernd Schmidt, Alessandro Bissacco
  */


//#define USE_BLITTER_EXTRA_INLINE
#define STOP_WHEN_NASTY

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "uae.h"
#include "options.h"
#include "debug_uae4all.h"
#include "events.h"
#include "memory-uae.h"
#include "custom.h"
#include "m68k/m68k_intrf.h"
#include "blitter.h"
#include "blit.h"

#ifdef USE_BLITTER_EXTRA_INLINE
#define _INLINE_ __inline__
#else
#define _INLINE_ 
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

uae_u16 bltcon0, bltcon1;
uae_u32 bltapt, bltbpt, bltcpt, bltdpt;
uae_u32 preva = 0, prevb = 0;

int blinea_shift, blitsign;
static uae_u16 blinea, blineb;
static int blitline, blitfc, blitfill, blitife, blitsing, blitdesc;
static int blitonedot;
static int blit_ch;

struct bltinfo blt_info;

static uae_u8 blit_filltable[256][4][2];
uae_u32 blit_masktable[BLITTER_MAX_WORDS];
enum blitter_states bltstate;

static long int blit_cyclecounter;
// blitter_slowdown doesn't work at the moment
//static int blit_slowdown;

static long blit_firstline_cycles;
static long blit_first_cycle;
static int blit_last_cycle, blit_dmacount, blit_dmacount2;
static int blit_nod;
static const uae_u8 *blit_diag;
static int ddat1use;

// New vars for blitter mode "partial"
int blitter_in_partial_mode = 0;        // Flag for new mode active
static int blit_total_required_cycles;  // Estimated number of cycles of entire blitter op
static int blit_cycles_per_op;          // Number of cycles per blitter op
static int blit_cycles_per_vsize;       // Number of cycles per line
static int blit_vblitsize_done;         // Number of vblitsize-lines are done
static unsigned long blit_cycle_at_start;
static unsigned long blit_init_cycles;  // Cycles required for blitter initialisation
static unsigned long blit_cycle_current;
static unsigned long blit_cycle_entered_wait;    // At this cycle, blitter entered in wait-state because of disabled DMA


/*
Blitter Idle Cycle:

Cycles that are free cycles (available for CPU) and
are not used by any other Agnus DMA channel. Blitter
idle cycle is not "used" by blitter, CPU can still use
it normally if it needs the bus.

same in both block and line modes

number of cycles, initial cycle, main cycle
*/

#define DIAGSIZE 10

static const uae_u8 blit_cycle_diagram[][DIAGSIZE] =
{
	{ 2, 0,0,	    0,0 },		/* 0   -- */
	{ 2, 0,0,	    0,4 },		/* 1   -D */
	{ 2, 0,3,	    0,3 },		/* 2   -C */
	{ 3, 0,3,0,	    0,3,4 },    /* 3  -CD */
	{ 3, 0,2,0,	    0,2,0 },    /* 4  -B- */
	{ 3, 0,2,0,	    0,2,4 },    /* 5  -BD */
	{ 3, 0,2,3,	    0,2,3 },    /* 6  -BC */
	{ 4, 0,2,3,0,   0,2,3,4 },  /* 7 -BCD */
	{ 2, 1,0,	    1,0 },		/* 8   A- */
	{ 2, 1,0,	    1,4 },		/* 9   AD */
	{ 2, 1,3,	    1,3 },		/* A   AC */
	{ 3, 1,3,0,	    1,3,4, },	/* B  ACD */
	{ 3, 1,2,0,	    1,2,0 },	/* C  AB- */
	{ 3, 1,2,0,	    1,2,4 },	/* D  ABD */
	{ 3, 1,2,3,	    1,2,3 },	/* E  ABC */
	{ 4, 1,2,3,0,   1,2,3,4 }	/* F ABCD */
};

/*

following 4 channel combinations in fill mode have extra
idle cycle added (still requires free bus cycle)

*/

static const uae_u8 blit_cycle_diagram_fill[][DIAGSIZE] =
{
	{ 0 },						/* 0 */
	{ 3, 0,0,0,	    0,4,0 },	/* 1 */
	{ 0 },						/* 2 */
	{ 0 },						/* 3 */
	{ 0 },						/* 4 */
	{ 4, 0,2,0,0,   0,2,4,0 },	/* 5 */
	{ 0 },						/* 6 */
	{ 0 },						/* 7 */
	{ 0 },						/* 8 */
	{ 3, 1,0,0,	    1,4,0 },	/* 9 */
	{ 0 },						/* A */
	{ 0 },						/* B */
	{ 0 },						/* C */
	{ 4, 1,2,0,0,   1,2,4,0 },	/* D */
	{ 0 },						/* E */
	{ 0 },						/* F */
};

/*
-C-D C-D- ... C-D- --

line draw takes 4 cycles (-C-D)
idle cycles do the same as above, 2 dma fetches
(read from C, write to D, but see below)

Oddities:

- first word is written to address pointed by BLTDPT
but all following writes go to address pointed by BLTCPT!
(some kind of internal copy because all bus cyles are
using normal BLTDDAT)
- BLTDMOD is ignored by blitter (BLTCMOD is used)
- state of D-channel enable bit does not matter!
- disabling A-channel freezes the content of BPLAPT
- C-channel disabled: nothing is written

There is one tricky situation, writing to DFF058 just before
last D write cycle (which is normally free) does not disturb
blitter operation, final D is still written correctly before
blitter starts normally (after 2 idle cycles)

There is at least one demo that does this..

*/

// 5 = internal "processing cycle"
static const uae_u8 blit_cycle_diagram_line[] =
{
	4, 0,3,5,4,	    0,3,5,4
};

void build_blitfilltable (void)
{
	unsigned int d, fillmask;
	int i;

	for (i = 0; i < BLITTER_MAX_WORDS; i++)
		blit_masktable[i] = 0xFFFF;

	for (d = 0; d < 256; d++) {
		for (i = 0; i < 4; i++) {
			int fc = i & 1;
			uae_u8 data = d;
			for (fillmask = 1; fillmask != 0x100; fillmask <<= 1) {
				uae_u16 tmp = data;
				if (fc) {
					if (i & 2)
						data |= fillmask;
					else
						data ^= fillmask;
				}
				if (tmp & fillmask) fc = !fc;
			}
			blit_filltable[d][i][0] = data;
			blit_filltable[d][i][1] = fc;
		}
	}
}

static _INLINE_ void blitter_dofast(void)
{
    int i,j;
    uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
    uae_u8 mt = bltcon0 & 0xFF;

    blit_masktable[BLITTER_MAX_WORDS - 1] = blt_info.bltafwm;
    blit_masktable[BLITTER_MAX_WORDS - blt_info.hblitsize] &= blt_info.bltalwm;

    if (bltcon0 & 0x800) {
	bltadatptr = bltapt;
	bltapt += ((blt_info.hblitsize*2) + blt_info.bltamod)*blt_info.vblitsize;
    }
    if (bltcon0 & 0x400) {
	bltbdatptr = bltbpt;
	bltbpt += ((blt_info.hblitsize*2) + blt_info.bltbmod)*blt_info.vblitsize;
    }
    if (bltcon0 & 0x200) {
	bltcdatptr = bltcpt;
	bltcpt += ((blt_info.hblitsize*2) + blt_info.bltcmod)*blt_info.vblitsize;
    }
    if (bltcon0 & 0x100) {
	bltddatptr = bltdpt;
	bltdpt += ((blt_info.hblitsize*2) + blt_info.bltdmod)*blt_info.vblitsize;
    }

    if (blitfunc_dofast[mt] && !blitfill)
	(*blitfunc_dofast[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
    else {
	uae_u32 blitbhold = blt_info.bltbhold;
	uaecptr dstp = 0;
	uae_u32 *blit_masktable_p = blit_masktable + BLITTER_MAX_WORDS - blt_info.hblitsize;

	for (j = blt_info.vblitsize; j--;) {
	    blitfc = !!(bltcon1 & 0x4);
	    for (i = blt_info.hblitsize; i--;) {
		uae_u32 bltadat, blitahold;
		if (bltadatptr) {
		    blt_info.bltadat = bltadat = CHIPMEM_WGET_CUSTOM (bltadatptr);
		    bltadatptr += 2;
		} else
		    bltadat = blt_info.bltadat;
		bltadat &= blit_masktable_p[i];
		blitahold = (((uae_u32)preva << 16) | bltadat) >> blt_info.blitashift;
		preva = bltadat;

		if (bltbdatptr) {
		    uae_u16 bltbdat;
		    blt_info.bltbdat = bltbdat = CHIPMEM_WGET_CUSTOM (bltbdatptr);
		    bltbdatptr += 2;
		    blitbhold = (((uae_u32)prevb << 16) | bltbdat) >> blt_info.blitbshift;
		    prevb = bltbdat;
		}
		if (bltcdatptr) {
		    blt_info.bltcdat = CHIPMEM_WGET_CUSTOM (bltcdatptr);
		    bltcdatptr += 2;
		}
		if (dstp) 
		  CHIPMEM_WPUT_CUSTOM (dstp, blt_info.bltddat);
		blt_info.bltddat = blit_func (blitahold, blitbhold, blt_info.bltcdat, mt);
		if (blitfill) {
		    uae_u16 d = blt_info.bltddat;
		    int ifemode = blitife ? 2 : 0;
		    int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
		    blt_info.bltddat = (blit_filltable[d & 255][ifemode + blitfc][0]
					+ (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
		    blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
		}
		if (blt_info.bltddat)
		    blt_info.blitzero = 0;
		if (bltddatptr) {
		    dstp = bltddatptr;
		    bltddatptr += 2;
		}
	    }
	    if (bltadatptr) bltadatptr += blt_info.bltamod;
	    if (bltbdatptr) bltbdatptr += blt_info.bltbmod;
	    if (bltcdatptr) bltcdatptr += blt_info.bltcmod;
	    if (bltddatptr) bltddatptr += blt_info.bltdmod;
	}
	if (dstp)
	  CHIPMEM_WPUT_CUSTOM (dstp, blt_info.bltddat);
	blt_info.bltbhold = blitbhold;
    }
    blit_masktable[BLITTER_MAX_WORDS - 1] = 0xFFFF;
    blit_masktable[BLITTER_MAX_WORDS - blt_info.hblitsize] = 0xFFFF;

    bltstate = BLT_done;
}

static _INLINE_ void blitter_dofast_desc(void)
{
    int i,j;
    uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
    uae_u8 mt = bltcon0 & 0xFF;

    blit_masktable[BLITTER_MAX_WORDS - 1] = blt_info.bltafwm;
    blit_masktable[BLITTER_MAX_WORDS - blt_info.hblitsize] &= blt_info.bltalwm;

    if (bltcon0 & 0x800) {
	bltadatptr = bltapt;
	bltapt -= ((blt_info.hblitsize*2) + blt_info.bltamod)*blt_info.vblitsize;
    }
    if (bltcon0 & 0x400) {
	bltbdatptr = bltbpt;
	bltbpt -= ((blt_info.hblitsize*2) + blt_info.bltbmod)*blt_info.vblitsize;
    }
    if (bltcon0 & 0x200) {
	bltcdatptr = bltcpt;
	bltcpt -= ((blt_info.hblitsize*2) + blt_info.bltcmod)*blt_info.vblitsize;
    }
    if (bltcon0 & 0x100) {
	bltddatptr = bltdpt;
	bltdpt -= ((blt_info.hblitsize*2) + blt_info.bltdmod)*blt_info.vblitsize;
    }
    if (blitfunc_dofast_desc[mt] && !blitfill)
		(*blitfunc_dofast_desc[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
    else {
	uae_u32 blitbhold = blt_info.bltbhold;
	uaecptr dstp = 0;
	uae_u32 *blit_masktable_p = blit_masktable + BLITTER_MAX_WORDS - blt_info.hblitsize;

	for (j = blt_info.vblitsize; j--;) {
			blitfc = !!(bltcon1 & 0x4);
	    for (i = blt_info.hblitsize; i--;) {
				uae_u32 bltadat, blitahold;
				if (bltadatptr) {
		    blt_info.bltadat = bltadat = CHIPMEM_WGET_CUSTOM (bltadatptr);
					bltadatptr -= 2;
				} else
					bltadat = blt_info.bltadat;
				bltadat &= blit_masktable_p[i];
				blitahold = (((uae_u32)bltadat << 16) | preva) >> blt_info.blitdownashift;
				preva = bltadat;

				if (bltbdatptr) {
		    uae_u16 bltbdat;
		    blt_info.bltbdat = bltbdat = CHIPMEM_WGET_CUSTOM (bltbdatptr);
					bltbdatptr -= 2;
					blitbhold = (((uae_u32)bltbdat << 16) | prevb) >> blt_info.blitdownbshift;
					prevb = bltbdat;
				}

				if (bltcdatptr) {
		    blt_info.bltcdat = blt_info.bltbdat = CHIPMEM_WGET_CUSTOM (bltcdatptr);
					bltcdatptr -= 2;
				}
				if (dstp)
		  CHIPMEM_WPUT_CUSTOM (dstp, blt_info.bltddat);
		blt_info.bltddat = blit_func (blitahold, blitbhold, blt_info.bltcdat, mt);
		if (blitfill) {
		    uae_u16 d = blt_info.bltddat;
		    int ifemode = blitife ? 2 : 0;
		    int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
		    blt_info.bltddat = (blit_filltable[d & 255][ifemode + blitfc][0]
					+ (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
		    blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
		}
		if (blt_info.bltddat)
		    blt_info.blitzero = 0;
		if (bltddatptr) {
		    dstp = bltddatptr;
		    bltddatptr -= 2;
		}
	    }
	    if (bltadatptr) bltadatptr -= blt_info.bltamod;
	    if (bltbdatptr) bltbdatptr -= blt_info.bltbmod;
	    if (bltcdatptr) bltcdatptr -= blt_info.bltcmod;
	    if (bltddatptr) bltddatptr -= blt_info.bltdmod;
	}
	if (dstp)
	  CHIPMEM_WPUT_CUSTOM (dstp, blt_info.bltddat);
		blt_info.bltbhold = blitbhold;
	}
    blit_masktable[BLITTER_MAX_WORDS - 1] = 0xFFFF;
    blit_masktable[BLITTER_MAX_WORDS - blt_info.hblitsize] = 0xFFFF;

    bltstate = BLT_done;
}

static __inline__ void blitter_read(void)
{
	if (bltcon0 & 0x200) {
      blt_info.bltcdat = CHIPMEM_WGET_CUSTOM(bltcpt);
	}
}

static __inline__ void blitter_write(void)
{
	if (blt_info.bltddat)
		blt_info.blitzero = 0;
	/* D-channel state has no effect on linedraw, but C must be enabled or nothing is drawn! */
	if (bltcon0 & 0x200) {
      CHIPMEM_WPUT_CUSTOM(bltdpt, blt_info.bltddat);
	}
}

static __inline__ void blitter_line_incx(void)
{
    if (++blinea_shift == 16) {
	blinea_shift = 0;
	bltcpt += 2;
    }
}

static __inline__ void blitter_line_decx(void)
{
    if (blinea_shift-- == 0) {
	blinea_shift = 15;
	bltcpt -= 2;
    }
}

static __inline__ void blitter_line_decy(void)
{
    bltcpt -= blt_info.bltcmod;
    blitonedot = 0;
}

static __inline__ void blitter_line_incy(void)
{
    bltcpt += blt_info.bltcmod;
    blitonedot = 0;
}

static _INLINE_ int blitter_line(void)
{
	uae_u16 blitahold = (blinea & blt_info.bltafwm) >> blinea_shift;

	blt_info.bltbhold = (blineb & 1) ? 0xFFFF : 0;
	int blitlinepixel = !blitsing || (blitsing && !blitonedot);
	blt_info.bltddat = blit_func (blitahold, blt_info.bltbhold, blt_info.bltcdat, bltcon0 & 0xFF);
	blitonedot++;

	if (bltcon0 & 0x800) {
		if (blitsign)
			bltapt += (uae_s16)blt_info.bltbmod;
		else
			bltapt += (uae_s16)blt_info.bltamod;
	}

	if (!blitsign) {
		if (bltcon1 & 0x10) {
			if (bltcon1 & 0x8)
				blitter_line_decy ();
			else
				blitter_line_incy ();
		} else {
			if (bltcon1 & 0x8)
				blitter_line_decx ();
			else
				blitter_line_incx ();
		}
	}
	if (bltcon1 & 0x10) {
		if (bltcon1 & 0x4)
			blitter_line_decx ();
		else
			blitter_line_incx ();
	} else {
		if (bltcon1 & 0x4)
			blitter_line_decy ();
		else
			blitter_line_incy ();
	}

	blitsign = 0 > (uae_s16)bltapt;
	return blitlinepixel;
}

static __inline__ void blitter_nxline(void)
{
	blineb = (blineb << 1) | (blineb >> 15);
	blt_info.vblitsize--;
}

static __inline__ void blitter_done ()
{
	ddat1use = 0;
	bltstate = BLT_done;
	INTREQ(0x8040);
	blitter_done_notify ();
	eventtab[ev_blitter].active = 0;
	unset_special (SPCFLAG_BLTNASTY);
}

static _INLINE_ void actually_do_blit(void)
{
    if (blitline) {
	do {
			blitter_read ();
			if (ddat1use)
				bltdpt = bltcpt;
			ddat1use = 1;
			if (blitter_line ()) {
				blitter_write ();
			}
			blitter_nxline ();
			if (blt_info.vblitsize == 0)
				bltstate = BLT_done;
		} while (bltstate != BLT_done);
		if(!blitter_in_partial_mode)
		  bltdpt = bltcpt;
	} else {
		if (blitdesc)
			blitter_dofast_desc ();
		else
			blitter_dofast ();
		bltstate = BLT_done;
	}
}

static __inline__ void blitter_doit(void)
{
  actually_do_blit ();
  blitter_done();
}

void blitter_handler(void)
{
	static int blitter_stuck;

	if (!dmaen (DMA_BLITTER)) {
	eventtab[ev_blitter].active = 1;
	eventtab[ev_blitter].oldcycles = get_cycles ();
	eventtab[ev_blitter].evtime = 10 * CYCLE_UNIT + get_cycles (); /* wait a little */
		blitter_stuck++;
		if (blitter_stuck < 20000 || !currprefs.immediate_blits)
			return; /* gotta come back later. */
		/* "free" blitter in immediate mode if it has been "stuck" ~3 frames
		* fixes some JIT game incompatibilities
		*/
	}
	blitter_stuck = 0;

  if(blitter_in_partial_mode)
  {
    blitter_do_partial(1);
    return;
  }

// blitter_slowdown doesn't work at the moment
//	if (blit_slowdown > 0 && !currprefs.immediate_blits) {
//	eventtab[ev_blitter].active = 1;
//	eventtab[ev_blitter].oldcycles = get_cycles ();
//	eventtab[ev_blitter].evtime = blit_slowdown * CYCLE_UNIT + get_cycles (); /* wait a little */
//		blit_slowdown = -1;
//		return;
//	}

  blitter_doit();
}

static __inline__ void blit_bltset (int con)
{
	int i;

	if (con & 2) {
		blitdesc = bltcon1 & 2;
		blt_info.blitbshift = bltcon1 >> 12;
		blt_info.blitdownbshift = 16 - blt_info.blitbshift;
	}

	if (con & 1) {
		blt_info.blitashift = bltcon0 >> 12;
		blt_info.blitdownashift = 16 - blt_info.blitashift;
	}

	blit_ch = (bltcon0 & 0x0f00) >> 8;
	blitline = bltcon1 & 1;
	blitfill = !!(bltcon1 & 0x18);

	if (blitline) {
		blit_diag = blit_cycle_diagram_line;
	} else {
		if (con & 2) {
			blitfc = !!(bltcon1 & 0x4);
			blitife = !!(bltcon1 & 0x8);
			if ((bltcon1 & 0x18) == 0x18) {
				blitife = 0;
			}
		}
		blit_diag = blitfill && blit_cycle_diagram_fill[blit_ch][0] ? blit_cycle_diagram_fill[blit_ch] : blit_cycle_diagram[blit_ch];
	}

	blit_dmacount = blit_dmacount2 = 0;
	blit_nod = 1;
	for (i = 0; i < blit_diag[0]; i++) {
		int v = blit_diag[1 + blit_diag[0] + i];
		if (v <= 4)
			blit_dmacount++;
		if (v > 0 && v < 4)
			blit_dmacount2++;
		if (v == 4)
			blit_nod = 0;
	}
	if (blit_dmacount2 == 0) {
		ddat1use = 0;
	}
}

static _INLINE_ void blitter_start_init(void)
{
	blt_info.blitzero = 1;
	preva = 0;
	prevb = 0;

	blit_bltset (1 | 2);
	ddat1use = 0;

	if (blitline) {
		blinea = blt_info.bltadat;
		blineb = (blt_info.bltbdat >> blt_info.blitbshift) | (blt_info.bltbdat << (16 - blt_info.blitbshift));
		blitonedot = 0;
		blitsing = bltcon1 & 0x2;
	}
}

void do_blitter(void)
{
	int cycles;
  
	bltstate = BLT_done;

	blit_firstline_cycles = blit_first_cycle = get_cycles ();
	blit_last_cycle = 0;
	blit_cyclecounter = 0;

	blitter_start_init ();

  if(blitter_in_partial_mode)
  {
    blit_cycle_entered_wait = 0;
    if(blitline)
    {
      blit_cycles_per_op = blit_dmacount2 + (blit_nod ? 0 : 1);
      //blit_cycles_per_op = blit_diag[0];
      blit_cycles_per_vsize = blit_cycles_per_op;
      blit_init_cycles = 2;   // HRM says nothing about how much cycles required for blitter initialisation
    }
    else
    {
      blit_cycles_per_op = blit_dmacount2 + (blit_nod ? 0 : 1);
      //blit_cycles_per_op = blit_diag[0];
      blit_cycles_per_vsize = blit_cycles_per_op * blt_info.hblitsize;
      blit_init_cycles = 2;   // HRM says nothing about how much cycles required for blitter initialisation
      // Used from code below so that maybe_blit behaves same as in normal mode
      blit_firstline_cycles = blit_first_cycle + (blit_diag[0] * blt_info.hblitsize) * CYCLE_UNIT;
    }
    blit_total_required_cycles = blit_cycles_per_vsize * blt_info.vblitsize;
    blit_cyclecounter = blit_init_cycles + blit_total_required_cycles;
    blit_init_cycles *= CYCLE_UNIT;
    blit_vblitsize_done = 0;
  }
  else
  {        
  	if (blitline) {
  		cycles = blt_info.vblitsize;
  	} else {
  		cycles = blt_info.vblitsize * blt_info.hblitsize;
  		blit_firstline_cycles = blit_first_cycle + (blit_diag[0] * blt_info.hblitsize) * CYCLE_UNIT;
  	}
	  blit_cyclecounter = cycles * (blit_dmacount2 + (blit_nod ? 0 : 1)); 
  }
  
	bltstate = BLT_init;
// blitter_slowdown doesn't work at the moment
//	blit_slowdown = 0;

    if (dmaen(DMA_BLITPRI))
        setnasty();
    else
    	unset_special (SPCFLAG_BLTNASTY);

  if(!dmaen (DMA_BLITTER))
    return;
    
  bltstate = BLT_work;

	if (blitline && blt_info.hblitsize != 2) {
		blitter_done ();
		return;
	}

	if (currprefs.immediate_blits) {
		blitter_doit ();
		return;
	}

  if(blitter_in_partial_mode)
  {
    blit_cycle_at_start = get_cycles();
    blit_cycle_current = blit_cycle_at_start;
  }

  eventtab[ev_blitter].active = 1;
  eventtab[ev_blitter].oldcycles = get_cycles ();
  eventtab[ev_blitter].evtime = blit_cyclecounter * CYCLE_UNIT + get_cycles ();
  events_schedule();
}

void blitter_dma_disabled(void)
{
  if(bltstate != BLT_work || !blitter_in_partial_mode)
    return;
  // execute blitter till current cycle
  blitter_do_partial(0);
  if(bltstate == BLT_work)
  {
    // We still have something to do...
    bltstate = BLT_waitDMA;
    blit_cycle_entered_wait = get_cycles();
    eventtab[ev_blitter].active = 0;
    events_schedule();
  }
}

void blitter_dma_enabled(void)
{
  if(bltstate != BLT_waitDMA || !blitter_in_partial_mode)
    return;
    
  bltstate = BLT_work;
  // Add cycles we waited to current cycles...
  unsigned long cycles_waited = get_cycles() - blit_cycle_entered_wait;
  blit_cycle_current += cycles_waited;
  eventtab[ev_blitter].active = 1;
  eventtab[ev_blitter].evtime += cycles_waited;
  events_schedule();
  blit_cycle_entered_wait = 0;
}

void blitter_do_partial(int do_all)
{
  if(bltstate != BLT_work && bltstate != BLT_waitDMA)
    return;

  if (!dmaen (DMA_BLITTER) && !do_all)
    return;
  
  unsigned long curr_cpu_cycles = get_cycles();
  if((curr_cpu_cycles < blit_cycle_current + blit_init_cycles) && !do_all)
    return; // Blitter not finished with initialisation
  if(blit_init_cycles > 0)
  {
    blit_cycle_current += blit_init_cycles;
    blit_init_cycles = 0;
  }
  
  int num_lines = 0;
  while(curr_cpu_cycles > blit_cycle_current || do_all)
  {
    num_lines++;
    blit_cycle_current += blit_cycles_per_vsize * CYCLE_UNIT;
    if(num_lines + blit_vblitsize_done >= blt_info.vblitsize)
      break;
  }

  if(num_lines > 0)
  {
    int tmp_vblitsize = blt_info.vblitsize;
    blt_info.vblitsize = num_lines;
    actually_do_blit();		
    blt_info.vblitsize = tmp_vblitsize;
    blit_vblitsize_done += num_lines;
    if(blit_vblitsize_done >= blt_info.vblitsize)
    {
      if(blitline)
        bltdpt = bltcpt;
      blitter_done();
    }
    else
      bltstate = BLT_work; // still not finished
  }
}

// Called only from custom.cpp if blitter DMA is now enabled
void blitter_check_start (void)
{
	if (bltstate != BLT_init)
		return;

  // do the same as in do_blitter after dmaen check
	bltstate = BLT_work;

	if (blitline && blt_info.hblitsize != 2) {
		blitter_done ();
		return;
	}

	if (currprefs.immediate_blits) {
		blitter_doit ();
		return;
	}
	
  if(blitter_in_partial_mode)
  {
    blit_cycle_at_start = get_cycles();
    blit_cycle_current = blit_cycle_at_start;
  }

  eventtab[ev_blitter].active = 1;
  eventtab[ev_blitter].oldcycles = get_cycles ();
  eventtab[ev_blitter].evtime = blit_cyclecounter * CYCLE_UNIT + get_cycles ();
  events_schedule();
}

void maybe_blit (int modulo)
{
    if (bltstate == BLT_done)
	return;

    if (modulo && get_cycles() < blit_firstline_cycles)
	return;

    blitter_handler ();
}

int blitnasty (void)
{
	int cycles, ccnt;
    if (!(_68k_spcflags & SPCFLAG_BLTNASTY))
	return 0;
	if (bltstate == BLT_done)
		return 0;
	if (!dmaen (DMA_BLITTER))
		return 0;
	if (blit_last_cycle >= blit_diag[0] && blit_dmacount == blit_diag[0])
		return 0;
	cycles = (get_cycles () - blit_first_cycle) / CYCLE_UNIT;
	ccnt = 0;
	while (blit_last_cycle < cycles) {
	int c;
	if (blit_last_cycle < blit_diag[0])
	  	c = blit_diag[1 + blit_last_cycle];
	  else
	    c = blit_diag[1 + blit_diag[0] + ((blit_last_cycle - blit_diag[0]) % blit_diag[0])];
    blit_last_cycle++;
		if (!c)
			ccnt++;
	}
	return ccnt;
}

// blitter_slowdown doesn't work at the moment (causes gfx glitches in Shadow of the Beast)
/* very approximate emulation of blitter slowdown caused by bitplane DMA */
/*
void blitter_slowdown (int ddfstrt, int ddfstop, int totalcycles, int freecycles)
{
	static int oddfstrt, oddfstop, ototal, ofree;
	static int slow;

	if (!totalcycles || ddfstrt < 0 || ddfstop < 0)
		return;
	if (ddfstrt != oddfstrt || ddfstop != oddfstop || totalcycles != ototal || ofree != freecycles) {
		int linecycles = ((ddfstop - ddfstrt + totalcycles - 1) / totalcycles) * totalcycles;
		int freelinecycles = ((ddfstop - ddfstrt + totalcycles - 1) / totalcycles) * freecycles;
		int dmacycles = (linecycles * blit_dmacount) / blit_diag[0];
		oddfstrt = ddfstrt;
		oddfstop = ddfstop;
		ototal = totalcycles;
		ofree = freecycles;
		slow = 0;
		if (dmacycles > freelinecycles)
			slow = dmacycles - freelinecycles;
	}
	if (blit_slowdown < 0 || blitline)
		return;
	blit_slowdown += slow;
}
*/
