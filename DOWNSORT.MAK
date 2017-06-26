#
# Make MAXimus CBCS DownSORT program
#

CL=-c -Zl -Zp -G2 -J -W4 -AC -Ot
LK=/A:16 /F /NOI /NOD /PACKC:65500 /EXEPACK
L1=CLIBCE OS2
L2=f:\toolkt13\lib\api.lib c:\os2\doscalls.lib
OBJ1=downsort.obj downcol.obj  downpar.obj
OBJ2=downfnt.obj  downsrv.obj  downvars.obj
OBJ3=downrpt1.obj downrpt2.obj downrpt3.obj downrpt4.obj
OBJS=$(OBJ1) $(OBJ2) $(OBJ3)

downsort.exe: downos2.exe
  BIND downos2 $(L2) /o $@

downos2.exe: $(OBJS)
  LINK $**,$@ $(LK),nul,$(L1),downsort.def

.c.obj:
  CL $(CL) $*.c

$(OBJS): downfpro.h downsort.h ..\max\mstruct.h

