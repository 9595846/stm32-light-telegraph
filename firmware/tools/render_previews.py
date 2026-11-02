from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parents[1]
image=Image.new('RGB',(1100,1045),'#eef3f7');draw=ImageDraw.Draw(image)
font=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',23)
names=['停止后的首页','等待标定','正在标定','发送与接收示例','光敏数据边界检查','IMU 诊断（不实时刷新）']
for i,name in enumerate(names):
    data=(root/f'Previews/page{i+1}.bin').read_bytes()
    frame=Image.new('RGB',(128,64),'black')
    for y in range(64):
        for x in range(128):
            if data[(y//8)*128+x] & (1<<(y%8)):frame.putpixel((x,y),(255,255,255))
    x=25+(i%2)*550;y=20+(i//2)*345
    draw.text((x,y),name,font=font,fill='#163047')
    image.paste(frame.resize((512,256),Image.Resampling.NEAREST),(x,y+45))
image.save(root/'Previews/OLED页面.png')
print('Rendered actual OLED framebuffers with simulated inputs, not hardware photos.')
