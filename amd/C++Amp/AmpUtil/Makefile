DEPTH = ../../..

include $(DEPTH)/make/openclsdkdefs.mk 

####
#
#  Targets
#
####

CPPAMP			= 1
ifeq ($(strip $(CL_LIB_VER)),vc110)
SDK_STATIC_LIB	        = 1
LIB_TARGET 		= AmpUtil
LIB_TARGET_INSTALL   	= AmpUtil
LIB_DIR_INSTALL   	= $(DEPTH)/TempSDKUtil/lib/

####
#
#  C/CPP files
#
####

FILES = AmpCommandArgs \
        AmpSample \
		AmpCommon

INCLUDEDIRS += include 
endif
include $(DEPTH)/make/openclsdkrules.mk 
