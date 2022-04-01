// nano_line.h : 包含结构声明和使用这些结构的函数的原型。

#pragma once

#include <iostream>

// TODO: 在此处引用程序需要的其他标头。
#ifndef OPENCVINCFLAG
#define OPENCVINCFLAG
#include "opencv2/opencv.hpp"
#endif

#include "cordef.h"
#include "gevapi.h"
#include "SapX11Util.h"
#include "X_Display_utils.h"

#include <sched.h>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>

#define MAX_NETIF 8
#define MAX_CAMERAS_PER_NETIF 32
#define MAX_CAMERAS (MAX_NETIF * MAX_CAMERAS_PER_NETIF)

#define NUM_BUF 8

typedef struct tagMY_CONTEXT
{
    X_VIEW_HANDLE View;
    GEV_CAMERA_HANDLE camHandle;
    uint32_t depth;
    uint32_t format;
    void *convertBuffer;
    BOOL convertFormat;
    BOOL exit;
} MY_CONTEXT, *PMY_CONTEXT;

struct ParamNanoLine
{
    char *device_id;

    uint32_t height;
    uint32_t width;
    uint32_t x_offset;
    uint32_t y_offset;
    uint32_t format;

    GEV_DEVICE_INTERFACE interface;
    GEV_CAMERA_HANDLE handle;
};

class NanoLine
{
public:
    NanoLine();
    virtual ~NanoLine();

    void init(const ParamNanoLine &param_nano_line_in) { param_nano_line = param_nano_line_in; }

public:
    bool start();
    void stop();
    bool getData(cv::Mat &img);

private:
    void threadFunc();

private:
    bool initialization();
    void discoveCameras();
    bool openCameras();
    bool adjustInterfaceCamera();
    bool getCameraRegisters();
    bool getImageCamera();
    bool initTransferbuffer();
    bool startingGrab();

private:
    ParamNanoLine param_nano_line;
    GEV_STATUS status;
    GEV_BUFFER_OBJECT *nano_line_img;

    std::thread start_Thread;
    bool thread_flag;
    std::mutex mtx;

    uint64_t timestamp_last_ercv;
    uint64_t size;
    uint32_t maxHeight = 1600;
    uint32_t maxWidth = 2048;
    uint32_t numBuffers = NUM_BUF;
    PUINT8 bufAddress[NUM_BUF];
};
