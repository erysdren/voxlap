;G6.ASM by Ken Silverman (http://advsys.net/ken)
.586P
.8087
.MMX
.Model FLAT, C   ;Need this - Prevents segment overrides
;include mmx.inc       ;Include this if using < WATCOM 11.0 WASM

;Warning: IN THIS FILE, ALL SEGMENTS ARE REMOVED.  THIS MEANS THAT DS:[]
;MUST BE ADDED FOR ALL SELF-MODIFIES FOR MASM TO WORK.
;
;WASM PROBLEMS:
;   1. Requires all scaled registers (*1,*2,*4,*8) to be last thing on line
;   2. Using 'DATA' is nice for self-mod. code, but WDIS only works with 'CODE'
;
;MASM PROBLEMS:
;   1. Requires DS: to be written out for self-modifying code to work
;   2. Doesn't encode short jumps automatically like WASM
;   3. Stupidly adds wait prefix to ffree's

EXTRN _groucol1 : near
EXTRN _grouhei1 : near
EXTRN _groucol2 : near
EXTRN _grouhei2 : near
EXTRN _groucol3 : near
EXTRN _grouhei3 : near
EXTRN _blackcol : dword
EXTRN _ipos : near

CODE SEGMENT PUBLIC USE32 'CODE'
ASSUME cs:CODE

PUBLIC setupgrou6d_
setupgrou6d_:
	mov dword ptr [machrad1a-4], ecx  ;_radarindex+cnt
	mov dword ptr [machrad1b-4], ecx  ;_radarindex+cnt
	mov dword ptr [machrad2-4], ecx
	mov dword ptr [machrad3-4], ecx
	mov dword ptr [machrad4-4], ecx

	add edx, 8192
	mov dword ptr [machcmp1-4], edx  ;(zoff<<16)+4096
	add edx, 8192
	mov dword ptr [machcmp2-4], edx  ;(zoff<<16)+4096+8192
	add edx, 16384
	mov dword ptr [machcmp3-4], edx  ;(zoff<<16)+4096+8192+16384

	movd mm6, ebx                    ;vi: VVVVVVVVVVvvvvvvvvvvvvvvvvvvvvvv

	mov dword ptr [machui1-4], eax   ;ui: UUUUUUU1111111111UUUuuuuuuuuuuuu

	;EAX Before:    XXXXXXx1111111111XXXxxxxxxxxxxxx
	;     *2        XXXXXX1111111111XXXxxxxxxxxxxxx0
	; shl1 rt. half XXXXXX111111111XXXxxxxxxxxxxxx00

	add eax, eax
	mov ebx, eax
	and ebx, 0fffe0000h
	and eax, 0000ffffh
	add eax, eax
	add eax, ebx
	or eax, 03fe0000h

	mov dword ptr [machui2-4], eax

	;EAX Before:    XXXXXX111111111XXXxxxxxxxxxxxx00
	;     *2        XXXXX111111111XXXxxxxxxxxxxxx000
	; shl1 rt. half XXXXX11111111XXXxxxxxxxxxxxx0000

	add eax, eax
	mov ebx, eax
	and ebx, 0ffe00000h
	and eax, 000fffffh
	add eax, eax
	add eax, ebx
	or eax, 07f80000h

	mov dword ptr [machui3-4], eax

	movd mm7, esi   ;ji
	movd mm2, edi   ;ki
	punpckldq mm2, mm7
	ret

PUBLIC grou6d_
grou6d_:
	;eax = temp----------------------------
	;ebx = temp----------------------------
	;ecx = zoff------------d---------------
	;edx = temp----------------------------
	;esi = cnt/writeplc--------------------
	;edi = UUUUUUU0000000000UUUuuuuuuuuuuuu
	;ebp = temp----------------------------
	;esp = --------------------------------
	;mm0 = --------------------------------h---------------d---------------
	;mm1 = j-------------------------------k-------------------------------
	;mm2 = ji------------------------------ki------------------------------
	;mm3 = temp------------------------------------------------------------
	;mm4 = temp------------------------------------------------------------
	;mm5 = --------------------------------VVVVVVVVVVvvvvvvvvvvvvvvvvvvvvvv
	;mm6 = --------------------------------Vi------------------------------
	;mm7 = ----------------------------------------------------------------

	push ebp

	movd mm7, eax   ;j
	movd mm1, ebx   ;k
	punpckldq mm1, mm7
	movd mm5, edx   ;v
	movq mm3, mm1
	psrad mm3, 16
	packssdw mm3, mm3

	cmp byte ptr _ipos[7], 0
	jl endit1

	movd ebp, mm5
	paddd mm5, mm6
	shr ebp, 22
	mov eax, edi
	shr eax, 12
begit1:
	lea edx, [eax+ebp*8]
	movzx ebx, byte ptr _grouhei1[eax+ebp*8]
	shl ebx, 5+16
	add edi, 88888888h   ;ui: UUUUUUU1111111111UUUuuuuuuuuuuuu
machui1:
	add ebx, ecx
	and edi, 0fe007fffh
	movd mm0, ebx
	movq mm4, mm3
	movd ebp, mm5
	pmaddwd mm4, mm0
	shr ebp, 22
	mov eax, edi
	shr eax, 12
	add ecx, 32
	movd ebx, mm4
	paddd mm5, mm6
	test ebx, ebx
	jns short skiploop1
	mov dl, byte ptr _groucol1[edx]
begloop1:
	paddd mm1, mm2
	movq mm4, mm1
	mov byte ptr [esi+88888888h], dl  ;esi+88888888h = _radarindex
machrad1a:
	psrad mm4, 16
	movq mm7, mm1
	packssdw mm4, mm4
	paddd mm7, mm2
	movq mm3, mm4
	pmaddwd mm4, mm0
	psrad mm7, 16
	inc esi
	packssdw mm7, mm7
	jz enditall
	movd ebx, mm4
	test ebx, ebx
	jns short skiploop1
	movq mm3, mm7
	pmaddwd mm7, mm0
	mov byte ptr [esi+88888888h], dl  ;esi+88888888h = _radarindex
machrad1b:
	inc esi
	jz enditall
	movd ebx, mm4
	paddd mm1, mm2
	test ebx, ebx
	js short begloop1

skiploop1:
	cmp ecx, 88888888h   ;(zoff<<16)+dend
machcmp1:
	jb begit1
endit1:

	paddd mm6, mm6
	;edi before: XXXXXXX0 00000000 0XXXxxxx xxxxxxxx
	;     after: XXXXXX00 0000000X XXxxxxxx xxxxxxxx
	mov eax, edi
	mov ebp, edi
	shr eax, 9
	add ebp, ebp
	and edi, 0fc000000h
	and eax, 00010000h
	and ebp, 0000ffffh
	add eax, ebp
	add edi, eax

	cmp dword ptr _ipos[4], 0c3800000h  ;if (ipos.y >= -256)
	ja endit2

	movd ebp, mm5
	paddd mm5, mm6
	shr ebp, 23
	mov eax, edi
	shr eax, 14
begit2:
	lea edx, [eax+ebp*8]
	movzx ebx, byte ptr _grouhei2[eax+ebp*8]
	shl ebx, 5+16
	add edi, 88888888h   ;ui: UUUUUU111111111UUUuuuuuuuuuuuuuu
machui2:
	add ebx, ecx
	and edi, 0fc01ffffh
	movd mm0, ebx
	movq mm4, mm3
	movd ebp, mm5
	pmaddwd mm4, mm0
	shr ebp, 23
	mov eax, edi
	shr eax, 14
	add ecx, 64
	movd ebx, mm4
	paddd mm5, mm6
	test ebx, ebx
	jns short skiploop2
	mov dl, byte ptr _groucol2[edx]
begloop2:
	paddd mm1, mm2
	movq mm4, mm1
	mov byte ptr [esi+88888888h], dl  ;esi+88888888h = _radarindex
machrad2:
	psrad mm4, 16
	inc esi
	packssdw mm4, mm4
	jz enditall
	movq mm3, mm4
	pmaddwd mm4, mm0
	movd ebx, mm4
	test ebx, ebx
	js short begloop2
skiploop2:
	cmp ecx, 88888888h   ;(zoff<<16)+dend
machcmp2:
	jb short begit2
endit2:

	paddd mm6, mm6
	;edi before: XXXXXX00 0000000X XXxxxxxx xxxxxxxx
	;     after: XXXXX000 00000XXX xxxxxxxx xxxxxx00
	mov eax, edi
	mov ebp, edi
	shr eax, 8
	add ebp, ebp
	and edi, 0f8000000h
	and eax, 00040000h
	and ebp, 0003ffffh
	add eax, ebp
	add edi, eax

	movd ebp, mm5
	paddd mm5, mm6
	shr ebp, 24
	mov eax, edi
	shr eax, 16
begit3:
	lea edx, [eax+ebp*8]
	movzx ebx, byte ptr _grouhei3[eax+ebp*8]
	shl ebx, 5+16
	add edi, 88888888h   ;ui: UUUUU11111111UUUuuuuuuuuuuuuuuuu
machui3:
	add ebx, ecx
	and edi, 0f807ffffh
	movd mm0, ebx
	movq mm4, mm3
	movd ebp, mm5
	pmaddwd mm4, mm0
	shr ebp, 24
	mov eax, edi
	shr eax, 16
	sub ecx, -128
	movd ebx, mm4
	paddd mm5, mm6
	test ebx, ebx
	jns short skiploop3
	mov dl, byte ptr _groucol3[edx]
begloop3:
	paddd mm1, mm2
	movq mm4, mm1
	mov byte ptr [esi+88888888h], dl  ;esi+88888888h = _radarindex
machrad3:
	psrad mm4, 16
	inc esi
	packssdw mm4, mm4
	jz short enditall
	movq mm3, mm4
	pmaddwd mm4, mm0
	movd ebx, mm4
	test ebx, ebx
	js short begloop3
skiploop3:
	cmp ecx, 88888888h   ;(zoff<<16)+dend
machcmp3:
	jb short begit3
endit3:

	mov dl, byte ptr _blackcol[0]
begit4:
	mov byte ptr [esi+88888888h], dl
machrad4:
	inc esi
	jnz short begit4

enditall:

	emms
	pop ebp
	ret

CODE ENDS
END
