param(
    [Parameter(Mandatory=$true)][string]$Version,
    [Parameter(Mandatory=$true)][string]$OneDriveLink,
    [string]$Notes = 'Nouvelle version Acid303 Solo disponible.'
)
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Version invalide. Exemple : 0.31.0' }
if ($OneDriveLink -notmatch '^https://') { throw 'Le lien OneDrive doit commencer par https://' }
$data = [ordered]@{ version=$Version; download_url=$OneDriveLink; notes=$Notes }
$json = $data | ConvertTo-Json -Depth 5
$target = Join-Path $PSScriptRoot 'latest.json'
[System.IO.File]::WriteAllText($target, $json + "`n", (New-Object System.Text.UTF8Encoding($false)))
Write-Host 'Fichier pret :' $target
Write-Host 'ATTENTION : publier latest.json en ligne seulement APRES avoir copie et teste le nouvel EXE sur OneDrive.' -ForegroundColor Yellow
