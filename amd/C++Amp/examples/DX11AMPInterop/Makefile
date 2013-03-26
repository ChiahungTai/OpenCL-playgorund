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
EXE_TARGET 		= DX11AMPInterop
EXE_TARGET_INSTALL   	= DX11AMPInterop
endif
CLFILES = SimplePixelShader.cso SimpleVertexShader.cso
REMOVE_ON_CLEAN = SimplePixelShader.cso SimpleVertexShader.cso

####
#
#  C/CPP files
#
####
ifeq ($(strip $(CL_LIB_VER)),vc110)
FILES 	= AmpEngine \
	AMPInteropDX11 \
	BasicReader \
	DX11Engine

LLIBS  	+= AmpUtil
LLIBS   += d3dcompiler D3D11 dxguid winmm comctl32 DXGI Delayimp kernel32 gdi32 user32 advapi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid odbc32 odbccp32

INCLUDEDIRS += $(SDK_HEADERS_CPPAMP)

CFLAGS := $(subst /WX,/WX-,$(CFLAGS))
CFLAGS += /DD3DXFX_LARGEADDRESS_HANDLE $(C_INCLUDE_FLAG)"$(DXSDK_DIR)/Include" $(C_INCLUDE_FLAG)"$(FrameworkSDKDir)/include" /wd4005 /wd4505

ifeq ($(BITS), 32)
    LDFLAGS += $(LD_LIBDIR_FLAG)"$(DXSDK_DIR)/Lib/x86" $(LD_LIBDIR_FLAG)"$(FrameworkSDKDir)/lib/win8/um/x86"
else
    LDFLAGS += $(LD_LIBDIR_FLAG)"$(DXSDK_DIR)/Lib/x64" $(LD_LIBDIR_FLAG)"$(FrameworkSDKDir)/lib/win8/um/x86"
endif

LDFLAGS += /DELAYLOAD:"d3d11.dll" /DELAYLOAD:"DXGI.dll"

endif

FXC = fxc.exe

all: SimplePixelShader.cso SimpleVertexShader.cso

SimplePixelShader.cso: SimplePixelShader.hlsl
	$(FXC) /E"PS_MAIN" /Fo"$@" /T ps"_5_0" /nologo $<

SimpleVertexShader.cso: SimpleVertexShader.hlsl
	$(FXC) /E"VS_MAIN" /Fo"$@" /T vs"_5_0" /nologo $<

include $(DEPTH)/make/openclsdkrules.mk 
