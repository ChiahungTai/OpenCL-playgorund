/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DMethod.h
 *
 * @brief Defines methods for parallel tasks
 */

#pragma once

#include "Flux/CFlux2DScene.h"
#include "physics_3D_fluid_sim_common.h"
//OpenCL
#include "utils.h"

#include <Windows.h>

//For OpenMP parallel solvers
#include <omp.h>


extern char  g_device_name[128];


/**
 * @class CFLux2DMethod
 *
 * @brief A virtual class that defines an interface for calculations 
 */
class CFlux2DMethod
{
public:

    /**
     * @fn ~CFlux2DMethod()
     *
     * @brief Class destructor
     *
     * @note Declared as a virtual destructor 
     */
    virtual ~CFlux2DMethod()
    {
    }

    /**
     * @fn void Step(float dt)
     *
     * @brief Makes one iteration step
     *
     * @param dt - The time interval for the simulation 
     */
    virtual void Step(float dt) = 0;
};


/**
 * @fn CalcJobRange(int task, int taskNum, int jobNum, int* pJobStart, int* pJobEnd)
 *
 * @brief Calculates the start and stop indexes for a given job index 
 *
 * @param task - The index of the task for which the job range is calculated
 *
 * @param taskNum - The total number of tasks that could do all jobs
 *
 * @param jobNum - The total number of jobs
 *
 * @param pJobStart - Output index of the first job that the given task has to do
 *
 * @param pJobend - Output index of the last job that the given task has to do
 */
inline void CalcJobRange(int task, int taskNum, int jobNum, int* pJobStart, int* pJobEnd)
{
    int step = jobNum/taskNum;
    int rest = jobNum-taskNum*step;
    int j0,j1;

    if(task<rest)
    {
        /* big job basket */
        j0 = task*(step+1);
        j1 = j0+step;
    }
    else
    {
        /* small job basket */
        j0 = task*step+rest;
        j1 = j0+step-1;
    }
    if (j0<jobNum && j1<jobNum)
    {
        pJobStart[0] = j0;
        pJobEnd[0] = j1;
    } 
    else
    {
        pJobStart[0] = -1;
        pJobEnd[0] = -2;
    }
}/* CalcJobRange */

/**
 * @class CFlux2DMethodOpenMP
 *
 * @brief OpenMP parallelized calculator (physics solver) 
 */
template <class CALCULATOR>
class CFlux2DMethodOpenMP : 
    public CFlux2DMethod
{
private:

    /**
     * @var m_Calculators
     *
     * @brief An array of calculators, one for each thread
     */
    DynamicArray<CALCULATOR*>   m_Calculators;  

    /**
     * @var m_numThreads
     *
     * @brief The total number of threads in the system
     */
    int                         m_numThreads;

    /**
     * @var m_pScene
     *
     * @brief A pointer to the scene that will be processed
     */
    CFlux2DScene*              m_pScene;

    /**
     * @var m_Params
     *
     * @brief A local copy of the parameters for the current simulation step
     */
    CFlux2DCalculatorPars      m_Params;

public:
    /**
     * @fn CFlux2DMethodOpenMP(CFlux2DScene &scene, int in_numThreads)
     *
     * @brief Class constructor, initializes the calculator 
     *
     * @param scene - reference to scene that will be processed
     *
     * @param in_numThreads - number of threads that will be used for parallelization 
     */
    CFlux2DMethodOpenMP(CFlux2DScene &scene, int in_numThreads)
    {
        int i;
        // Store parameters
        m_pScene = &scene;
        if(in_numThreads<=0)
        {
            SYSTEM_INFO SystemInfo;
            GetSystemInfo(&SystemInfo);
            in_numThreads = SystemInfo.dwNumberOfProcessors;
            //in_numThreads = GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS);
            //in_numThreads = 8;
        }
        m_numThreads = in_numThreads;
        printf("Using %d number of threads for OpenMP according to GetSystemInfo...\n", m_numThreads);

        // Set the calculator capacity
        m_Calculators.SetCapacity(in_numThreads);
        // For each thread
        for (i = 0; i < in_numThreads; i++)
        {
            // Create a calculator for each threads and initialize for the scene to be processed
            CALCULATOR* pNewCalc = new(UseOwnMem) CALCULATOR;
            pNewCalc->InitForScene(&scene);
            // Append the new calculator to the calculator list
            m_Calculators.Append(pNewCalc);
        }
    }

    /**
     * @fn ~CFlux2DMethodOpenMP()
     *
     * @brief Destructor of parallel calculator, destroys all calculators
     */
    virtual ~CFlux2DMethodOpenMP()
    {
        int i;
        for (i = 0; i < m_numThreads; i++)
        {
            DestroyObject<CALCULATOR>(m_Calculators[i]);
        }
    }

    /**
     * @fn void Step(float dt)
     *
     * @brief Makes one simulation step with dt time
     *
     * @param dt - The time step
     */
    virtual void Step(float dt)
    {
        int i;
        int const numGrids = m_pScene->GetNumGrids();

        // Get the current simulation time
        m_Params.time = m_pScene->GetTime();
        // tau is delta time
        m_Params.tau = dt;

        // For each thread
        for (i = 0; i < m_numThreads; i++)
        {
            // Initialize the calculators
            m_Calculators[i]->Init(m_Params, m_pScene->GetGlobalPars());
        }

        // First thread call: the main job



#pragma omp parallel for schedule(static,1)
        for (int iii = 0; iii < m_numThreads; iii++)
        {
           TaskFunction(this, iii, m_numThreads);
        }


        // Apply actions for all grids
        for (i = 0; i < numGrids; i++)
        {
            m_pScene->GetGrid(i)->RunActions(m_Params,m_pScene->GetGlobalPars());
        }
        // Prepare the scene for the next step
        m_pScene->Proceed2NextStep(dt); 

    }// End of Step()

    /**
     * @fn TaskFunction(void* in_pData, const uint32_t in_taskIndex, const uint32_t in_tasksCount)
     *
     * #brief The Task function to run in parallel
     *
     * @param in_pData - Pointer to an object of this class
     *
     * @param in_taskIndex - The index of the current executing task 
     *
     * @param in_tasksCount - The total number of executed tasks
     */
    static void TaskFunction(void* in_pData, const uint32_t in_taskIndex, const uint32_t in_tasksCount)
    {
        CFlux2DMethodOpenMP* pThis = (CFlux2DMethodOpenMP*)(in_pData);
        int const numGrids = pThis->m_pScene->GetNumGrids();

        // For every grid in the scene
        for (int i = 0; i < numGrids; i++)
        {
            CFlux2DGrid*           pG = pThis->m_pScene->GetGrid(i);
            int                     y0,y1;
            // Calculate the strip for this task-thread */
            CalcJobRange(in_taskIndex,in_tasksCount,pG->GetDomainH(),&y0,&y1);
            pThis->m_Calculators[in_taskIndex]->ProcessTile(pG, 0,y0, pG->GetDomainW(), y1-y0+1);
        }

    }
};


class CFlux2DMethodOpenCLScalar : 
    public CFlux2DMethod
{
private:


    /**
     * @var m_pScene
     *
     * @brief A pointer to the scene that will be processed
     */
    CFlux2DScene*              m_pScene;

    /**
     * @var m_Params
     *
     * @brief A local copy of the parameters for the current simulation step
     */
    CFlux2DCalculatorPars      m_Params;

    //OpenCL
    /**
     * @var m_OCL_data
     *
     * @brief A data structure to pass parameters to kernel
     */
    typedef struct 
    {
        float RcpGridStepW;
        float RcpGridStepH;
        CFlux2DCalculatorPars m_CalcParams; 

        int SpeedCacheMatrixOffset;
        int SpeedCacheMatrixWidthStep;

        int HeightMapOffset;
        int HeightMapWidthStep;
        int UMapOffset;
        int UMapWidthStep;
        int VMapOffset;
        int VMapWidthStep;

        int HeightMapOffsetOut;
        int HeightMapWidthStepOut;
        int UMapOffsetOut;
        int UMapWidthStepOut;
        int VMapOffsetOut;
        int VMapWidthStepOut;

        int HeightMapBottomOffset;
        int HeightMapBottomWidthStep;

        int PrecompDataMapBottomOffset;
        int PrecompDataMapBottomWidthStep;

        float        Hmin;                          // The minimal allowed water bar height
        float        gravity;                       // A local copy of gravity value
        float        halfGravity;                   // The gravity multiplied by 0.5
        float        tau_div_deltaw;                // The time interval divided by grid cell width
        float        tau_div_deltah;                // The time interval divided by grid cell height
        float        tau_div_deltaw_quarter;        // A quarter of the time interval divided by the grid cell width
        float        tau_div_deltah_quarter;        // A quarter of the time interval divided by the grid cell height
        float        tau_mul_Cf;                    // The time interval multiplied by friction coefficient
        float        minNonZeroSpeed;               // The minimal value of water bar speed

    } CKernelData;


    __declspec(align(128)) CKernelData m_OCL_data;

    /**
     * @var m_context
     *
     * @brief A OpenCL context
     */
    cl_context	m_context;

    /**
     * @var m_cmd_queue
     *
     * @brief A OpenCL command queue
     */
    cl_command_queue m_cmd_queue;

    /**
     * @var m_program
     *
     * @brief A OpenCL program
     */
    cl_program	m_program;

    /**
     * @var m_kernel
     *
     * @brief A OpenCL kernel implementing shallow water solver
     */
    cl_kernel	m_kernel;

    /**
     * @var m_global_work_size
     *
     * @brief A OpenCL global work size
     */
    size_t m_global_work_size[1];

    /**
     * @var m_local_work_size
     *
     * @brief A OpenCL local work size
     */
    size_t m_local_work_size[1];


    //speed cache matrix
    DynamicMatrix<float> m_SpeedCacheMatrix;    // A cyclic buffer to keep the wave speed for the 3 last lines

public:
    /**
    * @fn RebuildOCLKernel(bool in_bRelaxedMath)
    *
    * @brief Rebuilds OpenCL kernel for OpenCL calculator
    *
    * @param  bOCLRelaxedMath - Relaxed math build option true or false
	*
	* @param  bRunOnGPU - Disable optimizations on GPU option true or false
    */
    void RebuildOCLKernel(bool bOCLRelaxedMath, bool bRunOnGPU)
    {
        cl_int err;
        //build with relaxed math
        //release previous version
        clFlush(m_cmd_queue);
        clFinish(m_cmd_queue);
        if( m_kernel ) 
        {
            clReleaseKernel( m_kernel );
            m_kernel = 0;
        }
        if( m_program ) 
        {
            clReleaseProgram( m_program );
            m_program = 0;
        }

        //create new version
        char *sources = ReadSources("ShallowWater.cl");	//read program .cl source file
        m_program = clCreateProgramWithSource(m_context, 1, (const char**)&sources, NULL, NULL);
        if (m_program == (cl_program)0)
        {
            printf("ERROR: Failed to create Program with source...\n");
            CleanupOpenCL();
            free(sources);
            return;
        }

        if(bOCLRelaxedMath && !bRunOnGPU)
        {
            const char buildOpts[] = "-cl-denorms-are-zero -cl-fast-relaxed-math";

            err = clBuildProgram(m_program, 0, NULL, buildOpts, NULL, NULL);
        }
        else
        {
            err = clBuildProgram(m_program, 0, NULL, NULL, NULL, NULL);
        }


        if (err != CL_SUCCESS)
        {
            printf("ERROR: Failed to build program...\n");
            BuildFailLog(m_program, 0);
            CleanupOpenCL();
            free(sources);
            return;
        }

        m_kernel = clCreateKernel(m_program, "ProcessTile", NULL);
        if (m_kernel == (cl_kernel)0)
        {
            printf("ERROR: Failed to create kernel...\n");
            CleanupOpenCL();
            free(sources);
            return;
        }

        free(sources);
    }


    /**
    * @fn CleanupOpenCL()
    *
    * @brief Cleanups OpenCL kernel, program, queue and context
    *
    */
    void CleanupOpenCL()
    {
        clFlush(m_cmd_queue);
        if( m_kernel ) 
        {
            clReleaseKernel( m_kernel );
            m_kernel = 0;
        }

        if( m_program ) 
        {
            clReleaseProgram( m_program );
            m_program = 0;
        }

        if( m_cmd_queue )
        {
            clReleaseCommandQueue( m_cmd_queue );
            m_cmd_queue = 0;
        }

        if( m_context ) 
        {
            clReleaseContext( m_context );
            m_context = 0;
        }
    }

    /**
     * @fn CFlux2DMethodOpenCLScalar(CFlux2DScene &scene, int in_numThreads)
     *
     * @brief Class constructor, initializes the calculator 
     *
     * @param scene - reference to scene that will be processed
     *
     * @param in_numThreads - number of threads that will be used for parallelization 
     */
    CFlux2DMethodOpenCLScalar(CFlux2DScene &scene, int in_numThreads, bool device_type_gpu)
    {
        int i;
        // Store parameters
        m_pScene = &scene;
        

        //OpenCL
        ///CleanupOpenCL();
        m_context = 0;
        m_cmd_queue = 0;
    	m_program = 0;
    	m_kernel = 0;


        cl_device_id devices[16];
        size_t cb;
        cl_int error;

        cl_platform_id intel_platform_id = GetIntelOCLPlatform();
        if( intel_platform_id == NULL )
        {
            printf("ERROR: Failed to find Intel OpenCL platform.\n");
            return;
        }

        cl_context_properties context_properties[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)intel_platform_id, NULL };

        // create the OpenCL context on a CPU device
		if(device_type_gpu)
		{
			m_context = clCreateContextFromType(context_properties, CL_DEVICE_TYPE_GPU, NULL, NULL, NULL);
		}
		else
		{
			m_context = clCreateContextFromType(context_properties, CL_DEVICE_TYPE_CPU, NULL, NULL, NULL);
		}
		if (m_context == (cl_context)0)
            return;

        // get the list of CPU devices associated with context
        error = clGetContextInfo(m_context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
        clGetContextInfo(m_context, CL_CONTEXT_DEVICES, cb, devices, NULL);

        m_cmd_queue = clCreateCommandQueue(m_context, devices[0], 0, NULL);
        if (m_cmd_queue == (cl_command_queue)0)
        {
            CleanupOpenCL();
            return;
        }

        char *sources = ReadSources("ShallowWater.cl");	//read program .cl source file
        m_program = clCreateProgramWithSource(m_context, 1, (const char**)&sources, NULL, NULL);
        if (m_program == (cl_program)0)
        {
            printf("ERROR: Failed to create Program with source...\n");
            CleanupOpenCL();
            free(sources);
            return;
        }

        //const char buildOpts[] = "-cl-denorms-are-zero "
        //    "-cl-fast-relaxed-math "
        //    ;

        error = clBuildProgram(m_program, 0, NULL, NULL, NULL, NULL);
        ///error = clBuildProgram(m_program, 0, NULL, buildOpts, NULL, NULL);
        if (error != CL_SUCCESS)
        {
            printf("ERROR: Failed to build program...\n");
            BuildFailLog(m_program, devices[0]);
            CleanupOpenCL();
            free(sources);
            return;
        }

        m_kernel = clCreateKernel(m_program, "ProcessTile", NULL);
        if (m_kernel == (cl_kernel)0)
        {
            printf("ERROR: Failed to create kernel...\n");
            CleanupOpenCL();
            free(sources);
            return;
        }
        free(sources);

        ///////////////////////////////////////////////////////////////////////////////////////////////

        //create speed cache and define global work size
        if(in_numThreads<=0)
        {
            // Query for the number of context devices
            if((error = clGetContextInfo(m_context, CL_CONTEXT_DEVICES, 0, NULL, &cb)) != CL_SUCCESS)	
            {
                printf("ERROR: Failed to clGetContextInfo for CL_CONTEXT_DEVICES...\n");
                CleanupOpenCL();
                return;
            }

            // Allocate memory for a device list based on the size returned by the query
            cl_device_id* devices = new cl_device_id[cb];
            if (!devices)	
            {
                printf("ERROR: Failed to create devices buffer...\n");
                CleanupOpenCL();
                return;
            }

            // Retrieve device information
            if((error = clGetContextInfo(m_context, CL_CONTEXT_DEVICES, cb, devices, NULL)) != CL_SUCCESS)	
            {
                printf("ERROR: Failed to clGetContextInfo for CL_CONTEXT_DEVICES...\n");
                CleanupOpenCL();
                return;
            }

            //First device ID
            cl_device_id device_ID = devices[0];
            delete[] devices;			//Don't need the list anymore

            ///char device_name[128] = {0};
            error = clGetDeviceInfo(device_ID, CL_DEVICE_NAME, 128, g_device_name, NULL);
            if (error!=CL_SUCCESS)
            {
                printf("ERROR: Failed to clGetContextInfo for CL_CONTEXT_DEVICES...\n");
                CleanupOpenCL();
                return;
            }
            printf("Using device %s...\n", g_device_name);
			

            int num_cores;
            error = clGetDeviceInfo(device_ID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &num_cores, NULL);
            if (error!=CL_SUCCESS)
            {
                return;
            }
            printf("Using %d compute units according to clGetDeviceInfo...\n", num_cores);
            in_numThreads  = num_cores;

            int min_align;
            error = clGetDeviceInfo(device_ID, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &min_align, NULL);
            if (error!=CL_SUCCESS)
            {
                return;
            }
            min_align /= 8; //in bytes
            printf("Expected min alignment for buffers is %d bytes...\n", min_align);

        }
                
        m_global_work_size[0] = 16*in_numThreads;
        m_local_work_size[0] = 16;

        int maxSize = 0;
        int num = m_pScene->GetNumGrids();
        for(i=0;i<num;++i)
        {/** calc maximal width of grid */
            maxSize = max(maxSize,m_pScene->GetGrid(i)->GetDomainW());
        }
        /** set size for cycle buffer */
        m_SpeedCacheMatrix.SetSize(maxSize,3*(int)m_global_work_size[0],1);

        //global params init
        m_OCL_data.Hmin = m_pScene->GetGlobalPars().Hmin;
        m_OCL_data.gravity = m_pScene->GetGlobalPars().gravity;
        m_OCL_data.halfGravity = 0.5f * m_pScene->GetGlobalPars().gravity;
        m_OCL_data.tau_div_deltaw = 0; //calculated in kernel 
        m_OCL_data.tau_div_deltah = 0; //calculated in kernel 
        m_OCL_data.tau_div_deltaw_quarter = 0; //calculated in kernel 
        m_OCL_data.tau_div_deltah_quarter = 0; //calculated in kernel 
        m_OCL_data.tau_mul_Cf = m_OCL_data.m_CalcParams.tau * m_pScene->GetGlobalPars().Cf; //recalculate before kernel call for current dt
        m_OCL_data.minNonZeroSpeed = m_pScene->GetGlobalPars().minNonZeroSpeed;

    }

    /**
     * @fn ~CFlux2DMethodOpenCLScalar()
     *
     * @brief Destructor of parallel calculator, destroys all calculators
     */
    virtual ~CFlux2DMethodOpenCLScalar()
    {
        CleanupOpenCL();
    }

    /**
     * @fn void Step(float dt)
     *
     * @brief Makes one simulation step with dt time
     *
     * @param dt - The time step
     */
    virtual void Step(float dt)
    {
        int i;
        int const numGrids = m_pScene->GetNumGrids();

        // Get the current simulation time
        m_Params.time = m_pScene->GetTime();
        // tau is delta time
        m_Params.tau = dt;

        for (i = 0; i < numGrids; i++)
        {
            //Init data struct

            m_OCL_data.RcpGridStepW = m_pScene->GetGrid(i)->GetRcpGridStepW();
            m_OCL_data.RcpGridStepH = m_pScene->GetGrid(i)->GetRcpGridStepH();
            m_OCL_data.m_CalcParams.tau = dt; 
            m_OCL_data.m_CalcParams.time = m_pScene->GetTime(); 

            m_OCL_data.SpeedCacheMatrixOffset = m_SpeedCacheMatrix.m_Offset; 
            m_OCL_data.SpeedCacheMatrixWidthStep = m_SpeedCacheMatrix.m_WidthStep;  

            //grid specific params (calculate for each grid in the grid loop before kernel call)
            m_OCL_data.HeightMapOffset = m_pScene->GetGrid(i)->GetCurrentSurface().m_heightMap.m_Offset;
            m_OCL_data.HeightMapWidthStep = m_pScene->GetGrid(i)->GetCurrentSurface().m_heightMap.m_WidthStep;
            m_OCL_data.UMapOffset = m_pScene->GetGrid(i)->GetCurrentSurface().m_uMap.m_Offset;
            m_OCL_data.UMapWidthStep = m_pScene->GetGrid(i)->GetCurrentSurface().m_uMap.m_WidthStep;
            m_OCL_data.VMapOffset = m_pScene->GetGrid(i)->GetCurrentSurface().m_vMap.m_Offset;
            m_OCL_data.VMapWidthStep = m_pScene->GetGrid(i)->GetCurrentSurface().m_vMap.m_WidthStep;

            m_OCL_data.HeightMapOffsetOut = m_pScene->GetGrid(i)->GetDestSurface().m_heightMap.m_Offset;
            m_OCL_data.HeightMapWidthStepOut = m_pScene->GetGrid(i)->GetDestSurface().m_heightMap.m_WidthStep;
            m_OCL_data.UMapOffsetOut = m_pScene->GetGrid(i)->GetDestSurface().m_uMap.m_Offset;
            m_OCL_data.UMapWidthStepOut = m_pScene->GetGrid(i)->GetDestSurface().m_uMap.m_WidthStep;
            m_OCL_data.VMapOffsetOut = m_pScene->GetGrid(i)->GetDestSurface().m_vMap.m_Offset;
            m_OCL_data.VMapWidthStepOut = m_pScene->GetGrid(i)->GetDestSurface().m_vMap.m_WidthStep;

            m_OCL_data.HeightMapBottomOffset = m_pScene->GetGrid(i)->GetBottom().m_heightMap.m_Offset;
            m_OCL_data.HeightMapBottomWidthStep = m_pScene->GetGrid(i)->GetBottom().m_heightMap.m_WidthStep;

            m_OCL_data.PrecompDataMapBottomOffset = m_pScene->GetGrid(i)->GetBottom().m_precompDataMap.m_Offset;
            m_OCL_data.PrecompDataMapBottomWidthStep = m_pScene->GetGrid(i)->GetBottom().m_precompDataMap.m_WidthStep;

            //global params init
            m_OCL_data.tau_mul_Cf = m_OCL_data.m_CalcParams.tau * m_pScene->GetGlobalPars().Cf; //recalculate before kernel call for current dt

            //Create OpenCL buffers
            cl_mem SpeedCacheMatrix;
            cl_mem heightMap; 
            cl_mem uMap;
            cl_mem vMap;
            cl_mem heightMapOut; 
            cl_mem uMapOut;
            cl_mem vMapOut;
            cl_mem heightMapBottom;
            cl_mem precompDataMapBottom;
            cl_mem OCLData; 

            cl_int errorcodeRet;

            //We use CL_MEM_USE_HOST_PTR to avoid buffer allocation inside Step function. As well clEnqueueReadBuffer is not required in such case.
            SpeedCacheMatrix = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, m_SpeedCacheMatrix.m_DataSize, m_SpeedCacheMatrix.m_pPointer, &errorcodeRet);
            if (CL_SUCCESS != errorcodeRet)
            {
                printf("fail to create SpeedCacheMatrix\n");
		        assert(0);
                return;
            }
            heightMap = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, m_pScene->GetGrid(i)->GetCurrentSurface().m_heightMap.m_DataSize, m_pScene->GetGrid(i)->GetCurrentSurface().m_heightMap.m_pPointer, &errorcodeRet);
            if (CL_SUCCESS != errorcodeRet)
            {
                printf("fail to create vMap\n");
		        assert(0);
                return;
            }
            uMap = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, m_pScene->GetGrid(i)->GetCurrentSurface().m_uMap.m_DataSize, m_pScene->GetGrid(i)->GetCurrentSurface().m_uMap.m_pPointer, &errorcodeRet);
            if (CL_SUCCESS != errorcodeRet)
            {
                printf("fail to create uMap\n");
		        assert(0);
                return;
            }
            vMap = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, m_pScene->GetGrid(i)->GetCurrentSurface().m_vMap.m_DataSize, m_pScene->GetGrid(i)->GetCurrentSurface().m_vMap.m_pPointer, &errorcodeRet);
            if (CL_SUCCESS != errorcodeRet)
            {
                printf("fail to create vMap\n");
		        assert(0);
                return;
            }
            heightMapOut = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, m_pScene->GetGrid(i)->GetDestSurface().m_heightMap.m_DataSize, m_pScene->GetGrid(i)->GetDestSurface().m_heightMap.m_pPointer, &errorcodeRet);
            if (CL_SUCCESS != errorcodeRet)
            {
                printf("fail to create heightMapOut\n");
		        assert(0);
                return;
            }
            uMapOut = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, m_pScene->GetGrid(i)->GetDestSurface().m_uMap.m_DataSize, m_pScene->GetGrid(i)->GetDestSurface().m_uMap.m_pPointer, &errorcodeRet);
            if (CL_SUCCESS != errorcodeRet)
            {
                printf("fail to create uMapOut\n");
		        assert(0);
                return;
            }
            vMapOut = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, m_pScene->GetGrid(i)->GetDestSurface().m_vMap.m_DataSize, m_pScene->GetGrid(i)->GetDestSurface().m_vMap.m_pPointer, &errorcodeRet);
            if (CL_SUCCESS != errorcodeRet)
            {
                printf("fail to create vMapOut\n");
		        assert(0);
                return;
            }
            heightMapBottom = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, m_pScene->GetGrid(i)->GetBottom().m_heightMap.m_DataSize, m_pScene->GetGrid(i)->GetBottom().m_heightMap.m_pPointer, &errorcodeRet);
            if (CL_SUCCESS != errorcodeRet)
            {
                printf("fail to create heightMapBottom\n");
		        assert(0);
                return;
            }
            precompDataMapBottom = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, m_pScene->GetGrid(i)->GetBottom().m_precompDataMap.m_DataSize, m_pScene->GetGrid(i)->GetBottom().m_precompDataMap.m_pPointer, &errorcodeRet);
            if (CL_SUCCESS != errorcodeRet)
            {
                printf("fail to create precompDataMapBottom\n");
		        assert(0);
                return;
            }
            OCLData = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(CKernelData), &m_OCL_data, &errorcodeRet);
            if (CL_SUCCESS != errorcodeRet)
            {
                printf("fail to create OCLData\n");
		        assert(0);
                return;
            }

            //set kernel args
            int iArg=0;
	        if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_mem), (void *)&SpeedCacheMatrix)) != CL_SUCCESS)	
            {
		        return;
	        }
	        if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_mem), (void *)&heightMap)) != CL_SUCCESS)	
            {
		        return;
	        }
            if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_mem), (void *)&uMap)) != CL_SUCCESS)	
            {
		        return;
	        }
	        if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_mem), (void *)&vMap)) != CL_SUCCESS)	
            {
		        return;
	        }
            if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_mem), (void *)&heightMapOut)) != CL_SUCCESS)	
            {
		        return;
	        }
	        if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_mem), (void *)&uMapOut)) != CL_SUCCESS)	
            {
		        return;
	        }
            if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_mem), (void *)&vMapOut)) != CL_SUCCESS)	
            {
		        return;
	        }
	        if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_mem), (void *)&heightMapBottom)) != CL_SUCCESS)	
            {
		        return;
	        }
	        if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_mem), (void *)&precompDataMapBottom)) != CL_SUCCESS)	
            {
		        return;
	        }
            if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_mem), (void *)&OCLData)) != CL_SUCCESS)	
            {
		        return;
	        }

            int tmp_width = m_pScene->GetGrid(i)->GetDomainW();
            int tmp_height = m_pScene->GetGrid(i)->GetDomainH();
	        if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_int), (void*)&tmp_width)) != CL_SUCCESS)	
            {
		        return;
	        }
            if((errorcodeRet = clSetKernelArg(m_kernel, iArg++, sizeof(cl_int), (void*)&tmp_height)) != CL_SUCCESS)	
            {
		        return;
	        }

            //run kernel
            if((errorcodeRet = clEnqueueNDRangeKernel(m_cmd_queue,m_kernel, 
                1,	
                NULL,				/* NULL in this version of openCL */
                m_global_work_size,	/* global work size */
                m_local_work_size,	/* local work size */
                0, NULL, 
                NULL
                )) != CL_SUCCESS)	
            {
                printf("fail to clEnqueueNDRangeKernel for m_kernel (shallow water)\n");
		        assert(0);
		        return;
            }
            if((errorcodeRet = clFinish(m_cmd_queue)) != CL_SUCCESS)	
            {
                printf("fail to clFinish for m_cmd_queue\n");
		        assert(0);
		        return;
            }

	        //Get the results
#if 0
            //don't need if use host ptr
	        if((errorcodeRet = clEnqueueReadBuffer(m_cmd_queue, heightMapOut, CL_TRUE, 0, 
                m_pScene->GetGrid(i)->GetDestSurface().m_heightMap.m_DataSize, 
                m_pScene->GetGrid(i)->GetDestSurface().m_heightMap.m_pPointer, 
                0, NULL, NULL)) != CL_SUCCESS)
            {
                printf("fail to clEnqueueReadBuffer for heightMapOut\n");
		        assert(0);
		        return;
	        }
	        if((errorcodeRet = clEnqueueReadBuffer(m_cmd_queue, uMapOut, CL_TRUE, 0, 
                m_pScene->GetGrid(i)->GetDestSurface().m_uMap.m_DataSize, 
                m_pScene->GetGrid(i)->GetDestSurface().m_uMap.m_pPointer, 
                0, NULL, NULL)) != CL_SUCCESS)
            {
                printf("fail to clEnqueueReadBuffer for uMapOut\n");
		        assert(0);
		        return;
	        }
	        if((errorcodeRet = clEnqueueReadBuffer(m_cmd_queue, vMapOut, CL_TRUE, 0, 
                m_pScene->GetGrid(i)->GetDestSurface().m_vMap.m_DataSize, 
                m_pScene->GetGrid(i)->GetDestSurface().m_vMap.m_pPointer, 
                0, NULL, NULL)) != CL_SUCCESS)
            {
                printf("fail to clEnqueueReadBuffer for vMapOut\n");
		        assert(0);
		        return;
	        }

#endif
            
            //release cl_mem buffers
	        if((errorcodeRet = clReleaseMemObject(SpeedCacheMatrix)) != CL_SUCCESS)	
            {
		        return;
	        }	
            if((errorcodeRet = clReleaseMemObject(heightMap)) != CL_SUCCESS)	
            {
		        return;
	        }	
            if((errorcodeRet = clReleaseMemObject(uMap)) != CL_SUCCESS)	
            {
		        return;
	        }	
            if((errorcodeRet = clReleaseMemObject(vMap)) != CL_SUCCESS)	
            {
		        return;
	        }	
	        if((errorcodeRet = clReleaseMemObject(heightMapOut)) != CL_SUCCESS)	
            {
		        assert(0);
		        return;
	        }	
            if((errorcodeRet = clReleaseMemObject(uMapOut)) != CL_SUCCESS)	
            {
		        return;
	        }	
            if((errorcodeRet = clReleaseMemObject(vMapOut)) != CL_SUCCESS)	
            {
		        return;
	        }	
            if((errorcodeRet = clReleaseMemObject(heightMapBottom)) != CL_SUCCESS)	
            {
		        return;
	        }	
            if((errorcodeRet = clReleaseMemObject(precompDataMapBottom)) != CL_SUCCESS)	
            {
		        return;
	        }	
            if((errorcodeRet = clReleaseMemObject(OCLData)) != CL_SUCCESS)	
            {
		        return;
	        }	

        }

        // Apply actions for all grids
        for (i = 0; i < numGrids; i++)
        {
            m_pScene->GetGrid(i)->RunActions(m_Params,m_pScene->GetGlobalPars());
        }
        // Prepare the scene for the next step
        m_pScene->Proceed2NextStep(dt); 

    }// End of Step()

};

/**
 * @class CFlux2DMethodSerial
 *
 * @brief Calculator based on serial execution
 */
template <class CALCULATOR>
class CFlux2DMethodSerial : 
    public CFlux2DMethod
{
private:

    /**
     * @var m_Calculator
     *
     * @brief An array of calculators, one for each thread
     */
    CALCULATOR                  m_Calculator;

    /**
     * @var m_pScene
     *
     * @brief The pointer to the scene that will be processed
     */
    CFlux2DScene*              m_pScene;

    /**
     * @var m_Params
     *
     * @brief A local copy of the parameters for the current simulation step
     */
    CFlux2DCalculatorPars      m_Params;

public:
    /**
     * @fn CFlux2DMethodSerial(CFlux2DScene &scene, int in_numThreads)
     *
     * @brief Class constructor of the calculator 
     *
     * @param scene - Reference to the scene that will be processed
     *
     * @param in_numThreads - An unused parameter  
     */
    CFlux2DMethodSerial(CFlux2DScene &scene, int in_numThreads)
    {
        m_pScene = &scene;
        m_Calculator.InitForScene(&scene);
    }

    /** 
     * @fn Step(float dt)
     *
     * @brief Makes one simulation step by dt time
     *
     * @param dt - The time step, delta time
     */
    virtual void Step(float dt)
    {
        int i;
        int const numGrids = m_pScene->GetNumGrids();

        // Get the current simulation time
        m_Params.time = m_pScene->GetTime();
        // tau is the time delta
        m_Params.tau = dt;

        // Initialize the calculator
        m_Calculator.Init(m_Params, m_pScene->GetGlobalPars());

        // For each grid in the scene
        for (i = 0; i < numGrids; i++)
        {
            // Perform calculations
            CFlux2DGrid*           pG = m_pScene->GetGrid(i);
            m_Calculator.ProcessTile(pG, 0,0, pG->GetDomainW(), pG->GetDomainH());
        }

        // Apply actions for all grids
        for (i = 0; i < numGrids; i++)
        {
            m_pScene->GetGrid(i)->RunActions(m_Params,m_pScene->GetGlobalPars());
        }
        // Prepare the scene for the next step
        m_pScene->Proceed2NextStep(dt); 

    }// End of Step()
};
