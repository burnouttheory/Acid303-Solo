# Acid303 Solo V3.0.0 official - IN-PLACE V2 UPGRADE build.
# Built files are staged in LocalAppData; source files stay untouched.
# Building alone does not modify the installed FL Studio plugin. Running the resulting installer upgrades v2.0.6.
$ErrorActionPreference = 'Stop'

function Ensure-Success([string]$stage) {
    if ($LASTEXITCODE -ne 0) {
        throw "$stage failed (exit code $LASTEXITCODE)."
    }
}

try {
    $root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).ProviderPath
    if (-not $env:LOCALAPPDATA) { throw 'LOCALAPPDATA is missing.' }

    # A stable, short, source-specific folder avoids collisions between extracted archives.
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($root.ToLowerInvariant())
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try { $digest = [BitConverter]::ToString($sha.ComputeHash($bytes)).Replace('-', '').Substring(0, 10) }
    finally { $sha.Dispose() }
    $build = Join-Path $env:LOCALAPPDATA ("A303V3UpgradeBuild\v300_$digest")
    $build = [IO.Path]::GetFullPath($build)

    Write-Host ('Source project: ' + $root)
    Write-Host ('Short build path: ' + $build)
    if ($build.Length -gt 145) {
        throw "Build directory is too long ($($build.Length) chars). Use a short Windows user profile path."
    }
    New-Item -ItemType Directory -Force -Path $build | Out-Null

    foreach ($program in @('cmake.exe', 'git.exe')) {
        if (-not (Get-Command $program -ErrorAction SilentlyContinue)) {
            throw "$program missing from PATH."
        }
    }

    Write-Host '[1/3] Configure CMake + JUCE 8.0.10 ...'
    & cmake.exe -S $root -B $build -G 'Visual Studio 17 2022' -A 'x64'
    Ensure-Success 'CMake configuration'

    $licensePath = Join-Path $build '_deps\juce-src\LICENSE.md'
    if (-not (Test-Path -LiteralPath $licensePath -PathType Leaf)) {
        throw "Official JUCE 8.0.10 LICENSE.md not found at $licensePath"
    }
    $licenseDir = Join-Path $root 'LICENSES'
    $null = New-Item -ItemType Directory -Path $licenseDir -Force
    Copy-Item -LiteralPath $licensePath -Destination (Join-Path $licenseDir 'JUCE_UPSTREAM_LICENSE_8_0_10.md') -Force
    $spdx = Join-Path $build '_deps\juce-src\JUCE.spdx.json'
    if (Test-Path -LiteralPath $spdx) {
        Copy-Item -LiteralPath $spdx -Destination (Join-Path $licenseDir 'JUCE_8_UPSTREAM_SBOM.spdx.json') -Force
    }

    Write-Host '[2/3] Compile VST3 Release ...'
    & cmake.exe --build $build --config 'Release' --target 'Acid303Solo_VST3' --parallel 4
    Ensure-Success 'VST3 compilation'

    $vst = Join-Path $build 'Acid303Solo_artefacts\Release\VST3\Acid303 Solo.vst3'
    if (-not (Test-Path -LiteralPath (Join-Path $vst 'Contents') -PathType Container)) {
        throw "VST3 Release bundle not found: $vst"
    }
    Write-Host "VST3 generated: $vst"

    Write-Host '[3/3] Find Inno Setup and create OFFICIEL.exe ...'
    $inno = Get-Command 'ISCC.exe' -ErrorAction SilentlyContinue
    $iscc = if ($inno) { $inno.Source } else { $null }
    if (-not $iscc) {
        $candidates = @()
        if (${env:ProgramFiles(x86)}) { $candidates += (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe') }
        if ($env:ProgramFiles) { $candidates += (Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe') }
        $candidates += (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe')
        foreach ($candidate in $candidates) {
            if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
                $iscc = $candidate
                break
            }
        }
    }
    if (-not $iscc) {
        Write-Warning 'VST3 compiled successfully but Inno Setup 6 / ISCC.exe was not found.'
        Write-Warning 'Do not publish until the official EXE has been built and checked.'
        exit 0
    }

    $payload = Join-Path $root 'Installeur\payload\Acid303 Solo.vst3'
    if (Test-Path -LiteralPath $payload) { Remove-Item -LiteralPath $payload -Recurse -Force }
    $null = New-Item -ItemType Directory -Force -Path (Split-Path $payload -Parent)
    Copy-Item -LiteralPath $vst -Destination $payload -Recurse -Force
    if (-not (Test-Path -LiteralPath (Join-Path $payload 'Contents'))) {
        throw 'Installer payload VST3 copy failed.'
    }

    $iss = Join-Path $root 'Installeur\Acid303Solo.iss'
    & $iscc $iss
    Ensure-Success 'Inno Setup compilation'

    $exe = Join-Path $root 'Installeur\Output\Acid303Solo_Setup_3.0.0_OFFICIEL.exe'
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
        throw "Installer EXE missing: $exe"
    }
    $hash = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    Write-Host "SUCCESS - installer: $exe" -ForegroundColor Green
    Write-Host "Installer SHA256: $hash"
    Write-Host 'Do not publish before validating JUCE licence conditions, third-party notices, FL Studio behaviour and project recall.'
    exit 0
}
catch {
    Write-Host ('ERROR: ' + $_.Exception.Message) -ForegroundColor Red
    Write-Host 'The existing VST3 was not modified by the failed build.' -ForegroundColor Yellow
    exit 1
}
