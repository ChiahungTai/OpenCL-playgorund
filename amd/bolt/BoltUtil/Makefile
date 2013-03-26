DEPTH = ../../..

include $(DEPTH)/make/openclsdkdefs.mk 

####
#
#  Targets
#
####

BOLT			= 1
ifeq ($(BITS), 64)
ifneq ($(strip $(CL_LIB_VER)),)
SDK_STATIC_LIB	        = 1
LIB_TARGET 		= BoltUtil
LIB_TARGET_INSTALL   	= BoltUtil
LIB_DIR_INSTALL   	= $(DEPTH)/TempSDKUtil/lib/

####
#
#  C/CPP files
#
####

FILES = BoltCommandArgs \
        BoltSample

INCLUDEDIRS += include 
endif
endif
include $(DEPTH)/make/openclsdkrules.mk 
