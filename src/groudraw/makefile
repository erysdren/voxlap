#Note: groudraw.c requires the 16-bit Microsoft C compiler
groudraw.exe: groudraw.c; cl groudraw.c

WLOPTS = system dos4g
WCOPTS = /oneatx /s /6r /fpi87 /fp6 /bt=dos

all: groucom.exe groucube.exe overgrou.exe grou6d.exe steep.exe steep2.exe

groucom.exe: groucom.obj g.obj; wlink f groucom,g $(WLOPTS)
groucom.obj: groucom.c; wcc386 $(WCOPTS) groucom.c
g.obj: g.asm; wasm g.asm

groucube.exe: groucube.obj; wlink f groucube $(WLOPTS)
groucube.obj: groucube.c; wcc386 $(WCOPTS) groucube.c

overgrou.exe: overgrou.obj og.obj; wlink f overgrou,og $(WLOPTS)
overgrou.obj: overgrou.c; wcc386 $(WCOPTS) overgrou.c
og.obj: og.asm; wasm og.asm

grou6d.exe: grou6d.obj g6.obj; wlink f grou6d,g6 $(WLOPTS)
grou6d.obj: grou6d.c; wcc386 $(WCOPTS) grou6d.c
g6.obj: g6.asm; wasm g6.asm

steep.exe: steep.obj stp.obj; wlink f steep,stp $(WLOPTS)
steep.obj: steep.c; wcc386 $(WCOPTS) steep.c
stp.obj: stp.asm; wasm stp.asm

steep2.exe: steep2.obj stp2.obj kplib.obj; wlink f steep2,stp2,kplib $(WLOPTS)
steep2.obj: steep2.c; wcc386 $(WCOPTS) steep2.c
stp2.obj: stp2.asm; wasm stp2.asm
kplib.obj: kplib.c; wcc386 /oneatx /6r /fpi87 /fp6 /bt=dos kplib.c
