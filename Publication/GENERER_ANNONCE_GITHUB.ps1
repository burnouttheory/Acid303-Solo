# Acid303 Solo : creation locale de Publication/latest.json pour GitHub.
# Utiliser APRES la publication et le test de la Release.
$ErrorActionPreference = 'Stop'
Write-Host 'Publication des mises a jour Acid303 via GitHub'
$version = Read-Host 'Nouvelle version (ex : 0.30.3)'
if ($version -notmatch '^\d{1,5}\.\d{1,5}\.\d{1,5}$') {
    Write-Error 'Le numero doit avoir le format 0.30.3.'
    exit 1
}
$tag = 'v' + $version
$releaseUrl = 'https://github.com/burnouttheory/Acid303-Solo/releases/tag/' + $tag
Write-Host ('Verifiez que cette page existe et que le bon installateur est disponible : ' + $releaseUrl)
$confirm = Read-Host 'La Release est-elle DEJA publiee et testee ? (O/N)'
if ($confirm -ne 'O' -and $confirm -ne 'o') {
    Write-Host 'Annule. Aucun fichier modifie.'
    exit 1
}
$notes = Read-Host 'Description courte des nouveautes'
$manifest = [ordered]@{
    version = $version
    download_url = $releaseUrl
    notes = $notes
}
$target = Join-Path $PSScriptRoot 'latest.json'
$json = $manifest | ConvertTo-Json -Depth 3
[System.IO.File]::WriteAllText($target, $json + [Environment]::NewLine, [System.Text.UTF8Encoding]::new($false))
Write-Host ('Mis a jour : ' + $target)
