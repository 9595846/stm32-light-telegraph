# 光影电报机 V1.0 · 阶段 2

STM32F103C8T6 / Keil ARM Compiler 5，实际编译 0 错误、0 警告。

包含完整 Keil 源码工程、标准外设库与 CMSIS、预编译 HEX、中文接线图、操作与验收说明。公开包不包含带本机路径的调试 AXF；Rebuild 即可生成。

已实现自动明暗标定、固定 SOS 发送、光敏独立解码、旋钮调速、四键和 OLED 中文交互、串口诊断。舵机 PWM 保持关闭。

主机软件测试通过；尚未完成实体板验收。光学接收距离、响应速度与整套供电余量待实测。完整文字编辑、机械光闸与体感挑战尚未实现。

解压后打开 `firmware/Project.uvprojx`，选择 `Light_Telegraph`。先接好 LED1 与光敏光路，按 KEY1 标定，显示就绪后按 KEY3 发 SOS，KEY2 停止。
