;*******************************************************************
;
;                      MSX Sound Chips (68K)
;
; The MSX2 PSG (AY-3-8910) and MSX-MUSIC (YM2413/OPLL) emulated
; with SCSP slots, run by the sound 68K. The SH-2 queues chip
; register writes in RING; an RING_FRAME entry applies the frame's
; writes to the SCSP.
;
; Slots: 0-2 PSG A-C, 3-14 OPLL channels 0-5 (modulator, carrier),
; 15-20 rhythm phase bits and noise, 21-22 bass drum, 23-25 hi-hat
; logic, 26 hi-hat, 27 snare, 28 precomputed hi-hat tone. The
; hi-hat reads the tone table for channel 7's pitch if HHTAB.BIN
; has one, else the logic slots.
;
; Routines take arguments in d0-d4/a1 and may change d0-d4/a0-a1.
;
; This was written entirely by Claude Opus 5.5
;
;*******************************************************************

SCSP            equ $100000
SCIPD           equ $100420
SCIRE           equ $100422
SAMPLE_IRQ      equ 10              ;sample interval interrupt bit of SCIPD/SCIRE

KYONEX          equ $1000
KYONB           equ $0800
LOOP_NORMAL     equ $0020
PCM8B           equ $0010
SDIR            equ $0100

;Sound RAM layout, shared with msxchip.c and scsp.h.
PROGRAM_END     equ $3000
STACK_TOP       equ $3800
RING            equ $3800           ;256 words of (type << 8) | value
RING_WR         equ $3A00           ;written by the SH-2
RING_RD         equ $3A01
IDLE_COUNT      equ $3A04           ;idle loops in the last frame
RING_FRAME      equ $FF
RING_OPLL       equ $40

WAVE_BASE       equ $4000
FM_CYCLE        equ 1024
WAVE_SINE       equ WAVE_BASE+$00000+4*FM_CYCLE*2
WAVE_HALFSINE   equ WAVE_BASE+$05000+4*FM_CYCLE*2
WAVE_PSG_NOISE  equ WAVE_BASE+$0E000
WAVE_OPLL_NOISE equ WAVE_BASE+$1E000
WAVE_NOISE_LEN  equ $FFFF
WAVE_BIT_HH_A   equ WAVE_BASE+$2E000
WAVE_BIT_HH_B   equ WAVE_BASE+$2E800
WAVE_BIT_CYM_A  equ WAVE_BASE+$2F000
WAVE_BIT_CYM_B  equ WAVE_BASE+$2F800
WAVE_BIT_SD     equ WAVE_BASE+$30000
WAVE_XOR_LOOKUP equ WAVE_BASE+$30800+512*2
WAVE_OR_LOOKUP  equ WAVE_BASE+$31000+512*2
WAVE_SN_LOOKUP  equ WAVE_BASE+$31800+512*2
WAVE_HH_LOOKUP  equ WAVE_BASE+$32000+512*2
WAVE_SD_LOOKUP  equ WAVE_BASE+$32800+512*2
WAVE_HH_RAMP    equ WAVE_BASE+$33000+512*2
HH_TABLES       equ WAVE_BASE+$33800

;Band-limited PSG squares: count.w, then count period limits.w (table i for periods >= limit i),
;tables at +$40. All in one 64K bank, so a table switch only rewrites the low address word.
PSG_SQUARES     equ $44000
PSG_SQ_TABLES   equ PSG_SQUARES+$40
PSG_SQ_STRIDE   equ $804

;HHTAB.BIN: count.w, pad.w, then per table key.w (fnum | block << 9), length.w, offset.l.
HH_COUNT        equ 0
HH_ENTRIES      equ 4
HH_KEY          equ 0
HH_LENGTH       equ 2
HH_OFFSET       equ 4
HH_ENTRY_SIZE   equ 8

;Modulation levels for the lookup tables built by scsp.c.
MDL_BIT         equ $9
MDL_LOOKUP      equ $A

SLOT_PSG        equ 0
SLOT_FM         equ 3
SLOT_PHASE      equ 15
PHASE_COUNT     equ 5
SLOT_NOISE      equ 20
SLOT_BD         equ 21
SLOT_XOR        equ 23
SLOT_OR         equ 24
SLOT_SN         equ 25
SLOT_HH         equ 26
SLOT_SD         equ 27
SLOT_HH_TONE    equ 28
SLOT_COUNT      equ 29
SLOT_HH_A       equ SLOT_PHASE+0
SLOT_CYM_A      equ SLOT_PHASE+1
SLOT_HH_B       equ SLOT_PHASE+2
SLOT_CYM_B      equ SLOT_PHASE+3
SLOT_SD_BIT     equ SLOT_PHASE+4
PHASE_SLOTS     equ ((1<<PHASE_COUNT)-1)<<SLOT_PHASE
;Free-running slots restarted after a two sample key off.
RESTART_SLOTS   equ PHASE_SLOTS|(1<<SLOT_HH_TONE)

;Direct send levels: 5 = -12dB, 6 = -6dB. emu2413 mixes drums at twice the melody level.
PSG_DISDL       equ 5
FM_DISDL        equ 5
DRUM_DISDL      equ 6
;The hi-hat ramp lookup is at half scale.
HH_RAMP_DISDL   equ 7

;SCSP rate index = OPLL rate index + this (decay times compared at 96dB, approximate).
EG_RATE_OFS     equ 4
;Full scale modulator output shifts the carrier +-4 cycles on the OPLL.
MDL_FM          equ $C
;OPLL feedback 7 shifts +-2 cycles, halving per step down.
MDL_FB_OFS      equ 4

;PSG square: 1789772.5 / 16 Hz per period unit, 1024-sample cycle.
PSG_TONE_STEP   equ 170220846
;PSG noise: one LFSR shift per 1789772.5 / 16 / period Hz (datasheet rate).
PSG_NOISE_STEP  equ 166234

VOICE_BD        equ 6
VOICE_HH        equ 7
VOICE_SD        equ 8
VOICE_COUNT     equ 9

;OpllOp fields.
OP_AM           equ 0
OP_PM           equ 1
OP_EG           equ 2
OP_KR           equ 3
OP_ML           equ 4
OP_KL           equ 5
OP_TL           equ 6
OP_WS           equ 7
OP_FB           equ 8
OP_AR           equ 9
OP_DR           equ 10
OP_SL           equ 11
OP_RR           equ 12
OP_SIZE         equ 14

        org     0
        dc.l    STACK_TOP
        dc.l    start
        dcb.l   62,hang

hang:
        bra.s   hang

start:
        move.w  #$2700,sr
        lea     STACK_TOP,sp
        lea     SCSP,a5
        bsr     chipInit
        moveq   #0,d7
mainLoop:
        moveq   #0,d0
        move.b  RING_RD,d0
        cmp.b   RING_WR,d0
        bne.s   .entry
        addq.l  #1,d7
        bra.s   mainLoop
.entry:
        move.w  d0,d1
        add.w   d1,d1
        lea     RING,a0
        move.w  (a0,d1.w),d1
        addq.b  #1,d0
        move.b  d0,RING_RD
        move.w  d1,d0
        lsr.w   #8,d0
        cmp.b   #RING_FRAME,d0
        bne.s   .reg
        move.l  d7,IDLE_COUNT
        moveq   #0,d7
        bsr     chipFrame
        bra.s   mainLoop
.reg:
        cmp.b   #RING_OPLL,d0
        bcc.s   .opll
        bsr     psgWrite
        bra.s   mainLoop
.opll:
        sub.b   #RING_OPLL,d0
        bsr     opllWrite
        bra.s   mainLoop

;d0 = register, d1 = value.
psgWrite:
        andi.w  #15,d0
        lea     psgReg,a0
        move.b  d1,(a0,d0.w)
        rts

;d0 = register 0x00-0x3F, d1 = value. Records key on edges.
opllWrite:
        andi.w  #$3F,d0
        lea     opllReg,a0
        lea     keyEvent,a1
        move.b  (a0,d0.w),d2
        cmp.b   #$20,d0
        bcs.s   .rhythm
        cmp.b   #$20+VOICE_BD,d0
        bcc.s   .store
        btst    #4,d2
        bne.s   .store
        btst    #4,d1
        beq.s   .store
        move.b  #1,-$20(a1,d0.w)
        bra.s   .store
.rhythm:
        cmp.b   #$0E,d0
        bne.s   .store
        move.b  d2,d3
        not.b   d3
        and.b   d1,d3
        btst    #4,d3
        beq.s   .noBd
        move.b  #1,VOICE_BD(a1)
.noBd:
        btst    #0,d3
        beq.s   .noHh
        move.b  #1,VOICE_HH(a1)
.noHh:
        btst    #3,d3
        beq.s   .store
        move.b  #1,VOICE_SD(a1)
.store:
        move.b  d1,(a0,d0.w)
        rts

;d0.l = d0.l / d1.w, unsigned.
div32:
        move.l  d0,d2
        clr.w   d2
        swap    d2
        divu    d1,d2
        move.w  d2,d3
        move.w  d0,d2
        divu    d1,d2
        swap    d3
        move.w  d2,d3
        move.l  d3,d0
        rts

;d0.l = waveform samples per output sample (16.16) -> d0.w = OCT/FNS.
scspPitch:
        tst.l   d0
        bne.s   .nonZero
        move.w  #8<<11,d0
        rts
.nonZero:
        moveq   #0,d1
.up:
        cmp.l   #$20000,d0
        bcs.s   .down
        cmp.w   #7,d1
        bge.s   .down
        lsr.l   #1,d0
        addq.w  #1,d1
        bra.s   .up
.down:
        cmp.l   #$10000,d0
        bcc.s   .fns
        cmp.w   #-8,d1
        ble.s   .fns
        add.l   d0,d0
        subq.w  #1,d1
        bra.s   .down
.fns:
        cmp.l   #$10000,d0
        bcc.s   .frac
        moveq   #0,d0
        bra.s   .oct
.frac:
        sub.l   #$10000,d0
        lsr.l   #6,d0
        cmp.l   #1023,d0
        bls.s   .oct
        move.l  #1023,d0
.oct:
        andi.w  #$F,d1
        moveq   #11,d2
        lsl.w   d2,d1
        or.w    d1,d0
        rts

;d0 = source slot, d1 = reading slot, d2 = 0 for the latest sample, 1 for the one before
;-> d0 = MDXSL/MDYSL value.
modSel:
        sub.w   d1,d0
        bpl.s   .after
        add.w   #32,d0
        move.w  d2,d1
        bra.s   .done
.after:
        cmp.w   #28,d0
        bcs.s   .near
        move.w  d2,d1
        bra.s   .done
.near:
        moveq   #1,d1
        sub.w   d2,d1
.done:
        andi.w  #$1F,d0
        lsl.w   #5,d1
        or.w    d1,d0
        rts

;Starts a slot's waveform with LSA 0. d0 = slot, d1.l = address, d2 = LEA, d3 = 8-bit flag.
setWave:
        moveq   #0,d4
        move.b  d0,d4
        add.w   d4,d4
        lea     slotCtl,a1
        move.w  (a1,d4.w),d0
        andi.w  #KYONB,d0
        ori.w   #LOOP_NORMAL,d0
        tst.b   d3
        beq.s   .pcm16
        ori.w   #PCM8B,d0
.pcm16:
        swap    d1
        or.w    d1,d0
        swap    d1
        move.w  d0,(a1,d4.w)
        lsl.w   #4,d4
        lea     (a5,d4.w),a0
        move.w  d0,(a0)
        move.w  d1,2(a0)
        clr.w   4(a0)
        move.w  d2,6(a0)
        rts

;Sets the KYONB bit of the slots in d0.l to d1 and executes KEY_ON/KEY_OFF.
keySlots:
        movem.l d5-d7/a2,-(sp)
        move.l  d0,d5
        move.b  d1,d6
        moveq   #0,d7
        lea     slotCtl,a2
.slot:
        btst    d7,d5
        beq.s   .next
        move.w  d7,d0
        add.w   d0,d0
        move.w  (a2,d0.w),d1
        tst.b   d6
        beq.s   .off
        ori.w   #KYONB,d1
        bra.s   .write
.off:
        andi.w  #$FFFF-KYONB,d1
.write:
        move.w  d1,(a2,d0.w)
        lsl.w   #4,d0
        move.w  d1,(a5,d0.w)
.next:
        addq.w  #1,d7
        cmp.w   #SLOT_COUNT,d7
        bcs.s   .slot
        move.w  (a5),d0
        ori.w   #KYONEX,d0
        move.w  d0,(a5)
        movem.l (sp)+,d5-d7/a2
        rts

;Waits d0 samples.
waitSamples:
        move.w  #1<<SAMPLE_IRQ,SCIRE
.wait:
        move.w  SCIPD,d1
        btst    #SAMPLE_IRQ,d1
        beq.s   .wait
        subq.w  #1,d0
        bne.s   waitSamples
        rts

;Unpacks operator d0 (0 modulator, 1 carrier) of the patch at a0 into opw.
getOp:
        lea     opw,a1
        move.b  (a0,d0.w),d1
        move.b  d1,d2
        lsr.b   #7,d2
        move.b  d2,OP_AM(a1)
        move.b  d1,d2
        lsr.b   #6,d2
        andi.b  #1,d2
        move.b  d2,OP_PM(a1)
        move.b  d1,d2
        lsr.b   #5,d2
        andi.b  #1,d2
        move.b  d2,OP_EG(a1)
        move.b  d1,d2
        lsr.b   #4,d2
        andi.b  #1,d2
        move.b  d2,OP_KR(a1)
        andi.b  #15,d1
        move.b  d1,OP_ML(a1)
        move.b  3(a0),d1
        tst.w   d0
        bne.s   .carrier
        move.b  2(a0),d2
        lsr.b   #6,d2
        move.b  d2,OP_KL(a1)
        move.b  2(a0),d2
        andi.b  #63,d2
        move.b  d2,OP_TL(a1)
        move.b  d1,d2
        lsr.b   #3,d2
        andi.b  #1,d2
        move.b  d2,OP_WS(a1)
        andi.b  #7,d1
        move.b  d1,OP_FB(a1)
        bra.s   .envelope
.carrier:
        move.b  d1,d2
        lsr.b   #6,d2
        move.b  d2,OP_KL(a1)
        clr.b   OP_TL(a1)
        lsr.b   #4,d1
        andi.b  #1,d1
        move.b  d1,OP_WS(a1)
        clr.b   OP_FB(a1)
.envelope:
        move.b  4(a0,d0.w),d1
        move.b  d1,d2
        lsr.b   #4,d2
        move.b  d2,OP_AR(a1)
        andi.b  #15,d1
        move.b  d1,OP_DR(a1)
        move.b  6(a0,d0.w),d1
        move.b  d1,d2
        lsr.b   #4,d2
        move.b  d2,OP_SL(a1)
        andi.b  #15,d1
        move.b  d1,OP_RR(a1)
        rts

;Attenuation in 0.375dB steps, the same unit as SCSP TL. d0 = fnum, d1 = block, d2 = KSL -> d0.
opllKsl:
        tst.b   d2
        beq.s   .zero
        lsr.w   #5,d0
        lea     opllKsl8,a0
        moveq   #0,d3
        move.b  (a0,d0.w),d3
        moveq   #7,d0
        sub.w   d1,d0
        mulu    #24,d0
        sub.w   d0,d3
        cmp.w   #4,d3
        blt.s   .zero
        lsr.w   #2,d3
        moveq   #3,d0
        sub.b   d2,d0
        lsr.w   d0,d3
        lsl.w   #3,d3
        divu    #3,d3
        moveq   #0,d0
        move.w  d3,d0
        rts
.zero:
        moveq   #0,d0
        rts

;Phase step in 1024-sample cycles per output sample (16.16). d0 = fnum, d1 = block, d2 = ML -> d0.l.
opllStep:
        lea     opllMul2,a0
        andi.w  #15,d2
        move.b  (a0,d2.w),d2
        mulu    d2,d0
        lsl.l   d1,d0
        ;x * 18470 / 256 = x * 72 + x * 38 / 256.
        move.l  d0,d1
        lsl.l   #3,d1
        move.l  d1,d2
        lsl.l   #3,d2
        add.l   d1,d2
        add.l   d0,d0
        move.l  d0,d1
        add.l   d1,d1
        add.l   d1,d0
        lsl.l   #3,d1
        add.l   d1,d0
        lsr.l   #8,d0
        add.l   d2,d0
        rts

;d0 = fnum, d1 = block -> d0 = key scale rate of opw.
opllRks:
        tst.b   opw+OP_KR
        beq.s   .low
        add.w   d1,d1
        lsr.w   #8,d0
        or.w    d1,d0
        rts
.low:
        move.w  d1,d0
        lsr.w   #1,d0
        rts

;d0 = OPLL rate, d1 = key scale rate -> d0 = SCSP rate. Changes d0 and d2 only.
egRate:
        andi.w  #$FF,d0
        beq.s   .done
        move.w  d1,d2
        lsr.w   #2,d2
        add.w   d2,d0
        cmp.w   #15,d0
        bls.s   .high
        moveq   #15,d0
.high:
        add.w   d0,d0
        add.w   d0,d0
        move.w  d1,d2
        andi.w  #3,d2
        add.w   d2,d0
        addq.w  #EG_RATE_OFS+1,d0
        lsr.w   #1,d0
        cmp.w   #31,d0
        bls.s   .done
        moveq   #31,d0
.done:
        rts

;Envelope registers from opw. d0 = slot, d1 = key scale rate, d2 = sustain,
;d3 = released on key off (melody modulators hold their level).
writeEg:
        movem.l d5-d7,-(sp)
        move.w  d0,d5
        move.b  d2,d6
        move.b  d3,d7
        lea     opw,a1
        moveq   #0,d0
        move.b  OP_AR(a1),d0
        move.w  d1,d2
        lsr.w   #2,d2
        add.w   d0,d2
        cmp.w   #15,d2
        bcs.s   .attack
        moveq   #31,d3
        bra.s   .decay
.attack:
        bsr     egRate
        move.w  d0,d3
.decay:
        moveq   #0,d0
        move.b  OP_DR(a1),d0
        bsr     egRate
        move.w  d0,d4
        moveq   #0,d0
        tst.b   OP_EG(a1)
        bne.s   .sustain
        move.b  OP_RR(a1),d0
        bsr     egRate
.sustain:
        moveq   #11,d2
        lsl.w   d2,d0
        lsl.w   #6,d4
        or.w    d0,d4
        or.w    d3,d4
        moveq   #0,d0
        tst.b   d7
        beq.s   .release
        moveq   #5,d0
        tst.b   d6
        bne.s   .rate
        moveq   #7,d0
        tst.b   OP_EG(a1)
        beq.s   .rate
        move.b  OP_RR(a1),d0
.rate:
        bsr     egRate
.release:
        moveq   #0,d2
        move.b  OP_SL(a1),d2
        lsl.w   #5,d2
        or.w    d2,d0
        ori.w   #$F<<10,d0
        lsl.w   #5,d5
        move.w  d4,8(a5,d5.w)
        move.w  d0,$A(a5,d5.w)
        movem.l (sp)+,d5-d7
        rts

;d0 = slot, d1 = attenuation.
writeTl:
        cmp.w   #255,d1
        bls.s   .write
        move.w  #255,d1
.write:
        lsl.w   #5,d0
        move.w  d1,$C(a5,d0.w)
        rts

;d0 = LFO register for opw.
lfoReg:
        lea     opw,a1
        moveq   #0,d0
        move.b  OP_AM(a1),d1
        or.b    OP_PM(a1),d1
        beq.s   .done
        ;OPLL vibrato is about 6.1Hz, +-14 cents; tremolo about 3.7Hz, 4.8dB.
        move.w  #($10<<10)|(2<<8)|(2<<3),d0
        tst.b   OP_PM(a1)
        beq.s   .tremolo
        ori.w   #(4<<10)|(2<<5),d0
        bra.s   .am
.tremolo:
        ori.w   #2<<10,d0
.am:
        tst.b   OP_AM(a1)
        beq.s   .done
        ori.w   #4,d0
.done:
        rts

;A two operator voice: melody channel or bass drum.
;d0 = first slot, d1 = OPLL channel, a1 = patch, d3 = volume, d4 = retrigger.
fmVoice:
        movem.l d5-d7/a2,-(sp)
        move.l  a1,a2
        move.w  d0,fvSlot
        move.b  d3,fvVol
        move.b  d4,fvRetrig
        lea     opllReg,a0
        moveq   #0,d5
        move.b  $10(a0,d1.w),d5
        move.b  $20(a0,d1.w),d2
        btst    #0,d2
        beq.s   .fnum
        bset    #8,d5
.fnum:
        move.w  d5,fvFnum
        moveq   #0,d6
        move.b  d2,d6
        lsr.w   #1,d6
        andi.w  #7,d6
        move.w  d6,fvBlk
        lsr.b   #5,d2
        andi.b  #1,d2
        move.b  d2,fvSus
        moveq   #0,d7
.op:
        move.w  fvSlot,d0
        add.w   d7,d0
        move.w  d0,fvS
        move.l  a2,a0
        move.w  d7,d0
        bsr     getOp
        move.w  fvFnum,d0
        move.w  fvBlk,d1
        bsr     opllRks
        move.w  d0,fvRks
        tst.b   fvRetrig
        beq.s   .env
        move.w  fvS,d0
        move.l  #WAVE_SINE,d1
        tst.b   opw+OP_WS
        beq.s   .wave
        move.l  #WAVE_HALFSINE,d1
.wave:
        move.w  #FM_CYCLE,d2
        moveq   #0,d3
        bsr     setWave
.env:
        move.w  fvS,d0
        move.w  fvRks,d1
        move.b  fvSus,d2
        move.b  d7,d3
        bsr     writeEg
        move.w  fvFnum,d0
        move.w  fvBlk,d1
        move.b  opw+OP_KL,d2
        bsr     opllKsl
        moveq   #0,d1
        tst.w   d7
        beq.s   .modTl
        move.b  fvVol,d1
        lsl.w   #3,d1
        bra.s   .tl
.modTl:
        move.b  opw+OP_TL,d1
        add.w   d1,d1
.tl:
        add.w   d0,d1
        move.w  fvS,d0
        bsr     writeTl
        move.w  fvS,d6
        lsl.w   #5,d6
        tst.w   d7
        beq.s   .modulator
        move.w  fvSlot,d0
        move.w  fvS,d1
        moveq   #0,d2
        bsr     modSel
        move.w  d0,d1
        lsl.w   #6,d1
        or.w    d1,d0
        ori.w   #MDL_FM<<12,d0
        bra.s   .modWrite
.modulator:
        moveq   #0,d0
        moveq   #0,d5
        move.b  opw+OP_FB,d5
        beq.s   .modWrite
        ;Feedback averages the last two outputs, as the OPLL does.
        move.w  fvS,d0
        move.w  d0,d1
        moveq   #1,d2
        bsr     modSel
        move.w  d0,d4
        move.w  fvS,d0
        move.w  d0,d1
        moveq   #0,d2
        bsr     modSel
        lsl.w   #6,d0
        or.w    d4,d0
        addq.w  #MDL_FB_OFS,d5
        moveq   #12,d1
        lsl.w   d1,d5
        or.w    d5,d0
.modWrite:
        move.w  d0,$E(a5,d6.w)
        move.w  fvFnum,d0
        move.w  fvBlk,d1
        moveq   #0,d2
        move.b  opw+OP_ML,d2
        bsr     opllStep
        bsr     scspPitch
        move.w  d0,$10(a5,d6.w)
        bsr     lfoReg
        move.w  d0,$12(a5,d6.w)
        moveq   #0,d0
        tst.w   d7
        beq.s   .send
        move.w  #FM_DISDL<<13,d0
        cmp.w   #SLOT_BD,fvSlot
        bne.s   .send
        move.w  #DRUM_DISDL<<13,d0
.send:
        move.w  d0,$16(a5,d6.w)
        addq.w  #1,d7
        cmp.w   #2,d7
        bcs     .op
        movem.l (sp)+,d5-d7/a2
        rts

;Snare or hi-hat output: a lookup slot with opw's envelope and dvSend's send level.
;d0 = slot, d1 = volume, d2/d3 = input slots, a1 = lookup table, d4 = retrigger.
drumVoice:
        movem.l d5-d7,-(sp)
        move.w  d0,d5
        move.b  d1,dvVol
        move.w  d2,dvX
        move.w  d3,dvY
        move.l  a1,dvWave
        lea     opllReg,a0
        moveq   #0,d6
        move.b  $17(a0),d6
        btst    #0,$27(a0)
        beq.s   .fnum
        bset    #8,d6
.fnum:
        moveq   #0,d7
        move.b  $27(a0),d7
        lsr.w   #1,d7
        andi.w  #7,d7
        move.w  d6,d0
        move.w  d7,d1
        bsr     opllRks
        move.w  d0,dvRks
        tst.b   d4
        beq.s   .env
        move.w  d5,d0
        move.l  dvWave,d1
        moveq   #1,d2
        moveq   #0,d3
        bsr     setWave
.env:
        move.b  opllReg+$27,d2
        lsr.b   #5,d2
        andi.b  #1,d2
        move.w  d5,d0
        move.w  dvRks,d1
        moveq   #1,d3
        bsr     writeEg
        move.w  d6,d0
        move.w  d7,d1
        move.b  opw+OP_KL,d2
        bsr     opllKsl
        moveq   #0,d1
        move.b  dvVol,d1
        lsl.w   #3,d1
        add.w   d0,d1
        move.w  d5,d0
        bsr     writeTl
        move.w  dvX,d0
        move.w  d5,d1
        moveq   #0,d2
        bsr     modSel
        lsl.w   #6,d0
        move.w  d0,d4
        move.w  dvY,d0
        move.w  d5,d1
        moveq   #0,d2
        bsr     modSel
        or.w    d4,d0
        ori.w   #MDL_LOOKUP<<12,d0
        lsl.w   #5,d5
        move.w  d0,$E(a5,d5.w)
        move.w  #8<<11,$10(a5,d5.w)
        move.w  dvSend,$16(a5,d5.w)
        movem.l (sp)+,d5-d7
        rts

;Points the tone slot at the HHTAB.BIN table for channel 7's pitch, or hhEntry = 0 if none.
;Returns d0 = 1 if the tone slot must restart.
hhTone:
        moveq   #0,d1
        move.b  opllReg+$17,d1
        moveq   #$F,d0
        and.b   opllReg+$27,d0
        lsl.w   #8,d0
        or.w    d0,d1
        lea     HH_TABLES,a0
        move.w  HH_COUNT(a0),d2
        lea     HH_ENTRIES(a0),a1
        bra.s   .next
.find:
        cmp.w   HH_KEY(a1),d1
        beq.s   .found
        addq.l  #HH_ENTRY_SIZE,a1
.next:
        dbra    d2,.find
        suba.l  a1,a1
.found:
        moveq   #0,d0
        cmpa.l  hhEntry,a1
        beq.s   .done
        move.l  a1,hhEntry
        beq.s   .done
        move.l  HH_OFFSET(a1),d1
        add.l   #HH_TABLES,d1
        move.w  HH_LENGTH(a1),d2
        moveq   #SLOT_HH_TONE,d0
        moveq   #1,d3
        bsr     setWave
        moveq   #1,d0
.done:
        rts

;Phase bit slot d0 for a rhythm operator. Returns d0 = 1 if it must restart (stopped again).
phaseVoice:
        movem.l d5-d7/a2,-(sp)
        move.w  d0,d7
        lea     phaseBits,a2
        lsl.w   #3,d0
        adda.w  d0,a2
        lea     opllReg,a0
        moveq   #0,d1
        move.b  (a2),d1
        moveq   #0,d5
        move.b  $10(a0,d1.w),d5
        btst    #0,$20(a0,d1.w)
        beq.s   .fnum
        bset    #8,d5
.fnum:
        moveq   #0,d6
        move.b  $20(a0,d1.w),d6
        lsr.w   #1,d6
        andi.w  #7,d6
        moveq   #0,d0
        move.b  1(a2),d0
        lsl.w   #3,d0
        lea     opllRom,a0
        adda.w  d0,a0
        moveq   #0,d0
        move.b  2(a2),d0
        bsr     getOp
        move.w  d5,d0
        move.w  d6,d1
        moveq   #0,d2
        move.b  opw+OP_ML,d2
        bsr     opllStep
        move.l  d0,d5
        lea     phaseStopped,a0
        moveq   #0,d6
        tst.l   d5
        bne.s   .running
        moveq   #1,d6
.running:
        moveq   #0,d4
        tst.b   d6
        beq.s   .restart
        tst.b   (a0,d7.w)
        bne.s   .restart
        moveq   #1,d4
.restart:
        move.b  d4,pvRestart
        cmp.b   (a0,d7.w),d6
        bne.s   .loop
        tst.b   d4
        beq.s   .pitch
.loop:
        ;Stopped: loop on samples 0-1, which hold the phase 0 value.
        move.b  d6,(a0,d7.w)
        move.w  d7,d0
        add.w   #SLOT_PHASE,d0
        move.l  4(a2),d1
        move.w  #1024,d2
        tst.b   d6
        beq.s   .length
        moveq   #1,d2
.length:
        moveq   #1,d3
        bsr     setWave
.pitch:
        move.l  d5,d0
        bsr     scspPitch
        move.w  d7,d1
        add.w   #SLOT_PHASE,d1
        lsl.w   #5,d1
        move.w  d0,$10(a5,d1.w)
        moveq   #0,d0
        move.b  pvRestart,d0
        movem.l (sp)+,d5-d7/a2
        rts

;d1.w = PSG period. Returns d0.w = low address word of its band-limited square.
psgSquare:
        lea     PSG_SQUARES,a0
        move.w  (a0)+,d0
        subq.w  #1,d0
        add.w   d0,d0
.find:
        cmp.w   (a0,d0.w),d1
        bcc.s   .found
        subq.w  #2,d0
        bne.s   .find
.found:
        lsr.w   #1,d0
        mulu.w  #PSG_SQ_STRIDE,d0
        add.w   #PSG_SQ_TABLES&$FFFF,d0
        rts

;PSG channel d0, d1 = retrigger.
psgVoice:
        movem.l d5-d7,-(sp)
        move.w  d0,d7
        move.w  d0,d5
        lsl.w   #5,d5
        tst.b   d1
        beq.s   .pitch
        lea     psgMode,a0
        tst.b   (a0,d7.w)
        beq.s   .square
        move.l  #WAVE_PSG_NOISE,d1
        move.w  #WAVE_NOISE_LEN,d2
        moveq   #1,d3
        bsr     setWave
        bra.s   .pitch
.square:
        move.l  #PSG_SQ_TABLES,d1
        move.w  #1024,d2
        moveq   #0,d3
        bsr     setWave
.pitch:
        lea     psgReg,a0
        move.b  7(a0),d6
        moveq   #3,d0
        add.w   d7,d0
        btst    d0,d6
        bne.s   .tone
        moveq   #31,d1
        and.b   6(a0),d1
        bne.s   .noise
        moveq   #1,d1
.noise:
        move.l  #PSG_NOISE_STEP,d0
        bsr     div32
        bra.s   .step
.tone:
        move.w  d7,d0
        add.w   d0,d0
        moveq   #15,d1
        and.b   1(a0,d0.w),d1
        lsl.w   #8,d1
        move.b  (a0,d0.w),d1
        tst.w   d1
        bne.s   .period
        moveq   #1,d1
.period:
        bsr     psgSquare
        move.w  d0,2(a5,d5.w)
        move.l  #PSG_TONE_STEP,d0
        bsr     div32
.step:
        bsr     scspPitch
        move.w  d0,$10(a5,d5.w)
        ;About 3dB per volume step.
        lea     psgReg,a0
        moveq   #15,d1
        and.b   8(a0,d7.w),d1
        move.w  #255,d0
        tst.b   d1
        beq.s   .tl
        move.b  d6,d2
        lsr.b   d7,d2
        andi.b  #9,d2
        cmp.b   #9,d2
        beq.s   .tl
        moveq   #15,d0
        sub.w   d1,d0
        lsl.w   #3,d0
.tl:
        move.w  d0,$C(a5,d5.w)
        movem.l (sp)+,d5-d7
        rts

;d0 = voice -> d0 = 1 if keyed on.
keyState:
        lea     opllReg,a0
        cmp.w   #VOICE_BD,d0
        bcc.s   .rhythm
        move.b  $20(a0,d0.w),d0
        lsr.b   #4,d0
        andi.w  #1,d0
        rts
.rhythm:
        moveq   #4,d2
        cmp.w   #VOICE_HH,d0
        bne.s   .notHh
        moveq   #0,d2
.notHh:
        cmp.w   #VOICE_SD,d0
        bne.s   .test
        moveq   #3,d2
.test:
        moveq   #0,d0
        move.b  $0E(a0),d1
        btst    #5,d1
        beq.s   .done
        btst    d2,d1
        beq.s   .done
        moveq   #1,d0
.done:
        rts

;Applies the frame's register writes to the SCSP.
chipFrame:
        movem.l d0-d7/a0-a4,-(sp)
        moveq   #0,d6               ;slots to retrigger
        moveq   #0,d5               ;slots to key off
        lea     keyEvent,a2
        lea     keyWas,a3
        lea     voiceSlots,a4
        moveq   #0,d7
.keys:
        move.w  d7,d0
        bsr     keyState
        move.w  d7,d1
        add.w   d1,d1
        add.w   d1,d1
        move.l  (a4,d1.w),d1
        tst.b   (a2,d7.w)
        beq.s   .noEvent
        tst.b   d0
        beq.s   .noEvent
        or.l    d1,d6
        bra.s   .keyNext
.noEvent:
        tst.b   d0
        bne.s   .keyNext
        tst.b   (a3,d7.w)
        beq.s   .keyNext
        or.l    d1,d5
.keyNext:
        move.b  d0,(a3,d7.w)
        clr.b   (a2,d7.w)
        addq.w  #1,d7
        cmp.w   #VOICE_COUNT,d7
        bcs.s   .keys

        lea     psgNew,a2
        lea     psgMode,a3
        moveq   #0,d7
.mode:
        moveq   #8,d0
        lsl.b   d7,d0
        and.b   psgReg+7,d0
        seq     d0
        andi.b  #1,d0
        move.b  d0,(a2,d7.w)
        cmp.b   (a3,d7.w),d0
        beq.s   .modeNext
        bset    d7,d6
.modeNext:
        addq.w  #1,d7
        cmp.w   #3,d7
        bcs.s   .mode

        move.l  d6,d0
        or.l    d5,d0
        beq.s   .psg
        moveq   #0,d1
        bsr     keySlots
        moveq   #2,d0
        bsr     waitSamples

.psg:
        moveq   #0,d7
.psgVoice:
        move.b  (a2,d7.w),(a3,d7.w)
        move.w  d7,d0
        btst    d7,d6
        sne     d1
        bsr     psgVoice
        addq.w  #1,d7
        cmp.w   #3,d7
        bcs.s   .psgVoice

        moveq   #0,d7
.melody:
        lea     opllReg,a0
        moveq   #0,d2
        move.b  $30(a0,d7.w),d2
        moveq   #15,d3
        and.b   d2,d3
        lsr.b   #4,d2
        beq.s   .user
        lsl.w   #3,d2
        lea     opllRom,a1
        adda.w  d2,a1
        bra.s   .patch
.user:
        move.l  a0,a1
.patch:
        move.w  d7,d0
        add.w   d0,d0
        addq.w  #SLOT_FM,d0
        btst    d0,d6
        sne     d4
        move.w  d7,d1
        bsr     fmVoice
        addq.w  #1,d7
        cmp.w   #VOICE_BD,d7
        bcs.s   .melody

        moveq   #SLOT_BD,d0
        moveq   #6,d1
        lea     opllRom+16*8,a1
        moveq   #15,d3
        and.b   opllReg+$36,d3
        btst    #SLOT_BD,d6
        sne     d4
        bsr     fmVoice

        bsr     hhTone
        tst.b   d0
        beq.s   .hhOp
        bset    #SLOT_HH_TONE,d6
.hhOp:
        lea     opllRom+17*8,a0
        moveq   #0,d0
        bsr     getOp
        moveq   #SLOT_HH,d0
        moveq   #0,d1
        move.b  opllReg+$37,d1
        lsr.b   #4,d1
        moveq   #SLOT_SN,d2
        lea     WAVE_HH_LOOKUP,a1
        move.w  #DRUM_DISDL<<13,dvSend
        tst.l   hhEntry
        beq.s   .hhLogic
        moveq   #SLOT_HH_TONE,d2
        lea     WAVE_HH_RAMP,a1
        move.w  #HH_RAMP_DISDL<<13,dvSend
.hhLogic:
        moveq   #SLOT_NOISE,d3
        btst    #SLOT_HH,d6
        sne     d4
        bsr     drumVoice

        lea     opllRom+17*8,a0
        moveq   #1,d0
        bsr     getOp
        moveq   #SLOT_SD,d0
        moveq   #15,d1
        and.b   opllReg+$37,d1
        moveq   #SLOT_SD_BIT,d2
        moveq   #SLOT_NOISE,d3
        lea     WAVE_SD_LOOKUP,a1
        move.w  #DRUM_DISDL<<13,dvSend
        btst    #SLOT_SD,d6
        sne     d4
        bsr     drumVoice

        moveq   #0,d7
.phase:
        move.w  d7,d0
        bsr     phaseVoice
        tst.b   d0
        beq.s   .phaseNext
        moveq   #SLOT_PHASE,d0
        add.w   d7,d0
        bset    d0,d6
.phaseNext:
        addq.w  #1,d7
        cmp.w   #PHASE_COUNT,d7
        bcs.s   .phase

        move.l  d6,d0
        andi.l  #RESTART_SLOTS,d0
        beq.s   .keyOn
        moveq   #0,d1
        bsr     keySlots
        moveq   #2,d0
        bsr     waitSamples
.keyOn:
        move.l  d6,d0
        beq.s   .done
        moveq   #1,d1
        bsr     keySlots
.done:
        movem.l (sp)+,d0-d7/a0-a4
        rts

;Always-on helper slot: raw output to the sound stack only.
;d0 = slot, d1.l = address, d2 = LEA, d3 = 8-bit flag, d4.l = step (16.16).
helperSlot:
        move.l  d4,-(sp)
        move.w  d0,-(sp)
        bsr     setWave
        move.w  (sp)+,d0
        lsl.w   #5,d0
        lea     (a5,d0.w),a0
        move.w  #31,8(a0)
        move.w  #$F<<10,$A(a0)
        move.w  #SDIR,$C(a0)
        move.l  (sp)+,d0
        bsr     scspPitch
        move.w  d0,$10(a0)
        rts

;Two-input logic lookup on the phase bit slots. d0 = slot, d1.l = table, d2/d3 = input slots.
logicSlot:
        movem.l d5-d7,-(sp)
        move.w  d0,d5
        move.w  d2,d6
        move.w  d3,d7
        moveq   #1,d2
        moveq   #0,d3
        moveq   #0,d4
        bsr     helperSlot
        move.w  d6,d0
        move.w  d5,d1
        moveq   #0,d2
        bsr     modSel
        lsl.w   #6,d0
        move.w  d0,d4
        move.w  d7,d0
        move.w  d5,d1
        moveq   #0,d2
        bsr     modSel
        or.w    d4,d0
        ori.w   #MDL_BIT<<12,d0
        lsl.w   #5,d5
        move.w  d0,$E(a5,d5.w)
        movem.l (sp)+,d5-d7
        rts

chipInit:
        lea     workStart,a0
.clear:
        clr.b   (a0)+
        cmp.l   #workEnd,a0
        bcs.s   .clear
        move.b  #$B8,psgReg+7

        moveq   #0,d7
.psg:
        move.w  d7,d0
        move.l  #PSG_SQ_TABLES,d1
        move.w  #1024,d2
        moveq   #0,d3
        bsr     setWave
        move.w  d7,d0
        lsl.w   #5,d0
        lea     (a5,d0.w),a0
        move.w  #31,8(a0)
        move.w  #($F<<10)|31,$A(a0)
        move.w  #255,$C(a0)
        move.w  #PSG_DISDL<<13,$16(a0)
        addq.w  #1,d7
        cmp.w   #3,d7
        bcs.s   .psg

        lea     phaseBits,a2
        lea     phaseStopped,a3
        moveq   #0,d7
.phase:
        move.b  #1,(a3,d7.w)
        moveq   #SLOT_PHASE,d0
        add.w   d7,d0
        move.l  4(a2),d1
        moveq   #1,d2
        moveq   #1,d3
        moveq   #0,d4
        bsr     helperSlot
        addq.l  #8,a2
        addq.w  #1,d7
        cmp.w   #PHASE_COUNT,d7
        bcs.s   .phase

        moveq   #SLOT_NOISE,d0
        move.l  #WAVE_OPLL_NOISE,d1
        move.w  #WAVE_NOISE_LEN,d2
        moveq   #1,d3
        move.l  #$10000,d4
        bsr     helperSlot
        ;Silent (the snare bit table's sample 0) until hhTone picks a table.
        moveq   #SLOT_HH_TONE,d0
        move.l  #WAVE_BIT_SD,d1
        moveq   #1,d2
        moveq   #1,d3
        move.l  #$10000,d4
        bsr     helperSlot
        ;short_noise = (h3 ^ c5) | ((h2 ^ h7) | (c3 ^ c5)).
        moveq   #SLOT_XOR,d0
        move.l  #WAVE_XOR_LOOKUP,d1
        moveq   #SLOT_HH_B,d2
        moveq   #SLOT_CYM_B,d3
        bsr     logicSlot
        moveq   #SLOT_OR,d0
        move.l  #WAVE_OR_LOOKUP,d1
        moveq   #SLOT_HH_A,d2
        moveq   #SLOT_CYM_A,d3
        bsr     logicSlot
        moveq   #SLOT_SN,d0
        move.l  #WAVE_SN_LOOKUP,d1
        moveq   #SLOT_XOR,d2
        moveq   #SLOT_OR,d3
        bsr     logicSlot

        move.l  #(7<<SLOT_PSG)|RESTART_SLOTS|(1<<SLOT_NOISE)|(7<<SLOT_XOR),d0
        moveq   #1,d1
        bsr     keySlots
        rts

;emu2413 YM2413 tones 1-15 and the rhythm tones (16 BD, 17 HH/SD, 18 TOM/CYM, from VRC7).
opllRom:
        dc.b    $00,$00,$00,$00,$00,$00,$00,$00
        dc.b    $71,$61,$1e,$17,$d0,$78,$00,$17
        dc.b    $13,$41,$1a,$0d,$d8,$f7,$23,$13
        dc.b    $13,$01,$99,$00,$f2,$c4,$21,$23
        dc.b    $11,$61,$0e,$07,$8d,$64,$70,$27
        dc.b    $32,$21,$1e,$06,$e1,$76,$01,$28
        dc.b    $31,$22,$16,$05,$e0,$71,$00,$18
        dc.b    $21,$61,$1d,$07,$82,$81,$11,$07
        dc.b    $33,$21,$2d,$13,$b0,$70,$00,$07
        dc.b    $61,$61,$1b,$06,$64,$65,$10,$17
        dc.b    $41,$61,$0b,$18,$85,$f0,$81,$07
        dc.b    $33,$01,$83,$11,$ea,$ef,$10,$04
        dc.b    $17,$c1,$24,$07,$f8,$f8,$22,$12
        dc.b    $61,$50,$0c,$05,$d2,$f5,$40,$42
        dc.b    $01,$01,$55,$03,$e9,$90,$03,$02
        dc.b    $41,$41,$89,$03,$f1,$e4,$c0,$13
        dc.b    $01,$01,$18,$0f,$df,$f8,$6a,$6d
        dc.b    $01,$01,$00,$00,$c8,$d8,$a7,$68
        dc.b    $05,$01,$00,$00,$f8,$aa,$59,$55

;emu2413 ml_table (multiplier x 2).
opllMul2:
        dc.b    1,2,4,6,8,10,12,14,16,18,20,20,24,24,30,30
;emu2413 kl_table in dB x 8.
opllKsl8:
        dc.b    0,72,96,111,120,129,135,141,144,150,153,156,159,162,165,168

        even
;Rhythm phase bit slots in slot order: OPLL channel, rhythm tone, carrier operator, pad, bit table.
phaseBits:
        dc.b    7,17,0,0
        dc.l    WAVE_BIT_HH_A
        dc.b    8,18,1,0
        dc.l    WAVE_BIT_CYM_A
        dc.b    7,17,0,0
        dc.l    WAVE_BIT_HH_B
        dc.b    8,18,1,0
        dc.l    WAVE_BIT_CYM_B
        dc.b    7,17,1,0
        dc.l    WAVE_BIT_SD

;Slots of each OPLL voice.
voiceSlots:
        dc.l    3<<(SLOT_FM+0),3<<(SLOT_FM+2),3<<(SLOT_FM+4)
        dc.l    3<<(SLOT_FM+6),3<<(SLOT_FM+8),3<<(SLOT_FM+10)
        dc.l    3<<SLOT_BD,1<<SLOT_HH,1<<SLOT_SD

        even
workStart:
slotCtl:        ds.w    SLOT_COUNT
fvSlot:         ds.w    1
fvS:            ds.w    1
fvFnum:         ds.w    1
fvBlk:          ds.w    1
fvRks:          ds.w    1
dvX:            ds.w    1
dvY:            ds.w    1
dvRks:          ds.w    1
dvWave:         ds.l    1
dvSend:         ds.w    1
hhEntry:        ds.l    1
opw:            ds.b    OP_SIZE
psgReg:         ds.b    16
opllReg:        ds.b    $40
psgMode:        ds.b    3
psgNew:         ds.b    3
keyEvent:       ds.b    VOICE_COUNT
keyWas:         ds.b    VOICE_COUNT
phaseStopped:   ds.b    PHASE_COUNT
fvVol:          ds.b    1
fvRetrig:       ds.b    1
fvSus:          ds.b    1
dvVol:          ds.b    1
pvRestart:      ds.b    1
        even
workEnd:

        ifgt    workEnd-PROGRAM_END
        fail    "68K program overlaps the stack"
        endif
