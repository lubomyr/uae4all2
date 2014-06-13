/*****************************************************************************/
/* FAME Fast and Accurate Motorola 68000 Emulation Core                      */
/* (c) 2005 Oscar Orallo Pelaez                                              */
/* Version: 1.24                                                             */
/* Date: 08-20-2005                                                          */
/* See FAME.HTML for documentation and license information                   */
/*****************************************************************************/

#ifndef __FAME_H__
#define __FAME_H__

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define UAE4ALL_ALIGN __attribute__ ((__aligned__ (16)))

/************************************/
/* General library defines          */
/************************************/

#ifndef M68K_OK
    #define M68K_OK 0
#endif
#ifndef M68K_RUNNING
    #define M68K_RUNNING 1
#endif
#ifndef M68K_NO_SUP_ADDR_SPACE
    #define M68K_NO_SUP_ADDR_SPACE 2
#endif
#ifndef M68K_DOUBLE_BUS_FAULT
    #define M68K_DOUBLE_BUS_FAULT -1
#endif
#ifndef M68K_INV_REG
    #define M68K_INV_REG -1
#endif

/* Hardware interrupt state */

#ifndef M68K_IRQ_LEVEL_ERROR
    #define M68K_IRQ_LEVEL_ERROR -1
#endif
#ifndef M68K_IRQ_INV_PARAMS
    #define M68K_IRQ_INV_PARAMS -2
#endif

/* Defines to specify hardware interrupt type */

#ifndef M68K_AUTOVECTORED_IRQ
    #define M68K_AUTOVECTORED_IRQ -1
#endif
#ifndef M68K_SPURIOUS_IRQ
    #define M68K_SPURIOUS_IRQ -2
#endif

#ifndef M68K_AUTO_LOWER_IRQ
	#define M68K_AUTO_LOWER_IRQ 1
#endif
#ifndef M68K_MANUAL_LOWER_IRQ
	#define M68K_MANUAL_LOWER_IRQ 0
#endif

/* Defines to specify address space */

#ifndef M68K_SUP_ADDR_SPACE
    #define M68K_SUP_ADDR_SPACE 0
#endif
#ifndef M68K_USER_ADDR_SPACE
    #define M68K_USER_ADDR_SPACE 2
#endif
#ifndef M68K_PROG_ADDR_SPACE
    #define M68K_PROG_ADDR_SPACE 0
#endif
#ifndef M68K_DATA_ADDR_SPACE
    #define M68K_DATA_ADDR_SPACE 1
#endif


/*******************/
/* Data definition */
/*******************/
#ifdef u8
#undef u8
#endif

#ifdef s8
#undef s8
#endif

#ifdef u16
#undef u16
#endif

#ifdef s16
#undef s16
#endif

#ifdef u32
#undef u32
#endif

#ifdef s32
#undef s32
#endif

#define u8	unsigned char
#define s8	signed char
#define u16	unsigned short
#define s16	signed short
#define u32	unsigned int
#define s32	signed int

#define u64 unsigned long long
#define s64 long long


/* M68K registers */
typedef enum {
      M68K_REG_D0=0,
      M68K_REG_D1,
      M68K_REG_D2,
      M68K_REG_D3,
      M68K_REG_D4,
      M68K_REG_D5,
      M68K_REG_D6,
      M68K_REG_D7,
      M68K_REG_A0,
      M68K_REG_A1,
      M68K_REG_A2,
      M68K_REG_A3,
      M68K_REG_A4,
      M68K_REG_A5,
      M68K_REG_A6,
      M68K_REG_A7,
      M68K_REG_USP,
      M68K_REG_PC,
      M68K_REG_SR
} m68k_register;

typedef union
{
	u8 B;
	s8 SB;
	u16 W;
	s16 SW;
	u32 D;
	s32 SD;
} famec_union32;


/* M68K CPU CONTEXT */
typedef struct
{
	famec_union32   dreg[8];
	famec_union32   areg[8];
	u32 *icust_handler;
	u32 usp;
	u32 pc;
	u32 cycles_counter;
	u8  interrupts[8];
	u16 sr;
	u16 execinfo;
   u32 vbr, sfc, dfc;  /* Control Registers, 68010+ */
   u32 cacr, caar;     /* Control Registers, 68020+ */
   u32 msp, isp;       /* Master/Interrupt Stack Pointer */
// We put everything what is needed for m68k_emulate and the opcodes in this 
// struct, so there is only one base register in generated assembly code. 
// This reduces the number of used and backed up register and we have better performance.
  u32 flag_c;
  u32 flag_v;
  u32 flag_notz;
  u32 flag_n;
  u32 flag_x;
  u32 flag_s;
  u32 flag_i;
  u32 flag_m;
  u32 flag_t;
  u16 *_pc;
  u32 basepc;
  u32 fetch[256];
  s32 more_cycles_to_do;
  s32 cycles_not_done;
  s32 io_cycle_counter;
} M68K_CONTEXT;


/************************/
/* Function definition  */
/************************/
#ifdef __cplusplus
extern "C" {
#endif

/* General purpose functions */
void     m68k_init(int force_table);
unsigned m68k_reset(void);
unsigned m68k_emulate(int n);
unsigned m68k_get_pc(void);
int      m68k_fetch(unsigned address);

/* CPU context handling functions */
M68K_CONTEXT *m68k_get_context(void);
void famec_SetBank(u32 low_addr, u32 high_addr, u32 fetch, void *rb, void *rw, void *wb, void *ww, void *data);
int  m68k_set_register(m68k_register reg, unsigned value);

/* Timing functions */
void     m68k_release_timeslice(void);


#ifdef __cplusplus
}
#endif

#endif
