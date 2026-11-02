param([string]$Uv4='E:\keil5\UV4\UV4.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
& (Join-Path $PSScriptRoot 'fix_keil_encoding.ps1') -ProjectDirectory $root
foreach($dir in @('BuildLogs','release','Objects','Listings')){New-Item -ItemType Directory -Force -Path (Join-Path $root $dir) | Out-Null}
$p=Start-Process -FilePath $Uv4 -ArgumentList '-r Project.uvprojx -t Light_Telegraph -j0 -o BuildLogs\Keil_Light_Telegraph.log' -WorkingDirectory $root -WindowStyle Hidden -Wait -PassThru
$log=Join-Path $root 'BuildLogs\Keil_Light_Telegraph.log'
Get-Content -LiteralPath $log
if($p.ExitCode -ne 0){throw "Keil build failed: $($p.ExitCode)"}
if(-not ((Get-Content -Raw -LiteralPath $log) -match '0 Error\(s\), 0 Warning\(s\)')){throw 'Expected a zero-error, zero-warning build'}
Copy-Item -LiteralPath (Join-Path $root 'Objects\Light_Telegraph\Light_Telegraph.hex') -Destination (Join-Path $root 'release\Light_Telegraph.hex')
