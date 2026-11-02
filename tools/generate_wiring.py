"""Deterministic terminal wiring diagram; no assumed physical board pin order."""
from pathlib import Path
from html import escape
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs'
W, H = 1800, 2070
im = Image.new('RGB', (W, H), '#eef3f7')
d = ImageDraw.Draw(im)
svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">',
       '<rect width="100%" height="100%" fill="#eef3f7"/>']
FONT = 'C:/Windows/Fonts/msyh.ttc'

def rect(box, fill, stroke=None, radius=12):
    d.rounded_rectangle(box, radius, fill=fill, outline=stroke, width=2)
    x1, y1, x2, y2 = box
    svg.append(f'<rect x="{x1}" y="{y1}" width="{x2-x1}" height="{y2-y1}" rx="{radius}" fill="{fill}" stroke="{stroke or fill}" stroke-width="2"/>')

def text(x, y, value, size=23, color='#163047'):
    d.text((x,y),value,font=ImageFont.truetype(FONT,size),fill=color,anchor='lt')
    svg.append(f'<text x="{x}" y="{y+size}" font-family="Microsoft YaHei, sans-serif" font-size="{size}" fill="{color}">{escape(value)}</text>')

def line(x1,y1,x2,y2,color='#21806b',width=4):
    d.line((x1,y1,x2,y2),fill=color,width=width)
    svg.append(f'<path d="M{x1} {y1} L{x2} {y2}" stroke="{color}" stroke-width="{width}" fill="none"/>')

text(45,30,'光影电报机 · 完整接线图',46)
text(47,95,'STM32F103C8T6  |  按丝印找引脚，图中位置不代表开发板排针顺序',25)
text(47,136,'绿色：信号    红色：电源    深灰：GND    每一行连接两端；所有 GND 共地',23)

# Left labels always describe the source/MCU terminal; right labels are the module terminal.
cards = [
 ('01  OLED · 四针 I²C', [('PB10','SCL'),('PB11','SDA'),('3.3V','VCC'),('GND','GND')], 'PB10/PB11 上拉先检查模块板载电路'),
 ('02  IMU · 0x70 兼容设备', [('PB6','SCL'),('PB7','SDA'),('3.3V','VCC'),('GND','GND'),('GND','AD0')], '与 OLED 分别走两条 I²C 总线'),
 ('03  光敏模块 · 接收光', [('PA1','AO'),('3.3V','VCC'),('GND','GND')], 'DO 不接；让光敏元件朝向 LED1'),
 ('04  旋转编码器', [('PA6','A / CLK'),('PA7','B / DT'),('GND','C / GND'),('3.3V','VCC（若有）')], 'SW 按压脚不接；仅裸编码器无 VCC'),
 ('05  KEY1 · 标定', [('PB12','有效触点 1'),('GND','有效触点 2')], '按下才导通，勿选始终相通的一对脚'),
 ('06  KEY2 · 停止', [('PB15','有效触点 1'),('GND','有效触点 2')], '同时按多键时，停止优先'),
 ('07  KEY3 · 发 SOS', [('PB8','有效触点 1'),('GND','有效触点 2')], '光路标定成功后才能发送'),
 ('08  KEY4 · 翻页', [('PB9','有效触点 1'),('GND','有效触点 2')], '首页 → 光敏数据 → IMU 诊断'),
 ('09  MH-FMD 蜂鸣器', [('PB14','I/O'),('3.3V','VCC'),('GND','GND')], '低电平触发；接收新字符时短响'),
 ('10  LED1 · 发射灯', [('PA0','1kΩ → LED 阳极'),('GND','LED 阴极')], '每颗灯独立限流；不要直连 GPIO'),
 ('11  LED2 · 状态灯', [('PB0','1kΩ → LED 阳极'),('GND','LED 阴极')], '10kΩ 可试但可能暗；别照到接收端'),
 ('12  SG90 · 本阶段 PWM 关闭', [('PA8','信号'),('独立 5V +','舵机电源正极'),('独立电源 GND','舵机负极 / 共地')], '线序以已调通接法为准；首次可断电'),
 ('13  USB 转串口 · 3.3V 逻辑', [('PA9 / TX','RXD'),('PA10 / RX','TXD'),('GND','GND')], 'VCC 不接；115200 / 8N1 / 无流控'),
 ('14  ST-Link · 专用 SWD 口', [('板上 SWDIO','SWDIO'),('板上 SWCLK','SWCLK'),('板上 GND','GND'),('板上 3.3V','3.3V 输出')], '沿用已确认供电；总负载余量待测'),
 ('15  IMU 总线上拉 · 已安装', [('3.3V','10kΩ → PB6'),('3.3V','10kΩ → PB7')], '两只分别连接，不能把 PB6/PB7 短接'),
]
for index,(title,rows,note) in enumerate(cards):
    x=45+(index%3)*575;y=195+(index//3)*280
    rect((x,y,x+550,y+260),'#ffffff','#d5dfe7')
    text(x+18,y+14,title,25)
    for j,(left,right) in enumerate(rows):
        yy=y+55+j*33
        color='#485866' if 'GND' in left else '#c64a3f' if '3.3V' in left or '5V' in left else '#21806b'
        text(x+18,yy,left,21,color)
        line(x+205,yy+13,x+265,yy+13,color,3)
        text(x+280,yy,right,21,color)
    text(x+18,y+225,note,19,'#586c7b')

rect((45,1610,1755,1820),'#163047')
text(70,1630,'供电与第一次验证',29,'#ffffff')
text(70,1680,'① 舵机使用独立 5V，独立电源 GND 与 STM32 GND 相连。独立 5V 不接主控 3.3V。',25,'#ffffff')
text(70,1721,'② 第一次只验证光路，舵机先断电；ST-Link 带整套模块的供电能力尚未实测。',25,'#ffffff')
text(70,1762,'③ 1kΩ 优先给发射 LED1。光敏 VCC 用 3.3V，AO 接 PA1，DO 留空。',25,'#ffffff')

text(55,1850,'光路（非电线）',28)
rect((65,1900,430,1973),'#ffffff','#d5dfe7');text(90,1919,'LED1 发光面',27)
line(445,1937,1280,1937,'#c89827',7)
rect((710,1890,1060,1982),'#ffffff','#d5dfe7');text(735,1903,'轻纸片遮光闸',25);text(735,1940,'后续由舵机带动',21)
rect((1300,1900,1735,1973),'#ffffff','#d5dfe7');text(1325,1919,'光敏元件 · 接收面',27)
text(55,2010,'先近距离对准并加纸筒遮杂光；光路可行性、阈值、速率和机械行程均以实测为准。',23)

OUT.mkdir(exist_ok=True)
(OUT/'接线图.svg').write_text('\n'.join(svg+['</svg>']),encoding='utf-8')
im.save(OUT/'接线图.png')
print('Generated wiring SVG and PNG from the same pin mapping.')
