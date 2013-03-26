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
EXE_TARGET 		= HelloC++AMP
EXE_TARGET_INSTALL   	= HelloC++AMP

####
#
#  C/CPP files
#
####

FILES 		= HelloC++AMP

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

