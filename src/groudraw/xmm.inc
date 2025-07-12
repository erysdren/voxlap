.Model FLAT, C   ;Need this - Prevents segment overrides
;06/29/2000: XMM.INC file started (by Ken Silverman)
;07/01/2000: Fully working
;03/15/2001: Fixed src/dest order bugs for: pavg*,pmax*,pmin*,psad*,maskmov*
;               WARNING! Couldn't find a macro to support: pinsrw mm?,m16,i8
;11/06/2001: Axel Meijer wrote new section for the 3DNow! instructions.
;11/07/2001: Axel Meijer simplifies 3DNow! code & fixes register swap bug
.686P

DEFMMX macro
	XMM0 TEXTEQU EAX
	XMM1 TEXTEQU ECX
	XMM2 TEXTEQU EDX
	XMM3 TEXTEQU EBX
	XMM4 TEXTEQU ESP
	XMM5 TEXTEQU EBP
	XMM6 TEXTEQU ESI
	XMM7 TEXTEQU EDI
	xmm0 TEXTEQU EAX
	xmm1 TEXTEQU ECX
	xmm2 TEXTEQU EDX
	xmm3 TEXTEQU EBX
	xmm4 TEXTEQU ESP
	xmm5 TEXTEQU EBP
	xmm6 TEXTEQU ESI
	xmm7 TEXTEQU EDI
	MM0 TEXTEQU EAX
	MM1 TEXTEQU ECX
	MM2 TEXTEQU EDX
	MM3 TEXTEQU EBX
	MM4 TEXTEQU ESP
	MM5 TEXTEQU EBP
	MM6 TEXTEQU ESI
	MM7 TEXTEQU EDI
	mm0 TEXTEQU EAX
	mm1 TEXTEQU ECX
	mm2 TEXTEQU EDX
	mm3 TEXTEQU EBX
	mm4 TEXTEQU ESP
	mm5 TEXTEQU EBP
	mm6 TEXTEQU ESI
	mm7 TEXTEQU EDI
endm

DEFMMX0 macro
	XMM0 TEXTEQU XMMREG
	XMM1 TEXTEQU XMMREG
	XMM2 TEXTEQU XMMREG
	XMM3 TEXTEQU XMMREG
	XMM4 TEXTEQU XMMREG
	XMM5 TEXTEQU XMMREG
	XMM6 TEXTEQU XMMREG
	XMM7 TEXTEQU XMMREG
	xmm0 TEXTEQU XMMREG
	xmm1 TEXTEQU XMMREG
	xmm2 TEXTEQU XMMREG
	xmm3 TEXTEQU XMMREG
	xmm4 TEXTEQU XMMREG
	xmm5 TEXTEQU XMMREG
	xmm6 TEXTEQU XMMREG
	xmm7 TEXTEQU XMMREG
	MM0 TEXTEQU MMXREG
	MM1 TEXTEQU MMXREG
	MM2 TEXTEQU MMXREG
	MM3 TEXTEQU MMXREG
	MM4 TEXTEQU MMXREG
	MM5 TEXTEQU MMXREG
	MM6 TEXTEQU MMXREG
	MM7 TEXTEQU MMXREG
	mm0 TEXTEQU MMXREG
	mm1 TEXTEQU MMXREG
	mm2 TEXTEQU MMXREG
	mm3 TEXTEQU MMXREG
	mm4 TEXTEQU MMXREG
	mm5 TEXTEQU MMXREG
	mm6 TEXTEQU MMXREG
	mm7 TEXTEQU MMXREG
endm

UNDEFMMX macro
	XMM0 TEXTEQU XMM?
	XMM1 TEXTEQU XMM?
	XMM2 TEXTEQU XMM?
	XMM3 TEXTEQU XMM?
	XMM4 TEXTEQU XMM?
	XMM5 TEXTEQU XMM?
	XMM6 TEXTEQU XMM?
	XMM7 TEXTEQU XMM?
	xmm0 TEXTEQU xmm?
	xmm1 TEXTEQU xmm?
	xmm2 TEXTEQU xmm?
	xmm3 TEXTEQU xmm?
	xmm4 TEXTEQU xmm?
	xmm5 TEXTEQU xmm?
	xmm6 TEXTEQU xmm?
	xmm7 TEXTEQU xmm?
	MM0 TEXTEQU MM?
	MM1 TEXTEQU MM?
	MM2 TEXTEQU MM?
	MM3 TEXTEQU MM?
	MM4 TEXTEQU MM?
	MM5 TEXTEQU MM?
	MM6 TEXTEQU MM?
	MM7 TEXTEQU MM?
	mm0 TEXTEQU mm?
	mm1 TEXTEQU mm?
	mm2 TEXTEQU mm?
	mm3 TEXTEQU mm?
	mm4 TEXTEQU mm?
	mm5 TEXTEQU mm?
	mm6 TEXTEQU mm?
	mm7 TEXTEQU mm?
endm

DEFX2 macro
	XMM0 TEXTEQU mm0
	XMM1 TEXTEQU mm1
	XMM2 TEXTEQU mm2
	XMM3 TEXTEQU mm3
	XMM4 TEXTEQU mm4
	XMM5 TEXTEQU mm5
	XMM6 TEXTEQU mm6
	XMM7 TEXTEQU mm7
	xmm0 TEXTEQU mm0
	xmm1 TEXTEQU mm1
	xmm2 TEXTEQU mm2
	xmm3 TEXTEQU mm3
	xmm4 TEXTEQU mm4
	xmm5 TEXTEQU mm5
	xmm6 TEXTEQU mm6
	xmm7 TEXTEQU mm7
endm

UNDEFX2 macro
	XMM0 TEXTEQU mm?
	XMM1 TEXTEQU mm?
	XMM2 TEXTEQU mm?
	XMM3 TEXTEQU mm?
	XMM4 TEXTEQU mm?
	XMM5 TEXTEQU mm?
	XMM6 TEXTEQU mm?
	XMM7 TEXTEQU mm?
	xmm0 TEXTEQU mm?
	xmm1 TEXTEQU mm?
	xmm2 TEXTEQU mm?
	xmm3 TEXTEQU mm?
	xmm4 TEXTEQU mm?
	xmm5 TEXTEQU mm?
	xmm6 TEXTEQU mm?
	xmm7 TEXTEQU mm?
endm

;===================AMD 3DNow! specific instructions begins===================

amd3dop macro op, dst, src
	local u, v, x, y
	local ?SpecCase

	?SpecCase=1
	ifidn src, <mm0>
		?SpecCase=0
	endif
	ifidn src, <mm1>
		?SpecCase=0
	endif
	ifidn src, <mm2>
		?SpecCase=0
	endif
	ifidn src, <mm3>
		?SpecCase=0
	endif
	ifidn src, <mm4>
		?SpecCase=0
	endif
	ifidn src, <mm5>
		?SpecCase=0
	endif
	ifidn src, <mm6>
		?SpecCase=0
	endif
	ifidn src, <mm7>
		?SpecCase=0
	endif
	ifidn src, <MM0>
		?SpecCase=0
	endif
	ifidn src, <MM1>
		?SpecCase=0
	endif
	ifidn src, <MM2>
		?SpecCase=0
	endif
	ifidn src, <MM3>
		?SpecCase=0
	endif
	ifidn src, <MM4>
		?SpecCase=0
	endif
	ifidn src, <MM5>
		?SpecCase=0
	endif
	ifidn src, <MM6>
		?SpecCase=0
	endif
	ifidn src, <MM7>
		?SpecCase=0
	endif

	DEFMMX

	if ?SpecCase eq 1
		x: cmpxchg dword ptr src, dst
		x=x+1
		y: org x
		db 0fh
		org y
		db op
	else
		db 0fh
		u: mov dst, src
		v: org u
		db 0fh
		org v
		db op
	endif
	UNDEFMMX
endm

pavgusb macro dst, src
	amd3dop 0bfh, dst, src
endm

pf2id macro dst, src
	amd3dop 01dh, dst, src
endm

pfacc macro dst, src
	amd3dop 0aeh, dst, src
endm

pfadd macro dst, src
	amd3dop 09eh, dst, src
endm

pfcmpeq macro dst, src
	amd3dop 0b0h, dst, src
endm

pfcmpge macro dst, src
	amd3dop 090h, dst, src
endm

pfcmpgt macro dst, src
	amd3dop 0a0h, dst, src
endm

pfmax macro dst, src
	amd3dop 0a4h, dst, src
endm

pfmin macro dst, src
	amd3dop 094h, dst, src
endm

pfmul macro dst, src
	amd3dop 0b4h, dst, src
endm

pfrcp macro dst, src
	amd3dop 096h, dst, src
endm

pfrcpit1 macro dst, src
	amd3dop 0a6h, dst, src
endm

pfrcpit2 macro dst, src
	amd3dop 0b6h, dst, src
endm

pfrsqrt macro dst, src
	amd3dop 097h, dst, src
endm

pfrsqit1 macro dst, src
	amd3dop 0a7h, dst, src
endm

pfsub macro dst, src
	amd3dop 09ah, dst, src
endm

pfsubr macro dst, src
	amd3dop 0aah, dst, src
endm

pi2fd macro dst, src
	amd3dop 0dh, dst, src
endm

pfmulhrw macro dst, src
	amd3dop 0b7h, dst, src
endm

pf2iw macro dst, src
	amd3dop 01ch, dst, src
endm

pfnacc macro dst, src
	amd3dop 08ah, dst, src
endm

pfpnacc macro dst, src
	amd3dop 08eh, dst, src
endm

pi2fw macro dst, src
	amd3dop 0ch, dst, src
endm

pswapd macro dst, src
	amd3dop 0bbh, dst, src
endm

femms macro
	db 0Fh, 0Eh
endm

prefetch macro dst
	local u, v
	byte 0Fh
	u: inc dst
	v: org u
	org u
	byte 0Dh
	org v
endm

prefetchw macro dst
	local u, v
	byte  0Fh
	u: inc dst
	v: org u
	org u
	byte  0Dh
	org v
endm

;=====================AMD 3DNow! specific instructions ends===================
;======================Intel SSE and MMX extensions begins====================

	;movss xmm1,xmm2/m32 0f3h,0fh,010h,/r
	;movss xmm2/m32,xmm1 0f3h,0fh,011h,/r
movss macro dst, src
	local u, v, x, y
	DEFMMX0
	IFIDN dst, XMMREG
		DEFMMX
		byte 0f3h
	u: xadd src, dst
	v: org u
		word 100fh
		org v
	else
		DEFMMX
		byte 0f3h
	x: xadd dst, src
	y: org x
		word 110fh
		org y
	endif
	UNDEFMMX
endm

	;movaps xmm1,xmm2/m128 0fh,028h,/r
	;movaps xmm2/m128,xmm1 0fh,029h,/r
movaps macro dst, src
	local u, v, x, y
	DEFMMX0
	IFIDN dst, XMMREG
		DEFMMX
	u: xadd src, dst
	v: org u
		word 280fh
		org v
	else
		DEFMMX
	x: xadd dst, src
	y: org x
		word 290fh
		org y
	endif
	UNDEFMMX
endm

	;movups xmm1,xmm2/m128 0fh,010h,/r
	;movups xmm2/m128,xmm1 0fh,011h,/r
movups macro dst, src
	local u, v, x, y
	DEFMMX0
	IFIDN dst, XMMREG
		DEFMMX
	u: xadd src, dst
	v: org u
		word 100fh
		org v
	else
		DEFMMX
	x: xadd dst, src
	y: org x
		word 110fh
		org y
	endif
	UNDEFMMX
endm

	;movhps xmm,m64 0fh,016h,/r
	;movhps m64,xmm 0fh,017h,/r
movhps macro dst, src
	local u, v, x, y
	DEFMMX0
	IFIDN dst, XMMREG
		UNDEFMMX
		DEFX2
	u: paddb dst, src
	v: org u
		word 160fh
		org v
	else
		UNDEFMMX
		DEFX2
	x: paddb src, dst
	y: org x
		word 170fh
		org y
	endif
	UNDEFX2
endm

	;movlps xmm,m64 0fh,012h,/r
	;movlps m64,xmm 0fh,013h,/r
movlps macro dst, src
	local u, v, x, y
	DEFMMX0
	IFIDN dst, XMMREG
		UNDEFMMX
		DEFX2
	u: paddb dst, src
	v: org u
		word 120fh
		org v
	else
		UNDEFMMX
		DEFX2
	x: paddb src, dst
	y: org x
		word 130fh
		org y
	endif
	UNDEFX2
endm

addps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0580fh
	org y
	UNDEFMMX
endm

addss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0580fh
	org y
	UNDEFMMX
endm

andnps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0550fh
	org y
	UNDEFMMX
endm

andps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0540fh
	org y
	UNDEFMMX
endm

cmpeqps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 0
	UNDEFMMX
endm

cmpltps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 1
	UNDEFMMX
endm

cmpleps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 2
	UNDEFMMX
endm

cmpunordps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 3
	UNDEFMMX
endm

cmpneqps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 4
	UNDEFMMX
endm

cmpnltps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 5
	UNDEFMMX
endm

cmpgeps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 5
	UNDEFMMX
endm

cmpnleps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 6
	UNDEFMMX
endm

cmpgtps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 6
	UNDEFMMX
endm

cmpordps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 7
	UNDEFMMX
endm

cmpps macro dst, src, imm
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte imm
	UNDEFMMX
endm

cmpeqss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 0
	UNDEFMMX
endm

cmpltss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 1
	UNDEFMMX
endm

cmpless macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 2
	UNDEFMMX
endm

cmpunordss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 3
	UNDEFMMX
endm

cmpneqss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 4
	UNDEFMMX
endm

cmpnltss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 5
	UNDEFMMX
endm

cmpgess macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 5
	UNDEFMMX
endm

cmpnless macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 6
	UNDEFMMX
endm

cmpgtss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 6
	UNDEFMMX
endm

cmpordss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte 7
	UNDEFMMX
endm

cmpss macro dst, src, imm
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0c20fh
	org y
	byte imm
	UNDEFMMX
endm

comiss macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 02f0fh
	org y
	UNDEFMMX
endm

cvtpi2ps macro dst, src
	local x, y
	DEFX2
x: paddb dst, src
y: org x
	word 02a0fh
	org y
	UNDEFX2
endm

cvtps2pi macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 02d0fh
	org y
	UNDEFMMX
endm

cvtsi2ss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 02a0fh
	org y
	UNDEFMMX
endm

cvtss2si macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 02d0fh
	org y
	UNDEFMMX
endm

cvttps2pi macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 02c0fh
	org y
	UNDEFMMX
endm

cvttss2si macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 02c0fh
	org y
	UNDEFMMX
endm

divps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	byte 0fh
	byte 05eh
	org y
	UNDEFMMX
endm

divss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 05e0fh
	org y
	UNDEFMMX
endm

maskmovq macro dst, src
	local x, y
	DEFMMX
x: xadd dst, src
y: org x
	word 0f70fh
	org y
	UNDEFMMX
endm

maxps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 05f0fh
	org y
	UNDEFMMX
endm

maxss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 05f0fh
	org y
	UNDEFMMX
endm

minps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 05d0fh
	org y
	UNDEFMMX
endm

minss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 05d0fh
	org y
	UNDEFMMX
endm

movhlps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0120fh
	org y
	UNDEFMMX
endm

movlhps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0160fh
	org y
	UNDEFMMX
endm

movmskps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0500fh
	org y
	UNDEFMMX
endm

movntps macro dst, src
	local x, y
	DEFMMX
x: xadd dst, src
y: org x
	word 02b0fh
	org y
	UNDEFMMX
endm

movntq macro dst, src
	local x, y
	DEFMMX
x: xadd dst, src
y: org x
	word 0e70fh
	org y
	UNDEFMMX
endm

mulps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0590fh
	org y
	UNDEFMMX
endm

mulss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0590fh
	org y
	UNDEFMMX
endm

orps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0560fh
	org y
	UNDEFMMX
endm

pavgb macro dst, src
	local x, y
	DEFX2
x: paddb dst, src
y: org x
	word 0e00fh
	org y
	UNDEFX2
endm

pavgw macro dst, src
	local x, y
	DEFX2
x: paddb dst, src
y: org x
	word 0e30fh
	org y
	UNDEFX2
endm

pextrw macro dst, src, imm
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c50fh
	org y
	byte imm
	UNDEFMMX
endm

pinsrw macro dst, src, imm
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c40fh
	org y
	byte imm
	UNDEFMMX
endm

pmaxsw macro dst, src
	local x, y
	DEFX2
x: paddb dst, src
y: org x
	word 0ee0fh
	org y
	UNDEFX2
endm

pmaxub macro dst, src
	local x, y
	DEFX2
x: paddb dst, src
y: org x
	word 0de0fh
	org y
	UNDEFX2
endm

pminsw macro dst, src
	local x, y
	DEFX2
x: paddb dst, src
y: org x
	word 0ea0fh
	org y
	UNDEFX2
endm

pminub macro dst, src
	local x, y
	DEFX2
x: paddb dst, src
y: org x
	word 0da0fh
	org y
	UNDEFX2
endm

pmovmskb macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0d70fh
	org y
	UNDEFMMX
endm

pmulhuw macro dst, src
	local x, y
	DEFX2
x: paddb dst, src
y: org x
	word 0e40fh
	org y
	UNDEFX2
endm

psadbw macro dst, src
	local x, y
	DEFX2
x: paddb dst, src
y: org x
	word 0f60fh
	org y
	UNDEFX2
endm

pshufw macro dst, src, imm
	local x, y
	DEFX2
x: paddb dst, src
y: org x
	word 0700fh
	org y
	byte imm
	UNDEFX2
endm

rcpps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0530fh
	org y
	UNDEFMMX
endm

rcpss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0530fh
	org y
	UNDEFMMX
endm

rsqrtps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0520fh
	org y
	UNDEFMMX
endm

rsqrtss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0520fh
	org y
	UNDEFMMX
endm

shufps macro dst, src, imm
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0c60fh
	org y
	byte imm
	UNDEFMMX
endm

sqrtps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0510fh
	org y
	UNDEFMMX
endm

sqrtss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 0510fh
	org y
	UNDEFMMX
endm

subps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 05c0fh
	org y
	UNDEFMMX
endm

subss macro dst, src
	local x, y
	DEFMMX
	byte 0f3h
x: xadd src, dst
y: org x
	word 05c0fh
	org y
	UNDEFMMX
endm

ucomiss macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 02e0fh
	org y
	UNDEFMMX
endm

unpckhps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0150fh
	org y
	UNDEFMMX
endm

unpcklps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0140fh
	org y
	UNDEFMMX
endm

xorps macro dst, src
	local x, y
	DEFMMX
x: xadd src, dst
y: org x
	word 0570fh
	org y
	UNDEFMMX
endm

fxrstor macro dst
	local x, y   ;fxrstor 0fh,0aeh,/1     01?????? m512?
x: sidt dst     ;sidt m: 0f 01 /1
y: org x
	word 0ae0fh
	org y
endm

fxsave macro dst
	local x, y   ;fxsave 0fh,0aeh,/0      00?????? m512?
x: sgdt dst     ;sgdt m: 0f 01 /0
y: org x
	word 0ae0fh
	org y
endm

ldmxcsr macro dst
	local x, y   ;ldmxcsr 0fh,0aeh,/2     10?????? m32?
x: lgdt dst     ;lgdt m: 0f 01 /2
y: org x
	word 0ae0fh
	org y
endm

stmxcsr macro dst
	local x, y
x: lidt dst     ;stmxcsr 0fh,0aeh,/3     11?????? m32?
y: org x        ;lidt m: 0f 01 /3
	word 0ae0fh
	org y
endm

prefetcht0 macro dst
	local x, y
x: sidt dst     ;prefetcht0 0fh,018h,/1  01hhh??? m8?
y: org x        ;sidt m: 0f 01 /1
	word 0180fh
	org y
endm

prefetcht1 macro dst
	local x, y
x: lgdt dst     ;prefetcht1 0fh,018h,/2  10hhh??? m8?
y: org x        ;lgdt m: 0f 01 /2
	word 0180fh
	org y
endm

prefetcht2 macro dst
	local x, y   ;prefetcht2 0fh,018h,/3  11hhh??? m8?
x: lidt dst     ;lidt m: 0f 01 /3
y: org x
	word 0180fh
	org y
endm

prefetchnta macro dst
	local x, y   ;prefetchnta 0fh,018h,/0 00hhh??? m8?
x: sgdt dst     ;sgdt m: 0f 01 /0
y: org x
	word 0180fh
	org y
endm

sfence macro
	db 0fh,0aeh,0f8h
endm

sysenter macro
	dw 340fh
endm

sysexit macro
	dw 350fh
endm

ud2 macro
	dw 0b0fh
endm

;======================Intel SSE and MMX extensions ends======================
