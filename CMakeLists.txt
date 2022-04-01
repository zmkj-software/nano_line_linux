# 声明要求的 cmake 最低版本
# 终端输入`cmake -version`可查看`cmake`的版本
cmake_minimum_required( VERSION 3.10.0 )

# 定义项目名称变量PROJECT_NAME, 默认值为demo
set( PROJECT_NAME nano_line_test )

# 项目名称以及版本
project( ${PROJECT_NAME} VERSION 0.0.1 LANGUAGES CXX)

# 设置 'cmake' 编译模式为 'debug'
set( CMAKE_BUILD_TYPE Debug )

# 设置C++的版本
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -std=c++11 -march=native")

message( "Build type:               ${CMAKE_BUILD_TYPE} ")
message( "CMAKE_CXX_FLAGS:          ${CMAKE_CXX_FLAGS} ")

# 判断编译器是否支持C++11
include( CheckCXXCompilerFlag )

# include 指令用来载入并运行来自于文件或模块的 CMake 代码
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    add_definitions(-DCOMPILEDWITHC11)
    message(STATUS "Using flag -std=c++11.")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    add_definitions(-DCOMPILEDWITHC0X)
    message(STATUS "Using flag -std=c++0x.")
else()
    message( "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()


# 第三方库
# 注意事项：
# 在CMakeLists.txt中使用第三方库的三部曲:find_package、include_directories、target_link_libraries
# 务必 link_directories 要放在 add_executable() or add_library() 前面;
# target_link_libraries 要放在 add_executable() or add_library() 后面。


# opencv
# 查找 OpenCV3.4.1的头文件和库文件路径以及库文件
find_package( OpenCV 3.4.1 REQUIRED )

if(OpenCV_FOUND)
    message( "Found OpenCV libraries" )
else()
    message( "Can't find libavcodec, libavformat or libavutil. Add them!")
endif()


# FFmpeg
# 设置头文件路径
set(FFmpeg_INCLUDE_DIR /opt/ffmpeg/include)
# 设置动态库路径
link_directories(/opt/ffmpeg/lib)
# 将动态库存入 FFmpeg_LIBS 变量
set(FFmpeg_LIBS avfilter avcodec avutil swscale avdevice avformat swresample)


# GenICam
set(GenICam_INCLUDE_DIRS /usr/dalsa/GigeV/include /opt/genicam_v3_0/library/CPP/include/GenApi /opt/genicam_v3_0/library/CPP/include)
# 设置动态库路径
link_directories(/usr/local/lib /opt/genicam_v3_0/bin/Linux64_x64)
# 将动态库存入 LIST_GenICam 变量
set(GenICam_LIBS pthread GevApi CorW32 GenApi_gcc421_v3_0 GCBase_gcc421_v3_0)


message( "PROJECT_SOURCE_DIR:      ${PROJECT_SOURCE_DIR}" )
message( "OpenCV_INCLUDE_DIRS:     ${OpenCV_INCLUDE_DIRS}" )
message( "OpenCV_LIBS:             ${OpenCV_LIBS}" )
message( "FFmpeg_INCLUDE_DIR:      ${FFmpeg_INCLUDE_DIR}" )
message( "FFmpeg_LIBS:             ${FFmpeg_LIBS}" )
message( "GenICam_INCLUDE_DIRS:    ${GenICam_INCLUDE_DIRS}" )
message( "GenICam_LIBS:            ${GenICam_LIBS}" )


# 设定头文件目录
include_directories(  ${PROJECT_SOURCE_DIR}
                      ${PROJECT_SOURCE_DIR}/common
                      ${OpenCV_INCLUDE_DIRS}
                      ${FFmpeg_INCLUDE_DIR}
                      ${GenICam_INCLUDE_DIRS}
)


# 包含 cmake 当前文件夹目录
set( CMAKE_INCLUDE_CURRENT_DIR ON )

# 查找当前目录下的所有源文件
aux_source_directory( . DIR_SOURCES )
aux_source_directory( ./common DIRS_COMMON_SOURCES)

# 设定编译文件生成目录

# 设定生成的库文件的存放目录
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
#MESSAGE("CMAKE_LIBRARY_OUTPUT_DIRECTORY: " ${PROJECT_SOURCE_DIR}/lib)
# CMAKE_LIBRARY_OUTPUT_DIRECTORY: /home/q/Projects/comp/ORB_SLAM2/lib
#SET( LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 生成静态库
#add_library(${PROJECT_NAME} SHARED)
# ORB_SLAM2: ORB_SLAM2


# 设定生成的二进制文件的存放目录
set( EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build )

# 添加一个可执行程序, 务必 要放在 link_directories 后面； 要放在 target_link_libraries 前面。
# 语法 : add_executable( 程序名 源代码文件 )
add_executable( ${PROJECT_NAME}  ${DIR_SOURCES} ${DIRS_COMMON_SOURCES})


# 需要链接的库文件
target_link_libraries(  ${PROJECT_NAME}
                        ${OpenCV_LIBS}
                        ${FFmpeg_LIBS}
                        ${GenICam_LIBS}
)

