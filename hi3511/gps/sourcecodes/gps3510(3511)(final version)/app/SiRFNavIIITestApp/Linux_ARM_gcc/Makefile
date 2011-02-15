# This makefile compiles C version of ASN.1
SRCPATH = .
#CC = arm-hismall-linux-g++
CC =  arm-hismall-linux-g++
AR= arm-hismall-linux-ar
OBJDIR = $(SRCPATH)/objs
DEPDIR = $(SRCPATH)/dep
EXE = SiRFNavIIIdemo
LINUX_LIB_DIR = /opt/hisilicon/toolchains/arm-hismall-linux/linux-soft/lib
PRE_COMPILED_LIBS=../libs/
SiRFNav_DIR = ../../../lib/SiRFNavIIILib/Linux_ARM_gcc/libs
SiRFPal_DIR =  ../../../lib/SiRFPAL/Linux_ARM_gcc/libs

P3_DIR = .

CFLAGS = 
CFLAGS += $(INCDIR) -DOS_POSIX -D_ENDIAN_LITTLE -DSIRFNAV_DEMO -DTOOLCHAIN_GCC -DSIRF_HOST -DSIRF_EXT_LOG -DSIRF_CODEC_NMEA -c

INCDIR =  \
         -I$(SRCPATH)/../../../sirf/include \
         -I$(SRCPATH)/../../../sirf/util/ext \
         -I$(SRCPATH)/../../../sirf/util/demo \
         -I$(SRCPATH)/../../../sirf/util/demo_lpl_cmdr \
         -I$(SRCPATH)/../../../sirf/util/codec \
         -I$(SRCPATH)/../../../sirf/util/proto \
         -I$(SRCPATH)/../../../sirf/include \
         -I$(SRCPATH)/../../../sirf/pal \
         -I$(SRCPATH)/../../../sirf/pal/Posix \
         -I$(SRCPATH)/../../../sirf/SiRFNav_Demo/cfg \
         -I$(SRCPATH)/../../../sirf/SiRFNav/SL/main \
         -I$(SRCPATH)/../../../sirf/SiRFNavIII/include \
         -I$(SRCPATH)/../../../sirf/SiRFNavIII/cfg 

         
         
vpath %.c $(SRCPATH)/source

COREOBJS =  \
		$(OBJDIR)/main.o \
		$(OBJDIR)/sirf_threads.o \
		$(OBJDIR)/sirf_codec_nmea.o \
		$(OBJDIR)/sirf_demo.o \
		$(OBJDIR)/sirf_proto_parse.o \
		$(OBJDIR)/sirf_ext_log.o \
		$(OBJDIR)/sirf_codec_ssb.o \
		$(OBJDIR)/sirf_codec_ascii.o \
		$(OBJDIR)/sirf_codec.o \
		$(OBJDIR)/sirf_proto_common.o
				

OBJS = $(COREOBJS)

$(OBJDIR)/%.o:$(SRCPATH)/main/%.c
	$(CC) $(CFLAGS) -MD $< -o $@
	@set -e; \
	sed 's,\($*\)\.o[ :]*,\1.o $(DEPDIR)\/\1.d : ,g' < $(addsuffix .d, $(basename $@))> $(DEPDIR)/$(addsuffix .d, $(basename $(notdir $@))); \
	rm -f $(addsuffix .d, $(basename $@))

all: sirfpal  makedir $(OBJS)
	$(CC) $(OBJDIR)/*.o -o $(EXE) -g -L $(SiRFNav_DIR) -lsirf_host \
	                                 -L $(SiRFPal_DIR) -lsirf_pal \
	                                 -L $(LINUX_LIB_DIR) -lm -lpthread -lrt \
	                                 -L $(P3_DIR) -lComuServ
	@echo "Build Complete"


sirfpal:
	make -C ../../../lib/SiRFPAL/Linux_ARM_gcc  -f Makefile_SiRFPAL
	@echo "#################################################"




sirfpalclean:
	make -C ../../../lib/SiRFPAL/Linux_ARM_gcc  -f Makefile_SiRFPAL clean
	@echo "#################################################"


clean: sirfpalclean
	rm -rf  $(OBJDIR) $(DEPDIR) 


makedir:
	@set -e; \
	mkdir -p $(OBJDIR) $(DEPDIR)

