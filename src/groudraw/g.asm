;G.ASM by Ken Silverman (http://advsys.net/ken)
.386P

EXTRN _groucol : near
EXTRN _grouhei : near
EXTRN _groucol2 : near
EXTRN _grouhei2 : near
EXTRN _groucol3 : near
EXTRN _grouhei3 : near

CODE SEGMENT BYTE PUBLIC USE32 'CODE'
ASSUME cs:CODE

	;de, dx, dy, dd, bpl, ptop
	PUBLIC setupgrouvline_
setupgrouvline_:
	mov dword ptr ds:[mach1a+2], eax
	mov dword ptr ds:[mach8a+2], eax
	mov dword ptr ds:[mach4a+2], edx
	mov dword ptr ds:[mach7a+2], edx
	add eax, eax
	add edx, edx
	mov dword ptr ds:[mach1b+2], eax
	mov dword ptr ds:[mach8b+2], eax
	mov dword ptr ds:[mach4b+2], edx
	mov dword ptr ds:[mach7b+2], edx
	add eax, eax
	add edx, edx
	mov dword ptr ds:[mach1c+2], eax
	mov dword ptr ds:[mach8c+2], eax
	mov dword ptr ds:[mach4c+2], edx
	mov dword ptr ds:[mach7c+2], edx

	mov dword ptr ds:[mach5a+2], edi
	mov dword ptr ds:[mach5b+2], edi
	mov dword ptr ds:[mach5c+2], edi
	mov dword ptr ds:[mach6a+2], esi
	mov dword ptr ds:[mach6b+2], esi
	mov dword ptr ds:[mach6c+2], esi

	mov dword ptr ds:[mach2a+1], ebx
	mov dword ptr ds:[mach3a+2], ecx

	;EBX Before:    XXXXXXx1111111111XXXxxxxxxxxxxxx
	;     *2        XXXXXX1111111111XXXxxxxxxxxxxxx0
	; shl1 rt. half XXXXXX111111111XXXxxxxxxxxxxxx00

	add ebx, ebx
	add ecx, ecx
	mov eax, ebx
	and eax, 0fffe0000h
	and ebx, 0000ffffh
	add ebx, ebx
	add ebx, eax
	or ebx, 03fe0000h

	mov dword ptr ds:[mach2b+1], ebx
	mov dword ptr ds:[mach3b+2], ecx

	;EBX Before:    XXXXXX111111111XXXxxxxxxxxxxxx00
	;     *2        XXXXX111111111XXXxxxxxxxxxxxx000
	; shl1 rt. half XXXXX11111111XXXxxxxxxxxxxxx0000

	add ebx, ebx
	add ecx, ecx
	mov eax, ebx
	and eax, 0ffe00000h
	and ebx, 000fffffh
	add ebx, ebx
	add ebx, eax
	or ebx, 07f80000h

	mov dword ptr ds:[mach2c+1], ebx
	mov dword ptr ds:[mach3c+2], ecx
	ret

	;x, y, z, dz, d, p-ptop
;eax: XXXXXXX0 00000000 0XXXxxxx xxxxxxxx
;ebx: YYYYYYYY YYyyyyyy yyyyyyyy --------
;ecx: ZZZZZZZZ ZZZZZZZZ zzzzzzzz zzzzzzzz
;edx: -------- -------- -------- --------
;esi: DDDDDDDD DDDDDDDD dddddddd dddddddd
;edi: vidplcxx xxxxxxxx xxxxxxxx xxxxxxxx
;ebp: -------- -------- -------- --------
;esp: dzxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx

espbak: dd 0
	PUBLIC grouvline_
grouvline_:
	push ebp

	cli
	mov dword ptr ds:[espbak], esp

	mov esp, edx
	and eax, 0fe007fffh
	jmp short begita

prebegita:
mach1a: cmp esi, 88888888h      ;de
	jge short endita
begita:
	mov ebp, ebx
	mov edx, eax
	shr ebp, 19
	add ecx, esp
	shr edx, 12
	and ebp, 0fffffff8h
	add ebp, edx
mach3a: add ebx, 88888888h      ;dy    YYYYYYYYYYyyyyyyyyyyyyyy--------
mach4a: add esi, 88888888h      ;dd
	xor edx, edx
	mov dl, byte ptr _grouhei[ebp]
mach2a: add eax, 88888888h      ;dx    XXXXXXX1111111111XXXxxxxxxxxxxxx
	shl edx, 16
	and eax, 0fe007fffh
	cmp edx, ecx
	jge short prebegita
	mov bl, byte ptr _groucol[ebp]
vidloop1a:
mach5a: mov [edi+88888888h], bl ;ptop
	sub ecx, esi
mach6a: sub edi, 88888888h      ;bpl
	jc enditall
mach7a: sub esp, 88888888h      ;dd
	cmp edx, ecx
	jl short vidloop1a
mach8a: cmp esi, 88888888h      ;de
	jl short begita

endita:

	;eax before: XXXXXXX0 00000000 0XXXxxxx xxxxxxxx
	;     after: XXXXXX00 0000000X XXxxxxxx xxxxxxxx
	mov edx, eax
	mov ebp, eax
	shr edx, 9
	add ebp, ebp
	and eax, 0fc000000h
	and edx, 00010000h
	and ebp, 0000ffffh
	add edx, ebp
	add eax, edx

	add esp, esp
	jmp begitb

prebegitb:
mach1b: cmp esi, 88888888h      ;de
	jge short enditb
begitb:
	mov ebp, ebx
	mov edx, eax
	shr ebp, 20
	add ecx, esp
	shr edx, 14
	and ebp, 0fffffff8h
	add ebp, edx
mach3b: add ebx, 88888888h      ;dy    YYYYYYYYYyyyyyyyyyyyyyyy--------
mach4b: add esi, 88888888h      ;dd
	xor edx, edx
	mov dl, byte ptr _grouhei2[ebp]
mach2b: add eax, 88888888h      ;dx    XXXXXX111111111XXXxxxxxxxxxxxxxx
	shl edx, 16
	and eax, 0fc01ffffh
	cmp edx, ecx
	jge short prebegitb
	mov bl, byte ptr _groucol2[ebp]
vidloop1b:
mach5b: mov [edi+88888888h], bl ;ptop
	sub ecx, esi
mach6b: sub edi, 88888888h      ;bpl
	jc enditall
mach7b: sub esp, 88888888h      ;dd
	cmp edx, ecx
	jl short vidloop1b
mach8b: cmp esi, 88888888h      ;de
	jl short begitb

enditb:

	;eax before: XXXXXX00 0000000X XXxxxxxx xxxxxxxx
	;     after: XXXXX000 00000XXX xxxxxxxx xxxxxx00
	mov edx, eax
	mov ebp, eax
	shr edx, 8
	add ebp, ebp
	and eax, 0f8000000h
	and edx, 00040000h
	and ebp, 0003ffffh
	add edx, ebp
	add eax, edx

	add esp, esp
	jmp begitc

prebegitc:
mach1c: cmp esi, 88888888h      ;de
	jge short enditall
begitc:
	mov ebp, ebx
	mov edx, eax
	shr ebp, 21
	add ecx, esp
	shr edx, 16
	and ebp, 0fffffff8h
	add ebp, edx
mach3c: add ebx, 88888888h      ;dy    YYYYYYYYYyyyyyyyyyyyyyyy--------
mach4c: add esi, 88888888h      ;dd
	xor edx, edx
	mov dl, byte ptr _grouhei3[ebp]
mach2c: add eax, 88888888h      ;dx    XXXXXX111111111XXXxxxxxxxxxxxxxx
	shl edx, 16
	and eax, 0f807ffffh
	cmp edx, ecx
	jge short prebegitc
	mov bl, byte ptr _groucol3[ebp]
vidloop1c:
mach5c: mov [edi+88888888h], bl ;ptop
	sub ecx, esi
mach6c: sub edi, 88888888h      ;bpl
	jc short enditall
mach7c: sub esp, 88888888h      ;dd
	cmp edx, ecx
	jl short vidloop1c
mach8c: cmp esi, 88888888h      ;de
	jl short begitc

enditall:

	mov esp, dword ptr ds:[espbak]
	sti

	pop ebp
	ret

CODE ENDS
END
