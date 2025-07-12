//GROUDRAW.C by Ken Silverman (http://advsys.net/ken)
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <dos.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>

static unsigned int a, b;
static long tantable[1024];
static int sintable[2048], radarang[360], moustat, bstatus, pixs, vidmode;
static unsigned int dinc[360], lastpageoffset, pageoffset;
static unsigned char readch, oldreadch, keystatus[256], extended;
static unsigned char tempbuf[256], palette[768];
static int clockspeed, xdim, ydim;
static long totalclock, numframes;
void interrupt far ksmhandler();
void (interrupt far *oldhand)();
void interrupt far keyhandler();
void (interrupt far *oldkeyhandler)();

void drawgrou (long posx, long posy, int posz, int ang, int horiz)
{
	unsigned char xdir, ydir;
	int i, j, k, x, y, pass, cnt;
	unsigned int plc, tplc, dink, xink, yink;
	long xinc, yinc;

	pass = 8;
	i = 0;
	cnt = 256;
	while (pass >= pixs)
	{
		outp(0x3c4,0x2);
		switch(pixs)
		{
			case 1: outp(0x3c5,1<<(i&3)); break;
			case 2: outp(0x3c5,3<<(i&3)); break;
			case 4: outp(0x3c5,15); break;
		}
		xinc = (((long)sintable[(512-(radarang[i]+ang))&2047])<<2);
		yinc = (((long)sintable[(    (radarang[i]+ang))&2047])<<2);
		if (xinc > 0) xdir = 1; else xdir = 255;
		if (yinc > 0) ydir = 1; else ydir = 255;
		xink = (unsigned)(labs(xinc)), yink = (unsigned)(labs(yinc));
		x = ((((int)(posx>>8))+16384)&255);
		y = ((((int)(posy>>8))+16384)&255);
		if (vidmode == 0)
		{
			tplc = pageoffset+(i>>2);
			plc = tplc + 18000;
		}
		else
		{
			tplc = pageoffset+(i>>2);
			plc = tplc + 21600;
		}
		dink = dinc[i];
		_asm \
		{
			mov ax, posz
			mov word ptr cs:[groulab1+1], ax
			mov ax, horiz
			mov word ptr cs:[groulab2+1], ax
			mov ax, xink
			mov word ptr cs:[groulab3+5], ax

			xor bl, bl                                    ;Quadruple xink
			mov cx, ax
			add ax, cx
			adc bl, 0
			add ax, cx
			adc bl, 0
			add ax, cx
			adc bl, 0
			mov byte ptr cs:[groulab4x+2], bl
			mov word ptr cs:[groulab3x+5], ax

			cmp xdir, 1
			je skipfixx1
			or byte ptr cs:[groulab4+1], 0x08
			or byte ptr cs:[groulab4x+1], 0x08
			xor al, al
			mov ah, byte ptr posx[0]
			neg ax
			mov word ptr cs:[xcntlab], ax
			jmp skipfixx2
skipfixx1:
			and byte ptr cs:[groulab4+1], 0xf7
			and byte ptr cs:[groulab4x+1], 0xf7
			xor al, al
			mov ah, byte ptr posx[0]
			mov word ptr cs:[xcntlab], ax
skipfixx2:
			mov ax, yink
			mov word ptr cs:[groulab5+5], ax

			xor bl, bl                                    ;Quadruple yink
			mov cx, ax
			add ax, cx
			adc bl, 0
			add ax, cx
			adc bl, 0
			add ax, cx
			adc bl, 0
			mov byte ptr cs:[groulab6x+2], bl
			mov word ptr cs:[groulab5x+5], ax

			cmp ydir, 1
			je skipfixy1
			or byte ptr cs:[groulab6+1], 0x08
			or byte ptr cs:[groulab6x+1], 0x08
			xor al, al
			mov ah, byte ptr posy[0]
			neg ax
			mov word ptr cs:[ycntlab], ax
			jmp skipfixy2
skipfixy1:
			and byte ptr cs:[groulab6+1], 0xf7
			and byte ptr cs:[groulab6x+1], 0xf7
			xor al, al
			mov ah, byte ptr posy[0]
			mov word ptr cs:[ycntlab], ax
skipfixy2:
			mov ax, dink
			mov word ptr cs:[groulab7+5], ax

			xor bl, bl                                    ;Quadruple dink
			mov cx, ax
			add ax, cx
			adc bl, 0
			add ax, cx
			adc bl, 0
			add ax, cx
			adc bl, 0
			mov byte ptr cs:[groulab7y+2], bl
			mov word ptr cs:[groulab7x+5], ax

			mov ax, tplc
			mov word ptr cs:[groulab8+1], ax

			mov word ptr cs:[bplab], bp
			mov word ptr cs:[splab], sp
			mov word ptr cs:[dseglab], ds
			mov word ptr cs:[sseglab], ss
			mov word ptr cs:[dcntlab], 0x0000

			mov bh, byte ptr x[0]
			mov bl, byte ptr y[0]
			mov si, ydim
			mov di, plc
			mov ax, 0xa000
			mov es, ax
			cli
			mov sp, cnt
			mov bp, 1
			mov ss, b
			mov ds, a
			mov cl, 4
			jmp begraytrace

bplab:   nop
			nop
splab:   nop
			nop
dseglab: nop
			nop
sseglab: nop
			nop
xcntlab: nop
			nop
ycntlab: nop
			nop
dcntlab: nop
			nop

fixdx:
			mov al, byte ptr ss:[bx]
			mov cx, si
begdrawvline2:
			sub di, 90
			mov byte ptr es:[di], al
			loop begdrawvline2
			jmp endclearvline

skipdrawanything:
groulab3x:add word ptr cs:[xcntlab], 0x8888           ;xink
groulab4x:adc bh, 0                                   ;xdir for adc/sbb
groulab5x:add word ptr cs:[ycntlab], 0x8888           ;yink
groulab6x:adc bl, 0                                   ;ydir for adc/sbb
groulab7x:add word ptr cs:[dcntlab], 0x8888           ;dink
groulab7y:adc bp, 0
			sub sp, 4
			js endraytrace

begraytrace:
			mov al, byte ptr ds:[bx]
			xor ah, ah
groulab1:add ax, 0x8888                    ;posz
			sal ax, cl
			cwd
			idiv bp
groulab2:add ax, 0x8888                    ;horiz
			jle fixdx
			cmp ax, si
			ja skipdrawanything
			je groulab3
			mov dl, byte ptr ss:[bx]
			mov cx, si
			sub cx, ax
			mov si, ax
begdrawvline1:
			sub di, 90
			mov byte ptr es:[di], dl
			loop begdrawvline1
			mov cl, 4
groulab3:add word ptr cs:[xcntlab], 0x8888           ;xink
groulab4:adc bh, 0                                   ;xdir for adc/sbb
groulab5:add word ptr cs:[ycntlab], 0x8888           ;yink
groulab6:adc bl, 0                                   ;ydir for adc/sbb
groulab7:add word ptr cs:[dcntlab], 0x8888           ;dink
			adc bp, 0
			sub sp, 1
			jns begraytrace


endraytrace:
groulab8:mov ax, 0x8888             ;tplc
			cmp di, ax
			jb endclearvline
begclearvline:
			sub di, 90
			mov byte ptr es:[di], 0
			cmp di, ax
			jae begclearvline
endclearvline:
			mov bp, word ptr cs:[bplab]
			mov sp, word ptr cs:[splab]
			mov ds, word ptr cs:[dseglab]
			mov ss, word ptr cs:[sseglab]
			sti
		}
		if (pass == 8)
			i += 8;
		else
			i += (pass<<1);
		if (i >= xdim)
		{
			pass >>= 1;
			i = pass;
		}
	}
}

void interrupt far keyhandler ()
{
	oldreadch = readch;
	_asm \
	{
		in al, 0x60
		mov readch, al
		in al, 0x61
		or al, 0x80
		out 0x61, al
		and al, 0x7f
		out 0x61, al
	}
	if ((readch|1) == 0xe1)
		extended = 128;
	else
	{
		if (oldreadch != readch)
			keystatus[(readch&127)+extended] = ((readch>>7)^1);
		extended = 0;
	}
	_asm \
	{
		mov al, 0x20
		out 0x20, al
	}
}

unsigned convallocate (unsigned numparagraphs)
{
	unsigned segnum;

	_asm \
	{
		mov ah, 0x48
		mov bx, numparagraphs
		int 0x21
		mov segnum, ax
		jnc allocated
		mov segnum, -1
allocated:
	}
	return(segnum);
}

unsigned convdeallocate (unsigned segnum)
{
	_asm \
	{
		mov ah, 0x49
		mov es, segnum
		int 0x21
		mov segnum, 0
		jnc deallocated
		mov segnum, -1
deallocated:
	}
	return(segnum);
}

int loadtables ()
{
	int fil, i;

	if ((fil = open("tables.dat",O_BINARY|O_RDWR,S_IREAD)) == -1)
		return(-1);
	read(fil,&sintable[0],4096);
	read(fil,&tantable[0],4096);
	read(fil,&radarang[0],720);
	close(fil);
	return(0);
}

void setmode360240 ()
{
	outp(0x3c4,0x4); outp(0x3c5,0x6);
	outp(0x3c4,0x0); outp(0x3c5,0x1);
	outp(0x3c2,0xe7);
	outp(0x3c4,0x0); outp(0x3c5,0x3);
	outp(0x3d4,0x11); outp(0x3d5,inp(0x3d5)&0x7f);      //Unlock indices 0-7
	outp(0x3d4,0x0); outp(0x3d5,0x6b);
	outp(0x3d4,0x1); outp(0x3d5,0x59);
	outp(0x3d4,0x2); outp(0x3d5,0x5a);
	outp(0x3d4,0x3); outp(0x3d5,0x8e);
	outp(0x3d4,0x4); outp(0x3d5,0x5e);
	outp(0x3d4,0x5); outp(0x3d5,0x8a);
	outp(0x3d4,0x6); outp(0x3d5,0xd);
	outp(0x3d4,0x7); outp(0x3d5,0x3e);
	outp(0x3d4,0x9); outp(0x3d5,0x41);
	outp(0x3d4,0x10); outp(0x3d5,0xea);
	outp(0x3d4,0x11); outp(0x3d5,0xac);
	outp(0x3d4,0x12); outp(0x3d5,0xdf);
	outp(0x3d4,0x13); outp(0x3d5,45);
	outp(0x3d4,0x14); outp(0x3d5,0x0);
	outp(0x3d4,0x15); outp(0x3d5,0xe7);
	outp(0x3d4,0x16); outp(0x3d5,0x6);
	outp(0x3d4,0x17); outp(0x3d5,0xe3);
}

void interrupt far ksmhandler ()
{
	clockspeed++;
	_asm \
	{
		mov al, 0x20
		out 0x20, al
	}
}

int setupmouse ()
{
	int snot;

	_asm \
	{
		mov snot, 0
		mov ax, 0x3533
		int 0x21
		mov dx, es
		or dx, bx
		jz nomouse
		cmp es:[bx], 0xcf
		je nomouse
		jmp skipnomouse
		mov ax, 0
		int 0x33
nomouse:
		mov snot, -1
skipnomouse:
	}
	return(snot);
}

int main (int argc, char **argv)
{
	unsigned char tempchar, filename[260];
	int i, j, posz, ang, vel, svel, angvel, depth, horiz, fil, oldposz;
	long oldposx, oldposy, posx, posy, templong;

	pixs = 4;
	vidmode = 0;
	xdim = 320;
	ydim = 200;
	strcpy(filename,"groudraw.dat");
	if ((argc == 2) || (argc == 3))
	{
		if (argc == 3)
		{
			strcpy(filename,argv[2]);
			strcat(filename,".dat");
		}
		pixs = atoi(argv[1]);
		if (pixs < 0)
		{
			vidmode = 1;
			xdim = 360;
			ydim = 240;
			pixs = -pixs;
		}
		if ((pixs != 1) && (pixs != 2) && (pixs != 4))
			pixs = 4;
	}
	moustat = setupmouse();
	if (loadtables() == -1)
	{
		printf("Tables.dat required to run Groudraw.\n");
		exit(0);
	}
	if ((a = convallocate((unsigned)(65536>>4))) == -1)
	{
		printf("Allocation failed!");
		exit(1);
	}
	if ((b = convallocate((unsigned)(65536>>4))) == -1)
	{
		printf("Allocation failed!");
		exit(1);
	}
	_asm \
	{
		mov ax, 0x13
		int 0x10
	}
	if (vidmode == 1)
		setmode360240();
	outp(0x3c4,0x4), outp(0x3c5,inp(0x3c5)&(255-8));    //CHAIN 4 OFF
	outp(0x3d4,0x17), outp(0x3d5,inp(0x3d5)|64);        //WORD MODE
	outp(0x3d4,0x14), outp(0x3d5,inp(0x3d5)&(255-64));  //2-WORD MODE
	outp(0x3d4,0x13); outp(0x3d5,45);
	for(i=0;i<360;i++)
		dinc[i] = (unsigned)(labs((long)sintable[(2560-radarang[i])&2047])<<1);
	fil = open(filename,O_BINARY|O_RDWR,S_IREAD);
	if (fil == -1)
	{
		_asm \
		{
			mov ax, 0x3
			int 0x10
		}
		printf("%s not found\n",filename);
		exit(1);
	}
	read(fil,&palette[0],768);
	outp(0x3c8,0);
	for(i=0;i<768;i++)
		outp(0x3c9,palette[i]);
	for(i=0;i<256;i++)
	{
		read(fil,&tempbuf[0],256);
		_asm \
		{
			mov es, a
			xor bl, bl
			mov ah, byte ptr i[0]
begloadata1:
			xor bh, bh
			mov al, tempbuf[bx]
			mov bh, ah
			mov es:[bx], al
			add bl, 1
			jnc begloadata1
		}
	}
	for(i=0;i<256;i++)
	{
		read(fil,&tempbuf[0],256);
		_asm \
		{
			mov es, b
			xor bl, bl
			mov ah, byte ptr i[0]
begloadata2:
			xor bh, bh
			mov al, tempbuf[bx]
			mov bh, ah
			mov es:[bx], al
			add bl, 1
			jnc begloadata2
		}
	}
	close(fil);
	bstatus = 0;
	posx = 0, posy = 0, posz = 0, ang = 256, depth = 64, horiz = 50;
	pageoffset = 256, lastpageoffset = 256;
	for(i=0;i<256;i++)
		keystatus[i] = 0;
	oldkeyhandler = _dos_getvect(0x9);
	_disable(); _dos_setvect(0x9, keyhandler); _enable();
	clockspeed = 0;
	totalclock = 0L;
	numframes = 0L;
	outp(0x43,54); outp(0x40,4972&255); outp(0x40,4972>>8);
	oldhand = _dos_getvect(0x8);
	_disable(); _dos_setvect(0x8, ksmhandler); _enable();
	while ((keystatus[1] == 0) && (bstatus == 0))
	{
		drawgrou(posx, posy, posz, ang, horiz);

		lastpageoffset = pageoffset;
		outp(0x3d4,0xc); outp(0x3d5,lastpageoffset>>8);
		if (lastpageoffset < 22016) pageoffset = 22016;
		if (lastpageoffset == 22016) pageoffset = 43776;
		if (lastpageoffset > 22016) pageoffset = 256;

		vel = 0;
		svel = 0;
		angvel = 0;
		if (moustat == 0)
			_asm \
			{
				mov ax, 11
				int 0x33
				add ang, cx
				neg dx
				sal dx, 1
				sal dx, 1
				sal dx, 1
				sal dx, 1
				sal dx, 1
				mov vel, dx
				mov ax, 5
				int 0x33
				mov bstatus, ax
			}
		if (keystatus[0x4e] > 0) horiz += 4;
		if (keystatus[0x4a] > 0) horiz -= 4;
		if (keystatus[0x1e] > 0) depth += 4;
		if (keystatus[0x2c] > 0) depth -= 4;
		if (keystatus[0x9d] == 0)
		{
			if (keystatus[0xcb] > 0) angvel = -16;
			if (keystatus[0xcd] > 0) angvel = 16;
		}
		else
		{
			if (keystatus[0xcb] > 0) svel = 384;
			if (keystatus[0xcd] > 0) svel = -384;
		}
		if (keystatus[0xc8] > 0) vel = 384;
		if (keystatus[0xd0] > 0) vel = -384;
		if (keystatus[0x2a] > 0)
		{
			vel <<= 1;
			svel <<= 1;
		}
		oldposx = posx;
		oldposy = posy;
		oldposz = posz;
		if (angvel != 0)
		{
			ang += ((angvel*clockspeed)>>3);
			ang &= 2047;
		}
		if ((vel != 0) || (svel != 0))
		{
			posx += (int)((((long)vel)*((long)clockspeed)*((long)sintable[(512+ang)&2047]))>>17);
			posy += (int)((((long)vel)*((long)clockspeed)*((long)sintable[(    ang)&2047]))>>17);
			posx += (int)((((long)svel)*((long)clockspeed)*((long)sintable[(    ang)&2047]))>>17);
			posy -= (int)((((long)svel)*((long)clockspeed)*((long)sintable[(512+ang)&2047]))>>17);
			posx = (posx+65536)&65535;
			posy = (posy+65536)&65535;
		}
		_asm \
		{
			mov es, a
			mov bh, byte ptr posx[1]
			mov bl, byte ptr posy[1]
			mov al, es:[bx]
			mov tempchar, al
		}
		posz = depth-(int)tempchar;
		/*if (posz > oldposz+16)
		{
			posx = oldposx;
			posy = oldposy;
			posz = oldposz;
		}*/
		numframes++;
		totalclock += ((long)clockspeed);
		clockspeed = 0;
	}
	outp(0x43,54); outp(0x40,255); outp(0x40,255);
	_disable(); _dos_setvect(0x8, oldhand); _enable();
	_disable(); _dos_setvect(0x9, oldkeyhandler); _enable();
	convdeallocate(a);
	convdeallocate(b);
	_asm \
	{
		mov ax, 0x3
		int 0x10
	}
	if (totalclock != 0)
	{
		templong = (numframes*24000L)/totalclock;
		printf("%d.%1d%1d frames per second\n",(int)(templong/100),(int)((templong/10)%10),(int)(templong%10));
	}
	return(0);
}
