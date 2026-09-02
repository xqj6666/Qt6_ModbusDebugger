# TCP/Modbus 协议调试助手

基于 Qt6 加 QTcpSocket 手写 Modbus-TCP 报文的协议调试工具，支持读写寄存器、收发日志实时显示、原始报文十六进制展示。

## 运行环境

- Windows 10 或 11
- 配合 Modbus Slave 模拟器测试

## 快速开始

1. 从 Releases 页面下载最新版 zip
2. 解压后双击 Qt6_ModbusDebugger.exe 运行
3. 打开 Modbus Slave，连接方式选 Modbus TCP/IP，端口 502
4. 点击连接，选择功能码，发送报文

## 功能特性

- TCP 客户端连接管理（IP 加端口）
- Modbus-TCP 报文手动组装（MBAP 头 加 功能码 加 数据）
- 支持功能码 0x03 读保持寄存器 和 0x06 写单个寄存器
- 响应报文解析，异常码识别
- 收发日志实时显示（发送蓝色，接收绿色）
- 原始报文十六进制展示
- 寄存器数据表格（地址、十六进制值、十进制值）

## 技术栈

- Qt 6.5：Core、Widgets、Network
- C++17
- CMake 3.19 以上
- QTcpSocket 手写 Modbus-TCP 报文
- QFormLayout 和 QSplitter 界面布局

## 报文结构

读保持寄存器请求（12 字节）：

00 01 00 00 00 06 01 03 00 00 00 0A

事务ID(2B) 协议ID(2B) 长度(2B) 单元ID(1B) 功能码(1B) 起始地址(2B) 数量(2B)

写单个寄存器请求（12 字节）：

00 01 00 00 00 06 01 06 00 00 00 19

事务ID(2B) 协议ID(2B) 长度(2B) 单元ID(1B) 功能码(1B) 起始地址(2B) 写入值(2B)

## 项目结构

Qt6_ModbusDebugger/
├── CMakeLists.txt
├── README.md
├── main.cpp
├── mainwindow.h
├── mainwindow.cpp
└── mainwindow.ui

## License

本项目仅用于学习和求职展示。