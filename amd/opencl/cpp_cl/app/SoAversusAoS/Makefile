DEPTH = ../../../../..

include $(DEPTH)/make/openclsdkdefs.mk 

####
#
#  Targets
#
####

OPENCL			= 1
SAMPLE_EXE		= 1
INSTALL_TO_PUBLIC       = 1
EXE_TARGET 		= SoAversusAoS
EXE_TARGET_INSTALL   	= SoAversusAoS

####
#
#  C/CPP files
#
####

FILES 	= SoAversusAoS


CLFILES = SoAversusAoS_Kernels.cl

LLIBS  += SDKUtil

include $(DEPTH)/make/openclsdkrules.mk 

