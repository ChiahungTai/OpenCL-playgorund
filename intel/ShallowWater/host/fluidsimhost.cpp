/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file fluidsimhost.cpp
 *
 * @brief Host side class objects for the fluid simulator
 */


#include "initparams.h"
#include "fluidsimhost.h"

#define SAMPLES_TOOLS_INSTANCE

// Include to print safe strings
#include <strsafe.h>

// Collada include files for scene visualization
#include "SkinningPlugin.h"
#include "SkinningAlgorithm.h"

#include "TriangulatorPlugin.h"
#include "TriangulatorAlgorithm.h"

#include "IndexCombinerPlugin.h"
#include "IndexCombinerAlgorithm.h"
#include "collada/SceneReaderDae.h"

#include "SceneUtils.h"

//OpenCL
#include "utils.h"

#include <Windows.h>

#include "AccessProviderDefault.h"
#include "d3d10/AccessProviderD3D10.h"

/**
 * @def BUFFERSNUM
 *
 * @brief The maximum number of buffer that can be transfered between OCL and the HOST at once
 */
#define BUFFERSNUM 6

// Move Collada scene values
// Offsets tuned for Collada scene and water surface simulation scene merging
#define COLLADA_SCENE_OFFSET_X -127.2f
#define COLLADA_SCENE_OFFSET_Y -0.2f
///#define COLLADA_SCENE_OFFSET_Y -5.2f
#define COLLADA_SCENE_OFFSET_Z 127.0f
///#define COLLADA_SCENE_OFFSET_Z 100.0f


static float g_dx=COLLADA_SCENE_OFFSET_X;
static float g_dy=COLLADA_SCENE_OFFSET_Y;
static float g_dz=COLLADA_SCENE_OFFSET_Z;

// Static variables for wave generation on scene border.
float g_Noise = INITIAL_NOISE;
float g_NoiseTimeCoherence = TIME_COHERENSE_NOISE;
float g_NoiseSpaceCoherence = SPACE_COHERENSE_NOISE;
float g_omega = INITIAL_OMEGA;
float g_ampl = INITIAL_AMPL;

char  g_device_name[128]={0};

/**
 * @fn CreateSceneByName()
 *
 * @brief Creates scene by its name, reads the scene from a media file
 *
 * @param name - name of the scene
 *
 * @return - A scene pointer to the scene that was created
 *           NULL if an error occurred
 */
static CFlux2DScene* CreateSceneByName(wchar_t const *name)
{
    CFlux2DScene* pScene = NULL;

    // Create scene
    // Set special dissipation and time step for some scenes (manually adjusted for best result)
    if(_wcsicmp(name, L"Lighthouse")==0)
    {
        pScene = SceneCreateBasicParametrized(
            CSceneDef( L"data/landHeight.bmp", -15.0f, 20.0f, 256.0f, 256.0f ), 
            256, 256,
            1.5f, 0.7f, 0.0f);
        //pScene->m_recommendedTimeStep /=4.0f; 
    }
    else if(_wcsicmp(name, L"Lake")==0) 
    {
        pScene = SceneCreateWaterRisingParametrized(
            CSceneDef( L"data/hm_lake.bmp", -5.0f, 5.0f, 256.0f, 256.0f ), 
            256, 256,
            1.5f, 3.9f, 20.0f);
        if(pScene)pScene->SetCf(0.13f);                 // set special dissipation
    }
    else if(_wcsicmp(name, L"Lighthouse_Large")==0)
    {
        pScene = SceneCreateBasicParametrized(
            CSceneDef( L"data/landHeight.bmp", -15.0f, 20.0f, 256.0f, 256.0f ), 
            512, 512,
            1.5f, 0.7f, 0.0f);
        pScene->m_recommendedTimeStep /=2.0f; 
    }
    else if(_wcsicmp(name, L"Lake_Large")==0) 
    {
        pScene = SceneCreateWaterRisingParametrized(
            CSceneDef( L"data/hm_lake.bmp", -5.0f, 5.0f, 256.0f, 256.0f ), 
            512, 512,
            1.5f, 3.9f, 20.0f);
        pScene->m_recommendedTimeStep /=4.0f; 
        if(pScene)pScene->SetCf(0.03f);                 // set special dissipation
    }
    else if(_wcsicmp(name, L"Lighthouse_XL")==0)
    {
        pScene = SceneCreateBasicParametrized(
            CSceneDef( L"data/landHeight.bmp", -15.0f, 20.0f, 256.0f, 256.0f ), 
            1024, 1024,
            1.5f, 0.7f, 0.0f);
        pScene->m_recommendedTimeStep /=4.0f; 
    }
    else if(_wcsicmp(name, L"Lake_XL")==0) 
    {
        pScene = SceneCreateWaterRisingParametrized(
            CSceneDef( L"data/hm_lake.bmp", -5.0f, 5.0f, 256.0f, 256.0f ), 
            1024, 1024,
            1.5f, 3.9f, 20.0f);
        pScene->m_recommendedTimeStep /=8.0f; 
        if(pScene)pScene->SetCf(0.03f);                 // set special dissipation
    }
    return pScene;
} //CreateSceneByName


/// release old scene, create new by ID
/// @param iSceneID - ID of new scene
void CFluidSimHost::SetScene(int iSceneID)
{
    if(m_pScene)
    {
        DestroyObject(m_pScene);
        m_pScene = 0;
        ///m_pScene->~CFlux2DScene();
    }
    switch ( iSceneID )
    {
    case 0: m_pScene = CreateSceneByName(SCENE_NAME); break; //sea
    case 1: m_pScene = CreateSceneByName(SCENE_NAME_2); break; //river
    case 2: m_pScene = CreateSceneByName(SCENE_NAME_4); break; //sea large 
    case 3: m_pScene = CreateSceneByName(SCENE_NAME_5); break; //river large
    case 4: m_pScene = CreateSceneByName(SCENE_NAME_6); break; //sea XL 
    case 5: m_pScene = CreateSceneByName(SCENE_NAME_7); break; //river XL
    }

    if(m_pScene == NULL)
    {   // Loading failed
        
        WRITE_LOGFILE2("Could not load scene %d\n",iSceneID);
        CLOSE_LOGFILE();
        return;
    }

    // Set time step
    m_WaterSimulationParams.time_step = m_pScene->GetRecommendedTimeStep();

    
    if(m_pCommonData)
    {// update visuaizer for new scene 
        SAFE_DELETE(m_pSceneVis);
        // Create scene visual
        m_pSceneVis = new CSceneVisual(*m_pScene, *m_pCommonData);

        // Update first time
        m_pSceneVis->Update(0.0f);

        if (m_pSceneVis->GatheredVisual() != NULL)
        {
            m_maxDepth         = m_pSceneVis->GatheredVisual()->GetMaxDepth();
            m_localNormalScale = m_pSceneVis->GatheredVisual()->GetLocalNormalScale();
            m_resetLatency     = m_pSceneVis->GatheredVisual()->GetLatency();
        }
    }
};


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
void CFluidSimHost::PrepareForRender(AccessProviderD3D10 *providerD3D10, AccessProviderLocal *providerLocal, Scene *source, Scene **ppResult)
{
    // Use pointers that will automatically destruct. "tmp" will be used
    // for transitory data and is initialized to "source". The results of
    // the return from all operations will be stored in "result", 
    // then assigned to "tmp" and then "result" will be re-used

    AutoPtr<Scene> tmp = source;
    AutoPtr<Scene> result;


	// convert skinned meshes to look like a regular static mesh
    AutoPtr<ISkinningAlgorithm> skinningAlgorithm;
    CreateSkinningAlgorithmDefault(providerLocal, &skinningAlgorithm);
    CreateSkinningSceneAdapter(tmp, &result, skinningAlgorithm, providerLocal);
    tmp = result;
    SafeReleasePtr(result); // Assign NULL to the smart pointer and decrease the reference count


    // Triangulate all non-triangulated data
    AutoPtr<ITriangulatorAlgorithm> triangulatorAlgorithm;
    CreateTriangulatorAlgorithmDefault(providerLocal, &triangulatorAlgorithm);
    CreateTriangulatedSceneAdapter(tmp, &result, triangulatorAlgorithm, providerLocal);
    tmp = result;
    SafeReleasePtr(result);


    // Transform index buffers and prepare for rendering
    AutoPtr<IIndexCombinerAlgorithm> indexCombinerAlgorithm;
    CreateIndexCombinerAlgorithmDefault(providerLocal, &indexCombinerAlgorithm);
    CreateUnifiedSceneAdapter(tmp, &result, indexCombinerAlgorithm, providerLocal);
    tmp = result;
    SafeReleasePtr(result);

	
    // Assign NULL to the smart pointer (tmp) and return it's raw pointer
    // Because the function loses scope, the AutoPtr "tmp" will be deallocated
    DetachRawPtr(tmp, ppResult);
}

/**
 * @fn CFluidSimHost()
 *
 * @brief Class constructor. Creates a baseDX class using command line information
 *
 * @param in_cmdLineArg - The command line string
 */
CFluidSimHost::CFluidSimHost(LPWSTR in_cmdLineArg)
    : m_bShowHelp(true),
    m_bShowControls(true),
    m_bPause(false),
    m_bOneStep(false),
    m_bRealTime(false),
    m_bAsync(false),
    m_bOCLRelaxedMath(false),
    m_bFollowGround(false),
    m_bDisableGathered(false),
    m_fLevelOfDetails(1.0f),
    m_maxDepth(70.0f),
    m_localNormalScale(1.0f),
    m_resetLatency(5.0f)
{
    AutoPtr<IMessageListener> msgListener;
    CreateMessageListenerStd(&msgListener);

    CreateAccessProviderD3D10(&m_providerD3D10);
    CreateAccessProviderLocal(&m_providerLocal);
    m_ColladaRender = new CSceneRenderDX10(msgListener, m_providerD3D10);
    m_ColladaRenderStatic = new CSceneRenderDX10(msgListener, m_providerD3D10);

    // Initialize parameters
    // Start values for wind waves generator (manually adjusted for best result)
    m_WaterSimulationParams.is_wind_waves = false;
    m_WaterSimulationParams.ww.number = 50.0f;
    m_WaterSimulationParams.ww.speed = 3.0f;
    m_WaterSimulationParams.ww.amplitude = 0.3f;
    m_WaterSimulationParams.ww.force = 0.2f;
    m_WaterSimulationParams.ww.desipation = 0.01f;
    m_WaterSimulationParams.ww.directed_waves = 0;
    m_WaterSimulationParams.ww.direction = 0.0f;

    m_WaterSimulationParams.enableBoat = true;   // enable boat emulation by default

    m_WaterSimulationParams.wasClicked = false;
    m_WaterSimulationParams.x = 0;
    m_WaterSimulationParams.y = 0;
    m_pTransformBuffer = 0;

    m_WaterSimulationParams.solverNumber = 0;

    m_WaterSimulationParams.sceneNumber = 0;//0;
	m_WaterSimulationParams.enableGPUProcessing = false;

    // Set the current camera
    m_pCamera = &m_MVCamera;

    //m_pLogfile = fopen("logfile.log", "w");  DEPRECATED!!!

    m_pLogfile = NULL; // do not open log file and do not write anything to it!!

    //// Still, if we have a command line, there might be a (file) name of a scene to load
    //if (in_cmdLineArg != NULL && in_cmdLineArg[0] != 0 && wcschr(in_cmdLineArg, L'-') != in_cmdLineArg)
    //{
    //    // Load the scene from the command line
    //    m_pScene = CreateSceneByName(in_cmdLineArg);
    //}
    //else
    {   
        // Load the default scene
        m_pScene = CreateSceneByName(SCENE_NAME);
    }

    if (m_pScene == NULL)
    {   
        // Loading failed
        WRITE_LOGFILE("Scene with grids is not loaded")
    }

    m_currentFPS = 0.0f;
    m_FPScounter = 0;


    m_hThread = 0;
    m_hEvent = 0;

    m_pVertices = 0;                                // The boat model vertices
    m_pCollisionVertices = 0;                       // The collision geometry vertices
    m_verticesTransformed = 0;                      // The boat model vertices after transformations according to the matrices in the model
    m_pCollisionIndices = 0;                        // Indices for boat triangles in the boat geometry vertices array
    m_pIndices = 0;                                 // Indices of boat model triangles in the boat vertices array



}// END of CFluidSimHost::CFluidSimHost

/**
 * @fn ~CFluidSimHost()
 *
 * @brief Class Destructor. Releases resources
 */
CFluidSimHost::~CFluidSimHost()
{
    CLOSE_LOGFILE();
}


int SimulationStep(void* in_pData);
void RebuildOCLKernel(bool in_bRelaxedMath, bool in_bRunOnGPU);



///unsigned __stdcall CFluidSimHost::ProcessThread(void *ArgList) 
DWORD WINAPI  ProcessThreadWraper(LPVOID ArgList)
{
    ((CFluidSimHost*)(ArgList))->ProcessThread();
    return 0;
}


DWORD WINAPI  CFluidSimHost::ProcessThread()
{

    int i;
    while (WaitForSingleObject(m_hEvent, 0) != WAIT_OBJECT_0) 
    {
        ///printf("Waiting for signal\n");
        // For each grid in the scene
        for(i=0;i<m_pScene->GetNumGrids();++i)
        {
            // Set the grid level of dtail and the height and width of the water simulations
            m_pScene->GetGrid(i)->SetLOD(m_fLevelOfDetails);
            m_WaterSimulationParams.DomainW[i] = m_pScene->GetGrid(i)->GetDomainW();
            m_WaterSimulationParams.DomainH[i] = m_pScene->GetGrid(i)->GetDomainH();
        }

        // Set some parameters for the simulation, use global settings
        m_WaterSimulationParams.Noise = g_Noise;
        m_WaterSimulationParams.NoiseSpaceCoherence = g_NoiseSpaceCoherence;
        m_WaterSimulationParams.NoiseTimeCoherence = g_NoiseTimeCoherence;
        m_WaterSimulationParams.omega = g_omega;
        m_WaterSimulationParams.ampl = g_ampl;
        m_WaterSimulationParams.time_step = m_pScene->GetRecommendedTimeStep();


        // If we are not paused, then update the scene
        if (!m_bPause)
        {
            // If not in real-time then.....
            // Real time branch is currently disabled
            // Do as much simulation steps as possible
            if (!m_bRealTime)
            {
                EnterCriticalSection(&m_cs);
                // If the boat is enabled
                if(m_WaterSimulationParams.enableBoat)
                {
                    // Get the time if the animation is not frozen
                    m_time = m_isAnimationFreezed ? 0 : (m_time + ANIMATION_TIME_STEP);
                    // If we have the invisible scene
                    if(m_pSceneColladaInvisible)
                    {
                        // Get the transformation matrix based on the time
                        const float* pT = GetNewTransformationMatrix(m_pSceneColladaInvisible, m_time);
                        // and then set the boat transformation matrix
                        if(pT)memcpy(m_WaterSimulationParams.boatTransform, pT, sizeof(float)*16);
                    }
                }

                LARGE_INTEGER Frequency;
                LARGE_INTEGER PerformanceCount1;
                LARGE_INTEGER PerformanceCount2;
                Frequency.QuadPart = 0;
                PerformanceCount1.QuadPart = 0;
                PerformanceCount2.QuadPart = 0;

                if(m_FPScounter == 30)
                {
                    QueryPerformanceFrequency(&Frequency);
                    QueryPerformanceCounter(&PerformanceCount1);
                }

                // Make the calculations for the shallow water algorithm step
                SimulationStep((void*)&m_WaterSimulationParams);

                if(m_FPScounter == 30)
                {
                    QueryPerformanceCounter(&PerformanceCount2);
                    m_currentFPS = (float)Frequency.QuadPart/(float)(PerformanceCount2.QuadPart-PerformanceCount1.QuadPart);
                    m_FPScounter = 0;
                }
                else
                {
                    m_FPScounter++;
                }

                // Get the boat data from OCL 
                GetHybridMethodData();


                // Disable gathered visualization
                //{
                //    if(!m_bDisableGathered)
                //    {
                //        m_pSceneVis->SetMode(1);
                //        m_bDisableGathered = true;
                //    }
                //}

                LeaveCriticalSection(&m_cs);


                //Debug code
#ifdef STOP_SIMULATION_FOR_BAD_HEIGHT

                // This code is kept for debug purposes
                {   
                    // Check the grid for bad values and if found, stop the simulation
                    int i;
                    for(i=0;i<m_pScene->GetNumGrids();++i)
                    {
                        CFlux2DGrid* pG = m_pScene->GetGrid();
                        int w= pG->GetDomainW();
                        int h= pG->GetDomainH();
                        int x,y;
                        // For every line in the image
                        for(y=0;y<h;++y)
                        {
                            // For every pixel in the line
                            for(x=0;x<h;++x)
                            {
                                float val = pG->GetCurrentSurface().m_heightMap.At(x,y);
                                if(fabs(val)>20.0f)
                                {
                                    printf("\nx=%d,y=%d pixel = %f height, simulation paused\n",x,y,val);
                                    m_bPause = 1;
                                }
                            }
                        }

                    }
                }
#endif
            }

            // If we are processing one step at at time, pause the display
            if(m_bOneStep)
            {
                m_bPause = 1;
                m_bOneStep = 0;
            }
        }
    }
    printf("Thread ending\n");
    return 0;
}




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

int LoadScene(void* in_pData0, uint32_t size0, void* in_pData, uint32_t size,  float x_boat_pos, float y_boat_pos, bool device_type_gpu);

HRESULT CFluidSimHost::OnCreateDevice(
    ID3D10Device* pd3dDevice,
    const DXGI_SURFACE_DESC* in_pBufferSurfaceDesc )
{
    HRESULT hr;
    printf("OnCreateDevice call\n");
    if(m_pScene==NULL)
    {
        WRITE_LOGFILE( "HOST: There is no scene...\n" );
        return E_FAIL;
    }

    V_RETURN(__super::OnCreateDevice(pd3dDevice, in_pBufferSurfaceDesc));


    // Get a pointer to the user interface
    CDXUTDialog* pUI = GetSampleUI();

    UpdateViewAccording2Mesh();

    // Create common data
    m_pCommonData = new CVisual3DCommonData();


    // Create the sky
    m_pSky = new CSky(m_pCommonData->m_pSkyTexture);

    // Create the scene visual
    m_pSceneVis = new CSceneVisual(*m_pScene, *m_pCommonData);

    // Update the first time
    m_pSceneVis->Update(0.0f);

    {
        // Get gathered visualizer engine parameters 
        CVisual3DGathered* pGS = m_pSceneVis->GatheredVisual();
        if(pGS)
        {
            m_maxDepth         = pGS->GetMaxDepth();
            m_localNormalScale = pGS->GetLocalNormalScale();
            m_resetLatency     = pGS->GetLatency();
        }
    }

    // Initialize the combobox used to choose the visualization mode
    CDXUTComboBox* pComboBox = pUI->GetComboBox(IDC_VISMODE);
    if( pComboBox )
    {
        pComboBox->SetDropHeight( 80 );
        // Disable gathered (start from 1)
        for (int i = 0; m_pSceneVis->GetName(i); i++)
        {
            pComboBox->AddItem( m_pSceneVis->GetName(i), (void*)i );
        }
        //pComboBox->SetEnabled(false);
    }


    // Initialize the combobox used to choose the camera type
    pComboBox = pUI->GetComboBox(IDC_CAMMODE);
    if( pComboBox )
    {
        pComboBox->SetDropHeight( 150 );
        pComboBox->AddItem( L"Examine", (void*)0 );
        pComboBox->AddItem( L"WASD", (void*)1 );
        pComboBox->AddItem( L"WASD Follow", (void*)2 );
    }

    // Initialize the combobox used to choose solver type
    pComboBox = pUI->GetComboBox(IDC_OPENCL);
    if( pComboBox )
    {
        pComboBox->SetDropHeight( 100 );
        pComboBox->AddItem( L"OpenCL Scalar", (void*)0 );
        pComboBox->AddItem( L"Native Scalar OpenMP", (void*)1 );
        pComboBox->AddItem( L"Native SSE OpenMP", (void*)2 );
        pComboBox->AddItem( L"Native Scalar Serial", (void*)3 );
        pComboBox->AddItem( L"Native SSE Serial", (void*)4 );
    }

    // Initialize the combobox used to choose scene
    pComboBox = pUI->GetComboBox(IDC_SCENE);
    if( pComboBox )
    {
        pComboBox->SetDropHeight( 100 );
        pComboBox->AddItem( L"Sea", (void*)0 );
        pComboBox->AddItem( L"River", (void*)1 );
        pComboBox->SetSelectedByIndex( 0 );
    }

    // Initialize the combobox used to choose device
    pComboBox = pUI->GetComboBox(IDC_CPUGPU);
    if( pComboBox )
    {
        pComboBox->SetDropHeight( 100 );
        pComboBox->AddItem( L"CPU", (void*)0 );
        pComboBox->AddItem( L"GPU", (void*)1 );
        pComboBox->SetSelectedByIndex( 0 );
    }


	// Create the U, V, and height buffers 
    
    int array_size = 0; // The buffer length
    {
        // Find the maximal grid size
        int i;
        for(i=0;i<m_pScene->GetNumGrids();++i)
        {
            int s = m_pScene->GetGrid(i)->GetBottom().m_heightMap.m_DataSize;
            if(array_size < s || i==0)array_size = s;
        }
    }
    // The array size in bytes
    array_size = array_size*(sizeof(float));


    // Transformation Buffer
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j<4; j++)
        {
            m_pTransformMatrix[i*4+j] = i==j ? 1.0f : 0.0f;
        }
    }


    int size = sizeof(float)*16 + sizeof(bool);
    m_pTransformBuffer = malloc(size);
    memcpy(m_pTransformBuffer, m_pTransformMatrix, size);

    // Attach the device to the renderers
    V_RETURN(m_providerD3D10->GetDeviceManager()->SetDevice(pd3dDevice));

	LoadColladaScenes();


    // BEGIN Call Init function on the OCL side 
    {
        void*               pSceneBuffer;
        void*               pBoatData = NULL;           // A pointer to the memory with boat data
        unsigned int        BoatDataSize = 0;           // The size of the memory with boat data

        // Save the scene to the memory file
        CFileMemory memFile;
        m_pScene->Save(memFile);

        // Create an buffer with the scene
        pSceneBuffer = malloc(memFile.GetSize());
        memcpy(pSceneBuffer, memFile.GetData(), memFile.GetSize());


        // Fill pBoatData with the boat data 
        DynamicArray<char>  BoatData;
        SerializeBoatModel(
            m_collisionVerticesCount, m_pCollisionVertices, 
            m_collisionIndicesCount, m_pCollisionIndices, 
            &BoatData);
        BoatDataSize = BoatData.Size();
        if(BoatDataSize)pBoatData = &(BoatData[0]);

        // Run the LoadScene function on the OCL side
        float x_boat_pos, y_boat_pos;
        x_boat_pos = 50;
        y_boat_pos = 110;
        LoadScene(pSceneBuffer, (uint32_t)(memFile.GetSize()), pBoatData, (uint32_t)BoatDataSize, x_boat_pos, y_boat_pos, false);


        // Free the scene buffer.
        free(pSceneBuffer);
    }
    // END Call Init function on the OCL side 

    // Set time step
    m_WaterSimulationParams.time_step = m_pScene->GetRecommendedTimeStep();


    // Create processing thread for assync mode
 //   m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
 //   m_hThread = CreateThread( NULL, 0, ProcessThreadWraper, this, 0, &m_threadID);
 //   if (m_hThread == 0) 
 //   {
 //       printf( "fail to init async thread\n");
	//}
 //   SuspendThread(m_hThread);
    InitializeCriticalSection(&m_cs); 



    return S_OK;
} // END of CFluidSimHost::OnCreateDevice 

void CFluidSimHost::LoadColladaScenes()
{
    // If we do not have a static collada scene
    if( m_pSceneColladaStatic == NULL )
    {
        try
        {
            HRESULT hr;
            WCHAR str[ 1024 ];


            V( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Lighthouse\\lightHouseNoBeach.dae" ));

            // Initialize render settings for the device
            RenderSettingsDX10 renderSettings;
            m_ColladaRenderStatic->GetRenderSettings(&renderSettings);
            renderSettings.defaultLightColor = D3DXVECTOR4(1,1,1,1);
            renderSettings.cullMode = RenderSettingsDX10::CULL_FORCE_DOUBLE_SIDED;
            renderSettings.mayaAmbientFix = true;
            m_ColladaRenderStatic->SetRenderSettings(&renderSettings);
            // Read the media file
			collada::CreateScene(&m_pSceneColladaStatic);
            collada::ReadSceneDae(str, m_pSceneColladaStatic);

            // If successful and we acquired the static scene
            if(m_pSceneColladaStatic)
            {
                // Convert and render the scene
                collada::AutoPtr<collada::Scene> converted;
                PrepareForRender(m_providerD3D10, m_providerLocal, m_pSceneColladaStatic, &converted);
                m_ColladaRenderStatic->AttachScene( converted );
            }
        }
        catch( const std::exception &e )
        {
            printf( "could not load scene: %s\n", e.what( ));
        }
    }

    // If we do not have the invisible collada scene
    if( m_pSceneColladaInvisible == NULL )
    {
        try
        {
            // Open the media file and acquire the scene
            HRESULT hr;
            WCHAR str[ 1024 ];
            V( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"aniBoat.dae" ));

			collada::CreateScene(&m_pSceneColladaInvisible);
            collada::ReadSceneDae(str, m_pSceneColladaInvisible);

            // If we were successful and acquired the scene
            if(m_pSceneColladaInvisible)
            {   
                // Get the shape for animation checks
                for(uint32_t i = 0; i<m_pSceneColladaInvisible->GetMeshCount(); i++)
                {
                    if(_stricmp(m_pSceneColladaInvisible->GetMeshInstance(i)->GetEntity(m_pSceneCollada)->GetTag(), "boatShape") == 0)
                    {
                        m_pBoatMeshInstanceForAnimation = m_pSceneColladaInvisible->GetMeshInstance(i);
                    }
                }
            }
        }
        catch( const std::exception &e )
        {
            printf( "could not load scene: %s\n", e.what( ));
        }
    }

    // If we don't have the scene collada
    if( m_pSceneCollada == NULL )
    {
        try
        {
            HRESULT hr;
            WCHAR str[ 1024 ];
            // Find the media file
            V( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"boatWoody.dae" ));
            // Initialize and set render settings
            RenderSettingsDX10 renderSettings;
            m_ColladaRender->GetRenderSettings(&renderSettings);
            renderSettings.defaultLightColor = D3DXVECTOR4(1,1,1,1);
            renderSettings.cullMode = RenderSettingsDX10::CULL_FORCE_DOUBLE_SIDED;
            renderSettings.mayaAmbientFix = !renderSettings.mayaAmbientFix;
            m_ColladaRender->SetRenderSettings(&renderSettings);
            // Open the file and read the scene

            collada::CreateScene(&m_pSceneCollada);
            collada::ReadSceneDae(str, m_pSceneCollada);
            // Make sure we were successful
            if(m_pSceneCollada==NULL) throw "m_pSceneCollada==NULL";

            // Initialize variables for collisions
            collada::Mesh* boatCollisionGeometry = NULL;
            collada::Mesh* boatGeometry = NULL;
            collada::MeshInstance* boatCollisionGeometryInstance = NULL;
            collada::MeshInstance* boatGeometryInstance = NULL;

            unsigned int collisionVertBatchInd;
            unsigned int vertBatchInd;

            // Get the main and collision shapes
            for(uint32_t i = 0; i< m_pSceneCollada->GetMeshInstanceCount(); i++)
            {
                if(_stricmp(m_pSceneCollada->GetMeshInstance(i)->GetEntity(m_pSceneCollada)->GetTag(), "collisionBoat1Shape") == 0)
                {
                    boatCollisionGeometry = m_pSceneCollada->GetMeshInstance(i)->GetEntity(m_pSceneCollada);
                    boatCollisionGeometryInstance = m_pSceneCollada->GetMeshInstance(i);
                }

                if(_stricmp(m_pSceneCollada->GetMeshInstance(i)->GetEntity(m_pSceneCollada)->GetTag(), "boatShape1") == 0)
                {
                    boatGeometry = m_pSceneCollada->GetMeshInstance(i)->GetEntity(m_pSceneCollada);
                    boatGeometryInstance = m_pSceneCollada->GetMeshInstance(i);
                }
            }

            // Test to see that we have the geometry objects. If not, get them from the first mesh instance
            if(boatCollisionGeometry == NULL)
            {
                boatCollisionGeometry = m_pSceneCollada->GetMeshInstance(0)->GetEntity(m_pSceneCollada);
            }
            if(boatGeometry == NULL)
            {
                boatGeometry = m_pSceneCollada->GetMeshInstance(0)->GetEntity(m_pSceneCollada);
            }
            // End of get the main and collision shapes

            // Get the input slots and vertical offsets for each shape
            collada::Batch* collisionBatch = boatCollisionGeometry->GetBatch(0, m_pSceneCollada);
            collada::Batch* batch = boatGeometry->GetBatch(0, m_pSceneCollada);
            unsigned int collisionVertOffset;
            unsigned int vertOffset;
            for(uint32_t k = 0; k<batch->GetExtraVertexElementCount(); k++)
            {
                if(_stricmp (collisionBatch ->GetExtraVertexElementDescs()[k].semanticName, "POSITION") == 0)
                {
                    collisionVertBatchInd = collisionBatch->GetExtraVertexElementDescs()[k].inputSlot;
                    collisionVertOffset = collisionBatch->GetExtraVertexElementDescs()[k].indexOffset;
                }

                if(_stricmp (batch ->GetExtraVertexElementDescs()[k].semanticName, "POSITION") == 0)
                {
                    vertBatchInd = batch->GetExtraVertexElementDescs()[k].inputSlot;
                    vertOffset = batch->GetExtraVertexElementDescs()[k].indexOffset;
                }
            }

            // Now, acquire the stride for each shape
            unsigned int collisionStride = (unsigned int)collisionBatch->GetIndexStride();
            unsigned int stride = (unsigned int)batch->GetIndexStride();

            // Declare and initialize raw index data pointers
            AutoPtr<AccessorReadDefault> collisionAccessor;
            m_providerLocal->CreateAccessorReadDefault(collisionBatch->GetIndexData(), &collisionAccessor);
            AutoPtr<AccessorReadDefault> accessor;
            m_providerLocal->CreateAccessorReadDefault(batch->GetIndexData(), &accessor);

            // Decalare buffers to store indices based on type for both shapes
            MapHelper<const long, AccessorReadDefault> collisionIndices32(collisionAccessor);
            MapHelper<const long, AccessorReadDefault> indices32(accessor);
            
			// The index count for each shape
            m_collisionIndicesCount = collisionIndices32.count/stride;
			m_indicesCount = indices32.count/stride;
            
            unsigned int* collisionVerticesIndices32 = NULL;
            unsigned int* verticesIndices32 = NULL;

            // Allocate the memory
            collisionVerticesIndices32 = (unsigned int*)malloc(sizeof(unsigned int) * m_collisionIndicesCount);

            // And fill it in
            for(unsigned int i = 0; i<(int)m_collisionIndicesCount; i++)
            {
                collisionVerticesIndices32[i] = collisionIndices32[i*stride];
            }
            // Save to the local buffer
            m_pCollisionIndices = (void*)collisionVerticesIndices32;

            // Allocate the memory
            verticesIndices32 = (unsigned int*)malloc(sizeof(unsigned int) * m_indicesCount);

            // And fill it in
            for(unsigned int i = 0; i<m_indicesCount; i++)
            {
                verticesIndices32[i] = indices32[i*stride];
            }
            // Save to the local buffer
            m_pIndices = (void*)verticesIndices32;

            // Get the main position slot
		
			AutoPtr<AccessorReadDefault> accessorVert;
			m_providerLocal->CreateAccessorReadDefault(collada::FindVertexBuffer(boatGeometry, "POSITION"), &accessorVert);
			float3* arrayOfVertices = (float3*) malloc(accessorVert->GetMemoryObject()->GetBytesSize());
			memcpy(arrayOfVertices, accessorVert->Map(), accessorVert->GetMemoryObject()->GetBytesSize());
			accessorVert->Unmap();
			
            // If we have a collision position slot, set the array of vertices to the raw vertex data of the boat
			
			AutoPtr<AccessorReadDefault> collisionAccessorVert;
			m_providerLocal->CreateAccessorReadDefault(collada::FindVertexBuffer(boatCollisionGeometry, "POSITION"), &collisionAccessorVert);
			float3* collisionArrayOfVertices = (float3*) malloc(collisionAccessorVert->GetMemoryObject()->GetBytesSize());
			memcpy(collisionArrayOfVertices, collisionAccessorVert->Map(), collisionAccessorVert->GetMemoryObject()->GetBytesSize());
			collisionAccessorVert->Unmap();

            // Get the count of vertices for both shapes
            m_collisionVerticesCount = collisionAccessorVert->GetMemoryObject()->GetBytesSize()/sizeof(float3);
            m_verticesCount = accessorVert->GetMemoryObject()->GetBytesSize()/sizeof(float3);
            // Allocate memory
            m_pCollisionVertices = (float3*) malloc(sizeof(float3)*m_collisionVerticesCount);
            //m_pVertices = (float3*) malloc(sizeof(float3)*m_verticesCount);

            // Test for validity
            if(boatCollisionGeometryInstance==NULL) throw "boatCollisionGeometryInstance==NULL";
            if(m_pCollisionVertices==NULL) throw "m_pCollisionVertices==NULL";
            //if(m_pVertices==NULL) throw "m_pVertices==NULL";
            if(boatGeometryInstance==NULL) throw "boatGeometryInstance==NULL";

            // Declar world matricies for both shapes
            float4x4 collisionGeometryWorldMatrix(boatCollisionGeometryInstance->GetNode(m_pSceneCollada)->GetWorld(m_pSceneCollada));
            float4x4 geometryWorldMatrix(boatGeometryInstance->GetNode(m_pSceneCollada)->GetWorld(m_pSceneCollada));
            // Compute a scaling matrix
            float4x4 scalingMatrix;
            MatrixScaling(&scalingMatrix, BOAT_XSCALE, BOAT_YSCALE, BOAT_ZSCALE);
            // Declare and initialize a transformation matrix
            float4x4 transformationMatrix;
            MatrixMultiply(&transformationMatrix, &collisionGeometryWorldMatrix, &scalingMatrix);

            // For every collision vertet
            for(unsigned int i = 0; i<m_collisionVerticesCount; i++)
            {
                float4 translatedVertex;
                float4 initialVertex;
                // Store the initial vertex
                initialVertex.x = collisionArrayOfVertices[i].x;
                initialVertex.y = collisionArrayOfVertices[i].y;
                initialVertex.z = collisionArrayOfVertices[i].z;
                initialVertex.w = 1.0f;

                // Transform the initial vertex by scaling
                Vec4Transform(&translatedVertex, &initialVertex, &scalingMatrix);
                
                // Store the results
                m_pCollisionVertices[i].x = translatedVertex.x;
                m_pCollisionVertices[i].y = translatedVertex.y;
                m_pCollisionVertices[i].z = translatedVertex.z;
                collisionArrayOfVertices[i].x = 100000.0f;
                collisionArrayOfVertices[i].y = 100000.0f;
                collisionArrayOfVertices[i].z = 100000.0f; 
            }
			AutoPtr<AccessorWriteDefault> collisionAccessorVertWrite;
			m_providerLocal->CreateAccessorWriteDefault(collada::FindVertexBuffer(boatCollisionGeometry, "POSITION"), &collisionAccessorVertWrite, NULL);
			memcpy(collisionAccessorVertWrite->Map(), collisionArrayOfVertices, collisionAccessorVert->GetMemoryObject()->GetBytesSize());
			collisionAccessorVertWrite->Unmap();
			free(collisionArrayOfVertices);
            // For every vertex in the main shape
            for(unsigned int i = 0; i<m_verticesCount; i++)
            {
                float4 translatedVertex;
                float4 initialVertex;
                // Store the initial vertex, it will be used in the transform
                initialVertex.x = arrayOfVertices[i].x;
                initialVertex.y = arrayOfVertices[i].y;
                initialVertex.z = arrayOfVertices[i].z;
                initialVertex.w = 1.0f;

                // Transform the matrix by scaling
                Vec4Transform(&translatedVertex, &initialVertex, &scalingMatrix);

                // Store the result
                arrayOfVertices[i].x = translatedVertex.x;
                arrayOfVertices[i].y = translatedVertex.y;
                arrayOfVertices[i].z = translatedVertex.z;
            }
			AutoPtr<AccessorWriteDefault> accessorVertWrite;
			m_providerLocal->CreateAccessorWriteDefault(collada::FindVertexBuffer(boatGeometry, "POSITION"), &accessorVertWrite, NULL);

			memcpy(accessorVertWrite->Map(), arrayOfVertices, accessorVert->GetMemoryObject()->GetBytesSize());
			accessorVertWrite->Unmap();
			free(arrayOfVertices);
            // Convert and render the scene
            collada::AutoPtr<collada::Scene> converted;
            PrepareForRender(m_providerD3D10, m_providerLocal, m_pSceneCollada, &converted);
            m_ColladaRender->AttachScene(converted);
        }
        catch( const std::exception &e )
        {
            printf( "could not load scene: %s\n", e.what( ));
        }

    }
}
/**
 * @fn UpdateViewAccording2Mesh()
 *
 * @brief This method updates the center & camera parameters according to the mesh 
 */
void CFluidSimHost::UpdateViewAccording2Mesh()
{
    float sizeX = (m_pScene->GetRegion().x2 - m_pScene->GetRegion().x1) * 0.5f;
    float sizeZ = (m_pScene->GetRegion().y2 - m_pScene->GetRegion().y1) * 0.5f;
    D3DXVECTOR3 vCenter( m_pScene->GetRegion().x1 + sizeX, 0.0f, m_pScene->GetRegion().y1 + sizeZ );
    FLOAT fObjectRadius = __max(sizeX, sizeZ);

    D3DXMatrixTranslation( &m_mCenterMesh, -vCenter.x, -vCenter.y, -vCenter.z );

    // Setup a first person camera start position (manually adjusted)
    D3DXVECTOR3 vecEyeFP = D3DXVECTOR3(100.0f, 30.0f, 100.0f);
    D3DXVECTOR3 vecAtFP  = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
    m_FPCamera.SetViewParams( &vecEyeFP, &vecAtFP );
    m_FPCamera.SetScalers(0.005f, 20.0f);
    m_FPCamera.SetCurrentParamsDefault();

    // Setup the model view camera's view parameters (manually adjusted) 
    D3DXVECTOR3 vecEye(100.0f, 200.0f, -100.0f);
    D3DXVECTOR3 vecAt (500.0f, 150.0f, 150.0f);
    m_MVCamera.SetViewParams( &vecEye, &vecAt );
    m_MVCamera.SetRadius( fObjectRadius*1.3f, fObjectRadius*0.5f, fObjectRadius*10.0f );
    m_MVCamera.SetCurrentParamsDefault();
}

/**
 * @fn OnDestroyDevice()
 *
 * @brief This routine is called when the device is about to be destroyed.
 *
 * @note The application should release all device-dependent resources in this function.
 */
int ReleaseScene();

#define SAFE_FREE(POINTER_TO_OBJECT)\
    if((POINTER_TO_OBJECT) != NULL)\
    free(POINTER_TO_OBJECT);

void CFluidSimHost::OnDestroyDevice()
{
    printf("OnDestroyDevice call\n");
    ///CBufferList bufs0;

    //Resume and destroy asynchronous processing thread
    DeleteCriticalSection(&m_cs);
    if(m_hThread)
    {
        ///ResumeThread(m_hThread);
        SetEvent(m_hEvent);
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        CloseHandle(m_hEvent);
        m_hThread = 0;
        m_hEvent = 0;
    }



    SAFE_DELETE( m_pSceneVis );
    SAFE_DELETE( m_pSky );
    SAFE_DELETE( m_pCommonData );
    if(m_pScene)
    {
        DestroyObject( m_pScene );
        m_pScene = NULL;
    }
    // Run the ReleaseScene function
    ReleaseScene();




    // Destroy the transformation buffer and buffer map
    if(m_pTransformBuffer)
    {
        free(m_pTransformBuffer);
    }



    // If we have a collada, it needs to be deleted
    if( m_pSceneCollada )
    {
        // The scene must be detached before deleting
        m_ColladaRender->DetachScene( ); 
        SafeReleasePtr( m_pSceneCollada );
    }

    SafeReleasePtr(m_pSceneColladaInvisible);

    // If a static collada scene has been created, we need to delete it as well
    if( m_pSceneColladaStatic )
    {
        m_ColladaRenderStatic->DetachScene( );
        SafeReleasePtr( m_pSceneColladaStatic );
    }
    // Detach the device from the renderers
    m_providerD3D10->GetDeviceManager()->SetDevice(NULL);

    // Disable gathered visualization
    m_bDisableGathered = false;

    SAFE_FREE(m_pVertices);                    
    SAFE_FREE(m_pCollisionVertices);                  
    SAFE_FREE(m_verticesTransformed);                 
    SAFE_FREE(m_pCollisionIndices);                   
    SAFE_FREE(m_pIndices);                            


    // Then call the baseclass to handle common tasks
    __super::OnDestroyDevice();
}// OnDestroyDevice

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
UINT g_removeMe_width;
UINT g_removeMe_height;
HRESULT CFluidSimHost::OnResizedSwapChain( ID3D10Device* pd3dDevice,
    IDXGISwapChain *pSwapChain,
    const DXGI_SURFACE_DESC* in_pBufferSurfaceDesc )
{
	g_removeMe_width = in_pBufferSurfaceDesc->Width;
	g_removeMe_height = in_pBufferSurfaceDesc->Height;
    printf("OnResizedSwapChain call\n");
    // First, call the base class instance
    __super::OnResizedSwapChain(pd3dDevice, pSwapChain, in_pBufferSurfaceDesc);

    // Setup the camera's projection parameters
    float fAspectRatio = in_pBufferSurfaceDesc->Width / (FLOAT)in_pBufferSurfaceDesc->Height;
    m_MVCamera.SetProjParams( D3DX_PI/4, fAspectRatio, 10.0f, 5000.0f );
    m_MVCamera.SetWindow( in_pBufferSurfaceDesc->Width, in_pBufferSurfaceDesc->Height );
    m_MVCamera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );

    m_FPCamera.SetProjParams( D3DX_PI/4, fAspectRatio, 0.5f, 3000.0f );
    m_FPCamera.SetRotateButtons( true, false, false, false );
    return S_OK;
} // End of OnResizedSwapChain


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
void CFluidSimHost::MouseProc( bool bLeftButtonDown,  // The left mouse button is down.
    bool bRightButtonDown,                            // The right mouse button is down.
    bool bMiddleButtonDown,                           // The middle mouse button is down.
    bool bSideButton1Down,                            // Microsoft Windows 2000/Windows XP: The first X button is down.
    bool bSideButton2Down,                            // Windows 2000/Windows XP: The second X button is down.
    int nMouseWheelDelta,                             // The distance and direction the mouse wheel has rolled, expressed in multiples or divisions of WHEEL_DELTA, which is 120. A positive value indicates that the wheel was rotated forward, away from the user; a negative value indicates that the wheel was rotated backward, toward the user.
    int xPos,                                         // x-coordinate of the pointer, relative to the upper-left corner of the client area.
    int yPos )                                        // y-coordinate of the pointer, relative to the upper-left corner of the client area.
{
    // Call the base class instance to handle common mouse events
    __super::MouseProc(bLeftButtonDown, bRightButtonDown, bMiddleButtonDown, bSideButton1Down, bSideButton2Down, nMouseWheelDelta, xPos, yPos);

    // We only care about the right mouse button. If the right mouse button is down
    if (bRightButtonDown)
    {
        // Get the client rectangle
        RECT wndRect = DXUTGetWindowClientRect();
        // Get a local copy of the projection matrix
        D3DXMATRIX const *pmatProj = m_pCamera->GetProjMatrix();

        // Compute the vector of the pick ray in screen space
        D3DXVECTOR3 v;
        v.x =  ( ( ( 2.0f * xPos ) / float(wndRect.right)  ) - 1 ) / pmatProj->_11;
        v.y = -( ( ( 2.0f * yPos ) / float(wndRect.bottom) ) - 1 ) / pmatProj->_22;
        v.z =  1.0f;

        // Get the inverse view matrix
        D3DXMATRIX m;
        if (m_pCamera == &m_MVCamera)
        {
            D3DXMATRIX const &matView = *m_MVCamera.GetViewMatrix();
            D3DXMATRIX const &matWorld = *m_MVCamera.GetWorldMatrix();
            D3DXMATRIX mWorldView = matWorld * matView;
            D3DXMatrixInverse( &m, NULL, &mWorldView );
        }
        else
        {
            D3DXMatrixInverse( &m, NULL, m_pCamera->GetViewMatrix() );
        }

        // Transform the screen space pick ray into 3D space
        D3DXVECTOR3 vPickRayDir;
        D3DXVECTOR3 vPickRayOrig;
        vPickRayDir.x  = v.x*m._11 + v.y*m._21 + v.z*m._31;
        vPickRayDir.y  = v.x*m._12 + v.y*m._22 + v.z*m._32;
        vPickRayDir.z  = v.x*m._13 + v.y*m._23 + v.z*m._33;
        vPickRayOrig.x = m._41;
        vPickRayOrig.y = m._42;
        vPickRayOrig.z = m._43;

        // Test to see if there is an intersection with the surface 
        float x, y;
        if (IntersectWithSurface(vPickRayOrig, vPickRayDir, x, y))
        {
            m_WaterSimulationParams.wasClicked = true;
            m_WaterSimulationParams.x = x;
            m_WaterSimulationParams.y = y;
        }
    }
} // End of MouseProc


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
LRESULT CFluidSimHost::MsgProc( HWND   hWnd,    // Handle to the application window
    UINT   uMsg,                                // Windows message code
    WPARAM wParam,                              // wParam
    LPARAM lParam,                              // lParam
    bool* pbNoFurtherProcessing )
{
    LRESULT res;

    // Call the base class to handle common messages
    res = __super::MsgProc( hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing );

    // If the base class processed the message, the return value will be non-zero
    // and we need no further processing. Also, the input flag can specifity that
    // no further processing occur. In either case, return the returned result code
    if( *pbNoFurtherProcessing || 0 != res )
    {
        return res;
    }

    // Further processing is allowed, pass the message on to the camera
    m_pCamera->HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}

/**
 * @fn OnInitApp()
 *
 * @brief This routine is called only once after framework initialization and before the D3D device is created.
 *
 * @param io_StartupInfo - Pointer to the structure that contains the startup settings 
 */
#define MINIMIZE_CONTROLS_NUMBER

void CFluidSimHost::OnInitApp( StartupInfo *io_StartupInfo ) ///< Pointer to a structure that contains startup settings
{
    int yUIOffset = 10;

    __super::OnInitApp(io_StartupInfo);

    // Set the window title
    wcscpy_s(io_StartupInfo->WindowTitle, L"Shallow Water Simulation Sample");

    // Create and initialize UI controls
    CDXUTDialog* pUI = GetSampleUI();

#ifndef MINIMIZE_CONTROLS_NUMBER
    AddSlider(yUIOffset,&g_Noise,MIN_NOISE, MAX_NOISE, L"Noise: %0.f");

    AddSlider(yUIOffset,&g_omega, MIN_OMEGA, MAX_OMEGA, L"Omega: %3.2f");
#endif

#ifndef MINIMIZE_CONTROLS_NUMBER
    AddSlider(yUIOffset,&g_ampl, MIN_AMPL, MAX_AMPL, L"Amplitude: %3.2f");
#endif

    AddSlider(yUIOffset,&m_maxDepth, 1.0f, 140.0f, L"Max depth: %3.2f", true);

#ifndef MINIMIZE_CONTROLS_NUMBER
    AddSlider(yUIOffset,&m_localNormalScale, 0.1f, 2.0f, L"Local normal scale: %3.2f", true);
    AddSlider(yUIOffset,&m_resetLatency, 0.05f, 20.0f, L"Reset time: %3.2f sec", true);
#endif


    pUI->AddStatic(IDC_STATIC, L"(C)amera mode", STATIC_OFFSET, yUIOffset += CONTROLS_OFFSET, STATIC_WIDTH, CONTROLS_HEIGHT);
    pUI->AddComboBox( IDC_CAMMODE, COMBOBOX_OFFSET, yUIOffset += CONTROLS_OFFSET, COMBOBOX_WIDTH, CONTROLS_HEIGHT, 'C', true);

#ifndef MINIMIZE_CONTROLS_NUMBER
    pUI->AddStatic(IDC_STATIC, L"(V)isualization mode", STATIC_OFFSET, yUIOffset += CONTROLS_OFFSET, STATIC_WIDTH, CONTROLS_HEIGHT);
    pUI->AddComboBox( IDC_VISMODE, COMBOBOX_OFFSET, yUIOffset += CONTROLS_OFFSET, COMBOBOX_WIDTH, CONTROLS_HEIGHT, 'V', true);
#endif

    pUI->AddCheckBox( IDC_WIRE, L"WireFrame (F4)", CHECKBOX_OFFSET, yUIOffset += CONTROLS_OFFSET, CHECKBOX_WIDTH, CONTROLS_HEIGHT, false, VK_F4);
    pUI->GetCheckBox(IDC_WIRE)->SetEnabled(true);

    pUI->AddCheckBox( IDC_WIND_WAVE, L"Wind waves (F5)", CHECKBOX_OFFSET, yUIOffset += CONTROLS_OFFSET, CHECKBOX_WIDTH, CONTROLS_HEIGHT, m_WaterSimulationParams.is_wind_waves, VK_F5);
#ifndef MINIMIZE_CONTROLS_NUMBER
    AddSlider(yUIOffset,&m_WaterSimulationParams.ww.speed, 0.1f, 20.0f, L"WW velocity: m/s %3.0f");
    AddSlider(yUIOffset,&m_WaterSimulationParams.ww.amplitude, 0.01f, 1.0f, L"WW amplitude: m %3.2f");
#endif
    //yUIOffset += CONTROLS_OFFSET;
    pUI->AddStatic(IDC_STATIC, L"Solver (O)penCL / Native", STATIC_OFFSET, yUIOffset += CONTROLS_OFFSET, STATIC_WIDTH, CONTROLS_HEIGHT);
    pUI->AddComboBox( IDC_OPENCL, COMBOBOX_OFFSET, yUIOffset += CONTROLS_OFFSET, COMBOBOX_WIDTH+30, CONTROLS_HEIGHT, 'O', true);

    pUI->AddStatic(IDC_STATIC, L"Scene select Sea/River", STATIC_OFFSET, yUIOffset += CONTROLS_OFFSET, STATIC_WIDTH, CONTROLS_HEIGHT);
    pUI->AddComboBox( IDC_SCENE, COMBOBOX_OFFSET, yUIOffset += CONTROLS_OFFSET, COMBOBOX_WIDTH+30, CONTROLS_HEIGHT, 0, true);//'R', true);

    pUI->AddStatic(IDC_CPUGPU, L"Device select CPU/GPU", STATIC_OFFSET, yUIOffset += CONTROLS_OFFSET, STATIC_WIDTH, CONTROLS_HEIGHT);
    pUI->AddComboBox( IDC_CPUGPU, COMBOBOX_OFFSET, yUIOffset += CONTROLS_OFFSET, COMBOBOX_WIDTH+30, CONTROLS_HEIGHT, 0, true);//'R', true);

    pUI->AddCheckBox( IDC_ASYNC, L"Sync/Async (F6)", CHECKBOX_OFFSET, yUIOffset += CONTROLS_OFFSET, CHECKBOX_WIDTH, CONTROLS_HEIGHT, false, VK_F6);
    pUI->GetCheckBox(IDC_ASYNC)->SetEnabled(true);

    pUI->AddCheckBox( IDC_RELAXED, L"OCL relaxed math", CHECKBOX_OFFSET, yUIOffset += CONTROLS_OFFSET, CHECKBOX_WIDTH, CONTROLS_HEIGHT, false, 0);//VK_F7);
    pUI->GetCheckBox(IDC_RELAXED)->SetEnabled(true);


//    LoadColladaScenes();
}// End of OnInitApp

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
bool CFluidSimHost::ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings )///< Pointer to a structure that contains the settings for the new device.
{
    // Call the base class to handle common settings
    __super::ModifyDeviceSettings(pDeviceSettings);

    // We need to track the calls and only execute the very first time this is called
    static bool firstTime = true;

    if ( firstTime )
    {
        if ( pDeviceSettings->ver == DXUT_D3D10_DEVICE )
        {
            pDeviceSettings->d3d10.SyncInterval = 0;
        }

        firstTime = false;

        /* ******************************************** CODE ENABLES REFERENCE RASTERIZER ************************************* */
        //const int size = 32;
        //char pBuff[size];

        //{
        //    if ( pDeviceSettings->ver == DXUT_D3D9_DEVICE )
        //    {
        //        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_SW;
        //    }

        //    if ( pDeviceSettings->ver == DXUT_D3D10_DEVICE )
        //    {
        //        pDeviceSettings->d3d10.DriverType = D3D10_DRIVER_TYPE_REFERENCE;
        //    }
        //}
        /* ****************************************** END REFERENCE RASTERIZER *********************************** */
    }

    return true;
}// End of ModifyDeviceSettings

/**
 * @fn OnFrameMove()
 *
 * @brief A method called by the framework once each frame, before the application renders the scene.
 *
 * @param fTime - The time elapsed since the application started, in seconds.
 *
 * @param fElapsedTime - The time elapsed since the last frame, in seconds.
 */
bool is_first_call = false;//true;

void CFluidSimHost::OnFrameMove( double fTime,  // Time elapsed since the application started, in seconds.
    float fElapsedTime )                        // Time elapsed since the last frame, in seconds.
{

    // The base class implementation comes first
    __super::OnFrameMove(fTime, fElapsedTime);

    int i;
    
    // Limit the time delta
    if (fElapsedTime > 0.2f)
    {
        fElapsedTime = 0.05f;
    }

    //enable async processing as default 
    if(is_first_call)
    {
        // Get a pointer to the user interface
        CDXUTDialog* pUI = GetSampleUI();
        pUI->GetCheckBox(IDC_ASYNC)->SetChecked(true);
        m_bAsync = true;
        {
            // Create processing thread for assync mode
            // Create a manual-reset nonsignaled unnamed event
            m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            m_hThread = CreateThread( NULL, 0, ProcessThreadWraper, this, 0, &m_threadID);
            if (m_hThread == 0) 
            {
                printf( "fail to init async thread\n");
            }
            pUI->GetComboBox(IDC_SCENE)->SetEnabled(false);
            pUI->GetComboBox(IDC_CPUGPU)->SetEnabled(false);
            pUI->GetCheckBox(IDC_RELAXED)->SetEnabled(false);
        }
        is_first_call = false;
    }


    ///ProcessThread();
    ///Sync processing
    if(!m_bAsync)
    {
        // For each grid in the scene
        for(i=0;i<m_pScene->GetNumGrids();++i)
        {
            // Set the grid level of dtail and the height and width of the water simulations
            m_pScene->GetGrid(i)->SetLOD(m_fLevelOfDetails);
            m_WaterSimulationParams.DomainW[i] = m_pScene->GetGrid(i)->GetDomainW();
            m_WaterSimulationParams.DomainH[i] = m_pScene->GetGrid(i)->GetDomainH();
        }

        // Set some parameters for the simulation, use global settings
        m_WaterSimulationParams.Noise = g_Noise;
        m_WaterSimulationParams.NoiseSpaceCoherence = g_NoiseSpaceCoherence;
        m_WaterSimulationParams.NoiseTimeCoherence = g_NoiseTimeCoherence;
        m_WaterSimulationParams.omega = g_omega;
        m_WaterSimulationParams.ampl = g_ampl;
        m_WaterSimulationParams.time_step = m_pScene->GetRecommendedTimeStep();


        // If we are not paused, then update the scene
        if (!m_bPause)
        {
            // If not in real-time then.....
            // Real time branch is currently disabled
            // Do as much simulation steps as possible
            if (!m_bRealTime)
            {
                // If the boat is enabled
                if(m_WaterSimulationParams.enableBoat)
                {
                    // Get the time if the animation is not frozen
                    m_time = m_isAnimationFreezed ? 0 : (m_time + ANIMATION_TIME_STEP);
                    // If we have the invisible scene
                    if(m_pSceneColladaInvisible)
                    {
                        // Get the transformation matrix based on the time
                        const float* pT = GetNewTransformationMatrix(m_pSceneColladaInvisible, m_time);
                        // and then set the boat transformation matrix
                        if(pT)memcpy(m_WaterSimulationParams.boatTransform, pT, sizeof(float)*16);
                    }
                }

                LARGE_INTEGER Frequency;
                LARGE_INTEGER PerformanceCount1 , PerformanceCount2;
                Frequency.QuadPart = 0;
                PerformanceCount1.QuadPart = 0;
                PerformanceCount2.QuadPart = 0;

                if(m_FPScounter == 30)
                {
                    QueryPerformanceFrequency(&Frequency);
                    QueryPerformanceCounter(&PerformanceCount1);
                }

                // Make the calculations for the shallow water algorithm step
                SimulationStep((void*)&m_WaterSimulationParams);

                if(m_FPScounter == 30)
                {
                    QueryPerformanceCounter(&PerformanceCount2);
                    m_currentFPS = (float)Frequency.QuadPart/(float)(PerformanceCount2.QuadPart-PerformanceCount1.QuadPart);
                    m_FPScounter = 0;
                }
                else
                {
                    m_FPScounter++;
                }

                // Get the boat data from OCL 
                GetHybridMethodData();

                // Then update with the new grid data
                m_pSceneVis->Update(m_WaterSimulationParams.time_step);

                ///QueryPerformanceCounter(&PerformanceCount2);
                ///m_currentFPS = (float)Frequency.QuadPart/(float)(PerformanceCount2.QuadPart-PerformanceCount1.QuadPart);



                // Disable gathered visualization
                //{
                //    if(!m_bDisableGathered)
                //    {
                //        m_pSceneVis->SetMode(1);
                //        m_bDisableGathered = true;
                //    }
                //}

                //Debug code
#ifdef STOP_SIMULATION_FOR_BAD_HEIGHT

                // This code is kept for debug purposes
                {   
                    // Check the grid for bad values and if found, stop the simulation
                    int i;
                    for(i=0;i<m_pScene->GetNumGrids();++i)
                    {
                        CFlux2DGrid* pG = m_pScene->GetGrid();
                        int w= pG->GetDomainW();
                        int h= pG->GetDomainH();
                        int x,y;
                        // For every line in the image
                        for(y=0;y<h;++y)
                        {
                            // For every pixel in the line
                            for(x=0;x<h;++x)
                            {
                                float val = pG->GetCurrentSurface().m_heightMap.At(x,y);
                                if(fabs(val)>20.0f)
                                {
                                    printf("\nx=%d,y=%d pixel = %f height, simulation paused\n",x,y,val);
                                    m_bPause = 1;
                                }
                            }
                        }

                    }
                }
#endif
            }

            // If we are processing one step at at time, pause the display
            if(m_bOneStep)
            {
                m_bPause = 1;
                m_bOneStep = 0;
            }
        }
    }//m_bAsync
    else
    {
        // Then update with the new grid data
        while(TryEnterCriticalSection(&m_cs)==0)
        {
        }
        m_pSceneVis->Update(m_WaterSimulationParams.time_step);
        LeaveCriticalSection(&m_cs);
    }

    // Update the camera's position based on user input 
    m_pCamera->FrameMove( fElapsedTime );
    
    // A hack to get the camera to follow the ground if so enabled
    if (m_bFollowGround)
    {
        D3DXVECTOR3         eyePt = *m_pCamera->GetEyePt();
        D3DXVECTOR3         lookAtPt = *m_pCamera->GetLookAtPt();
        float shift = m_pScene->GetSurfaceHeight(eyePt.x - m_mCenterMesh._41,eyePt.z - m_mCenterMesh._43) + 2 - eyePt.y;
        eyePt.y += shift;
        lookAtPt.y += shift;
        m_pCamera->SetViewParams(&eyePt, &lookAtPt);
        m_pCamera->FrameMove( 0.0f );
    }
    
    // The mouse has already been processed. Reset the flag
    m_WaterSimulationParams.wasClicked = false;

}// End of CFluidSimHost::OnFrameMove

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
void CFluidSimHost::KeyboardProc( UINT nChar,
    bool bKeyDown,
    bool bAltDown )
{
    // Call the base class to handle common keyboard events
    __super::KeyboardProc(nChar, bKeyDown, bAltDown);

    // If there is no key down, then return, we are done
    if( !bKeyDown )
    {
        return;
    }

    // If a key is down, handle it
    switch( nChar )
    {
    case VK_F1: m_bShowHelp = !m_bShowHelp; break;  
    case VK_F10: m_bShowControls = !m_bShowControls; break;
    case VK_SPACE: m_bPause = !m_bPause; break;
    case VK_RETURN: m_bPause = 0; m_bOneStep=1; break;
    }
}// End of KeyboardProc



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
void CFluidSimHost::OnGUIEvent( UINT nEvent,
    int nControlID,
    CDXUTControl* pControl )
{
    // Call the base class implementation first to handle common events
    __super::OnGUIEvent( nEvent, nControlID, pControl);

    // Get the dialog box pointer
    CDXUTDialog * const pUI = GetSampleUI();

    UpdateSlider(pUI,nControlID);

    // Depending on the control, set the correct parameter
    switch( nControlID )
    {
    case IDC_VISMODE:
        {
            int index = int( pUI->GetComboBox(IDC_VISMODE)->GetSelectedData() );
            m_pSceneVis->SetMode(index);
            break;
        }

    case IDC_CAMMODE:
        {
            switch (pUI->GetComboBox(IDC_CAMMODE)->GetSelectedIndex())
            {
            case 0: m_pCamera = &m_MVCamera; m_bFollowGround = false; break;
            case 1: m_pCamera = &m_FPCamera; m_bFollowGround = false; break;
            case 2: m_pCamera = &m_FPCamera; m_bFollowGround = true; break;
            }
            break;
        }
    case IDC_OPENCL:
        {
            switch (pUI->GetComboBox(IDC_OPENCL)->GetSelectedIndex())
            {
            case 0: m_WaterSimulationParams.solverNumber = 0; break;
            case 1: m_WaterSimulationParams.solverNumber = 1; break;
            case 2: m_WaterSimulationParams.solverNumber = 2; break;
            case 3: m_WaterSimulationParams.solverNumber = 3; break;
            case 4: m_WaterSimulationParams.solverNumber = 4; break;
            }
            break;
        }
    case IDC_SCENE:
        {
            switch (pUI->GetComboBox(IDC_SCENE)->GetSelectedIndex())
            {
            case 0: m_WaterSimulationParams.sceneNumber = 0; break;
            case 1: m_WaterSimulationParams.sceneNumber = 1; break;
            case 2: m_WaterSimulationParams.sceneNumber = 2; break;
            case 3: m_WaterSimulationParams.sceneNumber = 3; break;
            case 4: m_WaterSimulationParams.sceneNumber = 4; break;
            case 5: m_WaterSimulationParams.sceneNumber = 5; break;
            }
            SetScene(m_WaterSimulationParams.sceneNumber);
            UpdateViewAccording2Mesh();
            //Send new scene to OCL()
            ReleaseScene();
            // BEGIN Call Init function on the OCL side 
            {
                void*               pSceneBuffer;
                void*               pBoatData = NULL;           // A pointer to the memory with boat data
                unsigned int        BoatDataSize = 0;           // The size of the memory with boat data

                // Save the scene to the memory file
                CFileMemory memFile;
                m_pScene->Save(memFile);

                // Create an buffer with the scene
                pSceneBuffer = malloc(memFile.GetSize());
                memcpy(pSceneBuffer, memFile.GetData(), memFile.GetSize());


                // Fill pBoatData with the boat data 
                DynamicArray<char>  BoatData;
                SerializeBoatModel(
                    m_collisionVerticesCount, m_pCollisionVertices, 
                    m_collisionIndicesCount, m_pCollisionIndices, 
                    &BoatData);
                BoatDataSize = BoatData.Size();
                if(BoatDataSize)pBoatData = &(BoatData[0]);

                // Run the LoadScene function on the OCL side
                float x_boat_pos, y_boat_pos;
                x_boat_pos = 50;
                y_boat_pos = 110;
                switch (m_WaterSimulationParams.sceneNumber)
                {
                case 0: x_boat_pos = 50; y_boat_pos = 110; break;
                case 1: x_boat_pos = 200; y_boat_pos = 110; break;
                case 2: x_boat_pos = 50; y_boat_pos = 110; break;
                case 3: x_boat_pos = 200; y_boat_pos = 110; break;
                case 4: x_boat_pos = 50; y_boat_pos = 110; break;
                case 5: x_boat_pos = 200; y_boat_pos = 110; break;
                }
				LoadScene(pSceneBuffer, (uint32_t)(memFile.GetSize()), pBoatData, (uint32_t)BoatDataSize, x_boat_pos, y_boat_pos, m_WaterSimulationParams.enableGPUProcessing);


                // Free the scene buffer.
                free(pSceneBuffer);
            } 

            m_pSceneVis->SetWireframe( pUI->GetCheckBox(IDC_WIRE)->GetChecked());

            // Rebuild OpenCL kernel
            RebuildOCLKernel(m_bOCLRelaxedMath, m_WaterSimulationParams.enableGPUProcessing);
            // END Call Init function on the OCL side 
            break;
        }
    case IDC_CPUGPU:
        {
            switch (pUI->GetComboBox(IDC_CPUGPU)->GetSelectedIndex())
            {
			case 0: m_WaterSimulationParams.enableGPUProcessing = false; break;
            case 1: m_WaterSimulationParams.enableGPUProcessing = true; break;
            }
            SetScene(m_WaterSimulationParams.sceneNumber);
            UpdateViewAccording2Mesh();
            //Send new scene to OCL()
            ReleaseScene();
            // BEGIN Call Init function on the OCL side 
            {
                void*               pSceneBuffer;
                void*               pBoatData = NULL;           // A pointer to the memory with boat data
                unsigned int        BoatDataSize = 0;           // The size of the memory with boat data

                // Save the scene to the memory file
                CFileMemory memFile;
                m_pScene->Save(memFile);

                // Create an buffer with the scene
                pSceneBuffer = malloc(memFile.GetSize());
                memcpy(pSceneBuffer, memFile.GetData(), memFile.GetSize());


                // Fill pBoatData with the boat data 
                DynamicArray<char>  BoatData;
                SerializeBoatModel(
                    m_collisionVerticesCount, m_pCollisionVertices, 
                    m_collisionIndicesCount, m_pCollisionIndices, 
                    &BoatData);
                BoatDataSize = BoatData.Size();
                if(BoatDataSize)pBoatData = &(BoatData[0]);

                // Run the LoadScene function on the OCL side
                float x_boat_pos, y_boat_pos;
                x_boat_pos = 50;
                y_boat_pos = 110;
                switch (m_WaterSimulationParams.sceneNumber)
                {
                case 0: x_boat_pos = 50; y_boat_pos = 110; break;
                case 1: x_boat_pos = 200; y_boat_pos = 110; break;
                case 2: x_boat_pos = 50; y_boat_pos = 110; break;
                case 3: x_boat_pos = 200; y_boat_pos = 110; break;
                case 4: x_boat_pos = 50; y_boat_pos = 110; break;
                case 5: x_boat_pos = 200; y_boat_pos = 110; break;
                }
				LoadScene(pSceneBuffer, (uint32_t)(memFile.GetSize()), pBoatData, (uint32_t)BoatDataSize, x_boat_pos, y_boat_pos, m_WaterSimulationParams.enableGPUProcessing);


                // Free the scene buffer.
                free(pSceneBuffer);
            } 

            m_pSceneVis->SetWireframe( pUI->GetCheckBox(IDC_WIRE)->GetChecked());

			if(m_WaterSimulationParams.enableGPUProcessing)
			{
				pUI->GetCheckBox(IDC_RELAXED)->SetVisible(false);
			}
			else
			{
				pUI->GetCheckBox(IDC_RELAXED)->SetVisible(true);
			}

            // Rebuild OpenCL kernel
            RebuildOCLKernel(m_bOCLRelaxedMath, m_WaterSimulationParams.enableGPUProcessing);
            // END Call Init function on the OCL side 
            break;
        }
    case IDC_WIRE:
        m_pSceneVis->SetWireframe( pUI->GetCheckBox(IDC_WIRE)->GetChecked());
        break;

    case IDC_WIND_WAVE:
        m_WaterSimulationParams.is_wind_waves = !m_WaterSimulationParams.is_wind_waves;
        break;

    case IDC_REALTIME:
        m_bRealTime = pUI->GetCheckBox(IDC_REALTIME)->GetChecked();
        break;
    case IDC_ASYNC:
        m_bAsync = pUI->GetCheckBox(IDC_ASYNC)->GetChecked();
        if(m_bAsync)
        {
            // Create processing thread for assync mode
            // Create a manual-reset nonsignaled unnamed event
            m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            m_hThread = CreateThread( NULL, 0, ProcessThreadWraper, this, 0, &m_threadID);
            if (m_hThread == 0) 
            {
                printf( "fail to init async thread\n");
            }
            pUI->GetComboBox(IDC_SCENE)->SetEnabled(false);
            pUI->GetCheckBox(IDC_RELAXED)->SetEnabled(false);
            pUI->GetComboBox(IDC_CPUGPU)->SetEnabled(false);
        }
        else
        {
            if(m_hThread)
            {
                SetEvent(m_hEvent);
                WaitForSingleObject(m_hThread, INFINITE);
                CloseHandle(m_hThread);
                CloseHandle(m_hEvent);
                m_hThread = 0;
                m_hEvent = 0;
            }
            pUI->GetComboBox(IDC_SCENE)->SetEnabled(true);
            pUI->GetCheckBox(IDC_RELAXED)->SetEnabled(true);
            pUI->GetComboBox(IDC_CPUGPU)->SetEnabled(true);
        }
        break;
    case IDC_RELAXED:
        m_bOCLRelaxedMath = pUI->GetCheckBox(IDC_RELAXED)->GetChecked();
        // Rebuild OpenCL kernel
        RebuildOCLKernel(m_bOCLRelaxedMath, m_WaterSimulationParams.enableGPUProcessing);

        break;

    }

    CVisual3DGathered* pGS = m_pSceneVis->GatheredVisual();
    if(pGS)
    {
        // Update visualisation parameters for the gathered visualization engine
        pGS->SetLocalNormalScale(m_localNormalScale);
        pGS->SetMaxDepth(m_maxDepth);
        pGS->SetLatency(m_resetLatency);
    }
} // End of CFluidSimHost::OnGUIEvent

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
#define DEFAULT_MATRIX_SCALING 11.0f

void CFluidSimHost::OnRenderScene( ID3D10Device* pd3dDevice,
    double fTime,
    float fElapsedTime )
{
    // Call the base class first
    __super::OnRenderScene(pd3dDevice, fTime, fElapsedTime);
    
    // Clear the render target and depth stencil
    float ClearColor[4] = { 0.0f, 0.25f, 0.25f, 0.55f }; //manually adjusted
    ID3D10RenderTargetView* pRTV = DXUTGetD3D10RenderTargetView();
    pd3dDevice->ClearRenderTargetView( pRTV, ClearColor );
    ID3D10DepthStencilView* pDSV = DXUTGetD3D10DepthStencilView();
    pd3dDevice->ClearDepthStencilView( pDSV, D3D10_CLEAR_DEPTH, 1.0, 0 );
	pd3dDevice->OMSetRenderTargets(1, &pRTV, pDSV);
	D3D10_RENDER_TARGET_VIEW_DESC desc;
	D3D10_VIEWPORT vp={
		0,0,
		g_removeMe_width,g_removeMe_height,
		0,1
	};
	pd3dDevice->RSSetViewports(1, &vp);
    if(m_pSceneColladaStatic)
    {
        // Handle animation 
        for( size_t i = 0; i < m_pSceneColladaStatic->GetAnimationClipCount(); ++i )
        {
            m_pSceneColladaStatic->GetAnimationClip(i)->SetTimePointer(fmod((float) fTime, m_pSceneColladaStatic->GetAnimationClip(i)->GetLength()));
        }
    }


    D3DXMATRIX  mWorldViewProjection;
    D3DXMATRIX  mWorld;
    D3DXMATRIX  mView;
    D3DXMATRIX  mProj;

    // Get the projection & view matrix from the camera class
    if (m_pCamera == &m_MVCamera)
    {
        D3DXMATRIX world = *m_MVCamera.GetWorldMatrix();
        mWorld = m_mCenterMesh * world;
    }
    else
    {
        // First person camera
        mWorld = m_mCenterMesh;
    }

    // Get the projection and view matricies
    mProj = *m_pCamera->GetProjMatrix();
    mView = *m_pCamera->GetViewMatrix();

    // And compute the world view projection matrix
    mWorldViewProjection = mWorld * mView * mProj;

    // Render the sky
    m_pSky->RenderSky(mWorldViewProjection);

    // For the collada
    D3DXMATRIX  mViewWorld;
    D3DXMATRIX  mRotationX;
    D3DXMATRIX  mRotationY;
    D3DXMATRIX  mRotationZ;
    D3DXMATRIX  mTranslation;
    D3DXMATRIX  mScale;

    // Boat model operations
    mViewWorld = mWorld*mView;
    mViewWorld = m_boatTransform * mViewWorld;
    m_ColladaRender->Render(&mViewWorld, &mProj);

    mViewWorld = mWorld*mView;
    
    // Rotate the scene
    D3DXMatrixRotationY(&mRotationY, (3.1415926f/2.0f)*1.0f);
    
    // Scene offset setup
    if(m_WaterSimulationParams.sceneNumber==1 || m_WaterSimulationParams.sceneNumber==3 || m_WaterSimulationParams.sceneNumber==5)
    {
        //river scene offset
        g_dy = -5.2f;
        g_dz = 100.0f;
    }
    else
    {
        //default (sea) scene offset 
        g_dy=COLLADA_SCENE_OFFSET_Y;
        g_dz=COLLADA_SCENE_OFFSET_Z;
    }
    // Create a matrix used to translate the scene
    D3DXMatrixTranslation(&mTranslation, g_dx, g_dy, g_dz);
    
    // And one to scale the scene
    D3DXMatrixScaling(&mScale, DEFAULT_MATRIX_SCALING, DEFAULT_MATRIX_SCALING, DEFAULT_MATRIX_SCALING);

    mViewWorld = mRotationY*mViewWorld;

    // Translate and scale the world view
    mViewWorld = mTranslation*mViewWorld;
    mViewWorld = mScale*mViewWorld;


    D3DXVECTOR3 eye;
    {
        // Calculate the eye position for the renderer
        D3DXMATRIX m;
        if (m_pCamera == &m_MVCamera)
        {
            D3DXMATRIX const &matView = *m_MVCamera.GetViewMatrix();
            D3DXMATRIX const &matWorld = *m_MVCamera.GetWorldMatrix();
            D3DXMATRIX mWorldView = matWorld * matView;
            D3DXMatrixInverse( &m, NULL, &mWorldView );
        }
        else
        {
            //  We use a first person camera
            D3DXMatrixInverse( &m, NULL, m_pCamera->GetViewMatrix() );
        }

        eye.x = m._41 - m_mCenterMesh._41;
        eye.y = m._42 - m_mCenterMesh._42;
        eye.z = m._43 - m_mCenterMesh._43;
    }

    // Render the scene
    m_pSceneVis->Render(mWorldViewProjection, eye);

    // Render the Collada scene
    m_ColladaRenderStatic->Render(&mViewWorld, &mProj);

} // End of CFluidSimHost::OnRenderScene

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
void CFluidSimHost::OnRenderText( CDXUTTextHelper* in_pTxtHelper,
    double fTime,
    float fElapsedTime )
{
    // Call the base class to render common text elements
    __super::OnRenderText(in_pTxtHelper, fTime, fElapsedTime);

    // Set up the helper and draw the help message
    in_pTxtHelper->Begin();
    in_pTxtHelper->SetInsertionPos( 2, 0 );
    in_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    in_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );  
    in_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );    
    in_pTxtHelper->DrawTextLine( GetDrawHelp() ? L"Hide help: F1" : L"Show help: F1" );

    in_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    {
        // Print the FPS 
        WCHAR str[256];
        StringCchPrintf(str, 256, L"FPS: %5.2f", DXUTGetFPS());
        in_pTxtHelper->DrawFormattedTextLine( str );

        // Print the solver FPS 
        StringCchPrintf(str, 256, L"Solver FPS: %5.2f", m_currentFPS);
        in_pTxtHelper->DrawFormattedTextLine( str );

        // Print OCL device 
        WCHAR*  out = new WCHAR[128];
        size_t convertedChars = 0;
        mbstowcs_s(&convertedChars, out, 128, &g_device_name[0], _TRUNCATE);
        StringCchPrintf(str, 256, L"OpenCL device: %s", out);
        in_pTxtHelper->DrawFormattedTextLine( str );
        delete out;

        // Print the time
        ///StringCchPrintf(str, 100, L"Time: %f", m_pScene->GetTime());
        ///in_pTxtHelper->DrawFormattedTextLine( str );
    }

    UINT height = DXUTGetDXGIBackBufferSurfaceDesc()->Height;
    // If the user has asked to see the help message
    if( GetDrawHelp() )
    {
        // then show the help messages
        in_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        in_pTxtHelper->SetInsertionPos(2, height - 15 * 8);
        in_pTxtHelper->DrawTextLine(L"Controls:");
        in_pTxtHelper->SetInsertionPos(20, height - 15 * 7);
        in_pTxtHelper->DrawTextLine( L"Rotate camera: Left mouse button\n"
            L"Move camera in Examine mode: mouse wheel\n"
            L"Move camera in WASD mode: W, A, S, D, Q, E\n"
            L"Start/stop simulation: Space \n"
            L"Change boat direction: Right mouse button \n"
            L"Quit: ESC\n");
    }

    in_pTxtHelper->End();

}// End of OnRenderText

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
void CFluidSimHost::SerializeBoatModel( const unsigned int in_verticesCount, 
    const float3* in_pVertices, 
    const unsigned int in_indicesCount, 
    const void* in_pIndices, 
    DynamicArray<char>* out_pData2Send )
{
    // Create a package to send to OCL:
    // package start-number of vertices- + --vertices array-- + -type of indices- + --number of indices-- + --indices array--  

    // Validate the output buffer array exists
    assert(out_pData2Send);

    // Initialize the output buffer array
    out_pData2Send->SetCapacity(sizeof(unsigned int) + sizeof(float3)*in_verticesCount + sizeof(collada::ScalarType) + sizeof(unsigned int) + sizeof(unsigned int)*in_indicesCount);
    out_pData2Send->AppendArray(sizeof(int),(char*)&in_verticesCount);
    out_pData2Send->AppendArray(sizeof(float3)*in_verticesCount,(char*)in_pVertices);
	int ST_UINT32_type = 3;
    out_pData2Send->AppendArray(sizeof(ST_UINT32_type),(char*)&ST_UINT32_type);
    out_pData2Send->AppendArray(sizeof(unsigned int),(char*)&in_indicesCount);
    
    // Append the correct array based on the index type
    out_pData2Send->AppendArray(sizeof(unsigned int)*in_indicesCount,(char*)in_pIndices);
}// End of CFluidSimHost::SerializeBoatModel

/**
 * @fn GetNewTransformationMatrix()
 *
 * @brief This method gets a transformation matrix for the boat during the animation state from a collada file
 *
 * @param in_pScene - The collada scene with animation
 *
 * @param time - The time given to generate the animation matrix
 *
 * @return - The new transformation matrix
 */
const float* CFluidSimHost::GetNewTransformationMatrix(collada::Scene* in_pScene, float time)
{
    // Get the clip count
    unsigned int animCount = (unsigned int)in_pScene->GetAnimationClipCount();
    // For every clip in the clip count set a time pointer based on the time parameter
    for(unsigned int i = 0; i<animCount; i++)
    {
        collada::AnimationClip* pAnimClip = in_pScene -> GetAnimationClip(i);
        float animationLength = pAnimClip -> GetLength();
        pAnimClip -> SetTimePointer(time - ((int)(time / animationLength)) * animationLength);
    }
//removed me
//    m_pBoatMeshInstanceForAnimation->FlushInternalState();

    return m_pBoatMeshInstanceForAnimation->GetNode(in_pScene)->GetWorld(in_pScene);
}// CFluidSimHost::GetNewTransformationMatrix

/**
 * @fn GetHybridMethodData()
 *
 * @brief This method gets the hybrid method as boat data from the OCL side. It gets
 *        the boat position and vertice parameters. This
 *        function also reallocates buffers for vertices if needed
 */
int GetHybridMethodDataHelper(void* in_pData);
void CFluidSimHost::GetHybridMethodData()
{
    void*           tmp_pointer = NULL;


    // Update the boat position if needed
    ///m_NativeGetHybridMethodDataFunction.Run(bufs);
    GetHybridMethodDataHelper(m_pTransformBuffer);


    // If the boat was enabled, we need to get the data back from OCL
    if(m_WaterSimulationParams.enableBoat)
    {

        // Map the buffer
        tmp_pointer = m_pTransformBuffer;

        // If we have a boat transformation and a mapped buffer, then copy the data
        if(m_boatTransform && tmp_pointer)
        {
            memcpy(m_boatTransform, tmp_pointer, 16*sizeof(float));
            memcpy(&m_isAnimationFreezed, &((float*)tmp_pointer)[16], sizeof(bool));
        }

        // Workaround: invert x-axes
        D3DXMATRIX tmpInvertX;
        D3DXMatrixScaling(&tmpInvertX, -1, 1, 1);
        D3DXMatrixMultiply(&m_boatTransform, &tmpInvertX, &m_boatTransform);


    }// End of if(m_WaterSimulationParams.enableBoat)

}// END of GetHybridMethodData


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
void CFluidSimHost::AddSlider( int& yUIOffset, 
    float* pVal, 
    float MinVal, 
    float MaxVal, 
    WCHAR* pStrToPrint, 
    bool bEnabled )
{
    CDXUTDialog*    pUI = GetSampleUI();
    DefSliderData*  pData = NULL;
    int             NewID = IDC_LAST;

    // If the object m_Sliders has been initialized
    if(m_Sliders.Size()>0)
    {
        // The new ID is the ID of the last entry + 2 (we will be adding a label as well)
        NewID = m_Sliders[m_Sliders.Size()-1].id + 2;
    }
    // Append a new slider to the structure and initialize the ID, string, current value and min/max values
    pData = &m_Sliders.Append();
    pData->id = NewID;
    pData->pStrToPrint = pStrToPrint;
    pData->pVal = pVal;
    pData->MinVal = MinVal;
    pData->MaxVal = MaxVal;

    WCHAR sz[100];
    swprintf_s( sz, 100, pData->pStrToPrint, pData->pVal[0]); 
    // Add the label
    pUI->AddStatic(NewID, sz, STATIC_OFFSET, yUIOffset += CONTROLS_OFFSET, STATIC_WIDTH, CONTROLS_HEIGHT);
    // Add the slider
    pUI->AddSlider(NewID+1, SLIDER_OFFSET, yUIOffset += CONTROLS_OFFSET, SLIDER_WIDTH, CONTROLS_HEIGHT, 0, 100, (int)(100.0f*(pVal[0]-MinVal)/(MaxVal-MinVal)));
    // Enable the slider
    pUI->GetSlider( NewID+1 )->SetEnabled( bEnabled );
} // End of AddSlider


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
void CFluidSimHost::UpdateSlider(CDXUTDialog *pUI, int IdForUpdate)
{
    int i,size = m_Sliders.Size();
    for(i=0;i<size;++i)
    {
        DefSliderData* pData = &m_Sliders[i];
        if(pData->id==IdForUpdate || pData->id+1==IdForUpdate)
        {
            // we found the updated slider, set its label to the new value
            WCHAR sz[100];
            int sliderValue = pUI->GetSlider(pData->id+1)->GetValue();
            pData->pVal[0] = pData->MinVal + (pData->MaxVal-pData->MinVal) * float(sliderValue) * 0.01f;
            swprintf_s( sz, 100, pData->pStrToPrint, pData->pVal[0]); 
            pUI->GetStatic(pData->id)->SetText( sz );
            break;
        }
    }
} // End of CFluidSimHost::UpdateSlider



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
#define SMALL_EPSILON 0.0001f
bool CFluidSimHost::IntersectWithSurface(D3DXVECTOR3 const &rRayPos_, D3DXVECTOR3 const &rRayDir, float &rX, float &rY)
{
    int const _numStartIterations = 50;
    D3DXVECTOR2     rSceneCenter(m_mCenterMesh._41,m_mCenterMesh._43);
    CFlux2DScene&  rScene = *m_pScene;
    D3DXVECTOR3 rayPos(rRayPos_);
    D3DXVECTOR2 size((rScene.GetRegion().x2 - rScene.GetRegion().x1), (rScene.GetRegion().y2 - rScene.GetRegion().y1));
    D3DXVECTOR2 minBound = rSceneCenter;
    D3DXVECTOR2 maxBound = rSceneCenter + size;

    // Fit the starting point into the grid (x component)
    if (rRayDir.x > SMALL_EPSILON)
    {
        if (rayPos.x > maxBound.x)
        {
            return false;
        }
        if (rayPos.x < minBound.x)
        {
            float coeff = (minBound.x - rayPos.x) / rRayDir.x;
            rayPos.x = minBound.x;
            rayPos.y += rRayDir.y * coeff;
            rayPos.z += rRayDir.z * coeff;
        }
    } 
    else if (rRayDir.x < -SMALL_EPSILON)
    {
        if (rayPos.x < minBound.x)
        {
            return false;
        }
        if (rayPos.x > maxBound.x)
        {
            float coeff = (maxBound.x - rayPos.x) / rRayDir.x;
            rayPos.x = maxBound.x;
            rayPos.y += rRayDir.y * coeff;
            rayPos.z += rRayDir.z * coeff;
        }
    }

    // Fit the starting point into the grid (y component)
    if (rRayDir.z > SMALL_EPSILON)
    {
        if (rayPos.z > maxBound.y)
        {
            return false;
        }
        if (rayPos.z < minBound.y)
        {
            float coeff = (minBound.y - rayPos.z) / rRayDir.z;
            rayPos.x += rRayDir.x * coeff;
            rayPos.y += rRayDir.y * coeff;
            rayPos.z = minBound.y;
        }
    } 
    else if (rRayDir.z < -SMALL_EPSILON)
    {
        if (rayPos.z < minBound.y)
        {
            return false;
        }
        if (rayPos.z > maxBound.y)
        {
            float coeff = (maxBound.y - rayPos.z) / rRayDir.z;
            rayPos.x += rRayDir.x * coeff;
            rayPos.y += rRayDir.y * coeff;
            rayPos.z = maxBound.y;
        }
    }

    // Find the minimum T value (x)
    float t = 0.0f;
    float minT = FLT_MAX;
    if (rRayDir.x > SMALL_EPSILON)
    {
        t = (maxBound.x - rayPos.x) / rRayDir.x;
        if (t < minT)
        {
            minT = t;
        }
    } 
    else if (rRayDir.x < -SMALL_EPSILON)
    {
        t = -(rayPos.x - minBound.x) / rRayDir.x;
        if (t < minT)
        {
            minT = t;
        }
    }

    // Find the minimum T value (y)
    if (rRayDir.z > SMALL_EPSILON)
    {
        t = (maxBound.y - rayPos.z) / rRayDir.z;
        if (t < minT)
        {
            minT = t;
        }
    } 
    else if (rRayDir.z < -SMALL_EPSILON)
    {
        t = -(rayPos.z - minBound.y) / rRayDir.z;
        if (t < minT)
        {
            minT = t;
        }
    }

    // Shift the ray starting position to get the scene coordinate system
    rayPos.x -= rSceneCenter.x;
    rayPos.z -= rSceneCenter.y;

    float   tStep = minT / (float)_numStartIterations;
    float   func=0;     // height of water on ray on prev point
    float   funcPrev;   // height of water on ray on prev point
    int     i = 0;

    // Calculate the difference in the ray starting positions
    funcPrev = rayPos.y - rScene.GetSurfaceHeight(rayPos.x, rayPos.z);
    for (i = 0, t = tStep; i < _numStartIterations; i++, t += tStep)
    {
        // Trace the ray until it intersects with water surface
        D3DXVECTOR3 pos = rayPos + rRayDir * t;
        func = pos.y - rScene.GetSurfaceHeight(pos.x, pos.z);
        if (funcPrev * func < 0.0f) break; // we found an intersection, finish ray tracing.
        funcPrev = func;
    }

    // If no intersection was found, simply return FALSE
    if (i == _numStartIterations)return false;

    // Correct the position on the ray
    t -= tStep * (1.0f + funcPrev / (func - funcPrev));
    
    // Calculate the (x,y) coordinate on scene
    rX = rayPos.x + rRayDir.x * t;
    rY = rayPos.z + rRayDir.z * t;

    // return true if we get a grid or false if we are out of scenes
    return rScene.FindGrid(rX,rY)?true:false;
}// End if CFluidSimHost::IntersectWithSurface
