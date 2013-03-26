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
EXE_TARGET 		= BoltIntro
EXE_TARGET_INSTALL   	= BoltIntro

####
#
#  C/CPP files
#
####

FILES 		= BoltIntro 

endif
endif
include $(DEPTH)/make/openclsdkrules.mk 

