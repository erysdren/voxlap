//STEEP2.C by Ken Silverman (http://advsys.net/ken)
#include <dos.h>
#include <math.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include "ves2.h"
#include "colook.h"

#define MAXXDIM 640
#define MAXYDIM 480
#define LOGROUSIZ 10             //Must change in STP2.ASM also!
#define GROUSIZ (1<<LOGROUSIZ)
#define PI 3.141592653589793

typedef struct { float x, y; } point2d;
typedef struct { float x, y, z; } point3d;

	//Opticast global variables:
	//radar: 320x200 requires  419560*2 bytes (area * 6.56*2)
	//radar: 400x300 requires  751836*2 bytes (area * 6.27*2)
	//radar: 640x480 requires 1917568*2 bytes (area * 6.24*2)
#define PREC (256*4096)
#define CMPPREC (1024.0)
static short *radar = 0, *radarmem = 0, *angstart[MAXXDIM*4];
static short *gscanptr;
#define CMPRECIPSIZ MAXXDIM+32
static float cmprecip[CMPRECIPSIZ], wx0, wy0, wx1, wy1;
static long lastx[max(MAXYDIM,GROUSIZ)], uurendmem[MAXXDIM*2+8], *uurend;

point3d gipos, gistr, gihei, gifor;
short spal[256];
static float gihx, gihy, gihz, grd, gzoff, ganginc, granginc;
static long giforzsgn, gstartu, gstartv;
static float hx, hy, hz, gvx, gvy, gvz;
static long ylookup[MAXYDIM], palette[256][4];
static char tempbuf[16384];

	//Parallaxing sky variables:
extern void kpzload (const char *, long *, long *, long *, long *);
long skypic = 0, skybpl, skyysiz, skycurlng, skycurdir;
float skylngmul;
point2d *skylng = 0;
	//Parallaxing sky variables (accessed by assembly code)
long skyoff = 0, skyxsiz, *skylat = 0;

#define GROUIND1(x,y) ((((x)&~7)<<(LOGROUSIZ  ))+((y)<<3)+((x)&7))
char grouhei1[GROUSIZ*GROUSIZ];     //Height                       (1048576)
char groucol1[GROUSIZ*GROUSIZ];     //Top color                    (1048576)
long grouptr1[GROUSIZ*GROUSIZ];     //Pointer to additional colors (4194304)
#define GROUIND2(x,y) ((((x)&~7)<<(LOGROUSIZ-1))+((y)<<3)+((x)&7))
char grouhei2[GROUSIZ*GROUSIZ>>2];  //Height                       ( 262144)
char groucol2[GROUSIZ*GROUSIZ>>2];  //Top color                    ( 262144)
long grouptr2[GROUSIZ*GROUSIZ>>2];  //Pointer to additional colors (1048576)
#define GROUIND3(x,y) ((((x)&~7)<<(LOGROUSIZ-2))+((y)<<3)+((x)&7))
char grouhei3[GROUSIZ*GROUSIZ>>4];  //Height                       (  65536)
char groucol3[GROUSIZ*GROUSIZ>>4];  //Top color                    (  65536)
long grouptr3[GROUSIZ*GROUSIZ>>4];  //Pointer to additional colors ( 262144)
char groucol[GROUSIZ*GROUSIZ*2];    //Data for bottom colors       (2097152)
long groucnt;                       //Size of groucol array

#define TIMERRATE (9942>>1)   //240 times/sec
volatile long totalclock, lockclock, synctics, chainintrclock = 0;
static void (__interrupt __far *oldtimerhandler)();
static void __interrupt __far timerhandler(void);

volatile char keystatus[256], readch, oldreadch, extended, keytemp;
static void (__interrupt __far *oldkeyhandler)();
static void __interrupt __far keyhandler(void);

	//FPS variables
long ofinetotalclock, ototalclock, averagefps;
#define AVERAGEFRAMES 32
long frameval[AVERAGEFRAMES], framecnt = 0;

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

long setupmouse ();
#pragma aux setupmouse =\
	"mov ax, 0"\
	"int 33h"\
	"and eax, 0x0000ffff"\
	modify exact [eax ebx]

void readmousexy (short *, short *);
#pragma aux readmousexy =\
	"mov ax, 11d"\
	"int 33h"\
	"mov [esi], cx"\
	"mov [edi], dx"\
	parm [esi][edi] modify exact [eax ebx ecx edx]

void readmousebstatus (short *);
#pragma aux readmousebstatus =\
	"mov ax, 5d"\
	"int 33h"\
	"mov [esi], ax"\
	parm [esi] modify exact [eax ebx ecx edx]

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

extern void setupgrou6d(long,long,long,long,long,long);
#pragma aux setupgrou6d parm [eax][ebx][ecx][edx][esi][edi];
extern void grou6d(long,long,long,long,long,long);
#pragma aux grou6d parm [eax][ebx][ecx][edx][esi][edi];

//ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
//³ Rounding: D11-10:  ³ Precision: D9-8:   ³
//ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
//³ 00 = near/even (0) ³ 00 = 24-bit    (0) ³
//³ 01 = -inf      (4) ³ 01 = reserved  (1) ³
//³ 10 = +inf      (8) ³ 10 = 53-bit    (2) ³
//³ 11 = 0         (c) ³ 11 = 64-bit    (3) ³
//ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

long asm;
void fpuinit ();
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

void fcossin (float, float *, float *);
#pragma aux fcossin =\
	"fsincos"\
	"fstp dword ptr [eax]"\
	"fstp dword ptr [ebx]"\
	parm [8087][eax][ebx]

long mulshr16 (long, long);
#pragma aux mulshr16 =\
	"imul edx"\
	"shrd eax, edx, 16"\
	parm nomemory [eax][edx] modify exact [eax edx] value [eax]

long shldiv16 (long, long);
#pragma aux shldiv16 =\
	"mov edx, eax"\
	"shl eax, 16"\
	"sar edx, 16"\
	"idiv ebx"\
	parm nomemory [eax][ebx] modify exact [eax edx] value [eax]

void emms ();
#pragma aux emms =\
	".686"\
	"emms"\
	parm nomemory [] modify exact [] value

static long fsqlookup[4096], fpuasm;
void initfsqrtasm ()
{
	long i;
	float f, s1, s2;

	s1 = 262144.0; s2 = sqrt(2.0)*131072.0;
	for(i=2048;i<4096;i++)
	{
		f = sqrt((float)i);
		fsqlookup[i-2048] = (long)(f*s1)+0xfe800000;
		fsqlookup[i     ] = (long)(f*s2)+0xff000000;
	}
}

void fsqrtasm (float, float *);
#pragma aux fsqrtasm =\
	"lea ebx, [eax+0x41000000]"\
	"shr eax, 10"\
	"and ebx, 0xff000000"\
	"and eax, 0x00003ffc"\
	"shr ebx, 1"\
	"add ebx, fsqlookup[eax]"\
	"mov dword ptr [ecx], ebx"\
	parm [eax][ecx] value modify exact [eax ebx]

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

void __interrupt __far keyhandler ()
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

char colorjitter (char i)
{
	long r, g, b;

	r = palette[i][0]+((rand()*3)>>15)-1;
	g = palette[i][1]+((rand()*3)>>15)-1;
	b = palette[i][2]+((rand()*3)>>15)-1;
	r = min(max(r,0),63);
	g = min(max(g,0),63);
	b = min(max(b,0),63);
	return((char)closestcol[(r<<12)+(g<<6)+b]);
}

void loadata (char *filename)
{
	long i, j, k, l, p, datcnt, leng, fil, h[9];
	char dat;

	filename[0] = 'd';
	fil = open(filename,O_BINARY|O_RDWR,S_IREAD); if (fil == -1) { initboard(); return; }
	lseek(fil,128,SEEK_SET);
	p = 0; read(fil,tempbuf,16384), datcnt = 0;
	while (p < GROUSIZ*GROUSIZ)
	{
		dat = tempbuf[datcnt++]; if (datcnt == 16384) read(fil,tempbuf,16384), datcnt = 0;
		if (dat >= 192) { leng = dat-192; dat = tempbuf[datcnt++]; if (datcnt == 16384) read(fil,tempbuf,16384), datcnt = 0; }
					  else { leng = 1; }
		while (leng-- > 0) { grouhei1[GROUIND1(p&(GROUSIZ-1),p>>LOGROUSIZ)] = 255-dat; p++; }
	}
	close(fil);

	for(i=0;i<256;i++) groucol[i] = i;
	groucnt = 256;

	filename[0] = 'c';
	fil = open(filename,O_BINARY|O_RDWR,S_IREAD); if (fil == -1) { initboard(); return; }
	lseek(fil,128,SEEK_SET);
	p = 0; read(fil,tempbuf,16384); datcnt = 0;
	while (p < GROUSIZ*GROUSIZ)
	{
		dat = tempbuf[datcnt++]; if (datcnt == 16384) read(fil,tempbuf,16384), datcnt = 0;
		if (dat >= 192) { leng = dat-192; dat = tempbuf[datcnt++]; if (datcnt == 16384) read(fil,tempbuf,16384), datcnt = 0; }
					  else { leng = 1; }
		while (leng-- > 0)
		{
			h[0] = (long)grouhei1[GROUIND1( p   &(GROUSIZ-1),  p>>LOGROUSIZ                )];
			h[1] = (long)grouhei1[GROUIND1((p-1)&(GROUSIZ-1),((p>>LOGROUSIZ)-1)&(GROUSIZ-1))];
			h[2] = (long)grouhei1[GROUIND1((p  )&(GROUSIZ-1),((p>>LOGROUSIZ)-1)&(GROUSIZ-1))];
			h[3] = (long)grouhei1[GROUIND1((p+1)&(GROUSIZ-1),((p>>LOGROUSIZ)-1)&(GROUSIZ-1))];
			h[4] = (long)grouhei1[GROUIND1((p-1)&(GROUSIZ-1),((p>>LOGROUSIZ)  )&(GROUSIZ-1))];
			h[5] = (long)grouhei1[GROUIND1((p+1)&(GROUSIZ-1),((p>>LOGROUSIZ)  )&(GROUSIZ-1))];
			h[6] = (long)grouhei1[GROUIND1((p-1)&(GROUSIZ-1),((p>>LOGROUSIZ)+1)&(GROUSIZ-1))];
			h[7] = (long)grouhei1[GROUIND1((p  )&(GROUSIZ-1),((p>>LOGROUSIZ)+1)&(GROUSIZ-1))];
			h[8] = (long)grouhei1[GROUIND1((p+1)&(GROUSIZ-1),((p>>LOGROUSIZ)+1)&(GROUSIZ-1))];

			groucol1[GROUIND1(p&(GROUSIZ-1),p>>LOGROUSIZ)] = dat;

			j = 0;
			for(i=8;i>0;i--)
				if (h[i]-h[0] > j) j = h[i]-h[0];

			if (j <= 2)
				grouptr1[GROUIND1(p&(GROUSIZ-1),p>>LOGROUSIZ)] = dat;
			else
			{
				//for(i=groucnt-j;i>=0;i--)   //TOO SLOW!
				//{
				//   for(k=j-1;k>=0;k--)
				//      if (groucol[i+k] != dat) break; //!= groucol[groucnt+k]) break;
				//   if (k < 0) { grouptr1[GROUIND1(p&(GROUSIZ-1),p>>LOGROUSIZ)] = i; break; }
				//}
				//if (i < 0)
				//{
					grouptr1[GROUIND1(p&(GROUSIZ-1),p>>LOGROUSIZ)] = groucnt;
					for(;j>1;j--) groucol[groucnt++] = dat;
				//}
			}
			p++;
		}
	}

	dat = tempbuf[datcnt++]; if (datcnt == 16384) read(fil,tempbuf,16384), datcnt = 0;
	if (dat == 0xc)
		for(i=0;i<256;i++)
			for(j=0;j<3;j++)
			{
				dat = tempbuf[datcnt++]; if (datcnt == 16384) read(fil,tempbuf,16384), datcnt = 0;
				palette[i][j] = (dat>>2);
			}

	close(fil);
}

	//Can update any rectangle (supports wrap-around!) (0,0,0,0) is entire board)
void mipit (long x1, long y1, long x2, long y2)
{
	long x, y, i, j, r, g, b, h[9];

	x1 = (x1  )&(GROUSIZ-4); y1 = (y1  )&(GROUSIZ-4);
	x2 = (x2+3)&(GROUSIZ-4); y2 = (y2+3)&(GROUSIZ-4);

	r = g = b = 0;
	y = (y1>>1);
	do
	{
		x = (x1>>1);
		do
		{
			i = GROUIND1(x<<1,y<<1); j = GROUIND2(x,y);
			grouhei2[j] = ((grouhei1[i]+grouhei1[i+1]+grouhei1[i+8]+grouhei1[i+8+1]+2)>>3);

			r = ((palette[groucol1[i]][0] + palette[groucol1[i+1]][0] + palette[groucol1[i+8]][0] + palette[groucol1[i+8+1]][0]+2)>>2);
			g = ((palette[groucol1[i]][1] + palette[groucol1[i+1]][1] + palette[groucol1[i+8]][1] + palette[groucol1[i+8+1]][1]+2)>>2);
			b = ((palette[groucol1[i]][2] + palette[groucol1[i+1]][2] + palette[groucol1[i+8]][2] + palette[groucol1[i+8+1]][2]+2)>>2);
			groucol2[j] = closestcol[(min(max(r,0),63)<<12)+(min(max(g,0),63)<<6)+min(max(b,0),63)];
			r -= palette[groucol2[j]][0];
			g -= palette[groucol2[j]][1];
			b -= palette[groucol2[j]][2];

			x++; if (x >= (GROUSIZ>>1)) x = 0;
		} while (x != (x2>>1));
		y++; if (y >= (GROUSIZ>>1)) y = 0;
	} while (y != (y2>>1));

	y = (y1>>1);
	do
	{
		x = (x1>>1);
		do
		{
			h[0] = (long)grouhei2[GROUIND2( x                    , y                    )];
			h[1] = (long)grouhei2[GROUIND2((x-1)&((GROUSIZ>>1)-1),(y-1)&((GROUSIZ>>1)-1))];
			h[2] = (long)grouhei2[GROUIND2( x                    ,(y-1)&((GROUSIZ>>1)-1))];
			h[3] = (long)grouhei2[GROUIND2((x+1)&((GROUSIZ>>1)-1),(y-1)&((GROUSIZ>>1)-1))];
			h[4] = (long)grouhei2[GROUIND2((x-1)&((GROUSIZ>>1)-1), y                    )];
			h[5] = (long)grouhei2[GROUIND2((x+1)&((GROUSIZ>>1)-1), y                    )];
			h[6] = (long)grouhei2[GROUIND2((x-1)&((GROUSIZ>>1)-1),(y+1)&((GROUSIZ>>1)-1))];
			h[7] = (long)grouhei2[GROUIND2( x                    ,(y+1)&((GROUSIZ>>1)-1))];
			h[8] = (long)grouhei2[GROUIND2((x+1)&((GROUSIZ>>1)-1),(y+1)&((GROUSIZ>>1)-1))];

			j = 0;
			for(i=8;i>0;i--)
				if (h[i]-h[0] > j) j = h[i]-h[0];

			i = GROUIND2(x,y);
			grouptr2[i] = groucol2[i];
			if (j > 2)
			{
				grouptr2[i] = groucnt;
				for(;j>1;j--) groucol[groucnt++] = groucol2[i];
			}

			x++; if (x >= (GROUSIZ>>1)) x = 0;
		} while (x != (x2>>1));
		y++; if (y >= (GROUSIZ>>1)) y = 0;
	} while (y != (y2>>1));


	r = g = b = 0;
	y = (y1>>2);
	do
	{
		x = (x1>>2);
		do
		{
			i = GROUIND2(x<<1,y<<1); j = GROUIND3(x,y);
			grouhei3[j] = ((grouhei2[i]+grouhei2[i+1]+grouhei2[i+8]+grouhei2[i+8+1]+2)>>3);

			r = ((palette[groucol2[i]][0] + palette[groucol2[i+1]][0] + palette[groucol2[i+8]][0] + palette[groucol2[i+8+1]][0]+2)>>2);
			g = ((palette[groucol2[i]][1] + palette[groucol2[i+1]][1] + palette[groucol2[i+8]][1] + palette[groucol2[i+8+1]][1]+2)>>2);
			b = ((palette[groucol2[i]][2] + palette[groucol2[i+1]][2] + palette[groucol2[i+8]][2] + palette[groucol2[i+8+1]][2]+2)>>2);
			groucol3[j] = closestcol[(min(max(r,0),63)<<12)+(min(max(g,0),63)<<6)+min(max(b,0),63)];
			r -= palette[groucol3[j]][0];
			g -= palette[groucol3[j]][1];
			b -= palette[groucol3[j]][2];

			x++; if (x >= (GROUSIZ>>2)) x = 0;
		} while (x != (x2>>2));
		y++; if (y >= (GROUSIZ>>2)) y = 0;
	} while (y != (y2>>2));

	y = (y1>>2);
	do
	{
		x = (x1>>2);
		do
		{
			h[0] = (long)grouhei3[GROUIND3( x                    , y                    )];
			h[1] = (long)grouhei3[GROUIND3((x-1)&((GROUSIZ>>2)-1),(y-1)&((GROUSIZ>>2)-1))];
			h[2] = (long)grouhei3[GROUIND3( x                    ,(y-1)&((GROUSIZ>>2)-1))];
			h[3] = (long)grouhei3[GROUIND3((x+1)&((GROUSIZ>>2)-1),(y-1)&((GROUSIZ>>2)-1))];
			h[4] = (long)grouhei3[GROUIND3((x-1)&((GROUSIZ>>2)-1), y                    )];
			h[5] = (long)grouhei3[GROUIND3((x+1)&((GROUSIZ>>2)-1), y                    )];
			h[6] = (long)grouhei3[GROUIND3((x-1)&((GROUSIZ>>2)-1),(y+1)&((GROUSIZ>>2)-1))];
			h[7] = (long)grouhei3[GROUIND3( x                    ,(y+1)&((GROUSIZ>>2)-1))];
			h[8] = (long)grouhei3[GROUIND3((x+1)&((GROUSIZ>>2)-1),(y+1)&((GROUSIZ>>2)-1))];

			j = 0;
			for(i=8;i>0;i--)
				if (h[i]-h[0] > j) j = h[i]-h[0];

			i = GROUIND3(x,y);
			grouptr3[i] = groucol3[i];
			if (j > 2)
			{
				grouptr3[i] = groucnt;
				for(;j>1;j--) groucol[groucnt++] = groucol3[i];
			}

			x++; if (x >= (GROUSIZ>>2)) x = 0;
		} while (x != (x2>>2));
		y++; if (y >= (GROUSIZ>>2)) y = 0;
	} while (y != (y2>>2));
}

void initboard ()
{
	long x, y, z, zz, h, c, grouput;

	for(z=0;z<256;z++) groucol[z] = z;
	groucnt = 256;

	for(y=0;y<GROUSIZ;y++)
		for(x=0;x<GROUSIZ;x++)
		{
			h = 128;
			if ((x < (GROUSIZ>>2)) || (x >= GROUSIZ-(GROUSIZ>>2)))
				if ((y < (GROUSIZ>>2)) || (y >= GROUSIZ-(GROUSIZ>>2)))
					h = 64+labs(((y+(GROUSIZ>>1))&(GROUSIZ-1))-(GROUSIZ>>1));
			if (h != 128)   //top
			{
				c = (((x+32)&63) + ((y+32)&63)) + (rand()&15) + 96;
				if (y < (GROUSIZ>>1)) c -= 8;
			}
			else   //bot
			{
				c = (x+y)/5 + (((x>>2) ^ (y>>2))&31) + (rand()&7) + 40;
				h += (sin(x*PI/8) + cos(y*PI/8)-2)*1.5;
				c += (sin((x+8)*PI/8) + cos((y+8)*PI/8))*8;
			}

			groucol1[GROUIND1(x,y)] = c;
			grouhei1[GROUIND1(x,y)] = h;

			grouput = groucnt;
			if (((x == (GROUSIZ>>2)-1) || (x == GROUSIZ-(GROUSIZ>>2))) && ((y < (GROUSIZ>>2)) || (y >= GROUSIZ-(GROUSIZ>>2))))
			{
				for(z=h+1;z<128;z++)  //sides 0&2
					groucol[groucnt++] = ((z^y)&15) + (rand()&7) + 80;
			}
			else if (((y == (GROUSIZ>>2)-1) || (y == GROUSIZ-(GROUSIZ>>2))) && ((x < (GROUSIZ>>2)) || (x >= GROUSIZ-(GROUSIZ>>2))))
			{
				for(z=h+1;z<128;z++)  //sides 1&3
					groucol[groucnt++] = ((z^x)&15) + (rand()&7) + 112;
			}

			if (groucnt-grouput < 2)
				{ groucnt = grouput; grouput = groucol[grouput]; }
			grouptr1[GROUIND1(x,y)] = grouput;
		}

	for(x=0;x<256;x++)
		for(y=0;y<3;y++)
			palette[x][y] = (x>>2);
}

long loadsky (char *skyfilnam)
{
	long x, y, xoff, yoff;
	float ang, f;

	if (skypic) { free((void *)skypic); skypic = skyoff = 0; }
	xoff = yoff = 0;

	for(x=y=0;skyfilnam[x];x++) if (skyfilnam[x] == '.') y = x;
	if ((x < 5) || (y < x-5)) return(-1);
	kpzload(skyfilnam,&skypic,&skybpl,&skyxsiz,&skyysiz);
	if (!skypic)
	{
		long r, g, b, *p;
			//Load default sky
		skyxsiz = 512; skyysiz = 1; skybpl = skyxsiz*4;
		if (!(skypic = (long)malloc(skyysiz*skybpl))) return(-1);

		p = (long *)skypic; y = skyxsiz*skyxsiz;
		for(x=0;x<=(skyxsiz>>1);x++)
		{
			p[x] = ((((x*1081 - skyxsiz*252)*x)/y + 35)<<16)+
					 ((((x* 950 - skyxsiz*198)*x)/y + 53)<<8)+
					  (((x* 439 - skyxsiz* 21)*x)/y + 98);
		}
		p[skyxsiz-1] = 0x50903c;
		r = ((p[skyxsiz>>1]>>16)&255);
		g = ((p[skyxsiz>>1]>>8)&255);
		b = ((p[skyxsiz>>1])&255);
		for(x=(skyxsiz>>1)+1;x<skyxsiz;x++)
		{
			p[x] = ((((0x50-r)*(x-(skyxsiz>>1)))/(skyxsiz-1-(skyxsiz>>1))+r)<<16)+
					 ((((0x90-g)*(x-(skyxsiz>>1)))/(skyxsiz-1-(skyxsiz>>1))+g)<<8)+
					 ((((0x3c-b)*(x-(skyxsiz>>1)))/(skyxsiz-1-(skyxsiz>>1))+b));
		}
		y = skyxsiz*skyysiz;
		for(x=skyxsiz;x<y;x++) p[x] = p[x-skyxsiz];
		//return(-1);
	}

		//Initialize look-up table for longitudes
	if (skylng) free((void *)skylng);
	if (!(skylng = (point2d *)malloc(skyysiz*8))) return(-1);
	f = PI*2.0 / ((float)skyysiz);
	for(y=skyysiz-1;y>=0;y--)
		fcossin((float)y*f+PI,&skylng[y].x,&skylng[y].y);
	skylngmul = (float)skyysiz/(PI*2);
		//This makes those while loops in gline() not lockup when skyysiz==1
	if (skyysiz == 1) { skylng[0].x = 0; skylng[0].y = 0; }

		//Initialize look-up table for latitudes
	if (skylat) free((void *)skylat);
	if (!(skylat = (long *)malloc(skyxsiz*4+8))) return(-1);
	f = PI*.5 / ((float)skyxsiz);
	for(x=skyxsiz-1;x;x--)
	{
		ang = (float)((x<<1)-skyxsiz)*f;
		ftol(cos(ang)*32767.0,&xoff);
		ftol(sin(ang)*32767.0,&yoff);
		skylat[x] = (xoff<<16)+((-yoff)&65535);
	}
	skylat[0] = 0; //Hack to make sure assembly index never goes < 0
	skyxsiz--; //Hack for assembly code

	return(0);
}

void printsmall (long xpos, long ypos, long forcol, long baccol, char *bufptr)
{
	long i, x, y, ptr;

	ptr = ylookup[ypos]+(xpos<<1)+frameplace;
	while (*bufptr)
	{
		i = smallfnt[*bufptr++];
		for(y=5;y>=0;y--)
		{
			if (i&8) *(short *)(ylookup[y]+ptr  ) = forcol;
			if (i&4) *(short *)(ylookup[y]+ptr+2) = forcol;
			if (i&2) *(short *)(ylookup[y]+ptr+4) = forcol;
			if (i&1) *(short *)(ylookup[y]+ptr+6) = forcol;
			i >>= 4;
		}
		if (baccol >= 0)
			for(y=5;y>=0;y--)
			{
				if (!(i&8)) *(short *)(ylookup[y]+ptr  ) = baccol;
				if (!(i&4)) *(short *)(ylookup[y]+ptr+2) = baccol;
				if (!(i&2)) *(short *)(ylookup[y]+ptr+4) = baccol;
				if (!(i&1)) *(short *)(ylookup[y]+ptr+6) = baccol;
				i >>= 4;
			}
		ptr += 8;
	}
}

void clipmove (point3d *dp, point3d *dv)
{
	point3d v;
	float t, tinc;
	long x, y, z, ys, ye, px, py, pz;

	fsqrtasm(dv->x*dv->x + dv->y*dv->y + dv->z*dv->z,&tinc);
	if (tinc < 1) tinc = 1;
	for(t=tinc;t>0;t--)
	{
		dp->x += dv->x/tinc; dp->y += dv->y/tinc; dp->z += dv->z/tinc;
	}
	ftol(dp->x,&px); ftol(dp->y,&py); ftol(dp->z,&pz);
	z = grouhei1[GROUIND1(px&(GROUSIZ-1),py&(GROUSIZ-1))]-4;
	if (dp->z > z) dp->z = z;
	if (dp->z < -512) dp->z = -512;
}

long hitscan (point3d *dp, point3d *dv, point3d *ret)
{
	float f;
	long x, y, z, xi, yi, zi, cnt;

	f = dv->x*dv->x + dv->y*dv->y + dv->z*dv->z; if (!f) return(0L);
	fsqrtasm(f,&f); f = 4194304.0 / f;
	ftol(dv->x*f,&xi); ftol(dv->y*f,&yi); ftol(dv->z*f,&zi);
	f = 4194304.0;
	ftol(dp->x*f,&x); ftol(dp->y*f,&y); ftol(dp->z*f,&z);

	for(cnt=1024;cnt>0;cnt--)
	{
		if (grouhei1[GROUIND1(((unsigned)x)>>22,((unsigned)y)>>22)] < (z>>22))
		{
			f = (1.0/4194304.0);
			ret->x = ((float)x)*f; ret->y = ((float)y)*f; ret->z = ((float)z)*f;
			return(1L);
		}
		x += xi; y += yi; z += zi;
	}
	return(0L);
}

void setcamera (point3d *ipo, point3d *ist, point3d *ihe, point3d *ifo,
					 float dahx, float dahy, float dahz)
{
	gipos.x = ipo->x; gipos.y = ipo->y; gipos.z = ipo->z;
	gistr.x = ist->x; gistr.y = ist->y; gistr.z = ist->z;
	gihei.x = ihe->x; gihei.y = ihe->y; gihei.z = ihe->z;
	gifor.x = ifo->x; gifor.y = ifo->y; gifor.z = ifo->z;
	gihx = dahx; gihy = dahy; gihz = dahz;
}

void gline (long cnt, float x0, float y0, float x1, float y1)
{
	float vx0, vy0, vz0, vx1, vy1, vz1, vd0, vd1, f;
	long j, ji, k, ki, ui, vi, jz, jiz, z;

	if (cnt <= 0) return;
	vx0 = x0*gistr.x + y0*gihei.x + gvx;
	vy0 = x0*gistr.y + y0*gihei.y + gvy;
	vz0 = x0*gistr.z + y0*gihei.z + gvz;
	vx1 = x1*gistr.x + y1*gihei.x + gvx;
	vy1 = x1*gistr.y + y1*gihei.y + gvy;
	vz1 = x1*gistr.z + y1*gihei.z + gvz;
	fsqrtasm(vx0*vx0 + vy0*vy0,&vd0);
	fsqrtasm(vx1*vx1 + vy1*vy1,&vd1);
	f = (1<<(32-LOGROUSIZ))/vd1; ftol(vx1*f,&ui); ftol(vy1*f,&vi);
	if (giforzsgn >= 0) { f = vd0; vd0 = vd1; vd1 = f;
								 f = vz0; vz0 = vz1; vz1 = f; }
	ftol((vd1-vd0)*cmprecip[cnt],&ji);
	ftol((vz1-vz0)*cmprecip[cnt],&ki);
	vd0 *= CMPPREC; ftol(vd0,&j);
	ftol(vz0*CMPPREC,&k);
	ftol(vd0      *gzoff,&jz);
	ftol((float)ji*gzoff,&jiz);

	if (skypic)
	{
		if (skycurlng < 0)
		{
			ftol((atan2(vy1,vx1)+PI)*skylngmul-.5,&skycurlng);
			if ((unsigned long)skycurlng >= skyysiz)
				skycurlng = ((skyysiz-1)&(j>>31));
		}
		else if (skycurdir > 0)
		{
			z = skycurlng+1; if (z >= skyysiz) z = 0;
			while (skylng[z].x*vy1 > skylng[z].y*vx1)
				{ skycurlng = z++; if (z >= skyysiz) z = 0; }
		}
		else
		{
			while (skylng[skycurlng].x*vy1 < skylng[skycurlng].y*vx1)
				{ skycurlng--; if (skycurlng < 0) skycurlng = skyysiz-1; }
		}
		skyoff = skycurlng*skybpl + skypic;
	}

	setupgrou6d(jz,jiz,gstartu,gstartv,ui,vi);
	grou6d(j,k,ji,((long)gscanptr)+(cnt<<1),ki,-cnt);
}

void hline (float x0, float y0, float x1, float y1, long *ix0, long *ix1)
{
	float dyx;

	dyx = (y1-y0) * grd; //grd = 1/(x1-x0)

		  if (y0 < wy0) ftol((wy0-y0)/dyx+x0,ix0);
	else if (y0 > wy1) ftol((wy1-y0)/dyx+x0,ix0);
	else ftol(x0,ix0);
		  if (y1 < wy0) ftol((wy0-y0)/dyx+x0,ix1);
	else if (y1 > wy1) ftol((wy1-y0)/dyx+x0,ix1);
	else ftol(x1,ix1);
	if ((float)(*ix0) < wx0) (*ix0) = (long)wx0;
	if ((float)(*ix0) > wx1) (*ix0) = (long)wx1; //(*ix1) = min(max(*ix1,wx0),wx1);
	gline(labs((*ix1)-(*ix0)),(float)(*ix0),((*ix0)-x1)*dyx + y1,
									  (float)(*ix1),((*ix1)-x1)*dyx + y1);
}

void vline (float x0, float y0, float x1, float y1, long *iy0, long *iy1)
{
	float dxy;

	dxy = (x1-x0) * grd; //grd = 1/(y1-y0)

		  if (x0 < wx0) ftol((wx0-x0)/dxy+y0,iy0);
	else if (x0 > wx1) ftol((wx1-x0)/dxy+y0,iy0);
	else ftol(y0,iy0);
		  if (x1 < wx0) ftol((wx0-x0)/dxy+y0,iy1);
	else if (x1 > wx1) ftol((wx1-x0)/dxy+y0,iy1);
	else ftol(y1,iy1);
	if ((float)(*iy0) < wy0) (*iy0) = (long)wy0;
	if ((float)(*iy0) > wy1) (*iy0) = (long)wy1;
	gline(labs((*iy1)-(*iy0)),((*iy0)-y1)*dxy + x1,(float)(*iy0),
									  ((*iy1)-y1)*dxy + x1,(float)(*iy1));
}

	//"db 0x0f" "sbb byte ptr angstart[eax*4-64], al"  //prefetchnta

void hrenderasm (long, long, long, long, long);
#pragma aux hrenderasm =\
	"push ebp"\
	"test edi, 2"\
	"jz short beg"\
	"mov eax, edx"\
	"shr eax, 16"\
	"mov eax, dword ptr angstart[eax*4]"\
	"mov ax, word ptr [eax+ebx*2]"\
	"mov [edi], ax"\
	"add edx, esi"\
	"add edi, 2"\
	"cmp edi, ecx"\
	"je endall"\
	"beg: lea eax, [edx+esi]"\
	"shr edx, 16"\
	"mov edx, dword ptr angstart[edx*4]"\
	"movzx ebp, word ptr [edx+ebx*2]"\
	"lea edx, [eax+esi]"\
	"shr eax, 16"\
	"add edi, 2"\
	"cmp edi, ecx"\
	"jae short endearly"\
	"mov eax, dword ptr angstart[eax*4]"\
	"movzx eax, word ptr [eax+ebx*2]"\
	"shl eax, 16"\
	"add eax, ebp"\
	"mov [edi-2], eax"\
	"add edi, 2"\
	"cmp edi, ecx"\
	"jb short beg"\
	"jmp short endall"\
	"endearly: mov word ptr [edi-2], bp"\
	"endall: pop ebp"\
	parm [ebx][ecx][edx][esi][edi] modify exact [eax edx edi]

void hrender (long sx, long sy, long p1, long plc, long incr, long j)
{
	hrenderasm(j,ylookup[sy]+frameplace+(p1<<1),plc,incr,ylookup[sy]+frameplace+(sx<<1));
	//p1 = ylookup[sy]+(p1<<1)+frameplace;
	//sy = ylookup[sy]+(sx<<1)+frameplace;
	//do
	//{
	//   *(short *)sy = angstart[plc>>16][j];
	//   plc += incr; sy += 2;
	//} while (sy != p1);
}

void vrenderasmp (long *, long *, long, long, long);
#pragma aux vrenderasmp =\
	"push ebp"\
	"beg: mov eax, [ebx]"\
	"mov ebp, eax"\
	"shr eax, 16"\
	"mov eax, angstart[eax*4]"\
	"mov ax, word ptr [eax+edx*2]"\
	"mov [edi], ax"\
	"add edi, 2"\
	"add ebp, [ebx+esi*4]"\
	"inc edx"\
	"mov [ebx], ebp"\
	"add ebx, 4"\
	"cmp ebx, ecx"\
	"jb short beg"\
	"pop ebp"\
	parm [ebx][ecx][edx][esi][edi] modify exact [eax ebx edx edi]

void vrenderasmn (long *, long *, long, long, long);
#pragma aux vrenderasmn =\
	"push ebp"\
	"beg: mov eax, [ebx]"\
	"mov ebp, eax"\
	"shr eax, 16"\
	"mov eax, angstart[eax*4]"\
	"mov ax, word ptr [eax+edx*2]"\
	"mov [edi], ax"\
	"add edi, 2"\
	"add ebp, [ebx+esi*4]"\
	"dec edx"\
	"mov [ebx], ebp"\
	"add ebx, 4"\
	"cmp ebx, ecx"\
	"jb short beg"\
	"pop ebp"\
	parm [ebx][ecx][edx][esi][edi] modify exact [eax ebx edx edi]

void vrender (long sx, long sy, long p1, long iplc, long iinc)
{
	long *usx = &uurend[sx], *up1 = &uurend[p1];
	sy = ylookup[sy]+(sx<<1)+frameplace;

	if (iinc > 0) vrenderasmp(usx,up1,iplc,MAXXDIM,sy);
				else vrenderasmn(usx,up1,iplc,MAXXDIM,sy);
	//do
	//{
	//   *(short *)sy = angstart[usx[0]>>16][iplc];
	//   usx[0] += usx[MAXXDIM]; iplc += iinc; sy += 2;
	//   usx++;
	//} while (usx < up1);
}

void opticast ()
{
	float f, ff, cx, cy, fx, fy, gx, gy, x0, y0, x1, y1, x2, y2, x3, y3;
	long i, j, sx, sy, p0, p1, cx16, cy16, kadd, kmul, u, u1, ui;

	if (gifor.z >= 0) giforzsgn = -1; else giforzsgn = 1;

	gvx = -gihx*gistr.x - gihy*gihei.x + gihz*gifor.x;
	gvy = -gihx*gistr.y - gihy*gihei.y + gihz*gifor.y;
	gvz = -gihx*gistr.z - gihy*gihei.z + gihz*gifor.z;
	gstartu = ((long)(gipos.x*64.0))<<(32-6-LOGROUSIZ);
	gstartv = ((long)(gipos.y*64.0))<<(32-6-LOGROUSIZ);
	gzoff = (float)grouhei1[GROUIND1((unsigned)gstartu>>(32-LOGROUSIZ),(unsigned)gstartv>>(32-LOGROUSIZ))]-gipos.z;

	if (gifor.z == 0) f = 32000; else f = gihz/gifor.z;
	f = min(max(f,-32000),32000);
	cx = gistr.z*f + gihx;
	cy = gihei.z*f + gihy;

	wx0 = -ganginc; wx1 = (float)(xres-1)+ganginc;
	wy0 = -ganginc; wy1 = (float)(yres-1)+ganginc;

	fx = wx0-cx; fy = wy0-cy; gx = wx1-cx; gy = wy1-cy;
	x0 = x3 = wx0; y0 = y1 = wy0; x1 = x2 = wx1; y2 = y3 = wy1;
	if (fy < 0)
	{
		if (fx < 0) { f = sqrt(fx*fy); x0 = cx-f; y0 = cy-f; }
		if (gx > 0) { f = sqrt(-gx*fy); x1 = cx+f; y1 = cy-f; }
	}
	if (gy > 0)
	{
		if (gx > 0) { f = sqrt(gx*gy); x2 = cx+f; y2 = cy+f; }
		if (fx < 0) { f = sqrt(-fx*gy); x3 = cx-f; y3 = cy+f; }
	}
	if (x0 > x1) { if (fx < 0) y0 = fx/gx*fy + cy; else y1 = gx/fx*fy + cy; }
	if (y1 > y2) { if (fy < 0) x1 = fy/gy*gx + cx; else x2 = gy/fy*gx + cx; }
	if (x2 < x3) { if (fx < 0) y3 = fx/gx*gy + cy; else y2 = gx/fx*gy + cy; }
	if (y3 < y0) { if (fy < 0) x0 = fy/gy*fx + cx; else x3 = gy/fy*fx + cx; }
		//This makes precision errors cause pixels to overwrite rather than omit
	x0 -= .01; x1 += .01;
	y1 -= .01; y2 += .01;
	x3 -= .01; x2 += .01;
	y0 -= .01; y3 += .01;

	uurend = &uurendmem[((frameplace&4)^(((long)uurendmem)&4))>>2];

	ftol(cx*65536,&cx16);
	ftol(cy*65536,&cy16);

	ftol((x1-x0)*granginc,&j);
	if ((fy < 0) && (j > 0)) //(cx,cy),(x0,wy0),(x1,wy0)
	{
		ff = (x1-x0) / (float)j; grd = 1.0f / (wy0-cy);
		gscanptr = (short *)radar; skycurlng = -1; skycurdir = -giforzsgn;
		for(i=0,f=x0+ff*.5f;i<j;f+=ff,i++)
		{
			vline(cx,cy,f,wy0,&p0,&p1);
			if (giforzsgn < 0) angstart[i] = gscanptr+p0; else angstart[i] = gscanptr-p1;
			gscanptr += labs(p1-p0)+1;
		}

		j <<= 16; f = (float)j / ((x1-x0)*grd); ftol((cx-x0)*grd*f,&kadd);
		ftol(cx-.5f,&p1); p0 = min(max(p1+1,0),xres); p1 = min(max(p1,0),xres);
		ftol(cy-0.50005f,&sy); if (sy >= yres) sy = yres-1;
		ff = (fabs((float)p1-cx)+1)*f/2147483647.0 + cy; //Anti-crash hack
		while ((ff < sy) && (sy >= 0)) sy--;
		if (sy >= 0)
		{
			ftol(f,&kmul); if (giforzsgn < 0) i = -sy; else i = sy;
			for(;sy>=0;sy--,i-=giforzsgn)
			{
				ui = shldiv16(kmul,(sy<<16)-cy16); //ftol(kmul/((float)sy-cy),&ui);
				u = mulshr16((p0<<16)-cx16,ui)+kadd;
				while ((p0 > 0) && (u >= ui)) { u -= ui; p0--; }
				u1 = (p1-p0)*ui + u;
				while ((p1 < xres) && (u1 < j)) { u1 += ui; p1++; }
				if (p0 < p1) hrender(p0,sy,p1,u,ui,i);
			}
			emms();
		}
	}

	ftol((y2-y1)*granginc,&j);
	if ((gx > 0) && (j > 0)) //(cx,cy),(wx1,y1),(wx1,y2)
	{
		ff = (y2-y1) / (float)j; grd = 1.0f / (wx1-cx);
		gscanptr = (short *)radar; skycurlng = -1; skycurdir = -giforzsgn;
		for(i=0,f=y1+ff*.5f;i<j;f+=ff,i++)
		{
			hline(cx,cy,wx1,f,&p0,&p1);
			if (giforzsgn < 0) angstart[i] = gscanptr-p0; else angstart[i] = gscanptr+p1;
			gscanptr += labs(p1-p0)+1;
		}

		j <<= 16; f = (float)j / ((y2-y1)*grd); ftol((cy-y1)*grd*f,&kadd);
		ftol(cy-.5f,&p1); p0 = min(max(p1+1,0),yres); p1 = min(max(p1,0),yres);
		ftol(cx+0.50005f,&sx); if (sx < 0) sx = 0;
		ff = (fabs((float)p1-cy)+1)*f/2147483647.0 + cx; //Anti-crash hack
		while ((ff > sx) && (sx < xres)) sx++;
		if (sx < xres)
		{
			ftol(f,&kmul);
			for(;sx<xres;sx++,i-=giforzsgn)
			{
				ui = shldiv16(kmul,(sx<<16)-cx16); //ftol(kmul/((float)sx-cx),&ui);
				u = mulshr16((p0<<16)-cy16,ui)+kadd;
				while ((p0 > 0) && (u >= ui)) { u -= ui; lastx[--p0] = sx; }
				uurend[sx] = u; uurend[sx+MAXXDIM] = ui; u += (p1-p0)*ui;
				while ((p1 < yres) && (u < j)) { u += ui; lastx[p1++] = sx; }
			}
			if (giforzsgn < 0)
				  { for(sy=p0;sy<p1;sy++) vrender(lastx[sy],sy,xres,lastx[sy],1); }
			else { for(sy=p0;sy<p1;sy++) vrender(lastx[sy],sy,xres,-lastx[sy],-1); }
			emms();
		}
	}

	ftol((x2-x3)*granginc,&j);
	if ((gy > 0) && (j > 0)) //(cx,cy),(x2,wy1),(x3,wy1)
	{
		ff = (x2-x3) / (float)j; grd = 1.0f / (wy1-cy);
		gscanptr = (short *)radar; skycurlng = -1; skycurdir = giforzsgn;
		for(i=0,f=x3+ff*.5f;i<j;f+=ff,i++)
		{
			vline(cx,cy,f,wy1,&p0,&p1);
			if (giforzsgn < 0) angstart[i] = gscanptr-p0; else angstart[i] = gscanptr+p1;
			gscanptr += labs(p1-p0)+1;
		}

		j <<= 16; f = (float)j / ((x2-x3)*grd); ftol((cx-x3)*grd*f,&kadd);
		ftol(cx-.5f,&p1); p0 = min(max(p1+1,0),xres); p1 = min(max(p1,0),xres);
		ftol(cy+0.50005f,&sy); if (sy < 0) sy = 0;
		ff = (fabs((float)p1-cx)+1)*f/2147483647.0 + cy; //Anti-crash hack
		while ((ff > sy) && (sy < yres)) sy++;
		if (giforzsgn < 0) i = sy; else i = -sy;
		if (sy < yres)
		{
			ftol(f,&kmul);
			for(;sy<yres;sy++,i-=giforzsgn)
			{
				ui = shldiv16(kmul,(sy<<16)-cy16); //ftol(kmul/((float)sy-cy),&ui);
				u = mulshr16((p0<<16)-cx16,ui)+kadd;
				while ((p0 > 0) && (u >= ui)) { u -= ui; p0--; }
				u1 = (p1-p0)*ui + u;
				while ((p1 < xres) && (u1 < j)) { u1 += ui; p1++; }
				if (p0 < p1) hrender(p0,sy,p1,u,ui,i);
			}
			emms();
		}
	}

	ftol((y3-y0)*granginc,&j);
	if ((fx < 0) && (j > 0)) //(cx,cy),(wx0,y3),(wx0,y0)
	{
		ff = (y3-y0) / (float)j; grd = 1.0f / (wx0-cx);
		gscanptr = (short *)radar; skycurlng = -1; skycurdir = giforzsgn;
		for(i=0,f=y0+ff*.5f;i<j;f+=ff,i++)
		{
			hline(cx,cy,wx0,f,&p0,&p1);
			if (giforzsgn < 0) angstart[i] = gscanptr+p0; else angstart[i] = gscanptr-p1;
			gscanptr += labs(p1-p0)+1;
		}

		j <<= 16; f = (float)j / ((y3-y0)*grd); ftol((cy-y0)*grd*f,&kadd);
		ftol(cy-.5f,&p1); p0 = min(max(p1+1,0),yres); p1 = min(max(p1,0),yres);
		ftol(cx-0.50005f,&sx); if (sx >= xres) sx = xres-1;
		ff = (fabs((float)p1-cy)+1)*f/2147483647.0 + cx; //Anti-crash hack
		while ((ff < sx) && (sx >= 0)) sx--;
		if (sx >= 0)
		{
			ftol(f,&kmul);
			for(;sx>=0;sx--,i-=giforzsgn)
			{
				ui = shldiv16(kmul,(sx<<16)-cx16);
				u = mulshr16((p0<<16)-cy16,ui)+kadd;
				while ((p0 > 0) && (u >= ui)) { u -= ui; lastx[--p0] = sx; }
				uurend[sx] = u; uurend[sx+MAXXDIM] = ui; u += (p1-p0)*ui;
				while ((p1 < yres) && (u < j)) { u += ui; lastx[p1++] = sx; }
			}
			for(sy=p0;sy<p1;sy++) vrender(0,sy,lastx[sy]+1,0,giforzsgn);
			emms();
		}
	}
}

long initvoxlap ()
{
	long i;

	  //WARNING: xres&yres are local to VOXLAP5.C so don't rely on them here!
	if (!(radarmem = (short *)malloc((MAXXDIM*MAXYDIM*27)>>2))) return(-1);
	radar = (short *)((((long)radarmem)+7)&~7);

		//Lookup table to save 1 divide for gline()
	for(i=1;i<CMPRECIPSIZ;i++) cmprecip[i] = CMPPREC/(float)i;

	ganginc = 2.0; //Higher=faster (1:full,2:half)
	granginc = 1.0 / ganginc;
	return(0);
}

int main (int argc, char **argv)
{
	point3d v, ipos, ivel, istr, ihei, ifor;
	float t, tt, ox, oy, oz, a1 = 0, a2 = 0, a3 = 0, rr[9], fx0, fy0;
	float cosa1, sina1, cosa2, sina2, cosa3, sina3, c1c3, c1s3, s1c3, s1s3;
	long i, j, k, l, x, y, z, xx, yy, zz, r, g, b, usingvesa, pag;
	short mousx, mousy, bstatus;
	char ch, filename[260];

	if (!gotmmx())
		{ printf("Sorry... This program requires MMX\n"); exit(0); }

	x = MAXXDIM; y = MAXYDIM;
	if ((usingvesa = setvesa(x,y,16)) < 0)
		{ setvmode(0x13); xres = 320; yres = 200; bytesperline = 320; frameplace = 0xa0000; }
	hx = (xres>>1); hy = (yres>>1); hz = hx;
	for(i=0;i<yres;i++) ylookup[i] = i*bytesperline;
	if (maxpages > 8) maxpages = 8;

	initfsqrtasm();
	fpuinit(0x00);  //24 bit, round to nearest/even

	initvoxlap();

	if (LOGROUSIZ != 10)
		initboard();
	else
	{
		strcpy(filename,"c1.dta");
		if (argc == 2) filename[1] = (char)(atoi(argv[1])+48);
		loadata(filename);
	}

	initclosestcolorfast(palette);

		//Warning: Must be done after initclosestcolorfast
	for(i=0;i<groucnt;i++) groucol[i] = colorjitter(groucol[i]);
	for(i=0;i<GROUSIZ*GROUSIZ;i++)
	{
		groucol1[i] = colorjitter(groucol1[i]);
		if (grouptr1[i] < 256) grouptr1[i] = colorjitter(grouptr1[i]);
	}

	loadsky("toonsky.jpg");
		//Quantize sky from 32-bit color to 16-bit color :/
	j = skybpl; skybpl >>= 1; r = g = b = 0;
	for(y=0;y<skyysiz;y++)
		for(x=0;x<skyxsiz;x++)
		{
			i = *(long *)(y*j+(x<<2)+skypic);
			r += ((i>>16)&255); r = min(max(r,0),255);
			g += ((i>> 8)&255); g = min(max(g,0),255);
			b += ((i    )&255); b = min(max(b,0),255);
			i = ((r>>(8-  RedMaskSize))<<  RedFieldPosition) +
				 ((g>>(8-GreenMaskSize))<<GreenFieldPosition) +
				 ((b>>(8- BlueMaskSize))<< BlueFieldPosition);
			r &= (255>>RedMaskSize);
			g &= (255>>GreenMaskSize);
			b &= (255>>BlueMaskSize);
			*(short *)(y*skybpl+(x<<1)+skypic) = i;
		}

		//  RedFieldPosition:11  RedMaskSize:5
		//GreenFieldPosition: 5  GreenMaskSize:6
		// BlueFieldPosition: 0  BlueMaskSize:5
	for(i=0;i<256;i++)
		spal[i] = (((palette[i][0]<<2)>>(8-  RedMaskSize))<<  RedFieldPosition)+
					 (((palette[i][1]<<2)>>(8-GreenMaskSize))<<GreenFieldPosition)+
					 (((palette[i][2]<<2)>>(8- BlueMaskSize))<< BlueFieldPosition);

	mipit(0L,0L,0L,0L);

	inittimer();
	inittimer42();
	initkeys();

	ipos.x = 0; ipos.y = 0; ipos.z = 192;
	ivel.x = 0; ivel.y = 0; ivel.z = 0;
	istr.x = -1; istr.y = 0; istr.z = 0;
	ihei.x = 0; ihei.y = 0; ihei.z = 1;
	ifor.x = 0; ifor.y = 1; ifor.z = 0;
	setupmouse();

	pag = 0;
	totalclock = lockclock = synctics = 0;
	while (!keystatus[1])
	{
		if (usingvesa >= 0) setactivepage(pag);

		setcamera(&ipos,&istr,&ihei,&ifor,hx,hy,hz);
		opticast();

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
		printsmall(0L,0L,-1,-1L,tempbuf);
		printsmall((xres>>1)-1,(yres>>1)-2,-1,-1L,"+");

		if (usingvesa >= 0)
		{
			setvisualpage(pag);
			pag++; if (pag >= maxpages) pag = 0;
		}

		readmousexy(&mousx,&mousy);
		readmousebstatus(&bstatus);
		if (bstatus&2) { a1 = (a1*.75-(float)mousx*.002); a3 = a3*.75; }
					 else { a3 = (a3*.75+(float)mousx*.002*hx/hz); a1 = a1*.75+istr.z*.02; }
		a2 = (a2*.75-(float)mousy*.002*hx/hz);

		fcossin(a1,&cosa1,&sina1);
		fcossin(a2,&cosa2,&sina2);
		fcossin(a3,&cosa3,&sina3);
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

		v.x = 0; v.y = 0; v.z = 0;
		if (keystatus[0xc8]) { v.x += ifor.x; v.y += ifor.y; v.z += ifor.z; }
		if (keystatus[0xd0]) { v.x -= ifor.x; v.y -= ifor.y; v.z -= ifor.z; }
		if (keystatus[0xcb]) { v.x -= istr.x; v.y -= istr.y; v.z -= istr.z; }
		if (keystatus[0xcd]) { v.x += istr.x; v.y += istr.y; v.z += istr.z; }
		if (keystatus[0x9d]) { v.x -= ihei.x; v.y -= ihei.y; v.z -= ihei.z; }
		if (keystatus[0x52]) { v.x += ihei.x; v.y += ihei.y; v.z += ihei.z; }
		t = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
		if (t != 0)
		{
			t = (float)synctics*.5/sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
			if (keystatus[0x2a]) t *= .125;
			if (keystatus[0x36]) t *= 8.0;
			ivel.x = v.x*t; ivel.y = v.y*t; ivel.z = v.z*t;
		}
		else
		{
			t = exp((float)synctics*-.03);
			ivel.x *= t; ivel.y *= t; ivel.z *= t;
		}

		clipmove(&ipos,&ivel);
		if (ipos.x < -(GROUSIZ>>1)) ipos.x += GROUSIZ;
		if (ipos.x >= (GROUSIZ>>1)) ipos.x -= GROUSIZ;
		if (ipos.y < -(GROUSIZ>>1)) ipos.y += GROUSIZ;
		if (ipos.y >= (GROUSIZ>>1)) ipos.y -= GROUSIZ;

		ftol(ipos.x,&x); ftol(ipos.y,&y);
		t = (float)(grouhei1[GROUIND1(x&(GROUSIZ-1),y&(GROUSIZ-1))]-4);
		ipos.z = min(max(ipos.z,-512),t);

		if (keystatus[0xc9]) //PGUP
		{
			if (hitscan(&ipos,&ifor,&v))
			{
				ftol(v.x,&xx); ftol(v.y,&yy);
				for(y=-64;y<=64;y++)
					for(x=-64;x<=64;x++)
					{
						i = x*x + y*y; if (i >= 64*64) continue;
						i = (cos(sqrt(i)*PI/64)+1)*16;
						j = GROUIND1((x+xx)&(GROUSIZ-1),(y+yy)&(GROUSIZ-1));
						grouhei1[j] = max((long)grouhei1[j]-i,0);
					}
				mipit(xx-64,yy-64,xx+64,yy+64);
			}
		}
		if (keystatus[0xd1]) //PGDN
		{
			if (hitscan(&ipos,&ifor,&v))
			{
				ftol(v.x,&xx); ftol(v.y,&yy);
				for(y=-64;y<=64;y++)
					for(x=-64;x<=64;x++)
					{
						i = x*x + y*y; if (i >= 64*64) continue;
						i = (cos(sqrt(i)*PI/64)+1)*16;
						j = GROUIND1((x+xx)&(GROUSIZ-1),(y+yy)&(GROUSIZ-1));
						grouhei1[j] = min((long)grouhei1[j]+i,255);
					}
				mipit(xx-64,yy-64,xx+64,yy+64);
			}
		}
		if (keystatus[0x33]) { keystatus[0x33] = 0; ganginc--; if (ganginc < 1) ganginc = 1; granginc = 1.0/ganginc; } //,
		if (keystatus[0x34]) { keystatus[0x34] = 0; ganginc++; if (ganginc > 16) ganginc = 16; granginc = 1.0/ganginc; } //.
		if (keystatus[0xb5]) hz = max(hz/((float)synctics*.005+1.0),hx*.25); //KP/
		if (keystatus[0x37]) hz = min(hz*((float)synctics*.005+1.0),hx*4.0); //KP*
		if (keystatus[0x35]) { hz = hx; ganginc = 2.0; granginc = 1.0/ganginc; }

		synctics = totalclock-lockclock;
		lockclock += synctics;
	}

	uninitkeys();
	uninittimer42();
	uninittimer();

	if (skylng) { free((void *)skylng); skylng = 0; }
	if (skylat) { free((void *)skylat); skylat = 0; }
	if (skypic) { free((void *)skypic); skypic = skyoff = 0; }

	uninitvesa();
	setvmode(0x3);

	for(i=j=0;i<GROUSIZ*GROUSIZ;i++) if (grouptr1[i] >= 256) j++;
	printf("Numptrs = %ld,",j);
	for(i=j=0;i<(GROUSIZ*GROUSIZ>>2);i++) if (grouptr2[i] >= 256) j++;
	printf("%ld,",j);
	for(i=j=0;i<(GROUSIZ*GROUSIZ>>4);i++) if (grouptr3[i] >= 256) j++;
	printf("%ld\n",j);
	printf("Groucnt = %ld\n",groucnt);
	return(0);
}
