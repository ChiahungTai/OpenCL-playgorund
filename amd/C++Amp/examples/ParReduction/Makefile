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
EXE_TARGET 		= ParReduction
EXE_TARGET_INSTALL   	= ParReduction

####
#
#  C/CPP files
#
####

FILES 		= ParReduction

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

