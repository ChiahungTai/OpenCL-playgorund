DEPTH = ../../../../..

include $(DEPTH)/make/openclsdkdefs.mk 

####
#
#  Targets
#
####

OPENCL			= 1
SAMPLE_EXE		= 1
SDK_UTIL_NOT_REQUIRED   = 1
EXE_TARGET 		= OpenCLService
EXE_TARGET_INSTALL   	= OpenCLService

####
#
#  C/CPP files
#
####

FILES 	= OpenCLService

LLIBS += kernel32 gdi32 user32 advapi32 Shlwapi

include $(DEPTH)/make/openclsdkrules.mk 

