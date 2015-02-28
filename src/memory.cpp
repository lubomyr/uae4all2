 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Memory management
  *
  * (c) 1995 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "uae.h"
#include "options.h"
#include "memory-uae.h"
#include "ersatz.h"
#include "custom.h"
#include "debug_uae4all.h"
#include "events.h"
#include "m68k/m68k_intrf.h"
#include "autoconf.h"
#include "savestate.h"
#ifdef ANDROIDSDL
#include <android/log.h> 
#endif

int bReloadKickstart = 0;

#include "zfile.h"

unsigned prefs_chipmem_size;
unsigned prefs_bogomem_size;

const char *kickstarts_rom_names[] = { "kick12.rom\0", "kick13.rom\0", "kick20.rom\0", "kick31.rom\0", "aros-amiga-m68k-rom.bin\0" };
const char *extended_rom_names[] = { "\0", "\0", "\0", "\0", "aros-amiga-m68k-ext.bin\0" };
#ifdef ANDROIDSDL
const char *af_kickstarts_rom_names[] = { "amiga-os-120.rom\0", "amiga-os-130.rom\0", "amiga-os-204.rom\0", "amiga-os-310-a1200.rom\0" };
#endif

void clear_fame_mem_dummy(void);

int ersatzkickfile = 0;

uae_u32 allocated_chipmem=0;
uae_u32 allocated_fastmem=0;
uae_u32 allocated_bogomem=0;
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
uae_u32 allocated_gfxmem=0;
uae_u32 allocated_z3fastmem=0;
uae_u32 allocated_a3000mem=0;
#endif

static long chip_filepos;
static long bogo_filepos;
static long rom_filepos;

#include <zlib.h>
static long compressed_size;

addrbank *mem_banks[65536];

uae_u32 chipmem_mask, kickmem_mask, bogomem_mask;

uae_u32 extendedkickmem_mask, a3000mem_mask;

/* A dummy bank that only contains zeros */

static uae_u32 dummy_lget (uaecptr) REGPARAM;
static uae_u32 dummy_wget (uaecptr) REGPARAM;
static uae_u32 dummy_bget (uaecptr) REGPARAM;
static void dummy_lput (uaecptr, uae_u32) REGPARAM;
static void dummy_wput (uaecptr, uae_u32) REGPARAM;
static void dummy_bput (uaecptr, uae_u32) REGPARAM;
static int dummy_check (uaecptr addr, uae_u32 size) REGPARAM;

uae_u32 REGPARAM2 dummy_lget (uaecptr addr)
{
    return NONEXISTINGDATA; /*0xFFFFFFFF;*/
}

uae_u32 REGPARAM2 dummy_wget (uaecptr addr)
{
    return NONEXISTINGDATA; /*0xFFFF*/;
}

uae_u32 REGPARAM2 dummy_bget (uaecptr addr)
{
    return NONEXISTINGDATA; /*0xFF;*/
}

void REGPARAM2 dummy_lput (uaecptr addr, uae_u32 l)
{
}

void REGPARAM2 dummy_wput (uaecptr addr, uae_u32 w)
{
}

void REGPARAM2 dummy_bput (uaecptr addr, uae_u32 b)
{
}

int REGPARAM2 dummy_check (uaecptr addr, uae_u32 size)
{
    return 0;
}

#if !( defined(PANDORA) || defined(ANDROIDSDL) )
/* A3000 "motherboard resources" bank.  */
static uae_u32 mbres_lget (uaecptr) REGPARAM;
static uae_u32 mbres_wget (uaecptr) REGPARAM;
static uae_u32 mbres_bget (uaecptr) REGPARAM;
static void mbres_lput (uaecptr, uae_u32) REGPARAM;
static void mbres_wput (uaecptr, uae_u32) REGPARAM;
static void mbres_bput (uaecptr, uae_u32) REGPARAM;
static int mbres_check (uaecptr addr, uae_u32 size) REGPARAM;

static int mbres_val = 0;

uae_u32 REGPARAM2 mbres_lget (uaecptr addr)
{
    return 0;
}

uae_u32 REGPARAM2 mbres_wget (uaecptr addr)
{
    return 0;
}

uae_u32 REGPARAM2 mbres_bget (uaecptr addr)
{
    return (addr & 0xFFFF) == 3 ? mbres_val : 0;
}

void REGPARAM2 mbres_lput (uaecptr addr, uae_u32 l)
{
}
void REGPARAM2 mbres_wput (uaecptr addr, uae_u32 w)
{
}
void REGPARAM2 mbres_bput (uaecptr addr, uae_u32 b)
{
    if ((addr & 0xFFFF) == 3)
	mbres_val = b;
}

int REGPARAM2 mbres_check (uaecptr addr, uae_u32 size)
{
    return 0;
}
#endif

/* Chip memory */

uae_u8 *chipmemory;
uae_u16 *chipmemory_word;

static int chipmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *chipmem_xlate (uaecptr addr) REGPARAM;

uae_u32 REGPARAM2 chipmem_lget (uaecptr addr)
{
    uae_u32 *m;
//    addr -= chipmem_start /*& chipmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= chipmem_mask;
#endif
    m = (uae_u32 *)(chipmemory + addr);

    return do_get_mem_long(m);
}

uae_u32 REGPARAM2 chipmem_wget (uaecptr addr)
{
   uae_u16 *m;
   //    addr -= chipmem_start /*& chipmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
   addr &= chipmem_mask;
#endif
   m = (uae_u16 *)(chipmemory + addr);

   return do_get_mem_word (m);
}

uae_u32 REGPARAM2 chipmem_bget (uaecptr addr)
{
	uae_u8 *m;
//    addr -= chipmem_start /*& chipmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= chipmem_mask;
#endif
    m = (uae_u8 *)(chipmemory + addr);
    return do_get_mem_byte(m);
}

void REGPARAM2 chipmem_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
//    addr -= chipmem_start /*& chipmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= chipmem_mask;
#endif
    m = (uae_u32 *)(chipmemory + addr);
    do_put_mem_long(m, l);
}

void REGPARAM2 chipmem_wput (uaecptr addr, uae_u32 w)
{
   uae_u16 *m;
   //    addr -= chipmem_start /*& chipmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
   addr &= chipmem_mask;
#endif
   m = (uae_u16 *)(chipmemory + addr);
   do_put_mem_word (m, w);
}

void REGPARAM2 chipmem_bput (uaecptr addr, uae_u32 b)
{
	uae_u8 *m;
//    addr -= chipmem_start /*& chipmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= chipmem_mask;
#endif
    m = (uae_u8 *)(chipmemory + addr);
    do_put_mem_byte(m, b);
}

int REGPARAM2 chipmem_check (uaecptr addr, uae_u32 size)
{
//    addr -= chipmem_start /*& chipmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= chipmem_mask;
#endif
    return (addr + size) <= allocated_chipmem;
}

uae_u8 REGPARAM2 *chipmem_xlate (uaecptr addr)
{
//    addr -= chipmem_start /*& chipmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
	addr &= chipmem_mask;
#endif
    return chipmemory + addr;
}

/* Slow memory */

static uae_u8 *bogomemory;

static uae_u32 bogomem_lget (uaecptr) REGPARAM;
static uae_u32 bogomem_wget (uaecptr) REGPARAM;
static uae_u32 bogomem_bget (uaecptr) REGPARAM;
static void bogomem_lput (uaecptr, uae_u32) REGPARAM;
static void bogomem_wput (uaecptr, uae_u32) REGPARAM;
static void bogomem_bput (uaecptr, uae_u32) REGPARAM;
static int bogomem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *bogomem_xlate (uaecptr addr) REGPARAM;
uae_u32 REGPARAM2 bogomem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr -= bogomem_start /*& bogomem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= bogomem_mask;
#endif
    m = (uae_u32 *)(bogomemory + addr);

    return do_get_mem_long (m);
}

uae_u32 REGPARAM2 bogomem_wget (uaecptr addr)
{
    uae_u16 *m;
    addr -= bogomem_start /*& bogomem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= bogomem_mask;
#endif
    m = (uae_u16 *)(bogomemory + addr);

    return do_get_mem_word (m);
}

uae_u32 REGPARAM2 bogomem_bget (uaecptr addr)
{
    uae_u8 *m;
    addr -= bogomem_start /*& bogomem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= bogomem_mask;
#endif
    m = (uae_u8 *)(bogomemory + addr);

    return do_get_mem_byte(m);
}

void REGPARAM2 bogomem_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr -= bogomem_start /*& bogomem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= bogomem_mask;
#endif
    m = (uae_u32 *)(bogomemory + addr);
    do_put_mem_long (m, l);
}

void REGPARAM2 bogomem_wput (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    addr -= bogomem_start /*& bogomem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= bogomem_mask;
#endif
    m = (uae_u16 *)(bogomemory + addr);
    do_put_mem_word (m, w);
}

void REGPARAM2 bogomem_bput (uaecptr addr, uae_u32 b)
{
    uae_u8 *m;
    addr -= bogomem_start /*& bogomem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= bogomem_mask;
#endif
    m = (uae_u8 *)(bogomemory + addr);
    do_put_mem_byte(m, b);
}

int REGPARAM2 bogomem_check (uaecptr addr, uae_u32 size)
{
    addr -= bogomem_start /*& bogomem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= bogomem_mask;
#endif
    return (addr + size) <= allocated_bogomem;
}

uae_u8 REGPARAM2 *bogomem_xlate (uaecptr addr)
{
    addr -= bogomem_start /*& bogomem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= bogomem_mask;
#endif
    return bogomemory + addr;
}

#if !( defined(PANDORA) || defined(ANDROIDSDL) )
/* A3000 motherboard fast memory */

static uae_u8 *a3000memory;

static uae_u32 a3000mem_lget (uaecptr) REGPARAM;
static uae_u32 a3000mem_wget (uaecptr) REGPARAM;
static uae_u32 a3000mem_bget (uaecptr) REGPARAM;
static void a3000mem_lput (uaecptr, uae_u32) REGPARAM;
static void a3000mem_wput (uaecptr, uae_u32) REGPARAM;
static void a3000mem_bput (uaecptr, uae_u32) REGPARAM;
static int a3000mem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *a3000mem_xlate (uaecptr addr) REGPARAM;

uae_u32 REGPARAM2 a3000mem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    m = (uae_u32 *)(a3000memory + addr);
    return do_get_mem_long (m);
}

uae_u32 REGPARAM2 a3000mem_wget (uaecptr addr)
{
    uae_u16 *m;
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    m = (uae_u16 *)(a3000memory + addr);
    return do_get_mem_word (m);
}

uae_u32 REGPARAM2 a3000mem_bget (uaecptr addr)
{
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    return a3000memory[addr];
}

void REGPARAM2 a3000mem_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    m = (uae_u32 *)(a3000memory + addr);
    do_put_mem_long (m, l);
}

void REGPARAM2 a3000mem_wput (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    m = (uae_u16 *)(a3000memory + addr);
    do_put_mem_word (m, w);
}

void REGPARAM2 a3000mem_bput (uaecptr addr, uae_u32 b)
{
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    a3000memory[addr] = b;
}

int REGPARAM2 a3000mem_check (uaecptr addr, uae_u32 size)
{
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    return (addr + size) <= allocated_a3000mem;
}

uae_u8 REGPARAM2 *a3000mem_xlate (uaecptr addr)
{
    addr -= a3000mem_start & a3000mem_mask;
    addr &= a3000mem_mask;
    return a3000memory + addr;
}
#endif

/* Kick memory */

uae_u8 *kickmemory;

static unsigned kickmem_checksum=0;
static unsigned get_kickmem_checksum(void)
{
	unsigned *p=(unsigned *)kickmemory;
	unsigned ret=0;
	if (p)
	{
		unsigned max=kickmem_size/4;
		unsigned i;
		for(i=0;i<max;i++)
			ret+=(i+1)*p[i];
	}
	return ret;
}

/*
 * A1000 kickstart RAM handling
 *
 * RESET instruction unhides boot ROM and disables write protection
 * write access to boot ROM hides boot ROM and enables write protection
 *
 */
static int a1000_kickstart_mode;
static uae_u8 *a1000_bootrom;
static void a1000_handle_kickstart (int mode)
{
    if (mode == 0) {
	a1000_kickstart_mode = 0;
	memcpy (kickmemory, kickmemory + 262144, 262144);
    } else {
	a1000_kickstart_mode = 1;
	memset (kickmemory, 0, 262144);
	memcpy (kickmemory, a1000_bootrom, 8192);
	memcpy (kickmemory + 131072, a1000_bootrom, 8192);
    }
}

static uae_u32 kickmem_lget (uaecptr) REGPARAM;
static uae_u32 kickmem_wget (uaecptr) REGPARAM;
static uae_u32 kickmem_bget (uaecptr) REGPARAM;
static void kickmem_lput (uaecptr, uae_u32) REGPARAM;
static void kickmem_wput (uaecptr, uae_u32) REGPARAM;
static void kickmem_bput (uaecptr, uae_u32) REGPARAM;
static int kickmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *kickmem_xlate (uaecptr addr) REGPARAM;

uae_u32 REGPARAM2 kickmem_lget (uaecptr addr)
{
   uae_u16 *m;
   addr -= kickmem_start /*& kickmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
   addr &= kickmem_mask;
#endif
   m = (uae_u16 *)(kickmemory + addr);

   return (do_get_mem_word(m) << 16) |
          (do_get_mem_word(m + 1));
}

uae_u32 REGPARAM2 kickmem_wget (uaecptr addr)
{
   uae_u16 *m;
   addr -= kickmem_start /*& kickmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
   addr &= kickmem_mask;
#endif
   m = (uae_u16 *)(kickmemory + addr);

   return do_get_mem_word (m);
}

uae_u32 REGPARAM2 kickmem_bget (uaecptr addr)
{
    uae_u8 *m;
    addr -= kickmem_start /*& kickmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= kickmem_mask;
#endif
    m = (uae_u8 *)(kickmemory + addr);

    return do_get_mem_byte(m);
}

void REGPARAM2 kickmem_lput (uaecptr addr, uae_u32 l)
{
   uae_u16 *m;
   if (a1000_kickstart_mode) {
      if (addr >= 0xfc0000) {
         addr -= kickmem_start /*& kickmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
         addr &= kickmem_mask;
#endif
         m = (uae_u16 *)(kickmemory + addr);
         do_put_mem_word(m, l >> 16);
         do_put_mem_word(m + 1, l);
         return;
      } else
         a1000_handle_kickstart (0);
   }
}

void REGPARAM2 kickmem_wput (uaecptr addr, uae_u32 w)
{
   uae_u16 *m;
   if (a1000_kickstart_mode) {
      if (addr >= 0xfc0000) {
         addr -= kickmem_start /*& kickmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
         addr &= kickmem_mask;
#endif
         m = (uae_u16 *)(kickmemory + addr);
         do_put_mem_word (m, w);
         return;
      } else
         a1000_handle_kickstart (0);
   }
}

void REGPARAM2 kickmem_bput (uaecptr addr, uae_u32 b)
{
   uae_u8 *m;
   if (a1000_kickstart_mode) {
      if (addr >= 0xfc0000) {
         addr -= kickmem_start /*& kickmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
         addr &= kickmem_mask;
#endif
         m = (uae_u8 *)(kickmemory + addr);
         do_put_mem_byte (m, b);
         return;
      } else
         a1000_handle_kickstart (0);
   }
}

int REGPARAM2 kickmem_check (uaecptr addr, uae_u32 size)
{
    addr -= kickmem_start /*& kickmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= kickmem_mask;
#endif
    return (addr + size) <= kickmem_size;
}

uae_u8 REGPARAM2 *kickmem_xlate (uaecptr addr)
{
    addr -= kickmem_start /*& kickmem_mask*/;
#ifdef SAFE_MEMORY_ACCESS
    addr &= kickmem_mask;
#endif
    return kickmemory + addr;
}

/* CD32/CDTV extended kick memory */

uae_u8 *extendedkickmemory;
static int extendedkickmem_size;
static uae_u32 extendedkickmem_start;

#define EXTENDED_ROM_CD32 1
#define EXTENDED_ROM_CDTV 2

static int extromtype (void)
{
    switch (extendedkickmem_size) {
    case 524288:
	return EXTENDED_ROM_CD32;
    case 262144:
	return EXTENDED_ROM_CDTV;
    }
    return 0;
}

static uae_u32 extendedkickmem_lget (uaecptr) REGPARAM;
static uae_u32 extendedkickmem_wget (uaecptr) REGPARAM;
static uae_u32 extendedkickmem_bget (uaecptr) REGPARAM;
static void extendedkickmem_lput (uaecptr, uae_u32) REGPARAM;
static void extendedkickmem_wput (uaecptr, uae_u32) REGPARAM;
static void extendedkickmem_bput (uaecptr, uae_u32) REGPARAM;
static int extendedkickmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *extendedkickmem_xlate (uaecptr addr) REGPARAM;

uae_u32 REGPARAM2 extendedkickmem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr -= extendedkickmem_start & extendedkickmem_mask;
    addr &= extendedkickmem_mask;
    m = (uae_u32 *)(extendedkickmemory + addr);
    return do_get_mem_long (m);
}

uae_u32 REGPARAM2 extendedkickmem_wget (uaecptr addr)
{
    uae_u16 *m;
    addr -= extendedkickmem_start & extendedkickmem_mask;
    addr &= extendedkickmem_mask;
    m = (uae_u16 *)(extendedkickmemory + addr);
    return do_get_mem_word (m);
}

uae_u32 REGPARAM2 extendedkickmem_bget (uaecptr addr)
{
    addr -= extendedkickmem_start & extendedkickmem_mask;
    addr &= extendedkickmem_mask;
    return extendedkickmemory[addr];
}

void REGPARAM2 extendedkickmem_lput (uaecptr addr, uae_u32 b)
{
}

void REGPARAM2 extendedkickmem_wput (uaecptr addr, uae_u32 b)
{
}

void REGPARAM2 extendedkickmem_bput (uaecptr addr, uae_u32 b)
{
}

int REGPARAM2 extendedkickmem_check (uaecptr addr, uae_u32 size)
{
    addr -= extendedkickmem_start & extendedkickmem_mask;
    addr &= extendedkickmem_mask;
    return (addr + size) <= extendedkickmem_size;
}

uae_u8 REGPARAM2 *extendedkickmem_xlate (uaecptr addr)
{
    addr -= extendedkickmem_start & extendedkickmem_mask;
    addr &= extendedkickmem_mask;
    return extendedkickmemory + addr;
}


/* Default memory access functions */

int REGPARAM2 default_check (uaecptr a, uae_u32 b)
{
    return 0;
}

uae_u8 REGPARAM2 *default_xlate (uaecptr a)
{
    write_log ("Your Amiga program just did something terribly stupid\n");
    uae_reset ();
    return kickmem_xlate (get_long (0xF80000));	/* So we don't crash. */
}

/* Address banks */

addrbank dummy_bank = {
    dummy_lget, dummy_wget, dummy_bget,
    dummy_lput, dummy_wput, dummy_bput,
    default_xlate, dummy_check, NULL

};

#if !( defined(PANDORA) || defined(ANDROIDSDL) )
addrbank mbres_bank = {
    mbres_lget, mbres_wget, mbres_bget,
    mbres_lput, mbres_wput, mbres_bput,
    default_xlate, mbres_check, NULL

};
#endif

addrbank chipmem_bank = {
    chipmem_lget, chipmem_wget, chipmem_bget,
    chipmem_lput, chipmem_wput, chipmem_bput,
    chipmem_xlate, chipmem_check, NULL

};

addrbank bogomem_bank = {
    bogomem_lget, bogomem_wget, bogomem_bget,
    bogomem_lput, bogomem_wput, bogomem_bput,
    bogomem_xlate, bogomem_check, NULL

};

#if !( defined(PANDORA) || defined(ANDROIDSDL) )
addrbank a3000mem_bank = {
    a3000mem_lget, a3000mem_wget, a3000mem_bget,
    a3000mem_lput, a3000mem_wput, a3000mem_bput,
    a3000mem_xlate, a3000mem_check, NULL

};
#endif

addrbank kickmem_bank = {
    kickmem_lget, kickmem_wget, kickmem_bget,
    kickmem_lput, kickmem_wput, kickmem_bput,
    kickmem_xlate, kickmem_check, NULL

};

addrbank extendedkickmem_bank = {
    extendedkickmem_lget, extendedkickmem_wget, extendedkickmem_bget,
    extendedkickmem_lput, extendedkickmem_wput, extendedkickmem_bput,
    extendedkickmem_xlate, extendedkickmem_check, NULL

};

static int decode_cloanto_rom (uae_u8 *mem, int size, int real_size)
{
  FILE *keyf;
  uae_u8 *p;
  long cnt, t;
  int keysize;
  
#ifdef ANDROIDSDL
  __android_log_print(ANDROID_LOG_INFO, "UAE", "decode_cloanto_rom %s", romkeyfile);
#endif

  if (strlen (romkeyfile) == 0) {
    return 0;
  } else {
    keyf = fopen (romkeyfile, "rb");
    if (keyf == 0)  {
#ifdef ANDROIDSDL
      __android_log_print(ANDROID_LOG_ERROR, "UAE",  "Error opening keyfile \"%s\"\n", romkeyfile );
#endif
      return 0;
    }
  
    p = (uae_u8 *)xmalloc (524288);
    keysize = fread (p, 1, 524288, keyf);
    if (keysize == 0 || p == 0) {
#ifdef ANDROIDSDL
      __android_log_print(ANDROID_LOG_ERROR, "UAE",  "Error reading keyfile \"%s\"\n", romkeyfile );
#endif
      fclose (keyf);
      free (p);
      return 0;
    }
#ifdef ANDROIDSDL
    __android_log_print(ANDROID_LOG_INFO, "UAE",  "rom size: %d %d, keyfile size: %d\n", size, real_size, keysize );
#endif
    for (t = cnt = 0; cnt < size; cnt++, t = (t + 1) % keysize)  {
      mem[cnt] ^= p[t];
      if (real_size == cnt + 1)
        t = keysize - 1;
    }
    fclose (keyf);
    free (p);
  }
  return 1;  
}

static int kickstart_checksum (uae_u8 *mem, int size)
{
    uae_u32 cksum = 0, prevck = 0;
    int i;
    for (i = 0; i < size; i += 4) {
	uae_u32 data = mem[i] * 65536 * 256 + mem[i + 1] * 65536 + mem[i + 2] * 256 + mem[i + 3];
	cksum += data;
	if (cksum < prevck)
	    cksum++;
	prevck = cksum;
    }
	if (cksum != 0xFFFFFFFFul) {
	write_log ("Kickstart checksum incorrect. You probably have a corrupted ROM image.\n");
    }
    return 0;
}

static int read_kickstart (FILE *f, uae_u8 *mem, int size, int dochecksum, int *cloanto_rom)
{
    unsigned char buffer[20];
    int i, cr = 0;

    if (cloanto_rom)
	*cloanto_rom = 0;
    i = uae4all_rom_fread (buffer, 1, 11, f);
    if (strncmp ((char *) buffer, "AMIROMTYPE1", 11) != 0) {
	uae4all_rom_fseek (f, 0, SEEK_SET);
    } else {
	cr = 1;
    }

    i = uae4all_rom_fread (mem, 1, size, f);
    if (i == 8192) {
	a1000_bootrom = (uae_u8*)xmalloc (8192);
	memcpy (a1000_bootrom, kickmemory, 8192);
	a1000_handle_kickstart (1);
    } else if (i == size / 2) {
	memcpy (mem + size / 2, mem, i);
    } else if (i != size) {
	write_log ("Error while reading Kickstart.\n");
	uae4all_rom_fclose (f);
	return 0;
    }
    uae4all_rom_fclose (f);

    if (cr)
	decode_cloanto_rom (mem, size, i);
    if (dochecksum && i >= 262144)
	kickstart_checksum (mem, size);
    if (cloanto_rom)
	*cloanto_rom = cr;
    return 1;
}

static int load_extendedkickstart (void)
{
  FILE *f;
  int size;

  if (strlen (extfile) == 0)
	  return 0;
  f = fopen (extfile, "rb");
  if (!f) 
  {
	  printf ("No extended Kickstart ROM found.\n");
	  return 0;
  }

  fseek (f, 0, SEEK_END);
  size = ftell (f);
  if (size > 300000)
	  extendedkickmem_size = 524288;
  else
	  extendedkickmem_size = 262144;
  fseek (f, 0, SEEK_SET);

  switch (extromtype ()) 
  {
    case EXTENDED_ROM_CDTV:
	    extendedkickmemory = (uae_u8 *) mapped_malloc (extendedkickmem_size, "rom_f0");
	    extendedkickmem_bank.baseaddr = (uae_u8 *) extendedkickmemory;
	    break;
    case EXTENDED_ROM_CD32:
	    extendedkickmemory = (uae_u8 *) mapped_malloc (extendedkickmem_size, "rom_e0");
	    extendedkickmem_bank.baseaddr = (uae_u8 *) extendedkickmemory;
	    break;
  }
  
  //read_kickstart (f, extendedkickmemory, 524288, 0, 0);
  int i;
  i = fread (extendedkickmemory, 1, 524288, f);
  if (i != 8192 && i != 65536 && i != 131072 && i != 262144 && i != 524288 && i != 524288 * 2 && i != 524288 * 4) 
  {
	  printf ("Error while reading Kickstart ROM file.\n");
	  fclose (f);
	  return 0;
  }
  fclose (f);
  printf("Extended ROM loaded: %s\n", extfile);
  swab_memory(extendedkickmemory, extendedkickmem_size);
  
  return 1;
}

void swab_memory(uae_u8 *apMemory, uae_u32 aSize)
{
	uae_u32 i;
	for(i=0; i<aSize; i+=2)
	{
		unsigned char b1=apMemory[i];
		apMemory[i]=apMemory[i+1];
		apMemory[i+1]=b1;

	}
}

static int load_kickstart (void)
{
    FILE *f = uae4all_rom_fopen(romfile, "rb");

    if (f == NULL) {
#if defined(AMIGA)||defined(__POS__)
#define USE_UAE_ERSATZ "USE_UAE_ERSATZ"
	if (!getenv (USE_UAE_ERSATZ)) {
	    write_log ("Using current ROM. (create ENV:%s to " "use uae's ROM replacement)\n", USE_UAE_ERSATZ);
	    memcpy (kickmemory, (char *) 0x1000000 - kickmem_size, kickmem_size);
	    kickstart_checksum (kickmemory, kickmem_size);
	    goto chk_sum;
	}
#endif
	return 0;
    }

    if (!read_kickstart (f, kickmemory, kickmem_size, 1, &cloanto_rom))
	return 0;

    return 1;
}

char *address_space, *good_address_map;
int good_address_fd;


uae_u8 *mapped_malloc (size_t s, const char *file)
{
    return (uae_u8 *)xmalloc (s);
}

void mapped_free (uae_u8 *p)
{
    free (p);
}


static void init_mem_banks (void)
{
    int i;
    for (i = 0; i < 65536; i++)
	put_mem_bank (i << 16, &dummy_bank);
    if (!savestate_state)
    init_memmaps(&dummy_bank);
}

static void allocate_memory (void)
{
	if (allocated_chipmem != prefs_chipmem_size) 
	{
		if (chipmemory)
			mapped_free (chipmemory);
		chipmemory = 0;
		
		allocated_chipmem = prefs_chipmem_size;
		chipmem_mask = allocated_chipmem - 1;
		
		chipmemory = mapped_malloc (allocated_chipmem, "chip");
		
		if (chipmemory == 0) {
			write_log ("Fatal error: out of memory for chipmem.\n");
			allocated_chipmem = 0;
		}
		else do_put_mem_long ((uae_u32 *)(chipmemory + 4), 0);
    }
	
	/* PocketUAE code */
	if (allocated_bogomem != prefs_bogomem_size) {
		if (bogomemory)
			mapped_free (bogomemory);
		bogomemory = 0;
		
		if(prefs_bogomem_size > 0x1c0000)
      prefs_bogomem_size = 0x1c0000;
    if (prefs_bogomem_size > 0x180000 && ((changed_prefs.chipset_mask & CSMASK_AGA) || (prefs_cpu_model >= 68020)))
      prefs_bogomem_size = 0x180000;
      
		allocated_bogomem = prefs_bogomem_size;
		bogomem_mask = allocated_bogomem - 1;

		if (allocated_bogomem) {
			bogomemory = mapped_malloc (allocated_bogomem, "bogo");
			if (bogomemory == 0) {
				write_log ("Out of memory for bogomem.\n");
				allocated_bogomem = 0;
			}
		}
	}

	/******************/
  if (savestate_state == STATE_RESTORE)
	{
	    fseek (savestate_file, chip_filepos, SEEK_SET);

	    void *tmp=malloc(compressed_size);
	    int outSize=allocated_chipmem;
	    int inSize=compressed_size;
	    int res;
	    fread (tmp, 1, compressed_size, savestate_file);
	    res=uncompress((Bytef *)chipmemory, (uLongf *)&outSize, (const Bytef *)tmp, (uLong) inSize);
	    free(tmp);
	    if(res != Z_OK)
	    {
	        // decompression failed - treat data literaly
		    allocated_chipmem=compressed_size;
		    fseek (savestate_file, chip_filepos, SEEK_SET);
		    fread (chipmemory, 1, allocated_chipmem, savestate_file);
	    }
	    if (allocated_bogomem > 0)
	    {
		    fseek (savestate_file, bogo_filepos, SEEK_SET);
		    fread (bogomemory, 1, allocated_bogomem, savestate_file);
	    }
	}

	chipmem_bank.baseaddr = chipmemory;
	bogomem_bank.baseaddr = bogomemory;
	chipmemory_word=(uae_u16 *)chipmemory;
}

static void reload_kickstart(void)
{
   load_extendedkickstart ();
   if (!load_kickstart ()) {
      init_ersatz_rom (kickmemory);
      ersatzkickfile = 1;
   }
   swab_memory(kickmemory, kickmem_size);
   kickmem_checksum=get_kickmem_checksum();
}

void memory_reset (void)
{
    int bnk, bnk_end, custom_start;

    init_mem_banks ();

    allocate_memory ();

//    memset(chipmemory,0,allocated_chipmem);
    clear_fame_mem_dummy();

    /* Can't be done here, or we'll lose all the extension/filesys traps that were set up */
//    rtarea_cleanup();
    
    if (kickmem_checksum!=get_kickmem_checksum() | bReloadKickstart)
    {
       bReloadKickstart=0;
       unsigned chksum=kickmem_checksum;
       reload_kickstart();
       if (chksum!=kickmem_checksum)
       {
          uae4all_rom_reinit();
          reload_kickstart();
       }
    }

    custom_start = 0xC0;
    map_banks (&custom_bank, custom_start, 0xE0 - custom_start, 0);
    map_banks (&cia_bank, 0xA0, 32, 0);
    if (!a1000_bootrom)
       /* D80000 - DDFFFF not mapped (A1000 = custom chips) */
       map_banks (&dummy_bank, 0xD8, 6, 0);

     /* Map "nothing" to 0x200000 - 0x9FFFFF (0xBEFFFF if PCMCIA or AGA) */
    bnk = allocated_chipmem >> 16;
    if (bnk < 0x20 + (allocated_fastmem >> 16))
       bnk = 0x20 + (allocated_fastmem >> 16);
    bnk_end = (((changed_prefs.chipset_mask & CSMASK_AGA) /*|| currprefs.cs_pcmcia*/) ? 0xBF : 0xA0);
    map_banks (&dummy_bank, bnk, bnk_end - bnk, 0);
    if (changed_prefs.chipset_mask & CSMASK_AGA)
       map_banks (&dummy_bank, 0xc0, 0xd8 - 0xc0, 0);

    /* Map chipmem */
    bnk = allocated_chipmem > 0x200000 ? (allocated_chipmem >> 16) : 0x20;
    map_banks (&chipmem_bank, 0x00, bnk, allocated_chipmem);

    if (bogomemory != 0) {
       int t = allocated_bogomem >> 16;

//       memset(bogomemory,0,allocated_bogomem);
       map_banks (&bogomem_bank, 0xC0, t, allocated_bogomem);
    }
    
    map_banks (&clock_bank, 0xDC, 1, 0);

#if !( defined(PANDORA) || defined(ANDROIDSDL) )
    if (a3000memory != 0)
    {
       map_banks (&a3000mem_bank, a3000mem_start >> 16, allocated_a3000mem >> 16, allocated_a3000mem);
    }
#endif

    map_banks (&rtarea_bank, RTAREA_BASE >> 16, 1, 0);
    
    map_banks (&kickmem_bank, 0xF8, 8, 0);

    /* map beta Kickstarts at 0x200000 */
    if (kickmemory[2] == 0x4e && kickmemory[3] == 0xf9 && kickmemory[4] == 0x00) {
       uae_u32 addr = kickmemory[5];
       if (addr == 0x20 && allocated_chipmem <= 0x200000 && allocated_fastmem == 0)
          map_banks (&kickmem_bank, addr, 8, 0);
    }

    if (a1000_bootrom)
       a1000_handle_kickstart (1);

#ifdef USE_AUTOCONFIG
    map_banks (&expamem_bank, 0xE8, 1, 0);
#endif

    switch (extromtype ()) 
    {
       case EXTENDED_ROM_CDTV:
          map_banks (&extendedkickmem_bank, 0xF0, 8, 0);
          break;
       case EXTENDED_ROM_CD32:
          map_banks (&extendedkickmem_bank, 0xE0, 8, 0);
          break;
       default:
          if (cloanto_rom)
          {
             map_banks (&kickmem_bank, 0xE0, 8, 0);
          }
    }
}


void memory_init (void)
{
    allocated_chipmem = 0;
    allocated_bogomem = 0;
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
    allocated_a3000mem = 0;
    a3000memory = 0;
#endif
    kickmemory = 0;
    extendedkickmemory = 0;
    chipmemory = 0;
    bogomemory = 0;

    kickmemory = mapped_malloc (kickmem_size, "kick");
    kickmem_bank.baseaddr = kickmemory;

    reload_kickstart();

    init_mem_banks ();
    memory_reset ();

    kickmem_mask = kickmem_size - 1;
    extendedkickmem_mask = extendedkickmem_size - 1;
}

void memory_cleanup (void)
{
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
    if (a3000memory)
	mapped_free (a3000memory);
#endif
    if (bogomemory)
	mapped_free (bogomemory);
    if (kickmemory)
	mapped_free (kickmemory);
    if (a1000_bootrom)
	free (a1000_bootrom);
    if (chipmemory)
	mapped_free (chipmemory);

#if !( defined(PANDORA) || defined(ANDROIDSDL) )
    a3000memory = 0;
#endif
    bogomemory = 0;
    kickmemory = 0;
    a1000_bootrom = 0;
    chipmemory = 0;
}

void map_banks (addrbank *bank, int start, int size, int realsize)
{
   int bnr;
   unsigned long int hioffs = 0, endhioffs = 0x100;
   addrbank *orgbank = bank;
   uae_u32 realstart = start;

   if (!realsize)
      realsize = size << 16;

   if ((size << 16) < realsize) {
      write_log ("Please report to bmeyer@cs.monash.edu.au, and mention:\n");
      write_log ("Broken mapping, size=%x, realsize=%x\n", size, realsize);
      write_log ("Start is %x\n", start);
      write_log ("Reducing memory sizes, especially chipmem, may fix this problem\n");
      return;
   }

   if (start >= 0x100) {
      int real_left = 0;
      for (bnr = start; bnr < start + size; bnr++) {
         if (!real_left) {
            realstart = bnr;
            real_left = realsize >> 16;
         }
         put_mem_bank (bnr << 16, bank);
         map_zone(bnr,bank,realstart);
         real_left--;
      }
      return;
   }
   for (hioffs = 0; hioffs < endhioffs; hioffs += 0x100) {
      int real_left = 0;
      for (bnr = start; bnr < start + size; bnr++) {
         if (!real_left) {
            realstart = bnr + hioffs;
            real_left = realsize >> 16;
         }
         put_mem_bank ((bnr + hioffs) << 16, bank);
         map_zone(bnr+hioffs,bank,realstart);
         real_left--;
      }
   }
}


/* memory save/restore code */

uae_u8 *save_cram (int *len)
{
    *len = allocated_chipmem;
    return chipmemory;
}

uae_u8 *save_bram (int *len)
{
    *len = allocated_bogomem;
    return bogomemory;
}

void restore_cram (int len, long filepos)
{
    chip_filepos = filepos;
    compressed_size=len;
}

void restore_bram (int len, long filepos)
{
    bogo_filepos = filepos;
}

uae_u8 *restore_rom (uae_u8 *src)
{
    restore_u32 ();
    restore_u32 ();
    restore_u32 ();
    restore_u32 ();
    restore_u32 ();

    return src;
}

uae_u8 *save_rom (int first, int *len)
{
    static int count;
    uae_u8 *dst, *dstbak;
    uae_u8 *mem_real_start;
    int mem_start, mem_size, mem_type, i, saverom;

    saverom = 0;
    if (first)
	count = 0;
    for (;;) {
	mem_type = count;
	switch (count) {
	case 0:		/* Kickstart ROM */
	    mem_start = 0xf80000;
	    mem_real_start = kickmemory;
	    mem_size = kickmem_size;
	    /* 256KB or 512KB ROM? */
	    for (i = 0; i < mem_size / 2 - 4; i++) {
		if (longget (i + mem_start) != longget (i + mem_start + mem_size / 2))
		    break;
	    }
	    if (i == mem_size / 2 - 4) {
		mem_size /= 2;
		mem_start += 262144;
	    }
	    mem_type = 0;
	    break;
	default:
	    return 0;
	}
	count++;
	if (mem_size)
	    break;
    }
    if(saverom)
    	dstbak = dst = (uae_u8 *)malloc (4 + 4 + 4 + 4 + 4 + mem_size);
    else
    	dstbak = dst = (uae_u8 *)malloc (4 + 4 + 4 + 4 + 4);
    save_u32 (mem_start);
    save_u32 (mem_size);
    save_u32 (mem_type);
    save_u32 (longget (mem_start + 12));	/* version+revision */
    save_u32 (0);
    // no memory allocated for "Kickstart %d.%d"
    //snprintf ((char *)dst, 32, "Kickstart %d.%d", wordget (mem_start + 12), wordget (mem_start + 14));
    //dst += strlen ((char *)dst) + 1;
    if (saverom) {
	for (i = 0; i < mem_size; i++)
	    *dst++ = byteget (mem_start + i);
    }
    *len = dst - dstbak;
    return dstbak;
}
