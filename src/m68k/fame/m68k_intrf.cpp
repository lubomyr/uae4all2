#include <string.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "debug_uae4all.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "memory-uae.h"
#include "custom.h"
#include "autoconf.h"
#include "ersatz.h"
#include "savestate.h"

#include "m68k/debug_m68k.h"

static unsigned short mimemoriadummy[65536/2];

void clear_fame_mem_dummy(void)
{
	memset((void *)&mimemoriadummy[0],0,65536);
}

static unsigned micontexto_fpa[256];


void process_exception(unsigned int vect);


void uae_chk_handler(unsigned vector)
{
	unsigned opcode=m68k_fetch(m68k_get_pc());
	unsigned pc=m68k_get_pc();

	if (cloanto_rom && (opcode & 0xF100) == 0x7100) {
		_68k_dreg((opcode >> 9) & 7) = (uae_s8)(opcode & 0xFF);
		m68k_set_register(M68K_REG_PC,pc+2);
		return;
	}

	if (opcode == 0x4E7B && get_long (0x10) == 0 && (pc & 0xF80000) == 0xF80000) {
		write_log ("Your Kickstart requires a 68020 CPU. Giving up.\n");
		set_special (SPCFLAG_BRK);
		quit_program = 1;
		return;
	}

	if (opcode == 0xFF0D) {
		if ((pc & 0xF80000) == 0xF80000) {
			// This is from the dummy Kickstart replacement
			uae_u16 arg = m68k_fetch(pc+2);
			m68k_set_register(M68K_REG_PC,pc+4);
			ersatz_perform (arg);
			return;
		}
		else
		if ((pc & 0xFFFF0000) == RTAREA_BASE) {
			// User-mode STOP replacement
			M68KCONTEXT.execinfo|=0x0080;
			m68k_set_register(M68K_REG_PC,pc+2);
			return;
		}
	}

	if ((opcode & 0xF000) == 0xA000 && (pc & 0xFFFF0000) == RTAREA_BASE) {
		// Calltrap.
		m68k_set_register(M68K_REG_PC,pc+2);
		call_calltrap (opcode & 0xFFF);
		return;
	}

	if ((opcode & 0xF000) == 0xF000) {
		// Exception 0xB
		process_exception(0xB);
		return;
	}

	if ((opcode & 0xF000) == 0xA000) {
		if ((pc & 0xFFFF0000) == RTAREA_BASE) {
			// Calltrap.
			call_calltrap (opcode & 0xFFF);
		}
		process_exception(0xA);
		return;
	}

	write_log ("Illegal instruction: %04x at %08lx\n", opcode, pc);
	process_exception(0x4);
}


void init_memmaps(addrbank* banco)
{
	unsigned i;

  M68K_CONTEXT *context = m68k_get_context();
  
	memset(context,0,sizeof(M68K_CONTEXT));
	memset(&micontexto_fpa,0,sizeof(unsigned)*256);

	micontexto_fpa[0x04]=(unsigned)&uae_chk_handler;

	/* PocketUAE/WinUAE traps */
	micontexto_fpa[0x0A]=(unsigned)&uae_chk_handler;
	micontexto_fpa[0x10]=(unsigned)&uae_chk_handler;
	micontexto_fpa[0x14]=(unsigned)&uae_chk_handler;
	micontexto_fpa[0x15]=(unsigned)&uae_chk_handler;

	context->icust_handler = (unsigned int*)&micontexto_fpa;

	for(i=0;i<256;i++)
	{
		unsigned offset=(unsigned)banco->baseaddr;
		unsigned low_addr=(i<<16);
		unsigned high_addr=((i+1)<<16)-1;
		void *data=NULL;
		void *mem_handler_r8=NULL;
		void *mem_handler_r16=NULL;
		void *mem_handler_w8=NULL;
		void *mem_handler_w16=NULL;

		if (offset)
			data=(void *)(offset-low_addr);
		else
		{
			mem_handler_r8=(void *)banco->bget;
			mem_handler_r16=(void *)banco->wget;
			mem_handler_w8=(void *)banco->bput;
			mem_handler_w16=(void *)banco->wput;
		}

    famec_SetBank(low_addr, high_addr, ((unsigned)&mimemoriadummy)-low_addr, mem_handler_r8, 
      mem_handler_r16, mem_handler_w8, mem_handler_w16, data);
	}
}

void map_zone(unsigned addr, addrbank* banco, unsigned realstart)
{
	unsigned offset=(unsigned)banco->baseaddr;
	if (addr>255)
		return;

	unsigned low_addr=(addr<<16);
	unsigned high_addr=((addr+1)<<16)-1;

	if (offset)
	{
		offset+=((addr-realstart)<<16);
    famec_SetBank(low_addr, high_addr, offset-low_addr, NULL, NULL, NULL, NULL, (void *)(offset-low_addr));
	}
	else
	{
    famec_SetBank(low_addr, high_addr, ((unsigned)&mimemoriadummy)-low_addr, 
      (void*)banco->bget, (void*)banco->wget, (void*)banco->bput, (void*)banco->wput, NULL);
	}
}
