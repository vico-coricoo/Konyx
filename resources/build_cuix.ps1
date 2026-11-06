# build_cuix.ps1
# Assemble les fichiers .cui en un fichier .cuix (format ZIP renomme).
# A lancer depuis le dossier resources/ :
#   powershell -ExecutionPolicy Bypass -File build_cuix.ps1

$sourceDir = "$PSScriptRoot\cuix"
$outputFile = "$PSScriptRoot\Konyx.cuix"

# Supprime l'ancien fichier si present
if (Test-Path $outputFile) { Remove-Item $outputFile -Force }

# Cree le zip depuis le contenu du dossier cuix/
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($sourceDir, $outputFile)

Write-Host "Konyx.cuix genere : $outputFile"
