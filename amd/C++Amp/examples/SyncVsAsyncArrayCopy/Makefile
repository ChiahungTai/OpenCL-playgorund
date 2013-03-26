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
EXE_TARGET 		= SyncVsAsyncArrayCopy
EXE_TARGET_INSTALL   	= SyncVsAsyncArrayCopy

####
#
#  C/CPP files
#
####

FILES 		= SyncVsAsyncArrayCopy

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

