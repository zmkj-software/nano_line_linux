// nano_line_test.cpp: 定义应用程序的入口。
// 包含调用与结构相关的代码

#include <iostream>
#include <memory>
#include "nano_line.h"

int main(int argc, const char *argv[])
{
    std::shared_ptr<NanoLine> sp_nanoline1 = std::make_shared<NanoLine>();
    std::shared_ptr<NanoLine> sp_nanoline2 = std::make_shared<NanoLine>();
    std::shared_ptr<NanoLine> sp_nanoline3 = std::make_shared<NanoLine>();

    ParamNanoLine param_nanoline1 = {"12226901"};
    std::printf("%s.\n", param_nanoline1.device_id);
    // ParamNanoLine param_nanoline2 = {"11020"};
    // std::printf("%s.\n", param_nanoline2);
    // ParamNanoLine param_nanoline3 = {"11020"};
    // std::printf("%s.\n", param_nanoline3);

    sp_nanoline1->init(param_nanoline1);
    // sp_nanoline1->init(param_nanoline2);
    // sp_nanoline1->init(param_nanoline3);

    sp_nanoline1->start();
    // sp_nanoline2->start();
    // sp_nanoline3->start();

    cv::Mat img1, img2, img3;

    // while (true)
    // {
    //     sp_nanoline1->getData(img1);
    //     sp_nanoline1->getData(img2);
    //     sp_nanoline1->getData(img3);
    //     if (!img1.empty())
    //     {
    //         cv::imshow("paramnanoline1", img1);
    //     }
    //     if (!img2.empty())
    //     {
    //         cv::imshow("paramnanoline2", img2);
    //     }
    //     if (!img3.empty())
    //     {
    //         cv::imshow("paramnanoline3", img3);
    //     }
    //     cv::waitKey(10);
    // }

    return 0;
}
