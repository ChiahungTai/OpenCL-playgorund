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
EXE_TARGET 		= ArrayVsArrayView
EXE_TARGET_INSTALL   	= ArrayVsArrayView

####
#
#  C/CPP files
#
####

FILES 		= ArrayVsArrayView

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

