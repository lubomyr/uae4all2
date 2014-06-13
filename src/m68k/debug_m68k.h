#include "debug_uae4all.h"
#include "m68k/m68k_intrf.h"

#ifdef DEBUG_M68K

#define make_a_sr() _68k_sreg

static __inline__ void dbg_cycle(unsigned opcode)
{
	dbgf("\t%.8X    PC=%.8X  OPCODE=%.8X  SR=%.8X\n", (get_cycles())>>8, m68k_get_pc(), opcode,make_a_sr());
	dbgf("A0=%.8X  A1=%.8X  A2=%.8X  A3=%.8X\n",_68k_areg(0),_68k_areg(1),_68k_areg(2),_68k_areg(3));
	dbgf("A4=%.8X  A5=%.8X  A6=%.8X  A7=%.8X\n",_68k_areg(4),_68k_areg(5),_68k_areg(6),_68k_areg(7));
	dbgf("D0=%.8X  D1=%.8X  D2=%.8X  D3=%.8X\n",_68k_dreg(0),_68k_dreg(1),_68k_dreg(2),_68k_dreg(3));
	dbgf("D4=%.8X  D5=%.8X  D6=%.8X  D7=%.8X\n",_68k_dreg(4),_68k_dreg(5),_68k_dreg(6),_68k_dreg(7));
}

static __inline__ unsigned getmemsum(void)
{
        unsigned *p=(unsigned *)chipmemory;
        unsigned i,ret=0;
        for(i=0;i<(0x100000/4);i++,p++)
        {
                uae_u8 *b = (uae_u8 *)p;
                ret+=((*(b+2) << 24) | (*(b+3) << 16) | (*(b) << 8) | (*(b+1)));
        }
        return ret;
}


#endif

