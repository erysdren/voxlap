;STP.ASM by Ken Silverman (http://advsys.net/ken)
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
;   3. shr used in a macro seems to always be arithmetic
;
;MASM PROBLEMS:
;   1. Requires DS: to be written out for self-modifying code to work
;   2. Doesn't encode short jumps automatically like WASM
;   3. Stupidly adds wait prefix to ffree's

LOGROUSIZE EQU 10

EXTRN _grouhei1 : near
EXTRN _groucol1 : near
EXTRN _grouptr1 : near
EXTRN _grouhei2 : near
EXTRN _groucol2 : near
EXTRN _grouptr2 : near
EXTRN _grouhei3 : near
EXTRN _groucol3 : near
EXTRN _grouptr3 : near
EXTRN _groucol : near
EXTRN _blackcol : dword
EXTRN _ipos : near

CODE SEGMENT PUBLIC USE32 'CODE'
ASSUME cs:CODE

ALIGN 8
uvmask1: dd (not (((1 shl (LOGROUSIZE  ))-1) shl (35-(LOGROUSIZE  )*2))), -1
uvmask2: dd (not (((1 shl (LOGROUSIZE-1))-1) shl (35-(LOGROUSIZE-1)*2))), -1
uvmask3: dd (not (((1 shl (LOGROUSIZE-2))-1) shl (35-(LOGROUSIZE-2)*2))), -1
mip01mask1: dd (not ((1 shl (35-(LOGROUSIZE-1)))-1)), -1
mip01mask2: dd      ((1 shl (35-(LOGROUSIZE-1)))-1) ,  0
mip12mask1: dd (not ((1 shl (35-(LOGROUSIZE-2)))-1)), -1
mip12mask2: dd      ((1 shl (35-(LOGROUSIZE-2)))-1) ,  0


	;s,si,u,v,ui,vi
PUBLIC setupgrou6d_
setupgrou6d_:
		;mm2: si------------------------------s-------------------------------
	movd mm2, eax            ;s
	movd mm5, ebx            ;si
	punpckldq mm2, mm5

		;mm3: VVVVVVVVVVvvvvvvvvvvvvvvvvvvvvvvUUUUUUU0000000000UUUuuuuuuuuuuuu
	mov eax, ecx
	shr eax, LOGROUSIZE
	and ecx, (-1 shl (35-LOGROUSIZE))
	and eax, (1 shl (35-LOGROUSIZE*2))-1
	or ecx, eax
	movd mm3, ecx            ;u
	movd mm6, edx            ;v
	punpckldq mm3, mm6

		;mm4: VIVVVVVVVVvvvvvvvvvvvvvvvvvvvvvvUIUUUUU1111111111UUUuuuuuuuuuuuu
	mov eax, esi
	shr eax, LOGROUSIZE
	and esi, (-1 shl (35-LOGROUSIZE))
	and eax, (1 shl (35-LOGROUSIZE*2))-1
	or esi, eax
	or esi, (((1 shl LOGROUSIZE)-1) shl (35-LOGROUSIZE*2))
	movd mm4, esi            ;ui
	movd mm7, edi            ;vi
	punpckldq mm4, mm7

	ret

ALIGN 4
edxbak: dd 0
PUBLIC grou6d_
grou6d_:
	;eax: ptr-------------(col)---h-------  ;j passed
	;ebx: 000000000000000000000000oh------  ;k passed
	;ecx: temp1---------------------------  ;ji passed
	;edx: temp2---------------------------  ;&radarptr[cnt] passed
	;esi: d-------------------------------  ;ki passed
	;edi: p-------------------------------  ;-cnt passed
	;ebp: --------------------------------
	;esp: --------------------------------
	;mm0: ji------------------------------j-------------------------------
	;mm1: ki------------------------------k-------------------------------
	;mm2: si------------------------------s-------------------------------
	;mm3: u-------------------------------v-------------------------------
	;mm4: ui------------------------------vi------------------------------
	;mm5: 00000000000000000000000000000000ji------------------------------
	;mm6: 00000000000000000000000000000000ki------------------------------
	;mm7: temp------------------------------------------------------------

	push ebp
	mov ebp, edx

	movd mm0, eax
	movd mm5, ecx
	punpckldq mm0, mm5
	movd mm1, ebx
	movd mm6, esi
	punpckldq mm1, mm6

	mov esi, 256

	movd ecx, mm3
	movq mm7, mm3
	shr ecx, 32-LOGROUSIZE*2
	psrlq mm7, 64-LOGROUSIZE
	movd edx, mm7
	xor ebx, ebx
	mov bl, _grouhei1[ecx+edx*8]

	cmp byte ptr _ipos[7], 0
	jl startmip1

	jmp m0intoloop

m0startloop1:
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m0skipdrawloop1
	mov ah, _groucol1[edx]
	psrlq mm7, 32
m0begdrawloop1:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m0begdrawloop1
m0skipdrawloop1:
	dec esi
	jz startmip1
m0intoloop:
	movd ecx, mm3
	movq mm7, mm3
	shr ecx, 32-LOGROUSIZE*2
	psrlq mm7, 64-LOGROUSIZE
	movd edx, mm7
	paddd mm3, mm4
	pand mm3, uvmask1
	psubd mm2, mm1
	mov al, _grouhei1[ecx+edx*8]
	lea edx, [ecx+edx*8]
	cmp bl, al
	je m0startloop1
	jb m0skipgreater

m0startloop2:
	inc al
	cmp bl, al
	jbe m0endslabloop2b
	mov dword ptr [edxbak], edx
	mov edx, dword ptr _grouptr1[edx*4]
	and eax, 255
	sub edx, eax
m0begslabloop2a:
	dec bl
	psubd mm2, mm0
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m0skipit2a
	mov ah, _groucol[edx+ebx]
	psrlq mm7, 32
m0begdrawloop2a:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m0begdrawloop2a
m0skipit2a:
	cmp bl, al
	jnz m0begslabloop2a
	mov edx, dword ptr [edxbak]
m0endslabloop2b:
	dec bl
	psubd mm2, mm0
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m0skipit2b
	mov ah, _groucol1[edx]
	psrlq mm7, 32
m0begdrawloop2b:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m0begdrawloop2b
m0skipit2b:
	dec esi
	jz startmip1
	movd ecx, mm3
	movq mm7, mm3
	shr ecx, 32-LOGROUSIZE*2
	psrlq mm7, 64-LOGROUSIZE
	movd edx, mm7
	paddd mm3, mm4
	pand mm3, uvmask1
	psubd mm2, mm1
	mov al, _grouhei1[ecx+edx*8]
	lea edx, [ecx+edx*8]
	cmp bl, al
	ja m0startloop2
	je m0startloop1

m0skipgreater:
	paddd mm2, mm0
	inc bl
	cmp bl, al
	jz m0startloop1
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m0begslabloop3
	mov ah, _groucol1[edx]
	psrlq mm7, 32
m0begdrawloop3:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m0begdrawloop3
m0begslabloop3:
	paddd mm2, mm0
	inc bl
	cmp bl, al
	jnz m0begslabloop3
	dec esi
	jz startmip1
	movd ecx, mm3
	movq mm7, mm3
	shr ecx, 32-LOGROUSIZE*2
	psrlq mm7, 64-LOGROUSIZE
	movd edx, mm7
	paddd mm3, mm4
	pand mm3, uvmask1
	psubd mm2, mm1
	mov al, _grouhei1[ecx+edx*8]
	lea edx, [ecx+edx*8]
	cmp bl, al
	jb m0skipgreater
	je m0startloop1
	jmp m0startloop2




startmip1:
	shr bl, 1               ;Halve lastheight
	jnc m1nocarry
	psubd mm0, mm5          ;Possibly fix j by half step
m1nocarry:
	paddd mm0, mm0          ;Duplicate ji,j
	paddd mm1, mm1          ;Duplicate ki,k
	paddd mm5, mm5          ;Duplicate 0,ji
	paddd mm6, mm6          ;Duplicate 0,ki
	shr esi, 1              ;Update UV countdown
	sub esi, -128
;VVVVVVVVVVvvvvvvvvvvvvvvvvvvvvvvUUUUUUU0000000000UUUuuuuuuuuuuuu mm3 (old)
;                                111111
;                                000000000UUUUUUU0000000000UUUuuu mm3 (old>>9)
;                                               1
;                                UUUUUU0000000000UUUuuuuuuuuuuuu0 mm3 (old<<1)
;                                                1111111111111111
;VVVVVVVVVvvvvvvvvvvvvvvvvvvvvvvvUUUUUU000000000UUUuuuuuuuuuuuuu0 mm3 (new)
	movd eax, mm3           ;fix u
	movq mm7, mm3
	mov ecx, eax
	mov edx, eax
	shr ecx, LOGROUSIZE-1
	add edx, edx
	and eax, (-1 shl (35-(LOGROUSIZE-1)))      ;0
	and ecx, (1 shl (34-(LOGROUSIZE-1)*2))     ;>>9
	and edx, (1 shl (34-(LOGROUSIZE-1)*2))-1   ;<<1
	or eax, ecx
	or eax, edx
	movd mm3, eax
	psrlq mm7, 32
	punpckldq mm3, mm7
;VVVVVVVVVVvvvvvvvvvvvvvvvvvvvvvvUUUUUUU1111111111UUUuuuuuuuuuuuu mm4 (old)
;VVVVVVVVVvvvvvvvvvvvvvvvvvvvvvv0UUUUUU111111111UUUuuuuuuuuuuuu00 mm4 (new)
	movq mm7, mm4           ;Duplicate ui,vi and fix ui
	paddd mm4, mm4
	pslld mm7, 2
	pand mm4, mip01mask1
	pand mm7, mip01mask2
	por mm4, mm7

	cmp dword ptr _ipos[4], 0c3800000h  ;if (ipos.y >= -256)
	ja startmip2
	jmp m1intoloop

m1startloop1:
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m1skipdrawloop1
	mov ah, _groucol2[edx]
	psrlq mm7, 32
m1begdrawloop1:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m1begdrawloop1
m1skipdrawloop1:
	dec esi
	jz startmip2
m1intoloop:
	movd ecx, mm3
	movq mm7, mm3
	shr ecx, 32-(LOGROUSIZE-1)*2
	psrlq mm7, 64-(LOGROUSIZE-1)
	movd edx, mm7
	paddd mm3, mm4
	pand mm3, uvmask2
	psubd mm2, mm1
	mov al, _grouhei2[ecx+edx*8]
	lea edx, [ecx+edx*8]
	cmp bl, al
	je m1startloop1
	jb m1skipgreater

m1startloop2:
	inc al
	cmp bl, al
	jbe m1endslabloop2b
	mov dword ptr [edxbak], edx
	mov edx, dword ptr _grouptr2[edx*4]
	and eax, 255
	sub edx, eax
m1begslabloop2a:
	dec bl
	psubd mm2, mm0
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m1skipit2a
	mov ah, _groucol[edx+ebx]
	psrlq mm7, 32
m1begdrawloop2a:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m1begdrawloop2a
m1skipit2a:
	cmp bl, al
	jnz m1begslabloop2a
	mov edx, dword ptr [edxbak]
m1endslabloop2b:
	dec bl
	psubd mm2, mm0
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m1skipit2b
	mov ah, _groucol2[edx]
	psrlq mm7, 32
m1begdrawloop2b:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m1begdrawloop2b
m1skipit2b:
	dec esi
	jz startmip2
	movd ecx, mm3
	movq mm7, mm3
	shr ecx, 32-(LOGROUSIZE-1)*2
	psrlq mm7, 64-(LOGROUSIZE-1)
	movd edx, mm7
	paddd mm3, mm4
	pand mm3, uvmask2
	psubd mm2, mm1
	mov al, _grouhei2[ecx+edx*8]
	lea edx, [ecx+edx*8]
	cmp bl, al
	ja m1startloop2
	je m1startloop1

m1skipgreater:
	paddd mm2, mm0
	inc bl
	cmp bl, al
	jz m1startloop1
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m1begslabloop3
	mov ah, _groucol2[edx]
	psrlq mm7, 32
m1begdrawloop3:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m1begdrawloop3
m1begslabloop3:
	paddd mm2, mm0
	inc bl
	cmp bl, al
	jnz m1begslabloop3
	dec esi
	jz startmip2
	movd ecx, mm3
	movq mm7, mm3
	shr ecx, 32-(LOGROUSIZE-1)*2
	psrlq mm7, 64-(LOGROUSIZE-1)
	movd edx, mm7
	paddd mm3, mm4
	pand mm3, uvmask2
	psubd mm2, mm1
	mov al, _grouhei2[ecx+edx*8]
	lea edx, [ecx+edx*8]
	cmp bl, al
	jb m1skipgreater
	je m1startloop1
	jmp m1startloop2




startmip2:
	shr bl, 1               ;Halve lastheight
	jnc m2nocarry
	psubd mm0, mm5          ;Possibly fix j by half step
m2nocarry:
	paddd mm0, mm0          ;Duplicate ji,j
	paddd mm1, mm1          ;Duplicate ki,k
	paddd mm5, mm5          ;Duplicate 0,ji
	paddd mm6, mm6          ;Duplicate 0,ki
	shr esi, 1              ;Update UV countdown
	sub esi, -128
;VVVVVVVVVvvvvvvvvvvvvvvvvvvvvvvvUUUUUU000000000UUUuuuuuuuuuuuuu0 mm3 (old)
;                                11111
;                                00000000UUUUUU000000000UUUuuuuuu mm3 (old>>8)
;                                             1
;                                UUUUU000000000UUUuuuuuuuuuuuuu00 mm3 (old<<1)
;                                              111111111111111111
;VVVVVVVVvvvvvvvvvvvvvvvvvvvvvvvvUUUUU00000000UUUuuuuuuuuuuuuuu00 mm3 (new)
	movd eax, mm3           ;fix u
	movq mm7, mm3
	mov ecx, eax
	mov edx, eax
	shr ecx, LOGROUSIZE-2
	add edx, edx
	and eax, (-1 shl (35-(LOGROUSIZE-2)))      ;0
	and ecx, (1 shl (34-(LOGROUSIZE-2)*2))     ;>>8
	and edx, (1 shl (34-(LOGROUSIZE-2)*2))-1   ;<<1
	or eax, ecx
	or eax, edx
	movd mm3, eax
	psrlq mm7, 32
	punpckldq mm3, mm7
;VVVVVVVVVvvvvvvvvvvvvvvvvvvvvvv0UUUUUU111111111UUUuuuuuuuuuuuu00 mm4 (old)
;VVVVVVVVvvvvvvvvvvvvvvvvvvvvvv00UUUUU11111111UUUuuuuuuuuuuuu0000 mm4 (new)
	movq mm7, mm4           ;Duplicate ui,vi and fix ui
	paddd mm4, mm4
	pslld mm7, 2
	pand mm4, mip12mask1
	pand mm7, mip12mask2
	por mm4, mm7
	jmp m2intoloop

m2startloop1:
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m2skipdrawloop1
	mov ah, _groucol3[edx]
	psrlq mm7, 32
m2begdrawloop1:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m2begdrawloop1
m2skipdrawloop1:
	dec esi
	jz startmip3
m2intoloop:
	movd ecx, mm3
	movq mm7, mm3
	shr ecx, 32-(LOGROUSIZE-2)*2
	psrlq mm7, 64-(LOGROUSIZE-2)
	movd edx, mm7
	paddd mm3, mm4
	pand mm3, uvmask3
	psubd mm2, mm1
	mov al, _grouhei3[ecx+edx*8]
	lea edx, [ecx+edx*8]
	cmp bl, al
	je m2startloop1
	jb m2skipgreater

m2startloop2:
	inc al
	cmp bl, al
	jbe m2endslabloop2b
	mov dword ptr [edxbak], edx
	mov edx, dword ptr _grouptr3[edx*4]
	and eax, 255
	sub edx, eax
m2begslabloop2a:
	dec bl
	psubd mm2, mm0
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m2skipit2a
	mov ah, _groucol[edx+ebx]
	psrlq mm7, 32
m2begdrawloop2a:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m2begdrawloop2a
m2skipit2a:
	cmp bl, al
	jnz m2begslabloop2a
	mov edx, dword ptr [edxbak]
m2endslabloop2b:
	dec bl
	psubd mm2, mm0
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m2skipit2b
	mov ah, _groucol3[edx]
	psrlq mm7, 32
m2begdrawloop2b:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m2begdrawloop2b
m2skipit2b:
	dec esi
	jz startmip3
	movd ecx, mm3
	movq mm7, mm3
	shr ecx, 32-(LOGROUSIZE-2)*2
	psrlq mm7, 64-(LOGROUSIZE-2)
	movd edx, mm7
	paddd mm3, mm4
	pand mm3, uvmask3
	psubd mm2, mm1
	mov al, _grouhei3[ecx+edx*8]
	lea edx, [ecx+edx*8]
	cmp bl, al
	ja m2startloop2
	je m2startloop1

m2skipgreater:
	paddd mm2, mm0
	inc bl
	cmp bl, al
	jz m2startloop1
	movd ecx, mm2
	movq mm7, mm2
	test ecx, ecx
	jns m2begslabloop3
	mov ah, _groucol3[edx]
	psrlq mm7, 32
m2begdrawloop3:
	mov [edi+ebp], ah
	paddd mm2, mm7
	inc edi
	jz enditall
	paddd mm0, mm5
	paddd mm1, mm6
	movd ecx, mm2
	test ecx, ecx
	js m2begdrawloop3
m2begslabloop3:
	paddd mm2, mm0
	inc bl
	cmp bl, al
	jnz m2begslabloop3
	dec esi
	jz startmip3
	movd ecx, mm3
	movq mm7, mm3
	shr ecx, 32-(LOGROUSIZE-2)*2
	psrlq mm7, 64-(LOGROUSIZE-2)
	movd edx, mm7
	paddd mm3, mm4
	pand mm3, uvmask3
	psubd mm2, mm1
	mov al, _grouhei3[ecx+edx*8]
	lea edx, [ecx+edx*8]
	cmp bl, al
	jb m2skipgreater
	je m2startloop1
	jmp m2startloop2




startmip3:
	mov ah, byte ptr _blackcol[0]
begfillrest:
	mov byte ptr [edi+ebp], ah
	inc edi
	jnz begfillrest




enditall:
	emms
	pop ebp
	ret

CODE ENDS
END


;VVVVVVVVVVvvvvvvvvvvvvvvvvvvvvvvUUUUUUU0000000000UUUuuuuuuuuuuuu
;000000000000VVVVVVVVVVvvvvvvvvvv000000000000UUUUUUU0000000000UUU
;000000000000000000000000000000000000000000000000000000VVVVVVVVVV
;
;   Method #1: (More MMX code)
;movq mm7, mm3            paddd mm3, mm4
;pand mm3, uvmask         psrld mm7, 32-LOGROUSIZE*2
;movd ecx, mm7            psrlq mm7, 32+LOGROUSIZE
;movd edx, mm7            psubd mm2, mm1
;mov al, _grouhei1[ecx+edx*8]
;
;   Method #2: (Better for Pentium MMX?)
;movd ecx, mm3            movq mm7, mm3
;shr ecx, 32-LOGROUSIZE*2 psrlq mm7, 64-LOGROUSIZE
;movd edx, mm7            paddd mm3, mm4
;pand mm3, uvmask         psubd mm2, mm1
;mov al, _grouhei1[ecx+edx*8]
