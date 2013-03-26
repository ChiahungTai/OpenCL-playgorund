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
EXE_TARGET 		= Stencil2D
EXE_TARGET_INSTALL   	= Stencil2D

####
#
#  C/CPP files
#
####

FILES 		= Stencil2D

LLIBS  		+= AmpUtil
endif
include $(DEPTH)/make/openclsdkrules.mk 

