$ErrorActionPreference='Stop'
try {
  $new=Join-Path $env:CommonProgramFiles 'VST3\Acid303 Solo.vst3'
  $old=Join-Path $env:CommonProgramFiles 'VST3\Acid303 Solo V3.vst3'
  if(!(Test-Path $new)) { throw 'Le nouveau plugin Acid303 Solo.vst3 n est pas installe. Ne rien effacer.' }
  if(!(Test-Path $old)) { Write-Host 'Aucun plugin V3 TEST restant.'; exit 0 }
  Write-Host 'ATTENTION: les projets FLP creees avec la V3 TEST (BuTh/A3V3) pourraient ne plus charger.' -ForegroundColor Yellow
  Write-Host 'Ne continuez que si vos projets ont ete migres et FL Studio est ferme.'
  $agree=Read-Host 'Pour sauvegarder et retirer le VST3 TEST, taper EXACTEMENT SUPPRIMER'
  if ($agree -cne 'SUPPRIMER') { Write-Host 'Operation annulee.'; exit 0 }
  $folder=Join-Path ([Environment]::GetFolderPath('MyDocuments')) ('Acid303V3_TEST_BACKUP_' +(Get-Date -Format 'yyyyMMdd_HHmmss'))
  New-Item -ItemType Directory -Path $folder -Force | Out-Null
  Copy-Item -LiteralPath $old -Destination (Join-Path $folder 'Acid303 Solo V3.vst3') -Recurse -Force
  # Only remove after a full verified backup. Do not touch user data in Acid303V3.
  Remove-Item -LiteralPath $old -Recurse -Force
  Write-Host ('ANCIEN V3 TEST sauvegarde et retire : '+$folder) -ForegroundColor Green
  Write-Host 'Relancer le scanner VST3 de FL Studio.'
  exit 0
} catch { Write-Host ('ERREUR : '+$_.Exception.Message) -ForegroundColor Red; exit 1 }
