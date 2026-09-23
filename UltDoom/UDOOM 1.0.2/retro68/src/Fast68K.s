| Faster inner loops for the 68040 (Retro68 build).  Hand-written; the
| routines in Draw68K.s / Blit68K.s are Lion's originals, translated.
|
| No FPU instructions, so these run on the 68LC040 too.

	.text

| -----------------------------------------------------------------------
| void R_DrawSpan040 (void)
|
| Same job as Lion's R_DrawSpanAsm: one horizontal span of a 64x64 flat,
| ds_x1..ds_x2 on row ds_y, through ds_colormap.
|
| The position is kept packed as two 6.10 fixed-point numbers, x in the
| upper word and y in the lower word (Lion packed them the other way round):
|
|     move.l  pos,idx
|     lsr.w   d7,idx      | (d7 = 10) idx.w = y (0..63), upper word x
|     rol.l   #6,idx      | idx.w = y << 6 | x  -- the flat offset
|
| replaces two BFEXTUs, a shift and an add: bit-field instructions are
| slow on the 68040.  As in id's 386 code, a carry out of y's fraction
| nudges x's fraction by 1/1024 of a texel; nothing visible.
| -----------------------------------------------------------------------

	.globl	R_DrawSpan040
	.align	4
R_DrawSpan040:
	movem.l	%a2/%d2-%d7,-(%sp)

	move.l	ds_y,%d1
	lea	ylookup,%a1
	move.l	(%a1,%d1.l*4),%a1
	move.l	ds_x1,%d0
	lea	columnofs,%a0
	add.l	(%a0,%d0.l*4),%a1	| a1 = dest
	move.l	ds_x2,%d3
	sub.l	%d0,%d3			| d3 = count - 1
	bmi	.Lspan_done

	move.l	ds_source,%a0
	move.l	ds_colormap,%a2

	| d2 = x << 10 in the upper word, y >> 6 in the lower word
	move.l	ds_xfrac,%d2
	moveq	#10,%d0
	lsl.l	%d0,%d2
	clr.w	%d2
	move.l	ds_yfrac,%d0
	lsr.l	#6,%d0
	and.l	#0xFFFF,%d0
	or.l	%d0,%d2

	| d4 = the step, packed the same way
	move.l	ds_xstep,%d4
	moveq	#10,%d0
	lsl.l	%d0,%d4
	clr.w	%d4
	move.l	ds_ystep,%d0
	lsr.l	#6,%d0
	and.l	#0xFFFF,%d0
	or.l	%d0,%d4

	moveq	#0,%d5			| texel (upper bytes stay zero)
	moveq	#10,%d7			| shift count (immediates stop at 8)
	addq.l	#1,%d3
	move.l	%d3,%d6
	lsr.l	#2,%d6
	beq.s	.Lspan_single
	subq.l	#1,%d6

.Lspan_quad:
	move.l	%d2,%d0
	lsr.w	%d7,%d0
	rol.l	#6,%d0
	add.l	%d4,%d2
	move.b	(%a0,%d0.w),%d5
	move.b	(%a2,%d5.w),(%a1)+

	move.l	%d2,%d0
	lsr.w	%d7,%d0
	rol.l	#6,%d0
	add.l	%d4,%d2
	move.b	(%a0,%d0.w),%d5
	move.b	(%a2,%d5.w),(%a1)+

	move.l	%d2,%d0
	lsr.w	%d7,%d0
	rol.l	#6,%d0
	add.l	%d4,%d2
	move.b	(%a0,%d0.w),%d5
	move.b	(%a2,%d5.w),(%a1)+

	move.l	%d2,%d0
	lsr.w	%d7,%d0
	rol.l	#6,%d0
	add.l	%d4,%d2
	move.b	(%a0,%d0.w),%d5
	move.b	(%a2,%d5.w),(%a1)+

	dbra	%d6,.Lspan_quad

.Lspan_single:
	andi.l	#3,%d3
	beq.s	.Lspan_done
	subq.l	#1,%d3
.Lspan_one:
	move.l	%d2,%d0
	lsr.w	%d7,%d0
	rol.l	#6,%d0
	add.l	%d4,%d2
	move.b	(%a0,%d0.w),%d5
	move.b	(%a2,%d5.w),(%a1)+
	dbra	%d3,.Lspan_one

.Lspan_done:
	movem.l	(%sp)+,%a2/%d2-%d7
	rts
