#
# Make MAXimus CBCS DownSORT program  (with IBM C/C++ for OS/2 2.0+)
#

#   CL=/c /Fi /G3 /Gd- /J+ /Ka+ /Kb+ /Wcmp /O+ /Si /Sp2 /D__386__
CL=/c /Fi /G3 /Gd- /J+ /Kb+ /O+ /Si /Sp2 /Ss /D__386__
LK=/NOI /PACKC:65500 /EXEPACK /ALIGN:4 /BASE:0x10000
OBJ1=downsort.obj downcol.obj downfnt.obj
OBJ2=downpar.obj downsrv.obj downvars.obj
OBJ3=downrpt1.obj downrpt2.obj downrpt3.obj downrpt4.obj
OBJS=$(OBJ1) $(OBJ2) $(OBJ3)

down386.exe: $(OBJS)  downsort.def
  LINK386 $(OBJS), $@ $(LK), NUL, $(L1) $(L2), downsort.def

$(OBJS): downfpro.h downsort.h ..\max\mstruct.h
  icc $(CL) $*.c

