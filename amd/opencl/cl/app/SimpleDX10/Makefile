DEPTH = ../../../../..

include $(DEPTH)/make/openclsdkdefs.mk 

####
#
#  Targets
#
####

OPENCL			= 1
SAMPLE_EXE		= 1
EXE_TARGET 		= SimpleDX10
EXE_TARGET_INSTALL   	= SimpleDX10
REMOVE_ON_CLEAN = SimpleDX10.fxo

####
#
#  C/CPP files
#
####

FILES = SimpleDX10
CLFILES = SimpleDX10_Kernels.cl SimpleDX10.bmp SimpleDX10.fxo

INCLUDEDIRS += $(SDK_HEADERS)

CFLAGS := $(subst /WX,/WX-,$(CFLAGS))
CFLAGS += $(C_INCLUDE_FLAG)"$(DXSDK_DIR)/Include" $(C_INCLUDE_FLAG)"$(FrameworkSDKDir)/include" /wd4005 /wd4505

LLIBS += SDKUtil
LLIBS += d3d10 d3dx10 DXGI kernel32 gdi32 user32 advapi32 Delayimp

# DirectX library path
ifeq ($(BITS), 32)
    LDFLAGS += $(LD_LIBDIR_FLAG)"$(DXSDK_DIR)/Lib/x86" $(LD_LIBDIR_FLAG)"$(FrameworkSDKDir)/lib/win8/um/x86"
else
    LDFLAGS += $(LD_LIBDIR_FLAG)"$(DXSDK_DIR)/Lib/x64" $(LD_LIBDIR_FLAG)"$(FrameworkSDKDir)/lib/win8/um/x86"
endif

LDFLAGS += /DELAYLOAD:"d3d10.dll" /DELAYLOAD:"DXGI.dll"

FXC = fxc.exe

all: SimpleDX10.fxo

SimpleDX10.fxo: SimpleDX10.fx
	$(FXC) /E"PS_MAIN" /Fo"$@" /T fx_4_0 /nologo $<

include $(DEPTH)/make/openclsdkrules.mk 

