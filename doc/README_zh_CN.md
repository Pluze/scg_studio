# SCG_box

基于Arduino的SCG信号与ECG信号同步采集设备固件。  
主控基于esp32，运行于单核心，采样率达到近1kHz。  
通过WiFi发送信号数据，支持AP模式与station模式，支持二进制与字符串模式发送。  
包含控制及信号采集软件，（持续开发中）。

[English](../README.md)


## 特点

<details>
<summary>简易开发</summary>

+ 基于ESP32的Arduino库，遵从Arduino编程习惯
+ 核心函数可直接调用

</details>

<details>
<summary>读取便捷</summary>

+ 可选用二进制发送数据，也可以直接发送字符串，便于上位机读取
+ 待发送数据以数组形式暂存，便于再开发

</details>

## 硬件设计
V0.1  
![V0.1](./doc/fig/board3d.png)  
V1.0_beta  
![V1.0_beta](./doc/fig/newboard3d.png)  

## 目录结构

| 目录                     | 描述                                                                             |
| ------------------------ | -------------------------------------------------------------------------------- |
| ``/doc``     | 说明文档相关文件                                                           |
| ``/scg_box_firmware`` | 当前版本的SCG box固件                                                           |
| ``/src_matlab``  | 基于matlab的控制及采集程序                                                      |
| ``/src_qt``                 | 基于qt的控制及采集程序                                                                 |

## 教程[未完工]
[1.烧录](./doc/upload.md)  
[2.再开发](./doc/dev.md)  

## 更新日志
[更新日志](./doc/CHANGELOG_zh_CN.md)
