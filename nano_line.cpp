// nano_line.cpp: 定义应用程序的方法。
// 包含于结构相关的函数的代码。

#include "nano_line.h"

// TODO: 在此处实现功能。

// Enable/disable Bayer to RGB conversion
// (If disabled - Bayer format will be treated as Monochrome).
#define ENABLE_BAYER_CONVERSION 1

// Enable/disable buffer FULL/EMPTY handling (cycling)
#define USE_SYNCHRONOUS_BUFFER_CYCLING 0

// Enable/disable transfer tuning (buffering, timeouts, thread affinity).
#define TUNE_STREAMING_THREADS 0

NanoLine::NanoLine()
{
}

NanoLine::~NanoLine()
{
}

bool NanoLine::start()
{
    discoveCameras();

    thread_flag = true;
    start_Thread = std::thread(&NanoLine::threadFunc, this);

    return true;
}
bool NanoLine::stop()
{
    if (thread_flag == true)
    {
        if (start_Thread.joinable())
        {
            start_Thread.join();
            thread_flag = false;
        }
    }

    if (param_nano_line.handle != NULL)
    {
        GevStopTransfer(param_nano_line.handle);
        GevAbortTransfer(param_nano_line.handle);
        GevFreeTransfer(param_nano_line.handle);
    }
    for (uint16_t i = 0; i < numBuffers; i++)
    {
        free(bufAddress[i]);
    }
    // Close the camera.
    GevCloseCamera(&param_nano_line.handle);
    // Close down the API.
    GevApiUninitialize();

    // Close socket API
    _CloseSocketAPI(); // must close API even on error

    printf("Cam stop get data! to exit.\n");
}
void NanoLine::threadFunc()
{
#if USE_SYNCHRONOUS_BUFFER_CYCLING
    syncThreadFunc();
#else
    asyncThreadFunc();
#endif
}

bool NanoLine::initialization()
{
    if (!openCameras() && adjustInterfaceCamera() && getCameraRegisters() && getImageCamera() && initTransferbuffer())
    {
        thread_flag = true;
        return true;
    }
    thread_flag = false;

    return false;
}

bool NanoLine::discoveCameras()
{
    int numCamera = 0;
    if (!GevGetCameraList(&param_nano_line.interface, MAX_CAMERAS, &numCamera))
    {
        std::printf("discove OK.\n");
        return true;
    }
    return false;
}
bool NanoLine::openCameras()
{
    if (!GevOpenCameraBySN(param_nano_line.device_id, GevExclusiveMode, &param_nano_line.handle))
    {
        std::printf("open OK.\n");
        return true;
    }
    return false;
}
bool NanoLine::adjustInterfaceCamera()
{
    GEV_CAMERA_OPTIONS camOptions = {0};
    // Adjust the camera interface options if desired (see the manual)
    GevGetCameraInterfaceOptions(param_nano_line.handle, &camOptions);
    // camOptions.heartbeat_timeout_ms = 60000;		// For debugging (delay camera timeout while in debugger)
    camOptions.heartbeat_timeout_ms = 5000; // Disconnect detection (5 seconds)

#if TUNE_STREAMING_THREADS
    // Some tuning can be done here. (see the manual)
    camOptions.streamFrame_timeout_ms = 1001;           // Internal timeout for frame reception.
    camOptions.streamNumFramesBuffered = 4;             // Buffer frames internally.
    camOptions.streamMemoryLimitMax = 64 * 1024 * 1024; // Adjust packet memory buffering limit.
    camOptions.streamPktSize = 9180;                    // Adjust the GVSP packet size.
    camOptions.streamPktDelay = 10;                     // Add usecs between packets to pace arrival at NIC.

    // Assign specific CPUs to threads (affinity) - if required for better performance.
    {
        int numCpus = _GetNumCpus();
        if (numCpus > 1)
        {
            camOptions.streamThreadAffinity = numCpus - 1;
            camOptions.serverThreadAffinity = numCpus - 2;
        }
    }
#endif
    // Write the adjusted interface options back.
    if (!GevSetCameraInterfaceOptions(param_nano_line.handle, &camOptions))
    {
        return true;
    }
}
bool NanoLine::getCameraRegisters()
{
    DALSA_GENICAM_GIGE_REGS reg = {0};
    // Get the camera registers data structure
    GevGetCameraRegisters(param_nano_line.handle, &reg, sizeof(reg));

    // Examples of using register accesses.

    if (!GevRegisterReadInt(param_nano_line.handle, &reg.Width, 0, &param_nano_line.width))
    {
        std::printf("register width Ok.\n");
    }
    if (!GevRegisterReadInt(param_nano_line.handle, &reg.Height, 0, &param_nano_line.height))
    {
        std::printf("register height Ok.\n");
    }

    if (!GevRegisterReadInt(param_nano_line.handle, &reg.OffsetX, 0, &param_nano_line.x_offset))
    {
        std::printf("register OffsetX Ok.\n");
    }
    if (!GevRegisterReadInt(param_nano_line.handle, &reg.OffsetY, 0, &param_nano_line.y_offset))
    {
        std::printf("register OffsetY Ok.\n");
    }

    return true;
}
bool NanoLine::getImageCamera()
{
    uint16_t i;
    uint32_t maxHeight = 1600;
    uint32_t maxWidth = 2048;
    uint32_t maxDepth = 2;

    // Set up a grab/transfer from this camera
    //
    // Get the current image settings in the camera
    if (!GevGetImageParameters(param_nano_line.handle, &param_nano_line.width, &param_nano_line.height, &param_nano_line.x_offset, &param_nano_line.y_offset, &param_nano_line.format))
    {
        printf("Camera ROI set for \n\theight = %d\n\twidth = %d\n\tx offset = %d\n\ty offset = %d, pixel format = 0x%08x\n", param_nano_line.height, param_nano_line.width, param_nano_line.x_offset, param_nano_line.y_offset, param_nano_line.format);

        maxHeight = param_nano_line.height;
        maxWidth = param_nano_line.width;
        maxDepth = GetPixelSizeInBytes(GevGetUnpackedPixelType(param_nano_line.format));

        // Allocate image buffers
        size = maxDepth * maxWidth * maxHeight;
        for (i = 0; i < numBuffers; i++)
        {
            bufAddress[i] = (PUINT8)malloc(size);
            memset(bufAddress[i], 0, size);
        }

        return true;
    }
    return false;
}
bool NanoLine::initTransferbuffer()
{
#if USE_SYNCHRONOUS_BUFFER_CYCLING
    // Initialize a transfer with synchronous buffer handling.
    if (!GevInitializeTransfer(param_nano_line.handle, SynchronousNextEmpty, size, numBuffers, bufAddress))
    {
        std::printf("synchronous buffer Ok.\n");
        return true;
    }
    return false;
#else
    // Initialize a transfer with asynchronous buffer handling.
    if (!GevInitializeTransfer(param_nano_line.handle, Asynchronous, size, numBuffers, bufAddress))
    {
        std::printf("asynchronous buffer Ok.\n");
        return true;
    }
    return false;
#endif
}

// void CreateImageDisplayWindow();

void NanoLine::syncThreadFunc()
{
    while (!thread_flag)
    {
        if (!initialization())
        {
            if (thread_flag)
                break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        uint32_t cnt_continuous_nano_failed = 0;
        nano_line_img = NULL;
        while (!thread_flag)
        {
            if (GevWaitForNextFrame(param_nano_line.handle, &nano_line_img, 1000))
            {
                cnt_continuous_nano_failed++;
                if (cnt_continuous_nano_failed > 128)
                {
                    std::printf("GevWaitForNextFrame error!.\n");
                    break;
                }
                continue;
            }
            cnt_continuous_nano_failed = 0;
        }
    }

} // Sync thread disconnection and reconnection

void NanoLine::asyncThreadFunc()
{
    std::printf("asyn.\n");
} // Asynchronous thread disconnection and reconnection