#if 0
kbarf.exe: kbarf.c; cl kbarf.c /Ox /G6fy /MD /link /opt:nowin98
	del kbarf.obj
!if 0
#endif

//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#define MAX_PATH 260
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>

#define MAXFILES 4096
#define BUFSIZE 16384

static char buf[BUFSIZE];
static char encrypt = 0;
static char encbyte = 0;

static long numfiles, anyfiles4extraction;
static char filelist[MAXFILES][16], marked4extraction[MAXFILES];
static long fileoffs[MAXFILES], fileleng[MAXFILES];

	//Brute-force case-insensitive, slash-insensitive, * and ? wildcard matcher
	//Given: string i and string j. string j can have wildcards
	//Returns: 1:matches, 0:doesn't match
static long wildmatch (const char *i, const char *j)
{
	const char *k;
	char c0, c1;

	if (!*j) return(1);
	do
	{
		if (*j == '*')
		{
			for(k=i,j++;*k;k++) if (wildmatch(k,j)) return(1);
			continue;
		}
		if (!*i) return(0);
		if (*j == '?') { i++; j++; continue; }
		c0 = *i; if ((c0 >= 'a') && (c0 <= 'z')) c0 -= 32;
		c1 = *j; if ((c1 >= 'a') && (c1 <= 'z')) c1 -= 32;
		if (c0 == '/') c0 = '\\';
		if (c1 == '/') c1 = '\\';
		if (c0 != c1) return(0);
		i++; j++;
	} while (*j);
	return(!*i);
}

static void findfiles (char *dafilespec)
{
	long i;
	for(i=numfiles-1;i>=0;i--)
		if (wildmatch(filelist[i],dafilespec))
			{ marked4extraction[i] = 1; anyfiles4extraction = 1; }
}

int main (int argc, char **argv)
{
	FILE *fil, *fil2;
	long i, j, k, l, offs;
	char stuffile[MAX_PATH], filename[MAX_PATH];

	if (argc < 3)
	{
		printf("KBARF [Blood .RFF file] [@file or filespec...]              by Ken Silverman\n");
		printf("   This program extracts files from a Blood Resource File (.RFF)\n");
		printf("   You can extract files using the ? and * wildcards.\n");
		printf("   Example 1: C:\\BLOOD>kbarf sounds.rff *.*\n");
		printf("   Example 2: C:\\BLOOD>kbarf blood.rff *.kvx *.mid e1m1.map\n");
		printf("\n");
		printf("KBARF IS NOT MADE BY OR SUPPORTED BY Monolith Productions, GT Interactive\n");
		printf(" Software, The WizardWorks Group, or any of their affiliates and subsidiaries.\n");
		return(0);
	}

	strcpy(stuffile,argv[1]);

	if (!(fil = fopen(stuffile,"rb")))
	{
		printf("Error: %s could not be opened\n",stuffile);
		return(-1);
	}

	fread(buf,32,1,fil);
	if ((buf[0] != 'R') || (buf[1] != 'F') || (buf[2] != 'F') || (buf[3] != 0x1a))
	{
		fclose(fil);
		printf("Error: %s not a valid .RFF file\n",stuffile);
		return(-1);
	}

	if (buf[5] == 3) encrypt = 1;
	encbyte = buf[4];

	offs = *(long *)&buf[8];
	numfiles = *(long *)&buf[12];

	if (numfiles > MAXFILES)
	{
		printf("Too many files in .RFF (>%d)\n",MAXFILES);
		return(-1);
	}

	fseek(fil,offs+16,SEEK_SET);
	l = BUFSIZE;
	for(i=0;i<numfiles;i++)
	{
		if (l >= BUFSIZE-48)
		{
			j = numfiles-i;
			if ((BUFSIZE/48) < j) j = (BUFSIZE/48);
			j *= 48;
			fread(buf,j,1,fil);
			if (encrypt) //Fix encryption
			{
				l = offs+16+i*48 + offs*encbyte;
				for(j--;j>=0;j--) buf[j] ^= (((l+j)>>1)&255);
			}
			l = 0;
		}

		fileoffs[i] = *(long *)&buf[l+0];
		fileleng[i] = *(long *)&buf[l+4];

			//Fix .RFF filenames
		j = 0;
		for(k=20;(k<28)&&(buf[l+k]);k++) filelist[i][j++] = buf[l+k];
		filelist[i][j++] = '.';
		for(k=17;(k<20)&&(buf[l+k]);k++) filelist[i][j++] = buf[l+k];
		filelist[i][j++] = 0;
		filelist[i][13] = (buf[l+16]>>4)&1; //test encryption flag

		l += 48;
	}

	for(i=0;i<numfiles;i++) marked4extraction[i] = 0;

	anyfiles4extraction = 0;
	for(i=argc-1;i>1;i--)
	{
		strcpy(filename,argv[i]);
		if (filename[0] == '@')
		{
			if (fil2 = fopen(&filename[1],"rb"))
			{
				l = fread(buf,8192,1,fil2);
				j = 0;
				while ((j < l) && (buf[j] <= 32)) j++;
				while (j < l)
				{
					k = j;
					while ((k < l) && (buf[k] > 32)) k++;

					buf[k] = 0;
					findfiles(&buf[j]);
					j = k+1;

					while ((j < l) && (buf[j] <= 32)) j++;
				}
				fclose(fil2);
			}
		}
		else
			findfiles(filename);
	}

	if (anyfiles4extraction == 0)
	{
		fclose(fil);
		printf("No files found in group file with those names\n");
		return(-1);
	}

	for(i=0;i<numfiles;i++)
	{
		if (marked4extraction[i] == 0) continue;

		if (!(fil2 = fopen(filelist[i],"wb")))
		{
			printf("Error: Could not write to %s\n",filelist[i]);
			continue;
		}
		printf("Extracting %s...\n",filelist[i]);
		fseek(fil,fileoffs[i],SEEK_SET);
		for(j=0;j<fileleng[i];j+=BUFSIZE)
		{
			k = fileleng[i]-j; if (k > BUFSIZE) k = BUFSIZE;
			fread(buf,k,1,fil);

			if ((filelist[i][13]) && (j < 256)) //Fix encryption
				for(l=0;l<256;l++)
					buf[l] ^= ((l>>1)&255);

			if (fwrite(buf,k,1,fil2) < 1)
			{
				printf("Write error (drive full?)\n");
				fclose(fil2);
				fclose(fil);
				return(-1);
			}
		}
		fclose(fil2);
	}
	fclose(fil);
	return(0);
}

#if 0
!endif
#endif
