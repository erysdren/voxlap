//COLOOK.H by Ken Silverman (http://advsys.net/ken)
//WARNING: Input palette is in weird format:
//   long palette[256][4];

char closestcol[262144];

void initclosestcolorfast (long *dapal)  //Fast approximate octahedron method
{
	long i, j, *clospos, *closend, *closcan;

	if (!(clospos = closend = closcan = malloc(262144*4)))
		{ printf("Out of memory!\n"); exit(0); }
	memset(closestcol,-1,sizeof(closestcol));
	for(j=0;j<255;j++)
	{
		i = (dapal[j<<2]<<12)+(dapal[(j<<2)+1]<<6)+dapal[(j<<2)+2];
		if (closestcol[i] == 255) { *closend++ = i; closestcol[i] = j; }
	}
	do
	{
		i = *clospos++; j = closestcol[i];
		if ((i&0x3f000)          && (closestcol[i-4096] == 255)) { closestcol[i-4096] = j; *closend++ = i-4096; }
		if ((i < 0x3f000)        && (closestcol[i+4096] == 255)) { closestcol[i+4096] = j; *closend++ = i+4096; }
		if ((i&0xfc0)            && (closestcol[i-  64] == 255)) { closestcol[i-  64] = j; *closend++ = i-  64; }
		if (((i&0xfc0) != 0xfc0) && (closestcol[i+  64] == 255)) { closestcol[i+  64] = j; *closend++ = i+  64; }
		if ((i&0x3f)             && (closestcol[i-   1] == 255)) { closestcol[i-   1] = j; *closend++ = i-   1; }
		if (((i&0x3f) != 0x3f)   && (closestcol[i+   1] == 255)) { closestcol[i+   1] = j; *closend++ = i+   1; }
	} while (clospos != closend);
	free(closcan);
}

static long *heapbuf, heapcnt;

	//Warning: Heap compare values are treated as unsigned longs!
void heapinit ();
#pragma aux heapinit =\
	"mov eax, heapbuf"\
	"mov dword ptr [eax], 0"\
	"mov heapcnt, 0"\
	modify exact [eax]

void heapinsert (long, long);
#pragma aux heapinsert =\
	"mov ecx, heapcnt"\
	"mov esi, heapbuf"\
	"inc ecx"\
	"mov edx, ecx"\
	"mov heapcnt, ecx"\
	"shr edx, 1"\
	"mov edi, [esi+edx*8]"\
	"cmp edi, eax"\
	"jna short endit"\
	"begit: mov [esi+ecx*8], edi"\
	"mov edi, [esi+edx*8+4]"\
	"mov [esi+ecx*8+4], edi"\
	"mov ecx, edx"\
	"shr edx, 1"\
	"mov edi, [esi+edx*8]"\
	"cmp edi, eax"\
	"ja short begit"\
	"endit: mov [esi+ecx*8], eax"\
	"mov [esi+ecx*8+4], ebx"\
	parm [eax][ebx] modify exact [ecx edx esi edi]

void heapremovetop ();
#pragma aux heapremovetop =\
	"mov ecx, heapcnt"\
	"mov esi, heapbuf"\
	"dec ecx"\
	"mov eax, 1"\
	"mov ebx, 2"\
	"mov heapcnt, ecx"\
	"mov edx, [esi+ecx*8+8]"\
	"mov [esi+ecx*8+8], 0xffffffff"\
	"cmp ebx, ecx"\
	"ja short endit"\
	"begit: mov edi, [esi+ebx*8]"\
	"cmp edi, [esi+ebx*8+8]"\
	"sbb ebx, -1"\
	"mov edi, [esi+ebx*8]"\
	"cmp edi, edx"\
	"jnc short endit"\
	"mov [esi+eax*8], edi"\
	"mov edi, [esi+ebx*8+4]"\
	"mov [esi+eax*8+4], edi"\
	"mov eax, ebx"\
	"add ebx, ebx"\
	"cmp ebx, ecx"\
	"jna short begit"\
	"endit: mov [esi+eax*8], edx"\
	"mov edx, [esi+ecx*8+12]"\
	"mov [esi+eax*8+4], edx"\
	modify exact [eax ebx ecx edx esi edi]

void initclosestcolor (long *dapal)  //Slow perfect ellipsoid method
{
	long i, j, nd, od, c, oc, px, py, pz, *ptr;
	long rdist[129], gdist[129], bdist[129];

	if (!(heapbuf = malloc(16384*4*2))) { printf("Out of memory!\n"); exit(0); }

	memset(closestcol,-1,sizeof(closestcol));

	j = 0;
	for(i=64;i>=0;i--)  //j = (i-64)*(i-64);
	{
		rdist[i] = rdist[128-i] = j*3;
		gdist[i] = gdist[128-i] = j*5;
		bdist[i] = bdist[128-i] = j*2;
		j += 129-(i<<1);
	}

	heapinit();
	for(i=0;i<255;i++)
	{
		j = (dapal[i<<2]<<12)+(dapal[(i<<2)+1]<<6)+dapal[(i<<2)+2];
		if (closestcol[j] == 255) { heapinsert(0L,j); closestcol[j] = i; }
	}
	do
	{
		j = heapbuf[2]; i = heapbuf[3]; heapremovetop();
		c = closestcol[i]; px = (i>>12); py = (i>>6)&63; pz = (i&63);

		if (px)
		{
			oc = closestcol[i-4096];
			if (oc == 255) { closestcol[i-4096] = c; heapinsert(j+(dapal[c<<2]-px)*6+3,i-4096); }
			else if (oc != c)
			{
				ptr = &dapal[oc<<2];
				od = rdist[ptr[0]-(px-1)+64]+gdist[ptr[1]-py+64]+bdist[ptr[2]-pz+64];
				nd = j+(dapal[c<<2]-px)*6+3;
				if (nd < od) { closestcol[i-4096] = c; heapinsert(nd,i-4096); }
			}
		}
		if (px != 63)
		{
			oc = closestcol[i+4096];
			if (oc == 255) { closestcol[i+4096] = c; heapinsert(j-(dapal[c<<2]-px)*6+3,i+4096); }
			else if (oc != c)
			{
				ptr = &dapal[oc<<2];
				od = rdist[ptr[0]-(px+1)+64]+gdist[ptr[1]-py+64]+bdist[ptr[2]-pz+64];
				nd = j-(dapal[c<<2]-px)*6+3;
				if (nd < od) { closestcol[i+4096] = c; heapinsert(nd,i+4096); }
			}
		}
		if (py)
		{
			oc = closestcol[i-64];
			if (oc == 255) { closestcol[i-64] = c; heapinsert(j+(dapal[(c<<2)+1]-py)*10+5,i-64); }
			else if (oc != c)
			{
				ptr = &dapal[oc<<2];
				od = rdist[ptr[0]-px+64]+gdist[ptr[1]-(py-1)+64]+bdist[ptr[2]-pz+64];
				nd = j+(dapal[(c<<2)+1]-py)*10+5;
				if (nd < od) { closestcol[i-64] = c; heapinsert(nd,i-64); }
			}
		}
		if (py != 63)
		{
			oc = closestcol[i+64];
			if (oc == 255) { closestcol[i+64] = c; heapinsert(j-(dapal[(c<<2)+1]-py)*10+5,i+64); }
			else if (oc != c)
			{
				ptr = &dapal[oc<<2];
				od = rdist[ptr[0]-px+64]+gdist[ptr[1]-(py+1)+64]+bdist[ptr[2]-pz+64];
				nd = j-(dapal[(c<<2)+1]-py)*10+5;
				if (nd < od) { closestcol[i+64] = c; heapinsert(nd,i+64); }
			}
		}
		if (pz)
		{
			oc = closestcol[i-1];
			if (oc == 255) { closestcol[i-1] = c; heapinsert(j+(dapal[(c<<2)+2]-pz)*4+2,i-1); }
			else if (oc != c)
			{
				ptr = &dapal[oc<<2];
				od = rdist[ptr[0]-px+64]+gdist[ptr[1]-py+64]+bdist[ptr[2]-(pz-1)+64];
				nd = j+(dapal[(c<<2)+2]-pz)*4+2;
				if (nd < od) { closestcol[i-1] = c; heapinsert(nd,i-1); }
			}
		}
		if (pz != 63)
		{
			oc = closestcol[i+1];
			if (oc == 255) { closestcol[i+1] = c; heapinsert(j-(dapal[(c<<2)+2]-pz)*4+2,i+1); }
			else if (oc != c)
			{
				ptr = &dapal[oc<<2];
				od = rdist[ptr[0]-px+64]+gdist[ptr[1]-py+64]+bdist[ptr[2]-(pz+1)+64];
				nd = j-(dapal[(c<<2)+2]-pz)*4+2;
				if (nd < od) { closestcol[i+1] = c; heapinsert(nd,i+1); }
			}
		}
	} while (heapcnt > 0);
	free(heapbuf);
}
