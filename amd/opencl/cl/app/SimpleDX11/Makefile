DEPTH = ../../../../..

include $(DEPTH)/make/openclsdkdefs.mk 

####
#
#  Targets
#
####

OPENCL			= 1
SAMPLE_EXE		= 1
EXE_TARGET 		= SimpleDX11
EXE_TARGET_INSTALL   	= SimpleDX11
REMOVE_ON_CLEAN = SimpleDX11PixelShader.cso SimpleDX11VertexShader.cso 

####
#
#  C/CPP files
#
####

FILES = SimpleDX11 BasicReader
CLFILES = SimpleDX11_Kernels.cl SimpleDX11.bmp SimpleDX11PixelShader.cso SimpleDX11VertexShader.cso

INCLUDEDIRS += $(SDK_HEADERS)
CFLAGS := $(subst /WX,/WX-,$(CFLAGS))
CFLAGS += $(C_INCLUDE_FLAG)"$(DXSDK_DIR)/Include" $(C_INCLUDE_FLAG)"$(FrameworkSDKDir)/include" /wd4005 /wd4505

LLIBS += SDKUtil
LLIBS += d3d11 d3dx11 DXGI kernel32 gdi32 user32 advapi32 Delayimp

# DirectX library path
ifeq ($(BITS), 32)
    LDFLAGS += $(LD_LIBDIR_FLAG)"$(DXSDK_DIR)/Lib/x86" $(LD_LIBDIR_FLAG)"$(FrameworkSDKDir)/lib/win8/um/x86"
else
    LDFLAGS += $(LD_LIBDIR_FLAG)"$(DXSDK_DIR)/Lib/x64" $(LD_LIBDIR_FLAG)"$(FrameworkSDKDir)/lib/win8/um/x86"
endif

LDFLAGS += /DELAYLOAD:"d3d11.dll" /DELAYLOAD:"DXGI.dll"

FXC = fxc.exe

all: SimpleDX11PixelShader.cso SimpleDX11VertexShader.cso

SimpleDX11PixelShader.cso: SimpleDX11PixelShader.hlsl
	$(FXC) /E"ps_main" /Fo"$@" /T ps"_5_0" /nologo $<

SimpleDX11VertexShader.cso: SimpleDX11VertexShader.hlsl
	$(FXC) /E"vs_main" /Fo"$@" /T vs"_5_0" /nologo $<

include $(DEPTH)/make/openclsdkrules.mk 

