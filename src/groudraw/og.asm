;OG.ASM by Ken Silverman (http://advsys.net/ken)
.386P

EXTRN _groucol : near
EXTRN _grouhei : near

CODE SEGMENT PUBLIC USE32 'CODE'
ASSUME cs:CODE

	;Passes posz
PUBLIC setupovergrou_
setupovergrou_:
	mov dword ptr ds:[machposz1+2], eax
	mov dword ptr ds:[machposz2+3], eax
	mov dword ptr ds:[machdinc+1], ebx
	ret

;eax: d - increments by dinc outer loop
;ebx: XXXXXXXXXXxxxxxxxxxxxxxxBBBBBBBB
;ecx: YYYYYYY0000000000YYYyyyyyyyyyyyy
;edx: nz - kind of temp
;esi: i - kind of temp
;edi: m - screwy
;ebp: z - increments by 1 inner loop

	;Passes: zend, x, y, xinc, yinc, rad
PUBLIC overgrou_
overgrou_:
	push ebp

	mov dword ptr ds:[machzend+2], eax
	mov dword ptr ds:[machrad+2], edi

		;Strip xinc&yinc
	mov dword ptr ds:[machxinc+2], edx
	mov eax, esi
	shr eax, 10
	and esi, 0fe000000h
	add eax, esi
	or eax, 01ff8000h
	mov dword ptr ds:[machyinc+2], eax

		;Strip x&y
	mov eax, ecx
	shr eax, 10
	and ecx, 0fe000000h
	and eax, 00007fffh
	add ecx, eax

	xor bl, bl  ;Set a value that can't be a valid height
	mov ebp, 1
	xor eax, eax
	jmp short begit

ALIGN 16
begit:
	mov esi, ebx
	mov edx, ecx
	shr esi, 19
machxinc: add ebx, 88888888h     ;xinc
	shr edx, 12
	and esi, 0fffffff8h
machyinc: add ecx, 88888888h     ;yinc
	add esi, edx

;P5 optimized:
;   xor edx, edx
;   and ecx, 0fe007fffh
;machdinc: add eax, 88888888h            ;dinc
;   mov dh, byte ptr _grouhei[esi]
;   cmp dh, bl
;   je short skipmul
;   mov bl, dh
;machposz1: lea edi, [edx+88888888h] ;posz
;   imul edi, ebp
;skipmul:
;P5 optimized ENDS

;P6 optimized:
	movzx edx, byte ptr _grouhei[esi]
	and ecx, 0fe007fffh
	shl edx, 8
machdinc: add eax, 88888888h            ;dinc
machposz1: lea edi, [edx+88888888h] ;posz
	imul edi, ebp
;P6 optimized ENDS

	cmp edi, eax
	jge short begit
	mov al, byte ptr _groucol[esi]
drawloop:
machposz2: lea edi, [edi+edx+88888888h]    ;posz
machrad: mov [ebp+88888888h], al       ;rad
	inc ebp
machzend: cmp ebp, 88888888h     ;zend
	jge short endit
	cmp edi, eax
	jge short begit
	jmp short drawloop
endit:

	pop ebp
	ret

CODE ENDS
END
