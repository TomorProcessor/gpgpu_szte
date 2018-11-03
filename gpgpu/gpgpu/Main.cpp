#include <afxwin.h>
#include <afxdlgs.h>
#include <Winuser.h>
#include <CL/cl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "FreeImage.h"


#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
const int N = 512;
const int MAX_PLATFORMS = 5;
const int MAX_DEVICES = 5;

int openClHost(CString imgFileUrl);

class MainWindow :public CFrameWnd
{
public:
	MainWindow()
	{
		Create(NULL, _T("GPGPU test"));
	}
	BOOL PreCreateWindow(CREATESTRUCT& cs)
	{
		int screenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYFULLSCREEN);
		cs.cy = 240; // width
		cs.cx = 320; // height
		cs.y = (screenHeight - cs.cy)/2; // top position
		cs.x = (screenWidth - cs.cx)/2; // left position

		if (!CFrameWnd::PreCreateWindow(cs))
			return FALSE;
		// TODO: Modify the Window class or styles here by modifying
		//  the CREATESTRUCT cs

		//my edits here
		cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

		cs.style &= (0xFFFFFFFF ^ WS_SIZEBOX);
		cs.style |= WS_BORDER;
		cs.style &= (0xFFFFFFFF ^ WS_MAXIMIZEBOX);
		//end my edits

		cs.lpszClass = AfxRegisterWndClass(0);

		// don't forget to call base class version, suppose you derived you window from CWnd
		return CFrameWnd::PreCreateWindow(cs);
	}

};

class OpenButton : public CButton {
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point) {
		//MessageBox(L"Left Mouse Button Up");

		const TCHAR szFilter[] = _T("Image Files (*.png)|*.png|");
		CFileDialog dlg(TRUE, _T("csv"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, this);
		if (dlg.DoModal() == IDOK)
		{
			CString sFilePath = dlg.GetPathName();
			//MessageBox(sFilePath);
			::openClHost(sFilePath);
		}
	}
};

BEGIN_MESSAGE_MAP(OpenButton, CButton)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

class MainDialog : public CDialog {
	OpenButton *openButton;

	BOOL OnInitDialog()
	{
		CDialog::OnInitDialog();

		openButton = new OpenButton();
		openButton->Create(_T("Open Image"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(40, 40, 260, 160), this, 1);

		return TRUE;
	}
};

class Main :public CWinApp
{
	MainDialog *dialog;
public:
	MainWindow *wnd;
	BOOL InitInstance()
	{
		wnd = new MainWindow();
		dialog = new MainDialog();
		dialog->Create(103, wnd);

		m_pMainWnd = wnd;
		m_pMainWnd->ShowWindow(SW_SHOW);
		return TRUE;
	}
};

Main main;

///
//  Constants
//
const int ARRAY_SIZE = 1000;
void DisplayPlatformInfo(
	cl_platform_id id,
	cl_platform_info name,
	std::string str)
{
	cl_int errNum;
	std::size_t paramValueSize;

	errNum = clGetPlatformInfo(
		id,
		name,
		0,
		NULL,
		&paramValueSize);
	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to find OpenCL platform " << str << "." << std::endl;
		return;
	}

	char * info = (char *)alloca(sizeof(char) * paramValueSize);
	errNum = clGetPlatformInfo(
		id,
		name,
		paramValueSize,
		info,
		NULL);
	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to find OpenCL platform " << str << "." << std::endl;
		return;
	}

	std::cout << "\t" << str << ":\t" << info << std::endl;
}
///
//  Create an OpenCL context on the first available platform using
//  either a GPU or CPU depending on what is available.
//
cl_context CreateContext(cl_uint *numDevices, cl_device_id *devices)
{
	cl_int errNum;

	cl_uint numPlatforms;
	cl_platform_id platformIds[MAX_PLATFORMS];
	cl_context context = NULL;


	errNum = clGetPlatformIDs(0, NULL, &numPlatforms);

	std::cout << "Number of platforms: \t" << numPlatforms << std::endl;

	if (errNum != CL_SUCCESS || numPlatforms <= 0)
	{
		std::cerr << "Failed to find any OpenCL platforms." << std::endl;
		return NULL;
	}

	errNum = clGetPlatformIDs(numPlatforms, platformIds, NULL);

	cl_uint selPlatform = 0;

	DisplayPlatformInfo(
		platformIds[selPlatform],
		CL_PLATFORM_PROFILE,
		"CL_PLATFORM_PROFILE");

	DisplayPlatformInfo(
		platformIds[selPlatform],
		CL_PLATFORM_VERSION,
		"CL_PLATFORM_VERSION");

	DisplayPlatformInfo(
		platformIds[selPlatform],
		CL_PLATFORM_VENDOR,
		"CL_PLATFORM_VENDOR");

	// Next, create an OpenCL context on the platform.  Attempt to
	// create a GPU-based context, and if that fails, try to create
	// a CPU-based context.
	cl_context_properties contextProperties[] =
	{
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)platformIds[selPlatform],
		0
	};


	errNum = clGetDeviceIDs(
		platformIds[selPlatform],
		CL_DEVICE_TYPE_ALL,
		0,
		NULL,
		numDevices);

	std::cout << "\tNumber of devices: \t" << *numDevices << std::endl;

	errNum = clGetDeviceIDs(
		platformIds[selPlatform],
		CL_DEVICE_TYPE_ALL,
		*numDevices,
		devices,
		NULL);

	context = clCreateContext(contextProperties, *numDevices, devices, NULL, NULL, NULL);

	return context;
}

///
//  Create a command queue on the first device available on the
//  context
//
cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device, cl_uint deviceNum, cl_device_id *devices)
{

	//cl_device_id *devices;
	cl_command_queue commandQueue = NULL;
	size_t deviceBufferSize = -1;

	commandQueue = clCreateCommandQueue(context, devices[deviceNum], 0, NULL);
	if (commandQueue == NULL)
	{
		delete[] devices;
		std::cerr << "Failed to create commandQueue for device 0";
		return NULL;
	}

	*device = devices[deviceNum];
	//delete[] devices;
	return commandQueue;
}

///
//  Create an OpenCL program from the kernel source file
//
cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName)
{
	cl_int errNum;
	cl_program program;

	std::ifstream kernelFile(fileName, std::ios::in);
	if (!kernelFile.is_open())
	{
		std::cerr << "Failed to open file for reading: " << fileName << std::endl;
		return NULL;
	}

	std::ostringstream oss;
	oss << kernelFile.rdbuf();

	std::string srcStdStr = oss.str();
	const char *srcStr = srcStdStr.c_str();
	program = clCreateProgramWithSource(context, 1,
		(const char**)&srcStr,
		NULL, NULL);
	if (program == NULL)
	{
		std::cerr << "Failed to create CL program from source." << std::endl;
		return NULL;
	}

	errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		// Determine the reason for the error
		char buildLog[16384];
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
			sizeof(buildLog), buildLog, NULL);

		std::cerr << "Error in kernel: " << std::endl;
		std::cerr << buildLog;
		clReleaseProgram(program);
		return NULL;
	}

	return program;
}

///
//  Create memory objects used as the arguments to the kernel
//  The kernel takes three arguments: result (output), a (input),
//  and b (input)
//
bool CreateMemObjects(cl_context context, cl_mem memObjects[3],
	float *a, float *b)
{
	memObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(float) * ARRAY_SIZE, a, NULL);
	memObjects[1] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(float) * ARRAY_SIZE, b, NULL);
	memObjects[2] = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float) * ARRAY_SIZE, NULL, NULL);

	if (memObjects[0] == NULL || memObjects[1] == NULL || memObjects[2] == NULL)
	{
		std::cerr << "Error creating memory objects." << std::endl;
		return false;
	}

	return true;
}

///
//  Cleanup any created OpenCL resources
//
void Cleanup(cl_context context, cl_command_queue commandQueue,
	cl_program program, cl_kernel kernel, cl_mem memObjects[2], cl_sampler sampler,
	cl_mem kernelBuffer)
{
	for (int i = 0; i < 2; i++)
	{
		if (memObjects[i] != 0)
			clReleaseMemObject(memObjects[i]);
	}
	if (commandQueue != 0)
		clReleaseCommandQueue(commandQueue);

	if (kernel != 0)
		clReleaseKernel(kernel);

	if (program != 0)
		clReleaseProgram(program);

	if (context != 0)
		clReleaseContext(context);

	if (sampler != 0)
		clReleaseSampler(sampler);

	if (kernelBuffer != 0)
		clReleaseMemObject(kernelBuffer);
}

size_t RoundUp(int groupSize, int globalSize)
{
	int r = globalSize % groupSize;
	if (r == 0)
	{
		return globalSize;
	}
	else
	{
		return globalSize + groupSize - r;
	}
}

cl_mem LoadImage(cl_context context, char *fileName, int &width, int &height)
{
	FREE_IMAGE_FORMAT format = FreeImage_GetFileType(fileName, 0);
	FIBITMAP* image = FreeImage_Load(format, fileName);


	// Convert to 32-bit image
	FIBITMAP* temp = image;
	image = FreeImage_ConvertTo32Bits(image);
	FreeImage_Unload(temp);

	width = FreeImage_GetWidth(image);
	height = FreeImage_GetHeight(image);

	char *buffer = new char[width * height * 4];
	memcpy(buffer, FreeImage_GetBits(image), width * height * 4);

	FreeImage_Unload(image);

	// Create OpenCL image
	cl_image_format clImageFormat;
	clImageFormat.image_channel_order = CL_BGRA;
	clImageFormat.image_channel_data_type = CL_UNORM_INT8;

	cl_int errNum;
	cl_mem clImage;
	clImage = clCreateImage2D(context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		&clImageFormat,
		width,
		height,
		0,
		buffer,
		&errNum);

	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error creating CL image object" << std::endl;
		return 0;
	}

	return clImage;
}

int openClHost(CString imgFileUrl) {
	cl_context context = 0;
	cl_command_queue commandQueue = 0;
	cl_program program = 0;
	cl_device_id device = 0;
	cl_device_id devices[MAX_DEVICES];


	cl_uint numDevices;
	cl_kernel kernel = 0;
	cl_mem imgObjects[2] = { 0, 0 };
	cl_sampler sampler = 0;
	cl_int errNum;
	cl_mem kernelBuffer = 0;

	// Create an OpenCL context on first available platform
	context = CreateContext(&numDevices, devices);
	if (context == NULL)
	{
		std::cerr << "Failed to create OpenCL context." << std::endl;
		return 1;
	}

	// Create a command-queue on the second device available
	// on the created context
	commandQueue = CreateCommandQueue(context, &device, 0, devices);
	if (commandQueue == NULL)
	{
		Cleanup(context, commandQueue, program, kernel, imgObjects, sampler, kernelBuffer);
		return 1;
	}


	// Create OpenCL program from HelloWorld.cl kernel source
	program = CreateProgram(context, device, "../device.cl");
	if (program == NULL)
	{
		Cleanup(context, commandQueue, program, kernel, imgObjects, sampler, kernelBuffer);
		return 1;
	}

	// Create OpenCL kernel
	kernel = clCreateKernel(program, "gaussian_filter", NULL);
	if (kernel == NULL)
	{
		std::cerr << "Failed to create kernel" << std::endl;
		Cleanup(context, commandQueue, program, kernel, imgObjects, sampler, kernelBuffer);
		return 1;
	}


	cl_bool imageSupport = CL_FALSE;

	clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool),
		&imageSupport, NULL);

	if (imageSupport != CL_TRUE)
	{
		std::cerr << "OpenCL device does not support images." << std::endl;

		Cleanup(context, commandQueue, program, kernel, imgObjects, sampler, kernelBuffer);

		return 1;
	}

	// Create sampler for sampling image object
	sampler = clCreateSampler(context,
		CL_FALSE, // Non-normalized coordinates
		CL_ADDRESS_CLAMP_TO_EDGE,
		CL_FILTER_NEAREST,
		&errNum);


	int width, height;
	//CString to UTF8 char*
	CT2A ascii(imgFileUrl, CP_UTF8);
	imgObjects[0] = LoadImage(context, ascii.m_psz, width, height);
	if (imgObjects[0] == 0)
	{
		std::cerr << "Error loading" << std::endl;
		Cleanup(context, commandQueue, program, kernel, imgObjects, sampler, kernelBuffer);
		return 1;
	}

	cl_image_format clImageFormat;
	clImageFormat.image_channel_order = CL_BGRA;
	clImageFormat.image_channel_data_type = CL_UNORM_INT8;


	imgObjects[1] = clCreateImage2D(context,
		CL_MEM_WRITE_ONLY,
		&clImageFormat,
		N,
		N,
		0,
		NULL,
		&errNum);



	if (imgObjects[0] == NULL)
	{
		std::cerr << "Error creating memory objects." << std::endl;
		return false;
	}

	const int kernelWidth = 5;
	const int sum = 25;

	cl_float  kernelMask[kernelWidth * kernelWidth] =
	{
		1.0f / sum, 1.0f / sum, 1.0f / sum, 1.0f / sum, 1.0f / sum,
		1.0f / sum, 1.0f / sum, 1.0f / sum, 1.0f / sum, 1.0f / sum,
		1.0f / sum, 1.0f / sum, 1.0f / sum, 1.0f / sum, 1.0f / sum,
		1.0f / sum, 1.0f / sum, 1.0f / sum, 1.0f / sum, 1.0f / sum,
		1.0f / sum, 1.0f / sum, 1.0f / sum, 1.0f / sum, 1.0f / sum
	};


	kernelBuffer = clCreateBuffer(
		context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_uint) * kernelWidth * kernelWidth,
		kernelMask,
		&errNum);

	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error creating kernelBuffer." << std::endl;
		Cleanup(context, commandQueue, program, kernel, imgObjects, sampler, kernelBuffer);
		return 1;
	}

	// Set the kernel arguments (result, a, b)
	errNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), &imgObjects[0]);
	errNum |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &imgObjects[1]);
	errNum |= clSetKernelArg(kernel, 2, sizeof(cl_sampler), &sampler);
	errNum |= clSetKernelArg(kernel, 3, sizeof(cl_int), &width);
	errNum |= clSetKernelArg(kernel, 4, sizeof(cl_int), &height);
	errNum |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &kernelBuffer);
	errNum |= clSetKernelArg(kernel, 6, sizeof(cl_int), &kernelWidth);

	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error setting kernel arguments." << std::endl; Cleanup(context, commandQueue, program, kernel, imgObjects, sampler, kernelBuffer);
		return 1;
	}


	size_t localWorkSize[2] = { 16, 16 };
	size_t globalWorkSize[2] = { RoundUp(localWorkSize[0], N),
		RoundUp(localWorkSize[1], N) };

	LARGE_INTEGER perfFrequency;
	LARGE_INTEGER performanceCountNDRangeStart;
	LARGE_INTEGER performanceCountNDRangeStop;

	QueryPerformanceCounter(&performanceCountNDRangeStart);

	// Queue the kernel up for execution across the array
	errNum = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL,
		globalWorkSize, localWorkSize,
		0, NULL, NULL);

	clFinish(commandQueue);

	QueryPerformanceCounter(&performanceCountNDRangeStop);

	QueryPerformanceFrequency(&perfFrequency);
	printf("NDRange performance counter time %f ms.\n",
		1000.0f*(float)(performanceCountNDRangeStop.QuadPart - performanceCountNDRangeStart.QuadPart) / (float)perfFrequency.QuadPart);


	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error queuing kernel for execution." << std::endl;
		Cleanup(context, commandQueue, program, kernel, imgObjects, sampler, kernelBuffer);
		return 1;
	}

	unsigned char* result = new  unsigned char[width * height * 4];
	size_t origin[3] = { 0, 0, 0 };
	size_t region[3] = { width, height, 1 };

	// Read the output buffer back to the Host

	errNum = clEnqueueReadImage(commandQueue, imgObjects[1], CL_TRUE,
		origin, region, 0, 0, result,
		0, NULL, NULL);


	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error reading result buffer." << std::endl;
		Cleanup(context, commandQueue, program, kernel, imgObjects, sampler, kernelBuffer);
		return 1;
	}

	// Output the result buffer


	//SaveImage("Output.bmp", result, width, height);


	std::cout << "Executed program succesfully." << std::endl;

	Cleanup(context, commandQueue, program, kernel, imgObjects, sampler, kernelBuffer);


	return 0;
}
