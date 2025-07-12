//VES2.H by Ken Silverman (http://advsys.net/ken)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>

#pragma pack(push,1);

typedef struct
{
	char VESASignature[4];
	short VESAVersion;
	long OemStringPtr, Capabilities, VideoModePtr;
	short TotalMemory, OemSoftwareRev;
	long OemVendorNamePtr, OemProductNamePtr, OemProductRevPtr;
	char reserved[222], OemDATA[256];
} VBE_vgaInfo;

typedef struct
{
	short ModeAttributes;
	char WinAAtributes, WinBAttributes;
	short WinGranularity, WinSize, WinASegment, WinBSegment;
	long WinFuncPtr;
	short BytesPerScanLine, XResolution, YResolution;
	char XCharSize, YCharSize, NumberOfPlanes, BitsPerPixel;
	char NumberOfBanks, MemoryModel, BankSize, NumberOfImagePages;
	char res1;
	char RedMaskSize, RedFieldPosition;
	char GreenMaskSize, GreenFieldPosition;
	char BlueMaskSize, BlueFieldPosition;
	char RsvdMaskSize, RsvdFieldPosition, DirectColorModeInfo;
	long PhysBasePtr, OffScreenMemOffset;
	short OffScreenMemSize;
	char res2[206];
} VBE_modeInfo;

struct _RMWORDREGS { unsigned short ax, bx, cx, dx, si, di, cflag; };
struct _RMBYTEREGS { unsigned char al, ah, bl, bh, cl, ch, dl, dh; };
typedef union { struct _RMWORDREGS x; struct _RMBYTEREGS h; } RMREGS;
typedef struct { unsigned short es, cs, ss, ds; } RMSREGS;

typedef struct
{
	long edi, esi, ebp, reserved, ebx, edx, ecx, eax;
	short flags, es, ds, fs, gs, ip, cs, sp, ss;
} _RMREGS;

#pragma pack(pop);

long VESABuf_sel = 0, VESABuf_rseg;
short modelist[256];
long validmodecnt = 0;
short validmode[256];
long validmodexdim[256], validmodeydim[256], validmodebits[256];
static char *screen = NULL, vesachecked = 0, vgacompatible = 0;
long xres, yres, bytesperline, frameplace, imageSize, maxpages;
long buffermode, linearmode;
char RedMaskSize = 0, RedFieldPosition = 0;
char GreenMaskSize = 0 , GreenFieldPosition = 0;
char BlueMaskSize = 0, BlueFieldPosition = 0;
long setactiveentry = 0, setvisualentry = 0, setpaletteentry = 0;
short visualpagelookup[64][2];
long activepagelookup[64];
static long davesapageshift;
static VBE_vgaInfo vgaInfo;
long globlinplace;
//static long backlinaddress = 0;  //Save address for free operation (0x801)

void qlimitrate ();
#pragma aux qlimitrate =\
	"mov dx, 0x3da"\
	"wait1: in al, dx"\
	"test al, 1"\
	"jnz wait1"\
	modify exact [eax edx]

short ods, oes, oss;
void backupsegs ();
#pragma aux backupsegs =\
	"mov ods, ds"\
	"mov oes, es"\
	"mov oss, ss"\
	modify [eax]

void restoresegs ();
#pragma aux restoresegs =\
	"mov ds, ods"\
	"mov es, oes"\
	"mov ss, oss"\
	modify [eax]

void setvmode (long);
#pragma aux setvmode =\
	"int 0x10"\
	parm [eax]

void copybufbyte (long, long, long);
#pragma aux copybufbyte =\
	"test cl, 1"\
	"jz skip1"\
	"movsb"\
	"skip1: shr ecx, 2"\
	"jnc skip2"\
	"movsw"\
	"skip2: rep movsd"\
	parm [esi][edi][ecx]

void koutp (long, long);
#pragma aux koutp =\
	"out dx, al"\
	parm [edx][eax] modify exact

void koutpw (long, long);
#pragma aux koutpw =\
	"out dx, ax"\
	parm [edx][eax] modify exact

long kinp (long);
#pragma aux kinp =\
	"in al, dx"\
	parm nomemory [edx] modify exact [eax]

void vesasetactive (long, long, long, long);
#pragma aux vesasetactive =\
	"call dword ptr [setactiveentry]"\
	parm [eax][ebx][ecx][edx]

void vesasetvisual (long, long, long, long);
#pragma aux vesasetvisual =\
	"call dword ptr [setvisualentry]"\
	parm [eax][ebx][ecx][edx]

long vesasetpalette (long, long, long, long, long, long);
#pragma aux vesasetpalette =\
	"call dword ptr [setpaletteentry]"\
	parm [eax][ebx][ecx][edx][esi][edi]

long DPMI_int86 (long intno, RMREGS *in, RMREGS *out)
{
	_RMREGS rmregs;
	union REGS r;
	struct SREGS sr;

	memset(&rmregs,0,sizeof(rmregs));
	rmregs.eax = in->x.ax; rmregs.ebx = in->x.bx;
	rmregs.ecx = in->x.cx; rmregs.edx = in->x.dx;
	rmregs.esi = in->x.si; rmregs.edi = in->x.di;

	segread(&sr);
	r.w.ax = 0x300; r.h.bl = intno; r.h.bh = 0; r.w.cx = 0; sr.es = sr.ds;
	r.x.edi = (unsigned)&rmregs;
	backupsegs(); int386x(0x31,&r,&r,&sr); restoresegs();

	out->x.ax = rmregs.eax; out->x.bx = rmregs.ebx;
	out->x.cx = rmregs.ecx; out->x.dx = rmregs.edx;
	out->x.si = rmregs.esi; out->x.di = rmregs.edi;
	out->x.cflag = rmregs.flags&1;
	return(out->x.ax);
}

long DPMI_int86x (long intno, RMREGS *in, RMREGS *out, RMSREGS *sregs)
{
	_RMREGS rmregs;
	union REGS r;
	struct SREGS sr;

	memset(&rmregs, 0, sizeof(rmregs));
	rmregs.eax = in->x.ax; rmregs.ebx = in->x.bx;
	rmregs.ecx = in->x.cx; rmregs.edx = in->x.dx;
	rmregs.esi = in->x.si; rmregs.edi = in->x.di;
	rmregs.es = sregs->es;
	rmregs.ds = sregs->ds;

	segread(&sr);
	r.w.ax = 0x300; r.h.bl = intno; r.h.bh = 0; r.w.cx = 0; sr.es = sr.ds;
	r.x.edi = (unsigned)&rmregs;
	backupsegs(); int386x(0x31,&r,&r,&sr); restoresegs();

	out->x.ax = rmregs.eax; out->x.bx = rmregs.ebx;
	out->x.cx = rmregs.ecx; out->x.dx = rmregs.edx;
	out->x.si = rmregs.esi; out->x.di = rmregs.edi;
	sregs->es = rmregs.es; sregs->cs = rmregs.cs;
	sregs->ss = rmregs.ss; sregs->ds = rmregs.ds;
	out->x.cflag = rmregs.flags&1;
	return(out->x.ax);
}

static void ExitVBEBuf ()
{
	union REGS r;
	r.w.ax = 0x101; r.w.dx = VESABuf_sel;
	backupsegs(); int386(0x31,&r,&r); restoresegs(); //DPMI free real seg
}

void VBE_callESDI (RMREGS *regs, void *buffer, long size)
{
	RMSREGS sregs;
	union REGS r;

	if (!VESABuf_sel)  //Init Real mode buffer
	{
		r.w.ax = 0x100; r.w.bx = 1024>>4;
		backupsegs(); int386(0x31,&r,&r); restoresegs();
		if (r.w.cflag) { printf("DPMI_allocRealSeg failed!\n"); exit(0); }
		VESABuf_sel = r.w.dx;
		VESABuf_rseg = r.w.ax;
		atexit(ExitVBEBuf);
	}
	sregs.es = VESABuf_rseg;
	regs->x.di = 0;
	_fmemcpy(MK_FP(VESABuf_sel,0),buffer,size);
	DPMI_int86x(0x10,regs,regs,&sregs);
	_fmemcpy(buffer,MK_FP(VESABuf_sel,0),size);
}

void getvalidvesamodes ()
{
	long i, j, k;
	short *p, *p2;
	VBE_modeInfo modeInfo;
	RMREGS regs;

	if (vesachecked) return;
	vesachecked = 1;

	modelist[0] = -1;

	strncpy(vgaInfo.VESASignature,"VBE2",4);
	regs.x.ax = 0x4f00;
	VBE_callESDI(&regs,&vgaInfo,sizeof(VBE_vgaInfo));
	if ((regs.x.ax != 0x004f) || (strncmp(vgaInfo.VESASignature,"VESA",4)))
		return;

	//if (vgaInfo.VESAVersion < 0x200) return;

		//LfbMapRealPointer
	p = (short *)(((vgaInfo.VideoModePtr&0xffff0000)>>12)+((vgaInfo.VideoModePtr)&0xffff));
	p2 = modelist;
	while (*p != -1) *p2++ = *p++;
	*p2 = -1;

	validmodecnt = 0;
	for(p=modelist;*p!=-1;p++)
	{
		regs.x.ax = 0x4f01; regs.x.cx = *p;
		VBE_callESDI(&regs,&modeInfo,sizeof(VBE_modeInfo));
		if (regs.x.ax != 0x004f) continue;
		if (!(modeInfo.ModeAttributes&1)) continue; //1 is vbeMdAvailable
		//if (modeInfo.MemoryModel != 4) continue;    //4 is vbeMemPK
		if ((modeInfo.BitsPerPixel != 8) &&
			 (modeInfo.BitsPerPixel != 15) &&
			 (modeInfo.BitsPerPixel != 16) &&
			 (modeInfo.BitsPerPixel != 24) &&
			 (modeInfo.BitsPerPixel != 32)) continue;
		//if (modeInfo.NumberOfPlanes != 1) continue;

		validmode[validmodecnt] = *p;
		validmodexdim[validmodecnt] = modeInfo.XResolution;
		validmodeydim[validmodecnt] = modeInfo.YResolution;
		validmodebits[validmodecnt] = modeInfo.BitsPerPixel;
		validmodecnt++;
	}

		//Sort modes
	for(j=1;j<validmodecnt;j++)
		for(k=0;k<j;k++)
			if ((validmodexdim[j] < validmodexdim[k]) ||
				 ((validmodexdim[j] == validmodexdim[k]) && (validmodeydim[j] < validmodeydim[k])) ||
				 ((validmodexdim[j] == validmodexdim[k]) && (validmodeydim[j] == validmodeydim[k]) && (validmodebits[j] < validmodebits[k])))
			{
				i = validmode[j]; validmode[j] = validmode[k]; validmode[k] = i;
				i = validmodexdim[j]; validmodexdim[j] = validmodexdim[k]; validmodexdim[k] = i;
				i = validmodeydim[j]; validmodeydim[j] = validmodeydim[k]; validmodeydim[k] = i;
				i = validmodebits[j]; validmodebits[j] = validmodebits[k]; validmodebits[k] = i;
			}
}

long setvesa (long x, long y, long colbits)
{
	div_t dived;
	long i, j, k;
	short *p, *p1, *p2;
	VBE_modeInfo modeInfo;
	union REGS r;
	RMREGS regs;
	RMSREGS sregs;

	getvalidvesamodes();

	xres = x; yres = y;
	for(k=0;k<validmodecnt;k++)
	{
		regs.x.ax = 0x4f01; regs.x.cx = validmode[k];
		VBE_callESDI(&regs,&modeInfo,sizeof(VBE_modeInfo));
		if (regs.x.ax != 0x004f) continue;
		if (!(modeInfo.ModeAttributes&1)) continue; //1 is vbeMdAvailable

		//if (modeInfo.MemoryModel != 4) continue;    //4 is vbeMemPK
		if (modeInfo.BitsPerPixel != colbits) continue;
		//if (modeInfo.NumberOfPlanes != 1) continue;
		if (modeInfo.XResolution != x) continue;
		if (modeInfo.YResolution != y) continue;

		bytesperline = modeInfo.BytesPerScanLine;
		maxpages = min(modeInfo.NumberOfImagePages+1,64);

mapphysicaltolinearfailed:;
		regs.x.ax = 0x4f02;
		regs.x.bx = validmode[k] | ((modeInfo.ModeAttributes&0x0080)<<7);
		DPMI_int86(0x10,&regs,&regs);

			//Linear mode
		if (modeInfo.ModeAttributes&0x0080) //0x0080 is vbeMdLinear
		{
			r.w.ax = 0x800; r.w.si = 0x3f; r.w.di = 0xffff; //Map hardcoded to 4M
			r.w.bx = modeInfo.PhysBasePtr >> 16;
			r.w.cx = modeInfo.PhysBasePtr & 0xffff;
			backupsegs(); int386(0x31,&r,&r); restoresegs();
			if (r.x.cflag)
			{
				modeInfo.ModeAttributes &= ~0x0080; goto mapphysicaltolinearfailed;
				/*printf("DPMI_mapPhysicalToLinear() failed!\n"); exit(0);*/
			}
			/*backlinaddress = */globlinplace = ((long)r.w.bx<<16)+r.w.cx;

			linearmode = 1;
			buffermode = (maxpages<=1);
			imageSize = bytesperline*yres;
			frameplace = globlinplace;

			j = 0;
			for(i=0;i<maxpages;i++)
			{
				activepagelookup[i] = globlinplace+j;
				j += imageSize;
			}
		}
		else  //Banked mode
		{
				//Get granularity
			switch(modeInfo.WinGranularity)
			{
				case 64: davesapageshift = 0; break;
				case 32: davesapageshift = 1; break;
				case 16: davesapageshift = 2; break;
				case 8: davesapageshift = 3; break;
				case 4: davesapageshift = 4; break;
				case 2: davesapageshift = 5; break;
				case 1: davesapageshift = 6; break;
			}

			linearmode = 0;
			if ((x == 320) && (y == 200) && (colbits == 8) && (maxpages >= 2))
			{
				buffermode = 0;
				imageSize = 65536;
				frameplace = 0xa0000;
			}
			else
			{
				buffermode = 1;
				imageSize = bytesperline*yres;
				maxpages = 1;
			}
		}

		if (buffermode)
		{
			if ((screen = (char *)malloc(imageSize+255)) == NULL)
			{
				printf("Not enough memory for VESA screen-buffering\n");
				exit(0);
			}
			frameplace = ((((long)screen)+255)&~255);

			for(i=0;i<maxpages;i++)
				activepagelookup[i] = frameplace;
		}

		j = 0;
		for(i=0;i<maxpages;i++)
		{
			dived = div(j,bytesperline);
			visualpagelookup[i][0] = (short)dived.rem;
			visualpagelookup[i][1] = (short)dived.quot;
			j += imageSize;
		}

		if (modeInfo.ModeAttributes&32) vgacompatible = 0;
											else vgacompatible = 1;

		RedMaskSize = modeInfo.RedMaskSize;
		GreenMaskSize = modeInfo.GreenMaskSize;
		BlueMaskSize = modeInfo.BlueMaskSize;
		RedFieldPosition = modeInfo.RedFieldPosition;
		GreenFieldPosition = modeInfo.GreenFieldPosition;
		BlueFieldPosition = modeInfo.BlueFieldPosition;

		regs.x.ax = 0x4f0a; regs.x.bx = 0; regs.x.cx = 0; regs.x.dx = 0;
		DPMI_int86x(0x10,&regs,&regs,&sregs);
		if (regs.x.ax == 0x004f)   //cx is length of protected mode code
		{
			i = (((long)sregs.es)<<4)+((long)regs.x.di);
			p1 = (short *)i;
			setactiveentry = ((long)p1[0]) + i;
			setvisualentry = ((long)p1[1]) + i;
			setpaletteentry = ((long)p1[2]) + i;
				//p1[2] is useless palette function - see vesprot.asm for code
		}

		return(1);
	}
	return(-1);
}

long setdacbits (long newdacbits)
{
	RMREGS regs;

	if ((vgaInfo.Capabilities&1) == 0) return(6L);
	regs.x.ax = 0x4f08;
	regs.x.bx = (((long)newdacbits)<<8);
	DPMI_int86(0x10,&regs,&regs);
	return(8L); //(long)regs.h.bh);
}

#define setvesapage(i)                                      \
{                                                           \
	if (setactiveentry)                                      \
		vesasetactive(0x4f05,0L,0L,i);                        \
	else                                                     \
	{                                                        \
		regs.x.ax = 0x4f05; regs.x.bx = 0; regs.x.dx = i;     \
		DPMI_int86(0x10,&regs,&regs);                         \
	}                                                        \
}

void setactivepage (long dapagenum)
{
	RMREGS regs;

	if (buffermode != 0) { frameplace = ((((long)screen)+255)&~255); return; }
	if (linearmode != 0) { frameplace = activepagelookup[dapagenum]; return; }
	setvesapage(dapagenum);
}

void setvisualpage (long dapagenum)
{
	RMREGS regs;
	long i, j;

	if (buffermode == 0)
	{
		if (setvisualentry)
		{
			i = imageSize*(dapagenum&0x7fffffff);
			if (dapagenum >= 0) qlimitrate();
			vesasetvisual(0L,0L,i>>2,i>>18);
		}
		else
		{
			regs.x.ax = 0x4f07; regs.x.bx = 0;
			regs.x.cx = visualpagelookup[dapagenum&0x7fffffff][0]; //X-coordinate
			regs.x.dx = visualpagelookup[dapagenum&0x7fffffff][1]; //Y-coordinate
			if (dapagenum >= 0) qlimitrate();
			DPMI_int86(0x10,&regs,&regs);
		}
		return;
	}
	for(i=0;i<imageSize;i+=65536)
	{
		if (linearmode == 0)
		{
			setvesapage((i>>16)<<davesapageshift);
			copybufbyte(frameplace+i,0xa0000,min(imageSize-i,65536));
		}
		else
			copybufbyte(frameplace+i,globlinplace+i,min(imageSize-i,65536));
	}
}

void uninitvesa ()
{
	//if (backlinaddress)
	//{
	//   union REGS r;
	//   r.w.ax = 0x801;
	//   r.w.bx = (backlinaddress >> 16);
	//   r.w.cx = (backlinaddress & 0xffff);
	//   backupsegs(); int386(0x31,&r,&r); restoresegs();
	//   if (r.x.cflag) { printf("Free Physical Address failed!\n"); }
	//   backlinaddress = 0;
	//}
	if (screen != NULL) { free(screen); screen = NULL; }
	VESABuf_sel = 0;
	vesachecked = 0;
}

long VBE_setPalette (long start, long num, char *dapal)
{
	RMREGS regs;
	long i;

	if ((vgacompatible) || (vgaInfo.VESAVersion < 0x200)) // || (vidoption != 1))
	{
		koutp(0x3c8,start);
		for(i=(num>>1);i>0;i--)
		{
			koutp(0x3c9,dapal[2]);
			while (kinp(0x3da)&1); while (!(kinp(0x3da)&1));
										  koutp(0x3c9,dapal[1]); koutp(0x3c9,dapal[0]);
			koutp(0x3c9,dapal[6]); koutp(0x3c9,dapal[5]); koutp(0x3c9,dapal[4]);
			dapal += 8;
		}
		if (num&1)
		{
			koutp(0x3c9,dapal[2]);
			while (kinp(0x3da)&1); while (!(kinp(0x3da)&1));
										  koutp(0x3c9,dapal[1]); koutp(0x3c9,dapal[0]);
		}
		return(1);
	}

	if (setpaletteentry)
	{
		i = (vesasetpalette(0x4f09,(vgaInfo.Capabilities&4)<<5,num,start,0L,(long)dapal)&65535);
	}
	else
	{
		regs.x.ax = 0x4f09; regs.h.bl = ((vgaInfo.Capabilities&4)<<5);
		regs.x.cx = num; regs.x.dx = start;
		VBE_callESDI(&regs,dapal,sizeof(dapal)*num);
		i = regs.x.ax;
	}
	if (i != 0x004f) return(0);
	return(1);
}

long VBE_getPalette (long start, long num, char *dapal)
{
	RMREGS regs;
	long i;

	if ((vgacompatible) || (vgaInfo.VESAVersion < 0x200)) // || (vidoption != 1))
	{
		koutp(0x3c7,start);
		for(i=num;i>0;i--)
		{
			dapal[2] = kinp(0x3c9);
			dapal[1] = kinp(0x3c9);
			dapal[0] = kinp(0x3c9);
			dapal += 4;
		}
		return(1);
	}

	regs.x.ax = 0x4f09; regs.h.bl = 1;
	regs.x.cx = num; regs.x.dx = start;
	VBE_callESDI(&regs,dapal,sizeof(dapal)*num);
	i = regs.x.ax;
	if (i != 0x004f) return(0);
	return(1);
}
