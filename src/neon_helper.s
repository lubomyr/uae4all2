@ Some functions and tests to increase performance in drawing.cpp and custom.cpp

.arm

.global ARM_Read_Byte
.global ARM_Read_Word
.global ARM_Read_Long
.global ARM_Write_Byte
.global ARM_Write_Word
.global ARM_Write_Long
.global ARM_doline_n1
.global NEON_doline_n2
.global NEON_doline_n3
.global NEON_doline_n4
.global NEON_doline_n6
.global NEON_doline_n8


.text

.align 8

ARM_Read_Byte:
  ldr       r2, =mem_handlerRB
  bic       r0, r0, #-16777216
  mov       r1, r0, lsr #16
  ldr       r3, [r2, r1, asl #2]
  cmp       r3, #0
  movne     pc, r3                @ Jump to mem_handler if not null (addr already in r0)
  ldr       r2, =mem_data
  eor       r3, r0, #1
  ldr       r1, [r2, r1, asl #2]
  ldrb      r0, [r1, r3]
  bx        lr


ARM_Read_Word:
  ldr       r2, =mem_handlerRW
  bic       r0, r0, #-16777216
  mov       r1, r0, lsr #16
  ldr       r3, [r2, r1, asl #2]
  cmp       r3, #0
  movne     pc, r3                @ Jump to mem_handler if not null (addr already in r0)
  ldr       r2, =mem_data
  tst       r0, #1
  ldr       r2, [r2, r1, asl #2]
  ldreqh    r0, [r2, r0]
  bxeq      lr
  eor       r3, r0, #1
  ldr       r1, [r2, r3]
  uxth      r0, r1, ror #24
  bx        lr


ARM_Read_Long:
  ldr       r2, =mem_handlerRW
  bic       r0, r0, #-16777216
  mov       r1, r0, lsr #16
  ldr       r3, [r2, r1, asl #2]
  cmp       r3, #0
  bne       ARM_Read_Long_viaMemHandler
  ldr       r2, =mem_data
  tst       r0, #1
  ldr       r2, [r2, r1, asl #2]
  ldreq     r1, [r2, r0]
  roreq     r0, r1, #16
  bxeq      lr

  eor       r3, r0, #1
  ldr       r1, [r2, r3]
  mov       r1, r1, ror #24
  add       r3, r3, #2
  ldr       r0, [r2, r3]
  mov       r0, r0, ror #24
  bfi       r0, r1, #16, #16
  bx        lr

ARM_Read_Long_viaMemHandler:
	stmdb	    sp!, {r4, r5, lr}
  mov       r4, r0
  mov       r5, r3
  bl        ARM_Read_Long_callMemHandler
  mov       r3, r5
  mov       r5, r0
  add       r0, r4, #2
  bl        ARM_Read_Long_callMemHandler
  bfi       r0, r5, #16, #16
 	ldmia	    sp!, {r4, r5, pc}
ARM_Read_Long_callMemHandler:
  mov       pc, r3 


ARM_Write_Byte:
  ldr       r2, =mem_handlerWB
  bic       r0, r0, #-16777216
  mov       r3, r0, lsr #16
  ldr       r2, [r2, r3, asl #2]
  cmp       r2, #0
  movne     pc, r2                @ Jump to mem_handler if not null
  ldr       r2, =mem_data
  eor       r0, r0, #1
  ldr       r2, [r2, r3, asl #2]
  strb      r1, [r2, r0]
  bx        lr


ARM_Write_Word:
  ldr       r2, =mem_handlerWW
  bic       r0, r0, #-16777216
  mov       r3, r0, lsr #16
  ldr       r2, [r2, r3, asl #2]
  cmp       r2, #0
  movne     pc, r2                @ Jump to mem_handler if not null
  ldr       r2, =mem_data
  tst       r0, #1
  ldr       r2, [r2, r3, asl #2]
  streqh    r1, [r2, r0]
  bxeq      lr
  add       r2, r2, r0
  strb      r1, [r2, #2]
  mov     	r0, r1, lsr #8
  strb      r0, [r2, #-1]
  bx        lr


ARM_Write_Long:
  ldr       r2, =mem_handlerWW
  bic       r0, r0, #-16777216
  mov       r3, r0, lsr #16
  ldr       r2, [r2, r3, asl #2]
  cmp       r2, #0
  bne       ARM_Write_Long_viaMemHandler
  ldr       r2, =mem_data
  tst       r0, #1
  ldr       r2, [r2, r3, asl #2]
  roreq     r1, r1, #16
  streq     r1, [r2, r0]
  bxeq      lr

  add       r2, r2, r0
  strb      r1, [r2, #4]
  mov       r0, r1, lsr #8
  strb      r0, [r2, #1]
  mov       r0, r1, lsr #16
  strb      r0, [r2, #2]
  mov       r0, r1, lsr #24
  strb      r0, [r2, #-1]
  bx        lr

ARM_Write_Long_viaMemHandler:
	stmdb	    sp!, {r0, r1, r2, lr}
  mov       r1, r1, lsr #16
  bl        ARM_Write_Long_callMemHandler
 	ldmia	    sp!, {r0, r1, r2, lr}
  add       r0, r0, #2
ARM_Write_Long_callMemHandler:
  mov       pc, r2 


.align 8

@----------------------------------------------------------------
@ ARM_doline_n1
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void ARM_doline_n1(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
ARM_doline_n1:
  stmdb     sp!, {r4-r9, lr}

  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r3, r3, r2           @ real_bplpt[0]

  ldr       r4, =Lookup_doline_n1

  sub       r3, r3, #4
  
ARM_doline_n1_loop:
  ldr       r2, [r3, #4]!
@  add       r3, r3, #4

  ubfx      r5, r2, #28, #4
  add       r5, r4, r5, lsl #2
  ldr       r6, [r5]
  
  ubfx      r5, r2, #24, #4
  add       r5, r4, r5, lsl #2
  ldr       r7, [r5]

  ubfx      r5, r2, #20, #4
  add       r5, r4, r5, lsl #2
  ldr       r8, [r5]

  ubfx      r5, r2, #16, #4
  add       r5, r4, r5, lsl #2
  ldr       r9, [r5]
  stmia     r0!, {r6-r9}
  
  ubfx      r5, r2, #12, #4
  add       r5, r4, r5, lsl #2
  ldr       r6, [r5]

  ubfx      r5, r2, #8, #4
  add       r5, r4, r5, lsl #2
  ldr       r7, [r5]

  ubfx      r5, r2, #4, #4
  add       r5, r4, r5, lsl #2
  ldr       r8, [r5]

  ubfx      r5, r2, #0, #4
  add       r5, r4, r5, lsl #2
  ldr       r9, [r5]
  stmia     r0!, {r6-r9}

  subs      r1, r1, #1
  bgt       ARM_doline_n1_loop
  
  ldmia     sp!, {r4-r9, pc}


.align 8

@----------------------------------------------------------------
@ NEON_doline_n2
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void NEON_doline_n2(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
NEON_doline_n2:

  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r2, r3, r2           @ real_bplpt[0]
  add       r3, r2, #200

@ Load masks to registers
  vmov.u8   d18, #0x55
  vmov.u8   d19, #0x33
  vmov.u8   d20, #0x0f

NEON_doline_n2_loop:
  @ Load data as early as possible
  vldmia    r2!, {d4}
  vldmia    r3!, {d6}

@      MERGE (b6, b7, 0x55555555, 1);
  vshr.u8   d16, d4, #1      @ tmpb = b >> shift
  vshl.u8   d17, d6, #1      @ tmpa = a << shift
  vbit.u8   d6, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d18     @ b = b and bit set from tmpa if mask is false

@      MERGE_0(b4, b6, 0x33333333, 2);
  vshr.u8   d16, d6, #2		  @ tmp = b >> shift
  vand.8    d2, d16, d19     @ a = tmp & mask
  vand.8    d6, d6, d19     @ b = b & mask
@      MERGE_0(b5, b7, 0x33333333, 2);
  vshr.u8   d16, d4, #2		  @ tmp = b >> shift
  vand.8    d0, d16, d19     @ a = tmp & mask
  vand.8    d4, d4, d19     @ b = b & mask

@      MERGE_0(b0, b4, 0x0f0f0f0f, 4);
  vshr.u8   d16, d2, #4		  @ tmp = b >> shift
  vand.8    d3, d16, d20     @ a = tmp & mask
  vand.8    d2, d2, d20     @ b = b & mask
@      MERGE_0(b1, b5, 0x0f0f0f0f, 4);
  vshr.u8   d16, d0, #4			@ tmp = b >> shift
  vand.8    d1, d16, d20     @ a = tmp & mask
  vand.8    d0, d0, d20     @ b = b & mask
@      MERGE_0(b2, b6, 0x0f0f0f0f, 4);
  vshr.u8   d16, d6, #4			@ tmp = b >> shift
  vand.8    d7, d16, d20     @ a = tmp & mask
  vand.8    d6, d6, d20     @ b = b & mask
@      MERGE_0(b3, b7, 0x0f0f0f0f, 4);
  vshr.u8   d16, d4, #4			@ tmp = b >> shift
  vand.8    d5, d16, d20     @ a = tmp & mask
  vand.8    d4, d4, d20     @ b = b & mask
  
  vzip.8    d3, d7
  vzip.8    d1, d5
  vzip.8    d2, d6
  vzip.8    d0, d4

  vzip.8    d3, d1
  vzip.8    d2, d0
  vzip.32   d3, d2
  vzip.32   d1, d0

  vst1.8    {d0, d1, d2, d3}, [r0]!
  
  cmp       r1, #1    @ Exit from here if odd number of words
  bxeq      lr
   
  subs      r1, r1, #2    @ We handle 2 words (64 bit) per loop: wordcount -= 2

  vzip.8    d7, d5
  vzip.8    d6, d4
  vzip.32   d7, d6
  vzip.32   d5, d4

  vst1.8    {d4, d5, d6, d7}, [r0]!

  bgt       NEON_doline_n2_loop
  
NEON_doline_n2_exit:
  bx        lr


.align 8

@----------------------------------------------------------------
@ NEON_doline_n3
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void NEON_doline_n3(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
NEON_doline_n3:
  stmdb     sp!, {r4, lr}

  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r2, r3, r2           @ real_bplpt[0]
  add       r3, r2, #200
  add       r4, r3, #200

  @ Load data as early as possible
  vldmia    r4!, {d0}

@ Load masks to registers
  vmov.u8   d18, #0x55
  vmov.u8   d19, #0x33
  vmov.u8   d20, #0x0f

NEON_doline_n3_loop:
@ Load from real_bplpt (now loaded earlier)
@  vld1.8    d0, [r4]!
@  vld1.8    d4, [r2]!
@  vld1.8    d6, [r3]!

  @ Load data as early as possible
  vldmia    r2!, {d4}
  vldmia    r3!, {d6}

@      MERGE_0(b4, b5, 0x55555555, 1);
  vshr.u8   d16, d0, #1		  @ tmp = b >> shift
  vand.8    d2, d16, d18     @ a = tmp & mask
  vand.8    d0, d0, d18     @ b = b & mask

@      MERGE (b6, b7, 0x55555555, 1);
  vshr.u8   d16, d4, #1      @ tmpb = b >> shift
  vshl.u8   d17, d6, #1      @ tmpa = a << shift
  vbit.u8   d6, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d18     @ b = b and bit set from tmpa if mask is false

@      MERGE (b4, b6, 0x33333333, 2);
  vshr.u8   d16, d6, #2      @ tmpb = b >> shift
  vshl.u8   d17, d2, #2      @ tmpa = a << shift
  vbit.u8   d2, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b5, b7, 0x33333333, 2);
  vshr.u8   d16, d4, #2      @ tmpb = b >> shift
  vshl.u8   d17, d0, #2      @ tmpa = a << shift
  vbit.u8   d0, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d19     @ b = b and bit set from tmpa if mask is false

@      MERGE_0(b0, b4, 0x0f0f0f0f, 4);
  vshr.u8   d16, d2, #4		  @ tmp = b >> shift
  vand.8    d3, d16, d20     @ a = tmp & mask
  vand.8    d2, d2, d20     @ b = b & mask
@      MERGE_0(b1, b5, 0x0f0f0f0f, 4);
  vshr.u8   d16, d0, #4			@ tmp = b >> shift
  vand.8    d1, d16, d20     @ a = tmp & mask
  vand.8    d0, d0, d20     @ b = b & mask
@      MERGE_0(b2, b6, 0x0f0f0f0f, 4);
  vshr.u8   d16, d6, #4			@ tmp = b >> shift
  vand.8    d7, d16, d20     @ a = tmp & mask
  vand.8    d6, d6, d20     @ b = b & mask
@      MERGE_0(b3, b7, 0x0f0f0f0f, 4);
  vshr.u8   d16, d4, #4			@ tmp = b >> shift
  vand.8    d5, d16, d20     @ a = tmp & mask
  vand.8    d4, d4, d20     @ b = b & mask
  
  vzip.8    d3, d7
  vzip.8    d1, d5
  vzip.8    d2, d6
  vzip.8    d0, d4

  vzip.8    d3, d1
  vzip.8    d2, d0
  vzip.32   d3, d2
  vzip.32   d1, d0

  vst1.8    {d0, d1, d2, d3}, [r0]!
  
  cmp       r1, #1    @ Exit from here if odd number of words
  ldmeqia   sp!, {r4, pc}
  
  subs      r1, r1, #2    @ We handle 2 words (64 bit) per loop: wordcount -= 2

  @ Load next data (if needed) as early as possible
  vldmiagt  r4!, {d0}

  vzip.8    d7, d5
  vzip.8    d6, d4
  vzip.32   d7, d6
  vzip.32   d5, d4

  vst1.8    {d4, d5, d6, d7}, [r0]!

  bgt       NEON_doline_n3_loop
  
NEON_doline_n3_exit:
  ldmia     sp!, {r4, pc}


.align 8

@----------------------------------------------------------------
@ NEON_doline_n4
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void NEON_doline_n4(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
NEON_doline_n4:
  stmdb     sp!, {r4-r5, lr}

  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r2, r3, r2           @ real_bplpt[0]
  add       r3, r2, #200
  add       r4, r3, #200
  add       r5, r4, #200

  @ Load data as early as possible
  vldmia    r4!, {d0}
  vldmia    r5!, {d2}

@ Load masks to registers
  vmov.u8   d18, #0x55
  vmov.u8   d19, #0x33
  vmov.u8   d20, #0x0f

NEON_doline_n4_loop:
@ Load from real_bplpt (now loaded earlier)
@  vld1.8    d0, [r4]!
@  vld1.8    d2, [r5]!
@  vld1.8    d4, [r2]!
@  vld1.8    d6, [r3]!

  @ Load data as early as possible
  vldmia    r2!, {d4}

@      MERGE (b4, b5, 0x55555555, 1);
  vshr.u8   d16, d0, #1      @ tmpb = b >> shift
  vshl.u8   d17, d2, #1      @ tmpa = a << shift

  vldmia    r3!, {d6}

  vbit.u8   d2, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d0, d17, d18     @ b = b and bit set from tmpa if mask is false
@      MERGE (b6, b7, 0x55555555, 1);
  vshr.u8   d16, d4, #1      @ tmpb = b >> shift
  vshl.u8   d17, d6, #1      @ tmpa = a << shift
  vbit.u8   d6, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d18     @ b = b and bit set from tmpa if mask is false

@      MERGE (b4, b6, 0x33333333, 2);
  vshr.u8   d16, d6, #2      @ tmpb = b >> shift
  vshl.u8   d17, d2, #2      @ tmpa = a << shift
  vbit.u8   d2, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b5, b7, 0x33333333, 2);
  vshr.u8   d16, d4, #2      @ tmpb = b >> shift
  vshl.u8   d17, d0, #2      @ tmpa = a << shift
  vbit.u8   d0, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d19     @ b = b and bit set from tmpa if mask is false

@      MERGE_0(b0, b4, 0x0f0f0f0f, 4);
  vshr.u8   d16, d2, #4		  @ tmp = b >> shift
  vand.8    d3, d16, d20     @ a = tmp & mask
  vand.8    d2, d2, d20     @ b = b & mask
@      MERGE_0(b1, b5, 0x0f0f0f0f, 4);
  vshr.u8   d16, d0, #4			@ tmp = b >> shift
  vand.8    d1, d16, d20     @ a = tmp & mask
  vand.8    d0, d0, d20     @ b = b & mask
@      MERGE_0(b2, b6, 0x0f0f0f0f, 4);
  vshr.u8   d16, d6, #4			@ tmp = b >> shift
  vand.8    d7, d16, d20     @ a = tmp & mask
  vand.8    d6, d6, d20     @ b = b & mask
@      MERGE_0(b3, b7, 0x0f0f0f0f, 4);
  vshr.u8   d16, d4, #4			@ tmp = b >> shift
  vand.8    d5, d16, d20     @ a = tmp & mask
  vand.8    d4, d4, d20     @ b = b & mask
  
  vzip.8    d3, d7
  vzip.8    d1, d5
  vzip.8    d2, d6
  vzip.8    d0, d4

  vzip.8    d3, d1
  vzip.8    d2, d0
  vzip.32   d3, d2
  vzip.32   d1, d0

  vst1.8    {d0, d1, d2, d3}, [r0]!
  
  cmp       r1, #1    @ Exit from here if odd number of words
  ldmeqia   sp!, {r4-r5, pc}
  
  subs      r1, r1, #2    @ We handle 2 words (64 bit) per loop: wordcount -= 2

  @ Load next data (if needed) as early as possible
  vldmiagt  r4!, {d0}

  vzip.8    d7, d5
  vzip.8    d6, d4

  vldmiagt  r5!, {d2}

  vzip.32   d7, d6
  vzip.32   d5, d4

  vst1.8    {d4, d5, d6, d7}, [r0]!

  bgt       NEON_doline_n4_loop
  
NEON_doline_n4_exit:
  ldmia     sp!, {r4-r5, pc}


.align 8

@----------------------------------------------------------------
@ NEON_doline_n6
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void NEON_doline_n6(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
NEON_doline_n6:
  stmdb     sp!, {r4-r7, lr}
  
  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r2, r3, r2           @ real_bplpt[0]
  add       r3, r2, #200
  add       r4, r3, #200
  add       r5, r4, #200
  add       r6, r5, #200
  add       r7, r6, #200
  
@ Load masks to registers
  vmov.u8   d18, #0x55
  vmov.u8   d19, #0x33
  vmov.u8   d20, #0x0f

NEON_doline_n6_loop:
  @ Load data as early as possible
  vldmia    r6!, {d5}
  vldmia    r7!, {d7}

  @ Load data as early as possible
  vldmia    r4!, {d0}
@      MERGE (b2, b3, 0x55555555, 1);
  vshr.u8   d16, d5, #1      @ tmpb = b >> shift
  vshl.u8   d17, d7, #1      @ tmpa = a << shift
  @ Load data as early as possible
  vldmia    r5!, {d2}
  vbit.u8   d7, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d5, d17, d18     @ b = b and bit set from tmpa if mask is false
  @ Load data as early as possible
  vldmia    r2!, {d4}
@      MERGE (b4, b5, 0x55555555, 1);
  vshr.u8   d16, d0, #1      @ tmpb = b >> shift
  vshl.u8   d17, d2, #1      @ tmpa = a << shift
  @ Load data as early as possible
  vldmia    r3!, {d6}
  vbit.u8   d2, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d0, d17, d18     @ b = b and bit set from tmpa if mask is false
@      MERGE (b6, b7, 0x55555555, 1);
  vshr.u8   d16, d4, #1      @ tmpb = b >> shift
  vshl.u8   d17, d6, #1      @ tmpa = a << shift
  vbit.u8   d6, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d18     @ b = b and bit set from tmpa if mask is false

@      MERGE_0(b0, b2, 0x33333333, 2);
  vshr.u8   d16, d7, #2		  @ tmp = b >> shift
  vand.8    d3, d16, d19     @ a = tmp & mask
  vand.8    d7, d7, d19     @ b = b & mask
@      MERGE_0(b1, b3, 0x33333333, 2);
  vshr.u8   d16, d5, #2		  @ tmp = b >> shift
  vand.8    d1, d16, d19     @ a = tmp & mask
  vand.8    d5, d5, d19     @ b = b & mask
@      MERGE (b4, b6, 0x33333333, 2);
  vshr.u8   d16, d6, #2      @ tmpb = b >> shift
  vshl.u8   d17, d2, #2      @ tmpa = a << shift
  vbit.u8   d2, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b5, b7, 0x33333333, 2);
  vshr.u8   d16, d4, #2      @ tmpb = b >> shift
  vshl.u8   d17, d0, #2      @ tmpa = a << shift
  vbit.u8   d0, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d19     @ b = b and bit set from tmpa if mask is false

@      MERGE (b0, b4, 0x0f0f0f0f, 4);
  vshr.u8   d16, d2, #4      @ tmpb = b >> shift
  vshl.u8   d17, d3, #4      @ tmpa = a << shift
  vbit.u8   d3, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d2, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b1, b5, 0x0f0f0f0f, 4);
  vshr.u8   d16, d0, #4      @ tmpb = b >> shift
  vshl.u8   d17, d1, #4      @ tmpa = a << shift
  vbit.u8   d1, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d0, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b2, b6, 0x0f0f0f0f, 4);
  vshr.u8   d16, d6, #4      @ tmpb = b >> shift
  vshl.u8   d17, d7, #4      @ tmpa = a << shift
  vbit.u8   d7, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b3, b7, 0x0f0f0f0f, 4);
  vshr.u8   d16, d4, #4      @ tmpb = b >> shift
  vshl.u8   d17, d5, #4      @ tmpa = a << shift
  vbit.u8   d5, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d20     @ b = b and bit set from tmpa if mask is false

  vzip.8    d3, d7
  vzip.8    d1, d5
  vzip.8    d2, d6
  vzip.8    d0, d4

  vzip.8    d3, d1
  vzip.8    d2, d0
  vzip.32   d3, d2
  vzip.32   d1, d0

  vst1.8    {d0, d1, d2, d3}, [r0]!
  
  cmp       r1, #1    @ Exit from here if odd number of words
  ldmeqia   sp!, {r4-r7, pc}

  subs      r1, r1, #2    @ We handle 2 words (64 bit) per loop: wordcount -= 2

  vzip.8    d7, d5
  vzip.8    d6, d4
  vzip.32   d7, d6
  vzip.32   d5, d4

  vst1.8    {d4, d5, d6, d7}, [r0]!

  bgt       NEON_doline_n6_loop
  
NEON_doline_n6_exit:
  ldmia     sp!, {r4-r7, pc}
 

.align 8

@----------------------------------------------------------------
@ NEON_doline_n8
@
@ r0: uae_u32   *pixels
@ r1: int       wordcount
@ r2: int       lineno 
@
@ void NEON_doline_n8(uae_u32 *pixels, int wordcount, int lineno);
@
@----------------------------------------------------------------
NEON_doline_n8:
  stmdb     sp!, {r4-r9, lr}
  
  mov       r3, #1600
  mul       r2, r2, r3
  ldr       r3, =line_data
  add       r2, r3, r2           @ real_bplpt[0]
  add       r3, r2, #200
  add       r4, r3, #200
  add       r5, r4, #200
  add       r6, r5, #200
  add       r7, r6, #200
  add       r8, r7, #200
  add       r9, r8, #200
  
  @ Load data as early as possible
  vldmia    r8!, {d1}
  vldmia    r9!, {d3}

@ Load masks to registers
  vmov.u8   d18, #0x55
  vmov.u8   d19, #0x33
  vmov.u8   d20, #0x0f

NEON_doline_n8_loop:
  @ Load data as early as possible
  vldmia    r6!, {d5}
@      MERGE (b0, b1, 0x55555555, 1);
  vshr.u8   d16, d1, #1      @ tmpb = b >> shift
  vshl.u8   d17, d3, #1      @ tmpa = a << shift
  @ Load data as early as possible
  vldmia    r7!, {d7}
  vbit.u8   d3, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d1, d17, d18     @ b = b and bit set from tmpa if mask is false
  @ Load data as early as possible
  vldmia    r4!, {d0}
@      MERGE (b2, b3, 0x55555555, 1);
  vshr.u8   d16, d5, #1      @ tmpb = b >> shift
  vshl.u8   d17, d7, #1      @ tmpa = a << shift
  @ Load data as early as possible
  vldmia    r5!, {d2}
  vbit.u8   d7, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d5, d17, d18     @ b = b and bit set from tmpa if mask is false
  @ Load data as early as possible
  vldmia    r2!, {d4}
@      MERGE (b4, b5, 0x55555555, 1);
  vshr.u8   d16, d0, #1      @ tmpb = b >> shift
  vshl.u8   d17, d2, #1      @ tmpa = a << shift
  @ Load data as early as possible
  vldmia    r3!, {d6}
  vbit.u8   d2, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d0, d17, d18     @ b = b and bit set from tmpa if mask is false
@      MERGE (b6, b7, 0x55555555, 1);
  vshr.u8   d16, d4, #1      @ tmpb = b >> shift
  vshl.u8   d17, d6, #1      @ tmpa = a << shift
  vbit.u8   d6, d16, d18     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d18     @ b = b and bit set from tmpa if mask is false

@      MERGE (b0, b2, 0x33333333, 2);
  vshr.u8   d16, d7, #2      @ tmpb = b >> shift
  vshl.u8   d17, d3, #2      @ tmpa = a << shift
  vbit.u8   d3, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d7, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b1, b3, 0x33333333, 2);
  vshr.u8   d16, d5, #2      @ tmpb = b >> shift
  vshl.u8   d17, d1, #2      @ tmpa = a << shift
  vbit.u8   d1, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d5, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b4, b6, 0x33333333, 2);
  vshr.u8   d16, d6, #2      @ tmpb = b >> shift
  vshl.u8   d17, d2, #2      @ tmpa = a << shift
  vbit.u8   d2, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d19     @ b = b and bit set from tmpa if mask is false
@      MERGE (b5, b7, 0x33333333, 2);
  vshr.u8   d16, d4, #2      @ tmpb = b >> shift
  vshl.u8   d17, d0, #2      @ tmpa = a << shift
  vbit.u8   d0, d16, d19     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d19     @ b = b and bit set from tmpa if mask is false

@      MERGE (b0, b4, 0x0f0f0f0f, 4);
  vshr.u8   d16, d2, #4      @ tmpb = b >> shift
  vshl.u8   d17, d3, #4      @ tmpa = a << shift
  vbit.u8   d3, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d2, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b1, b5, 0x0f0f0f0f, 4);
  vshr.u8   d16, d0, #4      @ tmpb = b >> shift
  vshl.u8   d17, d1, #4      @ tmpa = a << shift
  vbit.u8   d1, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d0, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b2, b6, 0x0f0f0f0f, 4);
  vshr.u8   d16, d6, #4      @ tmpb = b >> shift
  vshl.u8   d17, d7, #4      @ tmpa = a << shift
  vbit.u8   d7, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d6, d17, d20     @ b = b and bit set from tmpa if mask is false
@      MERGE (b3, b7, 0x0f0f0f0f, 4);
  vshr.u8   d16, d4, #4      @ tmpb = b >> shift
  vshl.u8   d17, d5, #4      @ tmpa = a << shift
  vbit.u8   d5, d16, d20     @ a = a and bit set from tmpb if mask is true 
  vbif.u8   d4, d17, d20     @ b = b and bit set from tmpa if mask is false

  vzip.8    d3, d7
  vzip.8    d1, d5
  vzip.8    d2, d6
  vzip.8    d0, d4

  vzip.8    d3, d1
  vzip.8    d2, d0
  vzip.32   d3, d2
  vzip.32   d1, d0

  vst1.8    {d0, d1, d2, d3}, [r0]!
  
  cmp       r1, #1    @ Exit from here if odd number of words
  ldmeqia   sp!, {r4-r9, pc}

  subs      r1, r1, #2    @ We handle 2 words (64 bit) per loop: wordcount -= 2

  @ Load data as early as possible
  vldmiagt  r8!, {d1}
  
  vzip.8    d7, d5
  vzip.8    d6, d4

  @ Load data as early as possible
  vldmiagt  r9!, {d3}

  vzip.32   d7, d6
  vzip.32   d5, d4

  vst1.8    {d4, d5, d6, d7}, [r0]!

  bgt       NEON_doline_n8_loop
  
NEON_doline_n8_exit:
  ldmia     sp!, {r4-r9, pc}


.data

.align 8

Lookup_doline_n1:
  .long 0x00000000, 0x01000000, 0x00010000, 0x01010000
  .long 0x00000100, 0x01000100, 0x00010100, 0x01010100
  .long 0x00000001, 0x01000001, 0x00010001, 0x01010001
  .long 0x00000101, 0x01000101, 0x00010101, 0x01010101
