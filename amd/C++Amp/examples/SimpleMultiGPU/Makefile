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
EXE_TARGET 		= SimpleMultiGPU
EXE_TARGET_INSTALL   	= SimpleMultiGPU

####
#
#  C/CPP files
#
####

FILES 		= SimpleMultiGPU

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

