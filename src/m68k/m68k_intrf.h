#ifndef M68KIntrfH
#define M68KIntrfH

#include "menu_config.h"

/* defined in m68k_cmn_intrf.cpp */
extern unsigned mispcflags;

void init_memmaps(addrbank* banco);
void map_zone(unsigned addr, addrbank* banco, unsigned realstart);
void m68k_go(int may_quit);

#define M68KCONTEXT m68kcontext

#define m68k_irq_update() m68kcontext.more_cycles_to_do=m68kcontext.io_cycle_counter; m68kcontext.io_cycle_counter=0;


#include "m68k/fame/fame.h"
#include "memory.h"

extern M68K_CONTEXT M68KCONTEXT;

#define _68k_dreg(num) (M68KCONTEXT.dreg[(num)].D)
#define _68k_areg(num) (M68KCONTEXT.areg[(num)].D)
#define _68k_sreg 	M68KCONTEXT.sr
#define _68k_ispreg 	M68KCONTEXT.isp
#define _68k_mspreg 	M68KCONTEXT.msp
#define _68k_uspreg 	M68KCONTEXT.usp
#define _68k_intmask   ((M68KCONTEXT.sr >> 8) & 7)
#define _68k_spcflags mispcflags

static __inline__ void _68k_setpc(unsigned mipc)
{
	M68KCONTEXT.pc=mipc;
	m68k_set_register(M68K_REG_PC, mipc);
}

static __inline__ void set_special (unsigned x)
{
    _68k_spcflags |= x;
}

static __inline__ void unset_special (uae_u32 x)
{
    _68k_spcflags &= ~x;
}


/* PocketUAE */

/* UAE redefine */
#define m68k_dreg(r,num) (m68kcontext.dreg[num].D)
#define m68k_areg(r,num) (m68kcontext.areg[num].D)

/***************/

#endif
