# ESP-DL [[English]](./README.md)

ESP-DL 是由乐鑫官方针对乐鑫系列芯片 [ESP32](https://www.espressif.com/en/products/socs/esp32)、[ESP32-S2](https://www.espressif.com/en/products/socs/esp32-s2)、[ESP32-S3](https://www.espressif.com/en/products/socs/esp32-s3) 和 [ESP32-C3](https://www.espressif.com/en/products/socs/esp32-c3) 所提供的高性能深度学习开发库。更多文档请查看 [ESP-DL 用户指南](https://docs.espressif.com/projects/esp-dl/zh_CN/latest/esp32/index.html)


## 概述

ESP-DL 为**神经网络推理**、**图像处理**、**数学运算**以及一些**深度学习模型**提供 API，通过 ESP-DL 能够快速便捷地将乐鑫各系列芯片产品用于人工智能应用。

ESP-DL 无需借助任何外围设备，因此可作为一些项目的组件，例如可将其作为 **[ESP-WHO](https://github.com/espressif/esp-who)** 的一个组件，该项目包含数个项目级图像应用实例。下图展示了 ESP-DL 的组成及作为组件时在项目中的位置。


<p align="center">
    <img width="%" src="./docs/_static/architecture_cn.drawio.svg">
</p>



## 入门指南

安装并入门 ESP-DL，请参考[快速入门](https://docs.espressif.com/projects/esp-dl/en/latest/esp32s3/get-started.html)。
> 请使用 ESP-IDF 5.0 或以上版本[最新版本](https://github.com/espressif/esp-idf/tree/release/v5.0)。



## 尝试模型库中的模型


ESP-DL 在 [模型库](./include/model_zoo) 中提供了一些模型的 API，如人脸检测、人脸识别、猫脸检测等。您可以使用下表中开箱即用的模型。


| 项目                 | API 实例                                                  |
| -------------------- | ------------------------------------------------------------ |
| 人脸检测 | [ESP-DL/examples/human_face_detect](examples/human_face_detect) |
| 人脸识别 | [ESP-DL/examples/face_recognition](examples/face_recognition) |
| 猫脸检测 | [ESP-DL/examples/cat_face_detect](examples/cat_face_detect)  |


## 部署你的模型

具体可参考 [手动部署模型](https://docs.espressif.com/projects/esp-dl/zh_CN/latest/esp32s3/tutorials/deploying-models.html)。


## 反馈

如果您在使用中发现了错误或者需要新的功能，请提交相关 [issue](https://github.com/espressif/esp-dl/issues)，我们会优先实现最受期待的功能。
