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
EXE_TARGET 		= TileStaticStorageBandwidth
EXE_TARGET_INSTALL   	= TileStaticStorageBandwidth

####
#
#  C/CPP files
#
####

FILES 		= TileStaticStorageBandwidth

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

