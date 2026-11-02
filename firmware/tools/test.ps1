param([string]$Gcc='C:\mingw64\bin\gcc.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    New-Item -ItemType Directory -Force -Path BuildLogs,Previews | Out-Null
    & $Gcc -std=c99 -Wall -Wextra -Werror -IUser tests/test_link.c User/light_link.c -o BuildLogs/test_link.exe
    if($LASTEXITCODE -ne 0){throw 'Core host compile failed'}
    & .\BuildLogs\test_link.exe | Tee-Object -FilePath BuildLogs/host_tests.log
    if($LASTEXITCODE -ne 0){throw 'Core tests failed'}
    & $Gcc -std=c99 -Wall -Wextra -Werror -DMAZE_HOST_VIEW -IUser -IHardware tests/test_app.c User/light_link.c Hardware/display.c -o BuildLogs/test_app.exe
    if($LASTEXITCODE -ne 0){throw 'App host compile failed'}
    & .\BuildLogs\test_app.exe | Tee-Object -FilePath BuildLogs/host_tests_app.log
    if($LASTEXITCODE -ne 0){throw 'App tests failed'}
} finally {Pop-Location}
