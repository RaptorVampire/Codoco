# =============================================================================
#  Codoco installer for Windows (PowerShell)
#  https://github.com/RaptorVampire/Codoco
#  SPDX-License-Identifier: Apache-2.0 OR MIT
#
#  Usage (from an elevated or normal PowerShell):
#    irm https://raw.githubusercontent.com/RaptorVampire/Codoco/main/install.ps1 | iex
# =============================================================================

[CmdletBinding()]
param(
    [string]$Version   = "latest",
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\Codoco",
    [switch]$Uninstall,
    [switch]$NoPath,
    [switch]$Force,
    [switch]$NoVerify
)

$ErrorActionPreference = "Stop"
$Repo = "RaptorVampire/Codoco"
$Name = "codoco"

function Info ($msg) { Write-Host "==> " -ForegroundColor Cyan -NoNewline; Write-Host $msg }
function Ok   ($msg) { Write-Host "  ok  " -ForegroundColor Green -NoNewline; Write-Host $msg }
function Warn ($msg) { Write-Host "  !!  $msg" -ForegroundColor Yellow }
function Die  ($msg) { Write-Host "error: $msg" -ForegroundColor Red; exit 1 }

# ---------------------------------------------------------------------------
#  Uninstall
# ---------------------------------------------------------------------------
if ($Uninstall) {
    Info "Uninstalling $Name"
    $target = Join-Path $InstallDir "$Name.exe"
    if (Test-Path $target) {
        Remove-Item -Force $target
        Ok "removed $target"
    }
    # remove from user PATH
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($userPath -and ($userPath -split ';' -contains $InstallDir)) {
        $new = ($userPath -split ';' | Where-Object { $_ -ne $InstallDir -and $_ -ne "" }) -join ';'
        [Environment]::SetEnvironmentVariable("Path", $new, "User")
        Ok "removed $InstallDir from user PATH"
    }
    Write-Host "`n  $Name has been removed.`n"
    exit 0
}

# ---------------------------------------------------------------------------
#  Platform detection
# ---------------------------------------------------------------------------
$arch = $env:PROCESSOR_ARCHITECTURE
if ($arch -eq "AMD64") { $Target = "windows-x86_64" }
elseif ($arch -eq "x86") {
    if ($env:PROCESSOR_ARCHITEW6432 -eq "AMD64") { $Target = "windows-x86_64" }
    else { $Target = "windows-i686" }
}
else { Die "unsupported architecture: $arch" }

Info "Platform  : Windows / $arch"
Info "Target    : $Target"

# ---------------------------------------------------------------------------
#  Resolve version
# ---------------------------------------------------------------------------
if ($Version -eq "latest") {
    Info "Resolving latest version..."
    try {
        $rel = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases/latest"
        $Version = $rel.tag_name
    } catch {
        Die "cannot resolve latest version: $_"
    }
}
$NumericVersion = $Version -replace '^v',''
Info "Version   : $Version"

# ---------------------------------------------------------------------------
#  Check existing install
# ---------------------------------------------------------------------------
$exe = Join-Path $InstallDir "$Name.exe"
if ((Test-Path $exe) -and (-not $Force)) {
    try {
        $cur = (& $exe --version 2>$null) -split ' ' | Select-Object -Last 1
        if ($cur -eq $NumericVersion) {
            Ok "$Name $Version already installed at $exe"
            Write-Host "`n  Use -Force to reinstall.`n"
            exit 0
        }
    } catch { }
}

# ---------------------------------------------------------------------------
#  Download
# ---------------------------------------------------------------------------
$artifact = "$Name-$Version-$Target.zip"
$url = "https://github.com/$Repo/releases/download/$Version/$artifact"
$tmp = Join-Path $env:TEMP ("codoco-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tmp | Out-Null

try {
    Info "Downloading $artifact"
    $zip = Join-Path $tmp $artifact
    try {
        Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing
    } catch {
        Die "download failed: $url`n$_"
    }

    # checksum
    if (-not $NoVerify) {
        try {
            $chkUrl = "https://github.com/$Repo/releases/download/$Version/checksums.txt"
            $chkFile = Join-Path $tmp "checksums.txt"
            Invoke-WebRequest -Uri $chkUrl -OutFile $chkFile -UseBasicParsing
            $line = Get-Content $chkFile | Where-Object { $_ -match "\s$([regex]::Escape($artifact))$" } | Select-Object -First 1
            if ($line) {
                $expected = ($line -split '\s+')[0].ToLower()
                $actual   = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
                if ($expected -ne $actual) { Die "checksum mismatch" }
                Ok "checksum verified"
            } else {
                Warn "no checksum entry; skipping verification"
            }
        } catch {
            Warn "verification skipped: $_"
        }
    }

    Info "Extracting"
    Expand-Archive -Path $zip -DestinationPath $tmp -Force

    $src = Get-ChildItem -Path $tmp -Recurse -Filter "$Name.exe" | Select-Object -First 1
    if (-not $src) { Die "codoco.exe not found in archive" }

    Info "Installing to $InstallDir"
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    Copy-Item -Force $src.FullName $exe
    Ok "installed: $exe"

    # ---------------------------------------------------------------------
    #  PATH
    # ---------------------------------------------------------------------
    if (-not $NoPath) {
        $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
        if (-not $userPath) { $userPath = "" }
        if (($userPath -split ';') -notcontains $InstallDir) {
            $new = ($userPath.TrimEnd(';') + ";" + $InstallDir).TrimStart(';')
            [Environment]::SetEnvironmentVariable("Path", $new, "User")
            Ok "added $InstallDir to user PATH"
            Write-Host ""
            Write-Host "  Open a new terminal to pick up PATH." -ForegroundColor Yellow
        } else {
            Ok "$InstallDir already in user PATH"
        }
    }

    # ---------------------------------------------------------------------
    #  Verify
    # ---------------------------------------------------------------------
    if (Test-Path $exe) {
        $v = & $exe --version 2>$null
        Ok "verified: $v"
    }

    Write-Host ""
    Write-Host "  Done!" -ForegroundColor Green -Bold
    Write-Host ""
    Write-Host "  Binary    $exe"
    Write-Host "  Version   $Version"
    Write-Host "  Platform  $Target"
    Write-Host ""
    Write-Host "  Try:"
    Write-Host "    codoco --help"
    Write-Host "    codoco ."
    Write-Host ""
} finally {
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}