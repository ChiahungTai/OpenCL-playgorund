DEPTH = ../../../..

include $(DEPTH)/make/openclsdkdefs.mk 

####
#
#  Targets
#
####

CPPAMP			= 1
ifeq ($(strip $(CL_LIB_VER)),vc110)
SAMPLE_EXE		= 1
EXE_TARGET 		= GEMM
EXE_TARGET_INSTALL   	= GEMM

####
#
#  C/CPP files
#
####

FILES 		= GEMM

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

