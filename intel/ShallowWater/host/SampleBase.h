/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SampleBase.h
///
/// Definition of the CSampleBase class.
///
/// The goal and design of SampleBase framework targets the traditional
/// functionality used in every sample. It encapsulates the initialization,
/// running, and termination of a sample. A sample built on the framework
/// must have one and only one object of a class derived from CSampleBase.
/// CSampleBase was designed to be a base class for building graphics samples.

#pragma once

#include "DXUT.h"
#include <d3dx10core.h>

// forward declarations
class CDXUTControl;
class CDXUTDialogResourceManager;
class CD3DSettingsDlg;
class CDXUTDialog;
class CDXUTTextHelper;
class CWidgetBase;

///////////////////////////////////////////////////////////////////////////////


struct StartupInfo
{
    WCHAR        WindowTitle[256];  ///< Title bar caption for the application window
    int          WindowWidth;  ///< Initial width of the application window
    int          WindowHeight; ///< Initial height of the application window
    bool         ShowConsole;  ///< Flag shows that application need a console window
    bool         Windowed;     ///< If \b true, the application will start in windowed mode and in full-screen otherwise
    bool         ShowCursorWhenFullScreen; ///< If \b true, the cursor will be visible when the application is running in full-screen mode.
    bool         ClipCursorWhenFullScreen; ///< If \b true, the cursor will be restricted from exiting the screen boundaries when the application is running in full screen mode.
    bool         ReceiveMouseMove; ///< If true, will call MouseProc on mouse move as well
};

///////////////////////////////////////////////////////////////////////////////
// CSampleBase class declaration

class CSampleBase
{
    CDXUTDialogResourceManager*  m_pDlgResourceManager;  ///< Manager for shared resources of dialogs.
    CD3DSettingsDlg*             m_pD3DSettingsDlg;      ///< Device settings dialog.
    CDXUTDialog*                 m_pRefWarningDlg;       ///< Switch to reference rasterizer warning.
    CDXUTDialog*                 m_pHUD;                 ///< Manages the 3D UI.
    CDXUTDialog*                 m_pSampleUI;            ///< Dialog for sample specific controls.

    ID3DX10Font*                 m_pFont;                ///< Font for drawing text.
    ID3DX10Sprite*               m_pSprite;              ///< Sprite for batching text drawing.
    CDXUTTextHelper*             m_pTxtHelper;           ///< The helper object that keeps track of text position, and color.

    ID3D10Effect*                m_pLogoEffect;          ///< Interface to manage the set of state objects, resources and shaders for the effect
    ID3D10EffectTechnique*       m_pLogoTechnique;       ///< The techniques and their collection of passes used for logo drawing
    ID3D10ShaderResourceView*    m_pLogoTexSRV;
    ID3D10EffectVectorVariable*  m_pLogoPositionVariable;
    ID3D10EffectVectorVariable*  m_pLogoColorVariable;
    D3DXCOLOR                    m_LogoColor;
    FLOAT m_fLogoDisplayTime;
    FLOAT m_fLogoWidth;
    FLOAT m_fLogoHeight;

    ID3D10RasterizerState*       m_pGuiRasterState;      ///< Rasterizer state used in GUI and logo rendering

    int   m_perfNumFramesRemain;
    int   m_perfSkipFramesRemain;

    int   m_yHUD;          ///< Current Y position for the HUD
    bool  m_drawHelp;      ///< Are we showing help?
    bool  m_noGui;         ///< Hide all gui elements

    /// Called at the end of every frame, and whenever the application needs to paint the scene.
    void OnFrameRender(ID3D10Device* pd3dDevice, ///< Pointer to the ID3D10Device Interface device used for rendering.
                       double fTime, ///< Time elapsed since the application started, in seconds.
                       float fElapsedTime ///< Time elapsed since the last frame, in seconds.
                      );

    /// private constructor prevents object copying
    CSampleBase(const CSampleBase&);
    CSampleBase& operator = (const CSampleBase&);

protected:
    static const int IDC_TOGGLEFULLSCREEN = 1;
    static const int IDC_TOGGLEREF        = 2;
    static const int IDC_CHANGEDEVICE     = 3;

    static const int IDC_TOGGLEREF_YES    = 5;
    static const int IDC_TOGGLEREF_NO     = 6;


public:
    /// Returns pointer to a dialog, which handles input and rendering for the sample-specific controls.
    CDXUTDialog* GetSampleUI() const;

    /// Returns pointer to a dialog, which handles input and rendering for controls shared across all samples.
    CDXUTDialog* GetHUD() const;

    /// Returns \k true if the application should draw a short help text.
    bool GetDrawHelp() const;

    /// Sets whether the application should draw a short help text or not.
    void SetDrawHelp(bool drawHelp);

    /// \brief Returns next position in the HUD dialog shifting the current position by offset value.
    ///
    /// Mostly used when a widget needs to place its own controls on a proper place.
    int GetNextYPosHUD(int offset  ///< Offset value, in pixels
                      );

    void SetLogoColor(D3DXCOLOR color);


    bool GetArgBool(const wchar_t *name) const;
    int GetArgInt(const wchar_t *name, int def = 0) const; ///< returns def if parameter does not exist; returns 1 if value is not specified
    double GetArgFloat(const wchar_t *name, double def = 0) const; ///< returns def if parameter does not exist; returns 1 if value is not specified
    const wchar_t* GetArgString(const wchar_t *name) const; ///< returns NULL if parameter does not exists; returns empty string if value is not specified

public:
    /// \brief Called by the framework to build an enumerated list of all possible Direct3D 10 devices.
    ///
    /// The framework then selects the best device for creation among this list. This function
    /// allows the application to prevent unwanted devices from being added to the list.
    virtual bool IsDeviceAcceptable(UINT Adapter, ///< Adapter ordinal of the Direct3D 10 device.
                                    UINT Output, ///< Output ordinal of the Direct3D 10 device.
                                    D3D10_DRIVER_TYPE DeviceType, ///< Driver type of the Direct3D 10 device.
                                    DXGI_FORMAT BufferFormat, ///< Back buffer format of the Direct3D 10 device.
                                    bool bWindowed ///< Indicates windowed mode.
                                   );

    /// \brief Called after the Direct3D 10 device has been created.
    ///
    /// Device creation will happen during application initialization and if the device is changed.
    virtual HRESULT OnCreateDevice(ID3D10Device* pd3dDevice, ///< Pointer to the newly created ID3D10Device Interface device.
                                   const DXGI_SURFACE_DESC* in_pBufferSurfaceDesc ///< Pointer to the backbuffer surface description.
                                  );

    /// Called when changing the window size and when a new device has been created and OnCreateDevice was called.
    virtual HRESULT OnResizedSwapChain(ID3D10Device* pd3dDevice,
                                       IDXGISwapChain *pSwapChain,
                                       const DXGI_SURFACE_DESC* in_pBufferSurfaceDesc ///< Pointer to the backbuffer surface description.
                                      );

    /// Called by the framework once each frame, before the application renders the scene.
    virtual void OnFrameMove(double fTime, ///< Time elapsed since the application started, in seconds.
                             float fElapsedTime ///< Time elapsed since the last frame, in seconds.
                            );

    /// \brief Called when the Direct3D 10 swap chain is about to be released.
    ///
    /// The swap chain will release when the window is resized or the resolution changes.
    virtual void OnReleasingSwapChain();

    /// \brief Called when the device is about to destroy.
    ///
    /// In this function the application should release all device-dependent resources.
    virtual void OnDestroyDevice();

    /// \brief Function is associated with the actual events processed by both, the UI and the HUD.
    ///
    /// This function is used to handle the interaction between the controls.
    virtual void OnGUIEvent(UINT nEvent,             ///<
                            int nControlID,          ///< IDC_ control identifier
                            CDXUTControl* pControl   ///< Pointer to a control rising the event
                           );

    /// \brief Windows messages processing routine.
    ///
    /// Called every time the application receives a message. The framework
    /// passes all messages to the application, and don't process further
    /// messages if the application says not to.
    virtual LRESULT MsgProc(HWND   hWnd,    ///< Handle to the application window
                            UINT   uMsg,    ///< Windows message code
                            WPARAM wParam,  ///< wParam
                            LPARAM lParam,  ///< lParam
                            bool* pbNoFurtherProcessing
                           );

    /// The framework consolidates the keyboard messages and passes them to the app via this function.
    virtual void KeyboardProc(UINT nChar,       ///< A virtual-key code for the key
                              bool bKeyDown,    ///< \b true if key is down and \b false if the key is up
                              bool bAltDown     ///< \b true if the ALT key is also down.
                             );

    /// \brief Mouse event handling function, called by the framework when it receives mouse events.
    ///
    /// This event handling mechanism is provided to simplify handling mouse
    /// messages through the window's message pump, but the application may
    /// still handle mouse messages directly through the MsgProc function.
    virtual void MouseProc(
        bool bLeftButtonDown,      ///< The left mouse button is down.
        bool bRightButtonDown,     ///< The right mouse button is down.
        bool bMiddleButtonDown,    ///< The middle mouse button is down.
        bool bSideButton1Down,     ///< Microsoft Windows 2000/Windows XP: The first X button is down.
        bool bSideButton2Down,     ///< Windows 2000/Windows XP: The second X button is down.
        int nMouseWheelDelta,      ///< The distance and direction the mouse wheel has rolled, expressed in multiples or divisions of WHEEL_DELTA, which is 120. A positive value indicates that the wheel was rotated forward, away from the user; a negative value indicates that the wheel was rotated backward, toward the user.
        int xPos,                  ///< x-coordinate of the pointer, relative to the upper-left corner of the client area.
        int yPos                   ///< y-coordinate of the pointer, relative to the upper-left corner of the client area.
        );

    /// \brief Called by the framework to allow changes in device settings before the device is created.
    ///
    /// Before a device is created by the framework, this function will be called to
    /// allow the application to examine or change the device settings before the device is
    /// created. This allows an application to modify the device creation settings as it sees fit.
    ///
    /// This function also allows applications to reject changing the device altogether.
    /// Returning \b false from inside this function will notify the framework to
    /// keep using the current device instead of changing to the new device.
    ///
    /// Anything in pDeviceSettings can be changed by the application.
    ///
    /// \return The application should return \b true to continue creating the device.
    ///         If the application returns \b false, the framework will continue using
    ///         the current device if one exists.
    ///
    virtual bool ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings ///< Pointer to a structure that contains the settings for the new device.
                                     );

    /// \brief The function, called by the framework to allow the application
    /// to respond to a Direct3D device removed event.
    ///
    /// If the Direct3D device is removed such as when a laptop undocks from a docking station,
    /// the framework notifies that application with this function call. This allows the
    /// application to respond to this event. If the application returns false, the framework
    /// will not try to find a new device and will shutdown.
    virtual bool OnDeviceRemoved();

    /// Called only once after framework initialization when the D3D device is not created yet.
    virtual void OnInitApp(StartupInfo *io_StartupInfo ///< Pointer to a structure that contains startup settings
                          );

    /// Called when the framework needs to clear the back buffer.
    virtual void OnClearRenderTargets(ID3D10Device* pd3dDevice, ///< Pointer to the ID3D10Device Interface device used for rendering.
                                      double fTime, ///< Time elapsed since the application started, in seconds.
                                      float fElapsedTime ///< Time elapsed since the last frame, in seconds.
                                     );

    /// Called when the framework needs to render the entire scene.
    virtual void OnRenderScene(ID3D10Device* pd3dDevice, ///< Pointer to the ID3D10Device Interface device used for rendering.
                               double fTime, ///< Time elapsed since the application started, in seconds.
                               float fElapsedTime ///< Time elapsed since the last frame, in seconds.
                              );

    /// \brief Called when the framework needs to render UI text.
    ///
    /// It allows application to draw lines of text anywhere on the screen
    virtual void OnRenderText(CDXUTTextHelper* pTextHelper,
                              double fTime, ///< Time elapsed since the application started, in seconds.
                              float fElapsedTime ///< Time elapsed since the last frame, in seconds.
                             );

    /// Widgets call this function when they want to notify the application about some events.
    virtual LRESULT OnWidgetNotify(CWidgetBase *pWidget, ///< Pointer to the widget rising the event
                                   WPARAM wParam,        ///< Message parameter
                                   LPARAM lParam         ///< Message parameter
                                  );
public:
    CSampleBase(void);             ///< Default constructor.
    virtual ~CSampleBase(void);    ///< Destructor.
    int Run();                     ///< Actual entry point for the application.
};

///////////////////////////////////////////////////////////////////////////////
// end of file
