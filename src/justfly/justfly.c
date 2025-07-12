#if 0 //To compile, type: "nmake justfly.c"
justfly.exe: justfly.obj ..\voxlap5.obj ..\v5.obj ..\kplib.obj ..\winmain.obj justfly.obj
				 link justfly ..\voxlap5 ..\v5 ..\kplib ..\winmain ddraw.lib dinput.lib dsound.lib dxguid.lib user32.lib gdi32.lib /opt:nowin98 /map
justfly.obj: justfly.c ..\voxlap5.h ..\sysmain.h; cl /c /J /TP justfly.c /Ox /G6fy /MD
..\voxlap5.obj: ..\voxlap5.c ..\voxlap5.h; cl /Fo..\voxlap5.obj /c /J /TP ..\voxlap5.c /Ox /G6fy /MD
..\v5.obj: ..\v5.asm; ml /Fo..\v5.obj /c /coff ..\v5.asm
..\kplib.obj: ..\kplib.c; cl /Fo..\kplib.obj /c /J /TP ..\kplib.c /Ox /G6fy /MD
..\winmain.obj: ..\winmain.cpp ..\sysmain.h; cl /Fo..\winmain.obj /c /J /TP ..\winmain.cpp /Ox /G6fy /MD /DUSEKZ /DZOOM_TEST /FAsc
!if 0
#endif

// Just flying over VXL map
// by Tom Dobrowolski 2003-04-08

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../key.h"
#include "../sysmain.h"
#include "../voxlap5.h"

#define AIRPUMP "data/airpump.wav"
#define BLOWUP "data/blowup.wav"
#define CLINK "data/clink.wav"
#define HITROCK "data/debris.wav"
#define DEBRIS2 "data/plop.wav"
#define LASER "data/airshoot.wav"
//#define LASER "data/laser.wav"
#define LOGOKFA "data\\voxlaphi.kfa"
#define EXPLODEKV6 "data\\explode.kv6"

#define LOGO 0
#define PLANE_SIM 1
#define EXPLOSION 2

	// Rock is hard!:
#define HDIMLOG 4 // = 16
#define HDIMX 64 // = VSID/16
#define HDIMY 64 // = VSID/16
#define HDIMZ 16 // = MAXZDIM/16
static unsigned char hardness[HDIMX*HDIMY*HDIMZ];

static _inline void ftol (float f, long *a)
{
	_asm
	{
		mov eax, a
		fld f
		fistp dword ptr [eax]
	}
}

static _inline void fcossin (float a, float *c, float *s)
{
	_asm
	{
		fld a
		fsincos
		mov eax, c
		fstp dword ptr [eax]
		mov eax, s
		fstp dword ptr [eax]
	}
}

void vecrand (float sc, point3d *a)
{
	float f;

		//UNIFORM spherical randomization (see spherand.c)
	a->z = ((double)(rand()&32767))/16383.5-1.0;
	f = (((double)(rand()&32767))/16383.5-1.0)*PI; a->x = cos(f); a->y = sin(f);
	f = sqrt(1.0 - a->z*a->z)*sc; a->x *= f; a->y *= f; a->z *= sc;
}

void matrand (float sc, point3d *a, point3d *b, point3d *c)
{
	float f;

	vecrand(sc,a);
	vecrand(sc,c);
	b->x = a->y*c->z - a->z*c->y;
	b->y = a->z*c->x - a->x*c->z;
	b->z = a->x*c->y - a->y*c->x;
	f = sc/sqrt(b->x*b->x + b->y*b->y + b->z*b->z);
	b->x *= f; b->y *= f; b->z *= f;

	c->x = a->y*b->z - a->z*b->y;
	c->y = a->z*b->x - a->x*b->z;
	c->z = a->x*b->y - a->y*b->x;

	a->x *= sc; a->y *= sc; a->z *= sc;
}

long isboxempty (long x0, long y0, long z0, long x1, long y1, long z1)
{
	long x, y;
	for(y=y0;y<y1;y++)
		for(x=x0;x<x1;x++)
			if (anyvoxelsolid(x,y,z0,z1)) return(1);
	return(0);
}

long isboxsolid (long x0, long y0, long z0, long x1, long y1, long z1)
{
	long x, y;
	for(y=y0;y<y1;y++)
		for(x=x0;x<x1;x++)
			if (anyvoxelempty(x,y,z0,z1)) return(0);
	return(1);
}

long getboxfloor (long x0, long y0, long z0, long x1, long y1, long z1)
{
	long x, y, z;
	for(z=z0;z<z1;z++)
		for(y=y0;y<y1;y++)
			for(x=x0;x<x1;x++)
				if (isvoxelsolid(x,y,z)) return z;
	return z1;
}


//---------------------------------------------------------------------------
// Fractal noise:

#define ndim 32
#define ndimlog 5
static float noise[ndim][ndim][ndim];
static long rnoise[ndim][ndim][ndim];

static void noisegen()
{
	long i, j, k, t, x, y, z;
	float fx1, fx2, fy1, fy2, fx, fy, fn, f, f2;
	for(x=ndim-1; x>=0; x--)
		for(y=ndim-1; y>=0; y--)
			for(z=ndim-1; z>=0; z--)
				noise[x][y][z] = 0;
	for(i=ndimlog-1; i>=0; i--) 
	{
		j = (ndim>>i);
		for(x=j-1; x>=0; x--)
			for(y=j-1; y>=0; y--)
				for(z=j-1; z>=0; z--)
					rnoise[x<<i][y<<i][z<<i] = rand();
		k = (1<<i);
		j = (~(k-1))&(ndim-1);      
		f = 1.0f/(float)k;
		f2 = (i+1)/(float)ndimlog;
		for(x=ndim-1; x>=0; x--)
			for(y=ndim-1; y>=0; y--)
				for(z=ndim-1; z>=0; z--)
				{
					fx = (x&(k-1))*f;
					fy = (y&(k-1))*f;
					t = rnoise[x&j][( y )&j][z&j]; fx1 = t + (rnoise[(x+k)&j][( y )&j][z&j] - t)*fx;
					t = rnoise[x&j][(y+k)&j][z&j]; fx2 = t + (rnoise[(x+k)&j][(y+k)&j][z&j] - t)*fx;
					fy1 = fx1 + (fx2-fx1)*fy;
					t = rnoise[x&j][( y )&j][(z+k)&j]; fx1 = t + (rnoise[(x+k)&j][( y )&j][(z+k)&j] - t)*fx;
					t = rnoise[x&j][(y+k)&j][(z+k)&j]; fx2 = t + (rnoise[(x+k)&j][(y+k)&j][(z+k)&j] - t)*fx;
					fy2 = fx1 + (fx2-fx1)*fy;
					fn = fy1 + (fy2-fy1)*((z&(k-1))*f);
					fn = fn*(1.0f/16383.5f) - 1;
					noise[x][y][z] += fn*f2;
				}
	}
}

//---------------------------------------------------------------------------


extern "C" char *sptr[VSID*VSID];
extern "C" long gmipnum;

static long flashbrival = 0x060606;

static _inline void colorsub (long *a)
{
	_asm
	{
		mov eax, a
		movd mm0, [eax]
		psubusb mm0, flashbrival
		movd [eax], mm0
	}
}
static _inline void emms () { _asm emms }

void dimbox (long x0, long y0, long z0, long x1, long y1, long z1)
{
	long ceilnum, x, y, z, t;
	long xc, yc, zc;
	float xr, yr, zr, f, f2, f3;
	char *v, *c;

	xc = (x0+x1)/2;
	yc = (y0+y1)/2;
	zc = (z0+z1)/2;
	xr = 4/(float)((x1-x0)*(x1-x0)); 
	yr = 4/(float)((y1-y0)*(y1-y0));
	zr = 4/(float)((z1-z0)*(z1-z0));

	for(x=x0; x<=x1; x++)
		for(y=y0; y<=y1; y++) {
			if ((unsigned long)(x|y) >= VSID) continue;         
			v = sptr[y*VSID+x];
			while (1)
			{
				for (z=z0; z<=z1; z++) 
					if (z <= v[2])
					{
						if (z < v[1]) ; // air
						else { 
							c = &v[(z-v[1])*4+4];
							f2 = sqrt((x-xc)*(x-xc)*xr + (y-yc)*(y-yc)*yr + (z-zc)*(z-zc)*zr);
							f3 = noise[x&(ndim-1)][y&(ndim-1)][z&(ndim-1)];
							f=(1-f2)*36*(max(cos(f2*10+f3*4),0.6)-0.5); if (f<0) f=0; //if ((*(long *)&f)&0x80000000) f=0;
							ftol(f,&flashbrival); 
							flashbrival *= 0x010101; colorsub((long *)c); emms();
							//if (c[3]>flashbrival) c[3]-=flashbrival; else c[3]=0;
							//c[3]=0;
						}
					}
				ceilnum = v[2]-v[1]-v[0]+2;

				if (!v[0]) break;
				v += v[0]*4;

				for (z=z0; z<=z1; z++) 
					if (z < v[3])
					{
						if (z-v[3] < ceilnum) ; // solid
						else { 
							c = &v[(z-v[3])*4];
							f2 = sqrt((x-xc)*(x-xc)*xr + (y-yc)*(y-yc)*yr + (z-zc)*(z-zc)*zr);
							f3 = noise[x&(ndim-1)][y&(ndim-1)][z&(ndim-1)];
							f=(1-f2)*36*(max(cos(f2*10+f3*4),0.6)-0.5); if (f<0) f=0; //if ((*(long *)&f)&0x80000000) f=0;
							ftol(f,&flashbrival); 
							flashbrival *= 0x010101; colorsub((long *)c); emms();
							//if (c[3]>flashbrival) c[3]-=flashbrival; else c[3]=0;
							//c[3]=0;
						}
					}
			}
		}
	updatebbox(x0,y0,z0,x1,y1,z1,0);
}

long capturecount = 0;

	//Mouse button state global variables:
long obstatus = 0, bstatus = 0;

	//Timer global variables:
double odtotclk, dtotclk;
float fsynctics;
long totclk;

	//FPS counter
long fpsometer[256], numframes = 0, showfps = 0;

	//FPS variables (integrated with timer handler)
#define AVERAGEFRAMES 32
long frameval[AVERAGEFRAMES], averagefps = 0;

point3d lastbulpos, lastbulvel; //hack to remember last bullet exploded

long rubble[] =
{
0x5111111,0x5121111,0x5131111,0x6111111,0x6121111,0x6131111,0x7111111,0x7121011,0x8111010,0x8121010,0x9111010,0x9121010,0xc101212,0xc111113,0xc121112,0xd101314,0xd111215,0xd121214,0xd131213,0xe101517,0xe111317,0xe121317,0xe131315,0xf0f1719,0xf10151a,0xf11141a,0xf121319,0xf131317,0xf141414,0x100e181c,0x100f171c,0x1010151c,0x1011131c,0x1012131b,0x10131319,0x10141315,0x10151314,0x110c1b1c,0x110d191c,0x110e181c,0x110f171c,0x1110151c,0x1111121c,0x11120a0a,0x1112121c,0x1113090b,0x1113111c,0x1114080b,0x11141216,0x1115080a,0x11151214,0x11160809,0x11161213,0x11171313,0x120b1b1c,0x120c1a1c,0x120d191c,0x120e181c,0x120f080c,0x120f161c,0x1210070e,0x1210151c,0x1211070f,0x1211141c,0x12120610,0x1212131c,0x1213060e,0x1213141c,0x1214060d,0x12141415,0x1214191b,0x1215060c,0x1216070b,0x1217080a,0x12171212,0x12181112,0x12191212,0x130b1b1c,0x130c1a1c,0x130d0a0b,0x130d181c,0x130e090c,0x130e171c,0x130f070d,0x130f151c,0x1310070e,0x1310141c,0x1311060e,0x1311141c,0x1312060e,0x1312141d,0x1313050e,0x1313161d,0x1314050d,0x1314191d,0x1315060c,0x1316060b,0x1317070b,0x13180809,0x140a1b1b,0x140b1a1c,0x140c191c,0x140d0a0b,0x140d171c,0x140e080c,0x140e161c,0x140f070d,0x140f141c,0x1410060e,0x1410131d,0x1411060e,0x1411141d,0x1412050e,0x1412151d,0x1413050d,0x1413161d,0x1414050d,0x1414181d,0x1415050c,0x1416060b,0x1417060b,0x1418080a,0x150a1a1b,0x150b191c,0x150c171c,0x150d090b,0x150d151c,0x150e080c,0x150e141c,0x150f070d,0x150f131c,0x1510060d,0x1510121d,0x1511050e,0x1511131d,0x1512050d,0x1512141d,0x1513050d,0x1513161d,0x1514050c,0x1514181d,0x1515050c,0x15151c1d,0x1516060b,0x1517060b,0x1518070a,0x15190909,0x16091a1b,0x160a191b,0x160b171c,0x160c151c,0x160d090a,0x160d141c,0x160e080c,0x160e131c,0x160f070d,0x160f121d,0x1610060d,0x1610111d,0x1611050e,0x1611121d,0x1612050d,0x1612131d,0x1613050d,0x1613151d,0x1614050c,0x1614171d,0x1615050c,0x16151b1d,0x1616060b,0x1617060b,0x1618070a,0x1709191b,0x170a171b,0x170b161c,0x170c141c,0x170d090a,0x170d131c,0x170e080c,0x170e111c,0x170f070d,0x170f101d,0x1710061d,0x1711060e,0x1711101d,0x1712050d,0x1712121d,0x1713050d,
0x1713141d,0x1714050c,0x1714171d,0x1715050b,0x17151a1d,0x1716060b,0x1717060a,0x1718080a,0x1808191a,0x1809171b,0x180a161b,0x180b141c,0x180c131c,0x180d0a0a,0x180d111c,0x180e090c,0x180e101c,0x180f081c,0x1810071d,0x1811061d,0x1812060e,0x1812111d,0x1813050d,0x1813131d,0x1814060c,0x1814151d,0x1815060b,0x18151a1d,0x1816060b,0x18161d1d,0x1817070a,0x1818080a,0x19071919,0x1908181a,0x1909161b,0x190a141b,0x190b131b,0x190c111c,0x190d0a0a,0x190d101c,0x190e0a1c,0x190f081c,0x1910071c,0x1911071d,0x1912061d,0x1913060d,0x1913111d,0x1914060c,0x1914131d,0x1915060b,0x1915191d,0x1916070b,0x19161c1d,0x1917080a,0x1a071819,0x1a08161a,0x1a09151a,0x1a0a131b,0x1a0b111b,0x1a0c101b,0x1a0d0b0b,0x1a0d0e1c,0x1a0e0b1c,0x1a0f0a1c,0x1a10091c,0x1a11081c,0x1a12071c,0x1a13071d,0x1a14070d,0x1a14121d,0x1a15080c,0x1a15171c,0x1a16080b,0x1a161b1c,0x1b071718,0x1b081519,0x1b09131a,0x1b0a121a,0x1b0b101b,0x1b0c0e1b,0x1b0d0c1b,0x1b0e0c1b,0x1b0f0c1b,0x1b100b1b,0x1b110a1c,0x1b12091c,0x1b13091c,0x1b14091c,0x1b150a0c,0x1b15111c,0x1b16191c,0x1c071518,0x1c081419,0x1c091219,0x1c0a101a,0x1c0b0f1a,0x1c0c0d1a,0x1c0d0d1a,0x1c0e0d1a,0x1c0f0e1b,0x1c100e1b,0x1c110e1b,0x1c120e1b,0x1c130f1b,0x1c14101b,0x1c15101b,0x1c16111b,0x1c171a1a,0x1d071416,0x1d081217,0x1d091118,0x1d0a0f19,0x1d0b0e19,0x1d0c0e19,0x1d0d0f19,0x1d0e0f19,0x1d0f0f19,0x1d10101a,0x1d11101a,0x1d12111a,0x1d13111a,0x1d14121a,0x1d15131a,0x1d161319,0x1d171518,0x1e081115,0x1e091016,0x1e0a1017,0x1e0b1018,0x1e0c1018,0x1e0d1118,0x1e0e1118,0x1e0f1218,0x1e101218,0x1e111318,0x1e121418,0x1e131418,0x1e141517,
0x50f1414,0x5101216,0x5111216,0x5121216,0x5131216,0x5141215,0x5151314,0x60e1216,0x60f1217,0x6101117,0x6111217,0x6121217,0x6131217,0x6141217,0x6151216,0x6161215,0x70a1618,0x70b1518,0x70c1319,0x70d1319,0x70e1218,0x70f1118,0x7101118,0x7111218,0x7121218,0x7131118,0x7141118,0x7151117,0x7161216,0x7171215,0x8081619,0x809141a,0x80a141a,0x80b141a,0x80c131a,0x80d131a,0x80e121a,0x80f1119,0x8101119,0x8111119,0x8121119,0x8131119,0x8141118,0x8151118,0x8161117,0x8171216,0x9071519,0x908151a,0x909141b,0x90a141b,0x90b141b,0x90c131b,0x90d131b,0x90e121b,0x90f111a,0x910111a,0x911111a,0x9121119,0x9131019,0x9141119,0x9151118,0x9161118,0x9171217,0x9181315,0xa061618,0xa07151a,0xa08151b,0xa09141c,0xa0a141c,0xa0b141c,0xa0c141c,0xa0d131c,0xa0e121b,0xa0f111b,0xa10111b,0xa11111a,0xa12101a,0xa13101a,0xa141119,0xa151119,0xa161118,0xa171217,0xa181315,0xb061519,0xb07151b,0xb08151c,0xb09141c,0xb0a141c,0xb0b151c,0xb0c161c,0xb0d151c,0xb0e131c,0xb0f121b,0xb10111b,0xb11111b,0xb12111a,0xb13111a,0xb141119,0xb151119,0xb161218,0xb171217,0xb181316,0xc051717,0xc06151a,0xc07151b,0xc08151c,0xc09141c,0xc0a141d,0xc0b171d,0xc0c171d,0xc0d171c,0xc0e161c,0xc0f151c,0xc10131b,0xc11141b,0xc12131a,0xc13111a,0xc14111a,0xc151219,0xc161219,0xc171318,0xc181416,0xd051618,0xd06161a,0xd07151b,0xd08151c,0xd09151c,0xd0a171d,0xd0b181d,0xd0c181d,0xd0d181d,0xd0e171c,0xd0f161c,0xd10151b,0xd11161b,0xd12151b,0xd13141a,0xd14131a,0xd151319,0xd161319,0xd171418,0xd181417,0xd191415,0xe051618,0xe06161a,0xe07151b,0xe08151c,0xe09151c,0xe0a191d,0xe0b191d,0xe0c191d,0xe0d181d,0xe0e181c,0xe0f171c,0xe10181c,0xe11181b,0xe12181b,0xe13161b,0xe14141a,0xe15141a,0xe161419,0xe171419,0xe181418,0xe191416,0xf051617,0xf06161a,0xf07161b,0xf08151c,0xf091a1c,0xf0a1a1c,0xf0b1a1d,0xf0c191d,0xf0d191c,0xf0e181c,0xf0f1a1c,0xf101b1c,0xf111b1b,0xf121a1b,0xf13181b,0xf14151b,0xf15141a,0xf16141a,0xf171419,0xf181419,0xf191418,0xf1a1416,0x10061619,0x10071a1a,0x10081b1b,0x10091b1c,0x100a1b1c,0x100b1b1c,0x100c1a1c,0x100d1a1c,0x10131a1b,0x1014161b,0x1015151b,0x1016141b,0x1017141a,0x1018141a,0x10191419,0x101a1418,0x101b1415,
0x110a1c1c,0x110b1b1c,0x1114171c,0x1115151c,0x1116141b,0x1117141b,0x1118131a,0x1119131a,0x111a1319,0x111b1317,0x12141618,0x12141c1c,0x1215141c,0x1216131c,0x1217131b,0x1218131b,0x1219131a,0x121a1319,0x121b1318,0x121c1316,0x13141618,0x1315151c,0x1316141c,0x1317131c,0x1318131b,0x1319131b,0x131a121a,0x131b1219,0x131c1217,0x14141717,0x1415151d,0x1416141d,0x1417141c,0x1418131c,0x1419131b,0x141a131a,0x141b1219,0x141c1217,0x15141717,0x1515161b,0x1516151d,0x1517141d,0x1518141c,0x1519131b,0x151a131b,0x151b1219,0x151c1218,0x16141616,0x1615151a,0x1616151d,0x1617141d,0x1618141c,0x1619131c,0x161a131b,0x161b1319,0x161c1217,0x17141516,0x17151519,0x1716141d,0x1717141d,0x1718141c,0x1719131b,0x171a131b,0x171b1319,0x171c1317,0x18151419,0x1816141c,0x1817141d,0x1818141c,0x1819131b,0x181a131a,0x181b1319,0x181c1516,0x19151418,0x1916141b,0x1917141c,0x1918131c,0x1919131b,0x191a131a,0x191b1318,0x1a151316,0x1a16131a,0x1a17131c,0x1a18131b,0x1a19131a,0x1a1a1319,0x1b161318,0x1b17131b,0x1b18131a,0x1b191319,0x1b1a1517,0x1c171319,0x1c181319,0x1c191517,
0x6141111,0xb0b1414,0xb0c1315,0xb0d1314,0xc0b1416,0xc0c1316,0xc0d1316,0xc0e1315,0xc0f1314,0xd0a1416,0xd0b1417,0xd0c1317,0xd0d1317,0xd0e1416,0xd0f1415,0xe0a1418,0xe0b1418,0xe0c1318,0xe0d1317,0xe0e1417,0xe0f1416,0xe101414,0xf091519,0xf0a1419,0xf0b1319,0xf0c1318,0xf0d1318,0xf0e1317,0xf0f1416,0xf101414,0x10071619,0x1008151a,0x1009141a,0x100a131a,0x100b121a,0x100c1219,0x100d1219,0x100e1317,0x100f1316,0x10101414,0x11061519,0x1107151a,0x11080e0f,0x1108131b,0x11090e1b,0x110a0e1b,0x110b0f1a,0x110c0f1a,0x110d1018,0x110e1117,0x110f1216,0x11101214,0x12051115,0x12060f18,0x12070e1a,0x12080d1a,0x12090d1b,0x120a0d1b,0x120b0c1a,0x120c0c19,0x120d0d18,0x120e0e17,0x120f0f15,0x12100f14,0x12111013,0x12121112,0x12130f13,0x12140e13,0x12150d13,0x12160c12,0x12170b11,0x12180a0e,0x13050f16,0x13060e18,0x13070d19,0x13080c1a,0x13090c1b,0x130a0c1b,0x130b0c1a,0x130c0b19,0x130d0c17,0x130e0d16,0x130f0e14,0x13100f13,0x13110f13,0x13120f13,0x13130f15,0x13140e15,0x13150d14,0x13160c13,0x13170c12,0x13180a12,0x13190c12,0x131a0f11,0x14040f14,0x14050e16,0x14060d18,0x14070c19,0x14080c1a,0x14090c1b,0x140a0b1a,0x140b0b19,0x140c0a18,0x140d0c16,0x140e0d15,0x140f0e13,0x14100f12,0x14110f13,0x14120f14,0x14130e15,0x14140e16,0x14150d14,0x14160c13,0x14170c13,0x14180b12,0x14190a12,0x141a0e12,0x141b1011,0x15040e15,0x15050d17,0x15060c18,0x15070c19,0x15080b1a,0x15090b1b,0x150a0b19,0x150b0b18,0x150c0a16,0x150d0c14,0x150e0d13,0x150f0e12,0x15100e11,0x15110f12,0x15120e13,0x15130e15,0x15140d16,0x15150d15,0x15160c14,0x15170c13,0x15180b13,0x15190a12,0x151a0e12,0x151b1011,0x16031012,0x16040e15,0x16050d17,0x16060c19,0x16070c1a,0x16080b1a,0x16090b19,0x160a0b18,0x160b0a16,0x160c0a14,0x160d0b13,0x160e0d12,0x160f0e11,0x16100e10,0x16110f11,0x16120e12,0x16130e14,0x16140d15,0x16150d14,0x16160c14,0x16170c13,0x16180b13,0x16190a12,0x161a0f12,0x161b1012,0x17031013,0x17040e15,0x17050d17,0x17060c19,0x17070b1a,0x17080b1a,0x17090b18,0x170a0b16,0x170b0a15,0x170c0a13,0x170d0b12,0x170e0d10,0x170f0e0f,0x17110f0f,0x17120e11,0x17130e13,0x17140d14,0x17150c14,0x17160c13,0x17170b13,0x17180b13,
0x17190a12,0x171a0f12,0x171b1112,0x18031013,0x18040e15,0x18050d17,0x18060c19,0x18070b1a,0x18080b18,0x18090b16,0x180a0b15,0x180b0b13,0x180c0a12,0x180d0b10,0x180e0d0f,0x18120f10,0x18130e12,0x18140d14,0x18150c13,0x18160c13,0x18170b13,0x18180b13,0x18190e12,0x181a1012,0x181b1212,0x19031111,0x19040e15,0x19050d17,0x19060c18,0x19070c18,0x19080b17,0x19090b15,0x190a0b13,0x190b0b12,0x190c0b10,0x190d0b0f,0x19130e10,0x19140d12,0x19150c13,0x19160c13,0x19170b13,0x19180b12,0x19190f12,0x191a1112,0x1a040f14,0x1a050d16,0x1a060d18,0x1a070c17,0x1a080c15,0x1a090b14,0x1a0a0b12,0x1a0b0b10,0x1a0c0b0f,0x1a0d0c0d,0x1a140e11,0x1a150d12,0x1a160c12,0x1a170b12,0x1a180f12,0x1a191112,0x1a1a1212,0x1b041012,0x1b050e15,0x1b060d17,0x1b070d16,0x1b080c14,0x1b090c12,0x1b0a0c11,0x1b0b0c0f,0x1b0c0c0d,0x1b150d10,0x1b160f12,0x1b171012,0x1b181112,0x1b191212,0x1c051013,0x1c060e16,0x1c070e14,0x1c080d13,0x1c090d11,0x1c0a0d0f,0x1c0b0d0e,0x1c171212,0x1d061113,0x1d070f13,0x1d080f11,0x1d090e10,0x1d0a0e0e,
0x60f1111,0x6101010,0x6111010,0x6121010,0x6131010,0x6141010,0x6151111,0x70d1212,0x70e1111,0x70f1010,0x7101010,0x7110f10,0x7120f0f,0x7130f10,0x7140f10,0x7151010,0x7161111,0x80b1313,0x80c1212,0x80d1112,0x80e1011,0x80f0f10,0x8100f10,0x8110f0f,0x8120f0f,0x8130f10,0x8140f10,0x8150f10,0x8161010,0x8171111,0x9081414,0x9091313,0x90a1313,0x90b1213,0x90c1112,0x90d1012,0x90e1011,0x90f0f10,0x9100f10,0x9110e0f,0x9120e0f,0x9130e0f,0x9140e10,0x9150f10,0x9160f10,0x9171111,0xa071414,0xa081314,0xa091213,0xa0a1213,0xa0b1113,0xa0c1113,0xa0d1012,0xa0e0f11,0xa0f0f10,0xa100e10,0xa110e10,0xa120e0f,0xa130e0f,0xa140e10,0xa150f10,0xa160f10,0xa171011,0xa181212,0xb071314,0xb081314,0xb091213,0xb0a1113,0xb0b1113,0xb0c1012,0xb0d0f12,0xb0e0f12,0xb0f0e11,0xb100e10,0xb110e10,0xb120e10,0xb130e10,0xb140e10,0xb150e10,0xb160f11,0xb171011,0xb181112,0xc061414,0xc071314,0xc081214,0xc091113,0xc0a1113,0xc0b1013,0xc0c1012,0xc0d0f12,0xc0e0f12,0xc0f0e12,0xc100e11,0xc110e10,0xc120e10,0xc130e10,0xc140e10,0xc150e11,0xc160f11,0xc171012,0xc181113,0xd061415,0xd071314,0xd081214,0xd091114,0xd0a1013,0xd0b1013,0xd0c0f12,0xd0d0f12,0xd0e0e13,0xd0f0e13,0xd100e12,0xd110d11,0xd120d11,0xd130d11,0xd140e12,0xd150e12,0xd160f12,0xd170f13,0xd181113,0xd191313,0xe061415,0xe071214,0xe081114,0xe091114,0xe0a1013,0xe0b0f13,0xe0c0f12,0xe0d0e12,0xe0e0e13,0xe0f0e13,0xe100d13,0xe110d12,0xe120d12,0xe130d12,0xe140d13,0xe150d13,0xe160e13,0xe170f13,0xe181013,0xe191213,0xf061315,0xf071215,0xf081114,0xf091014,0xf0a0f13,0xf0b0f12,0xf0c0e12,0xf0d0e12,0xf0e0d12,0xf0f0d13,0xf100c13,0xf110c13,0xf120b12,0xf130b12,0xf140b13,0xf150c13,0xf160d13,0xf170e13,0xf180f13,0xf191113,0xf1a1213,0x10061215,0x10071115,0x10081014,0x10090f13,0x100a0e12,0x100b0e11,0x100c0e11,0x100d0d11,0x100e0c12,0x100f0b12,0x10100a13,0x10110912,0x10120912,0x10130812,0x10140912,0x10150912,0x10160a13,0x10170c13,0x10180e13,0x10191013,0x101a1113,0x101b1313,0x11061114,0x11070f14,0x11081012,0x110a0d0d,0x110b0d0e,0x110c0d0e,0x110d0c0f,0x110e0b10,0x110f0a11,0x11100811,0x11110811,0x11120709,0x11120b11,0x11130708,0x11130c10,0x11140707,
0x11140c11,0x11150707,0x11150b11,0x11160a11,0x11170912,0x11180d12,0x11190f12,0x111a1012,0x111b1212,0x120d0b0c,0x120e0a0d,0x120f0d0e,0x12180f10,0x12190e11,0x121a0f12,0x121b1112,0x131b1011,
};
long rubblestart[4] = {0,326,623,899,};
long rubblenum[4] = {326,297,276,217,};

	// Sprites:
#define MAXSPRITES 1024 //NOTE: this shouldn't be static!
#ifdef __cplusplus
struct spritetype : vx5sprite //Note: C++!
{
	point3d v, r;  //other attributes (not used by voxlap engine)
	long owner, tim, tag;
};
#else
typedef struct
{
	point3d p; long flags;
	static union { point3d s, x; }; static union { kv6data *voxnum; kfatype *kfaptr; };
	static union { point3d h, y; }; long kfatim;
	static union { point3d f, z; }; long okfatim;
//----------------------------------------------------
	point3d v, r;  //other attributes (not used by voxlap engine)
	long owner, tim, tag;
} spritetype;
#endif

spritetype spr[MAXSPRITES];
long numsprites = 0;

#if LOGO == 1
		// Logo:
	kv6data *logo_kv6;
	vx5sprite logo_spr;
#endif

kv6data *explode_kv6;

	//Player position variables:
#define NOCLIPRAD 7
#define CLIPRAD 5
#define CLIPVEL 8.0
#define MINVEL 0.5
dpoint3d ipos, istr, ihei, ifor, ivel, ivelang;

	// Laser
#define MAXLASER 1024
typedef struct { point3d p, v; float tim; long hitcnt; long red; } lasertype;
lasertype laser[MAXLASER];
float     lasertim = 0;
float     laserdelay = 0.2;
long      lasercnt = 0;
float     laserang = 0;
long      laserred = 0;
long      laserredenable = 0;

	// Particle text:
#define MAXPTEXT 1024
typedef struct { point3d p, v; float tim, rad; long col; } ptexttype;
ptexttype ptext[MAXPTEXT];
long      ptextcnt = 0;
float     ptexttim = -2;
float     introtim = 0;

	// Powerup - red laser:
long    powerupon = 0;
point3d poweruppos;
float   poweruptim = 0;
float   poweruptim2 = 0;
long    origfogcol;

long    invertmouse = 1;
long    autorun = 0;

	//Lava F-X:
typedef struct { long f, p, x, y; } tiletype;
static tiletype lava;
#define PERIOD 128
static long sint[1024], ps, ops;
static float psf;
#define LWID2 128
#define LWID 1024
static long ground = 1;
static long nlavox = 0;
static long nolight = 0;
static long noclip = 0;
static struct { long x, y, p; } lavox[LWID*LWID];


void add_ptext(char *txt, float dist, float x0, float y0, float rad, long col)
{
	static long buf[256*8];
	long i, j, o;
	float x, y, cx, cy;
	dpoint3d ffor, fstr, fhei;
	
	memset(buf,0,sizeof(buf));
	voxsetframebuffer((long)buf,256*4,256,8);   
	print6x8(0,0,-1,-1,"%s",txt);   

	if (!(col&0x80000000)) {
		ffor = ifor;
		fstr = istr;
		fhei = ihei;
		dorthorotate(0.08f,0,0.1f,&ffor,&fstr,&fhei);
	}

	cx = strlen(txt)*rad*3;
	cy = rad*4 - y0*rad;
	for(o=0,y=-cy,j=7; j>=0; j--,y+=rad)
		for(x=-cx,i=255; i>=0; i--, o++,x+=rad)
			if (buf[o]) {
				if (ptextcnt>=MAXPTEXT) return;
				if (!(col&0x80000000)) {
					ptext[ptextcnt].p.x = ipos.x + ifor.x*dist + fstr.x*x + fhei.x*y + istr.x*x0;
					ptext[ptextcnt].p.y = ipos.y + ifor.y*dist + fstr.y*x + fhei.y*y + istr.y*x0;
					ptext[ptextcnt].p.z = ipos.z + ifor.z*dist + fstr.z*x + fhei.z*y + istr.z*x0;
					ptext[ptextcnt].tim = 9 + rand()*(2/32768.0f);
				} else {
					ptext[ptextcnt].p.x = x + x0;
					ptext[ptextcnt].p.y = y;
					ptext[ptextcnt].p.z = dist;
					ptext[ptextcnt].tim = 4 + rand()*(2/32768.0f);
				}
				//ptext[ptextcnt].h.x = fhei.x;
				//ptext[ptextcnt].h.y = fhei.y;
				//ptext[ptextcnt].h.z = fhei.z;
				vecrand(1,&ptext[ptextcnt].v);            
				ptext[ptextcnt].col = col;
				ptext[ptextcnt].rad = rad*.5f;
				ptextcnt++;
			}
}


void deletesprite (long index)
{
	numsprites--;
	playsoundupdate(&spr[index].p,(point3d *)0);
	playsoundupdate(&spr[numsprites].p,&spr[index].p);
	spr[index] = spr[numsprites];
}

void disable_light()
{
	unsigned char *v;
	long i, j, x, y, k;
		//Disable all intensities
	x = VSID; y = VSID; i = 0;
	for(j=0;j<gmipnum;j++) { i += x*y; x >>= 1; y >>= 1; }

	for(i--;i>=0;i--)
	{
		for(v=(unsigned char *)sptr[i];v[0];v+=v[0]*4)
			for(j=1;j<v[0];j++)
				{ k=(long)&v[j<<2]; *(long *)k=0x80000000|((*(long *)k)&0xFFFFFF); }
		for(j=1;j<=v[2]-v[1]+1;j++) 
				{ k=(long)&v[j<<2]; *(long *)k=0x80000000|((*(long *)k)&0xFFFFFF); }
	}
}

long groucol[9] = {0x506050,0x605848,0x705040,0x804838,0x704030,0x603828,
	0x503020,0x402818,0x302010,};
long mycolfunc (lpoint3d *p)
{
	long i, j;
	j = groucol[(p->z>>5)+1]; i = groucol[p->z>>5];
	i = ((((((j&0xff00ff)-(i&0xff00ff))*(p->z&31))>>5)+i)&0xff00ff) +
		 ((((((j&0x00ff00)-(i&0x00ff00))*(p->z&31))>>5)+i)&0x00ff00);
	i += (labs((p->x&31)-16)<<16)+(labs((p->y&31)-16)<<8)+labs((p->z&31)-16);
	//j = (p->x*513+p->y*123451+p->z*4257)&32767;
	j = rand(); i += (j&7) + ((j&0x70)<<4) + ((j&0x700)<<8);
	return(i+0x80000000);
}

void toggle_noclip()
{
	lpoint3d lp;
	noclip ^= 1; 
	if (!noclip) {
		voxrestore();
	} else {
		lp.x = ipos.x; lp.y = ipos.y; lp.z = ipos.z;
		voxbackup(lp.x-NOCLIPRAD+1, lp.y-NOCLIPRAD+1, lp.x+NOCLIPRAD, lp.y+NOCLIPRAD, 0x10000);
		vx5.colfunc = mycolfunc; vx5.curcol = 0x80704030;
		setsphere(&lp, NOCLIPRAD, -1);
		if (vx5.lightmode) updatevxl();
	} 
}

void prepare_sprite2d(vx5sprite *spr, float cx, float cy, float f)
{
	dpoint3d dpos, dp, dp2, dp3;   
	dpos.x = 0; dpos.y = 0; dpos.z = -2;
	  dp.x = 1;   dp.y = 0;   dp.z = 0;
	 dp2.x = 0;  dp2.y = 1;  dp2.z = 0;
	 dp3.x = 0;  dp3.y = 0;  dp3.z = 1;
	setcamera(&dpos,&dp,&dp2,&dp3,xres*cx,yres*cy,(max(xres,yres))*.5);
	spr->p.x = 0; spr->p.y = 0; spr->p.z = 0;
	spr->s.x = f; spr->s.y = 0; spr->s.z = 0;
	spr->h.x = 0; spr->h.y = 0; spr->h.z = -f;
	spr->f.x = 0; spr->f.y = f; spr->f.z = 0;   
}

long initapp (long argc, char **argv)
{
	long x, y, z, i, j, k, minz, lcol;
	char *level = NULL;
	
	prognam = "Justfly";
	xres = 640; yres = 480; colbits = 32; fullscreen = 1;

	noisegen();
	
	initvoxlap();
	vx5.maxscandist = (long)(VSID*1.42);
	
	kzaddstack("voxdata.zip");

	memset(hardness,3,sizeof(hardness)); // maximum hardness
	
	//fullscreen = 0;
	//level = "voxgen.vxl";
	
	vx5.lightmode = 1; // instead of "-1" option
	vx5.maxscandist = 384;
	vx5.fogcol = 0x808080;
	
	for(i=1; i<argc; i++)
		if (argv[i][0]=='/' || argv[i][0]=='-') {
		 if (argv[i][1]=='l') level=argv[i]+2;
		 else if (argv[i][1]=='w') fullscreen = 0;
		 else if (argv[i][1]=='f') fullscreen = 1;
		 //else if (argv[i][1]=='g') ground = 1;
		 else if (argv[i][1]=='n') nolight = 1;
		 //else if (argv[i][1]=='1') vx5.lightmode = 1;
		 else if (argv[i][1]=='d') {
			 vx5.maxscandist = atoi(argv[i]+2);
			 if (vx5.fogcol < 0) vx5.fogcol = 0x808080;
		 }
		 else if (argv[i][1]=='c') {
			 vx5.fogcol = strtol(argv[i]+2,NULL,0);
		 }
		} else level=argv[i];
	
	if (!level) {
		MessageBox(NULL,
						 "JustFly by Tom Dobrowolski and Ken Silverman\n\r\n\r"
						 "  usage:\n\r"
						 "     justfly <options>\n\r"
						 "  options:\n\r"
						 "     -l[VXL file]    (select level)\n\r"
						 "     -f                 (fullscreen)\n\r"
						 "     -w                (windowed)\n\r"
						 "     -d###        (fog distance)\n\r"
						 "     -c###        (fog color i.e. 0x808080 = grey)\n\r"
						 "     -n                (disable lights)\n\r",
						 "JustFly - command line help",
						 MB_OK);
		return -1;
	}

#if LOGO == 1
	getspr(&logo_spr, LOGOKFA);  
	logo_spr.kfatim = 2000;
#endif
	explode_kv6 = getkv6(EXPLODEKV6);
	
	if (!loadvxl(level,&ipos,&istr,&ihei,&ifor)) return -1;
	if (vx5.lightmode) updatevxl();
	//vx5.lightmode = 1;
	
if (!ground) {
		// Init lava:
	for (x=0; x<1024; x++) sint[x]=sin(x*PI/(float)(PERIOD/2))*6;
	kpzload("png/lava.png", &lava.f, &lava.p, &lava.x, &lava.y);
	//if (!lava.f) kpzload("lava.png", &lava.f, &lava.p, &lava.x, &lava.y);
	if (lava.p != 1024)  { free((void *)lava.f); lava.f = 0; }

		// Find mean lava color:
	x = y = z = 0;
	for(i=255; i>=0; i--) {
		j = *(long *)(lava.f + (i<<2));
		x += (j&255);
		y += ((j>>8)&255);
		z += ((j>>16)&255);
	}
	lcol = ((x>>8)&0xFF) | (y&0xFF00) | ((z<<8)&0xFF0000);
	lcol = ((lcol&0xFEFEFE)>>1);
	//lcol = 0x804000;
	
		// Find lava height (z):   
	ops = -1; ps = 0; psf = 0;
	minz = 161;
	for (y=0; y<LWID2; y++)
		for (x=0; x<LWID2; x++)
			for (z=161; z<255; z++) 
			  if (getcube(x,y,z)&0xfffffffe) { if (z>minz) minz=z; break; }
			  
		// Find lava voxels:
	nlavox = 0;   
	for (i=y=0; y<LWID; y++)
		for (x=0; x<LWID; x++, i++) {
			j = getcube(x,y,minz);
			if (j&0xfffffffe) {
				lavox[nlavox].x = (x*2)&255;
				lavox[nlavox].y = (y*2)&255;
				lavox[nlavox].p = j;
				nlavox++;
			}
			j = getcube(x,y,minz-1);
			if (j&0xfffffffe) {
				k = *(long *)j;
				*(long *)j = (((k&0xFEFEFE)>>1) + lcol) | (k&0xFF000000);
			}
		}
		
	/*
	for (y=0; y<LHEI; y++)
		for (x=0; x<LWID; x++, i++)
			lavox[i] = getcube(x,LWID-1,162+y);
	for (y=0; y<LHEI; y++)
		for (x=0; x<LWID; x++, i++)
			lavox[i] = getcube(LWID-1,x,162+y);
	*/
}

	if (nolight) disable_light();
	
	vx5.fallcheck = 1;
	setsideshades(0,4,1,3,2,2);
	
	//toggle_noclip();
	
	ivel.x = ivel.y = ivel.z = 0;
	ivelang.x = ivelang.y = ivelang.z = 0;
	
	lpoint3d lp, lp2;
	long dist;
	dist = -1;
	lp.x = ipos.x;
	lp.y = ipos.y;
	lp.z = ipos.z;
	lp2 = lp;
	for(i=31;i>=0;i--) {           
		k = 24;
		if (lp.z > dist && !isboxempty(lp.x-k,lp.y-k,lp.z-k,lp.x+k,lp.y+k,lp.z+k)) {
			ipos.x = lp.x;
			ipos.y = lp.y;
			ipos.z = lp.z;
			lp2 = lp;
			dist = lp.z;
		}
		lp.x = rand()*VSID/32768.f;
		lp.y = rand()*VSID/32768.f;
		lp.z = rand()*MAXZDIM/32768.f;
	}
	dist = -1;
	for(i=127;i>=0;i--) {
		k = 16;
		lp.x = (rand()*(VSID-k*4)>>15)+k*2;
		lp.y = (rand()*(VSID-k*4)>>15)+k*2;
		lp.z = (rand()*(MAXZDIM-k*4)>>15)+k*2;
		if (!isboxempty(lp.x-k,lp.y-k,lp.z-k,lp.x+k,lp.y+k,lp.z+k)
		  && isboxempty(lp.x-k,lp.y-k,lp.z+k,lp.x+k,lp.y+k,lp.z+k+8))
		  //&& isboxempty(lp.x-k,lp.y-k,lp.z-k*2,lp.x+k,lp.y+k,lp.z-k)) 
		{
			lp.z = getboxfloor(lp.x-1,lp.y-1,lp.z,lp.x+2,lp.y+2,255) - k;
			j = (lp.x-lp2.x)*(lp.x-lp2.x) + (lp.y-lp2.y)*(lp.y-lp2.y) + (lp.z-lp2.z)*(lp.z-lp2.z);
			if (j>dist) {
				poweruppos.x = lp.x;
				poweruppos.y = lp.y;
				poweruppos.z = lp.z;
				powerupon = 1;
				dist = j;
			}
		}
	}
	if (dist == -1) 
	for(i=127;i>=0;i--) { // last chance!
		k = 16;
		lp.x = (rand()*(VSID-k*4)>>15)+k*2;
		lp.y = (rand()*(VSID-k*4)>>15)+k*2;
		lp.z = (rand()*(MAXZDIM-k*4)>>15)+k*2;
		if (!isboxempty(lp.x-k,lp.y-k,lp.z-k,lp.x+k,lp.y+k,lp.z+k))
		{
			lp.z = getboxfloor(lp.x-1,lp.y-1,lp.z,lp.x+2,lp.y+2,255) - k;         
			poweruppos.x = lp.x;
			poweruppos.y = lp.y;
			poweruppos.z = lp.z;
			powerupon = 1;
			break;
		}
	}
	origfogcol = vx5.fogcol;
	
	// check if camera is not pointing outside level:
	for(i=5;i>=0;i--) {
		lp.x = ipos.x + ifor.x*128;
		lp.y = ipos.y + ifor.y*128;
		if ((unsigned long)(lp.x|lp.y)>=VSID)
			dorthorotate(0,0,-PI*0.6666,&istr,&ihei,&ifor);
		else break;
	}
	return(0);
}

void uninitapp ()
{
	// stop laser sounds:
	long i;
	for(i=lasercnt-1; i>=0; i--) playsoundupdate(&laser[i].p,NULL);
	// free lava:
	if (!ground && lava.f) { free((void *)lava.f); lava.f = 0; } // free lava
	uninitvoxlap();
}
	 
static float fov = .5;

void doframe ()
{
	dpoint3d dp, dp2, dp3, dpos;
	point3d fp, fp2, fp3, fpos;
	lpoint3d lp, lp2, lp3;
	double d;
	float f, f2, fmousx, fmousy;
	long i, j, t, k, l, m, *hind, hdir;
	char tempbuf[260];
	long x, y, px;
	
	long frameptr, pitch, xdim, ydim;
	
	if (startdirectdraw(&frameptr,&pitch,&xdim,&ydim)) {
		voxsetframebuffer(frameptr,pitch,xdim,ydim);
		setcamera(&ipos,&istr,&ihei,&ifor,xres*.5,yres*.5,xres*fov); // .5f
		setears3d(ipos.x,ipos.y,ipos.z,ifor.x,ifor.y,ifor.z,ihei.x,ihei.y,ihei.z);
		
	if (!ground) {
		if (ops != ps) {
			for (i=nlavox-1; i>=0; i--)
			{
				x = lavox[i].x;
				y = lavox[i].y;
				k = *(long *)( lava.f + (((sint[y+ps]+x)&255)<<2) + (((sint[x+ps]+y)&255)<<10) );
				m = *(long *)( lava.f + (((sint[y+ps]+x+1)&255)<<2) + (((sint[x+1+ps]+y)&255)<<10) );
				k = ((k&0xFEFEFE)>>1) + ((m&0xFEFEFE)>>1);
				l = *(long *)( lava.f + (((sint[y+1+ps]+x)&255)<<2) + (((sint[x+ps]+y+1)&255)<<10) );
				m = *(long *)( lava.f + (((sint[y+1+ps]+x+1)&255)<<2) + (((sint[x+1+ps]+y+1)&255)<<10) );
				l = ((l&0xFEFEFE)>>1) + ((m&0xFEFEFE)>>1);
				k = ((k&0xFEFEFE)>>1) + ((l&0xFEFEFE)>>1);
				*(long *)lavox[i].p = ((*(long *)lavox[i].p)&0xFF000000) | k;
			}
			ops = ps;
		}
		psf = psf + fsynctics*32.0f;
		ps = ((long)psf)&(PERIOD-1);
	}
		
		opticast();

#if LOGO == 1
		if (logo_spr.kfatim >= 2000) logo_spr.kfatim = 0;
		animsprite(&logo_spr, fsynctics*1000.0);
		prepare_sprite2d(&logo_spr,0.5,0.5,0.02f);
		orthorotate(totclk*.001f, 0, 0, &logo_spr.s, &logo_spr.h, &logo_spr.f);
		drawsprite(&logo_spr);
		setcamera(&ipos,&istr,&ihei,&ifor,xres*.5,yres*.5,xres*.5);
#endif

			// Draw sprites
		for(i=numsprites-1;i>=0;i--)
			drawsprite(&spr[i]);         

			// Draw lasers:
		f = 4;
		for(i=lasercnt-1; i>=0; i--) {    
			j = 0x7F*laser[i].tim + 0x80;
#if 0
				//Boring lines (original code):
			drawline3d(
				laser[i].p.x - laser[i].v.x*f,
				laser[i].p.y - laser[i].v.y*f,
				laser[i].p.z - laser[i].v.z*f,
				laser[i].p.x + laser[i].v.x*f,
				laser[i].p.y + laser[i].v.y*f,
				laser[i].p.z + laser[i].v.z*f,
				(j<<16)|(j<<8)|0x80);
#else
				//Exciting aerodynamic raindrops!
			if (laser[i].red) {
				for(k=16;k>=0;k--)
				{
					drawspherefill(
						laser[i].v.x*f*(float)(k-8)*.25f + laser[i].p.x,
						laser[i].v.y*f*(float)(k-8)*.25f + laser[i].p.y,
						laser[i].v.z*f*(float)(k-8)*.25f + laser[i].p.z,
						sqrt((float)(16-k))*(float)(k*k)*-.004, //raindrop shape, equation prototyped in evaldraw :)               
						j*0x10000+0x0060);
					j -= 10; if (j < 0) j = 0;
				}         
			} else {
				for(k=16;k>=0;k--)
				{
					drawspherefill(
						laser[i].v.x*f*(float)(k-8)*.125f + laser[i].p.x,
						laser[i].v.y*f*(float)(k-8)*.125f + laser[i].p.y,
						laser[i].v.z*f*(float)(k-8)*.125f + laser[i].p.z,
						sqrt((float)(16-k))*(float)(k*k)*-.002, //raindrop shape, equation prototyped in evaldraw :)               
						j*0x10100+128);
					j -= 10; if (j < 0) j = 0;
				}
			}
#endif         
		}

		if (powerupon) {
			fp.x = poweruppos.x - ipos.x;
			fp.y = poweruppos.y - ipos.y;
			fp.z = poweruppos.z - ipos.z;
			f = sqrt(fp.x*fp.x + fp.y*fp.y + fp.z*fp.z);
			if (f > 8) {
				j = 0x7F*f + 0x80;
				fp2.y=fp2.z=0; fp.x=1;
				f2=fabs(sin(poweruptim))+.5f;
				for(t=32; t>=0;t--) 
				{
					float co2, si2, co3, si3;
					const float A[3] = {1.0f, 0.6f, 1.4f};
					f = t*PI/16.0f;              
					fcossin(f + poweruptim*.33333f, &co2, &si2);
					fcossin(f*3 + poweruptim, &co3, &si3);
					f = (A[0] + A[1]*co3)*f2;
					fp.x = f*co2;
					fp.z = f*si2;
					fp.y = A[2]*si3*f2;
					axisrotate(&fp,&fp2,poweruptim/6.f);
					k = (si2+1)*0x70;
					drawspherefill(
						poweruppos.x + fp.x,
						poweruppos.y + fp.y,
						poweruppos.z + fp.z,
						-(co3*.1f + .3f), 0xFF0000 | (k<<0));
				}
				if ((poweruptim += fsynctics*4) > 12*PI) poweruptim -= 12*PI;
			} else {
				powerupon = 0;
				poweruptim2 = 1;
				playsound(AIRPUMP, 100, 1.5f, &poweruppos, KSND_3D);
				laserred = 1;
				laserredenable = 1;
				ptextcnt = 0;
				add_ptext("Red blaster found!", 10, 0, 0, .1f, 0x80FF8000);
				add_ptext("Enjoy!", 10, 0, 10, .1f, 0x80F0F000);
				voxsetframebuffer(frameptr,pitch,xdim,ydim);
			}
		}
		if (poweruptim2 > 0) {
			f = (1-poweruptim2)*16;
			d = sqrt((16-f)*(16-f)*f)/24.6;
			j = d*250;
			vx5.fogcol = (j*0x010001) | origfogcol;
			poweruptim2 -= fsynctics;         
		} else vx5.fogcol = origfogcol;

			// Draw ptext:
		for(i=ptextcnt-1; i>=0; i--) {
			if (ptext[i].col&0x80000000) f = 4; else f = 8;
			if (ptext[i].tim > f) {            
				f2 = f+1 - ptext[i].tim; if (f2 > 1) f2 = 1;
				f = ptext[i].rad*f2;
				f2 = (1 - f2)*.2f;
				ptext[i].tim -= fsynctics*2;            
			} else if (ptext[i].tim < 1) {
				f = ptext[i].rad*ptext[i].tim;
				f2 = (1 - ptext[i].tim)*.2f;
				if (ptext[i].tim<.8f && ptexttim>-1) { playsound(HITROCK, 50, 0.5f, &ptext[i].p, KSND_3D); ptexttim=-2; }
			} else {
				f = ptext[i].rad;
				f2 = 0;
			}
			f *= (sin(ptext[i].tim*8)*.3f + 1);
			if (f>1e-10) {
				if (ptext[i].col&0x80000000) {
					fp.x = ptext[i].p.x + ptext[i].v.x*f2;
					fp.y = ptext[i].p.y + ptext[i].v.y*f2;
					fp.z = ptext[i].p.z + ptext[i].v.z*f2;
					drawspherefill( ipos.x + fp.z*ifor.x + fp.x*istr.x + fp.y*ihei.x, 
										 ipos.y + fp.z*ifor.y + fp.x*istr.y + fp.y*ihei.y, 
										 ipos.z + fp.z*ifor.z + fp.x*istr.z + fp.y*ihei.z, 
										 -f, ptext[i].col);
				} else {
					drawspherefill( ptext[i].p.x + ptext[i].v.x*f2, 
										 ptext[i].p.y + ptext[i].v.y*f2, 
										 ptext[i].p.z + ptext[i].v.z*f2, 
										 -f, ptext[i].col);
				}
			}
			ptext[i].tim -= fsynctics;
			if (ptext[i].tim <= 0) {
				ptext[i] = ptext[--ptextcnt]; // remove!
			}
		}      

		
			//FPS counter
		fpsometer[numframes&255] = (long)(fsynctics*100000);
		if (showfps)
		{
			for(i=0;i<256;i++) drawpoint2d(i,fpsometer[(numframes+1+i)&255]>>4,0xffffff);
				//Print MIN frame period
			i = fpsometer[0]; for(j=255;j>0;j--) i = min(i,fpsometer[j]);
			print4x6(0,0,0xc0c0c0,-1,"%d.%02dms",i/100,i%100);
			if (i>0) print4x6(0,10,0xc0c0c0,-1,"%d FPS",100000/i);
			print4x6(0,20,0xc0c0c0,-1,"%.2f %.2f %.2f", ipos.x, ipos.y, ipos.z);
		}
		else 
		{
			ftol(1000.0/fsynctics,&i);
			frameval[numframes&(AVERAGEFRAMES-1)] = i;
				//Print MAX FRAME RATE
			i = frameval[0];
			for(j=AVERAGEFRAMES-1;j>0;j--) i = max(i,frameval[j]);
			averagefps = ((averagefps*3+i)>>2);
			print4x6(0,0,0xc0c0c0,-1,"%.1f (%0.2fms)",(float)averagefps*.001,1000000.0/max((float)averagefps,1));
		}
		
		print6x8((xres-(28*6))>>1,yres-6-10,0xc0c0c0,-1,"Cave Demo by Tom Dobrowolski");
		print4x6((xres-(50<<2))>>1,yres-6,0xc0c0c0,-1,"VOXLAP (c) 2003 Ken Silverman (www.advsys.net/ken)");
		
		if (keystatus[0x58]) //F12
		{
			char snotbuf[max(MAX_PATH+1,256)];
			keystatus[0x58] = 0;
			strcpy(snotbuf,"VXL50000.PNG");
			snotbuf[4] = ((capturecount/1000)%10)+48;
			snotbuf[5] = ((capturecount/100)%10)+48;
			snotbuf[6] = ((capturecount/10)%10)+48;
			snotbuf[7] = (capturecount%10)+48;
			if (keystatus[0x38]|keystatus[0xb8])
				{ if (!surroundcapture32bit(&ipos,snotbuf,512)) capturecount++; }
			else
				{ if (!screencapture32bit(snotbuf)) capturecount++; }
		}
		
		stopdirectdraw();
		nextpage();
		numframes++;
	}

		//Read keyboard, mouse, and timer
	readkeyboard();
	obstatus = bstatus; readmouse(&fmousx,&fmousy,&bstatus);
	odtotclk = dtotclk; readklock(&dtotclk);
	totclk = (long)(dtotclk*1000.0); fsynctics = (float)(dtotclk-odtotclk);
	
			//Rotate player's view
#if PLANE_SIM == 0
	if (bstatus&2) 
	{ dp.x = fmousx*-.008; dp.y = fmousy*-.008; dp.z = 0; }
	else
	{ dp.x = istr.z*.1; dp.y = fmousy*-.008; dp.z = fmousx*.008; }
#else      
	//f = sqrt(ivel.x*ivel.x + ivel.y*ivel.y + ivel.z*ivel.z)*0.1 + 0.5;   
	if (invertmouse) ivelang.y += fmousy*-.004;
	else             ivelang.y += fmousy* .004;
	if (bstatus&2) { ivelang.x += fmousx*-.002; } // fov += fmousy*-.005; }
	else           ivelang.z += fmousx*.004;
	//if (keystatus[0xcb]) ivelang.z -= fsynctics*.1;
	//if (keystatus[0xcd]) ivelang.z += fsynctics*.1;
	dp.x = ivelang.x; if (!(bstatus&2)) dp.x += istr.z*.02f;
	dp.y = ivelang.y; //*f;
	dp.z = ivelang.z;
#endif
	if (ptexttim > 0) dp.x = dp.y = dp.z = 0;
	dorthorotate(dp.x,dp.y,dp.z,&istr,&ihei,&ifor);

		//Draw less when turning fast to increase the frame rate
	i = 1; if (xres*yres >= 640*350) i++;
	f = dp.x*dp.x + dp.y*dp.y + dp.z*dp.z;
	if (f >= .01*.01)
	{
		i++;
		if (f >= .08*.08)
		{
			i++; if (f >= .64*.64) i++;
		}
	}
	vx5.anginc = (float)i;

			//Move player and perform simple physics (gravity,momentum,friction)
	f = fsynctics*4.0;
	//if (keystatus[KEY_LEFTSHIFT]) f *= .0625;
	if ((keystatus[KEY_RIGHTSHIFT]|keystatus[KEY_LEFTSHIFT])^autorun) f *= 3.0;
	if (keystatus[KEY_LEFTARROW]|keystatus[KEY_A]) { ivel.x -= istr.x*f; ivel.y -= istr.y*f; ivel.z -= istr.z*f; }
	if (keystatus[KEY_RIGHTARROW]|keystatus[KEY_D]) { ivel.x += istr.x*f; ivel.y += istr.y*f; ivel.z += istr.z*f; }   
	if (keystatus[KEY_DOWNARROW]|keystatus[KEY_S]) { ivel.x -= ifor.x*f; ivel.y -= ifor.y*f; ivel.z -= ifor.z*f; }
	if (keystatus[KEY_RIGHTCTRL]|keystatus[KEY_R]) { ivel.x -= ihei.x*f; ivel.y -= ihei.y*f; ivel.z -= ihei.z*f; } //Rt.Ctrl
	if (keystatus[KEY_PAD0]|keystatus[KEY_F]) { ivel.x += ihei.x*f; ivel.y += ihei.y*f; ivel.z += ihei.z*f; } //KP0   
#if PLANE_SIM == 0
	if (!keystatus[KEY_UPARROW]|keystatus[KEY_W]) { ivel.x += ifor.x*f; ivel.y += ifor.y*f; ivel.z += ifor.z*f; }
#else
	if (keystatus[KEY_UPARROW]|keystatus[KEY_W]) { ivel.x += ifor.x*f; ivel.y += ifor.y*f; ivel.z += ifor.z*f; }
	/*
	if (!keystatus[KEY_UPARROW]|keystatus[KEY_W]) f *= .2f;
	ivel.x += (ifor.x)*f; 
	ivel.y += (ifor.y)*f; 
	ivel.z += (ifor.z)*f;
	*/
#endif
	//ivel.z += fsynctics*2.0; //Gravity (used to be *4.0)

	f = fsynctics*64.0;
	dp.x = ivel.x*f;
	dp.y = ivel.y*f;
	dp.z = ivel.z*f;
	
			// Ptext:
	if (ptexttim > 0) {
		if (ptexttim > fsynctics) f = fsynctics; else f = ptexttim;
		f2 = f*16;
		dp.x = -f2*istr.x;
		dp.y = -f2*istr.y;
		dp.z = -f2*istr.z;
		ptexttim -= f;
		//if (ptexttim <= 0) playsound(HITROCK, 100, 0.5f, NULL, 0);      
		ivel.x = ivel.y = ivel.z = 0;
		ivelang.x = ivelang.y = ivelang.z = 0;
		k = 1; // block velocity update!
	} else k = 0;
	
	dp2 = ipos; 
	if (noclip) {
		ipos.x += dp.x;
		ipos.y += dp.y;
		ipos.z += dp.z;
		if (ipos.x < 1) ipos.x = 1; else if (ipos.x > VSID-2) ipos.x = VSID-2;
		if (ipos.y < 1) ipos.y = 1; else if (ipos.y > VSID-2) ipos.y = VSID-2;
		if (ipos.z > MAXZDIM-2) ipos.z = MAXZDIM-2;
		if (!(bstatus&1)) voxrestore();
		lp.x = ipos.x; lp.y = ipos.y; lp.z = ipos.z;
		voxbackup(lp.x-NOCLIPRAD+1, lp.y-NOCLIPRAD+1, lp.x+NOCLIPRAD, lp.y+NOCLIPRAD, 0x10000);
		vx5.colfunc = mycolfunc; vx5.curcol = 0x80704030;
		setsphere(&lp, NOCLIPRAD, -1);
		if (vx5.lightmode) updatevxl();
	} else clipmove(&ipos,&dp,CLIPRAD);
	if (f != 0 && !k)
	{
		f = .9/f; //Friction
		ivel.x = (ipos.x-dp2.x)*f;
		ivel.y = (ipos.y-dp2.y)*f;
		ivel.z = (ipos.z-dp2.z)*f;
		//f *= 1.01f;
		ivelang.x *= .5f;
		ivelang.y *= .5f;
		ivelang.z *= .5f;
	}
	f = ivel.x*ivel.x + ivel.y*ivel.y + ivel.z*ivel.z; //Limit maximum velocity
	if (f > CLIPVEL*CLIPVEL) { f = CLIPVEL/sqrt(f); ivel.x *= f; ivel.y *= f; ivel.z *= f; }
	//if (f > 0 && f < MINVEL*MINVEL) { f = MINVEL/sqrt(f); ivel.x *= f; ivel.y *= f; ivel.z *= f; }

#if 1
		// Update lasers:
	for(i=lasercnt-1; i>=0; i--) {
		fp3 = laser[i].p;      
		if (laser[i].red)
			  f = fsynctics*100.0f;
		else f = fsynctics*400.0f;
		laser[i].p.x += laser[i].v.x*f;
		laser[i].p.y += laser[i].v.y*f;
		laser[i].p.z += laser[i].v.z*f;
		ftol(fp3.x, &lp.x);
		ftol(fp3.y, &lp.y);
		ftol(fp3.z, &lp.z);
		if ((unsigned long)lp.x >= VSID || (unsigned long)lp.y >= VSID || (unsigned long)lp.z >= MAXZDIM) 
		{
			laser[i].tim -= fsynctics;
			if (laser[i].tim<0) { 
				playsoundupdate(&laser[i].p,(void*)-1);
				if (--lasercnt > 0) { // remove               
					laser[i] = laser[lasercnt];
					playsoundupdate(&laser[lasercnt].p,&laser[i].p);
					continue;
				}
			}
		}
		else if (!cansee(&fp3, &laser[i].p, &lp)) 
		{        
			estnorm(lp.x, lp.y, lp.z, &fp);
#if EXPLOSION == 2
			k = ((lp.x>>HDIMLOG)*HDIMY + (lp.y>>HDIMLOG))*HDIMZ + (lp.z>>HDIMLOG);
			if ((!laser[i].red && hardness[k] > 0) || lp.z==MAXZDIM-1) {
				if (lp.z!=MAXZDIM-1) hardness[k]--;
				j=20; dimbox(lp.x-j,lp.y-j,lp.z-j,lp.x+j,lp.y+j,lp.z+j);
				playsound(CLINK,100,0.45f+((float)rand()*.0000025),&laser[i].p,KSND_3D);
			} else {
				//----------------------------------------------------------------------------------------
				// Explosion code:
				if (laser[i].red) hardness[k] = 0;
				lastbulpos = laser[i].p;
				lastbulvel = laser[i].v;
				if (laser[i].red) {
					vx5sprite tempspr;
					for(lp2.x=lp.x-32; lp2.x<=lp.x; lp2.x+=16)
					for(lp2.y=lp.y-32; lp2.y<=lp.y; lp2.y+=16)
					for(lp2.z=lp.z-32; lp2.z<=lp.z; lp2.z+=16)
					for(t=0;t<4;t++)
					{
						if ((numsprites < MAXSPRITES) && (j = meltspans(&spr[numsprites],(vspans *)&rubble[rubblestart[t]],rubblenum[t],&lp2)))
						{
							k = numsprites++;
							spr[k].tag = -17; spr[k].tim = totclk; spr[k].owner = 0;

							for(j=4;j;j--)
							{
								vecrand(48.0,&spr[k].v);
								if ((!isvoxelsolid((long)(spr[k].p.x+spr[k].v.x),
															(long)(spr[k].p.y+spr[k].v.y),
															(long)(spr[k].p.z+spr[k].v.z))) &&
										(!isvoxelsolid((long)(spr[k].p.x+spr[k].v.x*.25),
															(long)(spr[k].p.y+spr[k].v.y*.25),
															(long)(spr[k].p.z+spr[k].v.z*.25))))
									break;
							}
							vecrand(1.0,&spr[k].r);
						}                  
					}
					if (explode_kv6) {
						k=32;
						tempspr.p.x = lp.x; tempspr.p.y = lp.y; tempspr.p.z = lp.z;
						matrand((float)k*.036,&tempspr.s,&tempspr.h,&tempspr.f);
						tempspr.flags = 0; tempspr.voxnum = explode_kv6;
						vx5.colfunc = mycolfunc; vx5.curcol = 0x80704030;
						setkv6(&tempspr,-1);                  
						//k=40;
						//updatebbox(lp.x-k,lp.y-k,lp.z-k,lp.x+k,lp.y+k,lp.z+k,0);
					}
					playsound(BLOWUP,100,0.95f+(rand()/200000.0f),&lastbulpos,KSND_3D);
				} else {               
					lp2.x = lp.x-16; lp2.y = lp.y-16; lp2.z = lp.z-16;
					for(t=0;t<4;t++)
					{
						if ((numsprites < MAXSPRITES) && (j = meltspans(&spr[numsprites],(vspans *)&rubble[rubblestart[t]],rubblenum[t],&lp2)))
						{
							k = numsprites++;
							spr[k].tag = -17; spr[k].tim = totclk; spr[k].owner = 0;

							for(j=4;j;j--)
							{
								vecrand(48.0,&spr[k].v);
								if ((!isvoxelsolid((long)(spr[k].p.x+spr[k].v.x),
															(long)(spr[k].p.y+spr[k].v.y),
															(long)(spr[k].p.z+spr[k].v.z))) &&
										(!isvoxelsolid((long)(spr[k].p.x+spr[k].v.x*.25),
															(long)(spr[k].p.y+spr[k].v.y*.25),
															(long)(spr[k].p.z+spr[k].v.z*.25))))
									break;
							}
							//spr[k].v.x = (((float)rand()/16383.5f)-1.f)*64.0;
							//spr[k].v.y = (((float)rand()/16383.5f)-1.f)*64.0;
							//spr[k].v.z = (((float)rand()/16383.5f)-1.f)*64.0;
							vecrand(1.0,&spr[k].r);
							//spr[k].r.x = ((float)rand()/16383.5f)-1.f;
							//spr[k].r.y = ((float)rand()/16383.5f)-1.f;
							//spr[k].r.z = ((float)rand()/16383.5f)-1.f;
						}
						vx5.colfunc = mycolfunc; vx5.curcol = 0x80704030;
						setspans((vspans *)&rubble[rubblestart[t]],rubblenum[t],&lp2,-1);
					}
					playsound(HITROCK,100,0.9f+(rand()/200000.0f),&lastbulpos,KSND_3D);
				}
				//----------------------------------------------------------------------------------------
			}
#elif EXPLOSION == 1
			vx5.colfunc = mycolfunc; vx5.curcol = 0x80704030;
			f = 4;
			lp2.x = lp.x - laser[i].v.x*f;
			lp2.y = lp.y - laser[i].v.y*f;
			lp2.z = lp.z - laser[i].v.z*f;
			f = 14;
			lp3.x = lp.x + laser[i].v.x*f;
			lp3.y = lp.y + laser[i].v.y*f;
			lp3.z = lp.z + laser[i].v.z*f;
			setellipsoid(&lp2, &lp3, 12, -1, 0);         
#endif                  
			f = (laser[i].v.x*fp.x + laser[i].v.y*fp.y + laser[i].v.z*fp.z)*2;
			laser[i].v.x -= f*fp.x;
			laser[i].v.y -= f*fp.y;
			laser[i].v.z -= f*fp.z;
			//laser[i].p = fp3;
			f = 2;
			laser[i].p.x = lp.x - fp.x*f;
			laser[i].p.y = lp.y - fp.y*f;
			laser[i].p.z = lp.z - fp.z*f;

			if (--laser[i].hitcnt < 0 || laser[i].red) {
				playsoundupdate(&laser[i].p,(void*)-1);
				if (--lasercnt > 0) { // remove               
					laser[i] = laser[lasercnt];
					playsoundupdate(&laser[lasercnt].p,&laser[i].p);
				}
			}
		}
	}
#endif

	//----------------------------------------------------------------------------------------
	// Update sprites:
	for(i=numsprites-1;i>=0;i--)
	{
		if (spr[i].tag == -17) //Animate melted sprites
		{
			if ((totclk > spr[i].tim+2000) || (spr[i].owner >= 3))
			{
				//spr[i].s.x *= .97; spr[i].s.y *= .97; spr[i].s.z *= .97;
				//spr[i].h.x *= .97; spr[i].h.y *= .97; spr[i].h.z *= .97;
				//spr[i].f.x *= .97; spr[i].f.y *= .97; spr[i].f.z *= .97;
				//if (spr[i].s.x*spr[i].s.x + spr[i].s.y*spr[i].s.y + spr[i].s.z*spr[i].s.z < .1*.1)
				{
					if (spr[i].voxnum)
					{
						j = spr[i].voxnum->vox[0].col;

						playsound(DEBRIS2,70,(float)(rand()-32768)*.00002+1.0,&spr[i].p,KSND_3D);
						//explodesprite(&spr[i],.125,0,3);

						//spawndebris(&spr[i].p,1,j,16,0);
						//spawndebris(&spr[i].p,0.5,j,8,0);
						//spawndebris(&spr[i].p,0.25,j,4,0);

						//setkv6(&spr[i],0);

							//Delete temporary sprite data from memory
						free(spr[i].voxnum);
					}
					deletesprite(i);
					continue;
				}
			}

			fpos = spr[i].p;

				//Do velocity & gravity
			spr[i].v.z += fsynctics*64;
			spr[i].p.x += spr[i].v.x * fsynctics;
			spr[i].p.y += spr[i].v.y * fsynctics;
			spr[i].p.z += spr[i].v.z * fsynctics;
			spr[i].v.z += fsynctics*64;

				//Do rotation
			f = min(totclk-spr[i].tim,250)*fsynctics*.01;
			axisrotate(&spr[i].s,&spr[i].r,f);
			axisrotate(&spr[i].h,&spr[i].r,f);
			axisrotate(&spr[i].f,&spr[i].r,f);

				//Make it bounce
			if (!cansee(&fpos,&spr[i].p,&lp))  //Wake up immediately if it hit a wall
			{
				spr[i].p = fpos;
				estnorm(lp.x,lp.y,lp.z,&fp);
				f = (spr[i].v.x*fp.x + spr[i].v.y*fp.y + spr[i].v.z*fp.z)*2.f;
				spr[i].v.x = (spr[i].v.x - fp.x*f)*.75f;
				spr[i].v.y = (spr[i].v.y - fp.y*f)*.75f;
				spr[i].v.z = (spr[i].v.z - fp.z*f)*.75f;
				vecrand(1.0,&spr[i].r);
				//spr[i].r.x = ((float)rand()/16383.5f)-1.f;
				//spr[i].r.y = ((float)rand()/16383.5f)-1.f;
				//spr[i].r.z = ((float)rand()/16383.5f)-1.f;

				if (f > 96)
				{
						//Make it shatter immediately if it hits ground too quickly
					if (spr[i].voxnum)
					{
						j = spr[i].voxnum->vox[0].col;

						playsound(DEBRIS2,70,(float)(rand()-32768)*.00002+1.0,&spr[i].p,KSND_3D);
						//explodesprite(&spr[i],.125,0,3);

						//spawndebris(&spr[i].p,f*.5  /96,j,16,0);
						//spawndebris(&spr[i].p,f*.25 /96,j,8,0);
						//spawndebris(&spr[i].p,f*.125/96,j,4,0);

							//Delete temporary sprite data from memory
						free(spr[i].voxnum);
					}
					deletesprite(i);
					continue;
				}
				else
				{
					//playsound(DIVEBORD,25,(float)(rand()-16384)*.00001+1.0,&spr[i].p,KSND_3D);
					spr[i].owner++;
				}
			}
		}
	}
	//----------------------------------------------------------------------------------------
	// Falling code:
	updatevxl();

	startfalls();
	for(i=vx5.flstnum-1;i>=0;i--)
	{
		if ((numsprites < MAXSPRITES) && (meltfall(&spr[numsprites],i,1)))
		{
			k = numsprites++;
			playsound(HITROCK,vx5.flstcnt[i].mass>>4,(float)(rand()-16384)*.00001+0.5,&spr[k].p,KSND_MOVE);
			spr[k].tag = -17; spr[k].tim = totclk; spr[k].owner = 0;
			spr[k].v.x = 0;
			spr[k].v.y = 0;
			spr[k].v.z = 0;
			spr[k].r.x = (lastbulpos.y-fp.y)*lastbulvel.z - (lastbulpos.z-fp.z)*lastbulvel.y;
			spr[k].r.y = (lastbulpos.z-fp.z)*lastbulvel.x - (lastbulpos.x-fp.x)*lastbulvel.z;
			spr[k].r.z = (lastbulpos.x-fp.x)*lastbulvel.y - (lastbulpos.y-fp.y)*lastbulvel.x;
		}
	}
	vx5.flstnum = 0;

	//----------------------------------------------------------------------------------------


	if (bstatus&1) {
		while (lasertim>=laserdelay && lasercnt<MAXLASER) 
		{
				// Spawn laser:
			fcossin(laserang,&f,&f2);
			f *= CLIPRAD;
			f2 *= CLIPRAD;
			laser[lasercnt].p.x = ipos.x + istr.x*f + ihei.x*f2;
			laser[lasercnt].p.y = ipos.y + istr.y*f + ihei.y*f2;
			laser[lasercnt].p.z = ipos.z + istr.z*f + ihei.z*f2;
			f = 0; //f = 0.1f*laserdir;
			laser[lasercnt].v.x = ifor.x + istr.x*f;
			laser[lasercnt].v.y = ifor.y + istr.y*f;
			laser[lasercnt].v.z = ifor.z + istr.z*f;         
			laser[lasercnt].tim = 1;
			laser[lasercnt].hitcnt = 4;
			laser[lasercnt].red = laserred;
#if 0
			fcossin(laserang,&f,&f2);
			f *= 0.2f;
			f2 *= 0.2f;
			ivel.x += istr.x*f + ihei.x*f2 - ifor.x*0.03f;
			ivel.y += istr.y*f + ihei.x*f2 - ifor.y*0.03f;
			ivel.z += istr.z*f + ihei.x*f2 - ifor.z*0.03f;
#endif
			if (laserred) 
				  playsound(LASER, 100, 0.5f, &laser[lasercnt].p, KSND_LOOP|KSND_MOVE);
			else playsound(LASER, 100, 1.2f, &laser[lasercnt].p, KSND_LOOP|KSND_MOVE);

			lasercnt++;
			if (laserred) lasertim -= laserdelay*2.5f;
			else lasertim -= laserdelay;
		}
		lasertim += fsynctics;
	} else lasertim = laserdelay;
	if ((laserang += fsynctics*8) > 2*PI) laserang -= 2*PI;   
	
	while(k = keyread()) {
		k = (k>>8)&255;
		switch(k) {
		case KEY_ESC: quitloop(); break;
		//case KEY_SPACE: showfps ^= 1; break; // space
		case KEY_ENTER: if (laserredenable) laserred ^= 1; break;
		//case KEY_ENTER: disable_light(); break; // enter
		//case KEY_SPACE: toggle_noclip(); break;
		case KEY_Y: invertmouse ^= 1; break;
		case KEY_CAPSLOCK: autorun ^= 1; break;
		//case KEY_T: introtim = 2; break;
		}
	}   
	if (introtim > 1) {
		for(i=15;i>=0;i--)dorthorotate(istr.z*.2f,0,0,&istr,&ihei,&ifor);
		f = 16; f2 = -12; d = .2f;      
		ptextcnt = 0;
#if 1   
		add_ptext("VOXLAP CAVE DEMO", f, f2, -30, d, 0x80FF00);
		add_ptext("Credits:", f, f2, -10, d, 0x8080FF);
		add_ptext("Tom Dobrowolski", f, f2, 0, d, 0xC0C0F0); 
		add_ptext("Ken Silverman", f, f2, 10, d, 0xC0C0F0); 
		if (powerupon) add_ptext("Find the red blaster!", f, f2, 30, d, 0xFF8000); 
#else
		add_ptext("VOXLAP CAVE DEMO", f, f2, -30, d, 0x80FF00);
		if (powerupon) add_ptext("Find the red blaster!", f, f2, -20, d, 0xFF8000); 
		add_ptext("Credits:", f, f2, 0, d, 0x8080FF);
		add_ptext("Tom Dobrowolski", f, f2, 10, d, 0xC0C0F0); 
		add_ptext("Ken Silverman", f, f2, 20, d, 0xC0C0F0); 
#endif
		if (ptextcnt) playsound(AIRPUMP, 100, 0.5f, &ptext[0].p, KSND_3D);
		ptexttim = -f2/16.f;
		ivel.x = ivel.y = ivel.z = 0;
		ivelang.x = ivelang.y = ivelang.z = 0;
		introtim = -1;
	} else if (introtim >= 0) introtim += fsynctics;
}


#if 0
!endif
#endif
