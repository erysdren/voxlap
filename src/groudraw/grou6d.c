//GROU6D.C by Ken Silverman (http://advsys.net/ken)
#include <dos.h>
#include <math.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include "pragmas.h"
#include "ves2.h"
#include "colook.h"

#define XDIM 640
#define YDIM 480
#define LOGROUSIZ 10
#define GROUSIZ (1<<LOGROUSIZ)
#define LOGBOXSIZ 4
#define BOXSIZ (1<<LOGBOXSIZ)
#define ANGDENSITY 2.0
#define PI 3.141592653589793
#define HOLEFIX 1

typedef struct { float x, y, z; } point3d;

char radar[XDIM*YDIM*4], *radarindex, *angstart[XDIM*4];
long angnum;
long arbuf[64*64*2], arplc[64*2], arinc[64*2];
long gstartu, gstartv, gzoff;
float cntrecip[XDIM+YDIM];

#if (HOLEFIX == 1)
float alookup[(XDIM+BOXSIZ)*2*BOXSIZ*4];
long rlookup[(XDIM+BOXSIZ)*2*BOXSIZ*4];
#endif

#define GROUIND1(x,y) (((x)&7)+(((x)&~7)<<(LOGROUSIZ  ))+((y)<<3))
char groucol1[(GROUSIZ   )*(GROUSIZ   )];
char grouhei1[(GROUSIZ   )*(GROUSIZ   )];
#define GROUIND2(x,y) (((x)&7)+(((x)&~7)<<(LOGROUSIZ-1))+((y)<<3))
char groucol2[(GROUSIZ>>1)*(GROUSIZ>>1)];
char grouhei2[(GROUSIZ>>1)*(GROUSIZ>>1)];
#define GROUIND3(x,y) (((x)&7)+(((x)&~7)<<(LOGROUSIZ-2))+((y)<<3))
char groucol3[(GROUSIZ>>2)*(GROUSIZ>>2)];
char grouhei3[(GROUSIZ>>2)*(GROUSIZ>>2)];

#define CLIPRAD 8
static long cliphei[CLIPRAD*2+1][CLIPRAD*2+1];

point3d ipos, ivel, istr, ihei, ifor;
float hx, hy, hz, gvx, gvy, gvz;

long xdim = XDIM, ydim = YDIM, ylookup[YDIM];
long xdimax, ydimax;

float wx0, wy0, wx1, wy1, wxc, wyc;

long palette[256][4], whitecol, blackcol;
char tempbuf[16384];

#define TIMERRATE (9942>>1)   //240 times/sec
volatile long totalclock, lockclock, synctics, chainintrclock = 0;
void (__interrupt __far *oldtimerhandler)();
void __interrupt __far timerhandler(void);

volatile char keystatus[256], readch, oldreadch, extended, keytemp;
static void (__interrupt __far *oldkeyhandler)();
static void __interrupt __far keyhandler(void);

	//FPS variables
long ofinetotalclock, ototalclock, averagefps;
#define AVERAGEFRAMES 32
long frameval[AVERAGEFRAMES], framecnt = 0;
char overtbits;

void inittimer42 ();
#pragma aux inittimer42 =\
	"in al, 0x61"\
	"or al, 1"\
	"out 0x61, al"\
	"mov al, 0xb4"\
	"out 0x43, al"\
	"xor al, al"\
	"out 0x42, al"\
	"out 0x42, al"\
	modify exact [eax]

void uninittimer42 ();
#pragma aux uninittimer42 =\
	"in al, 0x61"\
	"and al, 252"\
	"out 0x61, al"\
	modify exact [eax]

long gettimer42 ();
#pragma aux gettimer42 =\
	"mov al, 0x84"\
	"out 0x43, al"\
	"in al, 0x42"\
	"shl eax, 24"\
	"in al, 0x42"\
	"rol eax, 8"\
	modify [eax]

long smallfnt[256] =
{
	0x000000,0x6F9F60,0x69F960,0xAFFE40,0x4EFE40,0x6FF6F0,0x66F6F0,0x000000,
	0xEEAEE0,0x000000,0x000000,0x000000,0x000000,0x000000,0x7755C0,0x96F690,
	0x8CEC80,0x26E620,0x4E4E40,0xAAA0A0,0x7DD550,0x7CA6C0,0x000EE0,0x4E4EE0,
	0x4E4440,0x444E40,0x02F200,0x04F400,0x000000,0x000000,0x000000,0x000000,
	0x000000,0x444040,0xAA0000,0xAFAFA0,0x6C46C0,0xA248A0,0x4A4CE0,0x240000,
	0x488840,0x422240,0x0A4A00,0x04E400,0x000224,0x00E000,0x000040,0x224480,
	0xEAAAE0,0x444440,0xE2E8E0,0xE2E2E0,0xAAE220,0xE8E2E0,0xE8EAE0,0xE22220,
	0xEAEAE0,0xEAE220,0x040400,0x040480,0x248420,0x0E0E00,0x842480,0xC24040,
	0xEAECE0,0x4AEAA0,0xCACAC0,0x688860,0xCAAAC0,0xE8C8E0,0xE8C880,0xE8AAE0,
	0xAAEAA0,0xE444E0,0xE22A60,0xAACAA0,0x8888E0,0xAEEAA0,0xAEEEA0,0xEAAAE0,
	0xEAE880,0xEAAE60,0xEACAA0,0xE8E2E0,0xE44440,0xAAAAE0,0xAAA440,0xAAEEA0,
	0xAA4AA0,0xAAE440,0xE248E0,0xC888C0,0x844220,0x622260,0x4A0000,0x0000E0,
	0x420000,0x006A60,0x88EAE0,0x00E8E0,0x22EAE0,0x006E60,0x24E440,0x06A62C,
	0x88EAA0,0x040440,0x040448,0x88ACA0,0x444440,0x08EEE0,0x00CAA0,0x00EAE0,
	0x00EAE8,0x00EAE2,0x00E880,0x0064C0,0x04E440,0x00AA60,0x00AA40,0x00EEE0,
	0x00A4A0,0x00AA6C,0x00C460,0x648460,0x440440,0xC424C0,0x6C0000,0x04AE00,
	0x68886C,0xA0AA60,0x606E60,0xE06A60,0xA06A60,0xC06A60,0x046A60,0x00E8E4,
	0xE06E60,0xA06E60,0xC06E60,0x0A0440,0x0E0440,0x0C0440,0xA4AEA0,0x404EA0,
	0x60ECE0,0x007A70,0x7AFAB0,0xE0EAE0,0xA0EAE0,0xC0EAE0,0xE0AA60,0xC0AA60,
	0xA0AA6C,0xA0EAE0,0xA0AAE0,0x4E8E40,0x65C4F0,0xA4EE40,0xCAFAB0,0x64E4C0,
	0x606A60,0x060440,0x60EAE0,0x60AA60,0xC0CAA0,0xE0AEA0,0x6A60E0,0xEAE0E0,
	0x404860,0x007400,0x00C400,0x8A4E60,0x8A6E20,0x404440,0x05A500,0x0A5A00,
	0x282828,0x5A5A5A,0xD7D7D7,0x444444,0x44C444,0x44CC44,0x66E666,0x00E666,
	0x00CC44,0x66EE66,0x666666,0x00EE66,0x66EE00,0x66E000,0x44CC00,0x00C444,
	0x447000,0x44F000,0x00F444,0x447444,0x00F000,0x44F444,0x447744,0x667666,
	0x667700,0x007766,0x66FF00,0x00FF66,0x667766,0x00FF00,0x66FF66,0x44FF00,
	0x66F000,0x00FF44,0x00F666,0x667000,0x447700,0x007744,0x007666,0x66F666,
	0x44FF44,0x44C000,0x007444,0xFFFFFF,0x000FFF,0xCCCCCC,0x333333,0xFFF000,
	0x00DAD0,0xCACAC8,0xEA8880,0x00F660,0xE848E0,0x007A40,0x0AAC80,0x05A220,
	0xE4A4E0,0x4AEA40,0x6996F0,0x646AE0,0x06F600,0x16F680,0x68C860,0x4AAAA0,
	0xE0E0E0,0x4E40E0,0x4240E0,0x4840E0,0x4A8880,0x222A40,0x40E040,0x06EC00,
	0xEAE000,0x0CC000,0x00C000,0x644C40,0xCAA000,0xC4E000,0x0EEE00,0x000000,
};

void gline (long cnt, float x0, float y0, float x1, float y1);

extern void setupgrou6d(long,long,long,long,long,long);
#pragma aux setupgrou6d parm [eax][ebx][ecx][edx][esi][edi];
extern void grou6d(long,long,long,long,long,long);
#pragma aux grou6d parm [eax][ebx][ecx][edx][esi][edi];

//┌────────────────────┬────────────────────┐
//│ Rounding: D11-10:  │ Precision: D9-8:   │
//├────────────────────┼────────────────────┤
//│ 00 = near/even (0) │ 00 = 24-bit    (0) │
//│ 01 = -inf      (4) │ 01 = reserved  (1) │
//│ 10 = +inf      (8) │ 10 = 53-bit    (2) │
//│ 11 = 0         (c) │ 11 = 64-bit    (3) │
//└────────────────────┴────────────────────┘

long asm;
void fpuinit (long);
#pragma aux fpuinit =\
	"fninit"\
	"fstcw asm"\
	"mov ah, byte ptr asm[1]"\
	"and ah, 0f0h"\
	"or ah, al"\
	"mov byte ptr asm[1], ah"\
	"fldcw asm"\
	parm [eax]

void ftol (float, long *);
#pragma aux ftol =\
	"fistp dword ptr [eax]"\
	parm [8087][eax]

void fcossin (float *, float *, float *);
#pragma aux fcossin =\
	"fld dword ptr [eax]"\
	"fsincos"\
	"fstp dword ptr [ebx]"\
	"fstp dword ptr [ecx]"\
	parm [eax][ebx][ecx]

#define LOGFSQSIZ 10
static long fsqlookup[1<<LOGFSQSIZ], fpuasm;
void initfsqrtasm ()
{
	long i, j;
	float f, s1, s2;

	s1 = 16777216 / sqrt(1<<LOGFSQSIZ); s2 = s1*sqrt(0.5);
	for(i=(1<<(LOGFSQSIZ-1));i<(1<<LOGFSQSIZ);i++)
	{
		f = sqrt((float)i); j = (i<<(23-LOGFSQSIZ));
		fsqlookup[i-(1<<(LOGFSQSIZ-1))] = (long)(f*s1)-j+0x1f400000;
		fsqlookup[i                   ] = (long)(f*s2)-j+0x1f800000;
	}
}
#define fsqrtasm(i,o)\
	 ((*(long *)o) =\
	 ((*(unsigned long *)i)>>1)+\
	 fsqlookup[((*(long *)i)&(((1<<LOGFSQSIZ)-1)<<(24-LOGFSQSIZ)))>>(24-LOGFSQSIZ)])\

#define LOGFRECIPSIZ 10
long freciplookup[1<<LOGFRECIPSIZ];
void initfrecipasm ()
{
	long i;
	for(i=0;i<(1<<LOGFRECIPSIZ);i++)
	{
		freciplookup[i] = ((1<<LOGFRECIPSIZ)*16777216.0) / ((float)i+(1<<LOGFRECIPSIZ));
		freciplookup[i] += (i<<(23-LOGFRECIPSIZ))+0x7e000000;
	}
}
#define frecipasm(i,o)\
	((*(long *)o) = freciplookup[((*(long *)i)&0x007fffff)>>(23-LOGFRECIPSIZ)]-(*(long *)i))

	//render16(ap,ai,rp,ri,ptr);
void render16 (long, long, long, long, char *);
#pragma aux render16 =\
	"and ebx, -16"\
	"or ecx, 15"\
	"push ebp"\
	"jmp short intoit"\
	"beg: mov byte ptr [edi-1], al"\
	"intoit: mov eax, ebx"\
	"sar eax, 16"\
	"mov ebp, edx"\
	"sar ebp, 16"\
	"add ebx, ecx"\
	"mov eax, dword ptr angstart[eax*4]"\
	"inc edi"\
	"lea edx, [edx+esi]"\
	"test ebx, 15"\
	"mov al, byte ptr [eax+ebp]"\
	"jnz short beg"\
	"mov byte ptr [edi-1], al"\
	"pop ebp"\
	parm [ebx][ecx][edx][esi][edi] modify exact [eax ebx edx edi]

long gotmmx ();
#pragma aux gotmmx =\
	"pushfd"\
	"pop eax"\
	"mov ebx, eax"\
	"xor eax, 0x200000"\
	"push eax"\
	"popfd"\
	"pushfd"\
	"pop eax"\
	"cmp eax, ebx"\
	"je bad"\
	"xor eax, eax"\
	"dw 0xa20f"\
	"test eax, eax"\
	"jz bad"\
	"mov eax, 1"\
	"dw 0xa20f"\
	"test edx, 0x800000"\
	"jz bad"\
	"mov eax, 1"\
	"jmp endit"\
	"bad: xor eax, eax"\
	"endit:"\
	modify exact [eax ebx ecx edx]

long hitscan(point3d *dp, point3d *dv, point3d *ret);

int main (int argc, char **argv)
{
	point3d v;
	float t, tt, ttt, ox, oy, oz, fx, fy, fz, a1 = 0, a2 = 0, a3 = 0, rr[9];
	float cosa1, sina1, cosa2, sina2, cosa3, sina3, c1c3, c1s3, s1c3, s1s3;
	float ang0, ang1, ainc, rad1, fx0, fy0, fx1, fy1, r0, r1, cosang, sinang;
	float cosangi, sinangi;
	long i, j, k, l, sx, sy, x, y, z, xx, yy, zz, usingvesa, pag, *aptr;
	long aps, rps, api, rpi, ap, ai, rp, ri, xboxleng, badboxline;
	short mousx, mousy, bstatus;
	char ch, filename[260], *ptr;

	if (!gotmmx())
		{ printf("Sorry... This program requires MMX\n"); exit(1); }

	if ((usingvesa = setvesa(xdim,ydim,8)) < 0)
		{ setvmode(0x13); xdim = 320; ydim = 200; bytesperline = 320; frameplace = 0xa0000; }
	//printf("Usingvesa: %ld\n",usingvesa);

	hx = (xdim>>1); hy = (ydim>>1); hz = hx;
	xdimax = (xdim<<16)-1;
	ydimax = (ydim<<16)-1;
	for(i=0;i<ydim;i++) ylookup[i] = i*bytesperline;
	if (maxpages > 8) maxpages = 8;

	fpuinit(0x0c);  //24 bit, round to 0
	initfsqrtasm();
	initfrecipasm();

	for(i=0;i<1600;i++) angstart[i] = &radar[65536];
	wx0 = -ANGDENSITY-1; wx1 = (XDIM|(BOXSIZ-1))+ANGDENSITY+1;
	wy0 = -ANGDENSITY-1; wy1 = (YDIM|(BOXSIZ-1))+ANGDENSITY+1;
	wxc = XDIM*.5; wyc = YDIM*.5;

#if (HOLEFIX == 1)
	for(y=-(BOXSIZ<<1);y<(BOXSIZ<<1);y++)
		for(x=-XDIM-BOXSIZ;x<XDIM+BOXSIZ;x++)
		{
			fx0 = (float)x; fy0 = (float)y;
			tt = atan2(fy0,fx0); if (tt < 0) tt += PI*2;
			alookup[(y+(BOXSIZ<<1))*(XDIM+BOXSIZ)*2+x+XDIM+BOXSIZ] = tt*(1.0/65536.0);
			ftol(65536.0*sqrt(fx0*fx0+fy0*fy0),&z);
			rlookup[(y+(BOXSIZ<<1))*(XDIM+BOXSIZ)*2+x+XDIM+BOXSIZ] = (z>>16);
		}
#endif

	for(y=-CLIPRAD;y<=CLIPRAD;y++)
		for(x=-CLIPRAD;x<=CLIPRAD;x++)
		{
			t = CLIPRAD*CLIPRAD - x*x - y*y;
			if (t > 0) cliphei[x+CLIPRAD][y+CLIPRAD] = sqrt(t);
					else cliphei[x+CLIPRAD][y+CLIPRAD] = -1;
		}

	xboxleng = ((xdim>>LOGBOXSIZ)<<1);

	strcpy(filename,"c1.dta");
	if (argc == 2) filename[1] = (char)(atoi(argv[1])+48);
	if (!loadata(filename))
	{
		uninitvesa();
		setvmode(0x3);
		printf("%s/",filename); filename[0] = 'd'; printf("%s not found",filename);
		exit(1);
	}

	koutp(0x3c8,0);
	for(i=0;i<256;i++)
		for(j=0;j<3;j++)
			koutp(0x3c9,palette[i][j]);

	initclosestcolorfast(palette);
	blackcol = closestcol[0]; whitecol = closestcol[262143];
	blackcol += (blackcol<<8); whitecol += (whitecol<<8);
	blackcol += (blackcol<<16); whitecol += (whitecol<<16);
	kinp(0x3da); koutp(0x3c0,0x31); koutp(0x3c0,blackcol);

	mipit(0L,0L,0L,0L);

	for(i=0;i<XDIM+YDIM;i++) cntrecip[i] = 1.0/((float)i);

	if (vgacompatible)
	{
			//Init VR flag
		koutp(0x3d4,0x11);
		overtbits = kinp(0x3d5) & ~(16+32);
		koutp(0x3d5,overtbits);
		koutp(0x3d5,overtbits+16);
	}

	inittimer();
	inittimer42();
	initkeys();

	ipos.x = 0; ipos.y = 192; ipos.z = 0;
	ivel.x = 0; ivel.y = 0; ivel.z = 0;
	istr.x = -1; istr.y = 0; istr.z = 0;
	ihei.x = 0; ihei.y = 1; ihei.z = 0;
	ifor.x = 0; ifor.y = 0; ifor.z = 1;
	setupmouse();

	pag = 0;
	totalclock = lockclock = synctics = 0;
	while (!keystatus[1])
	{
		if (usingvesa >= 0) setactivepage(pag);

		//clearbuf(frameplace,xdim*ydim>>2,blackcol);

		gvx = -hx*istr.x - hy*ihei.x + hz*ifor.x;
		gvy = -hx*istr.y - hy*ihei.y + hz*ifor.y;
		gvz = -hx*istr.z - hy*ihei.z + hz*ifor.z;
		gstartu = ((long)(ipos.x*64.0))<<16;
		gstartu = (gstartu&0xfe000000)+((gstartu>>10)&0x00007fff);
		gstartv = ((long)(ipos.z*64.0))<<16;
		gzoff = (((long)(ipos.y*-32.0))<<16);


		if (fabs(ifor.y) > .01) t = hz/ifor.y;
		else if (ifor.y < 0) t = hz / -.01;
		else t = hz / .01;

		ox = istr.y*t + hx; fx0 = wx0-ox; fx1 = wx1-ox;
		oy = ihei.y*t + hy; fy0 = wy0-oy; fy1 = wy1-oy;

		if (ox > wxc) rad1 = fx0*fx0; else rad1 = fx1*fx1;
		if (oy > wyc) rad1 += fy0*fy0; else rad1 += fy1*fy1;

		rad1 = sqrt(rad1);
		ainc = ANGDENSITY/rad1;

		badboxline = 0x3fffffff;
		if (fx0 > 0)
		{  if (fy0 > 0) { ang0 = atan2(fy0,fx1); ang1 = atan2(fy1,fx0); }
			else if (fy1 < 0) { ang0 = atan2(fy0,fx0); ang1 = atan2(fy1,fx1); }
			else { ang0 = atan2(fy0, fx0); ang1 = atan2(fy1,fx0); }
		}
		else if (fx1 < 0)
		{  if (fy0 > 0) { ang0 = atan2(fy1,fx1); ang1 = atan2(fy0,fx0); }
			else if (fy1 < 0) { ang0 = atan2(fy1,fx0); ang1 = atan2(fy0,fx1); }
			else { ang0 = atan2(fy1,fx1); ang1 = atan2(fy0,fx1); }
		}
		else
		{  if (fy0 > 0) { ang0 = atan2(fy0,fx1); ang1 = atan2(fy0,fx0); }
			else if (fy1 < 0) { ang0 = atan2(fy1,fx0); ang1 = atan2(fy1,fx1); }
			else { ang0 = 0; ang1 = PI*2; ftol(oy,&badboxline); badboxline &= ~(BOXSIZ-1); }
		}
		if (ang0 > ang1) ang1 += PI*2;

			//Improve resolution for when view has horizon in center
		if ((ang1-ang0) < ainc*(XDIM*1)) ainc = (ang1-ang0)/(XDIM*1);
		if ((ang1-ang0) > ainc*(XDIM*4)) ainc = (ang1-ang0)/(XDIM*4);

		radarindex = radar+1024; angnum = 0;

		fcossin(&ang0,&cosang,&sinang);  //Sin interpolation - See FCALCSIN.BAS
		t = ang0+ainc; fcossin(&t,&cosangi,&sinangi);
		cosangi -= cosang; sinangi -= sinang;
		ftol((ang1-ang0)/ainc,&k);
		t = -ainc*ainc;
		for(;k>=0;k--)
		{
			r0 = 0;
			if (fx0 > 0) r0 = fx0/cosang;
			else if (fx1 < 0) r0 = fx1/cosang;
			if (fy0 > 0) { tt = fy0/sinang; if (tt > r0) r0 = tt; }
			else if (fy1 < 0) { tt = fy1/sinang; if (tt > r0) r0 = tt; }

			r1 = rad1;
			if (cosang < 0) { if (fx0 <= 0) r1 = fx0/cosang; }
			else if (cosang > 0) { if (fx1 >= 0) r1 = fx1/cosang; }
			if (sinang < 0) { if (fy0 <= 0) { tt = fy0/sinang; if (tt < r1) r1 = tt; } }
			else if (sinang > 0) { if (fy1 >= 0) { tt = fy1/sinang; if (tt < r1) r1 = tt; } }

			ftol(r0,&i); ftol(r1,&j); j++;
			if (ifor.y < 0)
			{
				angstart[angnum++] = radarindex+j;
				gline(j-i,ox+(float)j*cosang,oy+(float)j*sinang,ox+(float)i*cosang,oy+(float)i*sinang);
			}
			else
			{
				angstart[angnum++] = radarindex-i;
				gline(j-i,ox+(float)i*cosang,oy+(float)i*sinang,ox+(float)j*cosang,oy+(float)j*sinang);
			}

			cosang += cosangi; cosangi += cosang*t;
			sinang += sinangi; sinangi += sinang*t;
		}

		t = 65536.0/ainc;
		for(y=((ydim+BOXSIZ-1)>>LOGBOXSIZ);y>=0;y--)
		{
			fy0 = (float)(y<<LOGBOXSIZ)-oy;
			x = ((xdim+BOXSIZ-1)>>LOGBOXSIZ);
			aptr = &arbuf[(y<<7)+(x<<1)];
			ttt = fy0 * fy0;
			fx0 = (float)(x<<LOGBOXSIZ)-ox;
			for(;x>=0;x--)
			{
				tt = atan2(fy0,fx0)-ang0; if (tt < 0) tt += PI*2;
				ftol(tt*t,&aptr[0]);
				tt = fx0*fx0+ttt; fsqrtasm(&tt,&tt); ftol(tt*65536.0,&aptr[1]);
				if (ifor.y < 0) aptr[1] = -aptr[1];
				fx0 -= BOXSIZ;
				aptr -= 2;
			}
		}

		aps = arbuf[0]; rps = arbuf[1];
		for(i=0;i<xboxleng;i++) arplc[i] = arbuf[i+2]-arbuf[i];
		y = 0;
		do
		{
			aptr = &arbuf[(y>>LOGBOXSIZ)<<7];
			api = (aptr[128]-aptr[0])>>LOGBOXSIZ;
			rpi = (aptr[129]-aptr[1])>>LOGBOXSIZ;
			for(i=xboxleng-1;i>=0;i--)
				arinc[i] = (aptr[i+130]-aptr[i+128]-arplc[i])>>LOGBOXSIZ;
			if (klabs(y-badboxline) <= BOXSIZ)
			{
				for(j=min(ydim-y,BOXSIZ);j>0;j--)
				{
					ptr = (char *)(ylookup[y]+frameplace); y++;
					ap = aps; aps += api;
					rp = rps; rps += rpi;

					aptr = &arbuf[(y>>LOGBOXSIZ)<<7];
					fx0 = -ox; fy0 = (float)(y-1)-oy;
					tt = atan2(fy0,fx0)-ang0; if (tt < 0) tt += PI*2;
					ftol(tt*t,&l);

					for(i=0;i<xboxleng;i+=2)
					{
#if (HOLEFIX == 0)
						fx0 += BOXSIZ; k = l;
						tt = atan2(fy0,fx0)-ang0; if (tt < 0) tt += PI*2;
						ftol(tt*t,&l);

							//for(x=0;x<BOXSIZ;x++)   //render16 does this:
							//   *ptr++ = angstart[ap>>16][rp>>16], ap += ai, rp += ri;
						render16(k,(l-k)>>LOGBOXSIZ,rp,arplc[i+1]>>LOGBOXSIZ,ptr);
						ptr += BOXSIZ;
#endif
						ap += arplc[i  ]; arplc[i  ] += arinc[i  ];
						rp += arplc[i+1]; arplc[i+1] += arinc[i+1];
					}

#if (HOLEFIX == 1)
					z = ((long)fy0+(BOXSIZ<<1))*(XDIM+BOXSIZ)*2+(long)fx0+(XDIM+BOXSIZ);
					if (ifor.y > 0)
					{
						l = z+xdim;
						for(i=z;i<l;i++)
						{
							ftol(alookup[i]*t,&k);
							*ptr++ = angstart[k][rlookup[i]];
						}
					}
					else
					{
						l = z+xdim;
						for(i=z;i<l;i++)
						{
							ftol(alookup[i]*t,&k);
							*ptr++ = angstart[k][-rlookup[i]];
						}
					}
#endif
				}
			}
			else
			{
				for(j=min(ydim-y,BOXSIZ);j>0;j--)
				{
					ptr = (char *)(ylookup[y]+frameplace); y++;
					ap = aps; aps += api;
					rp = rps; rps += rpi;
					for(i=0;i<xboxleng;i+=2)
					{
							//for(x=0;x<BOXSIZ;x++)   //render16 does this:
							//   *ptr++ = angstart[ap>>16][rp>>16], ap += ai, rp += ri;
						render16(ap,arplc[i]>>LOGBOXSIZ,rp,arplc[i+1]>>LOGBOXSIZ,ptr);
						ptr += BOXSIZ;
						ap += arplc[i  ]; arplc[i  ] += arinc[i  ];
						rp += arplc[i+1]; arplc[i+1] += arinc[i+1];
					}
				}
			}
		} while (y < ydim);

		/*else
		{
			if (fabs(istr.y) > fabs(ihei.y))
			{
				x = (long)(ihei.y/istr.y*hx*65536.0);
				z = (xdimax>>1)+ydimax;
				//if (istr.y < 0)
				//   { for(i=-(xdimax>>1);i<=z;i+=(1<<16)) gline(0,i-x,xdimax,i+x); }
				//else
				//   { for(i=-(xdimax>>1);i<=z;i+=(1<<16)) gline(xdimax,i+x,0,i-x); }
			}
			else
			{
				x = (long)(istr.y/ihei.y*hy*65536.0);
				z = (ydimax>>1)+xdimax;
				//if (ihei.y < 0)
				//   { for(i=-(ydimax>>1);i<=z;i+=(1<<16)) gline(i-x,0,i+x,ydimax); }
				//else
				//   { for(i=-(ydimax>>1);i<=z;i+=(1<<16)) gline(i+x,ydimax,i-x,0); }
			}
		}*/

		if (vgacompatible)
		{
				//VR flag
			while (!(inp(0x3c2)&128));
			if (kinp(0x3c2)&128)
			{
				koutpw(0x3d4,((long)overtbits<<8)+0x11);
				koutp(0x3d5,overtbits+16);
			}
		}

		i = totalclock-ototalclock; ototalclock += i;
		j = ofinetotalclock-gettimer42(); ofinetotalclock -= j;
		i = (((i*TIMERRATE-(j&65535)+32768)&0xffff0000)+(j&65535));
		if (i) i = 11931810/i;
		frameval[framecnt&(AVERAGEFRAMES-1)] = i; framecnt++;
			//Print MAX FRAME RATE
		i = frameval[0];
		for(j=AVERAGEFRAMES-1;j>0;j--) i = max(i,frameval[j]);
		averagefps = ((averagefps*3+i)>>2);
		sprintf(tempbuf,"%ld.%ld",averagefps/10,averagefps%10);
		printsmall(0L,0L,whitecol,-1L,tempbuf);

		sprintf(tempbuf,"%ld",angnum);
		printsmall(0L,8L,whitecol,-1L,tempbuf);

		printsmall((xdim>>1)-1,(ydim>>1)-2,whitecol,-1L,"+");

		if (usingvesa >= 0)
		{
			setvisualpage(pag);
			pag++; if (pag >= maxpages) pag = 0;
		}

		readmousexy(&mousx,&mousy);
		readmousebstatus(&bstatus);
		if (bstatus&2) { a1 = (a1*.75-(float)mousx*.002); a3 = a3*.75; }
					 else { a3 = (a3*.75+(float)mousx*.002); a1 = a1*.75+istr.y*.02; }
		a2 = (a2*.75-(float)mousy*.002);

		fcossin(&a1,&cosa1,&sina1);
		fcossin(&a2,&cosa2,&sina2);
		fcossin(&a3,&cosa3,&sina3);
		c1c3 = cosa1*cosa3; c1s3 = cosa1*sina3;
		s1c3 = sina1*cosa3; s1s3 = sina1*sina3;
		rr[0] = s1s3*sina2 + c1c3; rr[1] = -c1s3*sina2 + s1c3; rr[2] = sina3*cosa2;
		rr[3] = -cosa2*sina1;      rr[4] = cosa2*cosa1;        rr[5] = sina2;
		rr[6] = s1c3*sina2 - c1s3; rr[7] = -c1c3*sina2 - s1s3; rr[8] = cosa3*cosa2;
		ox = istr.x; oy = ihei.x; oz = ifor.x;
		istr.x = ox*rr[0] + oy*rr[3] + oz*rr[6];
		ihei.x = ox*rr[1] + oy*rr[4] + oz*rr[7];
		ifor.x = ox*rr[2] + oy*rr[5] + oz*rr[8];
		ox = istr.y; oy = ihei.y; oz = ifor.y;
		istr.y = ox*rr[0] + oy*rr[3] + oz*rr[6];
		ihei.y = ox*rr[1] + oy*rr[4] + oz*rr[7];
		ifor.y = ox*rr[2] + oy*rr[5] + oz*rr[8];
		ox = istr.z; oy = ihei.z; oz = ifor.z;
		istr.z = ox*rr[0] + oy*rr[3] + oz*rr[6];
		ihei.z = ox*rr[1] + oy*rr[4] + oz*rr[7];
		ifor.z = ox*rr[2] + oy*rr[5] + oz*rr[8];

		ivel.x = ivel.y = ivel.z = 0; t = synctics*.5;
		if (keystatus[0x2a]) t *= .25;
		if (keystatus[0x36]) t *= 4.0;
		if (keystatus[0xc8]) ivel.x += ifor.x*t, ivel.y += ifor.y*t, ivel.z += ifor.z*t;
		if (keystatus[0xd0]) ivel.x -= ifor.x*t, ivel.y -= ifor.y*t, ivel.z -= ifor.z*t;
		if (keystatus[0xcb]) ivel.x -= istr.x*t, ivel.y -= istr.y*t, ivel.z -= istr.z*t;
		if (keystatus[0xcd]) ivel.x += istr.x*t, ivel.y += istr.y*t, ivel.z += istr.z*t;
		if (keystatus[0x9d]) ivel.x -= ihei.x*t, ivel.y -= ihei.y*t, ivel.z -= ihei.z*t;
		if (keystatus[0x52]) ivel.x += ihei.x*t, ivel.y += ihei.y*t, ivel.z += ihei.z*t;

		clipmove(&ipos,&ivel);
		if (ipos.x < -512) ipos.x += 1024;
		if (ipos.x >= 512) ipos.x -= 1024;
		if (ipos.z < -512) ipos.z += 1024;
		if (ipos.z >= 512) ipos.z -= 1024;

		if (keystatus[0xc9]) //PGUP
		{
			if (hitscan(&ipos,&ifor,&v))
			{
				ftol(v.x,&xx); ftol(v.z,&zz);
				for(z=-64;z<=64;z++)
					for(x=-64;x<=64;x++)
					{
						i = x*x + z*z; if (i >= 64*64) continue;
						i = (cos(sqrt(i)*PI/64)+1)*16;
						j = GROUIND1((x+xx)&1023,(z+zz)&1023);
						grouhei1[j] = max(grouhei1[j]-i,0);
					}
				mipit(xx-64,zz-64,xx+64,zz+64);
			}
		}
		if (keystatus[0xd1]) //PGDN
		{
			if (hitscan(&ipos,&ifor,&v))
			{
				ftol(v.x,&xx); ftol(v.z,&zz);
				for(z=-64;z<=64;z++)
					for(x=-64;x<=64;x++)
					{
						i = x*x + z*z; if (i >= 64*64) continue;
						i = (cos(sqrt(i)*PI/64)+1)*16;
						j = GROUIND1((x+xx)&1023,(z+zz)&1023);
						grouhei1[j] = min(grouhei1[j]+i,255);
					}
				mipit(xx-64,zz-64,xx+64,zz+64);
			}
		}

		synctics = totalclock-lockclock;
		lockclock += synctics;
	}

	uninitkeys();
	uninittimer42();
	uninittimer();

	if (vgacompatible)    //Uninit VR flag
		koutpw(0x3d4,(((long)overtbits+32)<<8)+0x11);

	uninitvesa();
	setvmode(0x3);
	return(0);
}

void inittimer ()
{
	koutp(0x43,0x36); koutp(0x40,TIMERRATE&255); koutp(0x40,TIMERRATE>>8);
	oldtimerhandler = _dos_getvect(0x8);
	_disable(); _dos_setvect(0x8, timerhandler); _enable();
}

void uninittimer ()
{
	koutp(0x43,0x36); koutp(0x40,0); koutp(0x40,0);           //18.2 times/sec
	_disable(); _dos_setvect(0x8, oldtimerhandler); _enable();
}

void __interrupt __far timerhandler()
{
	totalclock++;
	chainintrclock -= TIMERRATE;
	if (chainintrclock < 0)
	{
		chainintrclock += 65536;
		_chain_intr(oldtimerhandler);
	}
	//_chain_intr(oldtimerhandler);
	koutp(0x20,0x20);
}

void initkeys ()
{
	long i;

	for(i=0;i<256;i++) keystatus[i] = 0;
	oldkeyhandler = _dos_getvect(0x9);
	_disable(); _dos_setvect(0x9, keyhandler); _enable();
}

void uninitkeys ()
{
	short *ptr;

	_dos_setvect(0x9, oldkeyhandler);
		//Turn off shifts to prevent stucks with quitting
	ptr = (short *)0x417; *ptr &= ~0x030f;
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
		exit(1);
	}
}

#define loadatabyte(dat,datcnt,fil,tempbuf)                      \
{                                                                \
	dat = tempbuf[datcnt++];                                      \
	if (datcnt == 16384) { read(fil,tempbuf,16384); datcnt = 0; } \
}

int loadata (char *filename)
{
	long i, j, p, datcnt, leng, fil;
	char dat;

	fil = open(filename,O_BINARY|O_RDWR,S_IREAD); if (fil == -1) return(0);
	lseek(fil,128,SEEK_SET);
	p = 0; read(fil,tempbuf,16384); datcnt = 0;
	while (p < 1048576)
	{
		loadatabyte(dat,datcnt,fil,tempbuf);
		if (dat >= 192)
		{
			leng = dat-192; loadatabyte(dat,datcnt,fil,tempbuf);
			while (leng-- > 0) { groucol1[GROUIND1(p&1023,p>>10)] = dat; p++; }
		}
		else { groucol1[GROUIND1(p&1023,p>>10)] = dat; p++; }
	}

	loadatabyte(dat,datcnt,fil,tempbuf);
	if (dat == 0xc)
		for(i=0;i<256;i++)
			for(j=0;j<3;j++)
			{
				loadatabyte(dat,datcnt,fil,tempbuf);
				palette[i][j] = (dat>>2);
			}
	close(fil);

	filename[0] = 'd';
	fil = open(filename,O_BINARY|O_RDWR,S_IREAD); if (fil == -1) return(0);
	lseek(fil,128,SEEK_SET);
	p = 0; read(fil,tempbuf,16384), datcnt = 0;
	while (p < 1048576)
	{
		loadatabyte(dat,datcnt,fil,tempbuf);
		if (dat >= 192)
		{
			leng = dat-192; loadatabyte(dat,datcnt,fil,tempbuf);
			while (leng-- > 0) { grouhei1[GROUIND1(p&1023,p>>10)] = 255-dat; p++; }
		}
		else { grouhei1[GROUIND1(p&1023,p>>10)] = 255-dat; p++; }
	}
	close(fil);
	return(1);
}

	//Can update any rectangle (supports wrap-around!) (0,0,0,0) is entire board)
void mipit (long x1, long y1, long x2, long y2)
{
	long x, y, i, j, r, g, b;

	x1 &= (GROUSIZ-1); y1 &= (GROUSIZ-1);
	x2 &= (GROUSIZ-1); y2 &= (GROUSIZ-1);

	r = g = b = 0;
	y = (y1>>1);
	do
	{
		x = (x1>>1);
		do
		{
			i = GROUIND1(x<<1,y<<1); j = GROUIND2(x,y);
			grouhei2[j] = ((grouhei1[i]+grouhei1[i+1]+grouhei1[i+8]+grouhei1[i+9]+2)>>2);
			r += ((palette[groucol1[i]][0] + palette[groucol1[i+1]][0] + palette[groucol1[i+8]][0] + palette[groucol1[i+9]][0]+2)>>2);
			g += ((palette[groucol1[i]][1] + palette[groucol1[i+1]][1] + palette[groucol1[i+8]][1] + palette[groucol1[i+9]][1]+2)>>2);
			b += ((palette[groucol1[i]][2] + palette[groucol1[i+1]][2] + palette[groucol1[i+8]][2] + palette[groucol1[i+9]][2]+2)>>2);
			r = min(max(r,0),63);
			g = min(max(g,0),63);
			b = min(max(b,0),63);
			groucol2[j] = closestcol[(r<<12)+(g<<6)+b];
			r -= palette[groucol2[j]][0];
			g -= palette[groucol2[j]][1];
			b -= palette[groucol2[j]][2];

			x++; x &= ((GROUSIZ>>1)-1);
		} while (x != (x2>>1));
		y++; y &= ((GROUSIZ>>1)-1);
	} while (y != (y2>>1));

	r = g = b = 0;
	y = (y1>>2);
	do
	{
		x = (x1>>2);
		do
		{
			i = GROUIND2(x<<1,y<<1); j = GROUIND3(x,y);
			grouhei3[j] = ((grouhei2[i]+grouhei2[i+1]+grouhei2[i+8]+grouhei2[i+9]+2)>>2);
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

			x++; x &= ((GROUSIZ>>2)-1);
		} while (x != (x2>>2));
		y++; y &= ((GROUSIZ>>2)-1);
	} while (y != (y2>>2));
}

void printsmall (long xpos, long ypos, long forcol, long baccol, char *bufptr)
{
	long i, x, y;
	char *ptr;

	ptr = (char *)(frameplace+ylookup[ypos]+xpos);
	while (*bufptr)
	{
		i = smallfnt[*bufptr++];
		for(y=5;y>=0;y--)
		{
			if (i&8) ptr[ylookup[y]  ] = forcol;
			if (i&4) ptr[ylookup[y]+1] = forcol;
			if (i&2) ptr[ylookup[y]+2] = forcol;
			if (i&1) ptr[ylookup[y]+3] = forcol;
			i >>= 4;
		}
		if (baccol >= 0)
			for(y=5;y>=0;y--)
			{
				if (!(i&8)) ptr[ylookup[y]  ] = baccol;
				if (!(i&4)) ptr[ylookup[y]+1] = baccol;
				if (!(i&2)) ptr[ylookup[y]+2] = baccol;
				if (!(i&1)) ptr[ylookup[y]+3] = baccol;
				i >>= 4;
			}
		ptr += 4;
	}
}

void gline (long cnt, float x0, float y0, float x1, float y1)
{
	float vx0, vy0, vz0, vx1, vy1, vz1, vd0, vx, vz;
	long j, ji, k, ki, ui, vi;

	if (cnt <= 0) return;
	vx0 = x0*istr.x + y0*ihei.x + gvx;
	vy0 = x0*istr.y + y0*ihei.y + gvy;
	vz0 = x0*istr.z + y0*ihei.z + gvz;
	vx1 = x1*istr.x + y1*ihei.x + gvx;
	vy1 = x1*istr.y + y1*ihei.y + gvy;
	vz1 = x1*istr.z + y1*ihei.z + gvz;
	if (ifor.y < 0) { vz = vx0*vx0+vz0*vz0; fsqrtasm(&vz,&vz); frecipasm(&vz,&vz); vz *= 4194304.0; vx = vx0*vz; vz *= vz0; }
				  else { vz = vx1*vx1+vz1*vz1; fsqrtasm(&vz,&vz); frecipasm(&vz,&vz); vz *= 4194304.0; vx = vx1*vz; vz *= vz1; }
	vd0 = vx*vx0 + vz*vz0;
	ftol(vx,&ui);
	ftol(vz,&vi);
	ftol((vx*vx1+vz*vz1-vd0)*cntrecip[cnt],&ji); ftol(vd0,&j);
	ftol((vy1-vy0)*4194304.0*cntrecip[cnt],&ki); ftol(vy0*4194304.0,&k);

	radarindex += cnt;
	setupgrou6d((ui&0xfe000000)+((ui>>10)&0x00007fff)+0x01ff8000,
					vi,(long)radarindex,gzoff,ji,-ki);
	grou6d(j,-k,gzoff+0,gstartv,-cnt,gstartu);
}

void clipmove (point3d *dp, point3d *dv)
{
	point3d v;
	float t, tinc;
	long x, y, z, ys, ye, px, py, pz;

	tinc = sqrt(dv->x*dv->x + dv->y*dv->y + dv->z*dv->z); if (tinc < 1) tinc = 1;
	for(t=tinc;t>0;t--)
	{
		dp->x += dv->x/tinc; dp->y += dv->y/tinc; dp->z += dv->z/tinc;
		/*ftol(dp->x,&px); ftol(dp->y,&py); ftol(dp->z,&pz);
		v.x = v.y = v.z = 0;
		for(z=-CLIPRAD;z<=CLIPRAD;z++)
			for(x=-CLIPRAD;x<=CLIPRAD;x++)
			{
				ye = cliphei[x+CLIPRAD][z+CLIPRAD]; if (ye <= 0) continue;
				ys = grouhei1[GROUIND1((px+x)&1023,(pz+z)&1023)]-py;
				for(y=max(ys,-ye);y<ye;y++) { v.x -= x; v.y -= y; v.z -= z; }
			}
		dp->x += v.x*.01; dp->y += v.y*.01; dp->z += v.z*.01;*/
	}
	ftol(dp->x,&px); ftol(dp->y,&py); ftol(dp->z,&pz);
	y = grouhei1[GROUIND1(px&1023,pz&1023)]-4;
	if (dp->y > y) dp->y = y;
	if (dp->y < -512) dp->y = -512;
}

long hitscan (point3d *dp, point3d *dv, point3d *ret)
{
	float f;
	long x, y, z, xi, yi, zi, cnt;

	f = dv->x*dv->x + dv->y*dv->y + dv->z*dv->z; if (!f) return(0);
	f = 4194304.0 / sqrt(f);
	ftol(dv->x*f,&xi); ftol(dv->y*f,&yi); ftol(dv->z*f,&zi);
	f = 4194304.0;
	ftol(dp->x*f,&x); ftol(dp->y*f,&y); ftol(dp->z*f,&z);

	for(cnt=1024;cnt>0;cnt--)
	{
		if (grouhei1[GROUIND1(((unsigned)x)>>22,((unsigned)z)>>22)] < (y>>22))
		{
			f = (1.0/4194304.0);
			ret->x = ((float)x)*f; ret->y = ((float)y)*f; ret->z = ((float)z)*f;
			return(1);
		}
		x += xi; y += yi; z += zi;
	}
	return(0);
}
