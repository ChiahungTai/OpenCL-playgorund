/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file fluidsimhost.h
 *
 * @brief Host side class objects for the fluid simulator
 */
#pragma warning( disable: 4995 ) // 'name' was marked as #pragma deprecated

// DXUT headers 
#include <DXUT.h>
#include <DXUTcamera.h>
#include <DXUTgui.h>
#include <SDKmisc.h>

// host library headers include
#include "mathstub.h"


#include "CL\cl.h"



// Collada headers
#include <SceneGraph.h>
#include <MsgListenerWrapper.h>
#include <..\D3D10\RenderD3D10Impl.h>

// basedx header 
#include "SampleBase.h"

// fluid self headers 
#include "util/memory.h"
#include "Util/CFileMemory.h"
#include "scenes_creation.h"
#include "CommonDataStruct.h"
#include "common/physics_3D_fluid_sim_common.h"
#include "CSky.h"
#include "CVisual3DGathered.h"
#include "CSceneVisual.h"


#if defined(__INTEL_COMPILER)
#pragma warning(disable : 1684)
#else
#pragma warning(disable : 4311)
#pragma warning(disable : 4312)
#endif

// The scale for the boat loaded from the collada file 
#define BOAT_XSCALE 1.6f
#define BOAT_YSCALE 1.6f
#define BOAT_ZSCALE 1.6f

// Animation step to play an animation from the collada file
#define ANIMATION_TIME_STEP 0.03f

// Macros for safely writing messages into the log file and to the console 
#define WRITE_LOGFILE(_str) {if(m_pLogfile){fprintf(m_pLogfile, _str);fflush(m_pLogfile);}fprintf(stderr, _str);}
#define WRITE_LOGFILE2(_str,_arg2) {if(m_pLogfile){fprintf(m_pLogfile, _str,_arg2);fflush(m_pLogfile);}fprintf(stderr, _str,_arg2);}
#define CLOSE_LOGFILE() if(m_pLogfile){fclose(m_pLogfile);m_pLogfile=NULL;}

/**
 * @class CModelViewerCamera_DefPos
 *
 * brief A slightly modified DXUT CModelViewerCamera camera, 
 *       modified to be able to restore position with the Home key 
 */
class CModelViewerCamera_DefPos: 
    public CModelViewerCamera
{
public:

    /**
     * @fn FrameMove()
     *
     * @brief This method is called every time the scene is changed
     *
     * @param fElapsedTime - The elapsed time since the application started in seconds
     */
    virtual void    FrameMove( FLOAT fElapsedTime )
    {
        // If the camera reset key is pressed, set the ResetFlag to TRUE
        int ResetFlag = IsKeyDown( m_aKeys[CAM_RESET] );
        // Call the base class to handle common tasks
        __super::FrameMove( fElapsedTime );

        // If reset was pressed
        if(ResetFlag)
        {   // restore all parameters to their default values
            BYTE    OldState = m_aKeys[CAM_RESET];
            m_ViewArcBall.SetQuatNow(m_QuatDefault);
            m_fRadius = m_fRadiusDefault;
            m_aKeys[CAM_RESET] = 0;
            // Recalculate the view matrix for default parameters
            __super::FrameMove(0);
            m_aKeys[CAM_RESET] = OldState;
        }
    };

    /**
     * @fn SetCurrentParamsDefault()
     *
     * @brief Stores the current camera position parameters so that the position can be restored in the future 
     */
    void SetCurrentParamsDefault()
    {
        m_QuatDefault = m_ViewArcBall.GetQuatNow();
        m_fRadiusDefault = m_fRadius;
    };

protected:
    float           m_fRadiusDefault;       // The default distance from the camera to the central point
    D3DXQUATERNION  m_QuatDefault;          // The default quanternion that describe the camera's orientation
};

/**
 * @class 
 *
 * @brief A slightly modified DXUT CFirstPersonCamera camera,
 *        modified to be able to restore the position by pressing the Home key 
 */
class CFirstPersonCamera_DefPos: 
    public CFirstPersonCamera
{
public:

    /**
     * @fn FrameMove()
     *
     * @brief This method is called every time the scene is changed
     *
     * @param fElapsedTime - The elapsed time since the application started in seconds
     */
    virtual void    FrameMove( FLOAT fElapsedTime )
    {
        // If the camera reset key is pressed, set the ResetFlag to TRUE
        int ResetFlag = IsKeyDown( m_aKeys[CAM_RESET] );
        // Call the base class to handle common tasks
        __super::FrameMove( fElapsedTime );

        // If reset was pressed
        if(ResetFlag)
        { // Restore parameters to their default values
            BYTE    OldState = m_aKeys[CAM_RESET];
            m_fCameraPitchAngle = m_fCameraPitchAngleDefault;
            m_fCameraYawAngle = m_fCameraYawAngleDefault;
            m_vEye = m_vEyeDefault;
            m_aKeys[CAM_RESET] = 0;
            // re-calculate the view matrix for the default parameters
            __super::FrameMove(0);
            m_aKeys[CAM_RESET] = OldState;
        }
    };

    /**
     * @fn SetCurrentParamsDefault()
     *
     * @brief This method stores the current camera position parameters so that the position can be restored it in the future 
     */
    void SetCurrentParamsDefault()
    {
        m_fCameraPitchAngleDefault = m_fCameraPitchAngle;
        m_fCameraYawAngleDefault = m_fCameraYawAngle;
        m_vEyeDefault = m_vEye;
    };

protected:
    float           m_fCameraPitchAngleDefault;     // The default pitch of the camera
    float           m_fCameraYawAngleDefault;       // The default Yaw of the camera orientation
    D3DXVECTOR3     m_vEyeDefault;                  // The default camera position
};

/**
 * @struct DefSliderData
 *
 * @brief This structure is used to store information about each slider on the screen
 *        and is used to simplify adding sliders.
 */
typedef struct  
{
    WCHAR*  pStrToPrint;    // A string that is used to print the title of the slider
    int     id;             // The ID of the slider control
    float   MinVal;         // A minimal value for the control
    float   MaxVal;         // A maximum value for the control
    float*  pVal;           // A pointer to a float that will be used to store value
} DefSliderData;

/**
 * @class CFluidSimHost
 *
 * @brief The main class for the application, inherited from CSampleBase
 */
class CFluidSimHost : 
    public CSampleBase
{
public:

    /**
     * @fn CFluidSimHost()
     *
     * @brief Class constructor. Creates a baseDX class using command line information
     *
     * @param in_cmdLineArg - The command line string
     */
    CFluidSimHost(LPWSTR in_cmdLineArg);

    /**
     * @fn ~CFluidSimHost()
     *
     * @brief Class Destructor. Releases resources
     */
    ~CFluidSimHost();

    DWORD WINAPI        ProcessThread();

protected:

    DynamicArray<DefSliderData> m_Sliders;                          // An array of sliders
    
    CBaseCamera*        m_pCamera;                                  // A pointer to the currently used camera
    CModelViewerCamera_DefPos  m_MVCamera;                          // A model viewing camera
    CFirstPersonCamera_DefPos  m_FPCamera;                          // The first person camera
    D3DXMATRIXA16       m_mCenterMesh;                              // A matrix to move the scene center to the 0,0 coordinate.
    bool                m_bShowHelp;                                // If TRUE, render the UI control text (initialy set to TRUE)
    bool                m_bShowControls;                            // If TRUE, render the right GUI (initialy set to TRUE)
    bool                m_bPause;                                   // Pause flag, initialy FALSE
    bool                m_bOneStep;                                 // Single step flag, initialy FALSE
    bool                m_bRealTime;                                // Real time flag, initialy FALSE
    bool                m_bFollowGround;                            // Follow the ground flag, initialy FALSE
    bool                m_bDisableGathered;                         // Disables gathered visualization, initialy FALSE
    bool                m_bAsync;                                   // Enables asynchronius processing, initialy FALSE
    bool                m_bOCLRelaxedMath;                          // Enables OpenCL relaxed math, initialy FALSE
    float               m_fLevelOfDetails;                          // The level of detail, initialy set to 1.0f

    WATER_STRUCT        m_WaterSimulationParams;                    // The water simulation parameters to send to the OCL side
    float               m_maxDepth;                                 // The maxmimum depthm, initialy set to 70.0f
    float               m_localNormalScale;                         // The normal local scale, initialy set to 2.0f
    float               m_resetLatency;                             // The reset latency, initialy set to 5.0f

    CFlux2DScene*          m_pScene;                                // A pointer to a copy of the scene (that is a set of grids) received from OCL 
    CSceneVisual*          m_pSceneVis;
    CSky*                  m_pSky;
    CVisual3DCommonData*   m_pCommonData;
    
    // Collada stuff 
    collada::AutoPtr<collada::EditableScene>    m_pSceneCollada;
    collada::AutoPtr<collada::EditableScene>    m_pSceneColladaInvisible;
    collada::AutoPtr<collada::EditableScene>    m_pSceneColladaStatic;
    // Boat stuff
    void*               m_pCollisionIndices;                        // Indices for boat triangles in the boat geometry vertices array
    void*               m_pIndices;                                 // Indices of boat model triangles in the boat vertices array
    unsigned int        m_collisionIndicesCount;                    // The number of collision geometry indices
    unsigned int        m_indicesCount;                             // The number of boat model indices
    float3*             m_pVertices;                                // The boat model vertices
    float3*             m_pCollisionVertices;                       // The collision geometry vertices
    float*              m_verticesTransformed;                      // The boat model vertices after transformations according to the matrices in the model
    unsigned int        m_verticesCount;                            // The number of vertices in the boat model
    unsigned int        m_collisionVerticesCount;                   // The number of vertices in collision geometry
    float               m_pTransformMatrix[16];                     // An array for sending the boat transformation matrix to OCL
    D3DXMATRIX          m_boatTransform;                            // A matrix for rendering THE boat according to the transformations from OCL
    collada::MeshInstance*       m_pBoatMeshInstanceForAnimation;   // The boat mesh instance used for animation

    float               m_time;                                     // The time in the animation
    bool                m_isAnimationFreezed;                       // A flag for animation status. the animation will freeze when the user clicks 
                                                                    // the right mouse button to move the boat

    collada::AutoPtr<collada::AccessProviderD3D10> m_providerD3D10;
    collada::AutoPtr<collada::AccessProviderLocal> m_providerLocal;

    collada::AutoPtr<CSceneRenderDX10>    m_ColladaRenderStatic;                      // Initialy (m_env.GetPrinter())
    collada::AutoPtr<CSceneRenderDX10>    m_ColladaRender;                            // Initialy (m_env.GetPrinter())

    void*               m_pTransformBuffer;                         // A buffer to store boat data on the OCL
    FILE*               m_pLogfile;                                 // A pointer to the log file in which to save diagnostics messages

    float               m_currentFPS;
    int                 m_FPScounter;

    
    //Assync process thread handles and variables 
    HANDLE              m_hThread;
    HANDLE              m_hEvent;
    ///unsigned int        m_threadID;
    DWORD               m_threadID;
    CRITICAL_SECTION    m_cs;
    ///unsigned __stdcall  ProcessThread(void *ArgList); 


    //---------------------------------------------------------------------------------------------------------------------------------------------

    /**
     * @enum UI_IDs
     *
     * @brief UI control IDs for all checkbox and combo controls
     */
    enum
    {
        IDC_STATIC = 100,
        IDC_VISMODE,            // The id for visualisation mode
        IDC_CAMMODE,            // The id to choose the camera type
        IDC_WIRE,               // The id to choose whether wireframe will or will not be used for visualisation
        IDC_WIND_WAVE,          // The id to switch on wind waves
        IDC_REALTIME,           // The id to switch on real-time mode
        IDC_OPENCL,             // The id to switch between native and OpenCL solver 
        IDC_SCENE,              // The id to switch scene 
        IDC_ASYNC,              // The id to switch sync/async processing
        IDC_RELAXED,            // The id to switch relaxed math OCL processing
        IDC_CPUGPU,             // The id to switch CPU/GPU OCL processing
        IDC_LAST                // The last ide to use by slider add engines
    };
	void LoadColladaScenes();
    /**
     * @fn SetScene()
     *
     * @brief Release old scene, create new by ID
     *
     * @param iSceneID - ID of scene
     */
    void SetScene(int iSceneID);

    /**
     * @fn ConvertScene()
     *
     * @brief Some scene conversion to supported format
     *
     * @param pEnv - The working environment, ignored
     *
     * @param source - The scene loaded from a file
     *
     * @param ppREsult - Output, a pointer to the converted scene
     */
    void PrepareForRender(AccessProviderD3D10 *providerD3D10, AccessProviderLocal *providerLocal, Scene *source, Scene **ppResult);

    /**
     * @fn GetNewTransformationMatrix()
     *
     * @brief This method gets a transformation matrix for the boat during the animation state from a collada file
     *
     * @param in_pScene - The collada scene with animation
     *
     * @param time - The time given to generate the animation matrix
     */
    const float* GetNewTransformationMatrix(collada::Scene* in_pScene, float time);

    /**
     * @fn IntersectWithSurface()
     *
     * @brief This method is used to find the intersection of a 3D ray with the water surface
     *
     * @param  rRayPos_ - the starting position of the ray
     *
     * @param rRayDir_ - The direction of the ray
     *
     * @param rX - Output, the X coordinate of the found intersection point on the scene
     *
     * @param rY - Output, the Y coordinate of the found intersection point on the scene
     *
     * @return - TRUE if an intersection was found, FALSE otherwise
     */
    bool IntersectWithSurface(D3DXVECTOR3 const &rRayPos_, D3DXVECTOR3 const &rRayDir, float &rX, float &rY);

    /**
     * @fn SerializeBoatModel()
     *
     * @brief This method is used to serialize the boat model
     *
     * @param in_verticesCount - The number of vertices in the boat model
     *
     * @param in_pVertices - A pointer to the vertices buffer
     *
     * @param in_indicesType - The type of indexes
     *
     * @param in_indicesCount - The number of boat indexes
     *
     * @param in_pIndices - A pointer to the indexes
     *
     * @param out_data2Send - Output, An array in which to store the packed data
     */
    void SerializeBoatModel(
        const unsigned int in_verticesCount, 
        const float3* in_pVertices, 
        const unsigned int in_indicesCount, 
        const void* in_pIndices, 
        DynamicArray<char>* out_data2Send);

    /**
     * @fn UpdateViewAccording2Mesh()
     *
     * @brief This method updates the center & camera parameters according to the mesh 
     */
    void UpdateViewAccording2Mesh();
    
    /**
     * @fn GetHybridMethodData()
     *
     * @brief This method gets the hybrid method as boat data from the OCL side. It gets
     *        the boat position, and vertice parameters. This
     *        function also reallocates buffers for vertices if needed
     */
    void GetHybridMethodData();

    /**
     * @fn AddSlider()
     *
     * @brief This method is used to add a slider to the interface screen 
     *
     * @param yUIOffset - The Y coordinate of the slider (The X coordinate is fixed)
     *
     * @param pVal - A pointer to the floating point variable that will be changed due to slider movement
     *
     * @param MinVal - The value for the slider in the far-left position.
     *
     * @param MaxVal - The value for the slider in far-right position
     *
     * @param pStrToPrint - The slider title
     *
     * @param bEnabled - The slider is either enabled or disabled
     */
    void AddSlider( int& yUIOffset, 
        float* pVal, 
        float MinVal, 
        float MaxVal, 
        WCHAR* pStrToPrint, 
        bool bEnabled=true );
    
    /**
     * @fn UpdateSlider()
     *
     * @brief This method updates the displayed value for a given slider.
     *
     * @note This function should be called from OnGUIEvent()
     *
     * @param pUI - Pointer to the user interface dialog box passed from OnGUIEvent
     *
     * @param IdForUpdate - The ID of the slider to update
     */
    void UpdateSlider(CDXUTDialog *pUI, int IdForUpdate);


    /**
     * @fn OnCreateDevice()
     *
     * @brief This routine is called after the Direct3D10 device has been created.
     *
     * @note Device creation happens during application initialization or if the device is changed.
     *
     * @param pd3dDevice - Pointer to the newly created ID3D10Device Interface device.
     *
     * @param in_pBufferSurfaceDesc -  Pointer to the backbuffer surface description.
     *
     * @return - Windows result code
     */
    virtual HRESULT OnCreateDevice(
        ID3D10Device* pd3dDevice, 
        const DXGI_SURFACE_DESC* in_pBufferSurfaceDesc);

    /**
     * @fn OnDestroyDevice()
     *
     * @brief This routine is called when the device is about to be destroyed.
     *
     * @note The application should release all device-dependent resources in this function.
     */
    virtual void OnDestroyDevice();

    /**
     * @fn KeyboardProc()
     *
     * @brief The framework consolidates the keyboard messages and passes them to the application via this function.
     *
     * @param nChar - The virtual-key code for the key
     *
     * @param bKeyDown - TRUE if key is down and FALSE if the key is up
     *
     * @param bAltDown - TRUE if the ALT key is down, FALSE if it is up.
     */
    virtual void KeyboardProc(
        UINT nChar,
        bool bKeyDown,
        bool bAltDown);

    /**
     * @fn OnInitApp()
     *
     * @brief This routine is called only once after framework initialization and before the D3D device is created.
     *
     * @param io_StartupInfo - Pointer to the structure that contains the startup settings 
     */
    virtual void OnInitApp(
        StartupInfo *io_StartupInfo );

    /**
     * @fn MouseProc()
     *
     * @brief A mouse event handling function, called by the framework when it receives mouse events.
     *
     * @note This event handling mechanism is provided to simplify handling mouse
     *       messages through the window's message pump, but the application may
     *       still handle mouse messages directly through the MsgProc function.
     *
     * @param bLeftButtonDown - TRUE if the left mouse button is down.
     *
     * @param bRightButtonDown - TRUE if the right mouse button is down.
     *
     * @param bMiddleButtonDown - TRUE if the middle mouse button is down.
     *
     * @param bSideButton1Down = TRUE if the first X button is down (Microsoft Windows 2000/Windows XP)
     *
     * @param bSideButton2Down - TRUE if the second X button is down (Windows 2000/Windows XP)
     *
     * #param nMouseWheelDelta - The distance and direction the mouse wheel has rolled, expressed in multiples
     *                           or divisions of WHEEL_DELTA, which is 120. A positive value indicates that the
     *                           wheel was rotated forward, away from the user; a negative value indicates that
     *                           the wheel was rotated backward, toward the user.
     *
     * @param xPos - The x-coordinate of the mouse pointer, relative to the upper-left corner of the client area.
     *
     * @param yPos - The y-coordinate of the mouse pointer, relative to the upper-left corner of the client area.
     */
    virtual void MouseProc(
        bool bLeftButtonDown,
        bool bRightButtonDown,
        bool bMiddleButtonDown,
        bool bSideButton1Down,
        bool bSideButton2Down,
        int nMouseWheelDelta,
        int xPos,
        int yPos);
    
    /**
     * @fn OnGUIEvent()
     *
     * @brief This function is associated with the actual events processed by both, the UI and the HUD.
     *
     * @note This function is used to handle the interaction between the controls.
     *
     * @param nEvent - Event identifier
     *
     * @param nControlID - IDC control identifier
     *
     * @param pControl - Pointer to the control rising the event 
     */
    virtual void OnGUIEvent(UINT nEvent,
        int nControlID,
        CDXUTControl* pControl);

    /**
     * @fn OnRenderScene()
     *
     * @brief Called when the framework needs to render the entire scene.
     *
     * @param pd3dDevice - Pointer to the ID3D10Device Interface device used for rendering.
     *
     * @param fTime - The time elapsed since the application started, in seconds.
     *
     * @param fElapsedTime - The time elapsed since the last frame, in seconds.
     */
    virtual void OnRenderScene(
        ID3D10Device* pd3dDevice,
        double fTime,
        float fElapsedTime);

    /**
     * @fn ModifyDeviceSettings()
     *
     * @brief This routine is called by the framework to allow changes in the device settings before the device is created.
     *
     * @note Before a device is created by the framework, this function will be called to
     *       allow the application to examine or change the device settings before the device is
     *       created. This allows an application to modify the device creation settings as it sees fit.
     *
     * @note This function also allows applications to reject changing the device altogether.
     *       Returning FALSE from inside this function will notify the framework to
     *       keep using the current device instead of changing to the new device.
     *
     * @note Anything in pDeviceSettings can be changed by the application.
     *
     * @param pDeviceSettings - Pointer to a structure that contains the settings for the new device.
     *
     * @return - The application should return TRUE to continue creating the device.
     *           If the application returns FALSE, the framework will continue using
     *           the current device if one exists.
     */
    virtual bool ModifyDeviceSettings(
        DXUTDeviceSettings* pDeviceSettings);

    /**
     * @fn OnResizedSwapChain()
     *
     * @brief This method is called when changing the window size or when a new device has been created and OnCreateDevice was called.
     *
     * @param pd3dDevice - Pointer to the ID3D10Device Interface device used for rendering.
     *
     * @param pSwapChain - Pointer to the swap chain
     *
     * @param in_pBufferSurfaceDesc - Pointer to the backbuffer surface description.
     *
     * @return - Windows result code
     */
    virtual HRESULT OnResizedSwapChain(
        ID3D10Device* pd3dDevice,
        IDXGISwapChain *pSwapChain,
        const DXGI_SURFACE_DESC* in_pBufferSurfaceDesc );

    /**
     * @fn OnRenderText()
     *
     * @brief This routine is called when the framework needs to render UI text.
     *
     * @note This routine allows the application to draw lines of text anywhere on the screen
     *
     * @param  pTextHelper - Helper structure used in text rendering
     *
     * @param fTime - The time elapsed since the application started, in seconds.
     *
     * @param fElapsedTime - The time elapsed since the last frame, in seconds.
     */
    virtual void OnRenderText(
        CDXUTTextHelper* pTextHelper,
        double fTime,
        float fElapsedTime);

    /**
     * @fn OnFrameMove()
     *
     * @brief A method called by the framework once each frame, before the application renders the scene.
     *
     * @param fTime - The time elapsed since the application started, in seconds.
     *
     * @param fElapsedTime - The time elapsed since the last frame, in seconds.
     */
    virtual void OnFrameMove(
        double fTime,
        float fElapsedTime);

    /**
     * @fn MsgProc()
     *
     * @brief Windows messages processing routine.
     *
     * @note This method is called every time the application receives a message. The framework
     *       passes all messages to the application and does not process any further
     *       messages if the application says not to.
     *
     * @param hWnd - Handle to the application window
     *
     * @param uMsg - Windows message code
     *
     * @param wParam - Windows wParam
     *
     * @param lParam - Windows lParam
     *
     * @param pbNoFurtherProcessing - Boolean indicating whether or not to perform further processing
     *
     * @return - Windows result code
     */
    virtual LRESULT MsgProc(
        HWND   hWnd,
        UINT   uMsg,
        WPARAM wParam,
        LPARAM lParam,
        bool* pbNoFurtherProcessing);


}; // END OF FILE
