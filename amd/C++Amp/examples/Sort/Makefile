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
EXE_TARGET 		= Sort
EXE_TARGET_INSTALL   	= Sort

####
#
#  C/CPP files
#
####

FILES 		= Sort

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

