/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SampleBase.cpp

//#include "stdafx.h"

#include <map>
#include <string>

#include <io.h>
#include <fcntl.h>

#include "SampleBase.h"

#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"

#include "SDKmisc.h"
#include "SDKmesh.h"


#define MAX_CONSOLE_LINES 500

///////////////////////////////////////////////////////////////////////////////

static BOOL RedirectIOToConsole()
{
    int conHandle;
    intptr_t stdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    FILE *fp;

    // allocate a console for this app
    if( !AllocConsole() )
        return FALSE;

    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&coninfo);
    coninfo.dwSize.Y = MAX_CONSOLE_LINES;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),coninfo.dwSize);

    // redirect unbuffered STDOUT to the console
    stdHandle = (intptr_t) GetStdHandle(STD_OUTPUT_HANDLE);
    assert(stdHandle && (intptr_t) INVALID_HANDLE_VALUE != stdHandle);
    conHandle = _open_osfhandle(stdHandle, _O_TEXT);
    assert(-1 != conHandle);
    fp = _fdopen( conHandle, "w" );
    assert(fp);
    *stdout = *fp;
    setvbuf( stdout, NULL, _IONBF, 0 );

    // redirect unbuffered STDIN to the console
    stdHandle = (intptr_t) GetStdHandle(STD_INPUT_HANDLE);
    assert(stdHandle && (intptr_t) INVALID_HANDLE_VALUE != stdHandle);
    conHandle = _open_osfhandle(stdHandle, _O_TEXT);
    assert(-1 != conHandle);
    fp = _fdopen( conHandle, "r" );
    assert(fp);
    *stdin = *fp;
    setvbuf( stdin, NULL, _IONBF, 0 );

    // redirect unbuffered STDERR to the console
    stdHandle = (intptr_t) GetStdHandle(STD_ERROR_HANDLE);
    assert(stdHandle && (intptr_t) INVALID_HANDLE_VALUE != stdHandle);
    conHandle = _open_osfhandle(stdHandle, _O_TEXT);
    assert(-1 != conHandle);
    fp = _fdopen( conHandle, "w" );
    assert(fp);
    *stderr = *fp;
    setvbuf( stderr, NULL, _IONBF, 0 );

    return TRUE;
}

typedef std::map<std::wstring, std::wstring> ParamsMap;

static ParamsMap ParseCommandLineOptions()
{
    ParamsMap outParams;
    int argc = 0;
    if( wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc) )
    {
        wchar_t *currentParamName = NULL;
        for( int i = 0; i < argc; ++i )
        {
            if( argv[i][0] == L'-' && argv[i][1] != L'.' && !isdigit(argv[i][1]) )
            {
                if( currentParamName ) outParams[currentParamName] = L"";
                currentParamName = argv[i] + 1; // skip '-'
            }
            else if( currentParamName )
            {
                outParams[currentParamName] = argv[i];
                currentParamName = NULL;
            }
        }
        if( currentParamName ) outParams[currentParamName] = L"";
        LocalFree(argv);
    }
    return outParams;
}

static const ParamsMap& GetCommandLineOptions()
{
    static ParamsMap params = ParseCommandLineOptions();
    return params;
}


//
// CSampleBase class implementation
//

CSampleBase::CSampleBase(void)
  : m_pDlgResourceManager(NULL)
  , m_pD3DSettingsDlg(NULL)
  , m_pRefWarningDlg(NULL)
  , m_pHUD(NULL)
  , m_pSampleUI(NULL)
  , m_pFont(NULL)
  , m_pSprite(NULL)
  , m_pTxtHelper(NULL)
  , m_pLogoEffect(NULL)
  , m_pLogoTechnique(NULL)
  , m_pLogoTexSRV(NULL)
  , m_pLogoPositionVariable(NULL)
  , m_pLogoColorVariable(NULL)
  , m_pGuiRasterState(NULL)
  , m_drawHelp(false)
  , m_noGui(GetArgBool(L"nogui"))
  , m_yHUD(0)
  , m_fLogoDisplayTime(0)
  , m_LogoColor(1, 1, 1, 1)
  , m_perfNumFramesRemain(GetArgInt(L"iters", -1)) // infinite by default
  , m_perfSkipFramesRemain(GetArgInt(L"skip", 0))
{
}

CSampleBase::~CSampleBase(void)
{
    SAFE_DELETE(m_pRefWarningDlg);
    SAFE_DELETE(m_pSampleUI);
    SAFE_DELETE(m_pHUD);
    SAFE_DELETE(m_pD3DSettingsDlg);
    SAFE_DELETE(m_pDlgResourceManager);
}

bool CSampleBase::GetArgBool(const wchar_t *name) const
{
    assert(name);
    ParamsMap::const_iterator it = GetCommandLineOptions().find(name);
    if( GetCommandLineOptions().end() == it ) return false;
    if( it->second.empty() ) return true;
    return _wtof(it->second.c_str()) != 0;
}

int CSampleBase::GetArgInt(const wchar_t *name, int def) const
{
    assert(name);
    ParamsMap::const_iterator it = GetCommandLineOptions().find(name);
    if( GetCommandLineOptions().end() == it ) return def;
    if( it->second.empty() ) return 1;
    return _wtoi(it->second.c_str());
}

double CSampleBase::GetArgFloat(const wchar_t *name, double def) const
{
    assert(name);
    ParamsMap::const_iterator it = GetCommandLineOptions().find(name);
    if( GetCommandLineOptions().end() == it ) return def;
    if( it->second.empty() ) return 1;
    return _wtof(it->second.c_str());
}

const wchar_t* CSampleBase::GetArgString(const wchar_t *name) const
{
    assert(name);
    ParamsMap::const_iterator it = GetCommandLineOptions().find(name);
    if( GetCommandLineOptions().end() == it ) return NULL;
    return it->second.c_str();
}

int CSampleBase::Run()
{
    StartupInfo si;
    wcscpy_s(si.WindowTitle, L"Sample Base");
    si.WindowWidth = GetArgInt(L"width", 1024);
    si.WindowHeight = GetArgInt(L"height", 768);
    si.ShowConsole = true;
    si.Windowed    = !GetArgBool(L"fullscreen");
    si.ShowCursorWhenFullScreen = true;
    si.ClipCursorWhenFullScreen = true;
    si.ReceiveMouseMove = false;

    OnInitApp(&si);

    if( si.ShowConsole )
    {
        RedirectIOToConsole();
    }


    //
    // set DXUT callbacks
    //

    struct helper
    {
        static bool CALLBACK IsDeviceAcceptable(
            UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType,
            DXGI_FORMAT BufferFormat, bool bWindowed, void* pUserContext )
        {
            return reinterpret_cast<CSampleBase*>(pUserContext)
                ->IsDeviceAcceptable(Adapter,Output,DeviceType,BufferFormat,bWindowed);
        }

        static HRESULT CALLBACK OnCreateDevice(
            ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC*
            in_pBufferSurfaceDesc, void* pUserContext )
        {
            return reinterpret_cast<CSampleBase*>(pUserContext)
                ->OnCreateDevice(pd3dDevice, in_pBufferSurfaceDesc);
        }

        static HRESULT CALLBACK OnResizedSwapChain(
            ID3D10Device* pd3dDevice, IDXGISwapChain *pSwapChain,
            const DXGI_SURFACE_DESC* in_pBufferSurfaceDesc, void* pUserContext )
        {
            return reinterpret_cast<CSampleBase*>(pUserContext)
                ->OnResizedSwapChain(pd3dDevice, pSwapChain, in_pBufferSurfaceDesc);
        }

        static void CALLBACK OnFrameRender(
            ID3D10Device* pd3dDevice,double fTime,float fElapsedTime,void* pUserContext)
        {
            reinterpret_cast<CSampleBase*>(pUserContext)->OnFrameRender(pd3dDevice, fTime, fElapsedTime);
        }

        static void CALLBACK OnReleasingSwapChain( void* pUserContext )
        {
            reinterpret_cast<CSampleBase*>(pUserContext)->OnReleasingSwapChain();
        }

        static void CALLBACK OnDestroyDevice( void* pUserContext )
        {
            reinterpret_cast<CSampleBase*>(pUserContext)->OnDestroyDevice();
        }

        static LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
            bool* pbNoFurtherProcessing, void* pUserContext )
        {
            return reinterpret_cast<CSampleBase*>(pUserContext)
                ->MsgProc(hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing);
        }

        static void CALLBACK KeyboardProc(
            UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
        {
            reinterpret_cast<CSampleBase*>(pUserContext)->KeyboardProc( nChar, bKeyDown, bAltDown );
        }

        static void CALLBACK MouseProc( bool bLeftButtonDown, bool bRightButtonDown,
            bool bMiddleButtonDown, bool bSideButton1Down, bool bSideButton2Down,
            int nMouseWheelDelta, int xPos, int yPos, void* pUserContext )
        {
            reinterpret_cast<CSampleBase*>(pUserContext)->MouseProc( bLeftButtonDown, bRightButtonDown,
                bMiddleButtonDown, bSideButton1Down, bSideButton2Down,
                nMouseWheelDelta, xPos, yPos );
        }

        static void CALLBACK OnFrameMove(
            double fTime, float fElapsedTime, void* pUserContext)
        {
            reinterpret_cast<CSampleBase*>(pUserContext)->OnFrameMove(fTime, fElapsedTime);
        }

        static bool CALLBACK ModifyDeviceSettings(
            DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
        {
            return reinterpret_cast<CSampleBase*>(pUserContext)
                ->ModifyDeviceSettings(pDeviceSettings);
        }

        static bool CALLBACK OnDeviceRemoved(void* pUserContext)
        {
            return reinterpret_cast<CSampleBase*>(pUserContext)->OnDeviceRemoved();
        }
    };


    DXUTSetCallbackD3D10DeviceAcceptable(   &helper::IsDeviceAcceptable,    this );
    DXUTSetCallbackD3D10DeviceCreated(      &helper::OnCreateDevice,        this );
    DXUTSetCallbackD3D10SwapChainResized(   &helper::OnResizedSwapChain,    this );
    DXUTSetCallbackD3D10SwapChainReleasing( &helper::OnReleasingSwapChain,  this );
    DXUTSetCallbackD3D10DeviceDestroyed(    &helper::OnDestroyDevice,       this );
    DXUTSetCallbackD3D10FrameRender(        &helper::OnFrameRender,         this );
    DXUTSetCallbackMsgProc(                 &helper::MsgProc,               this );
    DXUTSetCallbackKeyboard(                &helper::KeyboardProc,          this );
    DXUTSetCallbackMouse(                   &helper::MouseProc, si.ReceiveMouseMove, this );
    DXUTSetCallbackFrameMove(               &helper::OnFrameMove,           this );
    DXUTSetCallbackDeviceChanging(          &helper::ModifyDeviceSettings,  this );
    DXUTSetCallbackDeviceRemoved(           &helper::OnDeviceRemoved,       this );


    //
    // init DXUT
    //

    // Parse the command line, show msgboxes on error, no extra command line params
    DXUTInit( true, true, NULL );
    DXUTSetCursorSettings( si.ShowCursorWhenFullScreen, si.ClipCursorWhenFullScreen );

    DXUTCreateWindow(si.WindowTitle);

    // Disable DXUT's default gamma-corrected SRGB colors, to preserve compatibility 
    // with sample code written prior to March 2008 DXSDK
    DXUTSetIsInGammaCorrectMode(false);

    DXUTCreateDevice(si.Windowed, si.WindowWidth, si.WindowHeight);


    DXUTMainLoop(); // Enter into the DXUT render loop

    if( -1 == m_perfNumFramesRemain )
    {
    }

    int result = DXUTGetExitCode();

    DXUTDestroyState(); // cleanup DXUT resources

    return result;
}

CDXUTDialog* CSampleBase::GetSampleUI() const
{
    return m_pSampleUI;
}

CDXUTDialog* CSampleBase::GetHUD() const
{
    return m_pHUD;
}

int CSampleBase::GetNextYPosHUD(int offset)
{
    return m_yHUD += offset;
}

bool CSampleBase::GetDrawHelp() const
{
    return m_drawHelp;
}

void CSampleBase::SetDrawHelp(bool drawHelp)
{
    m_drawHelp = drawHelp;
}

void CSampleBase::SetLogoColor(D3DXCOLOR color)
{
    m_LogoColor.r = color.r;
    m_LogoColor.g = color.g;
    m_LogoColor.b = color.b;
}

//
// basic DXUT callbacks implementation
//

bool CSampleBase::IsDeviceAcceptable( UINT Adapter, UINT Output,
    D3D10_DRIVER_TYPE DeviceType, DXGI_FORMAT BufferFormat, bool bWindowed )
{
    return true;
}

HRESULT CSampleBase::OnCreateDevice(
    ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC* in_pBufferSurfaceDesc)
{
    HRESULT hr;


    //
    // Initialize GUI resources
    //

    V_RETURN( m_pDlgResourceManager->OnD3D10CreateDevice( pd3dDevice ) );
    V_RETURN( m_pD3DSettingsDlg->OnD3D10CreateDevice( pd3dDevice ) );
    V_RETURN( D3DX10CreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                L"Arial", &m_pFont ) );
    V_RETURN( D3DX10CreateSprite( pd3dDevice, 512, &m_pSprite ) );
    m_pTxtHelper = new CDXUTTextHelper( NULL, NULL, m_pFont, m_pSprite, 15 );


    //
    // Initialize Logo resources
    //

    WCHAR str[MAX_PATH];

    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"..\\Shaders\\intel_embossed.png" ) );
    V_RETURN( D3DX10CreateShaderResourceViewFromFile( pd3dDevice, str, NULL, NULL, &m_pLogoTexSRV, NULL ) );

    D3DX10_IMAGE_INFO ii;
    V_RETURN( D3DX10GetImageInfoFromFile(str, NULL, &ii, NULL) );

    m_fLogoWidth   = (FLOAT) ii.Width;
    m_fLogoHeight  = (FLOAT) ii.Height;


    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"..\\Shaders\\logo.fx" ) );
    V_RETURN( D3DX10CreateEffectFromFile( str, NULL, NULL, "fx_4_0",
        D3D10_SHADER_ENABLE_STRICTNESS, 0, pd3dDevice, NULL, NULL, &m_pLogoEffect, NULL, NULL ) );
    m_pLogoEffect->GetVariableByName("g_LogoTexture")->AsShaderResource()->SetResource(m_pLogoTexSRV);
    m_pLogoTechnique        = m_pLogoEffect->GetTechniqueByName( "RenderLogo" );
    m_pLogoPositionVariable = m_pLogoEffect->GetVariableByName( "LogoPosition" )->AsVector();
    m_pLogoColorVariable    = m_pLogoEffect->GetVariableByName( "LogoColor" )->AsVector();



    //
    // Create rasterizer state for the logo and GUI drawing
    //

    D3D10_RASTERIZER_DESC rd;
    ZeroMemory(&rd, sizeof(D3D10_RASTERIZER_DESC));

    rd.FillMode = D3D10_FILL_SOLID;
    rd.CullMode = D3D10_CULL_BACK;

    pd3dDevice->CreateRasterizerState( &rd, &m_pGuiRasterState ); // Create the rasterizer state

    return S_OK;
}

void CSampleBase::OnDestroyDevice()
{
    m_pDlgResourceManager->OnD3D10DestroyDevice();
    m_pD3DSettingsDlg->OnD3D10DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_RELEASE( m_pGuiRasterState );
    SAFE_RELEASE( m_pFont );
    SAFE_RELEASE( m_pSprite );
    SAFE_DELETE( m_pTxtHelper );
    SAFE_RELEASE( m_pLogoEffect );
    SAFE_RELEASE( m_pLogoTexSRV );
}

HRESULT CSampleBase::OnResizedSwapChain(ID3D10Device* pd3dDevice,
    IDXGISwapChain *pSwapChain, const DXGI_SURFACE_DESC* in_pBufferSurfaceDesc)
{
    HRESULT hr;

    V_RETURN(m_pDlgResourceManager->OnD3D10ResizedSwapChain(pd3dDevice, in_pBufferSurfaceDesc));
    V_RETURN(m_pD3DSettingsDlg->OnD3D10ResizedSwapChain(pd3dDevice, in_pBufferSurfaceDesc));

    m_pHUD->SetSize( 170, 170 );
    m_pHUD->SetLocation( in_pBufferSurfaceDesc->Width - m_pHUD->GetWidth(), 0 );

    m_pSampleUI->SetSize( 170, 450 );
    m_pSampleUI->SetLocation( in_pBufferSurfaceDesc->Width - m_pSampleUI->GetWidth(), m_yHUD + 30 );

    m_pRefWarningDlg->SetLocation(
        (in_pBufferSurfaceDesc->Width - m_pRefWarningDlg->GetWidth()) / 2,
        (in_pBufferSurfaceDesc->Height - m_pRefWarningDlg->GetHeight()) / 2 );

    //
    // Update logo position
    //

    D3DXVECTOR4 pos;
    pos.x = -1;
    pos.w = -1;
    pos.z = pos.x + 2.0f / (float) in_pBufferSurfaceDesc->Width * m_fLogoWidth;
    pos.y = pos.w + 2.0f / (float) in_pBufferSurfaceDesc->Height * m_fLogoHeight;
    m_pLogoPositionVariable->SetFloatVector(pos);


    //
    // update label on 'Full Screen' button
    //
    GetHUD()->GetButton(IDC_TOGGLEFULLSCREEN)->SetText(
        DXUTIsWindowed() ? L"Full screen On (F11)" : L"Full screen Off (F11)");


    return S_OK;
}

void CSampleBase::OnFrameRender(ID3D10Device* pd3dDevice, double fTime, float fElapsedTime)
{
    if( 0 == m_perfSkipFramesRemain )
    {
        m_perfSkipFramesRemain = -1;
    }

    OnClearRenderTargets(pd3dDevice, fTime, fElapsedTime);


    //
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    //
    if( m_pD3DSettingsDlg->IsActive() )
    {
        m_pD3DSettingsDlg->OnRender(fElapsedTime);
        return;
    }


    if( m_pRefWarningDlg->GetVisible() )
    {
        m_pRefWarningDlg->OnRender(fElapsedTime);
        return;
    }


    //
    // render user scene
    //
    OnRenderScene(pd3dDevice, fTime, fElapsedTime);


    //
    // Render the GUI, text, logo
    //
    if( !m_noGui )
    {
        pd3dDevice->RSSetState(m_pGuiRasterState);

        // draw GUI
        m_pHUD->OnRender(fElapsedTime);
        m_pSampleUI->OnRender(fElapsedTime);

        // draw logo
        pd3dDevice->IASetInputLayout(NULL);
        pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_pLogoTechnique->GetPassByIndex(0)->Apply(0);
        if( !m_drawHelp )
        {
            pd3dDevice->Draw(4, 0);
        }

        // draw text; alpha blending is still enabled since logo drawing
        m_pTxtHelper->Begin();
        OnRenderText(m_pTxtHelper, fTime, fElapsedTime);
        m_pTxtHelper->End();

        // disable alpha blending
        m_pLogoTechnique->GetPassByIndex(1)->Apply(0);
    }


    // perf
    if( m_perfSkipFramesRemain < 0 )
    {
        if( m_perfNumFramesRemain > 0 )
        {
            m_perfNumFramesRemain--;
            if (m_perfNumFramesRemain==0)
            {
                DXUTShutdown();
            }
        }
    }
    else if (m_perfSkipFramesRemain > 0)
    {
        m_perfSkipFramesRemain--;
        if (m_perfSkipFramesRemain == 0)
        {
            m_perfSkipFramesRemain = -1;
        }
    }
}

void CSampleBase::OnReleasingSwapChain()
{
    m_pDlgResourceManager->OnD3D10ReleasingSwapChain();
}

void CSampleBase::OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen();
            break;
        case IDC_TOGGLEREF:
            if( D3D10_DRIVER_TYPE_REFERENCE == DXUTGetDeviceSettings().d3d10.DriverType )
                DXUTToggleREF();
            else
                m_pRefWarningDlg->SetVisible(true);
            break;
        case IDC_CHANGEDEVICE:
            m_pD3DSettingsDlg->SetActive( !m_pD3DSettingsDlg->IsActive() );
            break;

        case IDC_TOGGLEREF_YES:
            m_pRefWarningDlg->SetVisible(false);
            DXUTToggleREF();
            break;
        case IDC_TOGGLEREF_NO:
            m_pRefWarningDlg->SetVisible(false);
            break;
    }
}

LRESULT CSampleBase::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                              LPARAM lParam, bool* pbNoFurtherProcessing )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = m_pDlgResourceManager->MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( m_pD3DSettingsDlg->IsActive() )
    {
        m_pD3DSettingsDlg->MsgProc( hWnd, uMsg, wParam, lParam );
        return 1;
    }

    if( m_pRefWarningDlg->GetVisible() )
    {
        m_pRefWarningDlg->MsgProc( hWnd, uMsg, wParam, lParam );
        return 1;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = m_pHUD->MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = m_pSampleUI->MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    return 0;
}

void CSampleBase::KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
        case VK_F1:
            m_drawHelp = !m_drawHelp;
            break;
        }
    }
}

void CSampleBase::MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
    bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta, int xPos, int yPos )
{
}

void CSampleBase::OnFrameMove( double fTime, float fElapsedTime )
{
    m_fLogoDisplayTime += fElapsedTime;//1.0f / DXUTGetFPS();

    if( m_fLogoDisplayTime > 10 )
    {
        if( m_fLogoDisplayTime > 15 )
        {
            m_LogoColor.a = 0;
        }
        else
        {
            m_LogoColor.a = 1.0f - (m_fLogoDisplayTime - 10) / 5;
        }
    }
    else
    {
        if( m_fLogoDisplayTime > 3 )
        {
            m_LogoColor.a = 1;
        }
        else
        {
            if( m_fLogoDisplayTime > 1 )
            {
                m_LogoColor.a = (m_fLogoDisplayTime - 1) / 2;
            }
            else
            {
                m_LogoColor.a = 0;
            }
        }
    }

    m_LogoColor.a *= 0.5f; // change to adjust logo transparency
    m_pLogoColorVariable->SetFloatVector(m_LogoColor);
}

bool CSampleBase::ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings)
{
//    pDeviceSettings->d3d10.CreateFlags |= D3D10_CREATE_DEVICE_SWITCH_TO_REF;

    // Turn vsync off
    pDeviceSettings->d3d10.SyncInterval = 0;

    return true;
}

bool CSampleBase::OnDeviceRemoved()
{
    return false;
}


//
// other callbacks
//

void CSampleBase::OnInitApp(StartupInfo *io_StartupInfo)
{
    m_pDlgResourceManager = new CDXUTDialogResourceManager();
    m_pD3DSettingsDlg     = new CD3DSettingsDlg();
    m_pRefWarningDlg      = new CDXUTDialog();
    m_pHUD                = new CDXUTDialog();
    m_pSampleUI           = new CDXUTDialog();

    m_pD3DSettingsDlg->Init(m_pDlgResourceManager);
    m_pRefWarningDlg->Init(m_pDlgResourceManager);
    m_pHUD->Init(m_pDlgResourceManager);
    m_pSampleUI->Init(m_pDlgResourceManager);


    struct helper
    {
        static void CALLBACK OnGUIEvent(
            UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
        {
            reinterpret_cast<CSampleBase*>(pUserContext)->OnGUIEvent(nEvent, nControlID, pControl);
        }
    };

    m_pRefWarningDlg->SetCallback( &helper::OnGUIEvent, this );
    m_pHUD->SetCallback( &helper::OnGUIEvent, this );
    m_pSampleUI->SetCallback( &helper::OnGUIEvent, this );

    GetHUD()->AddButton( IDC_TOGGLEFULLSCREEN,
        DXUTIsWindowed() ? L"Full screen On (F11)" : L"Full screen Off (F11)",
        35, GetNextYPosHUD(10), 125, 22, VK_F11 );

    ///GetHUD()->AddButton( IDC_TOGGLEREF,    L"Toggle REF (F3)",    35, GetNextYPosHUD(24), 125, 22, VK_F3 );
    ///GetHUD()->AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, GetNextYPosHUD(24), 125, 22, VK_F2 );

    const int w = 300;
    const int h = 150;

    m_pRefWarningDlg->AddStatic(-1, L"WARNING\n"
                                    L"Switching to reference rasterizer will lead to\n"
                                    L"extremely low frame rate. Do you want to continue?", (w-300)/2, 10, 300, 50);
    m_pRefWarningDlg->AddButton(IDC_TOGGLEREF_YES, L"Yes",w/2-10-100, h-40, 100, 30, 'Y');
    m_pRefWarningDlg->AddButton(IDC_TOGGLEREF_NO,  L"No",  w/2+10, h-40, 100, 30, 'N');
    m_pRefWarningDlg->SetSize(w, h);
    m_pRefWarningDlg->SetBackgroundColors(0xff808080); // set gray background color
    m_pRefWarningDlg->SetVisible(false);
}


void CSampleBase::OnClearRenderTargets(ID3D10Device* pd3dDevice, double fTime, float fElapsedTime)
{
    //
    // Clear the back buffer
    //
    ID3D10RenderTargetView *pRTV = DXUTGetD3D10RenderTargetView();
    pd3dDevice->ClearRenderTargetView(pRTV, D3DXVECTOR4(0.64f, 0.70f, 0.93f, 1.0f));

    //
    // Clear the depth stencil
    //
    ID3D10DepthStencilView *pDSV = DXUTGetD3D10DepthStencilView();
    pd3dDevice->ClearDepthStencilView(pDSV, D3D10_CLEAR_DEPTH, 1.0, 0);
}

void CSampleBase::OnRenderScene(ID3D10Device* pd3dDevice, double fTime, float fElapsedTime)
{
    // should be done in the derived class
}

void CSampleBase::OnRenderText(CDXUTTextHelper* pTextHelper, double fTime, float fElapsedTime)
{
    pTextHelper->SetInsertionPos( 2, 0 );
    pTextHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    pTextHelper->DrawTextLine( DXUTGetFrameStats( true ) );
    pTextHelper->DrawTextLine( DXUTGetDeviceStats() );

    pTextHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    pTextHelper->DrawTextLine( GetDrawHelp() ? L"Hide help: F1" : L"Show help: F1" );
}

LRESULT CSampleBase::OnWidgetNotify(CWidgetBase *pWidget, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

// end of file
