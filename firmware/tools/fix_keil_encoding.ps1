param([string]$ProjectDirectory = (Split-Path $PSScriptRoot -Parent))
$ErrorActionPreference = 'Stop'
# Some localized uVision installations reject an otherwise valid XML file with UTF-8 BOM.
foreach ($name in @('Project.uvprojx','Project.uvoptx')) {
    $path = Join-Path $ProjectDirectory $name
    $bytes = [IO.File]::ReadAllBytes($path)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) {
        $backup = "$path.before_BOM_fix.bak"
        if (-not (Test-Path -LiteralPath $backup)) { Copy-Item -LiteralPath $path -Destination $backup }
        $body = [byte[]]$bytes[3..($bytes.Length-1)]
        [IO.File]::WriteAllBytes($path,$body)
        Write-Output "Removed UTF-8 BOM: $name (original backed up)"
    } else { Write-Output "No UTF-8 BOM: $name" }
}
