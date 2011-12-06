## Take a look at PPMdType.h for additional compiler & environment options
.AUTODEPEND
#               User defined variables
PRJNAME=PPMs
EXETYPE=DPMI32              # DOS DPMI16 DPMI32
DEBUG=1
CPP_SET=$(PRJNAME).cpp Model.cpp
C_SET=
ASM_SET=
.path.cpp = ;
.path.asm = ;
#               End of user defined variables


!if $d(USE_DEBUGGER)
    DEBUG=1
!endif
!if $(EXETYPE) == DOS
    CC     = Bcc
    TLINK  = TLink
    TASM   = Tasm
    ECFLAG = -ml -f287
    EAFLAG = /o
    ELFLAG = /Tde /d
    STARTM = c0l.obj
    LIBS   = fp87.lib mathl.lib cl.lib
    DEBGR  = td.exe
!elif $(EXETYPE) == DPMI16
    CC     = Bcc
    TLINK  = TLink
    TASM   = Tasm
    ECFLAG = -ml -WX -f287
    EAFLAG = /o
    ELFLAG = /Txe
    STARTM = c0x.obj
    LIBS   = dpmi16.lib mathwl.lib cwl.lib
    DEBGR  = td.exe
!elif $(EXETYPE) == DPMI32
    CC     = Bcc32
    TLINK  = TLink32
    TASM   = Tasm32
    ECFLAG = -WX -f- -a4
    EAFLAG = /os /dMemMod='F'
    ELFLAG = /Tpe /ax
    STARTM = c0x32.obj
    LIBS   = noeh32.lib dpmi32.lib cw32.lib
#    LIBS   = noeh32.lib import32.lib cw32.lib
    DEBGR  = td32.exe
!endif

!if $(DEBUG) != 0
    DCFLAG = -v -vi -N
    DAFLAG = -zi
    DLFLAG = /v
!else
    CC     = Bcc32i
    DCFLAG = -OaIS2 -k- -N-
    DAFLAG = -zn
!if ($(EXETYPE) == DPMI32 || $(EXETYPE) == WIN32)
    DLFLAG = -B:400000
!endif
!endif

SCFLAG  = -w -w-sig -w-inl -H=$(PRJNAME).csm -6 -Vmd -x- -RT- -D_MEM_CONFIG=1024
SAFLAG  = -ml -m5
SLFLAG  = /x /c
OBJ_SET = $(CPP_SET:.cpp=.obj) $(C_SET:.c=.obj) $(ASM_SET:.asm=.obj)

!if $d(USE_DEBUGGER)
DEBUG_TARGET : $(PRJNAME).exe
    -$(DEBGR) $(PRJNAME).exe
!endif
!if $d(MAKE_AND_RUN)
RUN_TARGET : $(PRJNAME).exe
    -CLS
    -$(PRJNAME).exe
!endif
$(PRJNAME).exe : $(OBJ_SET)
  @$(TLINK)    @&&|
$(SLFLAG) $(ELFLAG) $(DLFLAG) $(STARTM) $(OBJ_SET),$(PRJNAME).exe,,$(LIBS)
|

.cpp.obj:
    @$(CC) $(SCFLAG) $(ECFLAG) $(DCFLAG) -c {$< }

.c.obj:
    @$(CC) $(SCFLAG) $(ECFLAG) $(DCFLAG) -c {$< }

.asm.obj:
   @$(TASM) $(SAFLAG) $(EAFLAG) $(DAFLAG) $<
