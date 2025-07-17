#if 0
kv62stl.exe: kv62stl.c; cl kv62stl.c /Ox /G6Fy /MD /link /opt:nowin98
	del kv62stl.obj
!if 0
#endif

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char **argv)
{
	FILE *fil;
	#pragma pack(push,1)
	typedef struct { unsigned char b, g, r, a; unsigned short z; unsigned char vis, norm; } surf_t;
	typedef struct { float x, y, z; } point3d;
	typedef struct { point3d norm, pt[3]; unsigned short attr; } tri_t;
	#pragma pack(pop)
	surf_t *surf;
	tri_t tri;
	float f;
	int i, j, k, n, x, y, z, vis, numvoxs, xsiz, ysiz, zsiz, *xstart, numtris;
	unsigned short u, *xyoffs;
	char buf[1024];
	static const int lut[6][4][3] =
	{
		0,0,0, 0,1,0, 0,1,1, 0,0,1,
		1,1,0, 1,0,0, 1,0,1, 1,1,1,
		1,0,0, 0,0,0, 0,0,1, 1,0,1,
		0,1,0, 1,1,0, 1,1,1, 0,1,1,
		0,0,0, 1,0,0, 1,1,0, 0,1,0,
		0,1,1, 1,1,1, 1,0,1, 0,0,1,
	};

	if ((argc < 2) || (argc > 3)) { printf("kv62stl [KV6 file] (STL file)                    by Ken Silverman (%s)\n   Converts KV6 voxel format to STL file.",__DATE__); return(1); }

	fil = fopen(argv[1],"rb");
	if (!fil) { printf("Couldn't read %s",argv[1]); return(1); }
	printf("Reading %s..\n",argv[1]);

	fread(&i,4,1,fil); if (i != 0x6c78764b) { printf("Not a KV6 file"); return(1); }
	fread(&xsiz,4,1,fil);
	fread(&ysiz,4,1,fil);
	fread(&zsiz,4,1,fil);
	fseek(fil,12,SEEK_CUR); //skip pivots
	fread(&numvoxs,4,1,fil);

	surf = (surf_t *)malloc(numvoxs*sizeof(surf_t)); if (!surf) { printf("malloc failed"); return(1); }
	xstart = (int *)malloc(xsiz*4); if (!xstart) { printf("malloc failed"); return(1); }
	xyoffs = (unsigned short *)malloc(xsiz*ysiz*2); if (!xyoffs) { printf("malloc failed"); return(1); }

	fread(surf,numvoxs*sizeof(surf_t),1,fil);
	fread(xstart,xsiz*4,1,fil);
	fread(xyoffs,xsiz*ysiz*2,1,fil);
	fclose(fil);

	if (argc == 3)
	{
		if (sizeof(buf) < strlen(argv[2])+1) { printf("Buffer overflow detected. Computer asplode now."); return(1); }
		strcpy(buf,argv[2]);
	}
	else
	{
		j = strlen(argv[1]);
		for(i=0;argv[1][i];i++)
			if (argv[1][i] == '.') j = i;
		if (sizeof(buf) < strlen(argv[1])+1+4) { printf("Buffer overflow detected. Computer asplode now."); return(1); }
		strcpy(buf,argv[1]);
		strcpy(&buf[j],".stl");
	}
	fil = fopen(buf,"rb");
	if (fil)
	{
		fclose(fil);
		printf("%s already exists. Overwrite? (y/n)\n",buf);
		do
		{
			i = getc(stdin);
			if ((i == 27) || (i == 'N') || (i == 'n')) { printf("Operation cancelled.\n"); return(1); }
			if ((i == 0) || (i == 0xe0)) i = getc(stdin);
		} while ((i != 'Y') && (i != 'y'));
	}
	fil = fopen(buf,"wb"); if (!fil) { printf("Couldn't write %s",buf); return(1); }
	printf("Writing %s..\n",buf);
	memset(buf,0,80); fwrite(buf,80,1,fil); //fill 80 bytes of 0's for header
	numtris = 0; fwrite(&numtris,4,1,fil); //number of triangles (to be filled later)

	i = 0;
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
		{
			n = xyoffs[x*ysiz+y];
			for(j=0;j<n;j++,i++)
			{
				z = surf[i].z;
				vis = surf[i].vis;

				for(k=0;k<6;k++)
				{
					if (!(vis&(1<<k))) continue;

						// 0---1
						// | \ |
						// 3---2
					tri.pt[0].x = (float)(x+lut[k][0][0]);
					tri.pt[0].y = (float)(y+lut[k][0][1]);
					tri.pt[0].z = (float)(z+lut[k][0][2]);
					tri.pt[1].x = (float)(x+lut[k][2][0]);
					tri.pt[1].y = (float)(y+lut[k][2][1]);
					tri.pt[1].z = (float)(z+lut[k][2][2]);
					tri.pt[2].x = (float)(x+lut[k][1][0]);
					tri.pt[2].y = (float)(y+lut[k][1][1]);
					tri.pt[2].z = (float)(z+lut[k][1][2]);

					tri.norm.x = (tri.pt[1].y-tri.pt[0].y)*(tri.pt[2].z-tri.pt[0].z) - (tri.pt[1].z-tri.pt[0].z)*(tri.pt[2].y-tri.pt[0].y);
					tri.norm.y = (tri.pt[1].z-tri.pt[0].z)*(tri.pt[2].x-tri.pt[0].x) - (tri.pt[1].x-tri.pt[0].x)*(tri.pt[2].z-tri.pt[0].z);
					tri.norm.z = (tri.pt[1].x-tri.pt[0].x)*(tri.pt[2].y-tri.pt[0].y) - (tri.pt[1].y-tri.pt[0].y)*(tri.pt[2].x-tri.pt[0].x);
					f = 1.f/sqrt(tri.norm.x*tri.norm.x + tri.norm.y*tri.norm.y + tri.norm.z*tri.norm.z);
					tri.norm.x *= f; tri.norm.y *= f; tri.norm.z *= f;

					tri.attr = (surf[i].b>>3) +  ((surf[i].g>>3)<<5) + ((surf[i].r>>3)<<10) + (1<<15);

					fwrite(&tri,sizeof(tri),1,fil);
					numtris++;

					tri.pt[1].x = (float)(x+lut[k][3][0]);
					tri.pt[1].y = (float)(y+lut[k][3][1]);
					tri.pt[1].z = (float)(z+lut[k][3][2]);
					tri.pt[2].x = (float)(x+lut[k][2][0]);
					tri.pt[2].y = (float)(y+lut[k][2][1]);
					tri.pt[2].z = (float)(z+lut[k][2][2]);

					fwrite(&tri,sizeof(tri),1,fil);
					numtris++;
				}
			}
		}

	i = ftell(fil);
	fseek(fil,80,SEEK_SET);
	fwrite(&numtris,4,1,fil);
	fseek(fil,i,SEEK_SET);
	fclose(fil);

	if (xyoffs) { free(xyoffs); xyoffs = 0; }
	if (xstart) { free(xstart); xstart = 0; }
	if (surf) { free(surf); surf = 0; }

	printf("Done.\n");
	return(0);
}

#if 0
!endif
#endif
