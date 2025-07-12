//OVERGROU.C by Ken Silverman (http://advsys.net/ken)
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <dos.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include "pragmas.h"
#include "ves2.h"

#define XDIM 640
#define YDIM 480

#define LOGROUSIZE 10
#define GROUSIZE (1<<LOGROUSIZE)

#define LOGRAYANGLESKIP 1
#define RADARAD 800
	//((long)sqrt(XDIM*XDIM+YDIM*YDIM)*.5+2)   320x200: 188, 640x480: 400

static long sintable[2048], palette[256][4];
static long xdim = XDIM, ydim = YDIM, halfxdim, halfydim;
static char tempbuf[16384], darkcol;

long ylookup[YDIM];
char radar[RADARAD*(2048>>LOGRAYANGLESKIP)];
long scrind[XDIM*YDIM];
long grouzend[2048>>LOGRAYANGLESKIP];

char groucol[GROUSIZE*GROUSIZE];
char grouhei[GROUSIZE*GROUSIZE];

volatile char keystatus[256], readch, oldreadch, extended, keytemp;
static void (__interrupt __far *oldkeyhandler)();
static void __interrupt __far keyhandler(void);

volatile long clockspeed, totalclock, numframes;
static void (__interrupt __far *oldtimerhandler)();
static void __interrupt __far timerhandler(void);

extern void setupovergrou(long,long);
#pragma aux setupovergrou parm [eax][ebx];
extern void overgrou(long,long,long,long,long,long);
#pragma aux overgrou parm [eax][ebx][ecx][edx][esi][edi];

	//for(a=0;a<64000;a+=4)
	//{
	//   *(long *)(a+frameplace) = (((long)radar[scrind[a  ]])    )+
	//                             (((long)radar[scrind[a+1]])<< 8)+
	//                             (((long)radar[scrind[a+2]])<<16)+
	//                             (((long)radar[scrind[a+3]])<<24);
	//}
//#pragma aux blitradar =
//   "lea esi, scrind-16"
//   "mov ebx, [esi+ecx*4+8]"
//   "mov edx, [esi+ecx*4+12]"
//   "beg: mov al, radar[ebx]"
//   "mov ebx, [esi+ecx*4]"
//   "mov ah, radar[edx]"
//   "mov edx, [esi+ecx*4+4]"
//   "shl eax, 16"
//   "sub ecx, 4"
//   "mov al, radar[ebx]"
//   "mov ebx, [esi+ecx*4+8]"
//   "mov ah, radar[edx]"
//   "mov edx, [esi+ecx*4+12]"
//   "mov [edi+ecx], eax"
//   "jnz short beg"
//   parm [edi][ecx]
//   modify [eax ebx edx esi]

	//Optimized for P6
void blitradar (long, long);
#pragma aux blitradar =\
	"sub ecx, 2"\
	"beg: mov eax, scrind[ecx*4+4]"\
	"mov ebx, scrind[ecx*4]"\
	"mov al, radar[eax]"\
	"mov bl, radar[ebx]"\
	"mov [edi+ecx+1], al"\
	"mov [edi+ecx], bl"\
	"sub ecx, 2"\
	"jnc short beg"\
	parm [edi][ecx]\
	modify exact [eax ebx ecx]

static float oneover1024 = 1/1024.0, fsinrange = 16384.0;
void fcalcsinasm ();
#pragma aux fcalcsinasm =\
	"fldpi"\
	"fmul dword ptr [oneover1024]"\
	"fld st"\
	"fsin"\
	"fmul dword ptr [fsinrange]"\
	"fxch st(1)"\
	"fmul st, st"\
	"fchs"\
	"fldz"\
	"xor eax, eax"\
	"beg: fist dword ptr sintable[eax*4]"\
	"fadd st, st(2)"\
	"fld st(1)"\
	"fmul st, st(1)"\
	"inc eax"\
	"faddp st(3), st"\
	"cmp eax, 512"\
	"jb beg"\
	"fcompp"\
	"fstp st"\
	modify exact [eax]\

void fcalcsin ()
{
	long i;

	fcalcsinasm();
	sintable[512] = 16384;
	for(i=513;i<1024;i++) sintable[i] = sintable[1024-i];
	for(i=1024;i<2048;i++) sintable[i] = -sintable[i-1024];
}

void __interrupt __far timerhandler()
{
	clockspeed++;
	//_chain_intr(oldtimerhandler);
	koutp(0x20,0x20);
}

void __interrupt __far keyhandler()
{
	oldreadch = readch; readch = kinp(0x60);
	keytemp = kinp(0x61); koutp(0x61,keytemp|128); koutp(0x61,keytemp&127);
	if ((readch|1) == 0xe1)
		extended = 128;
	else
	{
		if (oldreadch != readch)
			keystatus[(readch&127)+extended] = ((readch>>7)^1);
		extended = 0;
	}
	koutp(0x20,0x20);
	if (keystatus[0x46])
	{
		koutp(0x43,0x36); koutp(0x40,0); koutp(0x40,0);
		_dos_setvect(0x9, oldkeyhandler);
		_dos_setvect(0x8, oldtimerhandler);
		setvmode(0x3);
		exit(0);
	}
}

int main (int argc, char **argv)
{
	long a, i, j, ang, angvel, templong, vel, svel, horiz, depth, fil;
	long oldposx, oldposy, oldposz, posx, posy, posz, x, y, r, g, b, zlock = 1;
	long usingvesa, pag = 0;
	short mousx, mousy;
	char filename[260];

	if ((usingvesa = setvesa(xdim,ydim,8)) < 0)
		{ setvmode(0x13); xdim = 320; ydim = 200; bytesperline = 320; frameplace = 0xa0000; }
	halfxdim = (xdim>>1); halfydim = (ydim>>1);
	for(i=0;i<ydim;i++) ylookup[i] = i*bytesperline;
	if (maxpages > 8) maxpages = 8;

	strcpy(filename,"c1.dta");
	if (argc == 2) filename[1] = (char)(atoi(argv[1])+48);
	fcalcsin();

	if (!loadata(filename))
	{
		uninitvesa();
		setvmode(0x3);
		printf("%s/",filename); filename[0] = 'd'; printf("%s not found",filename);
		exit(1);
	}

	for(y=(ydim>>1)-1;y>=0;y--)
		for(x=(xdim>>1)-1;x>=0;x--)
		{
			r = (long)sqrt((x-(xdim-1)*.5)*(x-(xdim-1)*.5)+(y-(ydim-1)*.5)*(y-(ydim-1)*.5));
			if (!r)
			{
				scrind[ylookup[y]+x] = 0;
				scrind[ylookup[y]+xdim-1-x] = 0;
				scrind[ylookup[ydim-1-y]+x] = 0;
				scrind[ylookup[ydim-1-y]+xdim-1-x] = 0;
			}
			else
			{
				a = ((long)(atan2(y-(ydim-1)*.5,x-(xdim-1)*.5)*1024/3.141592653589793)&2047);
				scrind[ylookup[y]+x] = (a>>LOGRAYANGLESKIP)*RADARAD+r;
				scrind[ylookup[y]+xdim-1-x] = (((1024-a)&2047)>>LOGRAYANGLESKIP)*RADARAD+r;
				scrind[ylookup[ydim-1-y]+x] = (((-a)&2047)>>LOGRAYANGLESKIP)*RADARAD+r;
				scrind[ylookup[ydim-1-y]+xdim-1-x] = (((a+1024)&2047)>>LOGRAYANGLESKIP)*RADARAD+r;
			}
		}

	for(i=0;i<(2048>>LOGRAYANGLESKIP);i++) grouzend[i] = 0;
	for(i=0;i<2048;i++)
	{
		x = sintable[(i+512)&1023];
		y = sintable[i&1023];
		if (y*xdim > x*ydim) { x = ydim*x/y; y = ydim; }
							 else { y = xdim*y/x; x = xdim; }
		grouzend[i>>LOGRAYANGLESKIP] = max(grouzend[i>>LOGRAYANGLESKIP],sqrt(x*x+y*y)*.5+2);
	}

	posx = posy = posz = 0L; ang = 256; depth = 128<<8; horiz = -20;
	vel = svel = angvel = 0;

	for(i=0;i<256;i++) keystatus[i] = 0;
	oldkeyhandler = _dos_getvect(0x9);
	_disable(); _dos_setvect(0x9, keyhandler); _enable();

	oldtimerhandler = _dos_getvect(0x8);
	_disable(); _dos_setvect(0x8, timerhandler); _enable();
	koutp(0x43,0x36); koutp(0x40,4972&255); koutp(0x40,4972>>8);

	clockspeed = totalclock = numframes = 0L;

	while (!keystatus[1])
	{
		if (usingvesa >= 0)
			setactivepage(pag);

		drawgrou(posx,posy,posz,ang,horiz);

		if (usingvesa >= 0)
		{
			setvisualpage(pag);
			pag++; if (pag >= maxpages) pag = 0;
		}

		svel = 0;
		readmousexy(&mousx,&mousy);
		angvel = (angvel*3+(mousx<<1));
		if (angvel > 0) angvel >>= 2;
		else if (angvel < 0) angvel = ((angvel+3)>>2);
		vel = (vel*3+(-mousy<<4));
		if (vel > 0) vel >>= 2;
		else if (vel < 0) vel = ((vel+3)>>2);

		if (keystatus[0x3a]) { keystatus[0x3a] = 0; zlock ^= 1; }
		if (keystatus[0xd1]) horiz += 4;
		if (keystatus[0xc9]) horiz -= 4;
		if (keystatus[0x1e]) depth += (clockspeed<<8);
		if (keystatus[0x2c]) depth = max(depth-(clockspeed<<8),4<<8);
		if (!keystatus[0x9d])
		{
			if (keystatus[0xcb]) angvel = max(angvel-16,(-24<<keystatus[0x2a]));
			if (keystatus[0xcd]) angvel = min(angvel+16,(24<<keystatus[0x2a]));
		}
		else
		{
			if (keystatus[0xcb]) svel = min(svel+64,(96<<keystatus[0x2a]));
			if (keystatus[0xcd]) svel = max(svel-64,(-96<<keystatus[0x2a]));
		}
		if (keystatus[0xc8]) vel = min(vel+64,(96<<keystatus[0x2a]));
		if (keystatus[0xd0]) vel = max(vel-64,(-96<<keystatus[0x2a]));

		oldposx = posx; oldposy = posy; oldposz = posz;
		if (angvel) ang = ((((angvel*clockspeed)>>3)+ang)&2047);
		if (vel || svel)
		{
			i = sintable[(ang+512)&2047]; j = sintable[ang&2047];
			posx += mulscale6(vel*i+svel*j,clockspeed);
			posy += mulscale6(vel*j-svel*i,clockspeed);
		}
		i = (grouhei[((posy>>16)&7)+((posx>>13)&0x1ff8)+((posy>>6)&0xfe000)]<<8);
		if (zlock) i = (255<<8);
		posz = max((posz*7+depth-i)>>3,(4<<8)-i);
		//if (posz > oldposz+(16<<8))
		//   { posx = oldposx; posy = oldposy; posz = oldposz; }
		numframes++;
		totalclock += clockspeed; clockspeed = 0;
	}
	koutp(0x43,0x36); koutp(0x40,0); koutp(0x40,0);
	_disable(); _dos_setvect(0x8, oldtimerhandler); _enable();
	_disable(); _dos_setvect(0x9, oldkeyhandler); _enable();
	uninitvesa();
	setvmode(0x3);
	if (totalclock)
	{
		templong = (numframes*24000L)/totalclock;
		printf("%d.%1d%1d frames per second\n",(int)(templong/100),(int)((templong/10)%10),(int)(templong%10));
	}
	return(0);
}

#define loadatabyte(dat,datcnt,fil,tempbuf)                      \
{                                                                \
	dat = tempbuf[datcnt++];                                      \
	if (datcnt == 16384) { read(fil,tempbuf,16384); datcnt = 0; } \
}

int loadata (char *filename)
{
	long i, j, p, datcnt, leng, fil, colval, mincolval;
	char dat;

	fil = open(filename,O_BINARY|O_RDWR,S_IREAD); if (fil == -1) return(0);
	read(fil,tempbuf,128);
	p = 0;
	read(fil,tempbuf,16384); datcnt = 0;
	while (p < 1048576)
	{
		loadatabyte(dat,datcnt,fil,tempbuf);
		if (dat >= 192)
		{
			leng = dat-192;
			loadatabyte(dat,datcnt,fil,tempbuf);
			while (leng-- > 0) { groucol[((p>>10)&7)+((p&1023)<<3)+(p&0xfe000)] = dat; p++; }
		}
		else { groucol[((p>>10)&7)+((p&1023)<<3)+(p&0xfe000)] = dat; p++; }
	}

	loadatabyte(dat,datcnt,fil,tempbuf);
	mincolval = 768;
	if (dat == 0xc)
	{
		koutp(0x3c8,0);
		for(i=0;i<256;i++)
		{
			colval = 0;
			for(j=0;j<3;j++)
			{
				loadatabyte(dat,datcnt,fil,tempbuf);
				koutp(0x3c9,dat>>2);
				palette[i][j] = (dat>>2);
				colval += (long)dat;
			}
			if (colval < mincolval) { darkcol = i; mincolval = colval; }
		}
	}
	kinp(0x3da); koutp(0x3c0,0x31); koutp(0x3c0,darkcol);
	close(fil);

	filename[0] = 'd';
	fil = open(filename,O_BINARY|O_RDWR,S_IREAD); if (fil == -1) return(0);
	read(fil,tempbuf,128);
	p = 0;
	read(fil,tempbuf,16384), datcnt = 0;
	while (p < 1048576)
	{
		loadatabyte(dat,datcnt,fil,tempbuf);
		if (dat >= 192)
		{
			leng = dat-192;
			loadatabyte(dat,datcnt,fil,tempbuf);
			while (leng-- > 0) { grouhei[((p>>10)&7)+((p&1023)<<3)+(p&0xfe000)] = 255-dat; p++; }
		}
		else { grouhei[((p>>10)&7)+((p&1023)<<3)+(p&0xfe000)] = 255-dat; p++; }
	}
	close(fil);
	return(1);
}

void drawgrou (long posx, long posy, long posz, long ang, long horiz)
{
	long i, a, aa, x, y, z, xinc, yinc, d, onz, nz, zend, m, t, sc;
	char *rad;

	//sc = min(max((posz+65536)>>7,64),65536);
	sc = 256;
	t = scale(320,sc,xdim);
	setupovergrou(posz,sc<<7);

	radar[0] = groucol[((posy>>16)&7)+((posx>>13)&0x1ff8)+((posy>>6)&0xfe000)];
	aa = ang+(2047-((1<<(LOGRAYANGLESKIP))>>1))+512;
	for(a=(2048>>LOGRAYANGLESKIP)-1;a>=0;a--)
	{
		xinc = sintable[(aa+512)&2047]*t;
		yinc = sintable[ aa     &2047]*t;
		aa -= (1<<LOGRAYANGLESKIP);
		overgrou(grouzend[a],(posx<<6)+xinc,(posy<<6)+yinc,xinc,yinc,(long)&radar[a*RADARAD]);

		/*x = (posx<<6)+xinc;
		y = (posy<<6)+yinc;
		zend = grouzend[a];
		rad = &radar[a*RADARAD];
		i = ((y>>22)&7)+((x>>19)&0x1ff8)+((y>>12)&0xfe000);
		z = 1; nz = m = 0; //nz = (grouhei[i]<<8)+posz; m = nz;
		for(d=(1<<15);;d+=(1<<15))
		{
			i = ((y>>22)&7)+((x>>19)&0x1ff8)+((y>>12)&0xfe000); x += xinc; y += yinc;
			onz = nz; nz = (grouhei[i]<<8)+posz;
			switch(nz-onz)
			{
				case -256: m -= (z<<8); break;
				case 0: break;
				case 256: m += (z<<8); break;
				default: m = z*nz; break;
			}
			while (m < d)
			{
				rad[z++] = groucol[i]; if (z >= zend) goto endit;
				m += nz;
			}
		}
		endit: i += 0;*/
	}

	blitradar(frameplace,xdim*ydim);
}
