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
EXE_TARGET 		= BlackScholes
EXE_TARGET_INSTALL   	= BlackScholes

####
#
#  C/CPP files
#
####

FILES 		= BlackScholes 

LLIBS  		+= BoltUtil
endif
endif
include $(DEPTH)/make/openclsdkrules.mk 

