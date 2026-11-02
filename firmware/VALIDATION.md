> GitHub 公开版包含源码和 HEX，不附带本机调试 AXF。请在 Keil 中 Rebuild 后再用 AXF 下载，或使用 HEX 烧录。

# 阶段 2 验证记录

日期：2026-11-02。目标：`Light_Telegraph`。

## 实际完成

- 从用户指定的 `defuse_box/firmware` 基础工程复制必要源码和配置；未修改原工程。来源 SHA-256 清单为 `BASELINE.json`。
- 保留 STM32F103C8、64KB Flash、20KB RAM、HSI/PLL 64MHz、CMSIS 和标准外设库；编译器为 ARM Compiler V5.06 update 7 build 960。
- 真实 Keil Rebuild：**0 Error(s), 0 Warning(s)**。日志：`BuildLogs/Keil_Light_Telegraph.log`。
- 程序占用：Code 11962、RO-data 2526、RW-data 120、ZI-data 6544 字节。
- 新增 `User/light_link.c/.h` 已加入 Keil 工程。工程 XML 为无 BOM UTF-8。
- 实现自动明暗标定、迟滞判断、5ms 目标间隔的 AO 采样、20ms 光学去抖、300～1000ms 码元、非阻塞 SOS 发送、独立 AO 摩斯码解码、输入诊断与停机。
- 新固件移除原来 PA0 自动心跳闪灯，改为受控发射。PA8 保持关闭，未启动任何舵机动作。

## 软件测试

使用 GCC 编译并执行实际 `User/light_link.c`，通过：

- 正向与反向 AO 明暗关系的自动标定、300ms 与 1000ms SOS 收发。
- 发射仍在运行但接收光路全程被挡时，不产生 SOS 接收文字。
- 无发送任务时，外部模拟脉冲仍能被接收端解码；接收不依赖发送队列。
- 低差异与高噪声拒绝、ADC 超时、停止和中断标定、调度延迟、毫秒计时回绕。
- 短毛刺拒绝、异常长脉冲报错、接收窗口滚动及终止符边界。

编译并执行实际 `User/main.c` 的解析/输入逻辑（模拟底层串口和提示输出），以及实际 `Hardware/display.c`，通过：

- 命令范围与格式、超长行和非法字符丢弃恢复、停止优先、旋钮限幅、忙时拒绝改速。
- 六个页面状态与极值文本的边界检查；导出真实渲染帧缓冲。
- 中文页面及 PNG 预览已目视检查，预览使用模拟数据，**不是实物照片**。

完整性检查也已通过：来源清单中的原工程文件 SHA-256 均未变化；新工程所有文件引用存在；两个 XML 可解析且无 BOM；时钟、芯片和内存布局保留；发布 HEX 的记录校验和及结束记录有效。记录见 `BuildLogs/integrity.log`。

日志：`BuildLogs/host_tests.log`、`BuildLogs/host_tests_app.log`。

## 仍需实体板验收

LED1 能否照亮接收端、AO 实际明暗差与响应速度、环境光干扰、实际采样抖动、真实编码器格数、四键、电源余量、串口吞吐、OLED 实物显示及 IMU 持续运行。

未实现完整消息编辑、舵机光闸标定或驱动、IMU 挑战与成绩。没有实物验收结论，不承诺接收距离、发送速度或机械行程。

完整验收顺序见 `../docs/03_烧录与验收.md`。

## 本轮修改文件

- `User/main.c`：替换旧检查主流程，接入标定、SOS、输入、串口和页面调度。
- 新增 `User/light_link.c`、`User/light_link.h`：独立收发与标定状态机。
- `Hardware/controls.c`、`Hardware/controls.h`：加入受控发射灯，移除 PA0 自动心跳；保留原按键、编码器、ADC 和舵机底层。
- `Hardware/display.c`：增加接收错误字符问号的显示。
- `Hardware/ui_text.h`、`Hardware/ui_text_list.h`、`Hardware/zh12.h`：改为本项目中文文案与字库。
- `Project.uvprojx`、`Project.uvoptx`：目标与输出名改为 `Light_Telegraph`；在 uvprojx 中登记新增核心文件，保留原设备、库和工具配置。
- 新增 `tests/test_link.c`、`tests/test_app.c`，以及 `tools/test.ps1`、`tools/build.ps1`、`tools/render_previews.py`；调整复制来的 `tools/generate_zh.ps1` 生成本项目字库。
- 项目根目录新增 `tools/generate_wiring.py` 和 `docs/接线图.svg/.png`；更新 README、接线与旧版验证说明，新增阶段 2 烧录验收说明。

其他复制的驱动、启动代码、库文件和 `memory.sct` 沿用基础工程。本轮没有修改 `defuse_box/firmware`。
