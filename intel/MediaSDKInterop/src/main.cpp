// Copyright (c) 2009-2011 Intel Corporation
// All rights reserved.
// 
// WARRANTY DISCLAIMER
// 
// THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
// MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Intel Corporation is the author of the Materials, and requests that all
// problem reports or change requests be submitted to it directly

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "pipeline_decode.h"

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders 



//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls
ID3DXFont*                  g_pFont9 = NULL;
ID3DXSprite*                g_pSprite9 = NULL;
D3DSURFACE_DESC             g_BackBufferDesc;
IDirect3DSurface9*          g_pBackBuffer = NULL;

// pipeline for decoding, includes input file reader, decoder and output file writer
static CDecodingPipeline*   g_pMSDKPipeline = NULL; 
// curretn CPU frequency
LARGE_INTEGER               g_Freq;
// counter to store last ticks measurements
LARGE_INTEGER               g_TicksPrev;
// averaget time to process one frame with drawing
float                       g_AverTimeAll = 0;
//flag to draw or not text
bool                        g_DrawCodeFLag = false;

//macro to check the pipeline pointer
#define CHECK_PIPELINE(_action)\
    if(g_pMSDKPipeline==NULL)\
{\
    printf("ERROR!!! MediaPipline is not created !!!\n");\
    _action;\
}

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
static const int IDC_TOGGLEFULLSCREEN   = 1;
static const int IDC_TOGGLEREF          = 2;
static const int IDC_CHANGEDEVICE       = 3;
static const int IDC_PARAM1_STATIC    = 101;
static const int IDC_PARAM1_SLIDER    = 102;
static const int IDC_PARAM2_STATIC    = 103;
static const int IDC_PARAM2_SLIDER    = 104;
static const int IDC_OPENCL_FILTER_STATIC  = 105;
static const int IDC_OPENCL_FILTER_COMBOBOX  = 106;
static const int IDC_DECODE_CHECKBOX  = 109;
static const int IDC_OPENCL_CHECKBOX  = 110;


// update GUI list of available OpenCL filters
// this function is called only if current folder was changed by adding or deleteing *.cl file
static void UpdateGUI_OpenCLFilterComboBox()
{
    CDXUTComboBox*  pCB = g_SampleUI.GetComboBox( IDC_OPENCL_FILTER_COMBOBOX );
    pCB->RemoveAllItems();
    if(!g_pMSDKPipeline->m_pOCLPlugin)return;
    CHECK_PIPELINE(return);
    for(int i=0;;++i)
    {
        WCHAR* name = g_pMSDKPipeline->m_pOCLPlugin->GetProgramName(i);
        if(name==NULL) break;
        pCB->AddItem(name, (void*)i);
    }
    int index = g_pMSDKPipeline->m_pOCLPlugin->GetProgram();
    if(index>=0)
        pCB->SetSelectedByIndex(index);
}

// update GUI elemnts like sliders or texts
static void UpdateGUI_SlidersText()
{
    WCHAR sz[100];

    CHECK_PIPELINE(return);
    if(!g_pMSDKPipeline->m_pOCLPlugin)return;
    g_SampleUI.GetSlider(IDC_PARAM1_SLIDER)->SetValue(g_pMSDKPipeline->m_pOCLPlugin->m_Param1);
    swprintf_s(sz, 100, L"Param1 %d", g_pMSDKPipeline->m_pOCLPlugin->m_Param1);
    g_SampleUI.GetStatic(IDC_PARAM1_STATIC)->SetText(sz);

    g_SampleUI.GetSlider(IDC_PARAM2_SLIDER)->SetValue(g_pMSDKPipeline->m_pOCLPlugin->m_Param2);
    swprintf_s(sz, 100, L"Param2 %d", g_pMSDKPipeline->m_pOCLPlugin->m_Param2);
    g_SampleUI.GetStatic(IDC_PARAM2_STATIC)->SetText(sz);
}

static HRESULT CreatePipeline(IDirect3DDevice9* pd3dDevice)
{
    SAFE_DELETE(g_pMSDKPipeline);
    g_pMSDKPipeline = new CDecodingPipeline(pd3dDevice);

    {// init decode pipeline
        sInputParams        Params;   // input parameters from command line

        MSDK_ZERO_MEMORY(Params);

        // set parameters for MediaSDK
        _tcscpy_s(Params.strSrcFile, _T("MediaSDKInterop_video.h264"));
        //        Params.videoType = MFX_CODEC_MPEG2;
        Params.videoType = MFX_CODEC_AVC;

        // init MediaSDK pipline
        if(g_pMSDKPipeline)
        {
            if(g_pMSDKPipeline->Init(&Params) < MFX_ERR_NONE) 
                return S_FALSE;
        }
        else
        {
            printf("!!!g_pMSDKPipeline is not yet created!!!!!! \n");
        }
    }

    {// reinit checkbox and sliders
        UpdateGUI_OpenCLFilterComboBox();
        UpdateGUI_SlidersText();
        g_SampleUI.GetCheckBox(IDC_DECODE_CHECKBOX)->SetChecked(g_pMSDKPipeline->m_DECODEFlag);
        if(g_pMSDKPipeline->m_pOCLPlugin)g_SampleUI.GetCheckBox(IDC_OPENCL_CHECKBOX)->SetChecked(g_pMSDKPipeline->m_pOCLPlugin->m_OCLFlag);
    }
    return S_OK;
}

void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    int iY = 10;

    g_HUD.SetCallback( OnGUIEvent ); 
//    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN,
        DXUTIsWindowed() ? L"Full screen On (F11)" : L"Full screen Off (F11)",
        35, iY+=24, 125, 22, VK_F11 );
    //g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
    //g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );

    g_SampleUI.SetCallback( OnGUIEvent ); 

    // add a UI slider control with label
    int W = 150;
    int X0 = 0;
    iY += 24;

    g_SampleUI.AddCheckBox( IDC_DECODE_CHECKBOX, L"Decode", X0, iY += 20, W+20, 22, true );
    g_SampleUI.AddCheckBox( IDC_OPENCL_CHECKBOX, L"OpenCL", X0, iY += 20, 80, 22, true );

    g_SampleUI.AddStatic( IDC_OPENCL_FILTER_STATIC, L"OPENCL filter", X0, iY += 20, W, 22 );
    g_SampleUI.AddComboBox( IDC_OPENCL_FILTER_COMBOBOX, X0, iY += 20, W+20, 22, false );


    g_SampleUI.AddStatic( IDC_PARAM1_STATIC, L"Param1 50", X0, iY += 20, W, 22 );
    g_SampleUI.AddSlider( IDC_PARAM1_SLIDER, X0, iY += 20, W, 22, 0, 100, 50 );
    g_SampleUI.AddStatic( IDC_PARAM2_STATIC, L"Param2 50", X0, iY += 20, W, 22 );
    g_SampleUI.AddSlider( IDC_PARAM2_SLIDER, X0, iY += 20, W, 22, 0, 100, 50 );


}


//--------------------------------------------------------------------------------------
// Render the statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    static          WCHAR pStrFPS[128];
    bool            UpdateFlag = 0;
    static float    Time = 0;
    Time += g_AverTimeAll;
    if(Time>1000)
    {
        Time -= 1000;
        UpdateFlag = 1;
    }

    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    if(UpdateFlag)
    {
        swprintf_s(pStrFPS, 128, L"FPS=%.1f\n",1000.0f/g_AverTimeAll);
    }

    g_pTxtHelper->DrawTextLine( pStrFPS );

    if(g_DrawCodeFLag)
    {
        int     y = 0;
        UINT nBackBufferHeight = DXUTGetDXGIBackBufferSurfaceDesc()->Height;

        CHECK_PIPELINE(return);

        g_pTxtHelper->SetInsertionPos(2, nBackBufferHeight - 15 * 6);
        g_pTxtHelper->SetForegroundColor(D3DXCOLOR(1.0f, 0.75f, 0.0f, 1.0f));
        g_pTxtHelper->DrawTextLine(L"Controls:");

        g_pTxtHelper->SetInsertionPos(20, nBackBufferHeight - 15 * 5);
        g_pTxtHelper->DrawTextLine(L"To change OpenCL parameters\nslide Param1 and Param2 sliders.\n");

        g_pTxtHelper->SetInsertionPos(20, nBackBufferHeight - 5 * 5);
        g_pTxtHelper->DrawTextLine(L"Quit: ESC\n");




        if(g_pMSDKPipeline->m_pOCLPlugin)
        {

            {  // draw times
                static WCHAR   str[128];
                if(UpdateFlag)
                {
                    OCLPlugin* p = g_pMSDKPipeline->m_pOCLPlugin;
                    swprintf_s(str, 128, L"Time=%.1f(ms), OCL %.2f(ms)\n",g_AverTimeAll,p->m_TimeAver);
                }
                g_pTxtHelper->SetInsertionPos(10, 12 * (y++) + 50);
                g_pTxtHelper->DrawTextLine(str);
            }



            // draw program source and build log
            char* str1 = g_pMSDKPipeline->m_pOCLPlugin->GetProgramSRC();
            char* str2 = g_pMSDKPipeline->m_pOCLPlugin->GetProgramBuildLOG();
            if(str1 || str2)
            {
                char*   PN = g_pMSDKPipeline->m_OCLStruct.m_PlatformNames[g_pMSDKPipeline->m_OCLStruct.m_PlatformCurrent];
                size_t  PNLen = (PN?strlen(PN):0);
                int     len1 = str1?(int)strlen(str1):0;
                int     len2 = str2?(int)strlen(str2):0;
                WCHAR*  out = new WCHAR[len1+len2+PNLen+1];

                if(PN)
                {
                    size_t convertedChars = 0;
                    mbstowcs_s(&convertedChars, out, PNLen, PN, _TRUNCATE);
                    g_pTxtHelper->SetInsertionPos(10, 12 * y + 50);
                    g_pTxtHelper->DrawTextLine(out);
                    y++;
                }
                g_pTxtHelper->SetInsertionPos(10, 12 * y + 50);
                g_pTxtHelper->DrawTextLine(g_pMSDKPipeline->m_pOCLPlugin->GetProgramName());
                y++;

                for(int k=0;k<2;++k)
                {
                    int strnum = 0;
                    int len = 0;
                    char* str = NULL;
                    if(k==0)
                    {
                        len = len1;
                        str = str1;
                    }
                    if(k==1)
                    {
                        len = len2;
                        str = str2;
                        g_pTxtHelper->SetInsertionPos(10, 12 * y + 50);
                        g_pTxtHelper->DrawTextLine(L"BuildLog:");
                        y++;
                    }
                    for(;len>0 && strnum<70;)
                    {
                        char*   str_end = str;
                        for(;(*str_end)!=0 && (*str_end)!=0xD && (*str_end)!=0xA;str_end++);
                        size_t lencur = ((size_t)str_end - (size_t)str);
                        if(lencur>0)
                        {
                            size_t convertedChars = 0;
                            mbstowcs_s(&convertedChars, out, lencur+1, str, _TRUNCATE);

                            g_pTxtHelper->SetInsertionPos(20, 12 * y + 50);
                            g_pTxtHelper->DrawTextLine(out);
                            y++;
                            strnum++;
                        }

                        str += lencur+1;
                        len -= (int)(lencur+1);
                    }
                }
                delete out;
            }
        }
    }
    g_pTxtHelper->End();
}//RenderText


//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
                                     D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
        AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
        D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    // No fallback defined by this app, so reject any device that 
    // doesn't support at least ps2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION( 2, 0 ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    if( pDeviceSettings->ver == DXUT_D3D9_DEVICE )
    {
        IDirect3D9* pD3D = DXUTGetD3D9Object();
        D3DCAPS9 Caps;
        pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps );

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
        // then switch to SWVP.
        if( ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
        {
            pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
        if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
        {
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
            pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
#endif
#ifdef DEBUG_PS
        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif

        // we use d3d device from different threads
        pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;
    }

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
            pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE ) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    return true;
}


// store mouse position for further OCL processing
static void CALLBACK OnMouseProc( bool bLeftButtonDown, bool bRightButtonDown,
                                 bool bMiddleButtonDown, bool bSideButton1Down, bool bSideButton2Down,
                                 int nMouseWheelDelta, int xPos, int yPos, void* pUserContext )
{
    if(g_pMSDKPipeline && g_pMSDKPipeline->m_pOCLPlugin)
    {
        g_pMSDKPipeline->m_pOCLPlugin->m_MouseX = xPos;
        g_pMSDKPipeline->m_pOCLPlugin->m_MouseY = yPos;
        g_pMSDKPipeline->m_pOCLPlugin->m_ButtonFlag = bLeftButtonDown;
        //printf("mouse %d %d %d\n",xPos, yPos, bLeftButtonDown);
    }
}





//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                    void* pUserContext )
{
    HRESULT hr;

    // store back buffer parameters
    g_BackBufferDesc = pBackBufferSurfaceDesc[0];

    V_RETURN( g_DialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnD3D9CreateDevice( pd3dDevice ) );

    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Arial", &g_pFont9 ) );

#ifdef DEBUG_VS
    dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
#endif
#ifdef DEBUG_PS
    dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
#endif
#ifdef D3DXFX_LARGEADDRESS_HANDLE
    dwShaderFlags |= D3DXFX_LARGEADDRESSAWARE;
#endif

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
// or that are tied to the back buffer size 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice,
                                   const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    // recreate pipline because it have to recreate surfaces    
    CreatePipeline(pd3dDevice);

    // store back buffer parameters
    g_BackBufferDesc = pBackBufferSurfaceDesc[0];
    V(pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &g_pBackBuffer));


    V_RETURN( g_DialogResourceManager.OnD3D9ResetDevice() );
    V_RETURN( g_SettingsDlg.OnD3D9ResetDevice() );

    if( g_pFont9 ) V_RETURN( g_pFont9->OnResetDevice() );

    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pSprite9 ) );
    g_pTxtHelper = new CDXUTTextHelper( g_pFont9, g_pSprite9, NULL, NULL, 15 );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_SampleUI.SetSize( 170, 300 );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mWorldViewProjection;
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and the zbuffer 
    //V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 45, 50, 170 ), 1.0f, 0 ) );


    {// run pieline and draw result
        CHECK_PIPELINE(return);
        if(g_pMSDKPipeline->m_pOCLPlugin)
        {//check folder with OCL filters each 100 frames
            static int count=0;
            if((count++)%100 == 0)
            {
                if(g_pMSDKPipeline->m_pOCLPlugin->UpdateProgramList())
                {
                    UpdateGUI_OpenCLFilterComboBox();
                }
                else
                    g_pMSDKPipeline->m_pOCLPlugin->UpdateProgram();
            }
        }

        // decode frame into texture and then copy it to screen
        assert(g_pBackBuffer);

        g_pMSDKPipeline->DecodeOneFrame(g_BackBufferDesc.Width, g_BackBufferDesc.Height,g_pBackBuffer,pd3dDevice); 

        {// measure execution time for whole pipeline with drawing
            LARGE_INTEGER       TicksCur;
            QueryPerformanceCounter(&TicksCur);
            // calc current execution time
            float T = (float)(TicksCur.QuadPart-g_TicksPrev.QuadPart) * (1000.0f/(float)g_Freq.QuadPart);
            // average result to get more smooth values
            g_AverTimeAll = 0.95f*g_AverTimeAll + 0.05f*T;
            g_TicksPrev = TicksCur;
        }
    }

    // Render the controls and text
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        RenderText();
        V( g_HUD.OnRender( fElapsedTime ) );
        V( g_SampleUI.OnRender( fElapsedTime ) );
        V( pd3dDevice->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                         void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
        case VK_F2:
            g_DrawCodeFLag = !g_DrawCodeFLag;
            break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
    case IDC_TOGGLEFULLSCREEN:
        DXUTToggleFullScreen(); break;
    case IDC_TOGGLEREF:
        DXUTToggleREF(); break;
    case IDC_CHANGEDEVICE:
        g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
    case IDC_PARAM1_SLIDER:
        {
            CHECK_PIPELINE(break);
            if(!g_pMSDKPipeline->m_pOCLPlugin)break;
            g_pMSDKPipeline->m_pOCLPlugin->m_Param1 = g_SampleUI.GetSlider(IDC_PARAM1_SLIDER)->GetValue();
            UpdateGUI_SlidersText();
            break;
        }
    case IDC_PARAM2_SLIDER:
        {
            CHECK_PIPELINE(break);
            if(!g_pMSDKPipeline->m_pOCLPlugin)break;
            g_pMSDKPipeline->m_pOCLPlugin->m_Param2 = g_SampleUI.GetSlider(IDC_PARAM2_SLIDER)->GetValue();
            UpdateGUI_SlidersText();
            break;
        }
    case IDC_OPENCL_FILTER_COMBOBOX:
        {
            CHECK_PIPELINE(break);
            if(!g_pMSDKPipeline->m_pOCLPlugin)break;
            int FilterIndex = (int)(__int64)g_SampleUI.GetComboBox(IDC_OPENCL_FILTER_COMBOBOX)->GetSelectedData();
            g_pMSDKPipeline->m_pOCLPlugin->SetProgram(FilterIndex);
            break;
        }
    case IDC_DECODE_CHECKBOX:
        {
            CHECK_PIPELINE(break);
            g_pMSDKPipeline->m_DECODEFlag = g_SampleUI.GetCheckBox(IDC_DECODE_CHECKBOX)->GetChecked();
            break;
        }
    case IDC_OPENCL_CHECKBOX:
        {
            CHECK_PIPELINE(break);
            if(!g_pMSDKPipeline->m_pOCLPlugin)break;
            g_pMSDKPipeline->m_pOCLPlugin->m_OCLFlag = g_SampleUI.GetCheckBox(IDC_OPENCL_CHECKBOX)->GetChecked();
            break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
    SAFE_DELETE(g_pMSDKPipeline);
    g_DialogResourceManager.OnD3D9LostDevice();
    g_SettingsDlg.OnD3D9LostDevice();
    if( g_pFont9 ) g_pFont9->OnLostDevice();
    //    if( g_pEffect9 ) g_pEffect9->OnLostDevice();
    SAFE_RELEASE( g_pSprite9 );
    SAFE_DELETE( g_pTxtHelper );
    SAFE_RELEASE(g_pBackBuffer);


}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9DestroyDevice();
    g_SettingsDlg.OnD3D9DestroyDevice();
    //    SAFE_RELEASE( g_pEffect9 );
    SAFE_RELEASE( g_pFont9 );

    SAFE_DELETE(g_pMSDKPipeline);
    SAFE_RELEASE(g_pBackBuffer);

}




//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int _tmain(int argc, TCHAR *argv[])
{



    QueryPerformanceCounter(&g_TicksPrev);
    QueryPerformanceFrequency(&g_Freq);


    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackMouse(OnMouseProc, true);
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );

    InitApp();
    DXUTInit( true, true, L"-d3d9ex -forcevsync:0" ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"SimpleSample" );
    DXUTCreateDevice( true, 1920, 1080 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    SAFE_FREE(g_pMSDKPipeline, delete);
    printf("Finish\n");

    return DXUTGetExitCode();
}

