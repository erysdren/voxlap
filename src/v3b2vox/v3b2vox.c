#if 0
v3b2vox.exe: v3b2vox.c; cl v3b2vox.c /Ox /G6Fy /MD /link /opt:nowin98
	del v3b2vox.obj
!if 0
#endif

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

FILE *rfil;
char *newbuf = 0;
long newbufplc = 0, newbufsiz = 0, newbufmal = 0;

void putbuf (char *buf, long leng)
{
	if (newbufsiz+leng > newbufmal)
	{
		newbufmal <<= 1;
		if (newbufsiz+leng > newbufmal) newbufmal = newbufsiz+leng;
		if (newbufmal < 65536) newbufmal = 65536;
		newbuf = realloc(newbuf,newbufmal);
	}
	memcpy(&newbuf[newbufsiz],buf,leng); newbufsiz += leng;
}

#define MAXPALENTRIES 255
static long pal[256], palentries = 0;
long rgb2pal (long rgb)
{
	static palhashead[2048], palhashnext[MAXPALENTRIES];
	long i, hval;

	hval = ((((rgb>>22)^(rgb>>11))-rgb)&2047);
	if (!palentries)
	{
		memset(palhashead,-1,sizeof(palhashead));
			//Force palette index 0 to black (due to Windows palette issues)
		pal[0] = 0; palhashnext[0] = palhashead[hval]; palhashead[hval] = 0; palentries = 1;
	}

	for(i=palhashead[hval];i>=0;i=palhashnext[i])
		if (pal[i] == rgb) return(i);
	if (palentries >= MAXPALENTRIES) return(-1);
	pal[palentries] = rgb;
	palhashnext[palentries] = palhashead[hval]; palhashead[hval] = palentries; palentries++;
	return(palentries-1);
}

void getst (char *st)
{
	for(;st[0];st++) while (newbuf[newbufplc++] != st[0]);
}

double getnum ()
{
	double base = 0.0, basesgn = 1.0, expo = 0.0, exposgn = 1.0;
	int inexpo = 0, indec = 0;
	char ch;

	while ((newbuf[newbufplc] == 13) || (newbuf[newbufplc] == 10) || (newbuf[newbufplc] == 32)) newbufplc++;
	while (1)
	{
		ch = newbuf[newbufplc]; newbufplc++;
		if ((ch >= '0') && (ch <= '9'))
		{
			if (!inexpo) { base = base*10.0 + ch-'0'; if (indec) basesgn *= 0.1; }
					  else { expo = expo*10.0 + ch-'0'; if (indec) exposgn *= 0.1; }
		}
		else if (ch == '.') indec = 1;
		else if ((ch == 'E') || (ch == 'e')) { inexpo = 1; indec = 0; }
		else if (ch == '-') { if (!inexpo) basesgn *= -1.0; else exposgn *= -1.0; }
		else if (ch != '+') break;
	}
	return(pow(10.0,expo*exposgn)*base*basesgn);
}

//--------------------------------------------------------------------------------------------------
static unsigned char *filptr, slidebuf[32768];
static long clen[320], cclen[19], ccind[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
static long bitpos, hxbit[59][2], ibuf0[288], nbuf0[32], ibuf1[32], nbuf1[32];

static long getbits (long n)
{
	long i = ((*(long *)&filptr[bitpos>>3])>>(bitpos&7))&((1<<n)-1);
	bitpos += n; return(i);
}

static long hufgetsym (long *hitab, long *hbmax)
{
	long v, n; v = n = 0;
	do { v = (v<<1)+getbits(1)+hbmax[n]-hbmax[n+1]; n++; } while (v >= 0);
	return(hitab[hbmax[n]+v]);
}

static void hufgencode (long *inbuf, long inum, long *hitab, long *hbmax)
{
	long i, tbuf[31];

	for(i=30;i;i--) tbuf[i] = 0;
	for(i=inum-1;i>=0;i--) tbuf[inbuf[i]]++;
	tbuf[0] = hbmax[0] = 0; //Hack to remove symbols of length 0?
	for(i=0;i<31;i++) hbmax[i+1] = hbmax[i]+tbuf[i];
	for(i=0;i<inum;i++) if (inbuf[i]) hitab[hbmax[inbuf[i]]++] = i;
}

long inflate (char *kfilebuf, long compleng, void (*putbuf)(char *,long ))
{
	long i, j, k, bfinal, btype, hlit, hdist, leng, slidew, slider;

	j = 1; k = 0;
	for(i=0;i<30;i++)
	{
		hxbit[i][1] = j; j += (1<<k);
		hxbit[i][0] = k; k += ((i&1) && (i >= 2));
	}
	j = 3; k = 0;
	for(i=257;i<285;i++)
	{
		hxbit[i+30-257][1] = j; j += (1<<k);
		hxbit[i+30-257][0] = k; k += ((!(i&3)) && (i >= 264));
	}
	hxbit[285+30-257][1] = 258; hxbit[285+30-257][0] = 0;

	filptr = kfilebuf; bitpos = 0; //Init for getbits()
	slidew = 0; slider = 16384;
	bitpos += 16; //skip 2 fields: 8:compmethflags, 8:addflagscheck
	do
	{
		if (bitpos >= (compleng<<3)) break;
		bfinal = getbits(1); btype = getbits(2);
		if (!btype) //Raw (uncompressed)
		{
			bitpos += ((-bitpos)&7);
			i = getbits(16); if ((getbits(16)^i) != 0xffff) return(-1);
			for(;i;i--)
			{
				if (slidew >= slider) { putbuf(&slidebuf[(slider-16384)&32767],16384); slider += 16384; }
				slidebuf[(slidew++)&32767] = (char)getbits(8);
			}
			continue;
		}
		if (btype == 1) //Fixed Huffman
		{
			hlit = 288; hdist = 32; i = 0;
			for(;i<144;i++) clen[i] = 8; //Fixed bit sizes (literals)
			for(;i<256;i++) clen[i] = 9; //Fixed bit sizes (literals)
			for(;i<280;i++) clen[i] = 7; //Fixed bit sizes (EOI,lengths)
			for(;i<288;i++) clen[i] = 8; //Fixed bit sizes (lengths)
			for(;i<320;i++) clen[i] = 5; //Fixed bit sizes (distances)
		}
		else //Dynamic Huffman
		{
			hlit = getbits(5)+257; hdist = getbits(5)+1; j = getbits(4)+4;
			for(i=0;i<j;i++) cclen[ccind[i]] = getbits(3);
			for(;i<19;i++) cclen[ccind[i]] = 0;
			hufgencode(cclen,19,ibuf0,nbuf0);

			j = 0; k = hlit+hdist;
			while (j < k)
			{
				i = hufgetsym(ibuf0,nbuf0);
				if (i < 16) { clen[j++] = i; continue; }
				if (i == 16)
					{ for(i=getbits(2)+3;i;i--) { clen[j] = clen[j-1]; j++; } }
				else
				{
					if (i == 17) i = getbits(3)+3; else i = getbits(7)+11;
					for(;i;i--) clen[j++] = 0;
				}
			}
		}

		hufgencode(clen,hlit,ibuf0,nbuf0);
		hufgencode(&clen[hlit],hdist,ibuf1,nbuf1);

		while (1)
		{
			if (slidew >= slider) { putbuf(&slidebuf[(slider-16384)&32767],16384); slider += 16384; }
			i = hufgetsym(ibuf0,nbuf0);
			if (i < 256) { slidebuf[(slidew++)&32767] = (char)i; continue; }
			if (i == 256) break;
			i = getbits(hxbit[i+30-257][0]) + hxbit[i+30-257][1];
			j = hufgetsym(ibuf1,nbuf1);
			j = getbits(hxbit[j][0]) + hxbit[j][1];
			i += slidew; do { slidebuf[slidew&32767] = slidebuf[(slidew-j)&32767]; slidew++; } while (slidew < i);
		}
	} while (!bfinal);

	slider -= 16384;
	if (!((slider^slidew)&32768))
		putbuf(&slidebuf[slider&32767],slidew-slider);
	else
	{
		putbuf(&slidebuf[slider&32767],(-slider)&32767);
		putbuf(slidebuf,slidew&32767);
	}
	return(0);
}
//--------------------------------------------------------------------------------------------------

int main (int argc, char **argv)
{
	FILE *wfil;
	long i, x, y, z, xsiz, ysiz, zsiz, r, g, b, a, *tcol, compleng;
	char ch, *compbuf;

	if (argc < 3)
	{
		printf("V3A/V3B to VOX converter                       by Ken Silverman (05/14/2007)\n");
		printf("Example #1: >v3b2vox model.v3b model.vox\n");
		printf("Example #2: >for %%i in (*.v3*) do v3b2vox \"%%i\" \"%%~ni.vox\"");
		exit(0);
	}
	rfil = fopen(argv[1],"rb"); if (!rfil) { printf("File not found: %s",argv[1]); exit(0); }
	fseek(rfil,0,SEEK_END);
	ch = argv[1][strlen(argv[1])-1];
	if ((ch == 'A') || (ch == 'a'))
	{
		newbufmal = ftell(rfil); fseek(rfil,0,SEEK_SET);
		newbuf = (char *)malloc(newbufmal);
		fread(newbuf,newbufmal,1,rfil);
	}
	else
	{
		compleng = ftell(rfil); fseek(rfil,0,SEEK_SET);
		compbuf = (char *)malloc(compleng);
		fread(compbuf,compleng,1,rfil);
		inflate(compbuf,compleng,putbuf);
		free(compbuf);
	}
	fclose(rfil);

	wfil = fopen(argv[2],"rb");
	if (wfil)
	{
		fclose(wfil);
		printf("%s already exists. Overwrite? (y/n)",argv[2]);
		do
		{
			ch = getch();
			if ((ch == 27) || (ch == 'N') || (ch == 'n')) { printf("\nOperation cancelled."); exit(0); }
			if ((ch == 0) || (ch == 0xe0)) ch = getch();
		} while ((ch != 'Y') && (ch != 'y'));
	}

	wfil = fopen(argv[2],"wb"); if (!wfil) { printf("Can't write file: %s",argv[2]); exit(0); }
	getst("VERSION"); if (getnum() != 1.f) { printf("Incorrect version"); exit(0); }
	getst("\nTYPE VoxelCubic\nDIMENSION"); getnum(); getnum(); getnum();
	getst("\nSIZE"); xsiz = (long)getnum(); ysiz = (long)getnum(); zsiz = (long)getnum(); getst("\nDATA");
	fwrite(&xsiz,4,1,wfil);
	fwrite(&ysiz,4,1,wfil);
	fwrite(&zsiz,4,1,wfil);

	tcol = (long *)malloc(ysiz*zsiz*sizeof(tcol[0]));
	for(x=0;x<xsiz;x++)
	{
		for(y=ysiz-1;y>=0;y--)
			for(z=zsiz-1;z>=0;z--)
			{
				r = getnum(); g = getnum(); b = getnum(); a = getnum();
				if (a >= 0) tcol[y*zsiz+z] = rgb2pal((r<<16)+(g<<8)+b); else tcol[y*zsiz+z] = 255;
			}
		for(y=0;y<ysiz;y++)
			for(z=0;z<zsiz;z++) fputc(tcol[y*zsiz+z],wfil);
	}
	free(tcol); free(newbuf);
	for(i=0;i<palentries;i++)
	{
		fputc((pal[i]>>16)&255,wfil);
		fputc((pal[i]>> 8)&255,wfil);
		fputc((pal[i]    )&255,wfil);
	}
	for(;i<256;i++)
	{
		fputc(rand()&255,wfil);
		fputc(rand()&255,wfil);
		fputc(rand()&255,wfil);
	}

	fclose(wfil);
}

#if 0
!endif
#endif
