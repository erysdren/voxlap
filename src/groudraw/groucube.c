//GROUCUBE.C by Ken Silverman (http://advsys.net/ken)
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

static short xdim, ydim, pixs, ang, umost[320], dmost[320], sintable[2048];
static long posx, posy, posz, newposz, vel, svel, angvel, horiz, depth;
static short int moustat, mousx, mousy;
static unsigned char grou[2097152], vidmode;
static unsigned char tempbuf[16384], palette[768], darkcol, scrbuf[128000];

volatile char keystatus[256], readch, oldreadch, extended;
volatile long clockspeed, totalclock, numframes;
static void (__interrupt __far *oldhand)();
static void __interrupt __far ksmhandler(void);
static void (__interrupt __far *oldkeyhandler)();
static void __interrupt __far keyhandler(void);

void setvmode (long);
#pragma aux setvmode =\
	"int 0x10"\
	parm [eax]

void setupmouse ();
#pragma aux setupmouse =\
	"mov ax, 0"\
	"int 33h"\
	"mov moustat, 1"\
	modify [ax bx]

void readmouse ();
#pragma aux readmouse =\
	"mov ax, 11d"\
	"int 33h"\
	"sar cx, 1"\
	"sar dx, 1"\
	"mov mousx, cx"\
	"mov mousy, dx"\
	modify [ax cx dx]

long drawslab (long, long, long);
#pragma aux drawslab =\
	"startdraw: mov [edi], al"\
	"sub edi, 80"\
	"loop startdraw"\
	"mov eax, edi"\
	parm [edi][ecx][eax]\
	modify [edi ecx eax]

void showscreen4pix320200 ();
#pragma aux showscreen4pix320200 =\
	"mov dx, 0x3c4"\
	"mov ax, 0x0f02"\
	"out dx, ax"\
	"mov ecx, 4000"\
	"mov esi, offset scrbuf"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	modify [eax ecx edx esi edi]

void showscreen4pix320400 ();
#pragma aux showscreen4pix320400 =\
	"mov dx, 0x3c4"\
	"mov ax, 0x0f02"\
	"out dx, ax"\
	"mov ecx, 8000"\
	"mov esi, offset scrbuf"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	modify [eax ecx edx esi edi]

void showscreen2pix320200 ();
#pragma aux showscreen2pix320200 =\
	"mov dx, 0x3c4"\
	"mov ax, 0x0302"\
	"out dx, ax"\
	"mov ecx, 4000"\
	"mov esi, offset scrbuf"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	"mov ax, 0x0c02"\
	"out dx, ax"\
	"mov ecx, 4000"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	modify [eax ecx edx esi edi]

void showscreen2pix320400 ();
#pragma aux showscreen2pix320400 =\
	"mov dx, 0x3c4"\
	"mov ax, 0x0302"\
	"out dx, ax"\
	"mov ecx, 8000"\
	"mov esi, offset scrbuf"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	"mov ax, 0x0c02"\
	"out dx, ax"\
	"mov ecx, 8000"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	modify [eax ecx edx esi edi]

void showscreen1pix320200 ();
#pragma aux showscreen1pix320200 =\
	"mov dx, 0x3c4"\
	"mov ax, 0x0102"\
	"out dx, ax"\
	"mov ecx, 4000"\
	"mov esi, offset scrbuf"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	"mov ax, 0x0202"\
	"out dx, ax"\
	"mov ecx, 4000"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	"mov ax, 0x0402"\
	"out dx, ax"\
	"mov ecx, 4000"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	"mov ax, 0x0802"\
	"out dx, ax"\
	"mov ecx, 4000"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	modify [eax ecx edx esi edi]

void showscreen1pix320400 ();
#pragma aux showscreen1pix320400 =\
	"mov dx, 0x3c4"\
	"mov ax, 0x0102"\
	"out dx, ax"\
	"mov ecx, 8000"\
	"mov esi, offset scrbuf"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	"mov ax, 0x0202"\
	"out dx, ax"\
	"mov ecx, 8000"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	"mov ax, 0x0402"\
	"out dx, ax"\
	"mov ecx, 8000"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	"mov ax, 0x0802"\
	"out dx, ax"\
	"mov ecx, 8000"\
	"mov edi, 0xa0000"\
	"rep movsd"\
	modify [eax ecx edx esi edi]

void interruptend ();
#pragma aux interruptend =\
	"mov al, 20h"\
	"out 20h, al"\
	modify [eax]

void readkey ();
#pragma aux readkey =\
	"in al, 0x60"\
	"mov readch, al"\
	"in al, 0x61"\
	"or al, 0x80"\
	"out 0x61, al"\
	"and al, 0x7f"\
	"out 0x61, al"\
	modify [eax]

void __interrupt __far ksmhandler ()
{
	clockspeed++;
	interruptend();
}

void __interrupt __far keyhandler ()
{
	oldreadch = readch;
	readkey();
	if ((readch|1) == 0xe1)
		extended = 128;
	else
	{
		if (oldreadch != readch)
			keystatus[(readch&127)+extended] = ((readch>>7)^1);
		extended = 0;
	}
	interruptend();
}

int main (int argc, char **argv)
{
	long i, j, templong;
	char filename[260];

	setvmode(0x13);
	inp(0x3da); outp(0x3c0,0x31); outp(0x3c0,darkcol);

	outp(0x3c4,0x4); outp(0x3c5,0x6);
	outp(0x3d4,0x14); outp(0x3d5,0x0);
	outp(0x3d4,0x17); outp(0x3d5,0xe3);

	setupmouse();
	if (!loadtables())
	{
		setvmode(0x3);
		printf("tables.dat not found\n");
		exit(1);
	}

	strcpy(filename,"c1.dta");
	if (argc == 2) filename[1] = (char)(atoi(argv[1])+48);
	if (!loadata(filename))
	{
		setvmode(0x3);
		printf("%s/",filename); filename[0] = 'd'; printf("%s not found",filename);
		exit(1);
	}

	xdim = 320; ydim = 200; pixs = 1; vidmode = 0;
	posx = 0L; posy = 0L; posz = 0L; depth = (32L<<20); horiz = 100; ang = 256;
	posz = depth-(((long)grou[((((posx>>10)&1023)<<10)+((posy>>10)&1023))<<1])<<20);

	oldkeyhandler = _dos_getvect(0x9);
	_disable(); _dos_setvect(0x9, keyhandler); _enable();
	clockspeed = 0L;
	totalclock = 0L;
	numframes = 0L;
	outp(0x43,54); outp(0x40,4972&255); outp(0x40,4972>>8);
	oldhand = _dos_getvect(0x8);
	_disable(); _dos_setvect(0x8, ksmhandler); _enable();
	while (keystatus[1] == 0)
	{
		for(i=0;i<xdim;i+=pixs)
		{
			umost[i] = -1;
			dmost[i] = ydim;
		}
		newposz = depth-(((long)grou[((((posx>>10)&1023)<<10)+((posy>>10)&1023))<<1])<<20);
		posz = (posz+posz+posz+newposz)>>2;
		for(i=0;i<xdim;i+=pixs)
			grouvline(i,384L);
		if (vidmode == 0)
		{
			if (pixs == 4) showscreen4pix320200();
			if (pixs == 2) showscreen2pix320200();
			if (pixs == 1) showscreen1pix320200();
		}
		else
		{
			if (pixs == 4) showscreen4pix320400();
			if (pixs == 2) showscreen2pix320400();
			if (pixs == 1) showscreen1pix320400();
		}

		vel = 0L;
		svel = 0L;
		angvel = 0;
		if (moustat == 1)
		{
			readmouse();
			ang += mousx;
			vel = (((long)-mousy)<<3);
		}
		if (keystatus[0xd1]|keystatus[0x4e]) horiz += clockspeed;
		if (keystatus[0xc9]|keystatus[0x4a]) horiz -= clockspeed;
		if (keystatus[0x1e]) depth += (clockspeed<<18);
		if (keystatus[0x2c]) depth = max(depth-(clockspeed<<18),4<<8);
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
		if (angvel != 0)
		{
			ang += ((angvel*((short int)clockspeed))>>3);
			ang &= 2047;
		}
		if ((vel != 0L) || (svel != 0L))
		{
			posx += ((vel*clockspeed*sintable[(512+ang)&2047])>>12);
			posy += ((vel*clockspeed*sintable[(    ang)&2047])>>12);
			posx += ((svel*clockspeed*sintable[(    ang)&2047])>>12);
			posy -= ((svel*clockspeed*sintable[(512+ang)&2047])>>12);
			posx &= 0x3ffffff;
			posy &= 0x3ffffff;
		}
		if (keystatus[0x10] > 0) { keystatus[0x10] = 0; pixs = 1; } //Q
		if (keystatus[0x11] > 0) { keystatus[0x11] = 0; pixs = 2; } //W
		if (keystatus[0x12] > 0) { keystatus[0x12] = 0; pixs = 4; } //E
		if ((keystatus[0x1f]|keystatus[0x20]) > 0)
		{
			if (keystatus[0x1f] > 0) //S
			{
				if (vidmode == 0)
				{
					vidmode = 1;
					outp(0x3d4,0x9); outp(0x3d5,inp(0x3d5)&254);
					keystatus[0x1f] = 0;
					ydim = 400L;
					horiz <<= 1;
				}
			}
			if (keystatus[0x20] > 0) //D
			{
				if (vidmode == 1)
				{
					vidmode = 0;
					outp(0x3d4,0x9); outp(0x3d5,inp(0x3d5)|1);
					keystatus[0x20] = 0;
					ydim = 200L;
					horiz >>= 1;
				}
			}
		}

		numframes++;
		totalclock += clockspeed;
		clockspeed = 0L;
	}
	outp(0x43,54); outp(0x40,255); outp(0x40,255);
	_dos_setvect(0x8, oldhand);
	_dos_setvect(0x9, oldkeyhandler);
	setvmode(0x3);
	if (totalclock != 0)
	{
		templong = (numframes*24000L)/totalclock;
		printf("%d.%1d%1d frames per second\n",(short int)(templong/100),(short int)((templong/10)%10),(short int)(templong%10));
	}
	return(0);
}

int loadtables ()
{
	short fil;

	if ((fil = open("tables.dat",O_BINARY|O_RDWR,S_IREAD)) != -1)
	{
		read(fil,&sintable[0],4096);
		close(fil);
		return(1);
	}
	return(0);
}

int loadata (char *filename)
{
	unsigned char dat;
	short int fil, colval, mincolval;
	long i, p, datcnt, leng;

	fil = open(filename,O_BINARY|O_RDWR,S_IREAD); if (fil == -1) return(0);
	read(fil,&tempbuf[0],128);
	p = 1;
	read(fil,&tempbuf[0],16384), datcnt = 0;
	while (p < 2097152)
	{
		dat = tempbuf[datcnt++];
		if (datcnt == 16384)
		{
			read(fil,&tempbuf[0],16384);
			datcnt = 0;
		}
		if ((dat&0xc0) == 0xc0)
		{
			leng = (dat&0x3f);
			dat = tempbuf[datcnt++];
			if (datcnt == 16384)
			{
				read(fil,&tempbuf[0],16384);
				datcnt = 0;
			}
			for(i=0;i<leng;i++)
			{
				grou[((p>>10)&0x7fe)+((p&0x7fe)<<10)+1] = dat;
				p += 2;
			}
		}
		else
		{
			grou[((p>>10)&0x7fe)+((p&0x7fe)<<10)+1] = dat;
			p += 2;
		}
	}
	dat = tempbuf[datcnt++];
	if (datcnt == 16384)
	{
		read(fil,&tempbuf[0],16384);
		datcnt = 0;
	}
	mincolval = 768;
	if (dat == 0xc)
	{
		outp(0x3c8,0);
		for(i=0;i<256;i++)
		{
			dat = tempbuf[datcnt++];
			if (datcnt == 16384)
			{
				read(fil,&tempbuf[0],16384);
				datcnt = 0;
			}
			palette[i+i+i+0] = dat>>2;
			outp(0x3c9,dat>>2);
			colval = (short int)dat;
			dat = tempbuf[datcnt++];
			if (datcnt == 16384)
			{
				read(fil,&tempbuf[0],16384);
				datcnt = 0;
			}
			palette[i+i+i+1] = dat>>2;
			outp(0x3c9,dat>>2);
			colval += (short int)dat;
			dat = tempbuf[datcnt++];
			if (datcnt == 16384)
			{
				read(fil,&tempbuf[0],16384);
				datcnt = 0;
			}
			palette[i+i+i+2] = dat>>2;
			outp(0x3c9,dat>>2);
			colval += (short int)dat;
			if (colval < mincolval)
			{
				darkcol = i;
				mincolval = colval;
			}
		}
	}
	inp(0x3da); outp(0x3c0,0x31); outp(0x3c0,darkcol);
	close(fil);


	filename[0] = 'd';
	fil = open(filename,O_BINARY|O_RDWR,S_IREAD); if (fil == -1) return(0);
	read(fil,&tempbuf[0],128);
	p = 0;
	read(fil,&tempbuf[0],16384), datcnt = 0;
	while (p < 2097152)
	{
		dat = tempbuf[datcnt++];
		if (datcnt == 16384)
		{
			read(fil,&tempbuf[0],16384);
			datcnt = 0;
		}
		if ((dat&0xc0) == 0xc0)
		{
			leng = (dat&0x3f);
			dat = tempbuf[datcnt++];
			if (datcnt == 16384)
			{
				read(fil,&tempbuf[0],16384);
				datcnt = 0;
			}
			for(i=0;i<leng;i++)
			{
				grou[((p>>10)&0x7fe)+((p&0x7fe)<<10)] = 255-dat;
				p += 2;
			}
		}
		else
		{
			grou[((p>>10)&0x7fe)+((p&0x7fe)<<10)] = 255-dat;
			p += 2;
		}
	}
	close(fil);
	return(1);
}

int grouvline (long x, long scandist)
{
	char col, index;
	short i, j, dir[2], grid[2], cnt;
	long plc, bufplc, cosval, sinval, dist[2], dinc[2], snx, sny, top, bot;
	long tempx, tempy, incr[2], they, lastdist, templong;

	if ((scandist <= 0) || (umost[x] >= dmost[x]))
		return(0);
	switch(pixs)
	{
		case 1:
			plc = (dmost[x]+1)*80+(x>>2)+((long)scrbuf);
			if ((x&2) > 0) plc += 32000*(vidmode+1);
			if ((x&1) > 0) plc += 16000*(vidmode+1);
			break;
		case 2:
			plc = (dmost[x]+1)*80+(x>>2)+((long)scrbuf);
			if ((x&2) > 0) plc += 16000*(vidmode+1);
			break;
		case 4:
			plc = (dmost[x]+1)*80+(x>>2)+((long)scrbuf);
	}

	cosval = sintable[(ang+512)&2047], sinval = sintable[ang&2047];
	tempx = (xdim>>1), tempy = (x-(xdim>>1));
	incr[0] = (tempx*cosval - tempy*sinval) / (xdim>>1);
	incr[1] = (tempy*cosval + tempx*sinval) / (xdim>>1);
	if (incr[0] < 0)
	{
		dir[0] = -1;
		incr[0] = -incr[0];
	}
	else
	{
		dir[0] = 1;
	}
	if (incr[1] < 0)
	{
		dir[1] = -1;
		incr[1] = -incr[1];
	}
	else
	{
		dir[1] = 1;
	}
	snx = (posx&1023); if (dir[0] == 1) snx ^= 1023;
	sny = (posy&1023); if (dir[1] == 1) sny ^= 1023;
	cnt = ((snx*incr[1] - sny*incr[0])>>10);
	grid[0] = (posx>>10)&1023; grid[1] = (posy>>10)&1023;

	if (incr[0] != 0)
	{
		dinc[0] = ((65536*2048)>>vidmode) / incr[0];
		dist[0] = (((dinc[0]>>5)*snx)>>5);
	}
	if (incr[1] != 0)
	{
		dinc[1] = ((65536*2048)>>vidmode) / incr[1];
		dist[1] = (((dinc[1]>>5)*sny)>>5);
	}
	if (dist[0] <= 0) dist[0] = 1;
	if (dist[1] <= 0) dist[1] = 1;

	templong = incr[0], incr[0] = incr[1], incr[1] = -templong;
	top = umost[x], bot = dmost[x];

	i = scandist;
	while (i > 0)
	{
		index = (cnt >= 0), cnt += incr[index];
		bufplc = (((grid[0]<<10)+grid[1])<<1);
		they = (((long)grou[bufplc])<<20)+posz, col = grou[bufplc+1];
		if (they < 0) goto restisabove;
		they /= dist[index];
		they += horiz;
		lastdist = dist[index], dist[index] += dinc[index];
		grid[index] += dir[index], grid[index] &= 1023;
		if (they <= bot)
		{
			if (they < top) break;
			plc = drawslab(plc,bot-they+1,col);
			bot = they-1;
		}
		i--;
	}
	goto endgrouv;
	while (i > 0)
	{
		index = (cnt >= 0), cnt += incr[index];
		bufplc = (((grid[0]<<10)+grid[1])<<1);
		they = (((long)grou[bufplc])<<20)+posz, col = grou[bufplc+1];
restisabove:
		they /= lastdist;
		they += horiz;
		lastdist = dist[index], dist[index] += dinc[index];
		grid[index] += dir[index], grid[index] &= 1023;
		if (they <= bot)
		{
			if (they < top) break;
			plc = drawslab(plc,bot-they+1,col);
			bot = they-1;
		}
		i--;
	}
	endgrouv:
	if (bot >= top)
	{
		if (they < top)
			drawslab(plc,bot-top+1,col);
		else
			drawslab(plc,bot-top+1,darkcol);
	}
	return(0);
}
