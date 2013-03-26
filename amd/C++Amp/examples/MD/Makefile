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
EXE_TARGET 		= MD
EXE_TARGET_INSTALL   	= MD

####
#
#  C/CPP files
#
####

FILES 		= MD

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

