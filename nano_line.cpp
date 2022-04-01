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

static inline uint64_t getTimestamp()
{
    // std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp;
    // tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());                                                                               //获取当前时间点
    // std::time_t timestamp = tp.time_since_epoch().count();                                                                                                                        //计算距离1970-1-1,00:00的时间长度
    // return timestamp;

    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
}

NanoLine::NanoLine()
{
}

NanoLine::~NanoLine()
{
    stop();
}

bool NanoLine::start()
{

    thread_flag = true;
    start_Thread = std::thread(&NanoLine::threadFunc, this);
    // start_Thread.join();

    // threadFunc();

    return true;
}
void NanoLine::stop()
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

bool NanoLine::getData(cv::Mat &img)
{
    std::unique_lock<std::mutex> lock(mtx);
    if (nano_line_img != NULL)
    {
        if (nano_line_img->state == 0)
        {
            img = cv::Mat(nano_line_img->h, nano_line_img->w, CV_8UC1, nano_line_img->address);
        }
    }

    return true;
}

void NanoLine::threadFunc()
{
    initialization();
    while (true)
    {
        std::printf("thread func while.\n");
        std::this_thread::sleep_for(std::chrono::microseconds(1000));

        for (uint16_t i = 0; i < 1; i++)
        {
            std::printf("time last: %ld.\n", timestamp_last_ercv);
        }

        status = GevWaitForNextFrame(param_nano_line.handle, &nano_line_img, 1000);
        if (status == 0)
        {
            timestamp_last_ercv = getTimestamp();
            std::printf("while next time: %ld.\n", timestamp_last_ercv);
        }
        else
        {
            if (getTimestamp() - timestamp_last_ercv > 8000)
            {
                std::printf("time out.\n");
                GevStopTransfer(param_nano_line.handle);
                GevAbortTransfer(param_nano_line.handle);
                status = GevFreeTransfer(param_nano_line.handle);
                status = GevSetImageParameters(param_nano_line.handle, maxWidth, maxHeight, param_nano_line.x_offset, param_nano_line.y_offset, param_nano_line.format);

                threadFunc();
            }
        }
    }
}

bool NanoLine::initialization()
{
    discoveCameras();

    openCameras();
    adjustInterfaceCamera();
    getCameraRegisters();
    getImageCamera();
    initTransferbuffer();
    startingGrab();

    if (status == 0)
    {
        std::printf("initialization OK.\n");
        thread_flag = true;
        return true;
    }
    std::printf("initialization error.\n");
    thread_flag = false;

    return false;
}

void NanoLine::discoveCameras()
{
    int numCamera = 0;
    timestamp_last_ercv = getTimestamp();
    status = GevGetCameraList(&param_nano_line.interface, MAX_CAMERAS, &numCamera);
    if (status == 0)
    {
        timestamp_last_ercv = getTimestamp();

        std::printf("discove OK.\n");
    }
    else
        std::printf("discove error.\n");
}
bool NanoLine::openCameras()
{
    timestamp_last_ercv = getTimestamp();
    status = GevOpenCameraBySN(param_nano_line.device_id, GevExclusiveMode, &param_nano_line.handle);
    if (status == 0)
    {
        timestamp_last_ercv = getTimestamp();

        std::printf("open OK.\n");
        return true;
    }
    std::printf("open error.\n");
    return false;
}
bool NanoLine::adjustInterfaceCamera()
{
    GEV_CAMERA_OPTIONS camOptions = {0};
    // Adjust the camera interface options if desired (see the manual)

    timestamp_last_ercv = getTimestamp();
    status = GevGetCameraInterfaceOptions(param_nano_line.handle, &camOptions);
    if (status == 0)
    {
        timestamp_last_ercv = getTimestamp();
    }

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

    status = GevSetCameraInterfaceOptions(param_nano_line.handle, &camOptions);
    if (status == 0)
    {
        timestamp_last_ercv = getTimestamp();
        std::printf("adjusted interface options ok.\n");
        return true;
    }
    std::printf("adjusted interface options error.\n");
    return false;
}
bool NanoLine::getCameraRegisters()
{
    DALSA_GENICAM_GIGE_REGS reg = {0};
    // Get the camera registers data structure

    status = GevGetCameraRegisters(param_nano_line.handle, &reg, sizeof(reg));
    if (status == 0)
    {
        timestamp_last_ercv = getTimestamp();
    }

    // Examples of using register accesses.

    status = GevRegisterReadInt(param_nano_line.handle, &reg.Width, 0, &param_nano_line.width);
    status = GevRegisterReadInt(param_nano_line.handle, &reg.Height, 0, &param_nano_line.height);
    status = GevRegisterReadInt(param_nano_line.handle, &reg.OffsetX, 0, &param_nano_line.x_offset);
    status = GevRegisterReadInt(param_nano_line.handle, &reg.OffsetY, 0, &param_nano_line.y_offset);

    if (status == 0)
    {
        timestamp_last_ercv = getTimestamp();
        std::printf("register width, height, x_offset, y_offset. Ok.\n");
        return true;
    }
    std::printf("register all error.\n");
    return false;
}
bool NanoLine::getImageCamera()
{
    uint16_t i;

    uint32_t maxDepth = 2;

    // Set up a grab/transfer from this camera
    //
    // Get the current image settings in the camera

    status = GevGetImageParameters(param_nano_line.handle, &param_nano_line.width, &param_nano_line.height, &param_nano_line.x_offset, &param_nano_line.y_offset, &param_nano_line.format);

    if (status == 0)
    {
        timestamp_last_ercv = getTimestamp();
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

        std::printf("Get the current image settings in the camera ok.\n");
        return true;
    }
    std::printf("Get the current image settings in the camera error.\n");
    return false;
}
bool NanoLine::initTransferbuffer()
{
#if USE_SYNCHRONOUS_BUFFER_CYCLING
    // Initialize a transfer with synchronous buffer handling.
    status = GevInitializeTransfer(param_nano_line.handle, SynchronousNextEmpty, size, numBuffers, bufAddress);
    if (status == 0)
    {
        timestamp_last_ercv = getTimestamp();
        std::printf("synchronous buffer Ok.\n");
        return true;
    }
    std::printf("synchronous buffer error.\n");
    return false;

#else
    // Initialize a transfer with asynchronous buffer handling.

    status = GevInitializeTransfer(param_nano_line.handle, Asynchronous, size, numBuffers, bufAddress);
    if (status == 0)
    {
        timestamp_last_ercv = getTimestamp();
        std::printf("asynchronous buffer Ok.\n");
        return true;
    }
    std::printf("asynchronous buffer error.\n");
    return false;

#endif
}
bool NanoLine::startingGrab()
{
    for (uint16_t i = 0; i < numBuffers; i++)
    {
        memset(bufAddress[i], 0, size);
    }
    status = GevStartTransfer(param_nano_line.handle, -1);
    if (status == 0)
    {
        timestamp_last_ercv = getTimestamp();
        std::printf("Call the main command loop or the example.");
        return true;
    }
    printf("Error starting grab! \n");
    return false;
}