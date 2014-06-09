#define __CL_ENABLE_EXCEPTIONS

 #if defined(__APPLE__) || defined(__MACOSX)
  #include <OpenCL/cl.hpp>
#else
  #include <CL/cl.hpp>
#endif

#include<iostream>
#include <fstream>

#define SUCCESS 0
#define FAILURE 1
#define EXPECTED_FAILURE 2

#define GlobalThreadSize 256
#define GroupSize 64

#define PERFERED_AMD

#ifdef PERFERED_AMD
	#define TARGET_PLAFORM "AMD"
	#define PERFER_DEVICE_TYPE CL_DEVICE_TYPE_ALL
#else
	#define TARGET_PLAFORM "Intel"
	#define PERFER_DEVICE_TYPE CL_DEVICE_TYPE_CPU
#endif


/* convert the kernel file into a string */
int convertToString(const char *filename, std::string& s)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if(f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size+1];
		if(!str)
		{
			f.close();
			return SUCCESS;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return SUCCESS;
	}
	std::cout<<"Error: failed to open file\n:"<<filename<<std::endl;
	return FAILURE;
}

int main(int argc, char* argv[])
{
	cl_int status = CL_SUCCESS;
	cl_float* input;
	try {
  		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		if (platforms.empty()) {
			std::cerr <<"Platform size = 0"<<std::endl;
			return FAILURE;
		}
		cl::Platform targetPlatform = *platforms.begin();
		for (std::vector<cl::Platform>::iterator iter = platforms.begin(); iter != platforms.end(); iter++) {
			std::string strPlatform = iter->getInfo<CL_PLATFORM_NAME>();
			std::cout<< "Found platform: "<< strPlatform<<std::endl;
			if (strPlatform.find(TARGET_PLAFORM) != std::string::npos) {
				targetPlatform = *iter;
    			std::cout<< "Select platform: "<< strPlatform<<std::endl;
			}
		}
		
		cl_context_properties properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)targetPlatform(), 0};
		cl::Context context(PERFER_DEVICE_TYPE, properties);

		std::vector<cl::Device> devices =context.getInfo<CL_CONTEXT_DEVICES>();
		for (std::vector<cl::Device>::iterator iter = devices.begin(); iter != devices.end(); iter++) {
			std::string strDevices = iter->getInfo<CL_DEVICE_NAME>();
			std::cout<< "Found device: "<< strDevices<<std::endl;
		}

		cl::CommandQueue queue(context, devices[0], 0, &status);
		if (status != CL_SUCCESS) {
			throw cl::Error(status, "CommandQueue!");
		}
		
		cl_uint inputSizeBytes = GlobalThreadSize * sizeof(cl_uint);
		input = (cl_float*) malloc(inputSizeBytes);

		for (int i=0; i< GlobalThreadSize; i++) {
			input[i] = (float)i;
		}

		cl_mem inputBuffer = clCreateBuffer(
			context(),
			CL_MEM_READ_ONLY |
			CL_MEM_USE_HOST_PTR,
			inputSizeBytes,
			(void*) input,
			&status);
		if (status != CL_SUCCESS) {
			throw cl::Error(status, "Input Memory Buffer!");
		}

		cl_mem outputBuffer = clCreateBuffer(
			context(),
			CL_MEM_WRITE_ONLY,
			inputSizeBytes,
			NULL,
			&status);
		if (status != CL_SUCCESS) {
			throw cl::Error(status, "Output Memory Buffer!");
		}
		const char* filename = "./BasicDebug_Kernel.cl";
		std::string sourceStr;
		status = convertToString(filename, sourceStr);
		const char *source = sourceStr.c_str();
		size_t sourceSize[] ={strlen(source)};

		const char* filename2 = "./BasicDebug_Kernel2.cl";
		std::string sourceStr2;
		status = convertToString(filename2, sourceStr2);
		const char *source2 = sourceStr2.c_str();
    	size_t sourceSize2[] ={strlen(source2)};

		cl::Program::Sources sources;
		cl::Program::Sources sources2;
		sources.push_back(std::make_pair(source, strlen(source)));
		sources2.push_back(std::make_pair(source2, strlen(source2)));

		cl::Program program = cl::Program(context, sources);
		program.build(devices, "-g");
		cl::Program program2 = cl::Program(context, sources2);
		program2.build(devices, "-g");

		cl::Kernel printfKernel(program, "printfKernel", &status);
		if (status != CL_SUCCESS) {
			throw cl::Error(status, "Printf Kernel!");
		}
		//set kernel args.
		status = clSetKernelArg(printfKernel(), 0, sizeof(cl_mem), (void *)&inputBuffer);

		cl::Kernel debugKernel(program2, "debugKernel", &status);
		if (status != CL_SUCCESS) {
			throw cl::Error(status, "DebugKernel!");
		}
		//set kernel args.
		status = clSetKernelArg(debugKernel(), 0, sizeof(cl_mem), (void *)&inputBuffer);
		status = clSetKernelArg(debugKernel(), 1, sizeof(cl_mem), (void *)&outputBuffer);

		size_t global_threads[1];
		size_t local_threads[1];
		global_threads[0] = GlobalThreadSize;
		local_threads[0] = GroupSize;

//		cl::Event event;
		status = queue.enqueueNDRangeKernel(
			printfKernel,
			cl::NullRange,
			cl::NDRange(global_threads[0]),
			cl::NDRange(local_threads[0]),
			NULL,
			NULL);
//			&event);
//		event.wait();
		if (status != CL_SUCCESS) {
			throw cl::Error(status, "Execute printfKernel!");
		}
		status = clFinish(queue());
		
		status = queue.enqueueNDRangeKernel(
			debugKernel,
			cl::NullRange,
			cl::NDRange(global_threads[0]),
			cl::NDRange(local_threads[0]),
			NULL,
			NULL);
		if (status != CL_SUCCESS) {
			throw cl::Error(status, "Execute DebugKernel!");
		}

		status = clReleaseMemObject(inputBuffer);//Release mem object.
		status = clReleaseMemObject(outputBuffer);

		status = clFinish(queue());
//#endif
		free(input);
	} catch (cl::Error err) {
		if (input) {
			free(input);
		}
		std::cerr<< "Error: " << err.what() << "("
					 << err.err	() << ")" << std::endl;
		return FAILURE;
	}
	return SUCCESS;
}