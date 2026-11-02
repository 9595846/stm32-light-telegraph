"""Export a clean public repository and release ZIP from the maintained project."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile

ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT.parent/'github_release'/'stm32-light-telegraph'
RELEASE=ROOT.parent/'deliverables'/'github_light_telegraph'

def allowed(p):
    rel=p.relative_to(ROOT)
    if '__pycache__' in rel.parts or p.suffix.lower() in ('.pyc','.bak','.exe'):
        return False
    if rel.parts[0]=='firmware' and len(rel.parts)>1:
        if rel.parts[1] in ('Objects','Listings'):
            return False
        if rel.parts[1]=='BuildLogs' and p.suffix!='.log':
            return False
        if rel.parts[1]=='Previews' and p.suffix!='.png':
            return False
    return True

def main():
    if DEST.exists() and any(DEST.iterdir()):
        raise SystemExit('Export exists. Inspect it before updating; refusing to overwrite a published checkout.')
    DEST.mkdir(parents=True,exist_ok=True)
    for p in sorted(ROOT.rglob('*')):
        if not p.is_file() or not allowed(p):continue
        target=DEST/p.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(p,target)
    baseline=DEST/'firmware/BASELINE.json'
    data=json.loads(baseline.read_text(encoding='utf-8'))
    data['source']='defuse_box/firmware (local baseline; not needed to build this repository)'
    baseline.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    (DEST/'README.md').write_text(README,encoding='utf-8')
    (DEST/'.gitignore').write_text(IGNORE,encoding='utf-8')
    (DEST/'.gitattributes').write_text('* text=auto\n*.uvprojx text eol=crlf\n*.uvoptx text eol=crlf\n*.png binary\n*.doc binary\n*.hex text eol=lf\n',encoding='utf-8')
    (DEST/'RELEASE_NOTES.md').write_text(NOTES,encoding='utf-8')
    # Debug AXF embeds workstation paths. Public downloads contain sources and HEX;
    # Keil regenerates AXF locally when rebuilding.
    for name in ('先看这里.md','docs/03_烧录与验收.md','firmware/VALIDATION.md'):
        p=DEST/name
        p.write_text('> GitHub 公开版包含源码和 HEX，不附带本机调试 AXF。请在 Keil 中 Rebuild 后再用 AXF 下载，或使用 HEX 烧录。\n\n'+p.read_text(encoding='utf-8'),encoding='utf-8')
    RELEASE.mkdir(parents=True,exist_ok=True)
    out=RELEASE/'LightTelegraph_Stage2_V1.0.zip'
    if out.exists():raise SystemExit('Release ZIP exists; refusing to overwrite.')
    files=sorted(p for p in DEST.rglob('*') if p.is_file())
    sums=''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(DEST).as_posix()}\n' for p in files)
    with zipfile.ZipFile(out,'w',compression=zipfile.ZIP_DEFLATED) as archive:
        for p in files:archive.write(p,'LightTelegraph_Stage2_V1.0/'+p.relative_to(DEST).as_posix())
        archive.writestr('LightTelegraph_Stage2_V1.0/SHA256SUMS.txt',sums.encode('utf-8'))
    with zipfile.ZipFile(out) as archive:assert archive.testzip() is None
    digest=hashlib.sha256(out.read_bytes()).hexdigest()
    (RELEASE/'SHA256SUMS.txt').write_text(f'{digest}  {out.name}\n',encoding='utf-8')
    print(f'Prepared {len(files)} repository files; public ZIP CRC verified.')
    print('Repository staging: github_release/stm32-light-telegraph')
    print('Release assets: deliverables/github_light_telegraph')

README='''# 光影电报机 · STM32 Light Telegraph

基于 STM32F103C8T6 的近距离光通信互动项目：LED1 发出摩斯码，光敏模块读取真实明暗变化并独立解码，OLED 显示结果。遮住光路后，接收端不会从发送队列直接取得消息。

当前版本为 **V1.0 / 阶段 2**：自动明暗标定与固定 SOS 收发。Keil 实际编译 **0 错误、0 警告**，主机软件测试通过；**尚未完成实体板验收**。

## 快速开始

1. [下载完整工程 ZIP](downloads/LightTelegraph_Stage2_V1.0.zip?raw=true)，完整解压；也可以克隆本仓库。
2. 按[接线说明](docs/01_接线.md)接线。首次测试让 LED1 对准光敏元件，舵机暂不供电。
3. 用 Keil 打开 [firmware/Project.uvprojx](firmware/Project.uvprojx)，目标选择 `Light_Telegraph`，使用 ARM Compiler 5，Rebuild 后通过 ST-Link / SWD 下载。
4. 上电后按 KEY1 标定；显示“就绪”后按 KEY3 发送 SOS。KEY2 停止，KEY4 翻页，旋钮调整码元时长。
5. 查看[烧录与实物验收步骤](docs/03_烧录与验收.md)。串口使用 115200 / 8N1 / 无流控。

公开工程包提供源码、库、HEX 和文档。AXF 调试文件由 Keil 在本机重新生成。HEX 在 [firmware/release/Light_Telegraph.hex](firmware/release/Light_Telegraph.hex)。

## 已实现

- LED1 熄灭/点亮的自动采集，判断 AO 极性并生成迟滞阈值；明暗差异不足则拒绝发送。
- 非阻塞 SOS 发送，短码元为 300～1000ms，旋钮每格调整 50ms。
- 从光敏 AO 独立解码 A～Z、0～9；首版发射内容固定为 SOS。
- 中文 OLED 页面、四键操作、停止优先、状态灯和接收提示音。
- 串口诊断、ADC 超时与调度异常处理。

舵机 PWM 本阶段保持关闭。IMU 保留诊断，标定与标定有效时暂停采样以避免干扰光学时序。完整文字编辑、机械光闸与体感挑战是后续计划，尚未实现。

## 接线图

![完整接线图](docs/接线图.png)

[查看可放大的 SVG](docs/接线图.svg)。图中按引脚名称连接，模块位置不代表开发板实际排针顺序。

## 环境与硬件

Keil MDK 5.34 / uVision5、ARM Compiler 5.06 update 7 build 960、STM32F10x 标准外设库和 CMSIS；STM32F103C8 配置为 64KB Flash、20KB RAM、HSI/PLL 64MHz。

OLED 位于 PB10/PB11；IMU 位于 PB6/PB7，用户此前测得 WHO_AM_I=0x70，按兼容设备处理，未独立确认芯片型号。光敏 AO 接 PA1，编码器 PA6/PA7，四键 PB12/PB15/PB8/PB9，LED1 PA0、LED2 PB0，蜂鸣器 PB14，舵机信号 PA8，串口 PA9/PA10。

舵机使用独立 5V，并与 STM32 共地；电源正负按已确认线序判断。ST-Link 的整套供电余量、光学距离、响应速度和舵机行程都需实测。

## 串口命令

```text
HELP
STATUS
CAL
SEND SOS
STOP
UNIT 500
LAMP ON
LAMP OFF
```

## 软件验证

在 `firmware` 目录使用 PowerShell：

```powershell
& .\\tools\\test.ps1
& .\\tools\\build.ps1
```

脚本的编译器路径可用 `-Gcc`、`-Uv4` 参数调整。源码与配置 XML 保持兼容的无 BOM 编码。

测试覆盖正反 AO 极性、300/1000ms 收发、遮光不产生接收文字、外部脉冲独立解码、ADC 超时、噪声拒绝、计时回绕、输入边界和 OLED 渲染。[查看验证记录](firmware/VALIDATION.md)与 [Keil 日志](firmware/BuildLogs/Keil_Light_Telegraph.log)。

![OLED 页面预览，模拟数据，非实物照片](firmware/Previews/OLED页面.png)

## 目录

| 路径 | 内容 |
|---|---|
| `firmware/` | 唯一维护的 Keil 工程、源码、驱动库与 HEX |
| `docs/` | 接线图、接线表与实物验收步骤 |
| `firmware/tests/` | 主机软件测试 |
| `firmware/tools/` | 编译、测试和页面预览工具 |
| `RELEASE_NOTES.md` | 当前发布内容及验证边界 |

第三方 STM32 库及其原许可文件保留在 `firmware/Library/`。公开仓库不改变第三方库的许可条件。
'''

IGNORE='''# Keil build products and local state
firmware/Objects/
firmware/Listings/
*.uvguix.*
*.uvgui.*
*.bak
*.tmp
*.exe
__pycache__/
*.pyc
firmware/Previews/*.bin
.DS_Store
Thumbs.db
'''

NOTES='''# 光影电报机 V1.0 · 阶段 2

STM32F103C8T6 / Keil ARM Compiler 5，实际编译 0 错误、0 警告。

包含完整 Keil 源码工程、标准外设库与 CMSIS、预编译 HEX、中文接线图、操作与验收说明。公开包不包含带本机路径的调试 AXF；Rebuild 即可生成。

已实现自动明暗标定、固定 SOS 发送、光敏独立解码、旋钮调速、四键和 OLED 中文交互、串口诊断。舵机 PWM 保持关闭。

主机软件测试通过；尚未完成实体板验收。光学接收距离、响应速度与整套供电余量待实测。完整文字编辑、机械光闸与体感挑战尚未实现。

解压后打开 `firmware/Project.uvprojx`，选择 `Light_Telegraph`。先接好 LED1 与光敏光路，按 KEY1 标定，显示就绪后按 KEY3 发 SOS，KEY2 停止。
'''

if __name__=='__main__':main()
