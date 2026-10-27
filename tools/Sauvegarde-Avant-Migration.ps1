$ErrorActionPreference='Stop'
try {
  $folder=Join-Path ([Environment]::GetFolderPath('MyDocuments')) ('Acid303_Backup_Avant_V3_' + (Get-Date -Format 'yyyyMMdd_HHmmss'))
  New-Item -ItemType Directory -Path $folder -Force | Out-Null
  $vst=Join-Path $env:CommonProgramFiles 'VST3\Acid303 Solo.vst3'
  if(Test-Path -LiteralPath $vst -PathType Container) {
    Copy-Item -LiteralPath $vst -Destination (Join-Path $folder 'Acid303 Solo.vst3') -Recurse -Force
  }
  $personal=Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Acid303'
  if(Test-Path -LiteralPath $personal -PathType Container) {
    Copy-Item -LiteralPath $personal -Destination (Join-Path $folder 'Acid303_Documents') -Recurse -Force
  }
  $appdata=Join-Path $env:ProgramData 'Acid303Solo\FactoryContent'
  if(Test-Path -LiteralPath $appdata -PathType Container) {
    Copy-Item -LiteralPath $appdata -Destination (Join-Path $folder 'FactoryContent_V2') -Recurse -Force
  }
  Write-Host ('SAUVEGARDE REUSSIE : '+$folder) -ForegroundColor Green
  exit 0
} catch { Write-Host ('ECHEC DE SAUVEGARDE : '+ $_.Exception.Message) -ForegroundColor Red; exit 1 }
