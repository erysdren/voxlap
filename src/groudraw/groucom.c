//GROUCOM.C by Ken Silverman (http://advsys.net/ken)
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
#include "colook.h"

#define LOGROUSIZE 10
#define GROUSIZE (1<<LOGROUSIZE)

static long sintable[2048], palette[256][4];
static long xdim, ydim, halfxdim, halfydim, pageoffset, pixs, bpl;
static char tempbuf[16384], darkcol;

char groucol[GROUSIZE*GROUSIZE];
char grouhei[GROUSIZE*GROUSIZE];
char groucol2[(GROUSIZE>>1)*(GROUSIZE>>1)];
char grouhei2[(GROUSIZE>>1)*(GROUSIZE>>1)];
char groucol3[(GROUSIZE>>2)*(GROUSIZE>>2)];
char grouhei3[(GROUSIZE>>2)*(GROUSIZE>>2)];

volatile char keystatus[256], readch, oldreadch, extended, keytemp;
static void (__interrupt __far *oldkeyhandler)();
static void __interrupt __far keyhandler(void);

volatile long clockspeed, totalclock, numframes;
static void (__interrupt __far *oldtimerhandler)();
static void __interrupt __far timerhandler(void);

extern void setupgrouvline(long,long,long,long,long,long);
#pragma aux setupgrouvline parm [eax][ebx][ecx][edx][esi][edi];
extern void grouvline(long,long,long,long,long,long);
#pragma aux grouvline parm [eax][ebx][ecx][edx][esi][edi];

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
}


int main (int argc,char **argv)
{
	long i, j, ang, angvel, templong, vel, svel, horiz, depth, fil;
	long oldposx, oldposy, oldposz, posx, posy, posz, x, y, r, g, b;
	short mousx, mousy;
	char filename[260];

	xdim = 320; ydim = 200; pixs = 1; bpl = 80;
	halfxdim = (xdim>>1); halfydim = (ydim>>1);
	strcpy(filename,"c1.dta");
	if (argc == 2) filename[1] = (char)(atoi(argv[1])+48);
	fcalcsin();

	setvmode(0x13);
	koutpw(0x03d4,0x0014);
	koutpw(0x03d4,0xe317);
	koutpw(0x03c4,0x0604);

	if (!loadata(filename))
	{
		setvmode(0x3);
		printf("%s/",filename); filename[0] = 'd'; printf("%s not found",filename);
		exit(1);
	}

	initclosestcolorfast(palette);
	r = g = b = 0;
	for(y=0;y<(GROUSIZE>>1);y++)
		for(x=0;x<(GROUSIZE>>1);x++)
		{
			i = ((y<<1)&7)+(x<<4)+((y<<11)&0xfe000);
			j = (y&7)+(x<<3)+((y<<9)&0x3f000);
			grouhei2[j] = ((grouhei[i]+grouhei[i+1]+grouhei[i+8]+grouhei[i+9]+2)>>2);
			//groucol2[j] = groucol[i];
			r += ((palette[groucol[i]][0] + palette[groucol[i+1]][0] + palette[groucol[i+8]][0] + palette[groucol[i+9]][0]+2)>>2);
			g += ((palette[groucol[i]][1] + palette[groucol[i+1]][1] + palette[groucol[i+8]][1] + palette[groucol[i+9]][1]+2)>>2);
			b += ((palette[groucol[i]][2] + palette[groucol[i+1]][2] + palette[groucol[i+8]][2] + palette[groucol[i+9]][2]+2)>>2);
			r = min(max(r,0),63);
			g = min(max(g,0),63);
			b = min(max(b,0),63);
			groucol2[j] = closestcol[(r<<12)+(g<<6)+b];
			r -= palette[groucol2[j]][0];
			g -= palette[groucol2[j]][1];
			b -= palette[groucol2[j]][2];
		}
	r = g = b = 0;
	for(y=0;y<(GROUSIZE>>2);y++)
		for(x=0;x<(GROUSIZE>>2);x++)
		{
			i = ((y<<1)&7)+(x<<4)+((y<<10)&0x3f000);
			j = (y&7)+(x<<3)+((y<<8)&0xf800);
			grouhei3[j] = ((grouhei2[i]+grouhei2[i+1]+grouhei2[i+8]+grouhei2[i+9]+2)>>2);
			//groucol3[j] = groucol2[i];
			r = ((palette[groucol2[i]][0] + palette[groucol2[i+1]][0] + palette[groucol2[i+8]][0] + palette[groucol2[i+9]][0]+2)>>2);
			g = ((palette[groucol2[i]][1] + palette[groucol2[i+1]][1] + palette[groucol2[i+8]][1] + palette[groucol2[i+9]][1]+2)>>2);
			b = ((palette[groucol2[i]][2] + palette[groucol2[i+1]][2] + palette[groucol2[i+8]][2] + palette[groucol2[i+9]][2]+2)>>2);
			r = min(max(r,0),63);
			g = min(max(g,0),63);
			b = min(max(b,0),63);
			groucol3[j] = closestcol[(r<<12)+(g<<6)+b];
			r -= palette[groucol3[j]][0];
			g -= palette[groucol3[j]][1];
			b -= palette[groucol3[j]][2];
		}

	posx = posy = posz = 0L; ang = 256; depth = 32<<8; horiz = -20;
	vel = svel = angvel = 0;
	pageoffset = 0;

	for(i=0;i<256;i++) keystatus[i] = 0;
	oldkeyhandler = _dos_getvect(0x9);
	_disable(); _dos_setvect(0x9, keyhandler); _enable();

	oldtimerhandler = _dos_getvect(0x8);
	_disable(); _dos_setvect(0x8, timerhandler); _enable();
	koutp(0x43,0x36); koutp(0x40,4972&255); koutp(0x40,4972>>8);

	clockspeed = totalclock = numframes = 0L;

	while (!keystatus[1])
	{
		drawgrou(posx,posy,posz,ang,horiz);

		koutpw(0x3d4,0xc+(pageoffset&0xff00));
		limitrate(); //Added 12/23/2005
		pageoffset += 16384; if (pageoffset >= 65536) pageoffset = 0;

		if (keystatus[0x02]) { keystatus[0x02] = 0; pixs = 1; }
		if (keystatus[0x03]) { keystatus[0x03] = 0; pixs = 2; }
		if (keystatus[0x05]) { keystatus[0x05] = 0; pixs = 4; }

		svel = 0;
		readmousexy(&mousx,&mousy);
		angvel = (angvel*3+(mousx<<1));
		if (angvel > 0) angvel >>= 2;
		else if (angvel < 0) angvel = ((angvel+3)>>2);
		vel = (vel*3+(-mousy<<4));
		if (vel > 0) vel >>= 2;
		else if (vel < 0) vel = ((vel+3)>>2);

		if (keystatus[0xd1]|keystatus[0x4e]) horiz += clockspeed;
		if (keystatus[0xc9]|keystatus[0x4a]) horiz -= clockspeed;
		if (keystatus[0x1e]) depth += (clockspeed<<7);
		if (keystatus[0x2c]) depth = max(depth-(clockspeed<<7),4<<8);
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
		i = ((posy>>16)&7)+((posx>>13)&0x1ff8)+((posy>>6)&0xfe000);
		posz = max((posz*7+depth-(grouhei[i]<<8))>>3,(4-grouhei[i])<<8);
		//if (posz > oldposz+(16<<8))
		//   { posx = oldposx; posy = oldposy; posz = oldposz; }
		numframes++;
		totalclock += clockspeed; clockspeed = 0;
	}
	koutp(0x43,0x36); koutp(0x40,0); koutp(0x40,0);
	_disable(); _dos_setvect(0x8, oldtimerhandler); _enable();
	_disable(); _dos_setvect(0x9, oldkeyhandler); _enable();
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

int drawgrou (long posx, long posy, long posz, long ang, long horiz)
{
	long i, h, sx, p, d, dd, de, x, y, z, dx, dy, dz, ox, oy, ptop, ddx, ddy;
	long pbot;
	char col;

	ptop = pageoffset+0xa0000;
	dd = 49152/halfxdim; de = dd*352;
	ox = sintable[(ang+512)&2047]; oy = sintable[ang&2047];
	x = (posx<<6); y = (posy<<6); z = (-posz<<8);
	dz = (dd*(halfydim-horiz));

	dx = mulscale8(dd,ox*halfxdim + halfxdim*oy);
	dy = mulscale8(dd,oy*halfxdim - halfxdim*ox);
	ddx = -mulscale8(dd,oy*pixs);
	ddy = mulscale8(dd,ox*pixs);

	pbot = (ydim-1)*bpl;
	koutpw(0x3c4,0x0f02);
	clearbuf(ptop,xdim*ydim>>4,(long)darkcol+((long)darkcol<<8)+((long)darkcol<<16)+((long)darkcol<<24));
	for(sx=0;sx<xdim;sx+=pixs)
	{
		switch(pixs)
		{
			case 1: koutp(0x3c5,1<<(sx&3)); break;
			case 2: koutp(0x3c5,3<<(sx&3)); break;
		}
		setupgrouvline(de,(dy&0xfe000000)+((dy&0x01ffffff)>>10)+0x01ff8000,dx,dd,bpl,ptop);
		grouvline((y&0xfe000000)+((y&0x01ffffff)>>10),x,z,dz,d,pbot+(sx>>2));

		dx += ddx; dy += ddy;
	}
	return(0);
}
