$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Add-Type -AssemblyName System.Drawing
$texts=[ordered]@{
 TITLE='光影电报机'; CAL='标定中保持光路畅通'; LAMP='手动亮灯接收暂停'
 TX='正在发送'; READY='就绪'; NEED_CAL='请先按1标定'
 BUTTONS='1标定 2停 3发 4页'; LIGHT='光敏标定数据'; IMU='姿态诊断非实时'; SERVO_OFF='本阶段舵机关闭'
}
$utf8=[Text.UTF8Encoding]::new($false)
$ui=[Collections.Generic.List[string]]::new();$ui.Add('#ifndef MAZE_UI_TEXT_H');$ui.Add('#define MAZE_UI_TEXT_H')
$chars=[Collections.Generic.SortedSet[char]]::new()
foreach($name in $texts.Keys){
 $bytes=[Text.Encoding]::UTF8.GetBytes($texts[$name]);$literal=($bytes|ForEach-Object {'\x{0:X2}' -f $_}) -join ''
 $ui.Add('#define UI_'+$name+' "'+$literal+'"')
 foreach($ch in $texts[$name].ToCharArray()){if([int]$ch -gt 127){[void]$chars.Add($ch)}}
}
$ui.Add('#endif');[IO.File]::WriteAllLines((Join-Path $root 'Hardware\ui_text.h'),$ui,$utf8)
$list=@('#include "ui_text.h"','static const char *const ui_strings[] = {')+@($texts.Keys | ForEach-Object {'UI_'+$_+','})+@('};')
[IO.File]::WriteAllLines((Join-Path $root 'Hardware\ui_text_list.h'),$list,$utf8)
$lines=[Collections.Generic.List[string]]::new();$lines.Add('#ifndef MAZE_ZH12_H');$lines.Add('#define MAZE_ZH12_H');$lines.Add('#include <stdint.h>')
$lines.Add('typedef struct {uint16_t code;uint16_t rows[12];} Zh12;');$lines.Add('static const Zh12 zh12[] = {')
$font=[Drawing.Font]::new('SimHei',12,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
try {foreach($ch in $chars){
 $b=[Drawing.Bitmap]::new(32,32);$g=[Drawing.Graphics]::FromImage($b)
 try{$g.Clear([Drawing.Color]::Black);$g.TextRenderingHint=[Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
  $g.DrawString([string]$ch,$font,[Drawing.Brushes]::White,[Drawing.PointF]::new(4,4),[Drawing.StringFormat]::GenericTypographic)
  $left=32;$top=32;$right=-1;$bottom=-1
  for($y=0;$y -lt 32;$y++){for($x=0;$x -lt 32;$x++){if($b.GetPixel($x,$y).R -ge 128){$left=[Math]::Min($left,$x);$right=[Math]::Max($right,$x);$top=[Math]::Min($top,$y);$bottom=[Math]::Max($bottom,$y)}}}
  if($right -lt $left){throw "Empty glyph: $ch"}
  $width=$right-$left+1;$height=$bottom-$top+1;$dw=[Math]::Min(12,$width);$dh=[Math]::Min(12,$height)
  $ox=[int][Math]::Floor((12-$dw)/2);$oy=[int][Math]::Floor((12-$dh)/2)
  $rows=@();for($y=0;$y -lt 12;$y++){$bits=0;for($x=0;$x -lt 12;$x++){
   if($x -ge $ox -and $x -lt $ox+$dw -and $y -ge $oy -and $y -lt $oy+$dh){
    $sx=$left+[int][Math]::Floor(($x-$ox)*$width/$dw);$sy=$top+[int][Math]::Floor(($y-$oy)*$height/$dh)
    if($b.GetPixel($sx,$sy).R -ge 128){$bits=$bits -bor (0x8000 -shr $x)}
   }
  };$rows+=('0x{0:X4}u' -f $bits)}
  $lines.Add(' {0x'+('{0:X4}' -f [int]$ch)+'u,{'+($rows -join ',')+'}},')
 }finally{$g.Dispose();$b.Dispose()}
}}finally{$font.Dispose()}
$lines.Add('};');$lines.Add('#endif');[IO.File]::WriteAllLines((Join-Path $root 'Hardware\zh12.h'),$lines,$utf8)
"Generated $($chars.Count) Chinese glyphs and $($texts.Count) UI strings"
