DEPTH = ../../../..

include $(DEPTH)/make/openclsdkdefs.mk 

####
#
#  Targets
#
####

BOLT			= 1
ifeq ($(BITS), 64)
ifneq ($(strip $(CL_LIB_VER)),)
SAMPLE_EXE		= 1
EXE_TARGET 		= RgbToYuv
EXE_TARGET_INSTALL   	= RgbToYuv

####
#
#  C/CPP files
#
####

FILES 		= RgbToYuv 

LLIBS  		+= BoltUtil
endif
endif
include $(DEPTH)/make/openclsdkrules.mk 

