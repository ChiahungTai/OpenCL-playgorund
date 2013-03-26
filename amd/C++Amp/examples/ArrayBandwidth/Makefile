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
EXE_TARGET 		= ArrayBandwidth
EXE_TARGET_INSTALL   	= ArrayBandwidth

####
#
#  C/CPP files
#
####

FILES 		= ArrayBandwidth

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

