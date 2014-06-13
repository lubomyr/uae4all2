 /*
  * UAE - The Un*x Amiga Emulator
  *
  * memory management
  *
  * Copyright 1995 Bernd Schmidt
  */

#ifndef UAE_MEMORY_H
#define UAE_MEMORY_H

void swab_memory (uae_u8 *apMemory, uae_u32 aSize);

extern void memory_reset (void);
extern unsigned chipmem_checksum(void);

typedef uae_u32 (*mem_get_func)(uaecptr) REGPARAM;
typedef void (*mem_put_func)(uaecptr, uae_u32) REGPARAM;
typedef uae_u8 *(*xlate_func)(uaecptr) REGPARAM;
typedef int (*check_func)(uaecptr, uae_u32) REGPARAM;

extern char *address_space, *good_address_map;
extern uae_u8 *chipmemory;
extern uae_u16 *chipmemory_word;

extern uae_u32 allocated_chipmem;
extern uae_u32 allocated_fastmem;
extern uae_u32 allocated_bogomem;
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
extern uae_u32 allocated_gfxmem;
extern uae_u32 allocated_z3fastmem;
extern uae_u32 allocated_a3000mem;
#endif

#undef DIRECT_MEMFUNCS_SUCCESSFUL
#include "maccess.h"

#define kickmem_size 0x080000

#define chipmem_start 0x00000000
#define bogomem_start 0x00C00000
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
#define a3000mem_start 0x07000000
#endif
#define kickmem_start 0x00F80000

extern int ersatzkickfile;
extern int cloanto_rom;

typedef struct {
    /* These ones should be self-explanatory... */
    mem_get_func lget, wget, bget;
    mem_put_func lput, wput, bput;
    /* Use xlateaddr to translate an Amiga address to a uae_u8 * that can
     * be used to address memory without calling the wget/wput functions.
     * This doesn't work for all memory banks, so this function may call
     * abort(). */
    xlate_func xlateaddr;
    /* To prevent calls to abort(), use check before calling xlateaddr.
     * It checks not only that the memory bank can do xlateaddr, but also
     * that the pointer points to an area of at least the specified size.
     * This is used for example to translate bitplane pointers in custom.c */
    check_func check;
    /* For those banks that refer to real memory, we can save the whole trouble
       of going through function calls, and instead simply grab the memory
       ourselves. This holds the memory address where the start of memory is
       for this particular bank. */
    uae_u8 *baseaddr;
    char *name;
    /* for instruction opcode/operand fetches */
    mem_get_func lgeti, wgeti;
    int flags;
} addrbank;

extern uae_u8 *filesysory;
extern uae_u8 *rtarea;

extern addrbank chipmem_bank;
extern addrbank kickmem_bank;
extern addrbank custom_bank;
extern addrbank clock_bank;
extern addrbank cia_bank;
extern addrbank rtarea_bank;
extern addrbank expamem_bank;
extern addrbank fastmem_bank;
extern addrbank gfxmem_bank;

extern void rtarea_init (void);
extern void rtarea_setup (void);
extern void expamem_init (void);
extern void expamem_reset (void);
extern void rtarea_cleanup (void);

#if !( defined(PANDORA) || defined(ANDROIDSDL) )
extern uae_u32 gfxmem_start;
extern uae_u8 *gfxmemory;
extern uae_u32 gfxmem_mask;
#endif

/* Default memory access functions */

extern int default_check(uaecptr addr, uae_u32 size) REGPARAM;
extern uae_u8 *default_xlate(uaecptr addr) REGPARAM;

#define bankindex(addr) (((uaecptr)(addr)) >> 16)

extern addrbank *mem_banks[65536];
#define get_mem_bank(addr) (*mem_banks[bankindex(addr)])
#define put_mem_bank(addr, b) \
  (mem_banks[bankindex(addr)] = (b));

extern void memory_init (void);
extern void memory_cleanup (void);
extern void map_banks (addrbank *bank, int first, int count, int realsize);

#define NONEXISTINGDATA 0

#define longget(addr) (call_mem_get_func(get_mem_bank(addr).lget, addr))
#define wordget(addr) (call_mem_get_func(get_mem_bank(addr).wget, addr))
#define byteget(addr) (call_mem_get_func(get_mem_bank(addr).bget, addr))
#define longput(addr,l) (call_mem_put_func(get_mem_bank(addr).lput, addr, l))
#define wordput(addr,w) (call_mem_put_func(get_mem_bank(addr).wput, addr, w))
#define byteput(addr,b) (call_mem_put_func(get_mem_bank(addr).bput, addr, b))

#ifndef MD_HAVE_MEM_1_FUNCS

#define longget_1 longget
#define wordget_1 wordget
#define byteget_1 byteget
#define longput_1 longput
#define wordput_1 wordput
#define byteput_1 byteput

#endif

static __inline__ uae_u32 get_long(uaecptr addr)
{
    return longget_1(addr);
}
static __inline__ uae_u32 get_word(uaecptr addr)
{
    return wordget_1(addr);
}
static __inline__ uae_u32 get_byte(uaecptr addr)
{
    return byteget_1(addr);
}
static __inline__ void put_long(uaecptr addr, uae_u32 l)
{
    longput_1(addr, l);
}
static __inline__ void put_word(uaecptr addr, uae_u32 w)
{
    wordput_1(addr, w);
}
static __inline__ void put_byte(uaecptr addr, uae_u32 b)
{
    byteput_1(addr, b);
}

static __inline__ uae_u8 * get_real_address(uaecptr addr)
{
    return get_mem_bank(addr).xlateaddr(addr);
}

static __inline__ int valid_address(uaecptr addr, uae_u32 size)
{
    return get_mem_bank(addr).check(addr, size);
}

/* For faster access in custom chip emulation.  */
extern uae_u32 chipmem_lget (uaecptr) REGPARAM;
extern uae_u32 chipmem_wget (uaecptr) REGPARAM;
extern uae_u32 chipmem_bget (uaecptr) REGPARAM;
extern void chipmem_lput (uaecptr, uae_u32) REGPARAM;
extern void chipmem_wput (uaecptr, uae_u32) REGPARAM;
extern void chipmem_bput (uaecptr, uae_u32) REGPARAM;

	/* IMPORTANT: Mask was hard-coded to ~0xfff00000 => 1MB chip memory MAX.
	 * 				Mask is now set to chipmem_mask => 8MB chip memory MAX.
	 */
extern uae_u32 chipmem_mask;

static __inline__ uae_u16 CHIPMEM_WGET (uae_u32 PT) {
   /* ! 68020+ CPUs can read/write words and longs at odd addresses ! */
   if (PT & 1) {
      return (*((uae_u8 *)&chipmemory[(PT - 1) & chipmem_mask]) << 8) |
              *((uae_u8 *)&chipmemory[(PT + 2) & chipmem_mask]);
   }
   else {
      return *((uae_u16 *)&chipmemory[PT & chipmem_mask]);
   }
}
static __inline void CHIPMEM_WPUT (uae_u32 PT, uae_u16 DA) {
   /* ! 68020+ CPUs can read/write words and longs at odd addresses ! */
   if (PT & 1) {
      *((uae_u8 *)&chipmemory[(PT - 1) & chipmem_mask]) = DA >> 8;
      *((uae_u8 *)&chipmemory[(PT + 2) & chipmem_mask]) = DA;
   }
   else {
      *((uae_u16 *)&chipmemory[PT & chipmem_mask]) = DA;
   }
}
static __inline__ uae_u32 CHIPMEM_LGET(uae_u32 PT) {
   /* ! 68020+ CPUs can read/write words and longs at odd addresses ! */
   if (PT & 1) {
      return (CHIPMEM_WGET(PT) << 16) | CHIPMEM_WGET(PT + 2);
   }
   else {
      return ((uae_u32)(*((uae_u16 *)&chipmemory[PT & chipmem_mask])) << 16) |
                       (*((uae_u16 *)&chipmemory[(PT + 2) & chipmem_mask]));
   }
}

// Custom chips use always even addresses -> no check required
static __inline__ uae_u16 CHIPMEM_WGET_CUSTOM (uae_u32 PT) {
  return *((uae_u16 *)&chipmemory[PT & chipmem_mask]);
}
static __inline void CHIPMEM_WPUT_CUSTOM (uae_u32 PT, uae_u16 DA) {
  *((uae_u16 *)&chipmemory[PT & chipmem_mask]) = DA;
}
static __inline__ uae_u32 CHIPMEM_LGET_CUSTOM(uae_u32 PT) {
  return ((uae_u32)(*((uae_u16 *)&chipmemory[PT & chipmem_mask])) << 16) |
                   (*((uae_u16 *)&chipmemory[(PT + 2) & chipmem_mask]));
}


extern uae_u8 *mapped_malloc (size_t, const char *);
extern void mapped_free (uae_u8 *);

#if defined(USE_ARMV7)

extern "C" {
	void *arm_memset(void *s, int c, size_t n);
	void *arm_memcpy(void *dest, const void *src, size_t n);
}

/* 4-byte alignment */
//#define UAE4ALL_ALIGN __attribute__ ((__aligned__ (4)))
#define UAE4ALL_ALIGN __attribute__ ((__aligned__ (16)))
#define uae4all_memclr(p,l) arm_memset(p,0,l)
#define uae4all_memcpy arm_memcpy

#else

/* 4-byte alignment */
#define UAE4ALL_ALIGN __attribute__ ((__aligned__ (4)))
#define uae4all_memcpy memcpy
#define uae4all_memclr(p,l) memset(p, 0, l)

#endif

#endif
